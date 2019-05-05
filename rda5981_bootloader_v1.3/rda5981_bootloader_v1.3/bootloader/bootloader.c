#include "bootloader.h"
#include "crc32.h"
#include "bootloader_ping_pong.h"
#include "bootloader_copy.h"
#include "bootloader_triple_area.h"

int bootloader_main(void)
{
    /*const struct bootloader_template bootloader =
    {
        bootloader_ping_pong_prepare,
        bootloader_ping_pong_select,
        bootloader_ping_pong_go,
    };*/

    const struct bootloader_template bootloader = {
        bootloader_copy_prepare,
        bootloader_copy_select,
        bootloader_copy_go,
    };

    /*const struct bootloader_template bootloader =
    {
        bootloader_triple_area_prepare,
        bootloader_triple_area_select,
        bootloader_triple_area_go,
    };*/

    do {
        uint32_t addr = 0;
        if (!bootloader.bootloader_prepare || !bootloader.bootloader_prepare()) {
            printf("\nbootloader prepare failed\n");
            break;
        }

        if (!bootloader.bootloader_select || (addr = bootloader.bootloader_select()) == 0) {
            printf("bootloader didn't select a valid address to jump 0x%08x\n", addr);
            break;
        }

        if (bootloader.bootloader_go) {
            printf("boot from 0x%08x\n", addr);
            bootloader.bootloader_go(addr);
        }
    } while (0);

    printf("if you see this, error occurs in bootloading!!!\n");
    return 0;
}
