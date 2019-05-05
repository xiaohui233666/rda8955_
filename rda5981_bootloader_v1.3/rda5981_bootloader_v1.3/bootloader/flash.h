/*
 * flash API, which should be implemented by hardware layer
 */
#ifndef FLASH_H__
#define FLASH_H__

int flash_unlock(void);
int flash_lock(void);

int flash_erase(uint32_t addr, uint32_t length);
int flash_write(uint32_t addr, uint32_t length, uint32_t mem_addr);

#endif
