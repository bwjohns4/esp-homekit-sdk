#include <Arduino.h>
extern "C" {
    #include "user_interface.h"
    int ets_printf(const char *fmt, ...);
    extern int spi_flash_read(uint32_t addr, void *dst, uint32_t size);
}

#define REAL_BOOTLOADER_ADDR 0x100000
#define REAL_BOOTLOADER_SIZE 32768

// ESP8266 image header (first 8 bytes)
struct esp_image_header {
    uint8_t magic;           // 0xE9
    uint8_t segment_count;
    uint8_t spi_mode;
    uint8_t spi_speed_size;
    uint32_t entry_addr;
} __attribute__((packed));

// Segment header (8 bytes)
struct esp_segment_header {
    uint32_t load_addr;
    uint32_t data_len;
} __attribute__((packed));

#define ESP_IMAGE_HEADER_MAGIC 0xE9

// Use IRAM for bootloader buffer to avoid flash access issues
static uint8_t IRAM_ATTR bootloader_ram[REAL_BOOTLOADER_SIZE];

void IRAM_ATTR load_real_bootloader() {
    ets_printf("\n\n=== Bootloader Stub ===\n");
    ets_printf("Loading IDF bootloader from 0x%06X\n", REAL_BOOTLOADER_ADDR);

    // Read bootloader into RAM
    spi_flash_read(REAL_BOOTLOADER_ADDR, (uint32_t*)bootloader_ram, REAL_BOOTLOADER_SIZE);

    // Parse header
    struct esp_image_header* hdr = (struct esp_image_header*)bootloader_ram;

    if (hdr->magic != ESP_IMAGE_HEADER_MAGIC) {
        ets_printf("ERROR: Bad magic 0x%02X\n", hdr->magic);
        while(1);
    }

    ets_printf("Segments: %d, Entry: 0x%08X\n", hdr->segment_count, hdr->entry_addr);

    // Load segments
    uint8_t* ptr = bootloader_ram + sizeof(struct esp_image_header);

    for (int i = 0; i < hdr->segment_count; i++) {
        struct esp_segment_header* seg = (struct esp_segment_header*)ptr;
        ptr += sizeof(struct esp_segment_header);

        ets_printf("Seg %d: 0x%08X (%u bytes)\n", i, seg->load_addr, seg->data_len);

        // Copy to target
        uint32_t* dst = (uint32_t*)seg->load_addr;
        uint32_t* src = (uint32_t*)ptr;
        uint32_t len = (seg->data_len + 3) / 4;

        for (uint32_t j = 0; j < len; j++) {
            dst[j] = src[j];
        }

        ptr += seg->data_len;
    }

    ets_printf("Jumping to 0x%08X\n\n", hdr->entry_addr);

    // Jump to bootloader entry
    void (*entry)(void) = (void(*)(void))hdr->entry_addr;
    entry();

    while(1);
}

void setup() {
    delay(100);
    load_real_bootloader();
}

void loop() {}
