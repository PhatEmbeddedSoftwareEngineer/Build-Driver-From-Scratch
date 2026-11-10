TARGET := UART_BARE_METAL
BUILD_DIR := build

CC := arm-none-eabi-gcc
CXX := arm-none-eabi-g++
OBJCOPY := arm-none-eabi-objcopy
SIZE := arm-none-eabi-size

CPU_FLAGS := -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard
CPPFLAGS := -ICore/Inc
CFLAGS ?= -std=gnu11 -O0 -g3 -Wall -Wextra
CXXFLAGS ?= -std=gnu++14 -O0 -g3 -Wall -Wextra \
            -fno-exceptions -fno-rtti -fno-use-cxa-atexit
COMMON_FLAGS := $(CPU_FLAGS) -ffunction-sections -fdata-sections
LDFLAGS ?= -Tlink.ld -nostartfiles -Wl,--gc-sections \
           -Wl,-Map=$(BUILD_DIR)/$(TARGET).map --specs=nano.specs -static
LDLIBS ?= -Wl,--start-group -lc -lm -lstdc++ -lsupc++ -Wl,--end-group

C_SOURCES := $(wildcard Core/Src/*.c)
CPP_SOURCES := $(wildcard Core/Src/*.cpp)
OBJECTS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(C_SOURCES)) \
           $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(CPP_SOURCES))
OBJECT_DIRS := $(sort $(BUILD_DIR)/ $(dir $(OBJECTS)))
DEPENDENCIES := $(OBJECTS:.o=.d)

.PHONY: all build clean flash size

all: build

build: $(BUILD_DIR)/$(TARGET).bin

$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS) link.ld
	$(CXX) $(CPU_FLAGS) $(LDFLAGS) $(OBJECTS) $(LDLIBS) -o $@

$(BUILD_DIR)/$(TARGET).bin: $(BUILD_DIR)/$(TARGET).elf
	$(OBJCOPY) -O binary $< $@

$(BUILD_DIR)/$(TARGET).hex: $(BUILD_DIR)/$(TARGET).elf
	$(OBJCOPY) -O ihex $< $@

$(BUILD_DIR)/%.o: %.c | $(OBJECT_DIRS)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(COMMON_FLAGS) -MMD -MP -MF $(@:.o=.d) -c $< -o $@

$(BUILD_DIR)/%.o: %.cpp | $(OBJECT_DIRS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(COMMON_FLAGS) -MMD -MP -MF $(@:.o=.d) -c $< -o $@

ifeq ($(OS),Windows_NT)
ifneq ($(wildcard /bin/sh.exe),)
SHELL := /bin/sh
$(OBJECT_DIRS):
	/usr/bin/mkdir -p "$@"

clean:
	/usr/bin/rm -rf "$(BUILD_DIR)"
else
SHELL := cmd.exe
$(OBJECT_DIRS):
	@if not exist "$(subst /,\,$@)" mkdir "$(subst /,\,$@)"

clean:
	-cmd /C rmdir /S /Q "$(BUILD_DIR)"
endif
else
$(OBJECT_DIRS):
	mkdir -p "$@"

clean:
	rm -rf "$(BUILD_DIR)"
endif

flash: $(BUILD_DIR)/$(TARGET).bin
	st-flash --reset write $< 0x08000000

size: $(BUILD_DIR)/$(TARGET).elf
	$(SIZE) $<

-include $(DEPENDENCIES)
