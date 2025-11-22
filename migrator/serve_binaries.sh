#!/bin/bash

# Simple HTTP server to serve IDF binaries for migration
# Usage: ./serve_binaries.sh [port] [build_dir]

PORT=${1:-8700}
BUILD_DIR=${2:-"/Users/byronjohnson/Downloads/build"}

if [ ! -d "$BUILD_DIR" ]; then
    echo "ERROR: Build directory not found: $BUILD_DIR"
    echo "Usage: $0 [port] [build_dir]"
    exit 1
fi

# Create temporary flat directory for serving
SERVE_DIR="/tmp/esp_migrator_bins_$$"
mkdir -p "$SERVE_DIR"

# Find and copy binaries to flat structure
BOOTLOADER="$BUILD_DIR/bootloader/bootloader.bin"
PARTITION="$BUILD_DIR/partitions_hap.bin"
APP="$BUILD_DIR/smart_outlet.bin"

if [ ! -f "$BOOTLOADER" ]; then
    echo "ERROR: bootloader.bin not found at $BOOTLOADER"
    exit 1
fi

if [ ! -f "$PARTITION" ]; then
    echo "ERROR: partitions_hap.bin not found at $PARTITION"
    exit 1
fi

if [ ! -f "$APP" ]; then
    echo "ERROR: smart_outlet.bin not found at $APP"
    exit 1
fi

# Copy files to serve directory
cp "$BOOTLOADER" "$SERVE_DIR/bootloader.bin"
cp "$PARTITION" "$SERVE_DIR/partitions_hap.bin"
cp "$APP" "$SERVE_DIR/smart_outlet.bin"

# Get local IP
LOCAL_IP=$(ipconfig getifaddr en0 2>/dev/null || echo "YOUR_IP_HERE")

echo "========================================="
echo "IDF Binary HTTP Server"
echo "========================================="
echo "Source: $BUILD_DIR"
echo "Serving from: $SERVE_DIR"
echo "Port: $PORT"
echo ""
echo "Available files:"
ls -lh "$SERVE_DIR"/*.bin
echo ""
echo "Server URLs:"
echo "  http://$LOCAL_IP:$PORT/bootloader.bin"
echo "  http://$LOCAL_IP:$PORT/partitions_hap.bin"
echo "  http://$LOCAL_IP:$PORT/smart_outlet.bin"
echo ""
echo "Update migrator/src/main.cpp with this URL:"
echo "  #define BINARY_SERVER_URL \"http://$LOCAL_IP:$PORT\""
echo ""
echo "Press Ctrl+C to stop server"
echo "(Temp directory $SERVE_DIR will be cleaned up on exit)"
echo "========================================="
echo ""

# Cleanup on exit
trap "rm -rf $SERVE_DIR" EXIT

cd "$SERVE_DIR"
python3 -m http.server $PORT
