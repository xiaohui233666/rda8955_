#ifndef BOOTLOADER_PING_PONG_H_
#define BOOTLOADER_PING_PONG_H_
#include <stdint.h>
#include <stdbool.h>
bool bootloader_ping_pong_prepare(void);
uint32_t bootloader_ping_pong_select(void);
void bootloader_ping_pong_go(uint32_t);
#endif
