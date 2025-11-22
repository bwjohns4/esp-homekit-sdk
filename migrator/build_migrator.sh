#!/bin/bash

# Complete build script for the KC→HomeKit migrator
# This script builds the IDF firmware, converts binaries, and builds the migrator

set -e

echo "========================================"
echo "KC to HomeKit Migrator - Full Build"
echo "========================================"
echo ""

# Step 1: Check if IDF firmware build exists
KC_EXAMPLE_DIR="../examples/keepconnect"
BUILD_DIR="$KC_EXAMPLE_DIR/build"

if [ ! -d "$BUILD_DIR" ]; then
    echo "ERROR: No build directory found at $BUILD_DIR"
    echo ""
    echo "You need to build the HomeKit firmware first."
    echo ""
    echo "To build the HomeKit firmware:"
    echo "  1. Set up ESP8266-RTOS-SDK environment:"
    echo "     export IDF_PATH=/path/to/ESP8266_RTOS_SDK"
    echo "     source \$IDF_PATH/export.sh"
    echo ""
    echo "  2. Build the keepconnect example:"
    echo "     cd $KC_EXAMPLE_DIR"
    echo "     idf.py build"
    echo ""
    echo "  3. Re-run this script"
    echo ""
    exit 1
fi

# Check for required binaries
BOOTLOADER="$BUILD_DIR/bootloader/bootloader.bin"
PARTITION_TABLE="$BUILD_DIR/partition_table/partition-table.bin"

# Try to find the app binary (name might vary)
if [ -f "$BUILD_DIR/smart_outlet.bin" ]; then
    APP_BIN="$BUILD_DIR/smart_outlet.bin"
elif [ -f "$BUILD_DIR/keepconnect.bin" ]; then
    APP_BIN="$BUILD_DIR/keepconnect.bin"
else
    # Find any .bin file in build root that's not bootloader or partition
    APP_BIN=$(find "$BUILD_DIR" -maxdepth 1 -name "*.bin" -type f | head -1)
fi

if [ ! -f "$BOOTLOADER" ]; then
    echo "ERROR: Bootloader not found at $BOOTLOADER"
    exit 1
fi

if [ ! -f "$PARTITION_TABLE" ]; then
    echo "ERROR: Partition table not found at $PARTITION_TABLE"
    exit 1
fi

if [ ! -f "$APP_BIN" ]; then
    echo "ERROR: App binary not found in $BUILD_DIR"
    echo "Looked for: smart_outlet.bin, keepconnect.bin, or any .bin file"
    exit 1
fi

echo "Found IDF firmware binaries:"
echo "  Bootloader:      $BOOTLOADER"
echo "  Partition table: $PARTITION_TABLE"
echo "  App binary:      $APP_BIN"
echo ""

# Step 2: Convert binaries to C arrays
echo "Step 1: Converting binaries to C arrays..."
./convert_bins.sh "$BOOTLOADER" "$PARTITION_TABLE" "$APP_BIN"
echo ""

# Step 3: Build migrator firmware
echo "Step 2: Building migrator firmware..."
echo ""

if ! command -v pio &> /dev/null; then
    echo "ERROR: PlatformIO (pio) not found!"
    echo ""
    echo "Install PlatformIO:"
    echo "  pip install platformio"
    echo "  or"
    echo "  brew install platformio"
    echo ""
    exit 1
fi

pio run

echo ""
echo "========================================"
echo "BUILD COMPLETE!"
echo "========================================"
echo ""
echo "Migrator firmware binary:"
echo "  .pio/build/esp8266_4mb/firmware.bin"
echo ""
echo "Next steps:"
echo "  1. Test on ONE sacrificial unit first via serial:"
echo "     pio run -t upload"
echo "     pio device monitor"
echo ""
echo "  2. If successful, deploy via your existing KC OTA:"
echo "     - Upload firmware.bin to your OTA server"
echo "     - Push to a small batch (5-10 units)"
echo "     - Monitor for successful migration"
echo "     - Roll out to all 200+ units"
echo ""
echo "WARNING: Power loss during migration = brick"
echo "         Acceptable for one-time conversion"
echo ""
