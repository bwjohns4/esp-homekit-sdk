# KC to HomeKit Migrator Firmware

This is a one-time migration firmware that converts Keep Connect (KC) ESP8266 units from Arduino/Non-OS SDK to ESP-IDF HomeKit firmware.

## What it does

1. Validates the ESP8266 has 4MB flash
2. Erases and writes the new IDF bootloader at 0x0000
3. Writes the new partition table at 0x8000
4. Writes the HomeKit app to ota_0 at 0x20000
5. Reboots into the new IDF-based HomeKit firmware

## How to use

1. Build the HomeKit firmware in `examples/keepconnect/` and extract binaries:
   - `build/bootloader/bootloader.bin`
   - `build/partition_table/partition-table.bin`
   - `build/smart_outlet.bin` (or your app name)

2. Convert binaries to C arrays:
   ```bash
   ./convert_bins.sh
   ```

3. Build the migrator:
   ```bash
   pio run
   ```

4. Deploy via your existing KC OTA mechanism to your units

## Safety

- Validates 4MB flash before proceeding
- Uses proper sector erase before writing
- Writes are aligned to 4-byte boundaries
- Power loss during migration = brick (acceptable for this one-time conversion)

## Post-migration

After successful migration, units will boot into IDF HomeKit firmware with:
- Dual OTA slots (ota_0 / ota_1)
- Proper partition table
- NVS storage for HomeKit credentials
- Standard esp_https_ota for future updates
