# Be quiet per default, but 'make V=1' will show all compiler calls.
ifneq ($(V),1)
Q		:= @
NULL		:= 2>/dev/null
endif

# Path you your toolchain installation, leave empty if already in system PATH
TOOLCHAIN_ROOT = /opt/st/gcc-arm-none-eabi-10.3-2021.10/bin/

# Path to the STM32 codebase, make sure to update the submodule to get the code
VENDOR_ROOT = ./bsp/STM32CubeF4/

###############################################################################

# Project specific
HEX_IMAGE = main.hex
TARGET = $(HEX_IMAGE:.hex=.elf)
SRC_DIR = src/
INC_DIR = inc/

# Toolchain
CC = $(TOOLCHAIN_ROOT)arm-none-eabi-gcc
CXX = $(CC)
CPP = $(TOOLCHAIN_ROOT)arm-none-eabi-g++
DB = $(TOOLCHAIN_ROOT)arm-none-eabi-gdb

# Project sources
SRC_FILES = $(wildcard $(SRC_DIR)*.c) $(wildcard $(SRC_DIR)*/*.c) $(wildcard $(SRC_DIR)*.cpp) $(wildcard $(SRC_DIR)*/*.cpp)
ASM_FILES = $(wildcard $(SRC_DIR)*.s) $(wildcard $(SRC_DIR)*/*.s)
LDSCRIPT = $(SRC_DIR)/device/STM32F469NIHx_FLASH.ld

# Project includes
INCLUDES   = -I$(INC_DIR)
INCLUDES  += -I$(INC_DIR)hal/

# Vendor sources: Note that files in "Templates" are normally copied into project for customization,
# but that is not necessary for this simple project.
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
INCLUDES += -I$(VENDOR_ROOT)Drivers/CMSIS/Core/Include
INCLUDES += -I$(VENDOR_ROOT)Drivers/CMSIS/Device/ST/STM32F4xx/Include
INCLUDES += -I$(VENDOR_ROOT)Drivers/STM32F4xx_HAL_Driver/Inc
INCLUDES += -I$(VENDOR_ROOT)Drivers/BSP/STM32469I-Discovery

# Compiler Flags
CXXFLAGS  = -g -O0 -Wall -Wextra -Warray-bounds -Wno-unused-parameter
CXXFLAGS += -mcpu=cortex-m7 -mthumb -mlittle-endian -mthumb-interwork
CXXFLAGS += -mfloat-abi=hard -mfpu=fpv4-sp-d16
CXXFLAGS += -DSTM32F469xx -DUSE_STM32469I_DISCOVERY -DUSE_STM32469I_DISCO_REVB -DUSE_HAL_DRIVER # Board specific defines
CXXFLAGS += $(INCLUDES)


# Linker Flags
LDFLAGS = -Wl,--gc-sections -Wl,-T$(LDSCRIPT) --specs=rdimon.specs

###############################################################################

# This does an in-source build. An out-of-source build that places all object
# files into a build directory would be a better solution, but the goal was to
# keep this file very simple.

C_SRC_FILES = $(filter %.c, $(SRC_FILES))
CPP_SRC_FILES = $(filter %.cpp, $(SRC_FILES))
C_OBJS = $(C_SRC_FILES:.c=.o)
CPP_OBJS = $(CPP_SRC_FILES:.cpp=.o)
CXX_OBJS = $(C_OBJS) $(CPP_OBJS)
ASM_OBJS = $(ASM_FILES:.s=.o)
ALL_OBJS = $(ASM_OBJS) $(CXX_OBJS)

.PRECIOUS: %.o	# Avoid deleting intermediate .o files at the end of make (see https://stackoverflow.com/questions/42830131/an-unexpected-rm-occur-after-make)

.PHONY: clean gdb-server_stlink gdb-server_openocd gdb-client

all: $(TARGET)

# Compile

%.o: %.s
	@echo "  CC      $(*).s"
	$(Q)$(CC) $(CFLAGS) -o $(*).o -c $(*).s

%.o: %.c
	@echo "  CC      $(*).c"
	$(Q)$(CC) $(INCLUDES) $(CXXFLAGS) $(CFLAGS) -o $(*).o -c $(*).c

%.o: %.cxx
	@echo "  CXX     $(*).cxx"
	$(Q)$(CXX) $(INCLUDES) $(CXXFLAGS) $(CPPFLAGS) -o $(*).o -c $(*).cxx

%.o: %.cpp
	@echo "  CPP     $(*).cpp"
	$(Q)$(CPP) $(INCLUDES) $(CXXFLAGS) $(CPPFLAGS) -o $(*).o -c $(*).cpp

# Link
%.elf: $(ALL_OBJS) $(LDSCRIPT)
	@echo "  LD      $(*).elf"
	$(Q)$(CC) $(CXXFLAGS) $(LDFLAGS) $(ALL_OBJS) $(LDLIBS) -o $(*).elf

%.hex: %.elf
	arm-none-eabi-objcopy -O ihex $< $@

# Program using st-flash utility
flash: $(HEX_IMAGE)
	$(ST_FLASH_PREFIX)st-flash --format ihex write $<

# Clean
clean:
	rm -f $(ALL_OBJS) $(TARGET)

# Debug
gdb-server_stlink:
	st-util

gdb-server_openocd:
	openocd -f ./openocd.cfg

gdb-client: $(TARGET)
	$(DB) -tui $(TARGET)
