# KC to HomeKit Migration Guide

Complete guide for migrating 200+ Keep Connect ESP8266 units from Arduino/Non-OS SDK to ESP-IDF HomeKit firmware via OTA.

## Background

Your existing KC units:
- Run Arduino/Non-OS SDK firmware
- Use `eagle.flash.2m.ld` linker script (2MB layout on 4MB chips)
- Have OTA capability via your existing infrastructure

Target HomeKit firmware:
- ESP8266-RTOS-SDK (IDF style)
- Proper partition table with dual OTA slots
- 4MB flash layout with ota_0 @ 0x20000, ota_1 @ 0x1B0000
- HomeKit/HAP support with NVS storage

## Migration Strategy

One-time "migrator" firmware that:
1. Runs on old KC firmware (Arduino SDK)
2. Embeds the new IDF firmware components
3. Erases and rewrites flash to IDF layout
4. Reboots into new HomeKit firmware

## Prerequisites

### Software Requirements

1. **ESP8266-RTOS-SDK** (for building HomeKit firmware)
   ```bash
   git clone --recursive https://github.com/espressif/ESP8266_RTOS_SDK.git
   export IDF_PATH=/path/to/ESP8266_RTOS_SDK
   source $IDF_PATH/export.sh
   ```

2. **PlatformIO** (for building migrator firmware)
   ```bash
   pip install platformio
   # or
   brew install platformio
   ```

3. **xxd** (for binary to C array conversion - usually pre-installed on macOS/Linux)

### Hardware Requirements

- At least ONE sacrificial KC unit for testing
- Serial connection capability (UART) for initial testing
- Your existing OTA infrastructure for deployment

## Step-by-Step Process

### Phase 1: Build the HomeKit Firmware

```bash
cd examples/keepconnect

# Configure if needed (should already be set for 4MB)
# idf.py menuconfig
# Verify: Serial flasher config → Flash size → 4MB
# Verify: Partition Table → Custom → partitions_hap.csv

# Build
idf.py build

# Verify output
ls -lh build/bootloader/bootloader.bin
ls -lh build/partition_table/partition-table.bin
ls -lh build/*.bin  # Your app binary
```

Expected sizes:
- Bootloader: ~20-30 KB
- Partition table: ~3 KB
- App: 300KB - 1.5MB (must be < 1600KB)

### Phase 2: Build the Migrator Firmware

```bash
cd migrator

# Option A: Use the helper script (recommended)
./build_migrator.sh

# Option B: Manual steps
./convert_bins.sh \
    ../examples/keepconnect/build/bootloader/bootloader.bin \
    ../examples/keepconnect/build/partition_table/partition-table.bin \
    ../examples/keepconnect/build/smart_outlet.bin

pio run
```

Output: `.pio/build/esp8266_4mb/firmware.bin`

### Phase 3: Test on Sacrificial Unit

**CRITICAL:** Test on ONE unit first!

```bash
# Connect unit via serial (GPIO0 to GND for bootloader mode)
pio run -t upload

# Monitor serial output
pio device monitor
```

Watch for:
```
Step 1: Validating flash size...
Detected flash size: 4194304 bytes (4 MB)
✓ Flash size OK

Step 2: Validating embedded binaries...
Bootloader size: XXXXX bytes
✓ Binaries look valid

Step 3: Writing IDF bootloader...
✓ Bootloader written

Step 4: Writing partition table...
✓ Partition table written

Step 5: Erasing data partitions...
✓ Data partitions erased

Step 6: Writing HomeKit app to ota_0...
✓ HomeKit app written

MIGRATION COMPLETE!
REBOOTING NOW!
```

After reboot, device should:
- Boot into IDF bootloader
- Load HomeKit app from ota_0
- Print IDF-style boot logs
- Initialize HomeKit HAP service
- Be discoverable in HomeKit app (needs re-pairing)

### Phase 4: Deploy via OTA

Once ONE unit successfully migrates:

1. **Small batch test (5-10 units):**
   ```bash
   # Upload firmware.bin to your OTA server
   # Example:
   scp .pio/build/esp8266_4mb/firmware.bin \
       user@secureapi.johnson-creative.com:/path/to/ota/
   ```

2. **Push to small batch via your existing KC OTA mechanism**
   - Units will download migrator firmware
   - Migrator runs once, rewrites flash
   - Units reboot into HomeKit firmware

3. **Verify batch:**
   - Check units come back online
   - Verify HomeKit functionality
   - Check for any failures (acceptable rate: < 5%)

4. **Full rollout:**
   - Push to all 200+ units
   - Monitor rollout
   - Track success/failure rate

## Flash Memory Map

### Before Migration (Old KC)
```
0x000000: Old bootloader (4KB)
0x001000: App slot 1 (user1)
0x101000: App slot 2 (user2/OTA)
0x3FB000: RF/System params (16KB)
```

### After Migration (New IDF)
```
0x000000: IDF Bootloader (28KB)
0x008000: Partition Table (3KB)
0x00D000: sec_cert (12KB)
0x010000: nvs (24KB)
0x016000: otadata (8KB)
0x018000: phy_init (4KB)
0x020000: ota_0 (1600KB) ← Initial HomeKit app
0x1B0000: ota_1 (1600KB) ← Empty, for future OTA
0x340000: factory_nvs (24KB)
0x346000: nvs_keys (4KB)
0x3FB000: RF/System params (16KB)
```

## Safety Features

The migrator includes multiple safety checks:

1. **Flash size validation:** Only proceeds on 4MB chips
2. **Binary validation:** Checks ESP8266 magic bytes (0xE9)
3. **Size validation:** Ensures app fits in ota_0 partition
4. **LED feedback:** Visual progress indication
5. **Serial logging:** Detailed progress output
6. **Proper sector erase:** All sectors erased before writing

## Failure Modes & Recovery

### Migrator Won't Build
- Check PlatformIO is installed: `pio --version`
- Check binaries are embedded: `ls -lh src/idf_binaries.cpp`
- Should be > 500KB if binaries are embedded

### Migration Fails (Serial Errors)
- Power loss during flash write → Unit bricked, needs serial recovery
- Invalid binary → Migrator aborts before writing (safe)
- Wrong flash size → Migrator aborts (safe)

### Unit Doesn't Boot After Migration
- Serial recovery required:
  ```bash
  # Re-flash via serial
  cd examples/keepconnect
  idf.py -p /dev/ttyUSB0 flash monitor
  ```

### Acceptable Failure Rate
- < 5% is normal for 200+ units
- Causes: power loss, bad flash, hardware faults
- Failed units: serial recovery or discard

## Post-Migration

### New OTA Process
Units now use ESP-IDF OTA (not Arduino):

```c
// In your HomeKit app (already implemented)
esp_http_client_config_t config = {
    .url = "http://secureapi.johnson-creative.com/SmartPlugs/v2firmware.bin",
};
esp_https_ota(&config);
```

Future updates:
- Build new app: `idf.py build`
- Upload app binary to OTA server
- Units download and write to ota_1
- Bootloader swaps to ota_1 on next boot
- Rinse and repeat (ota_0 ↔ ota_1)

### HomeKit Re-pairing
- Units lose old HomeKit credentials (NVS erased)
- Users must re-pair devices in HomeKit app
- Setup code from `sdkconfig.defaults` or programmed in app

## Troubleshooting

### "Bootloader doesn't start with ESP8266 magic byte"
→ You forgot to run `convert_bins.sh`. The embedded binaries are still placeholders.

### "App size too large"
→ Your HomeKit app is > 1600KB. Reduce app size:
- Disable logging: `idf.py menuconfig` → Component config → Log output
- Remove unused components
- Enable optimization: `-Os` flag

### "Failed to erase sector"
→ Bad flash hardware. Unit may be defective. Try serial flash erase:
```bash
esptool.py --port /dev/ttyUSB0 erase_flash
```

### Migration succeeds but device won't boot
→ Check serial output for IDF bootloader messages. May need to:
- Reflash via serial
- Check partition table is correct
- Verify app binary is valid

## Advanced: Espressif Official FOTA

If you want the "enterprise" approach, use Espressif's official FOTA example:
```
ESP8266_RTOS_SDK/examples/system/ota/native_ota/2+MB_flash/new_to_new_with_old
```

This uses a packed binary format where the new bootloader unpacks and writes components. More complex but potentially safer.

For your use case (200+ units, acceptable brick rate), the custom migrator is simpler and faster.

## Estimated Timeline

- Build setup: 1-2 hours
- Single unit test: 30 min
- Small batch test: 1 hour
- Full rollout: 2-4 hours (depends on your OTA infrastructure)

**Total: ~1 day** to migrate all units.

## Questions?

Review the original plan in `otaPlan.txt` for the technical deep-dive from Espressif docs.

Key insight: **4MB flash changes everything.** You have plenty of room for proper dual OTA with the partition layout in `partitions_hap.csv`.
