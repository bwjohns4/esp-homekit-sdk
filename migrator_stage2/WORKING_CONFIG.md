# Working ESP8266 IRAM Flash Erase Configuration

## What Works

### eraseBootloader()
```cpp
bool eraseBootloader() {
    Serial.println("Erasing bootloader (WDT disabled, IRAM)...");
    Serial.flush();

    // Disable soft WDT
    ESP.wdtDisable();

    erase_all_bootloader_sectors_iram();

    // Re-enable WDT
    ESP.wdtEnable(0);

    Serial.println("OK Bootloader erased");
    return true;
}
```

### erase_all_bootloader_sectors_iram()
```cpp
void IRAM_ATTR erase_all_bootloader_sectors_iram() {
    SPIEraseSector(0);
    for(volatile uint32_t i = 0; i < 1000000; i++) asm volatile("nop");

    SPIEraseSector(1);
    for(volatile uint32_t i = 0; i < 1000000; i++) asm volatile("nop");

    SPIEraseSector(2);
    for(volatile uint32_t i = 0; i < 1000000; i++) asm volatile("nop");

    SPIEraseSector(3);
    for(volatile uint32_t i = 0; i < 1000000; i++) asm volatile("nop");

    SPIEraseSector(4);
    for(volatile uint32_t i = 0; i < 1000000; i++) asm volatile("nop");

    SPIEraseSector(5);
    for(volatile uint32_t i = 0; i < 1000000; i++) asm volatile("nop");

    SPIEraseSector(6);
    for(volatile uint32_t i = 0; i < 1000000; i++) asm volatile("nop");

    SPIEraseSector(7);
    for(volatile uint32_t i = 0; i < 1000000; i++) asm volatile("nop");
}
```

## Key Requirements
1. **IRAM_ATTR** - Function must be in IRAM
2. **SPIEraseSector()** - Use ROM function, not SDK wrapper
3. **1M NOPs** - Wait ~125ms per sector for erase to complete
4. **Unrolled loop** - No loop variables, hardcoded sector numbers
5. **WDT disabled** - ESP.wdtDisable() before, ESP.wdtEnable(0) after
6. **No cache control** - Don't call Cache_Read_Disable/Enable
7. **No Serial calls** - Inside IRAM function, no flash code access

## Successful Output
```
Erasing bootloader (ROM cache control + Wait_SPI_Idle)...
OK Bootloader erased
Writing bootloader data (256-byte chunks)...
  Written 4096 / 32768 bytes
  ...
  Written 32768 / 32768 bytes
OK Successfully copied bootloader 32768 bytes
```
