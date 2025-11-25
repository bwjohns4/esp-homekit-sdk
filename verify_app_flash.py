#!/usr/bin/env python3
"""
verify_app_flash.py

Read back the app region from an ESP8266 and compare it to a local .bin file.

Usage:
    python verify_app_flash.py --port /dev/cu.usbserial-0001 \
                               --bin ./smart_outlet.bin \
                               --addr 0x220000
"""

import argparse
import os
import subprocess
import sys
import tempfile
import binascii

def run_esptool_read(port: str, addr: int, size: int, out_path: str):
    """
    Use esptool.py to read `size` bytes from flash at `addr` into out_path.
    """
    addr_hex = f"0x{addr:x}"
    size_hex = f"0x{size:x}"

    cmd = [
        "esptool.py",
        "--chip", "esp8266",
        "--port", port,
        "read_flash",
        addr_hex,
        size_hex,
        out_path,
    ]
    print(f"[+] Running: {' '.join(cmd)}")
    result = subprocess.run(cmd)
    if result.returncode != 0:
        print("[-] esptool.py read_flash failed!", file=sys.stderr)
        sys.exit(result.code)

def compare_files(file1: str, file2: str, max_mismatches: int = 10):
    """
    Compare two files byte-by-byte. Print first few mismatches.
    Return True if identical, False otherwise.
    """
    size1 = os.path.getsize(file1)
    size2 = os.path.getsize(file2)
    if size1 != size2:
        print(f"[-] Size mismatch: {file1}={size1} bytes, {file2}={size2} bytes")

    mismatches = 0
    with open(file1, "rb") as f1, open(file2, "rb") as f2:
        offset = 0
        while True:
            b1 = f1.read(4096)
            b2 = f2.read(4096)
            if not b1 and not b2:
                break
            for i, (c1, c2) in enumerate(zip(b1, b2)):
                if c1 != c2:
                    if mismatches < max_mismatches:
                        print(f"Mismatch at 0x{offset + i:08X}: "
                              f"{c1:02X} != {c2:02X}")
                    mismatches += 1
            offset += len(b1)
            # If one file is longer, count remaining bytes as mismatches
            if len(b1) != len(b2):
                extra = abs(len(b1) - len(b2))
                mismatches += extra
                print(f"[-] One file has {extra} extra bytes starting at 0x{offset:08X}")
                break

    if mismatches == 0:
        print("[+] Files are identical.")
        return True
    else:
        print(f"[-] Total mismatches: {mismatches}")
        return False

def main():
    parser = argparse.ArgumentParser(description="Verify ESP8266 app flash region against a local .bin file.")
    parser.add_argument("--port", required=True, help="Serial port for ESP8266 (e.g. /dev/cu.usbserial-0001 or COM3)")
    parser.add_argument("--bin", required=True, help="Path to local .bin file (e.g. smart_outlet.bin)")
    parser.add_argument("--addr", required=True, help="Flash address to read from (hex, e.g. 0x220000)")
    parser.add_argument("--size", help="Optional size in bytes to read (default: size of .bin file)", type=lambda x: int(x, 0))
    args = parser.parse_args()

    bin_path = args.bin
    if not os.path.isfile(bin_path):
        print(f"[-] Bin file not found: {bin_path}", file=sys.stderr)
        sys.exit(1)

    # Determine how many bytes to read
    bin_size = os.path.getsize(bin_path)
    read_size = args.size if args.size is not None else bin_size
    addr = int(args.addr, 0)

    print(f"[+] Local bin: {bin_path} ({bin_size} bytes)")
    print(f"[+] Will read {read_size} bytes from flash @ 0x{addr:x}")

    # Create a temporary file to store the dump
    with tempfile.TemporaryDirectory() as tmpdir:
        dump_path = os.path.join(tmpdir, "flash_dump.bin")
        run_esptool_read(args.port, addr, read_size, dump_path)

        print(f"[+] Flash dump saved to: {dump_path}")
        print("[+] Comparing flash dump to local bin...")
        match = compare_files(bin_path, dump_path)

        if not match:
            print("[-] FLASH CONTENT DOES NOT MATCH .bin file!")
            sys.exit(2)
        else:
            print("[✓] FLASH CONTENT MATCHES .bin file.")
            sys.exit(0)

if __name__ == "__main__":
    main()