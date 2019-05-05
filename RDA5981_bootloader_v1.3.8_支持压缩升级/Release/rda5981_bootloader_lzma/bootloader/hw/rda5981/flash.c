/* flash API */
#include <bootloader.h>
#include <flash.h>
#include <rda_api.h>
#define IBUF_SZ     SECTOR_SIZE

ALIGN(4)
uint8_t ibuf[IBUF_SZ];

//static const uint8_t firmware_info[FM_INFO_SECTOR_SZ] __attribute__ ((at(FM_INFO_ADDR))) = {0xff};

int flash_unlock()
{
    //__disable_irq();
    return 0;
}

int flash_lock()
{
    //__ISB();
    //__enable_irq();
    return 0;
}

int flash_erase(uint32_t addr, uint32_t length)
{
#if 1
    return erase_flash(addr, length);
#else
    int index;

    for (index = 0; index < length; index += IBUF_SZ) {
        erase_sector(addr);
        addr += IBUF_SZ;
    }
    return 0;
#endif
}

int flash_write(uint32_t flash_addr, uint32_t length, uint32_t mem_addr)
{
    int index;

    for (index = 0; index < length; index += IBUF_SZ) {
        int chunk;

        if (index + IBUF_SZ > length) {
            chunk = length - index;
            memset(ibuf, 0xff, IBUF_SZ);
        } else
            chunk = IBUF_SZ;

        /* copy to the ibuf, then write to the flash */
        memcpy(ibuf, (void*)mem_addr, chunk);
        //read_flash(mem_addr, ibuf, chunk);

        program_flash(flash_addr, (char *)ibuf, IBUF_SZ);

        flash_addr += IBUF_SZ;
        mem_addr += IBUF_SZ;
    }

    return 0;
}
