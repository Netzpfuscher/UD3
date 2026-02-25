# PARAMS.md - UD3 CLI Commands and Parameters Reference

## Overview

This document describes all CLI commands and configurable parameters for the UD3 Tesla coil controller. Parameters can be runtime-only or persisted to EEPROM. Commands provide control and diagnostics for the system.

**Related files:**
- **Commands**: `common/ud3core/tasks/tsk_cli.c` - Command registration
- **Parameters**: `common/ud3core/cli_common.c` - Parameter definitions
- **Parameter system**: `common/ud3core/cli_basic.h/c` - Parameter infrastructure

---

## Command Reference

### General Commands

#### `help`
Display list of available commands.

#### `get [parameter]`
Get parameter value(s).
- **No arguments**: Prints all visible parameters with current values
- **With parameter name**: Prints specific parameter value
- **Example**: `get pw` → Display current pulse width
- **Supports autocomplete** for parameter names

#### `set <parameter> <value>`
Set parameter to new value.
- **Arguments**: parameter name, new value
- **Validation**: Checks min/max bounds, calls parameter callback
- **Example**: `set pw 100` → Set pulse width to 100 µs
- **Supports autocomplete** for parameter names

#### `eeprom <load|save>`
Save or load configuration to/from EEPROM.
- **`eeprom save`**: Write all PARAM_CONFIG parameters to EEPROM
- **`eeprom load`**: Read configuration from EEPROM and apply settings
- **Note**: Only parameters marked `PARAM_CONFIG` are saved; `PARAM_DEFAULT` parameters are runtime-only

#### `load_default`
Load default parameter values (factory reset).
- Resets all configuration to built-in defaults
- Does **not** save to EEPROM automatically

#### `reset`
Software reset the controller (reboots firmware).

---

### Interrupter Control

#### `tr <start|stop>`
Control transient (TR) mode - classic DRSSTC interrupter.
- **`tr start`**: Enable TR mode (uses `pw`, `pwd`, `vol` parameters)
- **`tr stop`**: Disable TR mode
- **Source**: `common/ud3core/interrupter.c:465`

#### `oneshot <ontime_us> <volume>`
Fire a single pulse.
- **Arguments**:
  - `ontime_us`: Pulse duration in microseconds
  - `volume`: Volume/current level (0 to 32767)
- **Example**: `oneshot 200 16000`
- **Source**: `common/ud3core/interrupter.c:504`

#### `kill <set|reset|get>`
Emergency stop / killbit control.
- **`kill set`**: Emergency stop (kills interrupter, MIDI, turns off bus)
- **`kill reset`**: Clear killbit and faults, allow restart
- **`kill get`**: Display killbit status
- **Source**: `common/ud3core/cli_common.c:864`

---

### QCW Mode Commands

QCW (Quasi-Continuous Wave) mode commands are only available when `qcw_coil` parameter is set to 1.

#### `qcw <start|stop>`
Control QCW mode.
- **`qcw start`**:
  - If `qcw_repeat >= 100`: Creates auto-repeat timer
  - If `qcw_repeat < 100`: Single-shot pulse
- **`qcw stop`**: Stop QCW and delete timer
- **Source**: `common/ud3core/qcw.c:454`

#### `ramp <subcommand> [args]`
Control QCW current ramp waveform (manual shaping).
- **`ramp point <x> <y>`**: Set single point in ramp array
- **`ramp line <x1> <y1> <x2> <y2>`**: Draw line in ramp
- **`ramp clear`**: Zero entire ramp array
- **`ramp draw`**: Visualize ramp in Teslaterm chart (Teslaterm mode only)
- **Source**: `common/ud3core/qcw.c:342`

---

### System Monitoring

#### `status <start|stop>`
Real-time VT100 terminal status overlay.
- **`status start`**: Start status display (VT100 mode)
- **`status stop`**: Stop status display
- **Source**: `common/ud3core/tasks/tsk_overlay.c:563`

#### `signals`
Interactive signal state monitor (press CTRL+C to exit).
- Displays:
  - UVLO pin, clock failure
  - System faults (temp, fuse, charging, watchdog, EEPROM, bus, feedback, interlock)
  - Feedback error count
  - Relay states (1-4)
  - Fan status
  - Bus status
  - Temperatures (1 & 2)
  - Voltages (Vbus, Vbatt, Ibus, Vdriver)
- **Source**: `common/ud3core/cli_common.c:1215`

#### `synthmon`
MIDI synthesizer status monitor (press CTRL+C to exit).
- Displays:
  - Voice data for all channels (note, channel, volume, frequency, pulse width, hypervoice count, noise)
  - Channel program mappings
- **Source**: `common/ud3core/interrupter.c:630`

#### `alarms <get|reset|roll>`
Alarm/event queue management.
- **`alarms get`**: Display alarm history
- **`alarms reset`**: Clear alarm queue
- **`alarms roll`**: Roll/rotate alarm display
- **Source**: `common/ud3core/alarmevent.c:232`

---

### Network & Communication

#### `con <info|numcon|min>`
Display connection information and MIN protocol statistics.
- **`con info`**: Show connected clients with IP addresses
- **`con numcon`**: Show count of active CLI sessions
- **`con min`**: Interactive MIN protocol monitor (CTRL+C to exit, 'r' to reset stats)
  - Dropped frames, spurious ACKs, resets, sequence mismatches, CRC errors
  - Time synchronization stats
- **Source**: `common/ud3core/cli_common.c:724`

#### `tterm <start|stop|mqtt|notelemetry> [alarm]`
Control Teslaterm/MQTT mode.
- **`tterm start [alarm]`**: Enable Teslaterm mode with telemetry
- **`tterm mqtt [alarm]`**: Enable MQTT mode (telemetry without charts)
- **`tterm notelemetry`**: Teslaterm mode without telemetry
- **`tterm stop`**: Return to VT100 terminal mode
- **`alarm` flag**: Enable alarm forwarding to terminal
- **Source**: `common/ud3core/tasks/tsk_overlay.c:583`

#### `telemetry <subcommand> [args]`
Telemetry configuration.
- **`telemetry list`** / **`telemetry ls`**: List available telemetry channels
- **`telemetry gauge <n> [name]`**: Assign telemetry channel to gauge
- **`telemetry chart <n> [name]`**: Assign telemetry channel to chart
- **Source**: `common/ud3core/tasks/tsk_overlay.c:621`

---

### Hardware Configuration

#### `bus <on|off>`
Control bus power relay.
- **`bus on`**: Turn on bus power
- **`bus off`**: Turn off bus power
- **Source**: `common/ud3core/cli_common.c:1039`

#### `relay <3|4> <0|1>`
Control user relay 3 or 4.
- **Arguments**: Relay number (3 or 4), state (0 = off, 1 = on)
- **Example**: `relay 3 1` → Turn on relay 3
- **Source**: `common/ud3core/cli_common.c:1090`

#### `pwm <3|4> <0-255>`
Set PWM duty cycle for user relay 3 or 4.
- **Arguments**: PWM number (3 or 4), duty cycle (0-255)
- **Example**: `pwm 4 128` → Set relay 4 to 50% duty
- **Source**: `common/ud3core/cli_common.c:1121`

#### `calib <measured_voltage>`
Calibrate drive voltage measurement.
- Collects ADC samples and calculates calibration factor
- **Example**: `calib 15.2` → Calibrate with measured 15.2V
- **Source**: `common/ud3core/cli_common.c:758`

#### `hwrev`
Display hardware revision information.
- Shows configured revision and detected revision from hardware pins
- **Source**: `common/ud3core/cli_common.c:1152`

#### `hwGauge <subcommand> [args]`
Configure hardware LED gauges.
- **`hwGauge assign <gauge> <telemetry>`**: Assign telemetry channel to gauge
- **`hwGauge clear <gauge>`**: Clear gauge assignment
- **`hwGauge calibrate <gauge>`**: Interactive calibration mode
- **`hwGauge startColor <gauge> <r> <g> <b>`**: Set color at 0%
- **`hwGauge endColor <gauge> <r> <g> <b>`**: Set color at 100%
- **`hwGauge transition <gauge> <0-2>`**: Set transition style
- **Source**: `common/ud3core/tasks/tsk_hwGauge.c:221`

---

### Calibration & Tuning

#### `tune <prim|sec>`
Autotune resonant frequency.
- **`tune prim`**: Sweep frequency and measure primary current
- **`tune sec`**: Sweep frequency and measure secondary feedback
- Uses `tune_start`, `tune_end`, `tune_pw`, `tune_delay` parameters
- **Source**: `common/ud3core/autotune.c:89`

#### `ntc`
Calibrate NTC thermistor iDAC current source.
- Interactive calibration for accurate temperature measurement
- **Source**: `common/ud3core/tasks/tsk_thermistor.c:236`

---

### Development & Debugging

#### `debug <subcommand>`
Debug mode control.
- **Source**: `common/ud3core/helper/debug.c:88`

#### `nvm <blocks|maps>`
NVM (EEPROM) test functions.
- **`nvm blocks`**: Display EEPROM block information
- **`nvm maps`**: Display EEPROM memory maps
- **Source**: `common/ud3core/helper/nvm.c:364`

#### `midi <channel> <note> <velocity>`
Inject MIDI message for testing.
- **Source**: `common/ud3core/tasks/tsk_midi.c:78`

---

### Special Commands

#### `bootloader`
Enter bootloader mode for firmware updates.
- Jumps to UART bootloader (if compiled with `USE_BOOTLOADER`)
- **Source**: `common/ud3core/cli_common.c:843`

#### `features`
Send feature list to Teslaterm.
- Reports protocol version and supported features
- **Source**: `common/ud3core/cli_common.c:810`

#### `config_get`
Send configuration to Teslaterm (internal use).
- **Source**: `common/ud3core/cli_common.c:824`

---

## Parameter Reference

Parameters are defined in `common/ud3core/cli_common.c` in the `confparam[]` array. Each parameter has:
- **Type**: `PARAM_CONFIG` (saved to EEPROM) or `PARAM_DEFAULT` (runtime only)
- **Visibility**: Shown/hidden in normal CLI
- **Name**: CLI parameter name
- **Min/Max**: Valid range
- **Divisor**: Display scaling (e.g., 10 = show as value/10 with 1 decimal place)
- **Callback**: Function called when parameter changes
- **Help**: Description text

### Transient (TR) Mode Parameters

| Parameter | Type | Range | Default | Description |
|-----------|------|-------|---------|-------------|
| `pw` | Runtime | 0-10000 | 0 | Pulse width [µs] |
| `pwd` | Runtime | 0-60000 | - | Pulse period (1/frequency) [µs] |
| `vol` | Runtime | 0-32767 | 0 | Volume/current level [0-0xffff] |
| `bon` | Runtime | 0-1000 | 0 | Burst mode on-time [ms], 0=disabled |
| `boff` | Runtime | 0-1000 | 500 | Burst mode off-time [ms] |
| `offtime` | Config | 3-250 | 3 | Minimum off-time for MIDI mode [µs] |

### QCW Mode Parameters

| Parameter | Type | Range | Default | Description |
|-----------|------|-------|---------|-------------|
| `qcw_ramp` | Runtime | 1-100 | 200 | Ramp increment per 125µs |
| `qcw_offset` | Runtime | 0-255 | 0 | Ramp start value |
| `qcw_hold` | Runtime | 0-255 | 0 | Holdoff time before ramp starts [×125µs] |
| `qcw_max` | Runtime | 0-255 | 255 | Ramp end value |
| `qcw_freq` | Runtime | 0-4000 | 500.0 | Ramp modulation frequency [Hz] |
| `qcw_vol` | Runtime | 0-255 | 0 | Ramp modulation volume |
| `qcw_pw` | Runtime | 0-50 | 0 | QCW pulse width [ms] |
| `qcw_repeat` | Runtime | 0-1000 | 500 | Pulse repeat time [ms], <100=single shot |

### Synthesizer Parameters

| Parameter | Type | Range | Default | Description |
|-----------|------|-------|---------|-------------|
| `synth` | Runtime | 0-3 | 0 | Synth mode: 0=off, 1=MIDI, 2=SID, 3=TR |
| `sid_hpv_enabled` | Runtime | 0-1 | - | Use hypervoice for square SID voices |
| `sid_noise_volume` | Runtime | 0-32767 | - | SID noise channel volume |
| `sid_ch1_volume` | Runtime | 0-32767 | - | SID channel 1 volume |
| `sid_ch2_volume` | Runtime | 0-32767 | - | SID channel 2 volume |
| `sid_ch3_volume` | Runtime | 0-32767 | - | SID channel 3 volume |

### Safety Limits

| Parameter | Type | Range | Default | Description |
|-----------|------|-------|---------|-------------|
| `watchdog` | Config | 0-1 | 1 | Watchdog enable (1=on, 0=off) |
| `watchdog_timeout` | Config | 1-10000 | 1000 | Watchdog timeout [ms] |
| `max_tr_pw` | Config | 0-10000 | 1000 | Maximum TR pulse width [µs] |
| `max_tr_prf` | Config | 0-3000 | 800 | Maximum TR frequency [Hz] |
| `max_qcw_pw` | Config | 0-50 | 1000 | Maximum QCW pulse width [ms] |
| `max_tr_current` | Config | 0-8000 | 400 | Maximum TR current [A] |
| `min_tr_current` | Config | 0-8000 | 100 | Minimum TR current [A] |
| `max_qcw_current` | Config | 0-8000 | 300 | Maximum QCW current [A] |
| `max_tr_duty` | Config | 1-50 | 10.0 | Max TR duty cycle [%] |
| `max_qcw_duty` | Config | 1-50 | 35.0 | Max QCW duty cycle [%] |
| `temp1_max` | Config | 0-100 | 40 | Max temperature 1 [°C] |
| `temp2_max` | Config | 0-100 | 40 | Max temperature 2 [°C] |
| `max_fb_errors` | Config | 0-60000 | 0 | Max feedback errors/sec (0=disabled) |

### Current Transformers (CT)

| Parameter | Type | Range | Default | Description |
|-----------|------|-------|---------|-------------|
| `ct1_ratio` | Config | 1-5000 | 600 | CT1 (feedback) turns ratio |
| `ct2_ratio` | Config | 1-5000 | 1000 | CT2 (bus) turns ratio |
| `ct1_burden` | Config | 1-100 | 3.3 | CT1 burden resistor [Ω] |
| `ct2_burden` | Config | 1-100 | 50.0 | CT2 burden resistor [Ω] |
| `ct2_type` | Config | 0-1 | 0 | CT2 type: 0=current, 1=voltage |
| `ct2_current` | Config | 0-2000 | 0 | CT2 current @ ct2_voltage [A] |
| `ct2_voltage` | Config | 0-5 | 4.0 | CT2 voltage @ ct2_current [V] |
| `ct2_offset` | Config | 0-5 | 0 | CT2 offset voltage [V] |
| `min_fb_current` | Config | 0-255 | 25 | Current threshold to switch to feedback [A] |

**Note**: CT2 parameters have dynamic visibility. When `ct2_type=0` (current), ratio/burden are shown. When `ct2_type=1` (voltage), current/voltage/offset are shown.

### Resonant Frequency & Timing

| Parameter | Type | Range | Default | Description |
|-----------|------|-------|---------|-------------|
| `start_freq` | Config | 0-5000 | 63.0 | Resonant frequency [kHz] |
| `start_cycles` | Config | 0-20 | 3 | Start cycles before feedback [N] |
| `lead_time` | Config | 0-4000 | 200 | Phase lead time [ns] |

### Temperature Control

| Parameter | Type | Range | Default | Description |
|-----------|------|-------|---------|-------------|
| `temp1_setpoint` | Config | 0-100 | 30 | Fan setpoint temperature [°C] |
| `temp2_setpoint` | Config | 0-100 | 30 | TH2 setpoint [°C], 0=disabled |
| `temp2_mode` | Config | 0-4 | 0 | TH2 mode: 0=off, 1=FAN, 3=Relay3, 4=Relay4 |
| `pid_temp_set` | Config | 0-100 | 45 | PID temperature setpoint [°C] |
| `pid_temp_mode` | Config | 0-4 | 0 | PID mode: 0=off, 1=PID3_Temp1, 2=PID4_Temp1, 3=PID3_Temp2, 4=PID4_Temp2 |
| `pid_temp_p` | Config | 0-200 | 0.2 | Temperature PI proportional gain |
| `pid_temp_i` | Config | 0-200 | 0.2 | Temperature PI integral gain |
| `ntc_b` | Config | 0-10000 | 3977 | NTC beta constant [K] |
| `ntc_r25` | Config | 0-33000 | 1.0 | NTC resistance at 25°C [kΩ] |
| `ntc_idac` | Config | 0-2000 | 185 | iDAC measured current [µA] |

### Power Supply & Bus

| Parameter | Type | Range | Default | Description |
|-----------|------|-------|---------|-------------|
| `ps_scheme` | Config | 0-5 | 2 | Power supply control scheme |
| `charge_delay` | Config | 1-60000 | 1000 | Charge relay delay [ms] |
| `r_bus` | Config | 1.0-1000.0 | 500.0 | Voltage divider resistor [kΩ] |
| `vdrive` | Config | 10-24 | 15.0 | Drive voltage [V] (digipot control) |
| `d_factor` | Config (hidden) | 0-10 | 1.0 | Drive voltage calibration factor |
| `uvlo_analog` | Config | 0-32.0 | 0 | UVLO from ADC [V] , 0=GPIO |

**Note**: `vdrive` is only visible on hardware revisions > 0 (3.1b and later).

### Communication

| Parameter | Type | Range | Default | Description |
|-----------|------|-------|---------|-------------|
| `baudrate` | Config | 1200-4000000 | 460800 | Serial UART baudrate [bps] |
| `ivo_uart` | Config | 0-11 | 0 | UART inversion: 0=none, 1=TX, 10=RX, 11=both |

### Autotune

| Parameter | Type | Range | Default | Description |
|-----------|------|-------|---------|-------------|
| `tune_start` | Config | 5-500.0 | 40.0 | Autotune start frequency [kHz] |
| `tune_end` | Config | 5-500.0 | 100.0 | Autotune end frequency [kHz] |
| `tune_pw` | Config | 0-800 | 50 | Autotune pulse width [µs] |
| `tune_delay` | Config | 1-200 | 50 | Autotune delay between steps [ms] |
| `autotune_s` | Config | 1-32 | 1 | Number of samples per frequency |

### System Configuration

| Parameter | Type | Range | Default | Description |
|-----------|------|-------|---------|-------------|
| `ud_name` | Config | - | "UD3-Tesla" | Coil name string (15 chars max) |
| `hw_rev` | Config | 0-2 | auto | Hardware revision: 0=3.0-3.1a, 1=3.1b, 2=3.1c |
| `qcw_coil` | Config | 0-1 | 0 | Is QCW coil: 1=true, 0=false |
| `ena_ext_int` | Config | 0-2 | 0 | External interrupter: 0=off, 1=normal, 2=inverted |
| `pca9685` | Config | 0-1 | 0 | PCA9685 LED driver: 0=off, 1=on |
| `ivo_led` | Config | 0-1 | 0 | LED invert option |
| `autostart` | Config | 0-1 | 0 | Autostart on boot |

### Duty Compressor

| Parameter | Type | Range | Default | Description |
|-----------|------|-------|---------|-------------|
| `minDutyOffset` | Config | 0-100 | 0 | Min pulse width as % of max pulse width |
| `comp_attac` | Config | 0-255 | 20 | Compressor attack setting |
| `comp_sustain` | Config | 0-255 | 44 | Compressor sustain setting |
| `comp_release` | Config | 0-255 | 20 | Compressor release setting |
| `comp_dutyOffset` | Config | 0-255 | 64 | Max duty offset before hard limit |

### Hardware Gauge (Hidden)

| Parameter | Type | Range | Default | Description |
|-----------|------|-------|---------|-------------|
| `hwGauge_cfg` | Config (hidden) | - | - | Hardware gauge config buffer |

---

## Parameter Callbacks

Many parameters trigger callback functions when changed. These callbacks perform validation, hardware reconfiguration, or side effects:

### `callback_ConfigFunction`
**Triggered by**: Most config parameters (interrupter, duty, CT, power supply, hardware settings)

**Actions**:
1. Halt Tesla coil operation (`sysflt_set`)
2. Update hardware (DCDC, digipot, watchdog)
3. Reconfigure interrupter, charging, ZCD→PWM
4. Update parameter visibility
5. Recalculate telemetry limits
6. Reinitialize Teslaterm
7. Resume operation

### `callback_TTupdateFunction`
**Triggered by**: `max_tr_current`, `max_qcw_current`, `temp1_max`, `ct1_ratio`, `ct1_burden`, `r_bus`

**Actions**:
1. Validate max current against CT1 measurement range
2. Halt operation
3. Reconfigure ZCD→PWM hardware
4. Reinitialize Teslaterm
5. Resume operation

### `callback_TuneFunction`
**Triggered by**: `tune_start`, `tune_end`

**Actions**:
- Validate `tune_start < tune_end`

### `callback_PWFunction`
**Triggered by**: `pw`, `pwd`, `offtime`

**Actions**:
- Update transient interrupter timing

### `callback_VolFunction`
**Triggered by**: `vol`

**Actions**:
- Update volume/current level

### `callback_BurstFunction`
**Triggered by**: `bon`, `boff`

**Actions**:
- Update burst mode timing

### `callback_SynthFunction`
**Triggered by**: `synth`

**Actions**:
- Switch synthesizer mode (off/MIDI/SID/TR)

### `callback_rampFunction`
**Triggered by**: QCW ramp parameters (`qcw_ramp`, `qcw_offset`, `qcw_hold`, `qcw_max`, `qcw_freq`, `qcw_vol`, `qcw_pw`)

**Actions**:
- Mark ramp as changed
- Regenerate QCW ramp waveform

### `callback_baudrateFunction`
**Triggered by**: `baudrate`

**Actions**:
- Recalculate UART clock divider
- Stop/restart UART with new rate
- Update Teslaterm datarate limits

### `callback_ivoUART` / `callback_ivoLED`
**Triggered by**: `ivo_uart`, `ivo_led`

**Actions**:
- Update IVO (invert option) control register
- Validate UART inversion combinations

### `callback_ext_interrupter`
**Triggered by**: `ena_ext_int`

**Actions**:
- Enable/disable external interrupter input

### `callback_temp_pid`
**Triggered by**: `pid_temp_p`, `pid_temp_i`

**Actions**:
- Update temperature PID controller gains

### `callback_ntc`
**Triggered by**: `ntc_b`, `ntc_r25`, `ntc_idac`

**Actions**:
- Recalculate NTC thermistor lookup table

### `callback_siggen`
**Triggered by**: `minDutyOffset`

**Actions**:
- Update signal generator minimum duty cycle

### `callback_hwGauge`
**Triggered by**: `hwGauge_cfg`

**Actions**:
- Update hardware LED gauge configuration

---

## Parameter Types

### `PARAM_CONFIG`
- **Saved to EEPROM** when `eeprom save` command is issued
- Loaded from EEPROM at boot (or via `eeprom load`)
- Used for configuration that should persist across reboots

### `PARAM_DEFAULT`
- **Runtime only** - not saved to EEPROM
- Reset to defaults on boot or `load_default` command
- Used for operational parameters that change frequently

---

## Parameter Visibility

Parameters can be hidden or shown dynamically using the visibility system. Examples:

- **CT2 parameters**: When `ct2_type=0` (current mode), `ct2_ratio` and `ct2_burden` are visible; `ct2_current`, `ct2_voltage`, `ct2_offset` are hidden. When `ct2_type=1` (voltage mode), the reverse is true.
- **`vdrive`**: Only visible on hardware revision > 0 (boards with digipot control).
- **`d_factor`**: Hidden by default (internal calibration factor).
- **`hwGauge_cfg`**: Hidden (configured via `hwGauge` command, not direct parameter access).

Visibility is controlled by `update_visibilty()` function (cli_common.c:354) and `set_visibility()` calls.

---

## Usage Examples

### Basic Setup Workflow

```bash
# Load defaults and configure coil
load_default
set ud_name MyCoil
set start_freq 70.0       # 70.0 kHz resonant frequency
set max_tr_current 500    # 500A max current
set max_tr_pw 300         # 300µs max pulse width
set watchdog 1            # Enable watchdog
eeprom save               # Save to EEPROM
```

### Run Transient Mode

```bash
set pw 200                # 200µs pulse width
set vol 16000             # Medium volume
tr start                  # Start interrupter
# ... operate coil ...
tr stop                   # Stop
kill set                  # Emergency stop if needed
kill reset                # Clear fault
```

### QCW Mode

```bash
set qcw_coil 1            # Enable QCW mode
set qcw_pw 50.00          # 50ms pulse
set qcw_repeat 500        # Repeat every 500ms
qcw start                 # Start QCW
qcw stop                  # Stop
```

### MIDI Playback

```bash
set synth 1               # Enable MIDI mode
set offtime 5             # 5µs minimum off-time
# ... send MIDI via Teslaterm or external source ...
set synth 0               # Disable
```

### Monitoring & Diagnostics

```bash
status start              # Real-time VT100 status
signals                   # Interactive signal monitor (CTRL+C to exit)
synthmon                  # MIDI voice monitor (CTRL+C to exit)
alarms get                # View alarm history
con info                  # Show connected clients
```

### Calibration

```bash
# Calibrate drive voltage measurement
calib 15.2                # Measured 15.2V with multimeter

# Autotune resonant frequency
set tune_start 50.0       # 50kHz start
set tune_end 100.0        # 100kHz end
set tune_pw 50            # 50µs pulses
tune prim                 # Sweep primary

# NTC thermistor calibration
ntc                       # Interactive calibration
```

---

## Notes

- **Safety first**: Always verify `max_tr_current`, `max_tr_pw`, and `max_tr_duty` are appropriate for your hardware before operating.
- **Parameter bounds**: The system enforces min/max ranges. Out-of-range values are rejected.
- **EEPROM hash**: Parameter structure is hashed; changing parameter layout requires EEPROM migration or factory reset.
- **Teslaterm vs VT100**: Some commands (`ramp draw`, `telemetry`, `hwGauge`) require Teslaterm mode. Use `tterm start` to enable.
- **Callbacks can fail**: If a callback returns `pdFALSE`, the parameter change is rejected. Check terminal output for error messages.
- **Dynamic visibility**: Some parameters appear/disappear based on other settings. Use `get` with no arguments to see all currently visible parameters.

---

## See Also

- **AGENTS.md** - Comprehensive codebase guide for developers
- **cli_basic.h/c** - Parameter system implementation
- **alarmevent.h/c** - Alarm/event system
- **interrupter.h/c** - Interrupter core logic
- **qcw.h/c** - QCW mode implementation
- **TTerm library** - Terminal command infrastructure
