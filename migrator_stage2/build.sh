#!/bin/bash
set -e

echo "Building Stage 2 Finisher..."
cd "$(dirname "$0")"

platformio run -e esp8266_stage2

echo ""
echo "✓ Build complete!"
echo "Binary: .pio/build/esp8266_stage2/firmware.bin"

# Show size
SIZE=$(ls -lh .pio/build/esp8266_stage2/firmware.bin | awk '{print $5}')
echo "Size: $SIZE"

if [ -d "../http_server" ]; then
    echo ""
    echo "Copying to HTTP server directory..."
    cp .pio/build/esp8266_stage2/firmware.bin ../http_server/stage2.bin
    echo "✓ Copied to http_server/stage2.bin"
fi

echo ""
echo "Ready for Stage 1 to download!"
