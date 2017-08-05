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
make release
mv out/release/firmware.bin out_release/firmware-battery.bin

step "firmware-battery-mini.bin"
make release BATTERY_MINI=1
mv out/release/firmware.bin out_release/firmware-battery-mini.bin

step "firmware-power-module-RGBW-144.bin"
make release MODULE_POWER=1
mv out/release/firmware.bin out_release/firmware-power-module-RGBW-144.bin

step "firmware-power-module-RGBW-72.bin"
make release MODULE_POWER=1 LED_STRIP_COUNT=72
mv out/release/firmware.bin out_release/firmware-power-module-RGBW-72.bin

step "firmware-power-module-RGB-150.bin"
make release MODULE_POWER=1 LED_STRIP_COUNT=150 LED_STRIP_TYPE=3
mv out/release/firmware.bin out_release/firmware-power-module-RGB-150.bin

step "firmware-power-module-RGB-300.bin"
make release MODULE_POWER=1 LED_STRIP_COUNT=300 LED_STRIP_TYPE=3
mv out/release/firmware.bin out_release/firmware-power-module-RGB-300.bin
