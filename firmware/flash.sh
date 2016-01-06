#!/bin/bash

while true; do
	if lsusb | grep -q "04d8:003c"; then
		echo "Should flash"
		./mphidflash/binaries/mphidflash-1.6-linux-64 -reset -write "./tofe-lowspeedio.X/dist/default/production/tofe-lowspeedio.X.production.hex"
	else
		#echo "No device..."
		sleep 1
	fi
	sleep 1
done
