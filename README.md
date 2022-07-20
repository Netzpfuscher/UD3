UD3 PSOC
======

This is the firmware for the UD3 Teslacoil driver. It is originally developed by Steve Ward.

![UD3](https://github.com/Netzpfuscher/UD3/blob/master/ud3.jpg)

It controls everything on a DRSSTC or a QCW. The interrupter is embedded in the UD3 (Classic Mode / MIDI / SID(C64) / QCW Ramp / QCW MIDI / QCW SID)

Features
--------

* It runs FreeRTOS 
* Commandline over serial
* Multi user commandline over MIN-Protocol
* Embedded interrupter with MIDI and SID audio modulation
* Dual voltage measurement (differential)
* Bus current measurement (current or voltage mode CT)
* Output for up to 4 relays
* Support for Teslaterm and UD3-node
* Telemetry over serial
* System fault interlocks for undervoltage/temperature/watchdog
* UART with support for manchester encoding/decoding for AC coupled connections
* Bootloader with serial/usb support (firmware update with Teslaterm)
* Primary resonator fres measurement (frequency sweep)
* Alarm/Event system for fault tracing
* Primary peak current measurement over feedback CT
* Pulse skipping


Installation
------------

Compile with Psoc Creator [Link](https://www.cypress.com/products/psoc-creator-integrated-design-environment-ide)
or flash a precompiled binary [Link](https://github.com/Netzpfuscher/UD3/tree/master/common/binary)

Connect with a serial VT100 Terminal like Putty over USB or serial and configure your coil, or use Teslaterm/UD3-node
* [Teslaterm](https://github.com/malte0811/Teslaterm/releases)
* [UD3-node](https://github.com/Netzpfuscher/UD3-node)

Default baudrate: 460800

![Pinout](https://github.com/Netzpfuscher/UD3/blob/master/wiki/connections.png)

See the Wiki for more informations.


Acknowledgements
----------------

Many thanks to all of the open source software libraries used in UD3!

* [MIN-Protocol](https://github.com/min-protocol/min)
* [VMS Synthesizer](https://github.com/TMaxElectronics/MidiStick_Firmware)
* [TTerm](https://github.com/TMaxElectronics/TTerm))
* [FreeRTOS](https://www.freertos.org/)
