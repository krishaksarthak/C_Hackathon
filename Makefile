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
    TARGET  := $(TARGET_BASE).exe
    RM_FILE := del /Q "$(TARGET)" 2>nul || exit 0
    RM_DIR  := rmdir /S /Q "$(BUILD_DIR)" 2>nul || exit 0
    MKDIR_BUILD := if not exist "$(BUILD_DIR)" mkdir "$(BUILD_DIR)"
    MKDIR_LOGS  := if not exist "logs" mkdir "logs"
    RUN_CMD := .\$(TARGET)
    UI_CMD  := python ui.py
else
    TARGET  := $(TARGET_BASE)
    RM_FILE := rm -f $(TARGET)
    RM_DIR  := rm -rf $(BUILD_DIR)
    MKDIR_BUILD := mkdir -p $(BUILD_DIR)
    MKDIR_LOGS  := mkdir -p logs
    RUN_CMD := ./$(TARGET)
    UI_CMD  := python3 ui.py
endif

.PHONY: all run ui clean rebuild logs

all: logs $(TARGET)

# Ensure logs directory exists at build time
logs:
	$(MKDIR_LOGS)

# Link
$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $@ $(LDFLAGS)

# Compile + auto-generate dependency files
$(BUILD_DIR)/%.o: src/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

$(BUILD_DIR):
	$(MKDIR_BUILD)

# Run the C simulator directly in terminal
run: all
	$(RUN_CMD)

# Launch the Python UI (starts C process internally)
ui: all
	$(UI_CMD)

clean:
	-$(RM_FILE)
	-$(RM_DIR)

rebuild: clean all

-include $(DEP)