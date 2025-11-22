#!/usr/bin/env python3
"""
Create a simple bootloader stub binary
This creates a minimal ESP8266 image that just prints a message and hangs
The real bootloader will be at 0x100000 (left in place by Stage 1)
"""

import struct

# ESP8266 image header
header = bytearray([
    0xE9,  # magic
    0x00,  # segment count (0 = no segments, just hang)
    0x02,  # SPI mode (DIO)
    0x00,  # SPI speed/size (40MHz, 512KB)
])

# Entry point (doesn't matter if no segments)
header += struct.pack('<I', 0x40100000)

# Checksum (simple: 0xEF)
header += bytes([0xEF])

# Pad to 16 bytes
while len(header) < 16:
    header.append(0xFF)

# Write stub
with open("stub/stub_minimal.bin", "wb") as f:
    f.write(header)
    # Pad to 256 bytes for safety
    f.write(b'\xFF' * (256 - len(header)))

print(f"Created stub: {len(header)} bytes (padded to 256)")

# Convert to C header
with open("src/stub_binary.h", "w") as f:
    f.write("// Minimal bootloader stub\n")
    f.write("// NOTE: This is a placeholder. Real boot requires IDF bootloader at 0x100000\n\n")
    f.write("const unsigned char stub_binary[] = {\n")

    data = header + (b'\xFF' * (256 - len(header)))
    for i in range(0, len(data), 16):
        chunk = data[i:i+16]
        hex_bytes = ", ".join(f"0x{b:02x}" for b in chunk)
        f.write(f"  {hex_bytes},\n")

    f.write("};\n")
    f.write(f"const unsigned int stub_binary_len = {len(data)};\n")

print("Generated src/stub_binary.h")
