# AGENTS.md - UD3 Codebase Guide for AI Assistants

## Project Overview

**UD3** is a DRSSTC (Dual Resonant Solid State Tesla Coil) and QCW (Quasi-Continuous Wave) controller running on Cypress PSoC microcontrollers with FreeRTOS. It features an embedded interrupter with MIDI/SID audio modulation, safety interlocks, telemetry, and a comprehensive command-line interface.

- **Hardware**: Cypress PSoC (ARM Cortex-M3) in QFN and TQFP packages
- **RTOS**: FreeRTOS V9.0.0
- **Language**: C (C11 standard)
- **External tools**: Teslaterm, UD3-node for control
- **Protocol**: Custom MIN protocol for multi-user serial communication

## Repository Structure

```
UD3/
├── common/                      # Shared code between firmware and simulator
│   ├── ud3core/                 # Core UD3 functionality
│   │   ├── tasks/               # FreeRTOS tasks (analog, CLI, MIDI, fault, etc.)
│   │   ├── helper/              # Helper modules (printf, NVM, PID, etc.)
│   │   ├── cli_basic.h/c        # CLI parameter system
│   │   ├── cli_common.h/c       # CLI command implementation
│   │   ├── interrupter.h/c      # Main interrupter logic
│   │   ├── qcw.h/c              # QCW mode implementation
│   │   ├── config.h             # System-wide configuration constants
│   │   ├── version.h            # Protocol version information
│   │   └── FreeRTOSConfig.h     # FreeRTOS configuration
│   ├── tterm/                   # TTerm terminal library (VT100 terminal)
│   ├── min/                     # MIN protocol implementation
│   ├── vms/                     # Voice Music System (MIDI/SID processing)
│   ├── rtos/                    # FreeRTOS kernel
│   ├── Modules/                 # Git submodules (RingBuffer, DLL, utilH)
│   └── uart_ldr/                # UART bootloader code
├── simulator/                   # POSIX simulator for development/testing
│   ├── Makefile                 # Build configuration for simulator
│   ├── main.c                   # Simulator entry point
│   ├── sim_hw.c/h               # Hardware simulation layer
│   ├── UART.c/h                 # UART/UDP simulation
│   └── portable/                # POSIX port for FreeRTOS
├── UD3_QFN.cydsn/              # Cypress PSoC Creator project (QFN package)
├── UD3_TQFP.cydsn/             # Cypress PSoC Creator project (TQFP package)
├── UARTldr_QFN.cydsn/          # Bootloader project (QFN)
└── UARTldr_TQFP.cydsn/         # Bootloader project (TQFP)
```

## Build System

### Simulator Build

The simulator allows testing UD3 functionality on Linux without hardware.

**Location**: `simulator/`

**Build commands**:
```bash
cd simulator
make clean    # Clean build artifacts
make          # Build simulator (creates build/ud3sim)
./build/ud3sim # Run simulator
```

**Build details**:
- Compiler: `gcc`
- Standard: C11 (`-std=gnu11`)
- Optimization: `-Os` (size optimization)
- Debug info: `-ggdb3`
- Dependencies: `libpcap`, `pthread`, `libm`
- Sources pulled from `../common/` directories
- FreeRTOS POSIX port used for task scheduling
- Output binary: `build/ud3sim`

### Firmware Build

**Tool**: Cypress PSoC Creator (proprietary IDE)

**Project files**:
- `UD3_QFN.cydsn/UD3_QFN.cyprj` - QFN package variant
- `UD3_TQFP.cydsn/UD3_TQFP.cyprj` - TQFP package variant
- `UARTldr_*.cydsn/*.cyprj` - Bootloader variants

**Note**: Firmware builds require PSoC Creator IDE. Code edits in `common/` affect both firmware and simulator.

## Code Style and Formatting

### Code Formatting

**Formatter**: clang-format (configuration in `.clang-format`)

**Key style rules**:
- **Indentation**: Tabs (4 spaces wide) - `UseTab: Always`, `TabWidth: 4`
- **Brace style**: Attach (K&R style) - `BreakBeforeBraces: Attach`
- **Indent width**: 4 spaces - `IndentWidth: 4`
- **Column limit**: None (0) - `ColumnLimit: 0`
- **Pointer alignment**: Right - `PointerAlignment: Right` (e.g., `char *ptr`)
- **Function braces**: Same line as signature
- **Comment spacing**: Space after `//` - `SpacesBeforeTrailingComments: 1`

**Example**:
```c
void my_function(uint8_t *ptr) {
	if (condition) {
		do_something();
	} else {
		do_something_else();
	}
}
```

### Critical: TABS vs SPACES

**ALL indentation uses TABS**. When editing existing code:
1. View the file first to verify tab usage
2. Match existing indentation exactly (tabs, not spaces)
3. Edits will fail if you mix tabs and spaces

### Naming Conventions

**Functions**: 
- `snake_case` with module prefix
- Examples: `interrupter_kill()`, `EEPROM_write_conf()`, `alarm_push()`

**Variables**:
- `snake_case` for locals and globals
- Examples: `int1_prd`, `sysfault`, `configuration`

**Macros/Constants**:
- `SCREAMING_SNAKE_CASE`
- Examples: `MIDI_ISR_Hz`, `N_CHANNEL`, `HEAP_SIZE`

**Types**:
- Suffix `_t` or descriptive suffix
- Examples: `parameter_entry`, `cli_config`, `port_str`

**Tasks** (FreeRTOS):
- Prefix `tsk_` for task files
- Examples: `tsk_analog.c`, `tsk_midi.c`, `tsk_cli.c`

## Architecture Patterns

### FreeRTOS Tasks

Tasks are the primary concurrency model. Each task runs independently with assigned priority and stack size.

**Task priorities** (defined in `common/ud3core/tasks/tsk_priority.h`):
- `PRIO_FAULT`: 4 (highest - critical safety)
- `PRIO_DUTY`: 4
- `PRIO_TERMINAL`: 3
- `PRIO_QCW`: 3
- `PRIO_MIDI`: 2
- `PRIO_ANALOG`: 1
- `PRIO_OVERLAY`: 1

**Stack sizes** (in words, defined in same file):
- `STACK_TERMINAL`: 256
- `STACK_ANALOG`: 128
- `STACK_FAULT`: 100

**Common task pattern**:
```c
void tsk_example(void *pvParameters) {
	TickType_t xLastWakeTime = xTaskGetTickCount();
	
	for (;;) {
		// Do work
		
		// Wait for next period (e.g., 10ms)
		vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(10));
	}
}
```

**Task creation** (typically in `common/ud3core/main.c`):
```c
xTaskCreate(tsk_example, "Example", STACK_EXAMPLE, NULL, PRIO_EXAMPLE, NULL);
```

### Configuration System

UD3 uses a sophisticated parameter system for runtime configuration and EEPROM storage.

**Parameter definition** (`cli_common.c` or module-specific files):
```c
parameter_entry config_params[] = {
	ADD_PARAM(PARAM_CONFIG, visible, "param_name", value_var, min, max, div, callback_func, "Help text")
};
```

**Macros**:
- `ADD_PARAM()`: Defines a configurable parameter
- `PARAM_CONFIG`: Configuration parameter (saved to EEPROM)
- `PARAM_DEFAULT`: Runtime parameter (not saved)

**Types supported**:
- `TYPE_UNSIGNED` (uint8_t, uint16_t, uint32_t)
- `TYPE_SIGNED` (int8_t, int16_t, int32_t)
- `TYPE_FLOAT`
- `TYPE_STRING` (char*)
- `TYPE_BUFFER` (uint16_t*)

**Parameter variables** are typically stored in:
- `configuration` struct (EEPROM-backed config)
- `param` struct (runtime parameters)

**Callback functions**:
- Invoked when parameter changes
- Return `pdFALSE` to reject change, `pdTRUE` to accept
- Can perform validation or trigger side effects

### Command System (TTerm)

Commands use the TTerm library with a descriptor-based registration system.

**Command registration** (in `apps.c` or similar):
```c
uint8_t REGISTER_my_commands(TermCommandDescriptor *desc) {
	TERM_addCommand(CMD_mycommand, "mycommand", "Help text", 0, desc);
	return pdTRUE;
}
```

**Command handler signature**:
```c
uint8_t CMD_mycommand(TERMINAL_HANDLE *handle, uint8_t argCount, char **args) {
	// Parse args, execute command
	ttprintf("Result\r\n");
	return pdTRUE;
}
```

**Printing to terminal**:
- Use `ttprintf()` for formatted output (supports `%d`, `%s`, `%f`, etc.)
- Output goes to the terminal handle's stream buffer
- Use `\r\n` for line endings (VT100 terminal compatibility)

### Alarm/Event System

Safety-critical alarms are managed through a queue-based system.

**Push alarm** (in `common/ud3core/alarmevent.h`):
```c
alarm_push(ALM_PRIO_CRITICAL, "Error message", error_value);
```

**Priorities**:
- `ALM_PRIO_CRITICAL`: Critical fault, stops operation
- `ALM_PRIO_WARN`: Warning
- `ALM_PRIO_INFO`: Informational

**Pattern**: Critical errors should:
1. Push an alarm with descriptive message
2. Call `interrupter_kill()` to stop output
3. Set appropriate fault flags

### DMA and Hardware Abstraction

**Cypress-specific**: DMA, timers, PWM components configured in PSoC Creator TopDesign.

**Simulator stubs**: Hardware functions in `simulator/sim_hw.c` provide no-op or simulated implementations.

**Pattern**: Hardware access should be wrapped in functions or macros to allow simulator compilation.

### Interrupter System

Core timing control for Tesla coil pulses.

**Key functions** (`common/ud3core/interrupter.c`):
- `initialize_interrupter()`: One-time initialization
- `interrupter_kill()`: Emergency stop (sets interlock, kills audio)
- `interrupter_unkill()`: Release interlock
- `interrupter_updateTR()`: Update pulse parameters

**Signal path**:
1. MIDI/SID input → VMS processing
2. Signal Generator → modulation signals
3. Duty Compressor → pulse width/duty limiting
4. DMA → hardware PWM registers

## Common Configuration Constants

Defined in `common/ud3core/config.h`:

**Timing**:
- `MIDI_ISR_Hz`: 8000 (MIDI processing rate)
- `SG_CLOCK_Hz`: 320000 (Signal generator clock)
- `ADC_SAMPLE_CLK`: 32000 Hz

**Buffer sizes**:
- `NUM_MIN_CON`: 4 (MAX simultaneous MIN connections)
- `STREAMBUFFER_RX_SIZE`: 256 bytes
- `STREAMBUFFER_TX_SIZE`: 512 bytes
- `AE_QUEUE_SIZE`: 50 (Alarm/event queue depth)
- `N_QUEUE_SID`: 64 (SID frame buffer)
- `N_QUEUE_MIDI`: 64 (MIDI event buffer)

**Voice synthesis**:
- `N_CHANNEL`: 4 (Number of parallel voices for polyphony)

**Memory**:
- `HEAP_SIZE`: 48 KB (firmware), 512 KB (simulator)

**EEPROM**:
- Row size: 16 bytes (`CYDEV_EEPROM_ROW_SIZE`)
- Total size: 2048 bytes (`CYDEV_EE_SIZE`)

## Critical Gotchas

### 1. Simulator vs Firmware Differences

**`SIMULATOR` macro**: Code can conditionally compile for simulator:
```c
#ifndef SIMULATOR
	// Firmware-only code
#else
	// Simulator-only code
#endif
```

**Hardware access**: Functions like `CyDmaTdAllocate()`, `interrupter1_Start()` are Cypress PSoC-specific. Simulator provides stubs in `sim_hw.c`.

**Heap size**: Simulator has much larger heap (512 KB vs 48 KB).

### 2. EEPROM Parameter Storage

**Critical**: Parameter struct layout must match EEPROM layout. Adding/removing/reordering config parameters requires EEPROM migration or factory reset.

**Hash checking**: `EEPROM_check_hash()` validates parameter layout hasn't changed. Mismatch triggers warning.

**Save/Load**:
- `EEPROM_write_conf()`: Persist parameters
- `EEPROM_read_conf()`: Load parameters at startup

### 3. Thread Safety

**Shared data**: Access to global structs (`configuration`, `param`, `sysfault`) from multiple tasks requires:
- FreeRTOS mutexes/semaphores, OR
- Atomic operations, OR
- Single-task ownership pattern

**Queue-based communication**: Preferred for inter-task data transfer (e.g., MIDI events, alarms).

### 4. Critical Error Handling

**Never silently fail on critical errors**. Pattern:
```c
if (critical_condition) {
	alarm_push(ALM_PRIO_CRITICAL, "Descriptive error", error_code);
	interrupter_kill();
	return;
}
```

**Interlock system**: `sysfault.interlock` flag prevents output when set. Must be cleared with `interrupter_unkill()` after resolving fault.

### 5. DMA Descriptor Allocation

**TD (Transfer Descriptor) allocation**: Limited resource on PSoC. Check for `DMA_INVALID_TD` return:
```c
uint8_t td = CyDmaTdAllocate();
if (td == DMA_INVALID_TD) {
	critical_error("DMA TD allocation failed", 0);
}
```

### 6. VT100 Terminal Codes

**Terminal output**: TTerm uses VT100 escape sequences for colors, cursor control, etc.

**Functions**: Use `TERM_getVT100Code()` to generate sequences, not raw escape codes.

**Line endings**: Always `\r\n` for terminal output, not just `\n`.

### 7. Git Submodules

**Modules** in `common/Modules/`:
- RingBuffer
- DLL (Doubly-Linked List)
- utilH

**After clone**: Run `git submodule update --init --recursive` to fetch submodule code.

### 8. Visible Parameter System

**Visibility flag**: Parameters can be hidden/shown dynamically:
```c
set_visibility(params, param_size, "param_name", visible);
```

Used to hide advanced/dangerous settings from normal users.

### 9. Hardware Variants

**QFN vs TQFP**: Different package types, slight hardware differences in `hardware.h`:
- Drive voltage divider resistor values differ (v3.0 boards)
- Pin mappings may vary

**Check hardware version** at runtime if needed (typically stored in EEPROM config).

## Testing and Debugging

### Simulator Testing

**Workflow**:
1. Edit code in `common/`
2. `cd simulator && make clean && make`
3. Run `./build/ud3sim`
4. Connect via serial/UDP (symlink created at `/tmp/UD3Serial`)

**Simulator features**:
- FreeRTOS runs in POSIX threads
- UART simulated via pseudo-terminal + UDP port
- No real hardware timing constraints
- Easier to test CLI, parameter system, logic without hardware

### Serial Interface

**Physical hardware**:
- UART over USB (USB-CDC device)
- Baudrate configurable (default typically 115200)

**Simulator**:
- Symlink: `/tmp/UD3Serial` points to pseudo-terminal
- UDP: Port 21324 for Teslaterm/UD3-node communication

**Terminal programs**: minicom, screen, PuTTY, or Teslaterm (preferred).

### Common Commands (CLI)

Once connected to UD3 (hardware or simulator):

- `help` - List available commands
- `get [param]` - Get parameter value(s)
- `set <param> <value>` - Set parameter value
- `load` - Load config from EEPROM
- `save` - Save config to EEPROM
- `reset` - Reset to defaults
- `cls` - Clear screen
- `alarm` - Show alarm history
- `kill` - Emergency stop (interrupter_kill)
- `start` - Start interrupter (if unkilled)

**Autocomplete**: Dynamic autocomplete available for parameters (recent commit added this).

## Adding New Features

### Adding a New Task

1. **Create files**: `common/ud3core/tasks/tsk_myfeature.c/h`
2. **Define priority/stack**: Add to `tsk_priority.h`
3. **Implement task function**:
   ```c
   void tsk_myfeature(void *pvParameters) {
       for (;;) {
           // Work
           vTaskDelay(pdMS_TO_TICKS(100));
       }
   }
   ```
4. **Create task** in `main.c`:
   ```c
   xTaskCreate(tsk_myfeature, "MyFeature", STACK_MYFEATURE, NULL, PRIO_MYFEATURE, NULL);
   ```
5. **Add to Makefile** (simulator): Add source file to `SOURCE_FILES` in `simulator/Makefile`
6. **Add to PSoC project** (firmware): Use PSoC Creator to add files to project

### Adding a New Parameter

1. **Add variable** to `configuration` or `param` struct (in `cli_common.h`)
2. **Add entry** in parameter array (in `cli_common.c`):
   ```c
   ADD_PARAM(PARAM_CONFIG, visible, "my_param", configuration.my_param, min, max, 1, callback_func, "Description")
   ```
3. **Callback function** (optional):
   ```c
   uint8_t callback_my_param(parameter_entry *params, uint8_t index, TERMINAL_HANDLE *handle) {
       // Validation/side effects
       return pdTRUE; // Accept change
   }
   ```
4. **Visibility**: Set visible to `0` (hidden) or non-zero (visible)
5. **Division factor**: For displaying floats (e.g., 100 for 2 decimal places)

### Adding a New Command

1. **Implement command function** (in `apps.c` or module file):
   ```c
   uint8_t CMD_mycommand(TERMINAL_HANDLE *handle, uint8_t argCount, char **args) {
       if (argCount < 2) {
           ttprintf("Usage: mycommand <arg>\r\n");
           return pdFALSE;
       }
       // Execute command
       ttprintf("Success\r\n");
       return pdTRUE;
   }
   ```
2. **Register command** in `REGISTER_apps()`:
   ```c
   TERM_addCommand(CMD_mycommand, "mycommand", "Help text", 0, desc);
   ```

## File Locations Quick Reference

| Component | Header | Implementation |
|-----------|--------|----------------|
| Configuration params | `cli_common.h` | `cli_common.c` |
| System config constants | `config.h` | N/A |
| Task priorities/stacks | `tasks/tsk_priority.h` | N/A |
| Alarm/event system | `alarmevent.h` | `alarmevent.c` |
| Interrupter core | `interrupter.h` | `interrupter.c` |
| QCW mode | `qcw.h` | `qcw.c` |
| MIDI processing | `vms/MidiProcessor.h` | `vms/MidiProcessor.c` |
| SID processing | `vms/SidProcessor.h` | `vms/SidProcessor.c` |
| Signal generator | `SignalGenerator.h` | `SignalGenerator.c` |
| Terminal library | `tterm/TTerm.h` | `tterm/TTerm.c` |
| EEPROM/NVM | `helper/nvm.h` | `helper/nvm.c` |
| Printf implementation | `helper/printf.h` | `helper/printf.c` |
| Analog task (ADC) | `tasks/tsk_analog.h` | `tasks/tsk_analog.c` |
| Fault task | `tasks/tsk_fault.h` | `tasks/tsk_fault.c` |
| CLI task | `tasks/tsk_cli.h` | `tasks/tsk_cli.c` |

## Git Workflow

**Recent commits** (as of this guide):
- Adding parameters (e.g., `qcw_pw` for QCW operation)
- UI improvements (chart drawing, autocomplete)
- Bug fixes (EEPROM size, QCW bugs, simulator fixes)

**When committing**:
1. Test in simulator first: `cd simulator && make clean && make && ./build/ud3sim`
2. Test on hardware if available
3. Commit message should describe "why" not just "what"
4. Don't commit generated files (`build/`, `Generated_Source/`, etc.)

**Submodules**: If changing code in `common/Modules/*`, remember those are separate repos.

## Dependencies and External Tools

### Teslaterm

GUI application for controlling UD3 over serial/UDP. Supports:
- Parameter editing
- MIDI playback
- Telemetry visualization
- Firmware updates via bootloader

**Protocol**: MIN protocol + custom command set (see `version.h` for protocol version)

### UD3-node

Node.js-based web interface for UD3 control.

## License

MIT License (see file headers for full text)

**Copyright holders**:
- Jens Kerrinnes (primary UD3 author)
- Steve Ward (original concepts)
- Thorben Zethoff (TTerm library)

## Key Takeaways for AI Agents

1. **Always build simulator** after changes to test without hardware
2. **Use TABS for indentation**, not spaces - verify with View tool before editing
3. **Match existing code patterns** - this is a surgical, precision codebase
4. **Safety first** - critical errors must push alarms and call `interrupter_kill()`
5. **Parameters are delicate** - changing config structs requires EEPROM migration
6. **Test commands via simulator** - connect to `/tmp/UD3Serial` or UDP port 21324
7. **FreeRTOS patterns** - tasks, queues, semaphores are the concurrency model
8. **VT100 terminal** - use `ttprintf()` with `\r\n` line endings
9. **Hardware abstraction** - simulator stubs in `sim_hw.c`, real hardware in PSoC project
10. **Git submodules** - don't forget to initialize them

## Quick Start for Agents

When asked to work on this codebase:

1. **Identify the component**: Task? Parameter? Command? Core logic?
2. **Find the files**: Use table above or search in `common/ud3core/`
3. **Read existing code**: Match style, patterns, conventions
4. **Make surgical edits**: Don't refactor unnecessarily, respect existing architecture
5. **Test in simulator**: `cd simulator && make clean && make && ./build/ud3sim`
6. **Verify formatting**: Tabs for indentation, K&R brace style

For questions about hardware behavior, reference the PSoC datasheets or ask - don't guess.
