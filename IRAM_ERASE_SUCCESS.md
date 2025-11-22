# ESP8266 IRAM Flash Erase - What ACTUALLY Works

## The Problem
ESP8266 cannot erase flash while executing code from flash. When ANY sector erases, the entire flash chip becomes busy and the CPU crashes trying to fetch the next instruction.

## The Solution That WORKS

### Working Code (Tested and Confirmed)
```cpp
void IRAM_ATTR test_erase_from_ram() {
    // Erase sector 2 using bootloader ROM function
    SPIEraseSector(2);

    // CRITICAL: Wait in RAM before returning to flash code
    for(volatile uint32_t i = 0; i < 1000000; i++) {
        asm volatile("nop");
    }
}
```

### Test Output (SUCCESS!)
```
About to test IRAM erase of sector 2 (0x8000)...
Calling test_erase_from_ram()...
SUCCESS! Sector 2 erased from IRAM!
If you see this, IRAM erase works!
```

## Key Requirements

1. **Use `SPIEraseSector()` not `spi_flash_erase_sector()`**
   - `SPIEraseSector()` is the bootloader ROM function
   - `spi_flash_erase_sector()` causes Illegal Instruction exception

2. **Function MUST have `IRAM_ATTR`**
   - Places function in IRAM (RAM, not flash)
   - Function code must be small enough to fit in 32KB IRAM

3. **Wait INSIDE the IRAM function before returning**
   - Flash is busy after erase
   - Must wait in RAM (NOP loop) before returning to flash code
   - Returning too early = crash when fetching next instruction from flash

4. **Hardcoded sector number works**
   - Parameters and loops may cause issues
   - Keep it ULTRA SIMPLE

## What Doesn't Work

❌ `spi_flash_erase_sector()` - Wrong ROM function
❌ Returning immediately - Flash still busy
❌ Serial.printf() calls inside erase - Accesses flash code
❌ Complex loops with parameters - May not stay in IRAM

## The Mystery

Why does erasing ONE sector work, but erasing MULTIPLE sectors in a loop fail?

Possible reasons:
1. Loop code not fully in IRAM (even with IRAM_ATTR)
2. Function parameters stored in flash stack
3. Need MUCH longer delays between erases
4. First erase succeeds, second iteration crashes trying to loop

## Next Steps

Try unrolling the loop - call `SPIEraseSector()` 8 times with hardcoded sector numbers instead of a for loop.
