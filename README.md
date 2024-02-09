# Hardware setup

## UART-to-USB adapter

Adapter setup is unchanged from the homework assignments.
Connect the adapter as follows:

![](components/WolfTPM/pics/uart.jpg)

In words:
- GND is connected to RPi pin 6
- D1 is connected to RPi pin 8
- D0 is connected to RPi pin 10

## TPM

Connect the TPM:

![](components/WolfTPM/pics/tpm.jpg)

In words:
- RST# is connected to RPi pin 18
- CS1# is connected to RPi pin 26
- The TPM faces "inwards", towards the HDMI port

There is a pin header provided to help positioning. It is used like this:

![](components/WolfTPM/pics/tpm-header.jpg)

## Ethernet

Connect the Raspberry Pi and the PC using an ethernet cable.

# Compiling the TRENTOS application

There are no additional dependencies or changes to the build process.
Just compile it as any other TRENTOS application:

```sh
sdk/scripts/open_trentos_build_env.sh \
    sdk/build-system.sh \
    sdk/demos/demo_tpm \
    rpi3 \
    build-rpi3-Debug-demo_tpm \
    -DCMAKE_BUILD_TYPE=Debug
```

If the project directory isn't named `sdk/demo/demo_tpm`, substitute that
with its actual path.

# Running the Python client

// TODO
