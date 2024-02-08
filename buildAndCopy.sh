#!/bin/bash

# Source the environment script
source sdk/scripts/open_trentos_build_env.sh

# Run the build system script
./sdk/build-system.sh ws2023_team_3_ads_tpm rpi3 build-rpi3-ws2023_team_3_ads_tpm -DCMAKE_BUILD_TYPE=Debug

# Copy files to the USB disk
cp sdk/resources/rpi3_sd_card/* /media/wardstone/TRENTOS/
cp build-rpi3-ws2023_team_3_ads_tpm/images/os_image.elf /media/wardstone/TRENTOS

# Sync to ensure data is written to disk
sync

# Unmount the USB disk
umount /media/wardstone/TRENTOS/
