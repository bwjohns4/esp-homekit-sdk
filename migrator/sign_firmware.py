#!/usr/bin/env python3
"""
Post-build script to sign migrator firmware for KC OTA compatibility
"""

import os
import subprocess
import sys

Import("env")

def sign_firmware(source, target, env):
    """Sign the firmware binary after build"""

    # Paths
    firmware_bin = str(target[0])
    build_dir = os.path.dirname(firmware_bin)
    signed_bin = firmware_bin.replace(".bin", "_signed.bin")
    private_key = "/Users/byronjohnson/Development/keepconnect/src/private.key"
    signing_tool = os.path.expanduser("~/.platformio/packages/framework-arduinoespressif8266/tools/signing.py")

    # Also create copies with descriptive names in project root
    project_dir = env.get("PROJECT_DIR")
    unsigned_copy = os.path.join(project_dir, "migrator_unsigned.bin")
    signed_copy = os.path.join(project_dir, "migrator_signed.bin")

    # Check if key exists
    if not os.path.exists(private_key):
        print(f"ERROR: Private key not found at {private_key}")
        sys.exit(1)

    if not os.path.exists(signing_tool):
        print(f"ERROR: Signing tool not found at {signing_tool}")
        sys.exit(1)

    print(f"\n{'='*60}")
    print(f"Signing firmware with KC private key...")
    print(f"Input:  {firmware_bin}")
    print(f"Output: {signed_bin}")
    print(f"{'='*60}\n")

    # Run signing command
    cmd = [
        sys.executable,
        signing_tool,
        "--mode", "sign",
        "--privatekey", private_key,
        "--bin", firmware_bin,
        "--out", signed_bin
    ]

    try:
        result = subprocess.run(cmd, check=True, capture_output=True, text=True)
        print(result.stdout)
        if result.stderr:
            print(result.stderr)

        # Get file sizes
        unsigned_size = os.path.getsize(firmware_bin)
        signed_size = os.path.getsize(signed_bin)

        # Copy both versions to project root with descriptive names
        import shutil
        shutil.copy2(firmware_bin, unsigned_copy)
        shutil.copy2(signed_bin, signed_copy)

        print(f"\n{'='*60}")
        print(f"✓ Firmware built successfully!")
        print(f"\nBuild artifacts in {build_dir}:")
        print(f"  firmware.bin        - {unsigned_size:,} bytes (unsigned)")
        print(f"  firmware_signed.bin - {signed_size:,} bytes (signed)")
        print(f"\nDeployment binaries (copied to project root):")
        print(f"  migrator_unsigned.bin - For older KC units without sig check")
        print(f"  migrator_signed.bin   - For newer KC units with sig check")
        print(f"\nDeploy via KC OTA:")
        print(f"  Newer units: {signed_copy}")
        print(f"  Older units: {unsigned_copy}")
        print(f"{'='*60}\n")

    except subprocess.CalledProcessError as e:
        print(f"ERROR: Signing failed!")
        print(f"stdout: {e.stdout}")
        print(f"stderr: {e.stderr}")
        sys.exit(1)

# Register the post-build action
env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", sign_firmware)
