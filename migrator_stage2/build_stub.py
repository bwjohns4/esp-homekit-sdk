#!/usr/bin/env python3
"""
Pre-build script to build minimal bootloader stub
"""
Import("env")
import os
import subprocess
import sys
import shutil

def build_and_embed_stub(source, target, env):
    project_dir = env["PROJECT_DIR"]
    minimal_dir = os.path.join(project_dir, "stub_minimal")
    stub_bin = os.path.join(minimal_dir, "stub_tiny.bin")
    stub_header = os.path.join(project_dir, "src", "stub_binary.h")

    print("=" * 60)
    print("Building minimal bootloader stub...")
    print("=" * 60)

    # Try to build minimal stub
    toolchain = os.path.expanduser("~/.platformio/packages/toolchain-xtensa/bin/xtensa-lx106-elf-gcc")
    if os.path.exists(toolchain):
        print("Found xtensa toolchain, building minimal stub...")

        result = subprocess.run(
            ["bash", "build.sh"],
            cwd=minimal_dir,
            capture_output=True,
            text=True
        )

        if result.returncode == 0 and os.path.exists(stub_bin):
            stub_size = os.path.getsize(stub_bin)
            print(f"Minimal stub built: {stub_size} bytes")

            # Convert to header
            with open(stub_bin, "rb") as f:
                data = f.read()

            with open(stub_header, "w") as f:
                f.write("// Minimal bootloader stub (bare-metal)\n")
                f.write(f"// Size: {len(data)} bytes\n\n")
                f.write("const unsigned char stub_binary[] = {\n")
                for i in range(0, len(data), 16):
                    chunk = data[i:i+16]
                    hex_bytes = ", ".join(f"0x{b:02x}" for b in chunk)
                    f.write(f"  {hex_bytes},\n")
                f.write("};\n")
                f.write(f"const unsigned int stub_binary_len = {len(data)};\n")

            print(f"Stub embedded: {len(data)} bytes")
            print("=" * 60)
            return

    # Fallback: use placeholder (copy real bootloader approach)
    print("No xtensa toolchain found, using bootloader copy approach")
    print("Stage 2 will copy IDF bootloader from 0x100000 to 0x0")
    print("=" * 60)

    with open(stub_header, "w") as f:
        f.write("// Placeholder - will copy real bootloader instead\n")
        f.write("#define COPY_REAL_BOOTLOADER 1\n")
        f.write("const unsigned char stub_binary[] = {0};\n")
        f.write("const unsigned int stub_binary_len = 0;\n")

env.AddPreAction("$BUILD_DIR/src/main.cpp.o", build_and_embed_stub)
