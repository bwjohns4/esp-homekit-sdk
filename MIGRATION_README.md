# ESP8266 KC → IDF HomeKit OTA Migration Guide

## Overview

This migration uses a **two-stage OTA approach** to safely migrate from KC firmware to ESP-IDF HomeKit firmware without physical access to the device.

## Why Two Stages?

**The Problem**: The Stage 1 migrator is 294KB and runs from flash starting at 0x0. We cannot safely erase/write to low flash addresses (bootloader at 0x0, partition table at 0x8000) without corrupting our own running code.

**The Solution**:
- **Stage 1** downloads everything to temp storage and writes a tiny Stage 2 app to OTA_0 (0x20000)
- **Stage 2** runs from OTA_0, which is ABOVE the low addresses it needs to write, making it safe!

## Migration Process

### Step 1: Build Stage 2 Finisher

```bash
cd migrator_stage2
./build.sh
```

This creates `stage2.bin` (~30KB)

### Step 2: Serve Binaries via HTTP

Make sure your HTTP server has:
- `bootloader.bin` (IDF bootloader)
- `partitions_hap.bin` (IDF partition table)
- `smart_outlet.bin` (HomeKit app)
- `stage2.bin` (Stage 2 finisher)

### Step 3: Flash Stage 1 to Device

```bash
cd migrator
platformio run -e esp8266_4mb --target upload
```

### Step 4: Monitor Stage 1

Stage 1 will:
1. Connect to WiFi
2. Download bootloader to 0x300000 (temp storage)
3. Download partition table to 0x301000 (temp storage)
4. Download final app to 0x310000 (temp storage)
5. Prompt you to press any key when stage2.bin is ready
6. Download and write Stage 2 to OTA_0 (0x20000)
7. Reboot

**IMPORTANT**: At step 5, make sure `stage2.bin` is being served, then press any key in serial monitor.

### Step 5: Stage 2 Runs Automatically

After reboot, Stage 2 will:
1. Run from OTA_0 at 0x20000 (safe location!)
2. Copy bootloader from 0x300000 → 0x0
3. Copy partition table from 0x301000 → 0x8000
4. Copy final app from 0x310000 → 0x20000
5. Reboot into IDF HomeKit firmware!

## Memory Map

### Before Migration (KC/Arduino)
```
0x00000: Arduino bootloader
0x01000: App starts here (294KB migrator runs from here)
...
```

### Temp Storage (Stage 1)
```
0x300000: IDF bootloader (staged)
0x301000: IDF partition table (staged)
0x310000: HomeKit app (staged)
```

### After Migration (IDF)
```
0x00000: IDF bootloader
0x08000: IDF partition table
0x0D000: sec_cert (12KB)
0x10000: nvs (24KB)
0x16000: otadata (8KB)
0x18000: phy_init (4KB)
0x20000: ota_0 (1600KB) ← HomeKit app here
0x1B0000: ota_1 (1600KB)
0x340000: fctry_nvs (24KB)
0x346000: nvs_keys (4KB)
```

## Safety Features

- **No code self-corruption**: Stage 2 runs from high flash
- **Recoverable**: Even if power loss occurs, IDF bootloader can recover
- **Fully OTA**: No physical access needed
- **Tested watchdog handling**: Uses `optimistic_yield()` like Arduino Updater

## Troubleshooting

### Stage 1 crashes at partition table download
- Expected! We skip this now and use Stage 2 instead

### Stage 2 shows errors
- Check that temp storage downloads completed in Stage 1
- Verify flash size is 4MB

### Device won't boot after migration
- Flash IDF bootloader manually via UART as fallback

## Technical Details

**Why OTA_0 at 0x20000 works for Stage 2**:
- It's at 128KB, above all critical low-flash areas
- Can safely write to 0x0 (bootloader) and 0x8000 (partition table)
- When it overwrites itself at the end, all critical data is already written
- IDF bootloader takes over on reboot

**Flash write safety**:
- `optimistic_yield(10000)` prevents hardware watchdog timeouts
- 500ms delays between sector erases
- Based on Arduino Updater's proven approach
