#include <Arduino.h>
extern "C" {
    #include "user_interface.h"
    #include "spi_flash.h"

    // ROM functions (always in ROM, never flash)
    int ets_printf(const char *fmt, ...);
}

// IRAM-only installer for bootloader (NO flash access during install)
#include "iram_installer.h"

// SDK functions are already declared in flash.h (included via Arduino.h)
// int SPIEraseAreaEx(const uint32_t start, const uint32_t size);
// int SPIWrite(uint32_t addr, void *src, size_t size);

#define LED_PIN 2

// Source addresses (temp storage from Stage 1)
#define SRC_BOOTLOADER   0x100000  // IDF bootloader stays here, stub loads it
#define SRC_PARTITION    0x110000
#define SRC_APP          0x120000

// Destination addresses (IDF layout)
#define DST_PARTITION    0x8000
#define DST_APP          0x1B0000  // OTA_1 (we're running from OTA_0)

// Sizes
#define MAX_PARTITION_SIZE   (4 * 1024)    // 4KB
#define MAX_APP_SIZE         (1600 * 1024) // 1600KB

#define SECTOR_SIZE 4096

void blinkLED(int times, int delayMs = 200) {
    for (int i = 0; i < times; i++) {
        digitalWrite(LED_PIN, LOW);
        delay(delayMs);
        digitalWrite(LED_PIN, HIGH);
        delay(delayMs);
    }
}

void blinkError() {
    while(1) {
        blinkLED(10, 100);
        delay(2000);
    }
}

/**
 * IRAM function to erase ONE sector using bootloader ROM function
 * PROVEN TO WORK! Waits before returning to flash code
 */
void IRAM_ATTR erase_sector_iram(uint32_t sector) {
    // Use bootloader ROM function - PROVEN to work!
    SPIEraseSector(sector);

    // Wait for flash to be ready before returning
    for(volatile uint32_t i = 0; i < 1000000; i++) {
        asm volatile("nop");
    }
}

/**
 * IRAM function to write flash using bootloader ROM function
 * 100k NOPs = sufficient for 256-byte write (~3ms worst case)
 */
void IRAM_ATTR write_flash_iram(uint32_t addr, void* data, size_t size) {
    SPIWrite(addr, data, size);

    // Wait for write to complete
    for(volatile uint32_t i = 0; i < 100000; i++) {
        asm volatile("nop");
    }
}

/**
 * IRAM function to erase sector 0
 * 500k NOPs = ~62ms at 80MHz - covers worst-case erase time
 */
void IRAM_ATTR erase_sector_0_iram() {
    SPIEraseSector(0);
    for(volatile uint32_t i = 0; i < 500000; i++) {
        asm volatile("nop");
    }
}

/**
 * IRAM function to erase sector 1
 */
void IRAM_ATTR erase_sector_1_iram() {
    SPIEraseSector(1);
    for(volatile uint32_t i = 0; i < 500000; i++) {
        asm volatile("nop");
    }
}

/**
 * IRAM function to erase sector 2
 */
void IRAM_ATTR erase_sector_2_iram() {
    SPIEraseSector(2);
    for(volatile uint32_t i = 0; i < 500000; i++) {
        asm volatile("nop");
    }
}

/**
 * IRAM function to erase sector 3
 */
void IRAM_ATTR erase_sector_3_iram() {
    SPIEraseSector(3);
    for(volatile uint32_t i = 0; i < 500000; i++) {
        asm volatile("nop");
    }
}

/**
 * IRAM function to erase sector 4
 */
void IRAM_ATTR erase_sector_4_iram() {
    SPIEraseSector(4);
    for(volatile uint32_t i = 0; i < 500000; i++) {
        asm volatile("nop");
    }
}

/**
 * IRAM function to erase sector 5
 */
void IRAM_ATTR erase_sector_5_iram() {
    SPIEraseSector(5);
    for(volatile uint32_t i = 0; i < 500000; i++) {
        asm volatile("nop");
    }
}

/**
 * IRAM function to erase sector 6
 */
void IRAM_ATTR erase_sector_6_iram() {
    SPIEraseSector(6);
    for(volatile uint32_t i = 0; i < 500000; i++) {
        asm volatile("nop");
    }
}

/**
 * IRAM function to erase sector 7
 */
void IRAM_ATTR erase_sector_7_iram() {
    SPIEraseSector(7);
    for(volatile uint32_t i = 0; i < 500000; i++) {
        asm volatile("nop");
    }
}

/**
 * IRAM function to erase partition table sectors (at 0x8000)
 * Must be called from flash code BEFORE we touch the table,
 * but actual erase happens entirely in IRAM.
 */
void IRAM_ATTR erase_partition_sectors_iram() {
    // 0x8000 / 4096 = sector 8
    SPIEraseSector(8);

    // Wait for erase (1M NOPs like working single-sector test)
    for(volatile uint32_t i = 0; i < 1000000; i++) {
        asm volatile("nop");
    }
}

/**
 * IRAM function to erase ALL 8 sectors - MINIMAL NOPs
 * 50k NOPs per sector = just enough, total ~400k = ~0.5s in IRAM
 */
void IRAM_ATTR erase_all_bootloader_sectors_iram() {
    SPIEraseSector(0);
    for(volatile uint32_t i = 0; i < 50000; i++) asm volatile("nop");

    SPIEraseSector(1);
    for(volatile uint32_t i = 0; i < 50000; i++) asm volatile("nop");

    SPIEraseSector(2);
    for(volatile uint32_t i = 0; i < 50000; i++) asm volatile("nop");

    SPIEraseSector(3);
    for(volatile uint32_t i = 0; i < 50000; i++) asm volatile("nop");

    SPIEraseSector(4);
    for(volatile uint32_t i = 0; i < 50000; i++) asm volatile("nop");

    SPIEraseSector(5);
    for(volatile uint32_t i = 0; i < 50000; i++) asm volatile("nop");

    SPIEraseSector(6);
    for(volatile uint32_t i = 0; i < 50000; i++) asm volatile("nop");

    SPIEraseSector(7);
    for(volatile uint32_t i = 0; i < 50000; i++) asm volatile("nop");
}

// No longer needed - installer does everything from IRAM

/**
 * Erase partition table using official spi_flash API (NO IRAM NEEDED)
 */
bool erasePartitionTable() {
    Serial.println("Erasing partition table (sector 8 via IRAM)...");
    Serial.flush();

    erase_partition_sectors_iram();  // IRAM function does SPIEraseSector(8)
    delay(100);

    Serial.println("OK Partition table erased");
    return true;
}

/**
 * Write to flash using spi_flash_write (NO IRAM NEEDED for partition/app)
 */
bool writeFlashLowAddr(uint32_t addr, void* data, size_t size) {
    // size must be multiple of 4; caller already pads for that
    if (spi_flash_write(addr, (uint32_t*)data, size) != SPI_FLASH_RESULT_OK) {
        Serial.printf("ERROR: spi_flash_write failed at 0x%06X\n", addr);
        return false;
    }
    return true;
}

/**
 * Install IDF bootloader using IRAM-only installer
 * This reads bootloader into RAM, then jumps to IRAM code
 */
bool writeBootloaderStub() {
    const uint32_t srcAddr = SRC_BOOTLOADER;  // 0x100000
    const size_t maxSize = 32 * 1024;

    Serial.println("Installing IDF bootloader using IRAM-only installer...");
    Serial.printf("Reading bootloader from 0x%06X (%u bytes)...\n", srcAddr, maxSize);

    // Allocate RAM buffer for bootloader
    static uint8_t bootloader_ram[32768];

    // Read bootloader from flash into RAM
    Serial.println("Loading bootloader into RAM...");
    if (!ESP.flashRead(srcAddr, reinterpret_cast<uint32_t*>(bootloader_ram), maxSize)) {
        Serial.println("ERROR: Failed to read bootloader!");
        return false;
    }

    Serial.println("Bootloader loaded into RAM");
    Serial.println("");
    Serial.println("========================================");
    Serial.println("ENTERING IRAM-ONLY INSTALLER");
    Serial.println("========================================");
    Serial.println("The device will:");
    Serial.println("1. Disable interrupts and cache");
    Serial.println("2. Erase sectors 0-7");
    Serial.println("3. Write bootloader");
    Serial.println("4. Reboot automatically");
    Serial.println("");
    Serial.println("NO SERIAL OUTPUT during install!");
    Serial.println("Device will reboot in 3 seconds...");
    Serial.flush();

    delay(1000);
    Serial.println("3...");
    Serial.flush();
    delay(1000);
    Serial.println("2...");
    Serial.flush();
    delay(1000);
    Serial.println("1...");
    Serial.flush();
    delay(1000);

    // Set up installer parameters
    installer_params params;
    params.bootloader_data = bootloader_ram;
    params.bootloader_size = maxSize;
    params.target_addr = 0;

    // Call IRAM installer - THIS WILL NOT RETURN
    // It will erase/write/reboot
    install_bootloader_iram(&params);

    // Never reached
    while(1);
    return true;
}

/**
 * Copy partition table with IRAM erase
 */
bool copyPartitionTable(uint32_t srcAddr, uint32_t dstAddr, size_t maxSize) {
    Serial.printf("Copying partition table from 0x%06X to 0x%06X...\n", srcAddr, dstAddr);
    Serial.printf("Will copy %u bytes\n", maxSize);

    // Dump source bytes
    Serial.println("Dumping first 32 bytes of partition table SOURCE:");
    uint8_t src_dump[32] = {0};
    if (spi_flash_read(srcAddr, (uint32_t*)src_dump, sizeof(src_dump)) != SPI_FLASH_RESULT_OK) {
        Serial.println("ERROR: Failed to read src partition table for dump");
    } else {
        for (int i = 0; i < 32; ++i) {
            if (i % 16 == 0) Serial.println();
            Serial.printf("%02X ", src_dump[i]);
        }
        Serial.println("\n");
    }

    // Erase destination using IRAM function
    if (!erasePartitionTable()) {
        return false;
    }

    digitalWrite(LED_PIN, !digitalRead(LED_PIN));

    // Copy data in small chunks
    Serial.println("Writing partition table data (256-byte chunks)...");
    uint8_t buffer[256];
    size_t copied = 0;

    while (copied < maxSize) {
        delay(1);

        size_t chunk = min(sizeof(buffer), maxSize - copied);

        if (!ESP.flashRead(srcAddr + copied, reinterpret_cast<uint32_t*>(buffer), chunk)) {
            Serial.printf("ERROR: Failed to read at 0x%06X\n", srcAddr + copied);
            return false;
        }

        size_t alignedChunk = (chunk + 3) & ~3;
        if (alignedChunk > chunk) {
            memset(buffer + chunk, 0xFF, alignedChunk - chunk);
        }

        if (!writeFlashLowAddr(dstAddr + copied, buffer, alignedChunk)) {
            Serial.printf("ERROR: Failed to write at 0x%06X\n", dstAddr + copied);
            return false;
        }

        copied += chunk;
    }

    Serial.printf("OK Successfully copied partition table %u bytes\n", maxSize);

    // ---- HEX DUMP OF PARTITION TABLE DESTINATION ----
    Serial.println("Dumping first 32 bytes of partition table at destination:");
    uint8_t pt_dump[32] = {0};

    // Use ROM flash read directly to match what the IDF bootloader will see
    if (spi_flash_read(dstAddr, (uint32_t*)pt_dump, sizeof(pt_dump)) != SPI_FLASH_RESULT_OK) {
        Serial.println("ERROR: Failed to read back partition table for dump");
    } else {
        for (int i = 0; i < 32; ++i) {
            if (i % 16 == 0) Serial.println();
            Serial.printf("%02X ", pt_dump[i]);
        }
        Serial.println("\n");
    }
    // -----------------------------------------------

    return true;
}

/**
 * Copy app (no erase needed - will be overwriting itself)
 */
bool copyApp(uint32_t srcAddr, uint32_t dstAddr, size_t maxSize) {
    Serial.printf("Copying app from 0x%06X to 0x%06X...\n", srcAddr, dstAddr);
    Serial.printf("Will copy %u bytes (NO ERASE - overwriting self)\n", maxSize);

    digitalWrite(LED_PIN, !digitalRead(LED_PIN));

    // Copy in 256-byte chunks with frequent delay() calls
    Serial.println("Writing app data (256-byte chunks)...");
    uint8_t buffer[256];
    size_t copied = 0;

    while (copied < maxSize) {
        delay(1);

        size_t chunk = min(sizeof(buffer), maxSize - copied);

        if (!ESP.flashRead(srcAddr + copied, reinterpret_cast<uint32_t*>(buffer), chunk)) {
            Serial.printf("ERROR: Failed to read at 0x%06X\n", srcAddr + copied);
            return false;
        }

        size_t alignedChunk = (chunk + 3) & ~3;
        if (alignedChunk > chunk) {
            memset(buffer + chunk, 0xFF, alignedChunk - chunk);
        }

        if (!writeFlashLowAddr(dstAddr + copied, buffer, alignedChunk)) {
            Serial.printf("ERROR: Failed to write at 0x%06X\n", dstAddr + copied);
            return false;
        }

        copied += chunk;

        if (copied % (256 * 1024) == 0) {
            Serial.printf("  Written %u / %u KB\n", copied / 1024, maxSize / 1024);
            digitalWrite(LED_PIN, !digitalRead(LED_PIN));
        }
    }

    Serial.printf("OK Successfully copied app %u bytes\n", maxSize);
    return true;
}

void setup() {
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);

    Serial.begin(115200);
    delay(500);

    Serial.println("\n\n\n");
    Serial.println("========================================");
    Serial.println("KC Migration - Stage 2 Finisher");
    Serial.println("Version: " MIGRATOR_VERSION);
    Serial.println("========================================\n");

    Serial.println("This stage runs from OTA_0 at 0x20000");
    Serial.println("Using PROVEN IRAM erase/write functions!\n");

    Serial.println("Starting in 3 seconds...");
    for (int i = 3; i > 0; i--) {
        Serial.printf("%d...\n", i);
        blinkLED(1);
        delay(1000);
    }

    // Step 1: Copy partition table FIRST (fast, safe)
    Serial.println("\nStep 1: Installing partition table...");
    Serial.printf("Source: 0x%06X -> Dest: 0x%06X\n", SRC_PARTITION, DST_PARTITION);
    if (!copyPartitionTable(SRC_PARTITION, DST_PARTITION, MAX_PARTITION_SIZE)) {
        Serial.println("ERROR: Failed to install partition table!");
        blinkError();
        return;
    }
    Serial.println("OK Partition table installed\n");
    blinkLED(1);
    yield();

    // Step 2: Copy app to OTA_1 (slow but non-critical if it fails)
    Serial.println("Step 2: Installing HomeKit app to OTA_1...");
    Serial.println("NOTE: Writing to OTA_1 (0x1B0000) since we're running from OTA_0");
    Serial.printf("Source: 0x%06X -> Dest: 0x%06X\n", SRC_APP, DST_APP);
    if (!copyApp(SRC_APP, DST_APP, MAX_APP_SIZE)) {
        Serial.println("ERROR: Failed to install app!");
        blinkError();
        return;
    }
    Serial.println("OK HomeKit app installed to OTA_1\n");
    blinkLED(1);
    yield();

    // Step 3: Install IDF bootloader LAST (critical - do this last!)
    Serial.println("Step 3: Installing IDF bootloader...");
    Serial.println("IMPORTANT: Using IRAM-only installer (NO flash access)");
    Serial.println("This will erase sectors 0-7 and write bootloader");

    // This function will NOT return - it reboots the device
    writeBootloaderStub();

    // Never reached
    while(1);

    // Migration complete!
    Serial.println("\n========================================");
    Serial.println("MIGRATION COMPLETE!");
    Serial.println("========================================");
    Serial.println("OK IDF bootloader at 0x0 (all 8 sectors erased & written)");
    Serial.println("OK IDF partition table at 0x8000");
    Serial.println("OK IDF HomeKit app at 0x1B0000 (OTA_1)");
    Serial.println("\nNext boot:");
    Serial.println("1. Boot ROM loads IDF bootloader from 0x0");
    Serial.println("2. IDF bootloader reads partition table at 0x8000");
    Serial.println("3. IDF bootloader boots app from OTA_1");
    Serial.println("\nRebooting in 5 seconds...\n");

    for (int i = 0; i < 5; i++) {
        blinkLED(1, 100);
        delay(800);
    }

    Serial.println("REBOOTING NOW!");
    Serial.flush();
    ESP.restart();
}

void loop() {
    // Should never reach here
    delay(1000);
}
