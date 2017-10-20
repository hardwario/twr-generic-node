#!/bin/sh

set -eux

_step_counter=0

step() {
	_step_counter=$(( _step_counter + 1 ))
	printf '\n\033[1;36m%d) %s\033[0m\n' $_step_counter "$@" >&2  # bold cyan
}

rm -rf out_release
mkdir out_release

if [ -z "${TRAVIS_TAG:-}" ]; then
	TRAVIS_TAG=dev
fi

step "bcf-generic-node-battery-standard-${TRAVIS_TAG}"
make release OUT="bcf-generic-node-battery-standard-${TRAVIS_TAG}"

step "bcf-generic-node-battery-mini-${TRAVIS_TAG}"
make release BATTERY_MINI=1 OUT="bcf-generic-node-battery-mini-${TRAVIS_TAG}"

step "bcf-generic-node-power-module-RGBW-144-${TRAVIS_TAG}"
make release MODULE_POWER=1 OUT="bcf-generic-node-power-module-RGBW-144-${TRAVIS_TAG}"

step "bcf-generic-node-power-module-RGBW-72-${TRAVIS_TAG}"
make release MODULE_POWER=1 LED_STRIP_COUNT=72 OUT="bcf-generic-node-power-module-RGBW-72-${TRAVIS_TAG}"

step "bcf-generic-node-power-module-RGB-150-${TRAVIS_TAG}"
make release MODULE_POWER=1 LED_STRIP_COUNT=150 LED_STRIP_TYPE=3 OUT="bcf-generic-node-power-module-RGB-150-${TRAVIS_TAG}"

step "bcf-generic-node-power-module-RGB-300-${TRAVIS_TAG}"
make release MODULE_POWER=1 LED_STRIP_COUNT=300 LED_STRIP_TYPE=3 OUT="bcf-generic-node-power-module-RGB-300-${TRAVIS_TAG}"
