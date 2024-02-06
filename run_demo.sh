#!/bin/bash -em

#-------------------------------------------------------------------------------
#
# Script to run a demo application together with the proxy application
#
# Copyright (C) 2021, HENSOLDT Cyber GmbH
#
#-------------------------------------------------------------------------------

#-------------------------------------------------------------------------------
USAGE_STRING="run_demo.sh <path-to-project-build> <path-to-sdk>\n
This script runs demo applications together with the proxy app.\n"

if [ "$#" -lt "2" ]; then
    echo -e "${USAGE_STRING}"
    exit 1
fi

PROJECT_BUILD_PATH=$1
SDK_PATH=$2

if [ -z ${PROJECT_BUILD_PATH} ]; then
    echo "ERROR: missing project build path"
    exit 1
fi

# default is the zynq7000 platform
IMAGE_PATH=${PROJECT_BUILD_PATH}/images/capdl-loader-image-arm-zynq7000
if [ ! -f ${IMAGE_PATH} ]; then
    echo "ERROR: missing project image ${IMAGE_PATH}"
    exit 1
fi

# create the path to the proxy application
PROXY_BIN_PATH=${SDK_PATH}/bin/proxy_app
if [ ! -f ${PROXY_BIN_PATH} ]; then
    echo "ERROR: missing proxy application binary ${PROXY_BIN_PATH}"
    exit 1
fi

shift 2

QEMU_PARAMS=(
    -machine xilinx-zynq-a9
    -m size=512M
    -nographic
    -s
    -serial tcp:localhost:4444,server
    -serial mon:stdio
    -kernel ${IMAGE_PATH}
)

read -p "Please hit 'Enter' to let the holding instance of QEMU to continue. \
You can stop QEMU with 'Ctrl-a+x' and after that the proxy app with 'Ctrl-c'."

# run QEMU
qemu-system-arm ${QEMU_PARAMS[@]} $@ 2> qemu_stderr.txt &

# give QEMU some time to start up
sleep 1

# start proxy application
${PROXY_BIN_PATH} -c TCP:4444 -t 1  > proxy_app.out &
sleep 1

fg # trigger holding qemu
fg # trigger holding proxy_app
