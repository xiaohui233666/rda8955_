/* boot jumper for Cortex-M3/4 */

#ifndef BOOT_HW_H__
#define BOOT_HW_H__

#include <board.h>

#define BOOT(start_addr) \
    do {                                                \
        unsigned int _boot_addr = (unsigned int)start_addr;    \
        _boot_addr = *(unsigned int *)(_boot_addr + 4);          \
        /* boot to application firmware address */      \
        printf("boot user partition:0x%08x, pc:0x%08x\n", start_addr, _boot_addr); \
        printf("\n"); \
        while (!((*((volatile unsigned int *)(0x40012020))) & (0x01UL << 18))); \
        rda_boot(_boot_addr); \
    } while (0)

#endif
