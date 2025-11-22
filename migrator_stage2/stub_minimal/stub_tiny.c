/*
 * Ultra-minimal ESP8266 bootloader stub
 * Loads IDF bootloader from 0x100000 and jumps to it
 * Uses small buffer and loads segments individually
 */

typedef unsigned char uint8_t;
typedef unsigned int uint32_t;

// ROM functions (provided by boot ROM)
extern int SPIRead(uint32_t addr, void *dst, uint32_t size);

#define BOOTLOADER_ADDR 0x100000
#define BUF_SIZE 4096

struct __attribute__((packed)) esp_image_header {
    uint8_t magic;
    uint8_t segment_count;
    uint8_t spi_mode;
    uint8_t spi_speed_size;
    uint32_t entry_addr;
};

struct __attribute__((packed)) esp_segment_header {
    uint32_t load_addr;
    uint32_t data_len;
};

// Small buffer in BSS
static uint8_t buf[BUF_SIZE];

void call_user_start(void) {
    uint32_t addr = BOOTLOADER_ADDR;

    // Read header
    SPIRead(addr, buf, sizeof(struct esp_image_header) + 64);
    struct esp_image_header *hdr = (struct esp_image_header *)buf;

    if (hdr->magic != 0xE9) {
        while(1);  // Hang if invalid
    }

    addr += sizeof(struct esp_image_header);

    // Load each segment
    for (int i = 0; i < hdr->segment_count && i < 16; i++) {
        // Read segment header
        SPIRead(addr, buf, sizeof(struct esp_segment_header));
        struct esp_segment_header *seg = (struct esp_segment_header *)buf;

        addr += sizeof(struct esp_segment_header);

        // Load segment data in chunks
        uint32_t *dst = (uint32_t *)seg->load_addr;
        uint32_t remaining = seg->data_len;

        while (remaining > 0) {
            uint32_t chunk = remaining > BUF_SIZE ? BUF_SIZE : remaining;
            SPIRead(addr, buf, chunk);

            // Copy to destination
            uint32_t *src = (uint32_t *)buf;
            for (uint32_t j = 0; j < (chunk + 3) / 4; j++) {
                dst[j] = src[j];
            }

            dst += chunk / 4;
            addr += chunk;
            remaining -= chunk;
        }
    }

    // Jump to bootloader
    void (*entry)(void) = (void (*)(void))hdr->entry_addr;
    entry();

    while(1);
}
