# Working Sector 0 Erase Configuration

## What Successfully Worked

This configuration successfully erased sector 0 and wrote 32KB of bootloader data before attempting to boot (checksum error expected since only 1/8 sectors erased).

### Output
```
========================================
KC Migration - Stage 2 Finisher
Version: Stage2-1.0.0
========================================

This stage runs from OTA_0 at 0x20000
Using PROVEN IRAM erase/write functions!

Starting in 3 seconds...
3...
2...
1...

Step 1: Installing IDF bootloader...
Using UNROLLED IRAM erase (all sectors at once)!
Source: 0x100000 -> Dest: 0x000000
Copying bootloader from 0x100000 to 0x000000...
Will copy 32768 bytes
Erasing bootloader (sector 0 only)...
OK Bootloader erased
Writing bootloader data (256-byte chunks)...
  Written 4096 / 32768 bytes
  Written 8192 / 32768 bytes
  Written 12288 / 32768 bytes
  Written 16384 / 32768 bytes
  Written 20480 / 32768 bytes
  Written 24576 / 32768 bytes
  Written 28672 / 32768 bytes
  Written 32768 / 32768 bytes
OK Successfully copied bootloader 32768 bytes

 ets Jan  8 2013,rst cause:4, boot mode:(3,7)

wdt reset
load 0x40100000, len 7544, room 16
tail 8
chksum 0x6a
load 0x23860008, len 0, room 0
tail 0
chksum 0x6a
load 0x0014006c, len 0, room 8
tail 0
chksum 0x6a
csum 0x6a
csum err
ets_main.c
```

### Code

#### IRAM Erase Function
```cpp
/**
 * IRAM function to erase sector 0
 * 1M NOPs = ~12.5ms at 80MHz - MUST wait for erase to fully complete!
 */
void IRAM_ATTR erase_sector_0_iram() {
    SPIEraseSector(0);
    // CRITICAL: Wait FULL erase time (20-100ms) before returning to flash
    for(volatile uint32_t i = 0; i < 1000000; i++) {
        asm volatile("nop");
    }
}
```

#### Wrapper Function
```cpp
/**
 * Erase bootloader - JUST SECTOR 0 (this worked before!)
 */
bool eraseBootloader() {
    Serial.println("Erasing bootloader (sector 0 only)...");
    Serial.flush();

    erase_sector_0_iram();

    Serial.println("OK Bootloader erased");
    return true;
}
```

## Key Success Factors
1. **Single sector only** - Only erase sector 0
2. **1M NOPs** - Provides ~125ms wait (covers worst-case 100ms erase time)
3. **IRAM_ATTR** - Function must be in IRAM
4. **SPIEraseSector(0)** - ROM function, not SDK wrapper
5. **Return to flash** - After erase completes, normal execution resumes
6. **No WDT manipulation** - No need to disable WDT for single sector

## Challenge
- **Sector 0 works**
- **Sectors 0+1 work**
- **Sectors 0+1+2 CRASH** (Exception 0 or WDT reset)

This suggests cumulative instability when erasing multiple sectors sequentially.

## Next Steps to Try
1. Longer delays between sectors (500ms-1s)
2. Use SDK wrapper functions that handle cache internally
3. Different approach: don't erase bootloader area at all, use different migration strategy
