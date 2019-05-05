#include "rda_api.h"
#ifdef TARGET_RDA5981
#include <rda5981_bootrom_api.h>
#endif

bool check_align(int address, int align);

void rda5981_spi_flash_erase_64KB_block(uint32_t addr)
{
    spi_wip_reset();
    spi_write_reset();
    WRITE_REG32(FLASH_CTL_TX_CMD_ADDR_REG, CMD_64KB_BLOCK_ERASE | (addr<<8));
    wait_busy_down();
    spi_wip_reset();
}
void rda5981_spi_erase_partition(void *src, uint32_t counts)
{
    uint32_t a4k, a64k, a64kend, a4kend, atmp;
    if (counts > 0x00) {

        a4k  = ((uint32_t)src                          ) & (~((0x01UL << 12) - 0x01UL));
        a64k = ((uint32_t)src + (0x01UL << 16) - 0x01UL) & (~((0x01UL << 16) - 0x01UL));
        a64kend = ((uint32_t)src + counts                          ) & (~((0x01UL << 16) - 0x01UL));
        a4kend  = ((uint32_t)src + counts + (0x01UL << 12) - 0x01UL) & (~((0x01UL << 12) - 0x01UL));

        for (atmp = a4k; atmp < a4kend; atmp += (0x01UL << 12)) {
            if (a64kend > a64k) {
                if (atmp == a64k) {
                    for (; atmp < a64kend; atmp += (0x01UL << 16)) {
                        rda5981_spi_flash_erase_64KB_block(atmp);
                    }
                    if (atmp == a4kend)
                        break;
                }
            }
            rda5981_spi_flash_erase_4KB_sector(atmp);
        }
    }
}

IAPCode erase_flash(int address, uint32_t len) {
#ifdef IAPDEBUG
    printf("IAP: Erasing at %x\r\n", address);
#endif
    if (check_align(address, 0x1000)) {
        printf("erase AlignError, addr:0x%x\n", address);
        return AlignError;
    }

    if (check_align(len, 0x1000)) {
        len = (len + 0x1000) & (~(0x1000 -1));
    }
    address &= (FLASH_SIZE-1);
    rda5981_spi_erase_partition((void *)address, len);

    return Success;
}

IAPCode erase_sector(int address) {
#ifdef IAPDEBUG
    printf("IAP: Erasing at %x\r\n", address);
#endif
    if (check_align(address, 0x1000)) {
        printf("erase AlignError\n");
        return AlignError;
    }
    address &= (FLASH_SIZE-1);
    rda5991h_erase_flash((void *)address, SECTOR_SIZE);

    return Success;
}

IAPCode read_flash(int address, char *data, unsigned int length) {
#ifdef IAPDEBUG
    printf("IAP: read flash at %x with length %d, buf:%x\r\n", address, length, (int)data);
#endif
    if (check_align(address, 0x1000) || check_align((int)data, 4)) {
        printf("read AlignError\n");
        return AlignError;
    }
    address &= (FLASH_SIZE-1);
    spi_flash_read_data_for_mbed((void *)address, (void *)data, length);
    return Success;
}

IAPCode program_flash(int address, char *data, unsigned int length) {
#ifdef IAPDEBUG
    printf("IAP: Programming flash at %x with length %d\r\n", address, length);
#endif
    if (check_align(address, 0x1000) || check_align((int)data, 4)) {
        printf("program AlignError\n");
        return AlignError;
    }
    address &= (FLASH_SIZE-1);
    rda5991h_write_flash((void *)address, (void *)data, length);
    return Success;
}

/* Check if address is correctly aligned
   Returns true on violation */
bool check_align(int address, int align) {
    bool retval = address & (align-1);
#ifdef IAPDEBUG
    if (retval)
        printf("IAP: Alignment violation\r\n");
#endif
    return retval;
}

