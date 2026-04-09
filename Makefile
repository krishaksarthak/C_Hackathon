CC       := gcc
CFLAGS   := -Wall -Wextra -Wpedantic -std=c99 -Iinclude
LDFLAGS  :=
SRC      := $(wildcard src/*.c)
BUILD_DIR := build
OBJ      := $(patsubst src/%.c,$(BUILD_DIR)/%.o,$(SRC))
DEP      := $(OBJ:.o=.d)

TARGET_BASE := ecu_sim

# Platform detection
ifeq ($(OS),Windows_NT)
    TARGET := $(TARGET_BASE).exe
    RM_FILE := del /Q "$(TARGET)" 2>nul || exit 0
    RM_DIR  := rmdir /S /Q "$(BUILD_DIR)" 2>nul || exit 0
    MKDIR   := if not exist "$(BUILD_DIR)" mkdir "$(BUILD_DIR)"
    RUN_CMD := .\$(TARGET)
else
    TARGET := $(TARGET_BASE)
    RM_FILE := rm -f $(TARGET)
    RM_DIR  := rm -rf $(BUILD_DIR)
    MKDIR   := mkdir -p $(BUILD_DIR)
    RUN_CMD := ./$(TARGET)
endif

.PHONY: all run clean rebuild

all: $(TARGET)

# Create build directory (cross-platform)
$(BUILD_DIR):
	$(MKDIR)

# Link
$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $@ $(LDFLAGS)

# Compile
$(BUILD_DIR)/%.o: src/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

run: $(TARGET)
	$(RUN_CMD)

clean:
	-$(RM_FILE)
	-$(RM_DIR)

rebuild: clean all

-include $(DEP)
