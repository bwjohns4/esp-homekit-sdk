#!/bin/bash
# Quick build script for Ubuntu VM

set -e

if [ ! -d "$HOME/ESP8266_RTOS_SDK" ]; then
    echo "Installing ESP8266-RTOS-SDK..."
    cd ~
    git clone --recursive https://github.com/espressif/ESP8266_RTOS_SDK.git
    cd ESP8266_RTOS_SDK
    ./install.sh
fi

export IDF_PATH=$HOME/ESP8266_RTOS_SDK
source $IDF_PATH/export.sh

# Find the shared folder mount point
if [ -d "/media/psf/Home/Development/esp-homekit-sdk" ]; then
    PROJECT_DIR="/media/psf/Home/Development/esp-homekit-sdk"
elif [ -d "/mnt/hgfs/esp-homekit-sdk" ]; then
    PROJECT_DIR="/mnt/hgfs/esp-homekit-sdk"
else
    # Try to find it
    PROJECT_DIR=$(find /media /mnt -name "esp-homekit-sdk" 2>/dev/null | head -1)
    if [ -z "$PROJECT_DIR" ]; then
        echo "ERROR: Cannot find shared folder"
        echo "Please mount the esp-homekit-sdk folder from macOS"
        exit 1
    fi
fi

echo "Building in: $PROJECT_DIR/examples/keepconnect"
cd "$PROJECT_DIR/examples/keepconnect"

idf.py build

echo ""
echo "Build complete! Binaries:"
ls -lh build/bootloader/bootloader.bin
ls -lh build/partition_table/partition-table.bin
ls -lh build/*.bin | grep -v bootloader | grep -v partition
echo ""
echo "Now on macOS, run:"
echo "  cd migrator"
echo "  ./build_migrator.sh"
