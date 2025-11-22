# Quick Start - KC to HomeKit Migration

Ultra-simplified guide to get your 200+ units migrated ASAP.

## TL;DR

1. Build HomeKit firmware → Get 3 binary files
2. Build migrator → Embeds those binaries
3. OTA deploy migrator → Units rewrite their own flash
4. Done → Units boot into HomeKit firmware

---

## Step 1: Build HomeKit Firmware (One-Time)

### On macOS - Use Native IDF (30 min setup, most reliable)

```bash
# Install dependencies
brew install cmake ninja python3
pip3 install pyparsing==2.3.1

# Get ESP8266-RTOS-SDK (one-time)
cd ~/Development
git clone --recursive https://github.com/espressif/ESP8266_RTOS_SDK.git
cd ESP8266_RTOS_SDK
./install.sh

# Set up environment (do this every terminal session, or add to .zshrc)
export IDF_PATH=~/Development/ESP8266_RTOS_SDK
source $IDF_PATH/export.sh

# Build the firmware
cd /Users/byronjohnson/Development/esp-homekit-sdk/examples/keepconnect
idf.py build

# Done! Binaries are in build/
```

**Output files:**
- `build/bootloader/bootloader.bin` (~25KB)
- `build/partition_table/partition-table.bin` (~3KB)
- `build/smart_outlet.bin` (~500KB-1.5MB)

---

## Step 2: Build Migrator

```bash
cd /Users/byronjohnson/Development/esp-homekit-sdk/migrator

# Run the all-in-one build script
./build_migrator.sh
```

This script:
- Converts the 3 binaries to C arrays
- Compiles migrator firmware with embedded binaries
- Outputs: `.pio/build/esp8266_4mb/firmware.bin`

---

## Step 3: Test on ONE Unit (CRITICAL!)

```bash
# Connect unit via serial
cd /Users/byronjohnson/Development/esp-homekit-sdk/migrator
pio run -t upload
pio device monitor
```

Watch serial output. Should see:
```
Step 1: Validating flash size... ✓
Step 2: Validating embedded binaries... ✓
Step 3: Writing IDF bootloader... ✓
Step 4: Writing partition table... ✓
Step 5: Erasing data partitions... ✓
Step 6: Writing HomeKit app... ✓
MIGRATION COMPLETE! REBOOTING NOW!
```

After reboot → IDF boot logs → HomeKit service starts → Success!

---

## Step 4: Deploy via OTA

```bash
# Upload migrator firmware to your OTA server
scp .pio/build/esp8266_4mb/firmware.bin \
    user@secureapi.johnson-creative.com:/path/to/ota/v2firmware.bin

# Push to 5-10 units via your existing KC OTA mechanism
# Wait for them to migrate and come back online
# Check HomeKit functionality

# If successful, roll out to all 200+ units
```

---

## What Happens During Migration

1. Unit downloads migrator via existing KC OTA
2. Migrator runs once on boot
3. Validates 4MB flash
4. Erases sectors 0x0000-0x20000
5. Writes new bootloader @ 0x0000
6. Writes new partition table @ 0x8000
7. Writes HomeKit app @ 0x20000
8. Reboots
9. New IDF bootloader loads
10. HomeKit app starts from ota_0
11. Unit is now in IDF mode with dual OTA slots

**Time per unit:** ~60 seconds
**Risk:** Power loss = brick (< 5% expected)

---

## Shortcuts & Time Savers

### If PlatformIO Can Build (Test First)
```bash
cd examples/keepconnect
pio run  # May or may not work with complex components

# If successful, binaries in:
# .pio/build/esp8266_4mb_homekit/bootloader.bin
# .pio/build/esp8266_4mb_homekit/partitions.bin
# .pio/build/esp8266_4mb_homekit/firmware.bin
```

### If You Already Built Before
```bash
# Just find your old build directory and copy binaries
find ~ -name "bootloader.bin" -path "*/esp-homekit-sdk/*" 2>/dev/null
```

### Skip Testing (Not Recommended)
If you're feeling lucky and have 5-10 sacrificial units:
- Skip serial testing
- Deploy migrator directly to small batch
- Monitor for successful migration

---

## Expected Results

**Before:**
- KC firmware (Arduino/Non-OS)
- OTA via your custom endpoint
- 2MB flash layout (even though chip is 4MB)

**After:**
- HomeKit firmware (ESP-IDF RTOS)
- OTA via esp_https_ota
- 4MB flash layout with dual OTA (ota_0/ota_1)
- HomeKit HAP service
- NVS for credentials

---

## Troubleshooting Speed Run

**Q: Build fails with "idf.py not found"**
A: `source $IDF_PATH/export.sh`

**Q: Migrator build fails**
A: Did you run `convert_bins.sh` first? Check `src/idf_binaries.cpp` is > 500KB

**Q: Migration fails on unit**
A: Check serial output. LED rapid blinking = error

**Q: Unit won't boot after migration**
A: Serial recovery - reflash via UART

**Q: Do I need Ubuntu?**
A: No, everything works on macOS

---

## Timeline Estimate

- IDF setup: 30 min (one-time)
- Build firmware: 5 min
- Build migrator: 2 min
- Test one unit: 30 min
- Batch test: 1 hour
- Full rollout: 2-4 hours

**Total:** Half a day to migrate 200+ units

---

For detailed docs, see:
- `examples/keepconnect/BUILD_INSTRUCTIONS.md` - Build options
- `migrator/MIGRATION_GUIDE.md` - Deep dive
- `otaPlan.txt` - Original technical plan
