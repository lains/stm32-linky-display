# Be quiet per default, but 'make V=1' will show all compiler calls.
ifeq ($(V),1)
CMAKE_VERBOSE_OPT := -DCMAKE_VERBOSE_MAKEFILE:BOOL=ON
UT_MAKE_VERBOSE_OPT := VERBOSE=1
else
Q := @
NULL := 2>/dev/null
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
TEST_SUBDIR = test
TEST_DIR = $(TOPDIR)/$(TEST_SUBDIR)

TARGET_BOARD?=STM32F469I_DISCO

# Path to the STM32 codebase, make sure to fetch submodules to populate this directory
BSP_DIR ?= bsp
ifeq ($(TARGET_BOARD),STM32F469I_DISCO)
TARGET_BOARD_BSP_DIR ?= $(BSP_DIR)/STM32CubeF4
VENDOR_ROOT = $(TOPDIR)/$(TARGET_BOARD_BSP_DIR)
HAL_DRIVER_DIR=$(VENDOR_ROOT)/Drivers/STM32F4xx_HAL_Driver
HAL_DRIVER_PREFIX=stm32f4xx
endif
ifeq ($(TARGET_BOARD),STM32F769I_DISCO)
VENDOR_ROOT = $(TOPDIR)/bsp/STM32CubeF7
HAL_DRIVER_DIR=$(VENDOR_ROOT)/Drivers/STM32F7xx_HAL_Driver
HAL_DRIVER_PREFIX=stm32f7xx
endif

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
PROJECT_SRC_FILES = $(shell find $(SRC_DIR)/ -name '*.c' -o -name '*.cpp')
PROJECT_ASM_FILES = $(shell find $(SRC_DIR)/ -name '*.s')
ifeq ($(TARGET_BOARD),STM32F469I_DISCO)
LDSCRIPT = $(SRC_DIR)/device/STM32F469NIHx_FLASH.ld
endif
ifeq ($(TARGET_BOARD),STM32F769I_DISCO)
LDSCRIPT = $(SRC_DIR)/device/STM32F769NIHx_FLASH.ld
endif

# Project includes
PROJECT_INCLUDE_DIRS   = $(INC_DIR)
PROJECT_INCLUDE_DIRS  += $(INC_DIR)/domain
PROJECT_INCLUDE_DIRS  += $(INC_DIR)/hal
PROJECT_INCLUDE_DIRS  += $(TICDECODECPP)/include

# Vendor sources: Note that files in "Templates" are normally copied into project for customization,
# but we direclty use provided source files whenever possible.
ifeq ($(TARGET_BOARD),STM32F469I_DISCO)
BSP_ASM_FILES += $(VENDOR_ROOT)/Drivers/CMSIS/Device/ST/STM32F4xx/Source/Templates/gcc/startup_stm32f469xx.s
BSP_SRC_FILES += $(VENDOR_ROOT)/Drivers/BSP/STM32469I-Discovery/stm32469i_discovery.c
BSP_SRC_FILES += $(VENDOR_ROOT)/Drivers/BSP/STM32469I-Discovery/stm32469i_discovery_sdram.c
BSP_SRC_FILES += $(VENDOR_ROOT)/Drivers/BSP/STM32469I-Discovery/stm32469i_discovery_lcd.c
BSP_SRC_FILES += $(VENDOR_ROOT)/Drivers/BSP/STM32469I-Discovery/stm32469i_discovery_qspi.c
BSP_SRC_FILES += $(VENDOR_ROOT)/Drivers/BSP/Components/nt35510/nt35510.c
BSP_SRC_FILES += $(VENDOR_ROOT)/Drivers/BSP/Components/otm8009a/otm8009a.c
endif
ifeq ($(TARGET_BOARD),STM32F769I_DISCO)
BSP_ASM_FILES += $(VENDOR_ROOT)/Drivers/CMSIS/Device/ST/STM32F7xx/Source/Templates/gcc/startup_stm32f769xx.s
#BSP_SRC_FILES += $(VENDOR_ROOT)/Drivers/CMSIS/Device/ST/STM32F7xx/Source/Templates/system_stm32f7xx.c # We already have a (different) copy in src/
BSP_SRC_FILES += $(VENDOR_ROOT)/Drivers/BSP/STM32F769I-Discovery/stm32f769i_discovery.c
BSP_SRC_FILES += $(VENDOR_ROOT)/Drivers/BSP/STM32F769I-Discovery/stm32f769i_discovery_lcd.c
BSP_SRC_FILES += $(VENDOR_ROOT)/Drivers/BSP/STM32F769I-Discovery/stm32f769i_discovery_ts.c
BSP_SRC_FILES += $(VENDOR_ROOT)/Drivers/BSP/STM32F769I-Discovery/stm32f769i_discovery_qspi.c
BSP_SRC_FILES += $(VENDOR_ROOT)/Drivers/BSP/STM32F769I-Discovery/stm32f769i_discovery_sdram.c
BSP_SRC_FILES += $(VENDOR_ROOT)/Drivers/BSP/Components/wm8994/wm8994.c
BSP_SRC_FILES += $(VENDOR_ROOT)/Drivers/BSP/Components/ft6x06/ft6x06.c
BSP_SRC_FILES += $(VENDOR_ROOT)/Drivers/BSP/Components/otm8009a/otm8009a.c
endif
BSP_SRC_FILES += $(HAL_DRIVER_DIR)/Src/$(HAL_DRIVER_PREFIX)_ll_fmc.c
BSP_SRC_FILES += $(HAL_DRIVER_DIR)/Src/$(HAL_DRIVER_PREFIX)_hal.c
BSP_SRC_FILES += $(HAL_DRIVER_DIR)/Src/$(HAL_DRIVER_PREFIX)_hal_i2c.c
BSP_SRC_FILES += $(HAL_DRIVER_DIR)/Src/$(HAL_DRIVER_PREFIX)_hal_cortex.c
BSP_SRC_FILES += $(HAL_DRIVER_DIR)/Src/$(HAL_DRIVER_PREFIX)_hal_dma.c
BSP_SRC_FILES += $(HAL_DRIVER_DIR)/Src/$(HAL_DRIVER_PREFIX)_hal_gpio.c
BSP_SRC_FILES += $(HAL_DRIVER_DIR)/Src/$(HAL_DRIVER_PREFIX)_hal_pwr.c
BSP_SRC_FILES += $(HAL_DRIVER_DIR)/Src/$(HAL_DRIVER_PREFIX)_hal_pwr_ex.c
BSP_SRC_FILES += $(HAL_DRIVER_DIR)/Src/$(HAL_DRIVER_PREFIX)_hal_rcc.c
BSP_SRC_FILES += $(HAL_DRIVER_DIR)/Src/$(HAL_DRIVER_PREFIX)_hal_rcc_ex.c
BSP_SRC_FILES += $(HAL_DRIVER_DIR)/Src/$(HAL_DRIVER_PREFIX)_hal_uart.c
BSP_SRC_FILES += $(HAL_DRIVER_DIR)/Src/$(HAL_DRIVER_PREFIX)_hal_dsi.c
BSP_SRC_FILES += $(HAL_DRIVER_DIR)/Src/$(HAL_DRIVER_PREFIX)_hal_dma2d.c
BSP_SRC_FILES += $(HAL_DRIVER_DIR)/Src/$(HAL_DRIVER_PREFIX)_hal_ltdc.c
BSP_SRC_FILES += $(HAL_DRIVER_DIR)/Src/$(HAL_DRIVER_PREFIX)_hal_ltdc_ex.c
BSP_SRC_FILES += $(HAL_DRIVER_DIR)/Src/$(HAL_DRIVER_PREFIX)_hal_sdram.c

#libticdecodecpp related source files
PROJECT_SRC_FILES += $(TICDECODECPP)/src/TIC/Unframer.cpp
PROJECT_SRC_FILES += $(TICDECODECPP)/src/TIC/DatasetExtractor.cpp
PROJECT_SRC_FILES += $(TICDECODECPP)/src/TIC/DatasetView.cpp

# Vendor includes
BSP_INCLUDE_DIRS += $(VENDOR_ROOT)/Drivers/CMSIS/Core/Include
ifeq ($(TARGET_BOARD),STM32F469I_DISCO)
BSP_INCLUDE_DIRS += $(VENDOR_ROOT)/Drivers/CMSIS/Device/ST/STM32F4xx/Include
endif
ifeq ($(TARGET_BOARD),STM32F769I_DISCO)
BSP_INCLUDE_DIRS += $(VENDOR_ROOT)/Drivers/CMSIS/Device/ST/STM32F7xx/Include
endif
BSP_INCLUDE_DIRS += $(HAL_DRIVER_DIR)/Inc
ifeq ($(TARGET_BOARD),STM32F469I_DISCO)
BSP_INCLUDE_DIRS += $(VENDOR_ROOT)/Drivers/BSP/STM32469I-Discovery
endif
ifeq ($(TARGET_BOARD),STM32F769I_DISCO)
BSP_INCLUDE_DIRS += $(VENDOR_ROOT)/Drivers/BSP/STM32F769I-Discovery
endif
INCLUDE_DIRS_TO_SIMPLIFY = $(PROJECT_INCLUDE_DIRS) $(BSP_INCLUDE_DIRS)
INCLUDE_DIRS_SIMPLIFIED = $(shell realpath --relative-to $(TOPDIR) $(INCLUDE_DIRS_TO_SIMPLIFY))
INCLUDES += $(INCLUDE_DIRS_SIMPLIFIED:%=-I%)

# Compiler Flags
CXXFLAGS  = -g -O0 -Wall -Wextra -Warray-bounds -Wno-unused-parameter -fno-exceptions
CXXFLAGS += -mcpu=cortex-m7 -mthumb -mlittle-endian -mthumb-interwork
CXXFLAGS += -mfloat-abi=hard -mfpu=fpv4-sp-d16
CXXFLAGS += -DEMBEDDED_DEBUG_CONSOLE
ifeq ($(TARGET_BOARD),STM32F469I_DISCO)
CXXFLAGS += -DSTM32F469xx -DUSE_STM32469I_DISCOVERY -DUSE_STM32469I_DISCO_REVB -DUSE_HAL_DRIVER # Board specific defines
endif
ifeq ($(TARGET_BOARD),STM32F769I_DISCO)
CXXFLAGS += -DSTM32F769xx -DUSE_STM32F769I_DISCO -DUSE_HAL_DRIVER -DTS_MULTI_TOUCH_SUPPORTED # Board specific defines
endif
CXXFLAGS += $(INCLUDES)

# Linker Flags
LDFLAGS   = -Wl,--gc-sections -Wl,-T$(LDSCRIPT) --specs=rdimon.specs
LDLIBS   += -Wl,--start-group -lc -lgcc -Wl,--end-group
ifeq ($(TARGET_BOARD),STM32F469I_DISCO)
LDLIBS   += -lnosys
endif

###############################################################################

# This does an in-source build. An out-of-source build that places all object
# files into a build directory would be a better solution, but the goal was to
# keep this file very simple.

SRC_FILES = $(PROJECT_SRC_FILES) $(BSP_SRC_FILES)
ASM_FILES = $(PROJECT_ASM_FILES) $(BSP_ASM_FILES)
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

all: sanity fetch_bsp fetch_libticdecode elf

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

fetch_bsp: $(TARGET_BOARD_BSP_DIR) $(TOPDIR)/.gitmodules
	@echo "  GIT     $(<)"
	@git submodule init $(<) && git submodule update

fetch_libticdecode: ticdecodecpp $(TOPDIR)/.gitmodules
	@echo "  GIT     $(<)"
	@git submodule init $(<) && git submodule update

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
check: unit_testing
	./unit_testing

UT_BUILD_DIR=cmake-ut-build
UT_BUILT_EXEC=$(UT_BUILD_DIR)/$(TEST_SUBDIR)/unit_testing

unit_testing: $(UT_BUILT_EXEC)
	@cmp --quiet $< $@ || cp $< $@

#FIXME: also depend on all headers taken into account in INCLUDES
$(UT_BUILT_EXEC): $(PROJECT_SRC_FILES) $(PROJECT_ASM_FILES)
	$(Q)cmake $(CMAKE_VERBOSE_OPT) -B $(UT_BUILD_DIR)/
	$(Q)make -j $(nproc) -C $(UT_BUILD_DIR) $(UT_MAKE_VERBOSE_OPT)

# Program using st-flash utility
flash: $(SRC_BUILD_PREFIX)/$(BINARY).hex
	@echo "  FLASH   $(<)"
	$(STFLASH) --format ihex write $<

# Clean
clean:
	@rm -f $(ALL_OBJS) $(GENERATED_BINARIES)
	@rm -rf $(UT_BUILD_DIR)
	@rm -f $(UT_BUILT_EXEC)

# Debug
gdb-server_stlink:
	st-util

gdb-server_openocd:
	openocd -f ./openocd.cfg

gdb-client: $(BINARY).elf
	$(CROSS_GDB) -tui $(TARGET)
