#!/bin/bash
# Build minimal stub without Arduino framework

set -e

# Find xtensa toolchain (from PlatformIO)
TOOLCHAIN_PATH="${HOME}/.platformio/packages/toolchain-xtensa/bin"
CC="${TOOLCHAIN_PATH}/xtensa-lx106-elf-gcc"
OBJCOPY="${TOOLCHAIN_PATH}/xtensa-lx106-elf-objcopy"

if [ ! -f "$CC" ]; then
    echo "ERROR: xtensa toolchain not found at $TOOLCHAIN_PATH"
    exit 1
fi

echo "Building minimal bootloader stub..."

# Compile to object
$CC -c -Os -mlongcalls -mtext-section-literals \
    -ffunction-sections -fdata-sections \
    -nostdlib -o stub_tiny.o stub_tiny.c

# Link (allow undefined for ROM functions)
$CC -Os -nostdlib -Wl,--gc-sections -Wl,-static \
    -Wl,-T,stub.ld \
    -Wl,--unresolved-symbols=ignore-all \
    -o stub_tiny.elf stub_tiny.o -lgcc

# Extract binary
$OBJCOPY -O binary stub_tiny.elf stub_tiny.bin

SIZE=$(stat -f%z stub_tiny.bin 2>/dev/null || stat -c%s stub_tiny.bin)
echo "Stub size: $SIZE bytes"

if [ $SIZE -gt 4096 ]; then
    echo "WARNING: Stub is larger than 4KB"
fi

echo "Success!"
ls -lh stub_tiny.bin
