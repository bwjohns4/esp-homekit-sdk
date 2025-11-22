# Minimal ESP8266 bootloader stub Makefile
# Requires ESP8266 RTOS SDK or tools

SDK_BASE ?= $(ESP8266_RTOS_SDK_PATH)
XTENSA_TOOLS ?= xtensa-lx106-elf

CC = $(XTENSA_TOOLS)-gcc
LD = $(XTENSA_TOOLS)-ld
OBJCOPY = $(XTENSA_TOOLS)-objcopy
SIZE = $(XTENSA_TOOLS)-size

CFLAGS = -Os -mlongcalls -mtext-section-literals -ffunction-sections -fdata-sections
LDFLAGS = -nostdlib -Wl,--gc-sections -Wl,-static -Wl,--start-group -lgcc -Wl,--end-group

SRC = src/stub.c
OBJ = build/stub.o
ELF = build/stub.elf
BIN = build/bootloader_stub.bin

all: $(BIN)

$(OBJ): $(SRC)
	@mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@

$(ELF): $(OBJ)
	$(LD) $(LDFLAGS) -T stub.ld -o $@ $<

$(BIN): $(ELF)
	$(OBJCOPY) -O binary $< $@
	$(SIZE) $(ELF)
	@echo "Binary size:"
	@ls -lh $(BIN)
	@if [ `stat -f%z $(BIN)` -gt 4096 ]; then \
		echo "ERROR: Binary is larger than 4KB!"; \
		exit 1; \
	else \
		echo "OK: Binary fits in 4KB (sector 0)"; \
	fi

clean:
	rm -rf build

.PHONY: all clean
