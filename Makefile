CC      = gcc
CFLAGS  = -Wall -Wextra -Wpedantic -std=c99 -I./include
SRC     = src/main.c src/input.c src/mode.c src/control.c \
          src/fault.c src/state.c src/log.c
TARGET  = vehicle_ecu

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all clean