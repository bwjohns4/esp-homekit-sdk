/*
 * ESP8266 Bootloader Stub
 * Loads real IDF bootloader from 0x100000 and jumps to it
 */

#include <Arduino.h>

extern "C" {
    #include "spi_flash.h"
    int ets_printf(const char *fmt, ...);
}

#define BOOTLOADER_ADDR 0x100000
#define BOOTLOADER_SIZE 32768

struct esp_image_header {
    uint8_t magic;
    uint8_t segment_count;
    uint8_t spi_mode;
    uint8_t spi_speed_size;
    uint32_t entry_addr;
} __attribute__((packed));

struct esp_segment_header {
    uint32_t load_addr;
    uint32_t data_len;
} __attribute__((packed));

// Static buffer in BSS (not IRAM, saves space)
static uint8_t bootloader_buf[BOOTLOADER_SIZE];

void setup() {
    delay(10);

    ets_printf("\n\n=== Bootloader Stub ===\n");
    ets_printf("Loading IDF bootloader from 0x%06X\n", BOOTLOADER_ADDR);

    // Read bootloader into RAM
    spi_flash_read(BOOTLOADER_ADDR, (uint32_t*)bootloader_buf, BOOTLOADER_SIZE);

    esp_image_header* hdr = (esp_image_header*)bootloader_buf;

    if (hdr->magic != 0xE9) {
        ets_printf("ERROR: Bad magic 0x%02X\n", hdr->magic);
        while(1) delay(1000);
    }

    ets_printf("Segments: %d, Entry: 0x%08X\n", hdr->segment_count, hdr->entry_addr);

    uint8_t* ptr = bootloader_buf + sizeof(esp_image_header);

    for (int i = 0; i < hdr->segment_count; i++) {
        esp_segment_header* seg = (esp_segment_header*)ptr;
        ptr += sizeof(esp_segment_header);

        ets_printf("  Seg %d: 0x%08X (%u bytes)\n", i, seg->load_addr, seg->data_len);

        // Copy segment to target address
        memcpy((void*)seg->load_addr, ptr, seg->data_len);

        ptr += seg->data_len;
    }

    ets_printf("Jumping to 0x%08X\n\n", hdr->entry_addr);
    delay(10);

    // Jump to bootloader entry point
    void (*entry)(void) = (void(*)(void))hdr->entry_addr;
    entry();

    // Never reached
    while(1) delay(1000);
}

void loop() {
    // Never reached
}
