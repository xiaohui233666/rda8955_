#ifndef BOOTLOADER_COPY_H_
#define BOOTLOADER_COPY_H_
#include <stdint.h>
#include <stdbool.h>
bool bootloader_copy_prepare(void);
uint32_t bootloader_copy_select(void);
void bootloader_copy_go(uint32_t);
#endif
