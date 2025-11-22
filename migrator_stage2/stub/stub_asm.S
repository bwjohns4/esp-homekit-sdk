/*
 * Ultra-minimal bootloader stub - just jump to 0x100000
 * This is an ESP8266 image header followed by a jump instruction
 */

.section .text
.globl _start
.align 4

_start:
    /* ESP8266 image header (8 bytes) */
    .byte 0xE9          /* magic */
    .byte 0x01          /* 1 segment */
    .byte 0x02          /* SPI mode: DIO */
    .byte 0x00          /* SPI speed: 40MHz, size: 512KB */
    .long entry_point   /* entry address */

    /* Segment header (8 bytes) */
    .long 0x40100000    /* load address (IRAM) */
    .long code_end - entry_point  /* segment length */

entry_point:
    /* Just jump to the IDF bootloader at 0x100000 */
    movi a0, 0x40100000  /* Return address (doesn't matter) */
    movi a2, 0x100000    /* Bootloader address in flash */

    /* Load bootloader from flash and jump to it */
    /* For now, just infinite loop - real version needs SPIRead */
    j .

code_end:
    .byte 0xFFFFFFFF    /* Padding */

.align 4
