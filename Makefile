# Be quiet per default, but 'make V=1' will show all compiler calls.
ifneq ($(V),1)
Q		:= @
NULL		:= 2>/dev/null
endif

PREFIX		?= arm-none-eabi

# Path you your toolchain installation, leave empty if already in system PATH
TOOLCHAIN_ROOT = /opt/st/gcc-arm-none-eabi-10.3-2021.10/bin

# Path to the STM32 codebase, make sure to update the submodule to get the code
VENDOR_ROOT = ./bsp/STM32CubeF4/

###############################################################################

BINARY=stm32-linky
TEST_BINARY = test_runner

# User-defined makefile function
# path_simplify removes ./ prefixes, /./ and // occurrences in a path
path_simplify = $(subst //,/,$(subst /./,/,$(1:./%=%)))

# Project specific
SRC_DIR = src/
TEST_SRC_DIR = test/src/
INC_DIR = inc/

# Toolchain
CC		:= $(TOOLCHAIN_ROOT)/$(PREFIX)-gcc
CXX		:= $(TOOLCHAIN_ROOT)/$(PREFIX)-g++
LD		:= $(TOOLCHAIN_ROOT)/$(PREFIX)-gcc
AR		:= $(TOOLCHAIN_ROOT)/$(PREFIX)-ar
AS		:= $(TOOLCHAIN_ROOT)/$(PREFIX)-as
OBJCOPY		:= $(TOOLCHAIN_ROOT)/$(PREFIX)-objcopy
OBJDUMP		:= $(TOOLCHAIN_ROOT)/$(PREFIX)-objdump
GDB		:= $(TOOLCHAIN_ROOT)/$(PREFIX)-gdb
STFLASH		= $(shell which st-flash)

DB = $(TOOLCHAIN_ROOT)arm-none-eabi-gdb

# Project target build dir
SRC_BUILD_PREFIX = build/

# Own project sources
SRC_FILES = $(shell find $(SRC_DIR) -name '*.c' -o -name '*.cpp')
ASM_FILES = $(shell find $(SRC_DIR) -name '*.s')
LDSCRIPT = $(SRC_DIR)/device/STM32F469NIHx_FLASH.ld

# Own projet unit test sources
TEST_SRCS_FILES += $(shell find $(SRC_DIR) -name '*.c' -o -name '*.cpp')

# Project includes
INCLUDES_FILES   = $(INC_DIR)
INCLUDES_FILES  += $(INC_DIR)hal/

# Vendor sources: Note that files in "Templates" are normally copied into project for customization,
# but we direclty use provided source files whenever possible.
ASM_FILES += $(VENDOR_ROOT)Drivers/CMSIS/Device/ST/STM32F4xx/Source/Templates/gcc/startup_stm32f469xx.s
SRC_FILES += $(VENDOR_ROOT)Drivers/BSP/STM32469I-Discovery/stm32469i_discovery.c
SRC_FILES += $(VENDOR_ROOT)Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_ll_fmc.c
SRC_FILES += $(VENDOR_ROOT)Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal.c
SRC_FILES += $(VENDOR_ROOT)Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_i2c.c
SRC_FILES += $(VENDOR_ROOT)Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_cortex.c
SRC_FILES += $(VENDOR_ROOT)Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dma.c
#SRC_FILES += $(VENDOR_ROOT)Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_exti.c
#SRC_FILES += $(VENDOR_ROOT)Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash.c
SRC_FILES += $(VENDOR_ROOT)Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_gpio.c
SRC_FILES += $(VENDOR_ROOT)Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pwr.c
SRC_FILES += $(VENDOR_ROOT)Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pwr_ex.c
SRC_FILES += $(VENDOR_ROOT)Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc.c
SRC_FILES += $(VENDOR_ROOT)Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc_ex.c
SRC_FILES += $(VENDOR_ROOT)Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_uart.c
SRC_FILES += $(VENDOR_ROOT)Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dsi.c
SRC_FILES += $(VENDOR_ROOT)Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dma2d.c
SRC_FILES += $(VENDOR_ROOT)Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_ltdc.c
SRC_FILES += $(VENDOR_ROOT)Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_ltdc_ex.c
SRC_FILES += $(VENDOR_ROOT)Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_sdram.c
SRC_FILES += $(VENDOR_ROOT)Drivers/BSP/Components/nt35510/nt35510.c
SRC_FILES += $(VENDOR_ROOT)Drivers/BSP/Components/otm8009a/otm8009a.c
SRC_FILES += $(VENDOR_ROOT)Drivers/BSP/STM32469I-Discovery/stm32469i_discovery_sdram.c
SRC_FILES += $(VENDOR_ROOT)Drivers/BSP/STM32469I-Discovery/stm32469i_discovery_lcd.c
SRC_FILES += $(VENDOR_ROOT)Drivers/BSP/STM32469I-Discovery/stm32469i_discovery_qspi.c



# Vendor includes
INCLUDES_FILES += $(VENDOR_ROOT)Drivers/CMSIS/Core/Include
INCLUDES_FILES += $(VENDOR_ROOT)Drivers/CMSIS/Device/ST/STM32F4xx/Include
INCLUDES_FILES += $(VENDOR_ROOT)Drivers/STM32F4xx_HAL_Driver/Inc
INCLUDES_FILES += $(VENDOR_ROOT)Drivers/BSP/STM32469I-Discovery
INCLUDES_FILES_TO_SIMPLIFY = $(INCLUDES_FILES)
INCLUDES_FILES_SIMPLIFIED = $(call path_simplify,$(INCLUDES_FILES_TO_SIMPLIFY))
INCLUDES += $(INCLUDES_FILES_SIMPLIFIED:%=-I%)

# Compiler Flags
CXXFLAGS  = -g -O0 -Wall -Wextra -Warray-bounds -Wno-unused-parameter
CXXFLAGS += -mcpu=cortex-m7 -mthumb -mlittle-endian -mthumb-interwork
CXXFLAGS += -mfloat-abi=hard -mfpu=fpv4-sp-d16
CXXFLAGS += -DSTM32F469xx -DUSE_STM32469I_DISCOVERY -DUSE_STM32469I_DISCO_REVB -DUSE_HAL_DRIVER # Board specific defines
CXXFLAGS += $(INCLUDES)

# Linker Flags
LDFLAGS = -Wl,--gc-sections -Wl,-T$(LDSCRIPT) --specs=rdimon.specs
LDLIBS		+= -Wl,--start-group -lc -lgcc -lnosys -Wl,--end-group

###############################################################################

# This does an in-source build. An out-of-source build that places all object
# files into a build directory would be a better solution, but the goal was to
# keep this file very simple.

C_SRC_FILES = $(filter %.c, $(SRC_FILES))
C_OBJS_WITHOUT_PREFIX = $(C_SRC_FILES:.c=.o)
CPP_SRC_FILES = $(filter %.cpp, $(SRC_FILES))
CPP_OBJS_WITHOUT_PREFIX = $(CPP_SRC_FILES:.cpp=.o)
CXX_OBJS_WITHOUT_PREFIX = $(C_OBJS_WITHOUT_PREFIX) $(CPP_OBJS_WITHOUT_PREFIX)
ASM_OBJS_WITHOUT_PREFIX = $(ASM_FILES:.s=.o)
ALL_OBJS_WITHOUT_PREFIX  =  $(ASM_OBJS_WITHOUT_PREFIX) $(CXX_OBJS_WITHOUT_PREFIX)
ALL_OBJS_FILES_TO_SIMPLIFY = $(ALL_OBJS_WITHOUT_PREFIX:%=$(SRC_BUILD_PREFIX)/%)
ALL_OBJS_SIMPLIFIED = $(call path_simplify,$(ALL_OBJS_FILES_TO_SIMPLIFY))
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

# Compile

$(SRC_BUILD_PREFIX)/%.o: %.s
	@echo "  CC      $(call path_simplify,$(*)).s"
	@mkdir -p $(dir $@)
	$(Q)$(CC) $(CFLAGS) -o $@ -c $<

$(SRC_BUILD_PREFIX)/%.o: %.c
	@echo "  CC      $(call path_simplify,$(*)).c"
	@mkdir -p $(dir $@)
	$(Q)$(CC) $(INCLUDES) $(CXXFLAGS) $(CFLAGS) -o $(call path_simplify,$(@)) -c $<

$(SRC_BUILD_PREFIX)/%.o: %.cpp
	@echo "  CXX     $(call path_simplify,$(*)).cpp"
	@mkdir -p $(dir $@)
	$(Q)$(CXX) $(INCLUDES) $(CXXFLAGS) $(CPPFLAGS) -o $(call path_simplify,$(@)) -c $<

$(SRC_BUILD_PREFIX)/%.elf: $(ALL_OBJS_FILES_TO_SIMPLIFY) $(LDSCRIPT)
	@echo "  LD      $(call path_simplify,$(*)).elf"
	@mkdir -p $(dir $@)
	$(Q)$(CC) $(CXXFLAGS) $(LDFLAGS) $(ALL_OBJS) $(LDLIBS) -o $(call path_simplify,$(@))

$(SRC_BUILD_PREFIX)/%.bin: %.elf
	@echo "  OBJCOPY $(*).bin"
	@mkdir -p $(dir $@)
	$(Q)$(OBJCOPY) -Obinary $(*).elf $(*).bin

$(SRC_BUILD_PREFIX)/%.hex: %.elf
	@echo "  OBJCOPY $(*).hex"
	@mkdir -p $(dir $@)
	$(Q)$(OBJCOPY) -Oihex $(*).elf $(*).hex

# Program using st-flash utility
flash: $(BINARY).hex
	@echo "  FLASH   $<"
	$(ST_FLASH_PREFIX)st-flash --format ihex write $<

# Clean
clean:
	@rm -f $(ALL_OBJS) $(GENERATED_BINARIES) $(TEST_BINARY)

# Debug
gdb-server_stlink:
	st-util

gdb-server_openocd:
	openocd -f ./openocd.cfg

gdb-client: $(BINARY).elf
	$(DB) -tui $(TARGET)
