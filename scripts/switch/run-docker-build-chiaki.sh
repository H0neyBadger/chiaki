#!/bin/bash

cd "`dirname $(readlink -f ${0})`/../.."

docker run \
	-v "`pwd`:/build/chiaki" \
	-w "/build/chiaki" \
	-t \
	h0neybadger/chiaki-build-switch \
	-c "scripts/switch/build.sh"

