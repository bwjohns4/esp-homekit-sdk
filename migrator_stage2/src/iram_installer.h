/*
 * IRAM-ONLY bootloader installer
 * This ENTIRE module runs from IRAM with NO flash access
 */

#ifndef IRAM_INSTALLER_H
#define IRAM_INSTALLER_H

#include <stdint.h>

// ROM functions (already declared by Arduino, just reference them)
extern "C" {
    // These are already declared in flash.h:
    // int SPIEraseSector(uint32_t sector);
    // int SPIWrite(uint32_t addr, void *src, size_t size);

    // Additional ROM functions we need:
    extern void ets_delay_us(uint32_t us);
    extern void system_restart(void);
    extern void Cache_Read_Disable(void);
    extern void Cache_Read_Enable(uint32_t, uint32_t, uint32_t);
}

// Installer parameters (passed from main code)
struct installer_params {
    const uint8_t *bootloader_data;  // Pointer to bootloader in RAM
    uint32_t bootloader_size;        // Size in bytes
    uint32_t target_addr;            // Where to write (should be 0)
};

/**
 * IRAM-ONLY installer function
 * Must be copied to IRAM and called with interrupts disabled
 * NO FLASH ACCESS ALLOWED
 */
void IRAM_ATTR install_bootloader_iram(const installer_params *params) {
    // Disable cache completely - no flash reads during this function
    Cache_Read_Disable();

    // Disable interrupts
    asm volatile("rsil a2, 15" : : : "a2");

    // Erase all 8 bootloader sectors (0-7)
    for (uint32_t sector = 0; sector < 8; sector++) {
        SPIEraseSector(sector);
        // Wait for erase to complete (~20-100ms worst case)
        ets_delay_us(150000);  // 150ms per sector to be safe
    }

    // Write bootloader in 256-byte chunks
    uint8_t *src = (uint8_t*)params->bootloader_data;  // Cast away const for SPIWrite
    uint32_t addr = params->target_addr;
    uint32_t remaining = params->bootloader_size;

    while (remaining > 0) {
        uint32_t chunk = (remaining > 256) ? 256 : remaining;

        // Align to 4 bytes
        uint32_t aligned_chunk = (chunk + 3) & ~3;

        SPIWrite(addr, (void*)src, aligned_chunk);
        ets_delay_us(5000);  // 5ms per chunk

        src += chunk;
        addr += chunk;
        remaining -= chunk;
    }

    // Small delay before reboot
    ets_delay_us(100000);

    // Reboot
    system_restart();

    // Never reached
    while(1);
}

#endif // IRAM_INSTALLER_H
