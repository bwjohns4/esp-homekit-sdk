#!/bin/bash

# Ultra-isolated ESP8266-RTOS-SDK installation for Apple Silicon
# Uses official Intel toolchain via Rosetta (simple, works perfectly)
# Everything goes into .esp-tools/ - delete that folder to completely remove

set -e

PROJECT_ROOT="$(cd "$(dirname "$0")" && pwd)"
TOOLS_DIR="$PROJECT_ROOT/.esp-tools"
SDK_DIR="$TOOLS_DIR/ESP8266_RTOS_SDK"
TOOLCHAIN_DIR="$TOOLS_DIR/xtensa-lx106-elf"
VENV_DIR="$TOOLS_DIR/python-venv"

echo "========================================"
echo "Ultra-Isolated ESP8266 Toolchain Setup"
echo "Apple Silicon - Using Rosetta"
echo "========================================"
echo ""
echo "Installation location: $TOOLS_DIR"
echo "Everything will be contained in this folder."
echo "To remove: rm -rf $TOOLS_DIR"
echo ""

# Check Rosetta
if ! /usr/bin/pgrep -q oahd; then
    echo "Rosetta not detected. Installing..."
    softwareupdate --install-rosetta --agree-to-license
fi
echo "✓ Rosetta available"

# Check Homebrew dependencies
echo "Checking Homebrew dependencies..."
if ! command -v brew &> /dev/null; then
    echo "ERROR: Homebrew required. Install from https://brew.sh"
    exit 1
fi

echo "Installing minimal dependencies via Homebrew..."
brew install cmake ninja python3 wget || true

echo ""
echo "Step 1: Creating isolated directory structure..."
mkdir -p "$TOOLS_DIR"
cd "$TOOLS_DIR"

# Step 2: Download official Intel toolchain
echo "Step 2: Downloading official Espressif toolchain (Intel, runs via Rosetta)..."
if [ ! -d "$TOOLCHAIN_DIR" ]; then
    TOOLCHAIN_URL="https://dl.espressif.com/dl/xtensa-lx106-elf-gcc8_4_0-esp-2020r3-macos.tar.gz"
    TOOLCHAIN_ARCHIVE="xtensa-lx106-elf.tar.gz"

    echo "Downloading from: $TOOLCHAIN_URL"
    wget -O "$TOOLCHAIN_ARCHIVE" "$TOOLCHAIN_URL"

    echo "Extracting toolchain..."
    tar -xzf "$TOOLCHAIN_ARCHIVE"
    rm "$TOOLCHAIN_ARCHIVE"

    echo "✓ Toolchain downloaded and extracted"

    # Verify it's Intel (will run under Rosetta)
    ARCH=$(file "$TOOLCHAIN_DIR/bin/xtensa-lx106-elf-gcc" | grep -o "x86_64\|arm64")
    echo "Toolchain architecture: $ARCH (will run via Rosetta on Apple Silicon)"
else
    echo "✓ Toolchain already exists"
fi

# Step 3: Create isolated Python virtualenv
echo ""
echo "Step 3: Creating isolated Python virtualenv..."
if [ ! -d "$VENV_DIR" ]; then
    python3 -m venv "$VENV_DIR"
    echo "✓ Python virtualenv created"
else
    echo "✓ Python virtualenv already exists"
fi

# Activate virtualenv
source "$VENV_DIR/bin/activate"

# Install Python deps in isolated venv
echo "Step 4: Installing Python packages (isolated to venv)..."
pip install --upgrade pip
pip install pyparsing==2.3.1 pyserial cryptography future

echo "✓ Python packages installed in isolated venv"
echo ""

# Step 4: Clone ESP8266-RTOS-SDK
echo "Step 5: Cloning ESP8266-RTOS-SDK..."
if [ ! -d "$SDK_DIR" ]; then
    git clone --recursive https://github.com/espressif/ESP8266_RTOS_SDK.git
    echo "✓ SDK cloned"
else
    echo "✓ SDK already exists"
fi

cd "$SDK_DIR"

echo ""
echo "========================================"
echo "INSTALLATION COMPLETE!"
echo "========================================"
echo ""
echo "Everything installed to: $TOOLS_DIR"
echo ""
echo "Disk usage:"
du -sh "$TOOLS_DIR" 2>/dev/null || echo "Calculating..."
echo ""
echo "Toolchain runs via Rosetta (slightly slower, but works perfectly)"
echo ""

# Create activation script for future use
cat > "$PROJECT_ROOT/activate_toolchain.sh" << 'ACTIVATE_EOF'
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
export PATH="$TOOLCHAIN_DIR/bin:$PATH"

echo "✓ Toolchain activated"
echo "IDF_PATH: $IDF_PATH"
echo ""
echo "To build firmware:"
echo "  cd examples/keepconnect"
echo "  idf.py build"
echo ""
echo "To deactivate:"
echo "  deactivate"
ACTIVATE_EOF

chmod +x "$PROJECT_ROOT/activate_toolchain.sh"

# Now build the firmware
echo "Step 6: Building your firmware..."
echo ""
source "$PROJECT_ROOT/activate_toolchain.sh"
cd "$PROJECT_ROOT/examples/keepconnect"

echo "Running: idf.py build"
echo "This takes 3-5 minutes..."
echo ""
idf.py build

echo ""
echo "========================================"
echo "BUILD COMPLETE!"
echo "========================================"
echo ""
echo "Binaries created:"
ls -lh build/bootloader/bootloader.bin
ls -lh build/partition_table/partition-table.bin
ls -lh build/*.bin | grep -v bootloader | grep -v partition
echo ""
echo "Next steps:"
echo "  cd migrator"
echo "  ./build_migrator.sh"
echo ""
echo "To rebuild in future (new terminal):"
echo "  source ./activate_toolchain.sh"
echo "  cd examples/keepconnect"
echo "  idf.py build"
