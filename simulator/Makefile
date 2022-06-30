CC := gcc
BIN := ud3sim

BUILD_DIR := build

FREERTOS_DIR_REL := ../common/rtos
FREERTOS_DIR := $(abspath $(FREERTOS_DIR_REL))

CORE_DIR_REL := ../common
CORE_DIR := $(abspath $(CORE_DIR_REL))

FREERTOS_PLUS_DIR_REL := ../../../FreeRTOS-Plus
FREERTOS_PLUS_DIR := $(abspath $(FREERTOS_PLUS_DIR_REL))

INCLUDE_DIRS := -I.
INCLUDE_DIRS += -I${FREERTOS_DIR}/include
INCLUDE_DIRS += -I./portable/
INCLUDE_DIRS += -I./portable/utils
INCLUDE_DIRS += -I${CORE_DIR}/ud3core
INCLUDE_DIRS += -I${CORE_DIR}/ud3core/helper
INCLUDE_DIRS += -I${CORE_DIR}/tterm
INCLUDE_DIRS += -I${CORE_DIR}/min
INCLUDE_DIRS += -I${CORE_DIR}/vms

SOURCE_FILES := $(wildcard *.c)
SOURCE_FILES += $(wildcard ${FREERTOS_DIR}/*.c)
# Memory manager (use malloc() / free() )
SOURCE_FILES += ${FREERTOS_DIR}/portable/MemMang/heap_4.c
# posix port
SOURCE_FILES += portable/utils/wait_for_event.c
SOURCE_FILES += portable/port.c

# Demo library.
SOURCE_FILES += ${CORE_DIR}/ud3core/tasks/tsk_analog.c
SOURCE_FILES += ${CORE_DIR}/ud3core/tasks/tsk_cli.c
SOURCE_FILES += ${CORE_DIR}/ud3core/tasks/tsk_display.c
SOURCE_FILES += ${CORE_DIR}/ud3core/tasks/tsk_duty.c
SOURCE_FILES += ${CORE_DIR}/ud3core/tasks/tsk_eth_common.c
SOURCE_FILES += ${CORE_DIR}/ud3core/tasks/tsk_fault.c
SOURCE_FILES += ${CORE_DIR}/ud3core/tasks/tsk_hwGauge.c
SOURCE_FILES += ${CORE_DIR}/ud3core/tasks/tsk_midi.c
SOURCE_FILES += ${CORE_DIR}/ud3core/tasks/tsk_min.c
SOURCE_FILES += ${CORE_DIR}/ud3core/tasks/tsk_overlay.c
SOURCE_FILES += ${CORE_DIR}/ud3core/tasks/tsk_thermistor.c
SOURCE_FILES += ${CORE_DIR}/ud3core/tasks/tsk_uart.c
SOURCE_FILES += ${CORE_DIR}/ud3core/tasks/tsk_usb.c
SOURCE_FILES += ${CORE_DIR}/ud3core/alarmevent.c
SOURCE_FILES += ${CORE_DIR}/ud3core/autotune.c
SOURCE_FILES += ${CORE_DIR}/ud3core/cli_basic.c
SOURCE_FILES += ${CORE_DIR}/ud3core/cli_common.c
SOURCE_FILES += ${CORE_DIR}/ud3core/clock.c
SOURCE_FILES += ${CORE_DIR}/ud3core/DMA.c
SOURCE_FILES += ${CORE_DIR}/ud3core/interrupter.c
SOURCE_FILES += ${CORE_DIR}/ud3core/SignalGenerator.c
SOURCE_FILES += ${CORE_DIR}/ud3core/SignalGeneratorSID.c
SOURCE_FILES += ${CORE_DIR}/ud3core/SignalGeneratorMIDI.c
SOURCE_FILES += ${CORE_DIR}/ud3core/system.c
SOURCE_FILES += ${CORE_DIR}/ud3core/qcw.c
SOURCE_FILES += ${CORE_DIR}/ud3core/ZCDtoPWM.c
SOURCE_FILES += ${CORE_DIR}/ud3core/helper/brailledraw.c
SOURCE_FILES += $(wildcard ${CORE_DIR}/ud3core/helper/*.c)
SOURCE_FILES += $(wildcard ${CORE_DIR}/tterm/*.c)
SOURCE_FILES += $(wildcard ${CORE_DIR}/min/*.c)
SOURCE_FILES += $(wildcard ${CORE_DIR}/vms/*.c)


CFLAGS := -ggdb3 -Os -DprojCOVERAGE_TEST=0 -std=gnu11
LDFLAGS := -ggdb3 -Os -pthread -lpcap -lm -zmuldefs

OBJ_FILES = $(SOURCE_FILES:%.c=$(BUILD_DIR)/%.o)

DEP_FILE = $(OBJ_FILES:%.o=%.d)

${BIN} : $(BUILD_DIR)/$(BIN)

${BUILD_DIR}/${BIN} : ${OBJ_FILES}
	-mkdir -p ${@D}
	$(CC) $^ $(CFLAGS) $(INCLUDE_DIRS) ${LDFLAGS} -o $@


-include ${DEP_FILE}

${BUILD_DIR}/%.o : %.c
	-mkdir -p $(@D)
	$(CC) $(CFLAGS) ${INCLUDE_DIRS} -MMD -c $< -o $@

.PHONY: clean

clean:
	-rm -rf $(BUILD_DIR)
