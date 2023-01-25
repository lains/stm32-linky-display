# Be quiet per default, but 'make V=1' will show all compiler calls.
ifneq ($(V),1)
Q		:= @
NULL		:= 2>/dev/null
endif

TEST_BINARY = test_runner

# User-defined makefile function
# path_simplify removes ./ prefixes, /./ and // occurrences in a path
path_simplify = $(subst //,/,$(subst /./,/,$(1:./%=%)))

# Project specific path
SRC_DIR = ../src/
INC_DIR = ../inc/
TEST_SRC_DIR = ./
# Our own directory compared to the top dir, we will use this to reproduce the correct directory structure in the build directory
OWN_DIR = tests/

# Project target build dirs
TEST_BUILD_PREFIX = build/

TEST_BUILD_OBJ_PREFIX = $(call path_simplify,$(TEST_BUILD_PREFIX)$(OWN_DIR)/)

# Own project sources
SRC_FILES  += ../src/TicUnframer.cpp

TEST_SRC_FILES = $(addprefix $(OWN_DIR),$(shell find $(TEST_SRC_DIR) -name '*.c' -o -name '*.cpp'))

# Project includes
INCLUDES_FILES   = $(INC_DIR)
INCLUDES_FILES  += $(INC_DIR)hal/

# Vendor includes
INCLUDES_FILES_TO_SIMPLIFY = $(INCLUDES_FILES)
INCLUDES_FILES_SIMPLIFIED = $(call path_simplify,$(INCLUDES_FILES_TO_SIMPLIFY))
INCLUDES += $(INCLUDES_FILES_SIMPLIFIED:%=-I%)

# Compiler Flags
CXXFLAGS  = -g -O0 -Wall -Wextra -Warray-bounds -Wno-unused-parameter
CXXFLAGS += -DSTM32F469xx -DUSE_STM32469I_DISCOVERY -DUSE_STM32469I_DISCO_REVB -DUSE_HAL_DRIVER # Board specific defines
CXXFLAGS += $(INCLUDES)

# Linker Flags
#LDLIBS   += -Wl,--start-group -lc -lgcc -lnosys -Wl,--end-group

###############################################################################

# This does an in-source build. An out-of-source build that places all object
# files into a build directory would be a better solution, but the goal was to
# keep this file very simple.

C_SRC_FILES = $(filter %.c, $(TEST_SRC_FILES))
C_OBJS_WITHOUT_PREFIX = $(C_SRC_FILES:.c=.o)
CPP_SRC_FILES = $(filter %.cpp, $(TEST_SRC_FILES))
CPP_OBJS_WITHOUT_PREFIX = $(CPP_SRC_FILES:.cpp=.o)
CXX_OBJS_WITHOUT_PREFIX = $(C_OBJS_WITHOUT_PREFIX) $(CPP_OBJS_WITHOUT_PREFIX)
ALL_OBJS_WITHOUT_PREFIX = $(CXX_OBJS_WITHOUT_PREFIX)
ALL_OBJS_FILES_TO_SIMPLIFY = $(addprefix $(TEST_BUILD_PREFIX)/,$(ALL_OBJS_WITHOUT_PREFIX))
ALL_OBJS_SIMPLIFIED = $(call path_simplify,$(ALL_OBJS_FILES_TO_SIMPLIFY))
ALL_OBJS = $(ALL_OBJS_SIMPLIFIED)

.PRECIOUS: $(TEST_BUILD_PREFIX)/%.o	# Avoid deleting intermediate .o files at the end of make (see https://stackoverflow.com/questions/42830131/an-unexpected-rm-occur-after-make)

.PHONY: clean sanity

all: sanity check

sanity:
	@echo TEST_SRC_FILES=$(TEST_SRC_FILES)
	@echo ALL_OBJS=$(ALL_OBJS)
	@echo TEST_BUILD_OBJ_PREFIX=$(TEST_BUILD_OBJ_PREFIX)

# Compilation targets
$(TEST_BUILD_PREFIX)/src/%.o: %.c
	@echo "  CC      $(call path_simplify,$(*)).c"
	@mkdir -p $(dir $@)
	$(Q)$(CC) $(INCLUDES) $(CXXFLAGS) $(CFLAGS) -o $(call path_simplify,$(@)) -c $<

$(TEST_BUILD_PREFIX)/src/%.o: %.cpp
	@echo "  CXX     $(call path_simplify,$(*)).cpp"
	@mkdir -p $(dir $@)
	$(Q)$(CXX) $(INCLUDES) $(CXXFLAGS) $(CPPFLAGS) -o $(call path_simplify,$(@)) -c $<

$(TEST_BUILD_OBJ_PREFIX)src/TicUnframer_tests.o: src/TicUnframer_tests.cpp
	@mkdir -p $(dir $@)
	$(Q)$(CXX) $(INCLUDES) $(CXXFLAGS) $(CPPFLAGS) -o $(call path_simplify,$(@)) -c $<

$(TEST_BUILD_OBJ_PREFIX)src/main.o: src/main.cpp
	mkdir -p $(dir $@)
	$(Q)$(CXX) $(INCLUDES) $(CXXFLAGS) $(CPPFLAGS) -o $(call path_simplify,$(@)) -c $<

$(TEST_BUILD_OBJ_PREFIX)src/TicUnframer.o: ../src/TicUnframer.cpp
	@mkdir -p $(dir $@)
	$(Q)$(CXX) $(INCLUDES) $(CXXFLAGS) $(CPPFLAGS) -o $(call path_simplify,$(@)) -c $<

$(TEST_BINARY): $(ALL_OBJS) $(TEST_BUILD_OBJ_PREFIX)src/TicUnframer.o
	@echo "  LD      $@"
	@mkdir -p $(dir $@)
	$(Q)$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@

check: $(TEST_BINARY)
	./$<
	
# Clean
clean:
	@rm -f $(ALL_OBJS) $(TEST_BINARY)
