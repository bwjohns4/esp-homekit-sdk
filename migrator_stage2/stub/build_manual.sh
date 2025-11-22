#!/bin/bash
# Manual build script for bootloader stub

set -e

CC=xtensa-lx106-elf-gcc
OBJCOPY=xtensa-lx106-elf-objcopy
SIZE=xtensa-lx106-elf-size

echo "Building minimal bootloader stub..."

$CC -Os -mlongcalls -mtext-section-literals \
    -ffunction-sections -fdata-sections \
    -nostdlib -Wl,--gc-sections \
    -Wl,-static -Wl,-T,stub.ld \
    -o stub.elf src/stub.cpp -lgcc

echo "Creating binary..."
$OBJCOPY -O binary stub.elf stub.bin

echo "Size:"
$SIZE stub.elf
ls -lh stub.bin

STUB_SIZE=$(stat -f%z stub.bin 2>/dev/null || stat -c%s stub.bin)
if [ $STUB_SIZE -gt 4096 ]; then
    echo "ERROR: Stub too large ($STUB_SIZE > 4096 bytes)"
    exit 1
fi

echo "SUCCESS: Stub is $STUB_SIZE bytes"

# Convert to header
echo "Generating header file..."
cd ..
xxd -i stub/stub.bin | sed 's/unsigned char .*/const unsigned char stub_binary[] = {/' > src/stub_binary.h
echo "const unsigned int stub_binary_len = $STUB_SIZE;" >> src/stub_binary.h

echo "Done! Stub embedded in src/stub_binary.h"
