#!/bin/bash

set -eux

rm -rf out_release
mkdir out_release

make release MODULE_POWER=0
mv out/release/firmware.bin out_release/firmware-battery.bin

make release MODULE_POWER=1
mv out/release/firmware.bin out_release/firmware-power-module.bin
