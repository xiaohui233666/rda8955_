#ifndef BOOTLOADER_TRIPLE_AREA_H_
#define BOOTLOADER_TRIPLE_AREA_H_
#include <stdint.h>
#include <stdbool.h>
bool bootloader_triple_area_prepare(void);
uint32_t bootloader_triple_area_select(void);
void bootloader_triple_area_go(uint32_t);
#endif
