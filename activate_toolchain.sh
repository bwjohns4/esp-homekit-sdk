#!/bin/bash
# Activate isolated ESP8266 toolchain
# Run: source ./activate_toolchain.sh

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TOOLS_DIR="$PROJECT_ROOT/.esp-tools"
VENV_DIR="$TOOLS_DIR/python-venv"
SDK_DIR="$TOOLS_DIR/ESP8266_RTOS_SDK"
TOOLCHAIN_DIR="$TOOLS_DIR/xtensa-lx106-elf"

if [ ! -d "$TOOLS_DIR" ]; then
    echo "ERROR: Toolchain not installed. Run ./setup_isolated_toolchain.sh first"
    return 1
fi

echo "Activating isolated ESP8266 toolchain..."
source "$VENV_DIR/bin/activate"
export IDF_PATH="$SDK_DIR"
export PATH="$TOOLCHAIN_DIR/bin:$SDK_DIR/tools:$PATH"

echo "✓ Toolchain activated"
echo "IDF_PATH: $IDF_PATH"
echo ""
echo "To build firmware:"
echo "  cd examples/keepconnect"
echo "  idf.py build"
echo ""
echo "To deactivate:"
echo "  deactivate"
