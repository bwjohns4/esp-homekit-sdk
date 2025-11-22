/*
 * Minimal ESP8266 Bootloader Stub
 * Fits in sector 0 (4KB), loads real IDF bootloader from 0x100000
 */

#include <stdint.h>
#include <stdbool.h>

// ROM functions
extern int ets_printf(const char *fmt, ...);
extern void ets_delay_us(uint32_t us);
extern int spi_flash_read(uint32_t addr, void *dst, size_t size);

#define REAL_BOOTLOADER_ADDR 0x100000
#define BUFFER_SIZE 32768

// ESP8266 image header (ESP-IDF format)
typedef struct {
    uint8_t magic;              // 0xE9
    uint8_t segment_count;      // Number of segments
    uint8_t spi_mode;           // SPI mode
    uint8_t spi_speed_size;     // SPI speed and size
    uint32_t entry_addr;        // Entry point address
} __attribute__((packed)) esp_image_header_t;

typedef struct {
    uint32_t load_addr;         // Where to load this segment
    uint32_t data_len;          // Length of segment data
} __attribute__((packed)) esp_image_segment_header_t;

#define ESP_IMAGE_HEADER_MAGIC 0xE9

// RAM buffer for bootloader (32KB)
static uint8_t bootloader_ram[BUFFER_SIZE] __attribute__((aligned(4)));

void __attribute__((noreturn)) load_bootloader(void) {
    ets_printf("\n\nBootloader Stub v1.0\n");
    ets_printf("Loading IDF bootloader from 0x%06X...\n", REAL_BOOTLOADER_ADDR);

    // Read entire bootloader into RAM
    if (spi_flash_read(REAL_BOOTLOADER_ADDR, bootloader_ram, BUFFER_SIZE) != 0) {
        ets_printf("ERROR: Failed to read bootloader!\n");
        while(1) {}
    }

    ets_printf("Bootloader loaded to RAM\n");

    // Parse image header
    esp_image_header_t *header = (esp_image_header_t *)bootloader_ram;

    if (header->magic != ESP_IMAGE_HEADER_MAGIC) {
        ets_printf("ERROR: Invalid bootloader magic: 0x%02X\n", header->magic);
        while(1) {}
    }

    ets_printf("Valid image, segments: %d, entry: 0x%08X\n",
               header->segment_count, header->entry_addr);

    // Load all segments
    uint8_t *read_ptr = bootloader_ram + sizeof(esp_image_header_t);

    for (int i = 0; i < header->segment_count; i++) {
        esp_image_segment_header_t *seg = (esp_image_segment_header_t *)read_ptr;
        read_ptr += sizeof(esp_image_segment_header_t);

        ets_printf("  Segment %d: addr=0x%08X len=%u\n",
                   i, seg->load_addr, seg->data_len);

        // Copy segment to target address
        uint32_t *dest = (uint32_t *)seg->load_addr;
        uint32_t *src = (uint32_t *)read_ptr;
        uint32_t words = (seg->data_len + 3) / 4;

        for (uint32_t j = 0; j < words; j++) {
            dest[j] = src[j];
        }

        read_ptr += seg->data_len;
    }

    ets_printf("Jumping to 0x%08X...\n\n", header->entry_addr);
    ets_delay_us(10000);  // 10ms delay for serial output

    // Jump to entry point
    void (*entry)(void) = (void (*)(void))header->entry_addr;
    entry();

    // Never reached
    while(1) {}
}

void call_user_start(void) {
    load_bootloader();
}
