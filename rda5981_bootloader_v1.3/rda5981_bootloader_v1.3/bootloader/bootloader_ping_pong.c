#include "bootloader_ping_pong.h"
#include "bootloader.h"
#include "crc32.h"

static const struct image_item* get_ping_image(void) {
    extern const struct ping_pong_layout _ping_pong_layout_table;
    return &(_ping_pong_layout_table.ping);
}

static const struct image_item* get_pong_image(void) {
    extern const struct ping_pong_layout _ping_pong_layout_table;
    return &(_ping_pong_layout_table.pong);
}

static bool ping_pong_image_verify(const struct image_header* header)
{
    bool valid = false;
    uint32_t crc32 = 0;
    printf("size:%d\n", header->size);
    if (header && header->magic == IMAGE_MAGIC) {
        crc32 = crc32_halfbyte((uint8_t *)(header + 1), header->size, 0);

        if (crc32 == header->crc32) {
            valid = true;
        }
    }
    return valid;
}

static uint32_t get_ping_pong_boot_addr(void)
{
    uint32_t boot_addr = 0;
    bool ping_valid = false;
    bool pong_valid = false;
    const struct image_item* ping_image;
    const struct image_item* pong_image;
    const struct image_header* ping_header;
    const struct image_header* pong_header;

    ping_image = get_ping_image();
    pong_image = get_pong_image();
    ping_header = (const struct image_header*)(ping_image->image.addr);
    pong_header = (const struct image_header*)(pong_image->image.addr);

    ping_valid = ping_pong_image_verify(ping_header);
    pong_valid = ping_pong_image_verify(pong_header);

    printf("ping valid:%s\n", ping_valid?"yes":"no");
    printf("pong valid:%s\n", pong_valid?"yes":"no");
    if (ping_valid && pong_valid) {
        //compare version
        boot_addr = (strcmp((char*)ping_header->version, (char*)pong_header->version) > 0)?(uint32_t)(ping_header + 1):(uint32_t)(pong_header + 1);
    } else {
        boot_addr = ping_valid?(uint32_t)(ping_header + 1):(uint32_t)(pong_header + 1);
    }
    return boot_addr;
}

bool bootloader_ping_pong_prepare(void)
{
    return true;
}

uint32_t bootloader_ping_pong_select(void)
{
    uint32_t addr = 0;
    addr = get_ping_pong_boot_addr();
    return addr;
}

void bootloader_ping_pong_go(uint32_t addr)
{
    BOOT(addr);
    return;
}






