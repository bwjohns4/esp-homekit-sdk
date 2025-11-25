#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <Updater.h>
#include "RawFlashWriter.h"

#define LED_PIN 2  // Onboard LED for status indication
#define ADDR_APP_STAGE   0x120000

// WiFi credentials
const char* WIFI_SSID = "KCLabRouter";
const char* WIFI_PASSWORD = "Ehouse2613";

// Binary download URLs
#ifndef BINARY_SERVER_URL
#define BINARY_SERVER_URL "http://api.johnson-creative.com/SmartPlugs/Migration8266"
#endif

const char* BOOTLOADER_URL = BINARY_SERVER_URL "/bootloader.bin";
const char* PARTITION_URL = BINARY_SERVER_URL "/partitions_hap.bin";
const char* APP_URL = BINARY_SERVER_URL "/smart_outlet.bin";

// Flash addresses for IDF layout (matching partitions_hap.csv)
#define ADDR_BOOTLOADER    0x0000
#define ADDR_PARTITION_TBL 0x8000
#define ADDR_SEC_CERT      0xD000
#define ADDR_NVS           0x10000
#define ADDR_OTA_DATA      0x16000
#define ADDR_PHY_INIT      0x18000
#define ADDR_OTA_0         0x20000
#define ADDR_OTA_1         0x1B0000
#define ADDR_FACTORY_NVS   0x340000
#define ADDR_NVS_KEYS      0x346000

// Partition sizes
#define SIZE_SEC_CERT      0x3000   // 12KB
#define SIZE_NVS           0x6000   // 24KB
#define SIZE_OTA_DATA      0x2000   // 8KB
#define SIZE_PHY_INIT      0x1000   // 4KB
#define SIZE_OTA_0         0x190000 // 1600KB
#define SIZE_OTA_1         0x190000 // 1600KB
#define SIZE_FACTORY_NVS   0x6000   // 24KB
#define SIZE_NVS_KEYS      0x1000   // 4KB

#define SECTOR_SIZE        4096
#define EXPECTED_FLASH_SIZE (4 * 1024 * 1024)  // 4MB

// LED blink patterns
void blinkLED(int times, int delayMs = 200) {
    for (int i = 0; i < times; i++) {
        digitalWrite(LED_PIN, LOW);  // LED on (active low)
        delay(delayMs);
        digitalWrite(LED_PIN, HIGH); // LED off
        delay(delayMs);
    }
}

void blinkError() {
    while(1) {
        blinkLED(10, 100);  // Rapid blinking = error
        delay(2000);
    }
}

/**
 * Download binary from HTTP and write using Update class (auto-erases)
 * @param url URL to download from
 * @param addr Flash address to write to
 * @param expectedSize Expected size (0 = don't check)
 * @return true on success
 */
bool downloadAndFlash(const char* url, uint32_t addr, size_t expectedSize = 0) {
    WiFiClient client;
    HTTPClient http;

    Serial.printf("Downloading from %s...\n", url);

    if (!http.begin(client, url)) {
        Serial.println("ERROR: Failed to begin HTTP request");
        return false;
    }

    int httpCode = http.GET();
    if (httpCode != HTTP_CODE_OK) {
        Serial.printf("ERROR: HTTP GET failed, code: %d\n", httpCode);
        http.end();
        return false;
    }

    size_t totalSize = http.getSize();
    if (totalSize <= 0) {
        Serial.println("ERROR: Invalid content length");
        http.end();
        return false;
    }

    if (expectedSize > 0 && totalSize != expectedSize) {
        Serial.printf("WARNING: Size mismatch - expected %u, got %u\n", expectedSize, totalSize);
    }

    Serial.printf("File size: %u bytes\n", totalSize);
    Serial.printf("Writing to flash address: 0x%06X\n", addr);

    // Initialize Update class (will auto-erase sectors as it writes)
    if (!Update.begin(totalSize, U_FLASH, -1, addr)) {
        Serial.printf("ERROR: Update.begin failed: %s\n", Update.getErrorString().c_str());
        http.end();
        return false;
    }

    // Stream download and write
    Serial.println("Downloading and writing (Update class auto-erases)...");
    WiFiClient* stream = http.getStreamPtr();
    uint8_t buf[512];
    size_t written = 0;
    bool firstChunkLogged = false;

    while (http.connected() && written < totalSize) {
        size_t available = stream->available();
        if (available) {
            size_t toRead = min(available, sizeof(buf));
            toRead = min(toRead, totalSize - written);

            size_t bytesRead = stream->readBytes(buf, toRead);
            if (bytesRead == 0) break;


        if (!firstChunkLogged) {
            Serial.println("First 16 bytes from HTTP:");
            for (int i = 0; i < 16 && i < (int)bytesRead; i++) {
                Serial.printf("%02X ", buf[i]);
            }
            Serial.println();
            firstChunkLogged = true;
        }
            // Update.write() handles erase and watchdog automatically
            size_t bytesWritten = Update.write(buf, bytesRead);
            if (bytesWritten != bytesRead) {
                Serial.printf("ERROR: Update.write failed at offset %u\n", written);
                Serial.printf("Update error: %s\n", Update.getErrorString().c_str());
                http.end();
                return false;
            }

            written += bytesWritten;

            if (written % 8192 == 0 || written >= totalSize) {
                digitalWrite(LED_PIN, !digitalRead(LED_PIN));
                Serial.printf("  Progress: %u / %u bytes (%.1f%%)\n",
                             written, totalSize, (written * 100.0) / totalSize);
            }
        }
        yield();
    }

    http.end();

    if (written != totalSize) {
        Serial.printf("ERROR: Incomplete download - got %u of %u bytes\n", written, totalSize);
        return false;
    }

    // Finalize update (don't reboot)
    if (!Update.end(false)) {
        Serial.printf("ERROR: Update.end failed: %s\n", Update.getErrorString().c_str());
        return false;
    }

    Serial.printf("✓ Successfully wrote %u bytes to 0x%06X\n", written, addr);
    return true;
}

/**
 * Download and write to flash using RawFlashWriter (for non-app binaries)
 * Uses battle-tested Updater logic without image validation
 */
bool downloadAndFlashManual(const char* url, uint32_t addr) {
    WiFiClient client;
    HTTPClient http;

    Serial.printf("Downloading from %s...\n", url);

    if (!http.begin(client, url)) {
        Serial.println("ERROR: Failed to begin HTTP request");
        return false;
    }

    int httpCode = http.GET();
    if (httpCode != HTTP_CODE_OK) {
        Serial.printf("ERROR: HTTP GET failed, code: %d\n", httpCode);
        http.end();
        return false;
    }

    size_t totalSize = http.getSize();
    if (totalSize <= 0) {
        Serial.println("ERROR: Invalid content length");
        http.end();
        return false;
    }

    Serial.printf("File size: %u bytes\n", totalSize);
    Serial.printf("Writing to flash address: 0x%06X\n", addr);

    // Initialize RawFlashWriter
    RawFlashWriter writer;
    if (!writer.begin(totalSize, addr)) {
        Serial.println("ERROR: Failed to initialize RawFlashWriter");
        http.end();
        return false;
    }

    // Stream download and write (RawFlashWriter handles erase automatically)
    Serial.println("Downloading and writing (auto-erasing at sector boundaries)...");
    WiFiClient* stream = http.getStreamPtr();
    uint8_t buf[512];
    size_t written = 0;
    bool firstChunkLogged = false;

    while (http.connected() && written < totalSize) {
        size_t available = stream->available();
        if (available) {
            size_t toRead = min(available, sizeof(buf));
            toRead = min(toRead, totalSize - written);

            size_t bytesRead = stream->readBytes(buf, toRead);
            if (bytesRead == 0) break;

        if (!firstChunkLogged) {
            Serial.println("First 16 bytes from HTTP:");
            for (int i = 0; i < 16 && i < (int)bytesRead; i++) {
                Serial.printf("%02X ", buf[i]);
            }
            Serial.println();
            firstChunkLogged = true;
        }

            // RawFlashWriter handles erase and watchdog automatically
            size_t bytesWritten = writer.write(buf, bytesRead);
            if (bytesWritten != bytesRead) {
                Serial.printf("ERROR: Write failed at offset %u (error: %d)\n", written, writer.getError());
                http.end();
                return false;
            }

            written += bytesWritten;

            if (written % 4096 == 0 || written >= totalSize) {
                digitalWrite(LED_PIN, !digitalRead(LED_PIN));
                Serial.printf("  Progress: %u / %u bytes (%.1f%%)\n",
                             written, totalSize, (written * 100.0) / totalSize);
            }
        }
        yield();
    }

    http.end();

    if (written != totalSize) {
        Serial.printf("ERROR: Incomplete download - got %u of %u bytes\n", written, totalSize);
        return false;
    }

    // Finalize write
    if (!writer.end()) {
        Serial.printf("ERROR: Failed to finalize write (error: %d)\n", writer.getError());
        return false;
    }

    Serial.printf("✓ Successfully wrote %u bytes to 0x%06X\n", written, addr);
    return true;
}

/**
 * Erase a partition using RawFlashWriter (based on Updater logic)
 * Writes 0xFF bytes which triggers erase at sector boundaries
 */
bool flashEraseRegion(uint32_t addr, size_t len) {
    Serial.printf("Erasing region 0x%06X (%u bytes)...\n", addr, len);

    RawFlashWriter writer;
    if (!writer.begin(len, addr)) {
        Serial.println("ERROR: Failed to initialize RawFlashWriter");
        return false;
    }

    // Write 0xFF bytes (triggers erase at each sector boundary)
    uint8_t buf[512];
    memset(buf, 0xFF, sizeof(buf));

    size_t written = 0;
    while (written < len) {
        size_t toWrite = min(sizeof(buf), len - written);

        size_t bytesWritten = writer.write(buf, toWrite);
        if (bytesWritten != toWrite) {
            Serial.printf("ERROR: Write failed at offset %u (error: %d)\n", written, writer.getError());
            return false;
        }

        written += bytesWritten;

        if (written % 4096 == 0) {
            Serial.printf("  Erased %u / %u bytes\n", written, len);
            digitalWrite(LED_PIN, !digitalRead(LED_PIN));
        }
    }

    if (!writer.end()) {
        Serial.printf("ERROR: Failed to finalize erase (error: %d)\n", writer.getError());
        return false;
    }

    Serial.println("✓ Erase complete");
    return true;
}

void performMigration() {
    Serial.println("\n\n========================================");
    Serial.println("KC to HomeKit IDF Migration Tool");
    Serial.println("Version: " MIGRATOR_VERSION);
    Serial.println("========================================\n");

    // Step 1: Validate flash size
    Serial.println("Step 1: Validating flash size...");
    uint32_t flashSize = ESP.getFlashChipRealSize();
    Serial.printf("Detected flash size: %u bytes (%u MB)\n", flashSize, flashSize / (1024 * 1024));

    if (flashSize != EXPECTED_FLASH_SIZE) {
        Serial.printf("ERROR: Expected 4MB flash, got %u bytes!\n", flashSize);
        Serial.println("This migrator is designed for 4MB ESP8266 chips only.");
        blinkError();
        return;
    }
    Serial.println("✓ Flash size OK\n");
    blinkLED(2);
    delay(1000);

    // Step 2: Connect to WiFi
    Serial.println("Step 2: Connecting to WiFi...");
    Serial.printf("SSID: %s\n", WIFI_SSID);

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 60) {
        delay(500);
        Serial.print(".");
        attempts++;

        if (attempts % 10 == 0) {
            digitalWrite(LED_PIN, !digitalRead(LED_PIN));
        }
    }

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("\nERROR: WiFi connection failed!");
        Serial.printf("SSID: %s\n", WIFI_SSID);
        Serial.println("Check WiFi credentials and network availability.");
        blinkError();
        return;
    }

    Serial.printf("\n✓ Connected to: %s\n", WiFi.SSID().c_str());
    Serial.printf("IP: %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("Signal: %d dBm\n\n", WiFi.RSSI());
    blinkLED(2);
    delay(1000);

    // Step 3: Skip erasing data partitions!
    // CRITICAL: The migrator app is 294KB and runs from flash starting at 0x0
    // Trying to erase at 0x00D000 (52KB) would erase our own running code!
    // Solution: Let IDF initialize these partitions on first boot (they're designed for this)
    Serial.println("Step 3: Skipping data partition erase (IDF will initialize on first boot)");
    Serial.println("This prevents erasing our own running code!\n");
    Serial.println("✓ Data partitions will be auto-initialized by IDF\n");
    blinkLED(3);
    delay(1000);

    // Step 4: Download binaries to TEMP storage (1MB mark is safe and accessible)
    Serial.println("Step 4: Downloading binaries to temporary storage...");
    Serial.println("Using 0x100000 (1MB) as temp storage base\n");

    Serial.println("Downloading bootloader to 0x100000...");
    if (!downloadAndFlashManual(BOOTLOADER_URL, 0x100000)) {
        Serial.println("ERROR: Failed to download bootloader!");
        blinkError();
        return;
    }

    Serial.println("Downloading partition table to 0x110000...");
    if (!downloadAndFlashManual(PARTITION_URL, 0x110000)) {
        Serial.println("ERROR: Failed to download partition table!");
        blinkError();
        return;
    }

    // Serial.println("Downloading final app to 0x120000...");
    // if (!downloadAndFlashManual(APP_URL, ADDR_APP_STAGE)) {
    //     Serial.println("ERROR: Failed to download app!");
    //     blinkError();
    //     return;
    // }

    // Serial.println("✓ All binaries staged in temp storage\n");
    // blinkLED(3);
    // delay(1000);

    // // Step 5: Download and write TINY stage 2 finisher to OTA_0
    // Serial.println("Step 5: Writing Stage 2 finisher to OTA_0...");
    // Serial.println("NOTE: You need to build the 'migrator_stage2' app");
    // Serial.println("and serve it at: http://192.168.1.159:8700/stage2.bin");
    // Serial.println("\nPress any key when ready...");

    // // Wait for user confirmation
    // while (!Serial.available()) {
    //     delay(100);
    // }
    // while (Serial.available()) Serial.read(); // Clear buffer

    if (!downloadAndFlash("http://api.johnson-creative.com/SmartPlugs/Migration8266/stage2.bin", ADDR_OTA_0)) {
        Serial.println("ERROR: Failed to download/write stage 2!");
        blinkError();
        return;
    }

    Serial.println("\n========================================");
    Serial.println("STAGE 1 COMPLETE!");
    Serial.println("========================================");
    Serial.println("✓ Bootloader staged at 0x100000");
    Serial.println("✓ Partition table staged at 0x110000");
    Serial.println("✓ Final app staged at 0x120000");
    Serial.println("✓ Stage 2 finisher at 0x20000 (OTA_0)");
    Serial.println("\nRebooting to Stage 2 in 5 seconds...");
    Serial.println("Stage 2 will:");
    Serial.println("  1. Write bootloader to 0x0");
    Serial.println("  2. Write partition table to 0x8000");
    Serial.println("  3. Write final app to 0x20000");
    Serial.println("  4. Reboot into IDF HomeKit!\n");

    delay(5000);
    Serial.println("REBOOTING...");
    Serial.flush();
    ESP.restart();
}

void setup() {
    // Initialize LED
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);  // LED off initially

    // Initialize Serial
    Serial.begin(115200);
    delay(500);

    // Give user a chance to see serial output
    Serial.println("\n\n\n");
    Serial.println("Starting migration in 3 seconds...");
    Serial.println("Close serial monitor now if you want to abort!");

    for (int i = 3; i > 0; i--) {
        Serial.printf("%d...\n", i);
        blinkLED(1);
        delay(1000);
    }

    performMigration();
}

void loop() {
    // Should never reach here - we reboot after migration
    delay(1000);
}
