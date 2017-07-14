SDK_DIR ?= sdk

BC_LED_STRIP_TYPE_RGBW = 4
BC_LED_STRIP_TYPE_RGB  = 3

override MODULE_POWER ?= 0
override LED_STRIP_COUNT ?= 144
override LED_STRIP_TYPE ?= $(BC_LED_STRIP_TYPE_RGBW)

CFLAGS += -D'BC_SCHEDULER_MAX_TASKS=64'
CFLAGS += -D'MODULE_POWER=$(MODULE_POWER)'
CFLAGS += -D'LED_STRIP_COUNT=$(LED_STRIP_COUNT)'
CFLAGS += -D'LED_STRIP_TYPE=$(LED_STRIP_TYPE)'

-include sdk/Makefile.mk

.PHONY: all
all: sdk
	@$(MAKE) -s debug

.PHONY: sdk
sdk:
	@if [ ! -f $(SDK_DIR)/Makefile.mk ]; then echo "Initializing Git submodules..."; git submodule update --init; fi

.PHONY: update
update: sdk
	@echo "Updating Git submodules..."; git submodule update --remote --merge
