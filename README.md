Welcome to the UD3
======

![UD3](https://github.com/Netzpfuscher/UD3/blob/master/ud3.jpg)

It controls everything on a DRSSTC or a QCW. The interrupter is embedded in the UD3 (Classic Mode / MIDI / SID(C64) / QCW Ramp / QCW MIDI / QCW SID)

Wiki
----
See the [Wiki](https://github.com/Netzpfuscher/UD3/wiki) for all necessary information on hardware, installation, settings and trouble shooting. 

Features
--------

* Embedded interrupter with MIDI and SID audio modulation
* Primary resonator fres measurement (frequency sweep)
* Primary peak current measurement over feedback CT
* Pulse skipping
* Alarm/Event system for fault tracing
* System fault interlocks for undervoltage/temperature/watchdog
* Bus current measurement (current or voltage mode CT)
* Controls up to 4 relays to handle inrush current and other things
* Can control an external light display for showing selected parameters
* It runs FreeRTOS, making it easy to add new functionality
* Commandline over serial interface
* Multi user commandline over MIN-Protocol
* Dual voltage measurement (differential)
* Support for Teslaterm and UD3-node
* Telemetry over serial
* Bootloader with serial/usb support (firmware update with Teslaterm)
