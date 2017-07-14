#!/bin/sh

set -eux

_step_counter=0

step() {
	_step_counter=$(( _step_counter + 1 ))
	printf '\n\033[1;36m%d) %s\033[0m\n' $_step_counter "$@" >&2  # bold cyan
}

rm -rf out_release
mkdir out_release

step "firmware-battery.bin"
make release MODULE_POWER=0
mv out/release/firmware.bin out_release/firmware-battery.bin

step "firmware-power-module-144-RGBW.bin"
make release MODULE_POWER=1
mv out/release/firmware.bin out_release/firmware-power-module-144-RGBW.bin

step "firmware-power-module-150-RGB.bin"
make release MODULE_POWER=1 LED_STRIP_COUNT=150 LED_STRIP_TYPE=3
mv out/release/firmware.bin out_release/firmware-power-module-150-RGB.bin
