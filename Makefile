# Be quiet per default, but 'make V=1' will show all compiler calls.
ifneq ($(V),1)
Q		:= @
NULL		:= 2>/dev/null
endif

CROSS_PREFIX    ?= arm-none-eabi

# Path you your toolchain installation, leave empty if already in system PATH
TOOLCHAIN_PATH_PREFIX ?= /opt/st/gcc-arm-none-eabi-10.3-2021.10/bin/

###############################################################################

BINARY = stm32-linky

# Project specific path
THIS_MAKEFILE_PATH := $(abspath $(lastword $(MAKEFILE_LIST)))
THIS_MAKEFILE_DIR := $(patsubst %/,%,$(dir $(THIS_MAKEFILE_PATH)))
TOPDIR = $(shell realpath $(THIS_MAKEFILE_DIR))
SRC_DIR = $(TOPDIR)/src
INC_DIR = $(TOPDIR)/inc
TEST_DIR = $(TOPDIR)/tests

# Path to the STM32 codebase, make sure to fetch submodules to populate this directory
VENDOR_ROOT = $(TOPDIR)/bsp/STM32CubeF4

# Path to the ticdecodecpp library, make sure to fetch submodules to populate this directory
TICDECODECPP = $(TOPDIR)/ticdecodecpp


# Toolchain
CROSS_CC        := $(TOOLCHAIN_PATH_PREFIX)$(CROSS_PREFIX)-gcc
CROSS_CXX       := $(TOOLCHAIN_PATH_PREFIX)$(CROSS_PREFIX)-g++
CROSS_LD        := $(TOOLCHAIN_PATH_PREFIX)$(CROSS_PREFIX)-gcc
CROSS_AR        := $(TOOLCHAIN_PATH_PREFIX)$(CROSS_PREFIX)-ar
CROSS_AS        := $(TOOLCHAIN_PATH_PREFIX)$(CROSS_PREFIX)-as
CROSS_OBJCOPY   := $(TOOLCHAIN_PATH_PREFIX)$(CROSS_PREFIX)-objcopy
CROSS_OBJDUMP   := $(TOOLCHAIN_PATH_PREFIX)$(CROSS_PREFIX)-objdump
CROSS_GDB       := $(TOOLCHAIN_PATH_PREFIX)$(CROSS_PREFIX)-gdb
STFLASH         := $(shell which st-flash)

# Project target build dirs
SRC_BUILD_PREFIX = build

# Own project sources
SRC_FILES = $(shell find $(SRC_DIR)/ -name '*.c' -o -name '*.cpp')
ASM_FILES = $(shell find $(SRC_DIR)/ -name '*.s')
LDSCRIPT = $(SRC_DIR)/device/STM32F469NIHx_FLASH.ld

# Project includes
INCLUDES_FILES   = $(INC_DIR)
INCLUDES_FILES  += $(INC_DIR)/hal
INCLUDES_FILES  += $(TICDECODECPP)/include

# Vendor sources: Note that files in "Templates" are normally copied into project for customization,
# but we direclty use provided source files whenever possible.
ASM_FILES += $(VENDOR_ROOT)/Drivers/CMSIS/Device/ST/STM32F4xx/Source/Templates/gcc/startup_stm32f469xx.s
SRC_FILES += $(VENDOR_ROOT)/Drivers/BSP/STM32469I-Discovery/stm32469i_discovery.c
SRC_FILES += $(VENDOR_ROOT)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_ll_fmc.c
SRC_FILES += $(VENDOR_ROOT)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal.c
SRC_FILES += $(VENDOR_ROOT)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_i2c.c
SRC_FILES += $(VENDOR_ROOT)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_cortex.c
SRC_FILES += $(VENDOR_ROOT)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dma.c
#SRC_FILES += $(VENDOR_ROOT)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_exti.c
#SRC_FILES += $(VENDOR_ROOT)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash.c
SRC_FILES += $(VENDOR_ROOT)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_gpio.c
SRC_FILES += $(VENDOR_ROOT)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pwr.c
SRC_FILES += $(VENDOR_ROOT)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pwr_ex.c
SRC_FILES += $(VENDOR_ROOT)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc.c
SRC_FILES += $(VENDOR_ROOT)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc_ex.c
SRC_FILES += $(VENDOR_ROOT)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_uart.c
SRC_FILES += $(VENDOR_ROOT)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dsi.c
SRC_FILES += $(VENDOR_ROOT)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dma2d.c
SRC_FILES += $(VENDOR_ROOT)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_ltdc.c
SRC_FILES += $(VENDOR_ROOT)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_ltdc_ex.c
SRC_FILES += $(VENDOR_ROOT)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_sdram.c
SRC_FILES += $(VENDOR_ROOT)/Drivers/BSP/Components/nt35510/nt35510.c
SRC_FILES += $(VENDOR_ROOT)/Drivers/BSP/Components/otm8009a/otm8009a.c
SRC_FILES += $(VENDOR_ROOT)/Drivers/BSP/STM32469I-Discovery/stm32469i_discovery_sdram.c
SRC_FILES += $(VENDOR_ROOT)/Drivers/BSP/STM32469I-Discovery/stm32469i_discovery_lcd.c
SRC_FILES += $(VENDOR_ROOT)/Drivers/BSP/STM32469I-Discovery/stm32469i_discovery_qspi.c

#libticdecodecpp related source files
SRC_FILES += $(TICDECODECPP)/src/TIC/Unframer.cpp
SRC_FILES += $(TICDECODECPP)/src/TIC/DatasetExtractor.cpp
SRC_FILES += $(TICDECODECPP)/src/TIC/DatasetView.cpp

# Vendor includes
INCLUDES_FILES += $(VENDOR_ROOT)/Drivers/CMSIS/Core/Include
INCLUDES_FILES += $(VENDOR_ROOT)/Drivers/CMSIS/Device/ST/STM32F4xx/Include
INCLUDES_FILES += $(VENDOR_ROOT)/Drivers/STM32F4xx_HAL_Driver/Inc
INCLUDES_FILES += $(VENDOR_ROOT)/Drivers/BSP/STM32469I-Discovery
INCLUDES_FILES_TO_SIMPLIFY = $(INCLUDES_FILES)
INCLUDES_FILES_SIMPLIFIED = $(shell realpath --relative-to $(TOPDIR) $(INCLUDES_FILES_TO_SIMPLIFY))
INCLUDES += $(INCLUDES_FILES_SIMPLIFIED:%=-I%)

# Compiler Flags
CXXFLAGS  = -g -O0 -Wall -Wextra -Warray-bounds -Wno-unused-parameter -fno-exceptions
CXXFLAGS += -mcpu=cortex-m7 -mthumb -mlittle-endian -mthumb-interwork
CXXFLAGS += -mfloat-abi=hard -mfpu=fpv4-sp-d16
CXXFLAGS += -DSTM32F469xx -DUSE_STM32469I_DISCOVERY -DUSE_STM32469I_DISCO_REVB -DUSE_HAL_DRIVER # Board specific defines
CXXFLAGS += $(INCLUDES)

# Linker Flags
LDFLAGS   = -Wl,--gc-sections -Wl,-T$(LDSCRIPT) --specs=rdimon.specs
LDLIBS   += -Wl,--start-group -lc -lgcc -lnosys -Wl,--end-group

###############################################################################

# This does an in-source build. An out-of-source build that places all object
# files into a build directory would be a better solution, but the goal was to
# keep this file very simple.

C_SRC_FILES = $(shell realpath --relative-to $(TOPDIR) $(filter %.c, $(SRC_FILES)))
C_OBJS_WITHOUT_PREFIX = $(C_SRC_FILES:.c=.o)
CPP_SRC_FILES = $(shell realpath --relative-to $(TOPDIR) $(filter %.cpp, $(SRC_FILES)))
CPP_OBJS_WITHOUT_PREFIX = $(shell realpath -m --relative-to $(TOPDIR) $(CPP_SRC_FILES:.cpp=.o))
CXX_OBJS_WITHOUT_PREFIX = $(shell realpath -m --relative-to $(TOPDIR) $(C_OBJS_WITHOUT_PREFIX) $(CPP_OBJS_WITHOUT_PREFIX))
ASM_OBJS_WITHOUT_PREFIX = $(shell realpath -m --relative-to $(TOPDIR) $(ASM_FILES:.s=.o))
ALL_OBJS_WITHOUT_PREFIX = $(ASM_OBJS_WITHOUT_PREFIX) $(CXX_OBJS_WITHOUT_PREFIX)
ALL_OBJS_FILES_TO_SIMPLIFY = $(addprefix $(SRC_BUILD_PREFIX)/,$(ALL_OBJS_WITHOUT_PREFIX))
ALL_OBJS_SIMPLIFIED = $(shell realpath -m --relative-to $(TOPDIR) $(ALL_OBJS_FILES_TO_SIMPLIFY))
ALL_OBJS = $(ALL_OBJS_SIMPLIFIED)

.PRECIOUS: $(SRC_BUILD_PREFIX)/%.o	# Avoid deleting intermediate .o files at the end of make (see https://stackoverflow.com/questions/42830131/an-unexpected-rm-occur-after-make)

.PHONY: clean gdb-server_stlink gdb-server_openocd gdb-client

all: sanity elf

elf: $(SRC_BUILD_PREFIX)/$(BINARY).elf
bin: $(SRC_BUILD_PREFIX)/$(BINARY).bin
hex: $(SRC_BUILD_PREFIX)/$(BINARY).hex
srec: $(SRC_BUILD_PREFIX)/$(BINARY).srec
list: $(SRC_BUILD_PREFIX)/$(BINARY).list
map: $(SRC_BUILD_PREFIX)/$(BINARY).map

GENERATED_BINARIES=$(SRC_BUILD_PREFIX)/$(BINARY).elf \
	$(SRC_BUILD_PREFIX)/$(BINARY).bin \
	$(SRC_BUILD_PREFIX)/$(BINARY).hex \
	$(SRC_BUILD_PREFIX)/$(BINARY).srec \
	$(SRC_BUILD_PREFIX)/$(BINARY).list \
	$(SRC_BUILD_PREFIX)/$(BINARY).map

sanity:

# Cross-compilation targets
$(SRC_BUILD_PREFIX)/%.o: %.s
	@echo "  CC      $(<)"
	@mkdir -p $(dir $@)
	$(Q)$(CROSS_CC) $(CFLAGS) -o $@ -c $<

$(SRC_BUILD_PREFIX)/%.o: %.c
	@echo "  CC      $(<)"
	@mkdir -p $(dir $@)
	$(Q)$(CROSS_CC) $(INCLUDES) $(CXXFLAGS) $(CFLAGS) -o $(@) -c $<

$(SRC_BUILD_PREFIX)/%.o: %.cpp
	@echo "  CXX     $(<)"
	@mkdir -p $(dir $@)
	$(Q)$(CROSS_CXX) $(INCLUDES) $(CXXFLAGS) $(CPPFLAGS) -o $(@) -c $<

$(SRC_BUILD_PREFIX)/$(BINARY).elf: $(ALL_OBJS_FILES_TO_SIMPLIFY) $(LDSCRIPT)
	@echo "  LD      $(@)"
	@mkdir -p $(dir $@)
	$(Q)$(CROSS_CC) $(CXXFLAGS) $(LDFLAGS) $(ALL_OBJS) $(LDLIBS) -o $(@)

$(SRC_BUILD_PREFIX)/%.bin: $(SRC_BUILD_PREFIX)/%.elf
	@echo "  OBJCOPY $(@)"
	@mkdir -p $(dir $@)
	$(Q)$(CROSS_OBJCOPY) -Obinary $< $@

$(SRC_BUILD_PREFIX)/%.hex: $(SRC_BUILD_PREFIX)/%.elf
	@echo "  OBJCOPY $(@)"
	@mkdir -p $(dir $@)
	$(Q)$(CROSS_OBJCOPY) -Oihex $< $@

# Unit tests
check:
	$(MAKE) -C $(TOPDIR)/test check

# Program using st-flash utility
flash: $(SRC_BUILD_PREFIX)/$(BINARY).hex
	@echo "  FLASH   $(<)"
	$(STFLASH) --format ihex write $<

# Clean
clean:
	@rm -f $(ALL_OBJS) $(GENERATED_BINARIES)

# Debug
gdb-server_stlink:
	st-util

gdb-server_openocd:
	openocd -f ./openocd.cfg

gdb-client: $(BINARY).elf
	$(CROSS_GDB) -tui $(TARGET)
