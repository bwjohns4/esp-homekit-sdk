/* Minimal linker script for ESP8266 bootloader stub */

MEMORY
{
  iram (RX) : ORIGIN = 0x40100000, LENGTH = 0x8000
}

SECTIONS
{
  .text : {
    *(.text*)
  } > iram
}

ENTRY(call_user_start)
