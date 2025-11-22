# Stage 2 Build Process

## Overview

Stage 2 now uses a **bootloader stub** approach instead of trying to erase all 8 bootloader sectors.

## How It Works

1. **Stub at 0x0**: A tiny bootloader stub (< 4KB) is written to sector 0
2. **Real bootloader at 0x100000**: The full IDF bootloader stays where Stage 1 placed it
3. **Boot sequence**:
   - ESP8266 boot ROM loads stub from 0x0
   - Stub reads real IDF bootloader from 0x100000 into RAM
   - Stub parses and loads IDF bootloader segments
   - Stub jumps to IDF bootloader entry point
   - IDF bootloader continues normal boot

## Build Process

### Single Command Build (Recommended)

Just run:
```bash
cd migrator_stage2
platformio run
```

The build system automatically:
1. Compiles the bootloader stub (in `stub/`)
2. Converts it to a C header (`src/stub_binary.h`)
3. Embeds the stub binary in Stage 2
4. Builds Stage 2 with embedded stub

### Manual Build (If Needed)

If the automatic build fails, you can build manually:

```bash
# 1. Build the stub
cd stub
platformio run
cd ..

# 2. Convert to header
xxd -i stub/.pio/build/stub/firmware.bin | sed 's/unsigned char .*/const unsigned char stub_binary[] = {/' > src/stub_binary.h
echo "const unsigned int stub_binary_len = sizeof(stub_binary);" >> src/stub_binary.h

# 3. Build Stage 2
platformio run
```

## Files

- `stub/stub.cpp` - Bootloader stub source
- `stub/platformio.ini` - Stub build config
- `build_stub.py` - Pre-build script (auto-compiles stub)
- `src/stub_binary.h` - Generated stub binary (auto-created)
- `src/main.cpp` - Stage 2 main code

## Why This Works

Erasing multiple bootloader sectors (0-7) causes crashes due to ESP8266 flash controller issues. By only erasing sector 0 and writing a tiny stub there, we:

- ✅ Avoid multi-sector erase crashes
- ✅ Keep the full IDF bootloader intact at 0x100000
- ✅ Use proven IRAM erase for single sector
- ✅ Let the stub handle loading the real bootloader

The stub is small enough (< 4KB) to fit in sector 0, and uses simple ROM functions to load the real bootloader from a safe location.
