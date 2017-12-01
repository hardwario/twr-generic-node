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
	TRAVIS_TAG=vdev
fi

FIRMWARE="generic-node-battery-standard"
step "bcf-${FIRMWARE}-${TRAVIS_TAG}"
make release OUT="bcf-${FIRMWARE}-${TRAVIS_TAG}" FIRMWARE="${FIRMWARE}" VERSION="${TRAVIS_TAG}"

FIRMWARE="generic-node-battery-mini"
step "bcf-${FIRMWARE}-${TRAVIS_TAG}"
make release BATTERY_MINI=1 OUT="bcf-${FIRMWARE}-${TRAVIS_TAG}" FIRMWARE="${FIRMWARE}" VERSION="${TRAVIS_TAG}"

FIRMWARE="generic-node-power-module-rgbw144"
step "bcf-${FIRMWARE}-${TRAVIS_TAG}"
make release MODULE_POWER=1 OUT="bcf-${FIRMWARE}-${TRAVIS_TAG}" FIRMWARE="${FIRMWARE}" VERSION="${TRAVIS_TAG}"

FIRMWARE="generic-node-power-module-rgbw72"
step "bcf-${FIRMWARE}-${TRAVIS_TAG}"
make release MODULE_POWER=1 LED_STRIP_COUNT=72 OUT="bcf-${FIRMWARE}-${TRAVIS_TAG}" FIRMWARE="${FIRMWARE}" VERSION="${TRAVIS_TAG}"

FIRMWARE="generic-node-power-module-rgb150"
step "bcf-${FIRMWARE}-${TRAVIS_TAG}"
make release MODULE_POWER=1 LED_STRIP_COUNT=150 LED_STRIP_TYPE=3 OUT="bcf-${FIRMWARE}-${TRAVIS_TAG}" FIRMWARE="${FIRMWARE}" VERSION="${TRAVIS_TAG}"

FIRMWARE="generic-node-power-module-rgb300"
step "bcf-${FIRMWARE}-${TRAVIS_TAG}"
make release MODULE_POWER=1 LED_STRIP_COUNT=300 LED_STRIP_TYPE=3 OUT="bcf-${FIRMWARE}-${TRAVIS_TAG}" FIRMWARE="${FIRMWARE}" VERSION="${TRAVIS_TAG}"
