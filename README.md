![CI status](https://github.com/lains/stm32-linky-display/actions/workflows/main.yml/badge.svg)

# STM32-based TIC display

## Overview

This repository contains a energy meter decoder and display project for french meters using the [STM32F469-IDISCO development board](https://www.st.com/en/evaluation-tools/32f469idiscovery.html) (STM32F469NI microcontroller) or [STM32F769-IDISCO development board](https://www.st.com/en/evaluation-tools/32f769idiscovery.html) (STM32F769NI microcontroller).
French energy meters (like the Linky meter) continuously output metering information on a user-accessible connector on the meter (on most meters, two pins (I1, I2) are available for this).
This data is called "TIC" and this project aims to decode that data and display real-time power consumption on the LCD display embedded with the STM32 discovery board.

In order to convert the TIC data signal from the meter into a standard TTL-level serial stream, I soldered a home-made serial adapter including optocoupler and transitor-based level shifter. Examples can be found here: https://faire-ca-soi-meme.fr/domotique/2016/09/12/module-teleinformation-tic/

Once the serial adapter is connected to the STM32's GND, Vcc and serial USART RX (serial signal from meter to STM32), the STM32 will be able to receive and decode TIC stream data sent by the meter.

On the STM32F469I-DISCO, I use USART6, the above pins are all available on connector CN12 (16 pins in total, numbered from 1 to 16, pin 2 being the closest to the USB USER connector):
* GND is on CN12, pin 2
* Vcc is on CN12, pin 1
* USART6 RX is on CN12, pin 8

On the STM32F769I-DISCO., I use USART6, the above pins are all available on the ARDUINO Uno V3-compatible connectors (CN11, CN14, CN13, and CN9):
* GND is on CN11, pin 7
* Vcc is on CN11, pin 2
* USART6 RX is on CN13, pin 1 (maps to PC7)

The software is currently configured to decode Linky data in standard TIC mode (9600 bauds), which is not the default built-in mode for Linky meters.
In order to switch to this more verbose mode (that also provides more data), you need to make a request to your energy vendor.
As an alternative, the code can be slightly tweaked to switch to historical mode and to work at 1200 bauds (this is actually the default mode for Linky meters).
In that case, replace 9600 by 1200 in source file [Stm32SerialDriver.cpp](https://github.com/lains/stm32-linky-display/blob/master/src/hal/Stm32SerialDriver.cpp) inside the function called `MX_USART_TIC_UART_Init()`.

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

In order to clone this repository, you may use `git clone --recursive`, this will allow to pull all supported STM32Cube firmware package and the ticdecodecpp library, however, this might take unnecessary time because it will pull firmwares for platforms you don't want to support.

Instead, you can directly pull only the necessary bsp by running `make fetch_bsp fetch_libticdecode`.

### Build & flash

* Run `make TARGET_BOARD=STM32F469I_DISCO all` to build the project for the STM32F469I_DISCO board or
* Run `make TARGET_BOARD=STM32F769I_DISCO all` to build the project for the STM32F769I_DISCO board
  (if you see missing files error, make sure you have run `make fetch_bsp fetch_libticdecode` as a precondition).
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

### Debugging console

A debugging console is available using the Stm32DebugOuput class (singleton)
This serial port is wired to the virtual serial port provided by the ST-Link probe, so it's easy to see debugging messages, directly by reading from the ST-Link virtual serial port (something like /dev/ttyACM0 on Linux).

The following code snippet allows to print out debugging data:
```
#include "Stm32DebugOutput.h"

uint8_t buffer[3] = { 0x01, 0x02, 0x03 }
Stm32DebugOutput& debugSerial = Stm32DebugOutput::get();
debugSerial.send("Sample buffer content: ");
debugSerial.hexdumpBuffer(buffer, sizeof(buffer));
debugSerial.send("\n");
```

### Emulating TIC signal

It is possible to directly wire your STM32 TIC USART port to a PC in order to avoid having to connect to a realy Linky meter.

This can be useful to debug a TIC stream (with or without transmission errors), by first collecting the data, and then replaying it to the embedded code at a later stage, with debug on.

You will need a 3.3V TTL-level serial adapter plugged into your PC.
This often creates a virtual serial device on your PC, like `/dev/ttyUSB0` on Linux.

You can now configure this port with the same config as the Linky meter:
```
stty -F /dev/ttyUSB0 1200 sane evenp parenb cs7 -crtscts
```

> **Note**  
> 1200 is for historical TIC, use 9600 bauds instead for standard TIC

Now, replay a captured TIC stream, for example:
```
cat ticdecodecpp/test/samples/continuous_linky_3P_historical_TIC_with_rx_errors.bin | pv -L 100 >/dev/ttyUSB0
```

> **Note**  
> `pv` allows to give a specific pace for data flow (100 characters par second in the above example)

## Developper guide

### Porting

There are abstraction layers in place to ease porting of this code to another board.

Porting to another STM32 microcontroller will be easier than porting to another manufacturer.

For the STM32F769I-DISCO, I found quite useful to start from a [project that has an valid .ioc file already setup for that board](https://github.com/spark404/STM32F769I-DISCO_Blank_Project.git) concerning all major required peripherals (on that board, this means USART6, LCD and Ethernet).
Although that example uses a RTOS (actually very useful for networking), the ioc file is a good starting point and networking may be supported using active polling in a mainloop.

#### Display support

First, LCD should be taken care of (after all, this whole code is about displaying power usage to a screen).
For this, you can first compile and execute `LCD_DSi_CmdMode_DoubleBuffer` example projects for your specific board and use this as a starting point.

Once this example is running properly on your board, you will have to make the same code compile with the build system provided in my repository. This should be as easy as importing a submodule pointing to the correct STM32CubeXX HAL library (under [bsp/](./bsp)).

Most often, file `stm32xxx_hal_msp.c` from the example project will be irrelevant, and you can skip it.

File `stm32xxx_it.c` should however be used almost as is (it has a `DSI_IRQHandler()` and may also have a `LTDC_IRQHandler()` defined).
You should try to replace arguments `&hdsi` and `&hltdc` in the above function by `get_hdsi()` and `get_hltdc()` provided by my code.
Probably the example's file `system_stm32fxxx.c` can be copied over as is.

Most of the initialisation for double-buffering happens in the example project's `main.c`.
All LCD-related initialisation should thus be moved inside my `main.cpp`.

The content of function `LCD_Init()`, which in turns calls `LTDC_Init()` will both have to be transferred into `Stm32LcdDriver.cpp`'s `LCD_Init()` and `LTDC_Init()`. Note that in my version, `hdsi` and `hltdc` pointers are passed as arguments.

The content of function `HAL_DSI_EndOfRefreshCallback()` will end up into `Stm32LcdDriver.cpp`'s `HAL_DSI_EndOfRefreshCallback()` and `set_active_fb()`


Once the LDC initialisation has been moved to `main.cpp` and `Stm32LcdDriver.cpp`, you can compile and flash the firmware. It should display a blank page and refresh it properly on the screen.

#### Serial port support

Two usages are made from the serial ports on the code:
* reading the TIC stream from the Linky meter
* outputting debug information

In order to read the TIC stream from the meter, you will need to get an available USART RX pin on your board.
Once this is done, you will have to check which STM32 pin it is connected to and find out the corresponding alternate fonction on the datasheet

For this, you can first compile and execute on of the `UART_TwoBoards*` example projects for your specific board and use this as a starting point.

With this information, you will be able to update `Stm32SerialDriver.cpp` with the correct settings (mainly the correct USART to use for both incoming TIC serial bytes and optionally also a debug UART output).

The content of function `HAL_UART_MspInit()` should be moved from `stm32f7xx_hal_msp.c` into  `Stm32SerialDriver.cpp`'s `HAL_UART_MspInit()`.

Also remember to redirect USART interruptions to the proper handler. (in `stm32fxxx_it.c`, `USARTx_IRQHandler(void)` should grab an external reference to the uart pointer via `get_huartx()` and call `HAL_UART_IRQHandler()`.
File `stm32xxx_it.c` should however be used almost as is (it has a `USARTx_IRQHandler()` defined).
You should try to replace arguments `&huartx` in the above function by `get_huartx()` provided by my code.
