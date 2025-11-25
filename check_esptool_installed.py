#!/usr/bin/env python3
import os
import subprocess
import sys
from pathlib import Path

# Path you provided in your command
ESPPY_PATH = Path("/Users/bwjohns4/.platformio/packages/tool-esptoolpy@1.30000.201119/esptool.py")
PENV_PYTHON = Path("/Users/bwjohns4/.platformio/penv/bin/python")

def run_cmd(cmd):
    """Run a command safely and return (success, output)."""
    try:
        result = subprocess.run(
            cmd, check=True, capture_output=True, text=True
        )
        return True, result.stdout.strip()
    except subprocess.CalledProcessError as e:
        return False, e.output if e.output else str(e)
    except Exception as e:
        return False, str(e)

def main():
    print("========================================")
    print(" esptool.py Installation Check")
    print("========================================\n")

    # Check python interpreter exists
    if not PENV_PYTHON.exists():
        print(f"[ERROR] Python interpreter not found:\n  {PENV_PYTHON}")
        sys.exit(1)
    else:
        print(f"[OK] Python interpreter exists:\n  {PENV_PYTHON}")

    # Check esptool.py file existence
    if not ESPPY_PATH.exists():
        print(f"[ERROR] esptool.py not found at expected path:\n  {ESPPY_PATH}")
        sys.exit(1)
    else:
        print(f"[OK] esptool.py file exists:\n  {ESPPY_PATH}")

    # Check esptool.py is readable
    if not os.access(ESPPY_PATH, os.R_OK):
        print(f"[ERROR] esptool.py is not readable! Check permissions.")
        sys.exit(1)
    else:
        print(f"[OK] esptool.py is readable")

    # Try executing: python esptool.py --help
    print("\nRunning esptool.py to confirm it works...\n")
    success, output = run_cmd([str(PENV_PYTHON), str(ESPPY_PATH), "--help"])

    if success:
        print("[SUCCESS] esptool.py is installed and working!\n")
        print(output[:500] + "\n...")
    else:
        print("[ERROR] Could not execute esptool.py:\n")
        print(output)
        sys.exit(1)

    print("\nAll good! You can safely use this esptool in your flashing commands.")

if __name__ == "__main__":
    main()