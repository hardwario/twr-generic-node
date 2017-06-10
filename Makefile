SDK_DIR ?= sdk

override MODULE_POWER ?= 0

CFLAGS += -D'BC_SCHEDULER_MAX_TASKS=64'
CFLAGS += -D'MODULE_POWER=$(MODULE_POWER)'

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
