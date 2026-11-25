# UD3 Architecture

This document describes how the UD3 firmware and simulator are put together:
the runtime model, the signal and control chain, the communication stack,
persistent storage, safety mechanisms, and the simulator's models.

It is intended for developers who need to change the firmware without breaking
the timing, safety, or communication assumptions of the system. For the CLI and
parameter reference, see [INFO.md](INFO.md).

---

## 1. Scope and terminology

| Term | Meaning |
|---|---|
| **UD3** | The controller board and firmware described here |
| **DRSSTC** | Dual Resonant Solid State Tesla Coil |
| **QCW** | Quasi-Continuous Wave operation (long, ramped pulses) |
| **TR** | Transient / classic interrupter mode |
| **VMS** | Voice Modulation System: block-based envelopes, note maps and synthesis |
| **MIN** | Microcontroller Interconnect Network: framed serial protocol used by the CLI and Teslaterm |
| **TTerm** | The VT100 terminal library and command framework |
| **NVM** | The flash region used for VMS maps and blocks |
| **CT1** | Primary/feedback current transformer |
| **CT2** | DC-bus current transformer (current or voltage mode) |
| **SYSFLT** | Hardware fault/enable line driven by the fault task |

The firmware targets Cypress PSoC 5LP (ARM Cortex-M3) and runs FreeRTOS. A
POSIX port of the same application code is used by the simulator.

---

## 2. System context

```
                    USB / UART / MIN
  Teslaterm  <-------------------------->  UD3 firmware
  UD3-node                                (PSoC + FreeRTOS)
  serial terminal                              |
                                               | gate drive, current limit
                                               v
                                        Full-bridge inverter
                                               |
                                      primary tank / DRSSTC coil
                                               |
                                  CT1 feedback, ZCD, CT2 bus current,
                                  bus voltage, thermistors, relays
```

The controller owns:

- pulse generation and primary-current limiting,
- MIDI/SID/QCW audio modulation,
- bus precharge and relay sequencing,
- telemetry and the CLI,
- safety interlocks and fault latching,
- non-volatile storage for configuration and VMS data.

The physical fuse/breaker, gate-driver protection, and the inverter hardware
remain the primary safety layer; firmware interlocks are supplementary.

---

## 3. Repository layout

```
UD3/
├── common/
│   ├── ud3core/            Core firmware
│   │   ├── main.c          Boot, hardware init, task creation
│   │   ├── tasks/          FreeRTOS tasks and ISR-facing modules
│   │   ├── helper/         printf, NVM, PID, I2C, telemetry helpers
│   │   ├── cli_basic.*     Parameter/EEPROM infrastructure
│   │   ├── cli_common.*    UD3 parameter table and CLI commands
│   │   ├── interrupter.*   Kill/interlock, TR pulse path
│   │   ├── SignalGenerator.* Voice/pulse scheduler and duty limiting
│   │   ├── qcw.*           QCW ramp, timer and phase-shift output
│   │   ├── ZCDtoPWM.*      Feedback/ZCD and PWM configuration
│   │   ├── DMA.c           PSoC DMA channel setup
│   │   ├── alarmevent.*    Alarm/event queue
│   │   └── telemetry.h     Telemetry item definitions
│   ├── tterm/              VT100 terminal, command system and apps
│   ├── min/                MIN protocol framing and transport
│   ├── vms/                MIDI/SID/VMS synthesis and filters
│   ├── Modules/            RingBuffer, DLL, utilH submodules
│   └── rtos/               FreeRTOS kernel and PSoC port
├── simulator/              POSIX simulator
│   ├── main.c              Simulator boot
│   ├── sim_hw.c/.h         Hardware stubs and simulated models
│   ├── tsk_sim.c           Bus/current simulation task
│   ├── tsk_audio.c         Optional audio-output simulation
│   ├── UART.c              UDP + pseudo-terminal transport
│   ├── portable/           FreeRTOS POSIX port
│   └── tests/              MIN-over-UDP integration tests and shell
├── docs/                   Generated docs and this architecture note
├── UD3_QFN.cydsn/          PSoC Creator project (QFN)
├── UD3_TQFP.cydsn/         PSoC Creator project (TQFP)
└── UARTldr_*.cydsn/        Bootloader projects
```

The `common/` tree is compiled into both the firmware and the simulator. The
`SIMULATOR` macro selects POSIX/hardware stubs; see `simulator/sim_hw.h`.

---

## 4. Boot and initialization

Firmware entry is `common/ud3core/main.c`:

1. Force bus and charge relays off.
2. Install FreeRTOS exception vectors in the PSoC RAM vector table.
3. Initialize the alarm queue and suppress the hardware fault line during
   startup.
4. Load defaults (`init_config()`), start EEPROM, then load the stored
   configuration (`eeprom_load()`).
5. Initialize DMA, the interrupter, ZCD-to-PWM and the charging state machine.
6. Apply the final ZCD/PWM configuration.
7. Create the tasks (MIN, USB, CLI, MIDI, SID, analog, thermistor, fault, duty,
   optional hardware gauge, hypervisor) and start the FreeRTOS scheduler.

The simulator follows the same order in `simulator/main.c`, adding:

- `load_flash()` to load `flash.data`,
- `tsk_sim_Start()` for the bus/current model,
- `tsk_audio_Start()` for optional audio playback.

The scheduler never returns. `vApplicationStackOverflowHook()` and
`vApplicationMallocFailedHook()` halt the system with interrupts disabled; these
are fatal conditions, not recoverable errors.

---

## 5. FreeRTOS runtime

### 5.1 Tasks

Priorities are FreeRTOS priorities: larger numbers run first. Stack sizes are in
32-bit words from `common/ud3core/tasks/tsk_priority.h`.

| Task | Priority | Stack | Responsibility |
|---|---:|---:|---|
| `Duty` | 4 | 200 | Measure on-time/duty and run the grid/fuse limiter every 100 ms |
| `Fault-Svc` | 4 | 100 | UVLO, feedback-loss and watchdog supervision, SYSFLT line |
| `Hyper` | 4 | 100 | Hypervisor/runtime supervision |
| `SigGen` | 4 | 228 | Convert voice states into timed pulses; run duty limiting |
| `MIN-Svc` | 3 | 384 | Drain UART, run MIN transport, dispatch application frames |
| `USB-Svc` | 3 | 128 | USB CDC transport |
| `MIN0..3` / `USB-CLI` | 3 | 256 | One terminal task per CLI port |
| `VMS Task` | 3 | 356 | Execute VMS block lists and update voices |
| `MIDI-Svc` | 2 | 200 | MIDI parser/queue consumer |
| `SID-Svc` | 2 | 200 | SID frame queue consumer |
| `COMP` | 2 | 100 | Duty compressor (10 ms) |
| `Analog` | 1 | 128 | ADC DMA buffers, RMS filtering, telemetry |
| `Therm` | 1 | 100 | Thermistor sampling and temperature control |
| `Overlay` | 1 | 256 | VT100/Teslaterm telemetry overlay (one per active port) |
| `HWGauge-Svc` | 1 | 200 | Optional PCA9685 hardware gauge |
| `SIM` / `SIM_ISR` | 3 | 1024 | Simulator-only bus/current models |

The table is descriptive; always consult `tsk_priority.h` and the individual
`*_Start()` functions for the authoritative values.

### 5.2 Interrupts and timers

- **`isr_synth`** runs at `MIDI_ISR_Hz` (8 kHz). It advances the system clock
  and, while `QCW_enable_Control` is set, executes the QCW ramp.
- **`SigGen_PulseTimerISR`** consumes the pulse ring: it commands the previous
  pulse through `interrupter_oneshotRaw()` and loads the next pulse period.
- **`ADC_data_ready_ISR`** releases the analog sampling semaphore.
- FreeRTOS software timers handle watchdog timeout (`vWD_Timer_Callback`), QCW
  repeat (`vQCW_Timer_Callback`) and precharge sequencing
  (`vCharge_Timer_Callback`).
- Hardware blocks (ZCD comparators, feedback capture/filter, CT1 comparator,
  PWM) form the fast feedback and current-limit path; they are configured by
  `ZCDtoPWM.c`/`DMA.c`.

Keep ISR work short. The QCW ramp and pulse scheduler are deliberately placed in
the ISR path or a dedicated high-priority task; do not add blocking calls there.

### 5.3 Shared state and concurrency

The main shared objects are:

| Object | Owner/updater | Readers |
|---|---|---|
| `configuration` | CLI, EEPROM load | almost all tasks |
| `param` | CLI/commands | signal chain, MIDI/SID |
| `sysfault` / `system_fault_Control` | fault task, kill paths | signal chain, CLI |
| `tt` telemetry | analog, MIDI/SID, duty, fault | overlay/MIN |
| MIN context and transport FIFO | MIN task, `min_queue()` | CLI/telemetry tasks |
| VMS block list | VMS task (semaphore-protected) | MIDI/VMS |

Conventions:

- **Queues** are used for MIDI, SID and alarm events.
- **Signals/semaphores** protect the MIN transport and the VMS block list.
- **Atomic word accesses** are used for small flags such as `interlock` and
  `QCW_enable_Control`.
- **Critical sections** are used for short register/DMA updates.
- Long operations (flash writes, VMS upload validation) run in task context,
  not in ISRs.

When adding shared state, prefer a queue or a single-writer model. Avoid
blocking while holding `min_Semaphore` or the VMS semaphore.

---

## 6. Signal and control chain

### 6.1 Overview

```
MIN / USB / TTerm
      │
      ├── CLI commands ────────────────┐
      ├── VMS map/block uploads ──┐    │
      └── MIDI/SID frames ───┐    │    │
                             v    v    v
                       MIDI/SID/VMS processing
                             │
                             v
                     SignalGenerator voices
                             │
                    pulse ring buffer (128)
                             │
                   SigGen_PulseTimerISR
                             │
                   interrupter_oneshotRaw()
                             │
                  gate drive / PWM hardware
                             │
              CT1 comparator + DAC current limit
                             │
                  primary tank / DRSSTC coil

ADC DMA (Vbus, CT2, CT1 peak, temperatures)
      │
      v
 tsk_analog (RMS/filtering) ──> telemetry ──> MIN/Teslaterm

tsk_fault ──> sysfault ──> interrupter_kill() ──> SigGen/QCW off
tsk_duty  ──> grid limiter ──> duty scale ──> SigGen_limit/COMP/QCW
```

### 6.2 DRSSTC / TR mode

In TR mode the user controls `pw` (pulse width), `pwd` (period) and `vol`
(volume/current demand). `interrupter_updateTR()` computes the drive frequency
from `pwd` and updates the SignalGenerator TR voice. `SigGen_limit()` applies:

- the configured `max_tr_duty`,
- the duty compressor gain,
- the grid/fuse limiter scale,
- minimum on-time/period constraints.

The CT1 comparator and its DAC (`ct1_dac_val[]`) provide the fast hardware
primary-current limit. The firmware schedules pulses; the hardware terminates
or limits drive when the instantaneous current threshold is reached. This is
the mechanism behind the burst / skip-cycle behavior of a DRSSTC.

### 6.3 MIDI, SID and VMS

- MIDI events are parsed into a queue and consumed by `tsk_midi`.
- SID frames are parsed into `SIDFrame_t` and consumed by `tsk_sid`.
- VMS maps translate MIDI notes/programs to VMS blocks and voice parameters.
- VMS blocks are executed by `VMS_run()` in the VMS task and update the six
  SignalGenerator voices.
- Note maps and blocks are uploaded over MIN and stored in the NVM flash area.

SID uses three channels; the SignalGenerator voice pool is six voices. Do not
confuse `N_CHANNEL` (DMA channels) with voice count.

### 6.4 QCW

QCW uses the same 8 kHz `isr_synth` but bypasses normal pulse scheduling:

- `qcw_regenerate_ramp()` builds a current envelope in `ramp.data[]`.
- `qcw_handle()` advances the ramp in the 8 kHz ISR.
- `qcw_modulate()` converts the ramp value into phase-shift control.
- `qcw_start()`/`qcw_stop()` enable/disable the QCW hardware path.
- An optional timer (for `qcw_repeat >= 100`) restarts pulses.

The grid limiter scales the QCW ramp maximum and refuses to start a pulse when
the limiter has no headroom. If an active QCW pulse exceeds the continuous
grid limit, the limiter calls `qcw_stop()`; a severe I²t event kills the output.

### 6.5 Duty control and the grid/fuse limiter

Two slow control layers sit around the pulse scheduler:

1. **Duty compressor** (`DutyCompressor.c`) — 10 ms loop based on measured
   current duty. It adjusts an internal gain to keep duty below the effective
   maximum.
2. **Grid/fuse limiter** (`tasks/tsk_duty.c`, API in `tasks/grid_limit.h`) —
   100 ms loop based on DC-bus power.

The grid limiter estimates:

```
P_dc   = V_bus × I_bus
P_grid ≈ P_dc / efficiency
I_grid ≈ P_grid / (V_mains × PF × phase_factor)
```

and derives a DC power limit from `grid_fuse`, `grid_voltage`, `grid_phases`,
`grid_pf`, `grid_margin` and `grid_eff`. It outputs a duty scale in the range
0…1000 (1000 = unrestricted). That scale is applied in:

- `SigGen_limit()` — effective maximum duty,
- `DutyCompressor` — effective maximum duty,
- `qcw_regenerate_ramp()` — QCW ramp maximum.

The I²t model normalizes current as `x = I_grid / I_fuse`:

```
if x > 1.02: heat += (x² − 1) × 0.1 s
else:        heat -= 0.1 s        (floored at 0)
trip when heat >= 1.25 × grid_i2t_time
```

`grid_i2t_time` is the tolerated time at 150 % of the fuse rating. A hard trip
also occurs after 500 ms above 200 %. On trip the limiter kills the output,
opens the bus, and latches until `grid reset` or `kill reset`.

The limiter is intentionally supplementary. It estimates AC grid current from
DC-bus power and assumes a power factor/efficiency; the physical fuse/breaker
remains the primary protection.

---

## 7. Sensing and telemetry

- The ADC scans a ping-pong pair of 25-sample buffers through DMA.
- `tsk_analog` computes RMS/filtered values for bus voltage, CT2 current,
  battery voltage and temperatures, and updates `tt`.
- `ADC_peak` supplies the CT1 peak current used for primary-current telemetry
  and autotune.
- Telemetry items are declared in `telemetry.h` and initialized/configured in
  `tsk_overlay.c` (name, unit, range, divider, chart/gauge assignment).
- The overlay task streams gauges/charts to Teslaterm or prints a VT100 status
  page; MIN/CLI clients receive terminal output through TTerm.

Adding a telemetry item requires:

1. Add a `TELE` field to `TELE_HUMAN` in `telemetry.h`.
2. Initialize it in `init_telemetry()` and, if needed, update its range in
   `recalc_telemetry_limits()`.
3. Update the value in the appropriate task.

`N_TELE` is derived from `sizeof(TELE_HUMAN)`, so array access iterates the new
field automatically.

---

## 8. Communication stack

### 8.1 MIN

`common/min/min.c` implements MIN v2.0 framing and an optional reliable
transport layer:

- 3 × `0xAA` header, ID/control byte, optional 4-byte sequence, length,
  payload, 32-bit CRC, `0x55` EOF.
- `0xAA` byte stuffing.
- ACK/RESET frames, sliding-window retransmission, CRC and sequence counters.
- Per-context receive buffer and transport FIFO.

`tsk_min.c` maps MIN IDs to application handlers:

| ID | Purpose |
|---|---|
| 0–9 | Terminal/CLI streams (one per MIN socket) |
| 10 | Watchdog/time sync |
| 12 | Control commands |
| 13 | Socket connect/disconnect |
| 14 | Synth mode control |
| 20/21 | MIDI/SID data |
| 40/41 | Events and alarms |
| 43 | VMS uploads |

MIN is unauthenticated. Any host on the UART/USB link can send frames, so every
handler must validate lengths and ranges. The same rule applies to VMS uploads:
map/block sizes, IDs and flash offsets are checked before writing.

### 8.2 TTerm and CLI

`tterm/` provides a VT100 terminal, line editor/history/autocomplete, command
registration, and terminal "programs" (status monitor, synth monitor, top,
wizard, nvmmon). Each CLI port has a `TERMINAL_HANDLE` and a stream buffer; a
dedicated task feeds bytes into `TERM_processBuffer()`.

The parameter system (`cli_basic.*`) binds names to typed variables. Parameters
are either `PARAM_CONFIG` (saved to EEPROM) or `PARAM_DEFAULT` (runtime only).
Callbacks validate or apply changes.

### 8.3 Teslaterm

Teslaterm uses MIN for both CLI traffic and telemetry. The overlay task sends
gauge/chart frames and alarm events; MIN transport provides reliable delivery.
Teslaterm can also upload VMS maps/blocks and trigger autotune.

---

## 9. Persistent storage

### 9.1 EEPROM configuration

- 2 KB EEPROM, 16-byte rows.
- Dataset header: `00 C0 FF EE 00`, trailer: `DE AD BE EF`.
- Each parameter is stored as a 4-byte djb2 hash of its name, a 1-byte size,
  and the value bytes.
- `EEPROM_check_hash()` checks name-hash collisions.
- `EEPROM_read_conf()` matches by hash/name and validates the stored size.
- Adding parameters is hash-based; old datasets simply lack the new names and
  the defaults remain in effect. Run `eeprom save` after a firmware update to
  persist the new fields.

The grid limiter adds `grid_*` configuration fields at the end of
`config_struct`; they follow the same rules. `EEPROM_write_conf()` refuses to
start a dataset that cannot fit in 2 KB.

### 9.2 VMS flash (NVM)

`common/ud3core/helper/nvm.c` manages a 32 KB flash region:

- 16 KB map area (`MAPMEM_SIZE`) starting at `0x38000`.
- 16 KB block area (`BLOCKMEM_SIZE`).
- Map tables contain a 21-byte packed header and 8-byte entries.
- Blocks are 38-byte packed VMS blocks, bounded by
  `BLOCKMEM_SIZE / sizeof(VMS_Block_t)`.

Writes are buffered per 256-byte flash page; `nvm_flush()` commits the current
page, `nvm_discard()` drops an uncommitted page when an upload is cancelled.
VMS upload state is owned by the MIN module and is discarded when the last
socket disconnects.

---

## 10. Safety architecture

### 10.1 Fault latching

`tsk_fault` maintains `SYSFAULT sysfault` (UVLO, temperatures, fuse input,
charging, watchdog, EEPROM, bus UV, interlock, link, feedback). `handle_FAULT()`
drives the hardware SYSFLT line from the aggregate state.

`interrupter_kill()`:

- sets `sysfault.interlock`,
- stops QCW,
- kills SignalGenerator audio/voices,
- zeroes `param.pw`,
- updates the TR hardware path.

`kill reset` calls `interrupter_unkill()`, `reset_fault()`, resets the grid
limiter and releases SYSFLT. Output is not automatically restarted.

### 10.2 Watchdog and feedback

- The MIN task resets the software watchdog on valid traffic.
- A watchdog timeout kills the output and remains latched.
- Feedback-loss detection (`handle_no_fb`) kills the output after the
  configured error threshold.
- Undervoltage and over-temperature conditions set `sysfault` bits.

### 10.3 Grid/fuse trip

The grid limiter has its own latched trip. It calls `interrupter_kill()` and
opens the bus on severe overload or I²t exhaustion. A physical fuse/breaker is
still required; the limiter estimates grid current from DC-bus power and cannot
guarantee fuse selectivity.

### 10.4 Design rules for safety changes

- Never clear an interlock implicitly except through the documented reset path.
- A fault handler must stop output before returning.
- Do not block in an ISR or while holding a lock.
- Validate all externally supplied lengths, IDs, frequencies and duty values.
- Keep hardware protection independent of software.

---

## 11. Simulator architecture

The simulator is not a cycle-accurate hardware model. It is a development aid
for the application, CLI, protocol and control logic.

### 11.1 POSIX port and stubs

- FreeRTOS runs on POSIX threads (`simulator/portable/port.c`).
- `simulator/sim_hw.c` provides PSoC component stubs: GPIO, EEPROM, flash,
  timers, DMA, I2C, PCA9685, ADC and the interrupter registers.
- `simulator/cydevice*.h`, `cyfitter.h` and `project.h` provide the PSoC
  constants/types needed by the shared code.
- `SIMULATOR` selects the stubs and the simulator-specific paths.

### 11.2 Bus and current models

`tsk_sim.c` models:

- mains-rectified bus voltage with RC charging and bus status,
- a bus-current estimate driven by `param.pw`/`param.pwd` and active voices,
- the ping-pong ADC buffers consumed by `tsk_analog`.

`sim_hw.c` models the CT1 peak detector:

- `ADC_peak_GetResult16()` returns a simulated 12-bit value (1 mV/count),
- `sim_hw_oneshot()` handles explicit one-shot pulses: it computes
  `V_bus × t_on / L_pri`, clamps to the DAC current limit, and applies a
  Lorentzian frequency response for autotune,
- the periodic voice-based current model lives in `tsk_sim.c`.

Autotune resonance defaults to `630` (63.0 kHz) with a width of `25`
(2.5 kHz), overridable with `UD3SIM_RESONANCE` and `UD3SIM_RESONANCE_BW`.

### 11.3 UDP, pseudo-terminal and audio

- The simulated UART is reachable over UDP **port 1337**.
- A pseudo-terminal symlink is created at **`/tmp/ud3sim-uart`**.
- Each MIN frame is sent as one UDP datagram.
- `tsk_audio.c` optionally writes a PCM stream to `aplay` when
  `ud3sim_audio=1`.

### 11.4 Tests and shell

`simulator/tests/test_min_udp.py` is a complete MIN host implementation:
framing, stuffing, CRC32, transport sequence numbers, ACKs and RESET handling.
It runs integration tests for CLI, malformed frames, transport, VMS uploads,
idle reset and the grid limiter. `--shell` opens an interactive CLI-over-MIN
session.

```bash
python3 simulator/tests/test_min_udp.py --start-sim
python3 simulator/tests/test_min_udp.py --start-sim --shell
```

### 11.5 Simulator limitations

- Not real-time; timing-dependent behavior can differ from hardware.
- Hardware comparator/DMA interactions are approximated.
- The primary-current model is an approximation, not a circuit simulation.
- Audio output is optional and not part of normal tests.
- The simulator writes `flash.data` and `eeprom.txt` in its working directory;
  the test runner uses a temporary directory for isolation.

---

## 12. Build and test workflows

### Simulator

```bash
cd simulator
make clean
make
./build/ud3sim
```

### Tests

```bash
python3 simulator/tests/test_min_udp.py --start-sim
```

### Firmware

Open `UD3_QFN.cydsn/UD3_QFN.cyprj` or `UD3_TQFP.cydsn/UD3_TQFP.cyprj` in
PSoC Creator, build, and flash via the bootloader or SWD.

The simulator's Makefile collects `common/` sources by wildcard; the PSoC
project files list sources explicitly. When adding a new `.c` file, add it to
both PSoC projects as well as the simulator Makefile (if not covered by a
wildcard).

---

## 13. Extending the firmware

### Add a task

1. Create `common/ud3core/tasks/tsk_*.c/h`.
2. Define priority and stack in `tsk_priority.h`.
3. Create the task in `main.c` (and `simulator/main.c` if simulator-specific).
4. Add the source to the PSoC projects; the simulator wildcard usually picks it
   up automatically.
5. Keep initialization before `vTaskStartScheduler()`; keep loops non-blocking.

### Add a parameter

1. Add a field to `config_struct` (persistent) or `parameter_struct` (runtime).
2. Set a default in `init_config()`.
3. Add an `ADD_PARAM(...)` entry in `cli_common.c`.
4. Add a callback if the value needs validation or side effects.
5. Run `eeprom save`; old EEPROM datasets simply lack the new name and use the
   default. Check the 2 KB capacity if adding many fields.

### Add a command

1. Implement `uint8_t CMD_name(TERMINAL_HANDLE *, uint8_t, char **)`.
2. Declare it in the appropriate header.
3. Register it with `TERM_addCommand()` in `tsk_cli.c`.
4. Use `ttprintf()` with `\r\n`; keep terminal output serialized with the
   terminal/overlay conventions.

### Add a MIN frame ID

1. Add the ID to `common/min/min_id.h`.
2. Add a handler in `min_application_handler()` in `tsk_min.c`.
3. Validate `len_payload` before reading any field.
4. Bound external IDs, counts, flash offsets and allocation sizes.
5. Add a test to `simulator/tests/test_min_udp.py`.

### Add telemetry

1. Add a `TELE` field to `TELE_HUMAN`.
2. Initialize it in `init_telemetry()` and set ranges in
   `recalc_telemetry_limits()`.
3. Update it from the owning task.
4. Use an existing gauge/chart slot or add one carefully; gauge/chart numbers
   are part of the host UI contract.

---

## 14. Known design limitations

- MIN is unauthenticated; treat the serial/USB link as trusted.
- The grid limiter estimates AC current from DC-bus power, PF and efficiency.
  It is a conservative power/fuse limiter, not a calibrated AC RMS meter.
- The physical fuse/breaker remains mandatory.
- The simulator is not cycle-accurate and does not model all PSoC analog/DMA
  behavior.
- VMS upload data is validated, but flash writes are not transactional across a
  whole map/block upload; a disconnect discards the uncommitted page.
- EEPROM parameters are matched by name hash; adding fields is backward
  compatible in practice, but run `eeprom save` and verify after upgrades.
- The bootloader is a separate PSoC project and has its own memory map and
  update protocol.

---

## 15. Key file reference

| Area | Files |
|---|---|
| Boot/tasks | `common/ud3core/main.c`, `tasks/tsk_priority.h` |
| Signal generation | `SignalGenerator.c/.h`, `interrupter.c/.h` |
| QCW | `qcw.c/.h` |
| Feedback/ZCD/PWM | `ZCDtoPWM.c/.h`, `DMA.c` |
| Analog/telemetry | `tasks/tsk_analog.c`, `tasks/tsk_duty.c`, `telemetry.h`, `tasks/tsk_overlay.c` |
| Faults | `tasks/tsk_fault.c/.h`, `alarmevent.c/.h` |
| Grid limiter | `tasks/tsk_duty.c`, `tasks/grid_limit.h` |
| CLI/parameters | `cli_basic.c/.h`, `cli_common.c/.h`, `tterm/` |
| MIN | `common/min/min.c/h`, `common/min/min_id.h`, `tasks/tsk_min.c` |
| VMS | `common/vms/`, `helper/nvm.c/.h` |
| EEPROM | `cli_basic.c` |
| Simulator | `simulator/`, especially `sim_hw.c`, `tsk_sim.c`, `UART.c` |
| Tests | `simulator/tests/test_min_udp.py` |

---

For user-facing commands and parameters, see [INFO.md](INFO.md) and the
project Wiki. For build instructions, see [README.md](README.md).
