#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <board.h>

#include <bootloader.h>

#define RDA_TEST_BOOTLOADER

#ifdef RDA_TEST_BOOTLOADER
#include <flash.h>
#include <crc32.h>
#endif

int main(void)
{
    /* disable interrupt first */
    board_init();

    printf("bootloader starts...\n");
    bootloader_main();

    return 0;
}
