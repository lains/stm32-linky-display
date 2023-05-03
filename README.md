![CI status](actions/workflows/main.yml/badge.svg)

# STM32-based TIC display

## Overview

This repository contains a energy meter decoder and display project for french meters using the [STM32F469-IDISCO development board](https://www.st.com/en/evaluation-tools/32f469idiscovery.htmL) (STM32F469NI microcontroller).
French energy meters (like the Linky meter) continuously output metering information on a user-accessible connector on the meter (on most meters, two pins (I1, I2) are available for this).
This data is called "TIC" and this project aims to decode that data and display real-time power consumption on the LCD display embedded with the STM32 discovery board.

In order to convert the TIC data signal from the meter into a standard TTL-level serial stream, I soldered a home-made serial adapter including optocoupler and transitor-based level shifter. Examples can be found here: https://faire-ca-soi-meme.fr/domotique/2016/09/12/module-teleinformation-tic/

Once the serial adapter is connected to the STM32's GND, Vcc and serial USART RX (serial signal from meter to STM32), the STM32 will be able to receive and decode TIC stream data sent by the meter.

On the STM32F469I-DISCO, I use USART6, the above pins are all available on connector CN12 (16 pins in total, numbered from 1 to 16, pin 2 being the closest to the USB USER connector):
* GND is on CN12, pin 2
* Vcc is on CN12, pin 1
* USART6 RX is on CN12, pin 8

I am also planning to run this software on the STM32F769I-DISCO. In that case, I would use USART6, the above pins are all available on the ARDUINO Uno V3-compatible connectors (CN11, CN14, CN13, and CN9):
* GND is on CN11, pin 7
* Vcc is on CN11, pin 2
* USART6 RX is on CN13, pin 1 (maps to PC7)

The software is currently configured to decode Linky data in standard TIC mode (9600 bauds), which is not the default built-in mode for Linky meters.
In order to switch to this more verbose mode (that also provides more data), you need to make a request to your energy vendor. As an alternative, the code can be slightly tweaked to switch to historical mode and to work at 1200 bauds (this is actually the default mode for Linky meters).

In order to compile the code, this project uses:
* GNU Make (Build System)
* GNU ARM Embedded Toolchain (Compiler)
* STM32CubeF4 MCU Firmware Package (BSP/Drivers)
* [ticdecodecpp](https://github.com/lains/ticdecodecpp) as a C++ library to decode the TIC serial data on the fly
* ST-Link or OpenOCD (Debug)

The STM32-compatible makefile structure has been adapted from [a project developped by Ben Brown](https://github.com/bbrown1867/stm32-makefile).

## User Guide

### Setup & installation

* _GNU Make_ - Usually installed by default on Linux and macOS, so no work to do here.
* _GNU ARM Embedded Toolchain_ - [Download](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads) the toolchain and update the `TOOLCHAIN_ROOT` variable at the top of the Makefile. If you've added the `bin/` directory of the toolchain to your system `PATH` then you can leave this variable blank.
* The appropriate _STM32Cube MCU Firmware Package_ will be directly cloned as a submodule of this repository
* _ST-Link or OpenOCD_ - For debugging, you will need software that knows how to talk to your debug hardware over USB. On the Nucleo-144 board, there is an ST-Link debugger. You can talk to it using [ST-Link tools](https://github.com/stlink-org/stlink) or [OpenOCD](https://sourceforge.net/p/openocd/code/ci/master/tree/). On Linux I was able to build both of these packages from source easily following the instructions. On macOS both packages were downloadable in binary form using `brew install stlink openocd`.

In order to clone this repository, use `git clone --recursive`, this will allow to pull both the STM32Cube firmware package and the ticdecodecpp library.

### Build & flash

* Simply run `make` to build the project.
* To program to a board via a ST-Link proble, just type: `make flash`. The target board will be flashed with the binary thas has been built.

### Executing

Once programming is finished, you will have to wire the board to a serial link adapter (see instructions above).

Press the black reset button to start executing the program.

The instantaneous power consumption will be displayed in real-time.

### Debug

* In a terminal, start the GDB server by running `make gdb-server_openocd`.
  * To use ST-Link, run `make gdb-server_stlink`.
* Run `make gdb-client` to download the code and start debugging.
* Optionally, open a serial terminal to view the `printf` function calls.
  * For example, run `pyserial`: `python -m serial - 115200` and then select the port labeled "STM32 STLink".
