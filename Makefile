SDK_DIR ?= sdk
FIRMWARE ?= generic-node
VERSION ?= vdev

BC_LED_STRIP_TYPE_RGBW = 4
BC_LED_STRIP_TYPE_RGB  = 3

override MODULE_POWER ?= 0
override LED_STRIP_COUNT ?= 144
override LED_STRIP_TYPE ?= $(BC_LED_STRIP_TYPE_RGBW)

CFLAGS += -D'TWR_SCHEDULER_MAX_TASKS=64'
CFLAGS += -D'MODULE_POWER=$(MODULE_POWER)'
CFLAGS += -D'LED_STRIP_COUNT=$(LED_STRIP_COUNT)'
CFLAGS += -D'LED_STRIP_TYPE=$(LED_STRIP_TYPE)'
CFLAGS += -D'FIRMWARE="$(FIRMWARE)"'
CFLAGS += -D'VERSION="$(VERSION)"'

-include sdk/Makefile.mk

.PHONY: all
all: debug

.PHONY: sdk
sdk: sdk/Makefile.mk

.PHONY: update
update:
	@git submodule update --remote --merge sdk
	@git submodule update --remote --merge .vscode

sdk/Makefile.mk:
	@git submodule update --init sdk
	@git submodule update --init .vscode

