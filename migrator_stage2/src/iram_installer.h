/*
 * IRAM-ONLY bootloader + partition installer
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
    // Bootloader
    const uint8_t *bootloader_data;   // Pointer to bootloader in RAM
    uint32_t       bootloader_size;   // Size in bytes
    uint32_t       bootloader_target; // Where to write (should be 0)

    // Partition table
    const uint8_t *partition_data;    // Pointer to partition table in RAM
    uint32_t       partition_size;    // Size in bytes (e.g. 4096)
    uint32_t       partition_target;  // Where to write (0x00008000)
};

/**
 * IRAM-ONLY installer function
 * Must be copied to IRAM and called with interrupts disabled
 * NO FLASH ACCESS ALLOWED
 *
 * This will:
 *  1. Erase sector for partition table and write it
 *  2. Erase sectors 0-7 and write bootloader
 *  3. Reboot
 */
void IRAM_ATTR install_bootloader_iram(const installer_params *params) {
    // Disable cache completely - no flash reads during this function
    Cache_Read_Disable();

    // Disable interrupts
    asm volatile("rsil a2, 15" : : : "a2");

    // ---------- PARTITION TABLE: erase & write FIRST ----------
    // Compute partition sector index: partition_target / 4096
    uint32_t part_sector = params->partition_target / 4096U;  // 0x8000/4096 = 8

    SPIEraseSector(part_sector);
    ets_delay_us(5000);  // wait a bit for erase to complete

    uint8_t *src       = (uint8_t*)params->partition_data;
    uint32_t addr      = params->partition_target;
    uint32_t remaining = params->partition_size;

    while (remaining > 0) {
        uint32_t chunk         = (remaining > 256) ? 256 : remaining;
        uint32_t aligned_chunk = (chunk + 3U) & ~3U;  // align to 4 bytes

        SPIWrite(addr, (void*)src, aligned_chunk);
        ets_delay_us(5000);  // 5ms per chunk

        src       += chunk;
        addr      += chunk;
        remaining -= chunk;
    }

    // ---------- BOOTLOADER: erase all 8 bootloader sectors (0-7) ----------
    for (uint32_t sector = 0; sector < 8; sector++) {
        SPIEraseSector(sector);
        ets_delay_us(5000);  // wait a bit for each erase
    }

    // Write bootloader in 256-byte chunks
    src       = (uint8_t*)params->bootloader_data;  // Cast away const for SPIWrite
    addr      = params->bootloader_target;
    remaining = params->bootloader_size;

    while (remaining > 0) {
        uint32_t chunk         = (remaining > 256) ? 256 : remaining;
        uint32_t aligned_chunk = (chunk + 3U) & ~3U;

        SPIWrite(addr, (void*)src, aligned_chunk);
        ets_delay_us(5000);  // 5ms per chunk

        src       += chunk;
        addr      += chunk;
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