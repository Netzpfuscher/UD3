#!/usr/bin/env python3
"""
MIN-over-UDP integration tests for the UD3 simulator.

The simulator exposes its MIN UART on UDP port 1337.  This script implements
the MIN v2.0 wire format (COBS-style 0xAA stuffing, CRC32, optional transport
sequence numbers and ACKs) and exercises both the protocol core and the UD3
application handlers.

Usage:
    # Simulator already running:
    python3 simulator/tests/test_min_udp.py

    # Start a private simulator instance automatically:
    python3 simulator/tests/test_min_udp.py --start-sim

    # Interactive CLI shell over MIN:
    python3 simulator/tests/test_min_udp.py --start-sim --shell

    # Different endpoint / binary:
    python3 simulator/tests/test_min_udp.py --host 127.0.0.1 --port 1337 \
        --start-sim --sim simulator/build/ud3sim

Exit status is 0 when every test passes, 1 otherwise.
"""

from __future__ import annotations

import argparse
import os
import socket
import struct
import subprocess
import sys
import tempfile
import time
import traceback
import zlib
from dataclasses import dataclass
from pathlib import Path
from typing import Callable, Iterable, Optional

# ---------------------------------------------------------------------------
# MIN protocol constants (mirror common/min/min_id.h and min.c)
# ---------------------------------------------------------------------------

HEADER_BYTE = 0xAA
STUFF_BYTE = 0x55
EOF_BYTE = 0x55
ID_TRANSPORT_BIT = 0x80
ID_MASK = 0x3F
ACK = 0xFF
RESET = 0xFE

MIN_ID_TERM = 0
MIN_ID_WD = 10
MIN_ID_COMMAND = 12
MIN_ID_SOCKET = 13
MIN_ID_SYNTH = 14
MIN_ID_FEATURE = 15
MIN_ID_MIDI = 20
MIN_ID_SID = 21
MIN_ID_EVENT = 40
MIN_ID_ALARM = 41
MIN_ID_DEBUG = 42
MIN_ID_VMS = 43

EVENT_GET_INFO = 1
CMD_LINK = 3
SOCKET_DISCONNECTED = 0
SOCKET_CONNECTED = 1

VMS_WRT_BLOCK = 1
VMS_WRT_MAP_HEADER = 2
VMS_WRT_MAP_DATA = 3
VMS_WRT_FLUSH = 4

# Sizes of the wire structures in tsk_min.c (verified with sizeof()).
VMS_WIRE_SIZE = 44
MAP_HEADER_WIRE_SIZE = 28
MAP_ENTRY_WIRE_SIZE = 8

# A frame with a payload of p bytes is at most 14 + p bytes on the wire.
ON_WIRE_OVERHEAD = 14
MAX_PAYLOAD = 255

# How long a single request/reply exchange may take.
DEFAULT_TIMEOUT = 1.5


# ---------------------------------------------------------------------------
# Frame encoding / decoding
# ---------------------------------------------------------------------------

@dataclass
class MinFrame:
    """A decoded MIN frame."""

    id_control: int
    seq: int
    payload: bytes

    @property
    def app_id(self) -> int:
        return self.id_control & ID_MASK

    @property
    def transport(self) -> bool:
        return bool(self.id_control & ID_TRANSPORT_BIT)

    def __repr__(self) -> str:
        return (
            f"MinFrame(id=0x{self.id_control:02x}, app_id={self.app_id}, "
            f"transport={self.transport}, seq={self.seq}, "
            f"payload={self.payload[:24]!r}{'...' if len(self.payload) > 24 else ''})"
        )


def _stuff(body: bytes) -> bytes:
    """Apply MIN byte stuffing to the body after the three header bytes."""
    out = bytearray((HEADER_BYTE, HEADER_BYTE, HEADER_BYTE))
    countdown = 2
    for byte in body:
        out.append(byte)
        if byte == HEADER_BYTE:
            countdown -= 1
            if countdown == 0:
                out.append(STUFF_BYTE)
                countdown = 2
        else:
            countdown = 2
    out.append(EOF_BYTE)
    return bytes(out)


def encode_frame(
    id_control: int,
    payload: bytes = b"",
    seq: int = 0,
    *,
    crc_xor: int = 0,
) -> bytes:
    """Encode a complete MIN frame, including header, CRC and stuffing."""
    if len(payload) > MAX_PAYLOAD:
        raise ValueError(f"payload too long: {len(payload)} > {MAX_PAYLOAD}")
    if not 0 <= id_control <= 0xFF:
        raise ValueError("id_control out of range")

    body = bytearray((id_control,))
    if id_control & ID_TRANSPORT_BIT:
        body += (seq & 0xFFFFFFFF).to_bytes(4, "big")
    body.append(len(payload))
    body += payload

    crc = (zlib.crc32(body) & 0xFFFFFFFF) ^ (crc_xor & 0xFFFFFFFF)
    body += crc.to_bytes(4, "big")
    return _stuff(bytes(body))


def parse_frames(data: bytes) -> list[MinFrame]:
    """Decode zero or more MIN frames from one simulator UDP datagram.

    The simulator sends one complete MIN frame per datagram, but the parser is
    written as the same byte-at-a-time state machine used by min.c so it also
    copes with several back-to-back frames in one datagram.
    """
    frames: list[MinFrame] = []
    state = "search"
    header_seen = 0

    id_control = 0
    seq = 0
    length = 0
    payload = bytearray()
    crc_data = bytearray()
    crc_bytes = bytearray()
    checksum = 0

    for byte in data:
        # Three header bytes are followed by a header-context byte.  This is
        # the same special case as rx_handle_header_context() in min.c.
        if header_seen == 2:
            header_seen = 0
            if byte == HEADER_BYTE:
                state = "id"
            elif byte != STUFF_BYTE:
                state = "search"
            continue

        if byte == HEADER_BYTE:
            header_seen += 1
        else:
            header_seen = 0

        if state == "search":
            continue

        if state == "id":
            id_control = byte
            seq = 0
            payload = bytearray()
            crc_data = bytearray((byte,))
            if id_control & ID_TRANSPORT_BIT:
                state = "seq3"
            else:
                state = "length"

        elif state == "seq3":
            seq = byte << 24
            crc_data.append(byte)
            state = "seq2"
        elif state == "seq2":
            seq |= byte << 16
            crc_data.append(byte)
            state = "seq1"
        elif state == "seq1":
            seq |= byte << 8
            crc_data.append(byte)
            state = "seq0"
        elif state == "seq0":
            seq |= byte
            crc_data.append(byte)
            state = "length"

        elif state == "length":
            length = byte
            crc_data.append(byte)
            if length == 0:
                state = "crc3"
            elif length <= MAX_PAYLOAD:
                payload = bytearray()
                state = "payload"
            else:
                state = "search"

        elif state == "payload":
            payload.append(byte)
            crc_data.append(byte)
            length -= 1
            if length == 0:
                state = "crc3"

        elif state == "crc3":
            crc_bytes = bytearray((byte,))
            state = "crc2"
        elif state == "crc2":
            crc_bytes.append(byte)
            state = "crc1"
        elif state == "crc1":
            crc_bytes.append(byte)
            state = "crc0"
        elif state == "crc0":
            crc_bytes.append(byte)
            checksum = int.from_bytes(crc_bytes, "big")
            if checksum == (zlib.crc32(crc_data) & 0xFFFFFFFF):
                state = "eof"
            else:
                state = "search"

        elif state == "eof":
            if byte == EOF_BYTE:
                frames.append(MinFrame(id_control, seq, bytes(payload)))
            state = "search"

    return frames


# ---------------------------------------------------------------------------
# MIN UDP client with transport ACK handling
# ---------------------------------------------------------------------------

class MinUdpClient:
    """Small MIN host implementation for the simulator's UDP endpoint."""

    def __init__(self, host: str, port: int, verbose: bool = False):
        self.host = host
        self.port = port
        self.verbose = verbose
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock.bind(("", 0))
        self.sock.settimeout(0.1)
        self.tx_seq = 0
        self.rx_expected = 0
        self.received: list[MinFrame] = []
        self.resets = 0

    def close(self) -> None:
        try:
            self.sock.close()
        except OSError:
            pass

    def _log(self, message: str) -> None:
        if self.verbose:
            print(f"    [min] {message}")

    def send_control(
        self,
        id_control: int,
        payload: bytes = b"",
        seq: int = 0,
        *,
        crc_xor: int = 0,
    ) -> None:
        frame = encode_frame(id_control, payload, seq, crc_xor=crc_xor)
        self._log(
            f"TX id=0x{id_control:02x} seq={seq} len={len(payload)} "
            f"wire={len(frame)}"
        )
        self.sock.sendto(frame, (self.host, self.port))

    def send_app(
        self,
        app_id: int,
        payload: bytes = b"",
        transport: bool = False,
    ) -> int:
        """Send an application frame; returns the used transport sequence."""
        if transport:
            seq = self.tx_seq
            self.tx_seq = (self.tx_seq + 1) & 0xFFFFFFFF
            self.send_control(app_id | ID_TRANSPORT_BIT, payload, seq)
            return seq
        self.send_control(app_id & ID_MASK, payload)
        return 0

    def _ack(self, frame: MinFrame) -> None:
        # Acknowledge the next expected sequence number.  Re-ACK duplicates
        # with the same value so a retransmitted frame is retired.
        ack_seq = max(self.rx_expected, (frame.seq + 1) & 0xFFFFFFFF)
        self.rx_expected = ack_seq
        self.send_control(
            ACK,
            struct.pack(">II", 0, 4096),
            ack_seq,
        )

    def recv(self, timeout: float = DEFAULT_TIMEOUT) -> list[MinFrame]:
        """Receive and decode frames, ACKing transport frames automatically."""
        frames: list[MinFrame] = []
        deadline = time.monotonic() + timeout
        while True:
            remaining = deadline - time.monotonic()
            if remaining <= 0:
                break
            self.sock.settimeout(min(remaining, 0.05))
            try:
                data, _ = self.sock.recvfrom(8192)
            except socket.timeout:
                continue
            except ConnectionResetError:
                # ICMP port-unreachable from a simulator that is not running.
                break

            for frame in parse_frames(data):
                self.received.append(frame)
                if frame.id_control == RESET:
                    self.resets += 1
                    self.rx_expected = 0
                    self.tx_seq = 0
                elif frame.id_control != ACK:
                    if frame.transport:
                        self._ack(frame)
                frames.append(frame)
        return frames

    def wait_for(
        self,
        predicate: Callable[[MinFrame], bool],
        timeout: float = DEFAULT_TIMEOUT,
    ) -> Optional[MinFrame]:
        deadline = time.monotonic() + timeout
        while True:
            remaining = deadline - time.monotonic()
            if remaining <= 0:
                return None
            for frame in self.recv(remaining):
                if predicate(frame):
                    return frame

    def drain(self, timeout: float = 0.2) -> None:
        self.recv(timeout)

    # -- application helpers ------------------------------------------------

    def connect_socket(self, index: int = 0, info: bytes = b"pytest") -> None:
        self.send_app(MIN_ID_SOCKET, bytes((index, SOCKET_CONNECTED)) + info)
        self.drain(0.35)

    def disconnect_socket(self, index: int = 0) -> None:
        self.send_app(MIN_ID_SOCKET, bytes((index, SOCKET_DISCONNECTED)))
        self.drain(0.2)

    def event_info(self, timeout: float = DEFAULT_TIMEOUT) -> Optional[MinFrame]:
        self.send_app(MIN_ID_EVENT, bytes((EVENT_GET_INFO,)))
        return self.wait_for(lambda f: f.app_id == MIN_ID_EVENT, timeout)

    def cli(self, command: bytes, timeout: float = 2.0) -> bytes:
        """Send bytes to terminal socket 0 and collect its text response."""
        if isinstance(command, str):
            command = command.encode()
        self.send_app(MIN_ID_TERM, command)
        text = bytearray()
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            for frame in self.recv(0.2):
                if frame.app_id == MIN_ID_TERM:
                    text += frame.payload
        return bytes(text)

    def resync(self) -> None:
        """Send a MIN RESET and return both sequence counters to zero."""
        self.send_control(RESET, b"", 0)
        self.recv(0.15)
        self.tx_seq = 0
        self.rx_expected = 0

    def probe(self, timeout: float = 1.5) -> bool:
        """Verify the MIN task is alive by sending a transport app frame.

        The device ACKs every accepted transport app frame.  A missing ACK
        means the MIN task is hung, the sequence is desynchronised, or the
        simulator died.
        """
        self.recv(0.02)
        for _ in range(2):
            seq = self.send_app(MIN_ID_COMMAND, bytes((CMD_LINK, 1)), transport=True)
            ack = self.wait_for(
                lambda f: f.id_control == ACK and f.seq == ((seq + 1) & 0xFFFFFFFF),
                timeout,
            )
            if ack is not None:
                return True
            self.resync()
        return False


# ---------------------------------------------------------------------------
# Test payload builders
# ---------------------------------------------------------------------------

def vms_block(block_id: int, fill: int = 0x11) -> bytes:
    """VMS_WRT_BLOCK packet: type + VMS_WIRE_DATA_t (id + 38-byte block)."""
    return (
        bytes((VMS_WRT_BLOCK,))
        + struct.pack("<I", block_id & 0xFFFFFFFF)
        + bytes((fill,)) * 38
        + b"\x00\x00"  # struct padding to VMS_WIRE_SIZE
    )


def vms_map_header(entries: int, name: bytes = b"TEST") -> bytes:
    """VMS_WRT_MAP_HEADER packet: type + MAP_HEADER_WIRE_DATA_t."""
    name = name[:18].ljust(18, b"\x00")
    return (
        bytes((VMS_WRT_MAP_HEADER,))
        + struct.pack("<I", 0)          # wire id (unused by the handler)
        + bytes((entries & 0xFF, 0, 127))
        + name
        + b"\x00\x00\x00"               # struct padding to MAP_HEADER_WIRE_SIZE
    )


def vms_map_entry(start: int, end: int, freq: int = 0, ot: int = 0,
                  flags: int = 0, block: int = 0) -> bytes:
    """VMS_WRT_MAP_DATA packet: type + MAPTABLE_ENTRY_t."""
    return (
        bytes((VMS_WRT_MAP_DATA,))
        + struct.pack("<BBhBBH", start & 0xFF, end & 0xFF, freq, ot & 0xFF,
                      flags & 0xFF, block & 0xFFFF)
    )


def vms_flush() -> bytes:
    return bytes((VMS_WRT_FLUSH,))


# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------

TESTS: list[tuple[str, Callable[[MinUdpClient], None]]] = []


def test(name: str):
    def register(func: Callable[[MinUdpClient], None]):
        TESTS.append((name, func))
        return func
    return register


def check(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


@test("EVENT_GET_INFO request/reply and CRC rejection")
def test_event_info_and_crc(client: MinUdpClient) -> None:
    frame = client.event_info()
    check(frame is not None, "no EVENT_GET_INFO response")
    check(len(frame.payload) >= 2, f"event response too short: {len(frame.payload)}")
    check(frame.payload[0] == EVENT_GET_INFO, "wrong event id in response")

    # A frame with a corrupted CRC must be discarded, and the receiver must
    # resynchronise for the next valid frame.
    before = len(client.received)
    client.send_control(MIN_ID_EVENT, bytes((EVENT_GET_INFO,)), crc_xor=1)
    client.recv(0.3)
    check(
        not any(f.app_id == MIN_ID_EVENT for f in client.received[before:]),
        "corrupted frame was accepted",
    )
    check(client.event_info() is not None, "receiver did not resync after bad CRC")


@test("Transport sequence numbers and ACK")
def test_transport_ack(client: MinUdpClient) -> None:
    client.resync()
    seq = client.send_app(MIN_ID_COMMAND, bytes((CMD_LINK, 1)), transport=True)
    ack = client.wait_for(
        lambda f: f.id_control == ACK and f.seq == ((seq + 1) & 0xFFFFFFFF),
        DEFAULT_TIMEOUT,
    )
    check(ack is not None, "no ACK for transport app frame")
    check(client.probe(), "MIN task unresponsive after transport ACK test")


@test("CLI over MIN transport")
def test_cli_help(client: MinUdpClient) -> None:
    client.connect_socket()
    text = client.cli(b"help\r", 2.5)
    check(b"TTerm" in text or b"Commands available" in text,
          f"help output missing; got {text[:120]!r}")

    text = client.cli(b"get ud_name\r", 1.5)
    check(b"ud_name" in text, f"get ud_name output missing; got {text[:120]!r}")


@test("Short and malformed application frames do not kill MIN")
def test_malformed_frames(client: MinUdpClient) -> None:
    client.connect_socket()

    # Each of these was either accepted too eagerly or dereferenced stale
    # payload bytes before the validation fixes.
    cases = [
        (MIN_ID_ALARM, b"\x03"),              # len < 5
        (MIN_ID_EVENT, b""),                  # len == 0
        (MIN_ID_SOCKET, b""),                 # len == 0
        (MIN_ID_SOCKET, b"\x00"),             # len == 1
        (MIN_ID_SYNTH, b""),                  # len == 0
        (MIN_ID_VMS, b""),                    # len == 0
        (MIN_ID_VMS, b"\x01"),                # block type without block
        (MIN_ID_VMS, vms_map_entry(0, 127)),   # entry without a header
        (MIN_ID_SID, b""),                    # len == 0
        (MIN_ID_SID, b"\x00\x00\xc8\x00\x01"),# channel 200
        (MIN_ID_SOCKET, bytes((0, SOCKET_CONNECTED)) + b"A" * 64),
        (MIN_ID_WD, b""),                     # zero-length watchdog frame
        (MIN_ID_ALARM, b"\x01" + b"\x00" * 4 + b"PYTEST"),
    ]
    for mid, payload in cases:
        client.send_control(mid, payload)
        time.sleep(0.02)

    client.recv(0.1)
    check(client.probe(), "MIN task unresponsive after malformed frames")
    text = client.cli(b"help\r", 2.0)
    check(b"TTerm" in text or b"Commands available" in text,
          "CLI did not recover after malformed frames")


@test("Malformed ACK does not hang retransmission")
def test_malformed_ack(client: MinUdpClient) -> None:
    client.resync()
    # num_nacked = 0xFFFFFFFF used to make the uint8 loop wrap forever.
    client.send_control(
        ACK,
        struct.pack(">II", 0xFFFFFFFF, 4096),
        0,
    )
    time.sleep(0.05)
    client.recv(0.1)
    check(client.probe(), "MIN task hung after malformed ACK")


@test("Byte stuffing with a header-heavy payload")
def test_stuffing(client: MinUdpClient) -> None:
    client.connect_socket()
    # MIN_ID_DEBUG is ignored by the application, so only framing is tested.
    client.send_control(MIN_ID_DEBUG, bytes((HEADER_BYTE,)) * 200)
    client.send_control(MIN_ID_DEBUG, bytes((HEADER_BYTE,)) * 255)
    client.drain(0.1)
    check(client.event_info() is not None,
          "frame after a 0xAA-heavy payload was not decoded")
    check(client.probe(), "MIN task unresponsive after stuffing test")


@test("VMS block, map upload, and flush")
def test_vms_upload(client: MinUdpClient) -> None:
    client.connect_socket()

    # Valid block 0 and an out-of-range block id.
    client.send_control(MIN_ID_VMS, vms_block(0))
    client.send_control(MIN_ID_VMS, vms_block(0xFFFFFFFF))
    client.send_control(MIN_ID_VMS, vms_flush())
    client.drain(0.15)

    # Zero-entry map header (written immediately by the handler).
    client.send_control(MIN_ID_VMS, vms_map_header(0, b"ZERO"))
    client.send_control(MIN_ID_VMS, vms_flush())
    client.drain(0.15)

    # Two-entry map: header followed by two entry packets and a flush.
    client.send_control(MIN_ID_VMS, vms_map_header(2, b"TWO"))
    client.send_control(MIN_ID_VMS, vms_map_entry(0, 60, block=0))
    client.send_control(MIN_ID_VMS, vms_map_entry(61, 127, block=0))
    client.send_control(MIN_ID_VMS, vms_flush())
    client.drain(0.15)

    check(client.probe(), "MIN task unresponsive after VMS upload")
    text = client.cli(b"help\r", 2.0)
    check(b"TTerm" in text or b"Commands available" in text,
          "CLI did not recover after VMS upload")


@test("Abandoned VMS upload is cancelled on disconnect")
def test_vms_abandoned(client: MinUdpClient) -> None:
    client.connect_socket()
    # Allocate a two-entry map but send no entries and never flush.
    client.send_control(MIN_ID_VMS, vms_map_header(2, b"DROP"))
    client.drain(0.1)
    client.disconnect_socket()

    # The transport must still work without a connected terminal socket.
    check(client.probe(), "MIN task unresponsive after abandoned upload")

    # Reconnect and perform a fresh upload; stale state must not break it.
    client.connect_socket()
    client.send_control(MIN_ID_VMS, vms_map_header(0, b"FRESH"))
    client.send_control(MIN_ID_VMS, vms_flush())
    client.drain(0.15)
    check(client.probe(), "fresh VMS upload failed after abandoned one")


@test("Idle transport sends at most one RESET")
def test_idle_reset(client: MinUdpClient) -> None:
    client.connect_socket()
    client.drain(0.8)  # let feature frames and any first idle reset settle
    start = len(client.received)
    time.sleep(1.4)
    client.recv(0.3)
    resets = sum(
        1 for frame in client.received[start:] if frame.id_control == RESET
    )
    check(resets <= 1, f"idle transport emitted {resets} RESET frames")
    check(client.probe(), "MIN task unresponsive after idle period")


@test("Grid/fuse limiter parameters and status")
def test_grid_limiter_cli(client: MinUdpClient) -> None:
    client.connect_socket()
    client.cli(b"set grid_fuse 1\r", 1.2)
    client.cli(b"set grid_voltage 230\r", 1.2)
    client.cli(b"set grid_pf 0.6\r", 1.2)
    text = client.cli(b"grid\r", 1.5)
    check(b"Grid limiter:" in text, f"grid status missing; got {text[:160]!r}")
    check(b"Fuse: 1A" in text, f"fuse value missing; got {text[:160]!r}")
    check(b"PF 0.60" in text, f"power factor missing; got {text[:160]!r}")
    text = client.cli(b"grid reset\r", 1.0)
    check(b"reset" in text.lower(), f"grid reset missing; got {text[:160]!r}")
    client.cli(b"set grid_fuse 0\r", 1.2)


@test("Socket disconnect, reconnect, and CLI recovery")
def test_socket_reconnect(client: MinUdpClient) -> None:
    client.connect_socket()
    text = client.cli(b"help\r", 2.0)
    check(b"TTerm" in text or b"Commands available" in text,
          "CLI failed on first connection")
    client.disconnect_socket()
    check(client.probe(), "MIN task unresponsive after disconnect")
    client.connect_socket()
    text = client.cli(b"help\r", 2.0)
    check(b"TTerm" in text or b"Commands available" in text,
          "CLI failed after reconnect")


# ---------------------------------------------------------------------------
# Simulator lifecycle / runner
# ---------------------------------------------------------------------------

def port_is_free(host: str, port: int) -> bool:
    probe = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        probe.bind((host, port))
        return True
    except OSError:
        return False
    finally:
        probe.close()


def wait_ready(client: MinUdpClient, timeout: float = 8.0) -> bool:
    """Wait until the simulator answers EVENT_GET_INFO."""
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        client.send_app(MIN_ID_EVENT, bytes((EVENT_GET_INFO,)))
        frame = client.wait_for(
            lambda f: f.app_id == MIN_ID_EVENT,
            min(0.5, max(0.05, deadline - time.monotonic())),
        )
        if frame is not None:
            return True
    return False


def start_simulator(path: Path) -> tuple[subprocess.Popen, tempfile.TemporaryDirectory, Path]:
    workdir = tempfile.TemporaryDirectory(prefix="ud3-min-test-")
    log_path = Path(workdir.name) / "simulator.log"
    log = open(log_path, "w", encoding="utf-8")
    proc = subprocess.Popen(
        [str(path)],
        cwd=workdir.name,
        stdin=subprocess.DEVNULL,  # do not steal the shell's stdin
        stdout=log,
        stderr=subprocess.STDOUT,
    )
    return proc, workdir, log_path


def stop_simulator(proc: Optional[subprocess.Popen]) -> None:
    if proc is None:
        return
    if proc.poll() is None:
        proc.terminate()
        try:
            proc.wait(timeout=3)
        except subprocess.TimeoutExpired:
            proc.kill()
            proc.wait(timeout=3)


def print_log_tail(path: Path, lines: int = 30) -> None:
    try:
        data = path.read_text(encoding="utf-8", errors="replace").splitlines()
    except OSError:
        return
    if data:
        print("\n--- simulator log tail ---")
        for line in data[-lines:]:
            print(line)


def run_tests(client: MinUdpClient) -> int:
    failures = 0
    for index, (name, func) in enumerate(TESTS, 1):
        started = time.monotonic()
        print(f"[{index:02d}/{len(TESTS):02d}] {name} ... ", end="", flush=True)
        try:
            func(client)
        except Exception as exc:  # noqa: BLE001 - report every test failure
            failures += 1
            print(f"FAIL ({time.monotonic() - started:.2f}s)")
            print(f"    {type(exc).__name__}: {exc}")
            if client.verbose:
                traceback.print_exc()
            # Best effort: put the transport and terminal back into a usable
            # state before the next test.
            try:
                client.resync()
                client.disconnect_socket()
            except Exception:  # noqa: BLE001
                pass
        else:
            print(f"OK ({time.monotonic() - started:.2f}s)")
    return failures


def stream_cli_output(client: MinUdpClient, quiet: float = 0.4,
                     max_time: float = 10.0) -> None:
    """Print terminal frames as they arrive until the CLI goes quiet."""
    start = time.monotonic()
    last_data = start
    while True:
        frames = client.recv(0.05)
        now = time.monotonic()
        got_frame = bool(frames)
        for frame in frames:
            if frame.app_id == MIN_ID_TERM:
                data = frame.payload
                try:
                    sys.stdout.buffer.write(data)
                    sys.stdout.buffer.flush()
                except AttributeError:
                    sys.stdout.write(data.decode("latin-1"))
                    sys.stdout.flush()
        if got_frame:
            last_data = now
        if now - start >= max_time:
            break
        if not got_frame and now - last_data >= quiet:
            break


def interactive_shell(client: MinUdpClient) -> int:
    """Run a small interactive CLI-over-MIN shell."""
    try:
        import readline  # noqa: F401 - enables line editing/history when available
    except ImportError:
        pass

    client.connect_socket()
    client.drain(0.2)
    print("Interactive MIN CLI. Type :help for local commands, :q to quit.")
    print("Everything else is sent to the UD3 CLI. Ctrl+C aborts a command.\n")

    try:
        while True:
            try:
                line = input("MIN[0]> ")
            except EOFError:
                print()
                break
            except KeyboardInterrupt:
                client.send_app(MIN_ID_TERM, b"\x03")
                print("^C")
                continue

            stripped = line.strip()
            if stripped in (":q", ":quit", "exit", "quit"):
                break
            if stripped in (":h", ":help", ":?"):
                print("  :help       show this help")
                print("  :q, :quit   leave the shell")
                print("  :reset      reset the MIN transport/sequence counters")
                print("  :reconnect  disconnect and reconnect CLI socket 0")
                continue
            if stripped == ":reset":
                client.resync()
                print("MIN transport reset")
                continue
            if stripped == ":reconnect":
                client.disconnect_socket()
                client.connect_socket()
                print("CLI socket reconnected")
                continue

            try:
                client.send_app(MIN_ID_TERM, line.encode("utf-8", "replace") + b"\r")
                stream_cli_output(client)
            except KeyboardInterrupt:
                client.send_app(MIN_ID_TERM, b"\x03")
                print("^C")
    finally:
        client.disconnect_socket()
    return 0


def main(argv: Optional[list[str]] = None) -> int:
    script_dir = Path(__file__).resolve().parent
    default_sim = script_dir.parent / "build" / "ud3sim"

    parser = argparse.ArgumentParser(
        description="MIN-over-UDP integration tests for the UD3 simulator.",
    )
    parser.add_argument("--host", default="127.0.0.1", help="UDP host")
    parser.add_argument("--port", type=int, default=1337, help="UDP port")
    parser.add_argument(
        "--start-sim",
        action="store_true",
        help="start a private simulator instance for the test run",
    )
    parser.add_argument(
        "--sim",
        type=Path,
        default=default_sim,
        help=f"simulator binary (default: {default_sim})",
    )
    parser.add_argument(
        "--shell",
        action="store_true",
        help="open an interactive CLI-over-MIN shell instead of running tests",
    )
    parser.add_argument("--verbose", action="store_true", help="print MIN frames")
    parser.add_argument(
        "--allow-remote",
        action="store_true",
        help="allow a non-loopback host (VMS tests write flash; use with care)",
    )
    args = parser.parse_args(argv)

    if args.host not in ("127.0.0.1", "localhost", "::1") and not args.allow_remote:
        print(
            f"error: refusing non-loopback host {args.host!r}; "
            "the VMS tests write flash. Pass --allow-remote to override.",
            file=sys.stderr,
        )
        return 2

    proc: Optional[subprocess.Popen] = None
    workdir: Optional[tempfile.TemporaryDirectory] = None
    log_path: Optional[Path] = None
    client = MinUdpClient(args.host, args.port, verbose=args.verbose)

    try:
        if args.start_sim:
            path = args.sim.resolve()
            if not path.is_file():
                print(f"error: simulator binary not found: {path}", file=sys.stderr)
                print("build it with: cd simulator && make", file=sys.stderr)
                return 2
            if not port_is_free(args.host, args.port):
                print(
                    f"error: UDP port {args.port} is already in use; "
                    "stop the running simulator or omit --start-sim",
                    file=sys.stderr,
                )
                return 2
            proc, workdir, log_path = start_simulator(path)

        if not wait_ready(client, timeout=8.0 if args.start_sim else 3.0):
            print("error: MIN simulator did not answer EVENT_GET_INFO", file=sys.stderr)
            if log_path is not None:
                print_log_tail(log_path)
            return 1

        print(f"MIN UDP endpoint: {args.host}:{args.port}")
        if args.start_sim is False and not args.shell:
            print("note: VMS tests write the simulator flash; "
                  "use --start-sim for an isolated instance")
        if proc is not None and proc.poll() is not None:
            print("error: simulator exited during startup", file=sys.stderr)
            if log_path is not None:
                print_log_tail(log_path)
            return 1

        if args.shell:
            return interactive_shell(client)

        failures = run_tests(client)
        total = len(TESTS)
        if failures:
            print(f"\n{failures}/{total} test(s) failed")
            if log_path is not None:
                print_log_tail(log_path)
            return 1
        print(f"\nAll {total} tests passed")
        return 0
    finally:
        client.close()
        stop_simulator(proc)
        if workdir is not None:
            workdir.cleanup()


if __name__ == "__main__":
    sys.exit(main())
