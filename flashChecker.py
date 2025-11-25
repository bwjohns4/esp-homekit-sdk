#!/usr/bin/env python3
import argparse
import binascii
import subprocess
import tempfile
import sys
from pathlib import Path

# Regions we care about
REGIONS = {
    "bootloader_live":     (0x000000,  0x1000),   # first 4KB at 0x0
    "part_table_live":     (0x0008000, 0x40),     # 64 bytes at 0x8000
    "bootloader_staged":   (0x00100000, 0x1000),  # 4KB at 0x100000
    "part_table_staged":   (0x00110000, 0x40),    # 64 bytes at 0x110000
    "app_staged":          (0x00120000, 0x40),    # 64 bytes at 0x120000
    "app_dest_ota1":       (0x001B0000, 0x40),    # 64 bytes at 0x1B0000
}

def run_esptool_read(port: str, addr: int, length: int) -> bytes:
    """
    Use esptool.py to read a chunk of flash into a temp file and return its bytes.
    """
    with tempfile.NamedTemporaryFile(delete=False) as tf:
        tf_path = Path(tf.name)
    cmd = [
        "esptool.py",
        "--port", port,
        "read_flash",
        f"0x{addr:06X}",
        f"0x{length:X}",
        str(tf_path)
    ]
    print(f"\n[esptool] {' '.join(cmd)}")
    try:
        subprocess.run(cmd, check=True)
    except subprocess.CalledProcessError as e:
        print(f"ERROR: esptool.py read_flash failed: {e}", file=sys.stderr)
        tf_path.unlink(missing_ok=True)
        sys.exit(1)

    data = tf_path.read_bytes()
    tf_path.unlink(missing_ok=True)
    if len(data) != length:
        print(f"WARNING: expected {length} bytes, got {len(data)} bytes", file=sys.stderr)
    return data

def hexdump_block(label: str, data: bytes, width: int = 16):
    print(f"\n=== {label} (len={len(data)}) ===")
    for i in range(0, len(data), width):
        chunk = data[i:i+width]
        hex_part = " ".join(f"{b:02X}" for b in chunk)
        ascii_part = "".join(chr(b) if 32 <= b <= 126 else "." for b in chunk)
        print(f"{i:04X}: {hex_part:<{width*3}}  {ascii_part}")

def compare_prefix(label: str, a: bytes, b: bytes, length: int = 32):
    la = a[:length]
    lb = b[:length]
    same = la == lb
    print(f"\n--- Compare {label} (first {length} bytes) ---")
    print(f"Equal? {'YES' if same else 'NO'}")
    if not same:
        print("A:", " ".join(f"{x:02X}" for x in la))
        print("B:", " ".join(f"{x:02X}" for x in lb))
    return same

def main():
    parser = argparse.ArgumentParser(
        description="ESP8266 flash layout checker for bootloader / partition / app."
    )
    parser.add_argument(
        "--port", "-p",
        required=True,
        help="Serial port for esptool (e.g. /dev/cu.usbserial-0001 or COM3)"
    )
    args = parser.parse_args()
    port = args.port

    print("========================================")
    print(" ESP8266 Flash Layout Diagnostic Checker")
    print("========================================")
    print(f"Using port: {port}")

    # Read all regions
    data = {}
    for name, (addr, length) in REGIONS.items():
        data[name] = run_esptool_read(port, addr, length)

    # Hexdump key regions
    hexdump_block("Live partition table @ 0x8000", data["part_table_live"])
    hexdump_block("Staged partition table @ 0x110000", data["part_table_staged"])

    hexdump_block("Live bootloader @ 0x000000 (first 256 bytes)", data["bootloader_live"][:256])
    hexdump_block("Staged bootloader @ 0x100000 (first 256 bytes)", data["bootloader_staged"][:256])

    hexdump_block("Staged app @ 0x120000 (first 64 bytes)", data["app_staged"])
    hexdump_block("App dest OTA_1 @ 0x1B0000 (first 64 bytes)", data["app_dest_ota1"])

    # Comparisons
    compare_prefix("partition staged vs live", data["part_table_staged"], data["part_table_live"], length=32)
    compare_prefix("bootloader staged vs live", data["bootloader_staged"], data["bootloader_live"], length=64)
    compare_prefix("app staged vs OTA_1", data["app_staged"], data["app_dest_ota1"], length=64)

    print("\nDone. Review the hexdumps and comparisons above.")

if __name__ == "__main__":
    main()