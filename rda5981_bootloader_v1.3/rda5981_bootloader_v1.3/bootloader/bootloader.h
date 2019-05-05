#ifndef BOOTLOADER_H__
#define BOOTLOADER_H__

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "bootloader_config.h"

#include <rda_api.h>

#ifdef TARGET_RDA5981
#include <rda5981_bootrom_api.h>
#include "hw/boot_cm.h"
#endif

#define PATH_SZ             64
#define VERSION_SZ          24
#define BUFFER_SZ           4096
#define SECTOR_SZ           4096

#define IMAGE_TYPE_FLASH    0
#define IMAGE_TYPE_FILE     1

#define IMAGE_UPGRADE       0
#define IMAGE_SAFE          1

#define IMAGE_MAGIC         0xAEAE
#define FIRMWARE_MAGIC      0xEAEA

struct firmware_info {
    uint32_t magic;
    uint8_t  version[VERSION_SZ];

    uint32_t addr;
    uint32_t size;
    uint32_t crc32;
    uint32_t bootaddr;//add for rf_test
    uint32_t bootmagic;
};

/*image_header is directly mapped from nor flash, and byte order is little-endian,
  if target device is big-endian, we should convert data before we use this struct instance*/
struct image_header {
    uint16_t magic;
    uint8_t  encrypt_algo;
    uint8_t  resv[1];

    uint8_t  version[VERSION_SZ];

    uint32_t crc32;
    uint32_t size;
    //uint8_t  padding[4060];
};

struct image_item {
    uint32_t type;

    union image_type {
        uint32_t addr;
        uint8_t  path[PATH_SZ];
    } image;
};

struct ping_pong_layout {
    struct image_item ping;
    struct image_item pong;
};

struct copy_layout {
    uint32_t info_addr;  //firmware info address, store a "struct firmware_info"
    uint32_t firmware_addr;  //firmware store address, as EIP indicates, it's also program start address
    uint32_t image_size_max; //firmware size limmit
    struct image_item upgrade; //upgrade image store address
};

struct triple_area_layout {
    uint32_t info_addr;
    uint32_t firmware_addr;

    struct image_item upgrade;
    struct image_item safe;
};

struct bootloader_template {
    bool (*bootloader_prepare)(void);
    uint32_t (*bootloader_select)(void);
    void (*bootloader_go)(uint32_t);
};

int bootloader_main(void);

#endif
