# stm32-makefile

## Overview

This repository contains a Linky TIC decoder project for [STM32F469-IDISCO development board](https://www.st.com/en/evaluation-tools/32f469idiscovery.htmL) targeting the STM32F469NI microcontroller. The project uses:

* GNU Make (Build System)
* GNU ARM Embedded Toolchain (Compiler)
* STM32CubeF4 MCU Firmware Package (BSP/Drivers)
* ST-Link or OpenOCD (Debug)

## Motivation

The makefile structure has been initially developped by [Ben Brown](https://github.com/bbrown1867/stm32-makefile).

## User Guide

### Setup

* _GNU Make_ - Usually installed by default on Linux and macOS, so no work to do here.
* _GNU ARM Embedded Toolchain_ - [Download](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads) the toolchain and update the `TOOLCHAIN_ROOT` variable at the top of the Makefile. If you've added the `bin/` directory of the toolchain to your system `PATH` then you can leave this variable blank.
* _STM32CubeF4 MCU Firmware Package_ - This can be cloned directly from https://github.com/STMicroelectronics/STM32CubeF4 into the bsp/ subdirectory. However if you already have it installed on your system, skip the submodule commands and just update the `VENDOR_ROOT` variable in the Makefile to point to your copy.
* _ST-Link or OpenOCD_ - For debugging, you will need software that knows how to talk to your debug hardware over USB. On the Nucleo-144 board, there is an ST-Link debugger. You can talk to it using [ST-Link tools](https://github.com/stlink-org/stlink) or [OpenOCD](https://sourceforge.net/p/openocd/code/ci/master/tree/). On Linux I was able to build both of these packages from source easily following the instructions. On macOS both packages were downloadable in binary form using `brew install stlink openocd`.

### Build and Debug

* Simply run `make` to build the project.
* In another terminal, start the GDB server by running `make gdb-server_openocd`.
  * To use ST-Link, run `make gdb-server_stlink`.
* Run `make gdb-client` to download the code and start debugging.
* Optionally, open a serial terminal to view the `printf` function calls.
  * For example, run `pyserial`: `python -m serial - 115200` and then select the port labeled "STM32 STLink".
