#include "bootloader_triple_area.h"
#include "bootloader.h"
#include "crc32.h"
#include "flash.h"

static const struct triple_area_layout* bootloader_get_layout(void) {
    extern const struct triple_area_layout _triple_area_layout_table;

    return &_triple_area_layout_table;
}

static const struct firmware_info* bootloader_get_firmware_info(void) {
    const struct firmware_info *info = NULL;
    const struct triple_area_layout *layout = bootloader_get_layout();

    if (layout) {
        info = (const struct firmware_info *)layout->info_addr;
    }

    return info;
}

static bool image_is_valid(const struct image_header *header)
{
    if ((header) && (header->magic == IMAGE_MAGIC)) {
        return true;
    }

    return false;
}

static const struct image_item* image_get_item(int type) {
    const struct image_item *image = NULL;
    const struct triple_area_layout* layout = bootloader_get_layout();

    /* get image */
    if (type == IMAGE_UPGRADE) {
        image = &(layout->upgrade);
    } else if (type == IMAGE_SAFE) {
        image = &(layout->safe);
    }

    return image;
}

static bool image_verify(int type)
{
    bool result = false;
    const struct image_item *image = image_get_item(type);
    if (!image) goto __exit;

    if (image->type == IMAGE_TYPE_FLASH) {
        uint32_t crc32;
        const struct image_header *header;

        header = (const struct image_header*)image->image.addr;
        if (header->magic == IMAGE_MAGIC) {
            crc32 = crc32_halfbyte((uint8_t *)(header + 1), header->size, 0);

            if (crc32 == header->crc32) result = true;
        }
    } else if (image->type == IMAGE_TYPE_FILE) {
        /* TODO */
    }

__exit:
    return result;
}

/*static const struct image_header* image_get_header(int type)
{
    const struct image_header *header = NULL;
    const struct image_item *image = image_get_item(type);

    if (!image) return header;

    if (image->type == IMAGE_TYPE_FLASH)
    {
        header = (const struct image_header*)image->image.addr;
    }

    return header;
}*/

static bool image_unpack(int type)
{
    bool result = true;
    const struct image_item *image = image_get_item(type);
    if (!image) {
        result = false;
        goto __exit;
    }

    if (image->type == IMAGE_TYPE_FLASH) {
        uint8_t *src_ptr, *flash_ptr;
        const struct image_header* header;
        const struct triple_area_layout *layout = bootloader_get_layout();

        header = (const struct image_header*)image->image.addr;
        /* check image valid */
        if (image_is_valid(header) == false) {
            result = false;
            goto __exit;
        }

        /* unpack firmware */
        src_ptr = (uint8_t *)(header + 1);
        flash_ptr = (uint8_t *)layout->firmware_addr;

        flash_unlock();
        /* erase flash sector */
        flash_erase((uint32_t)flash_ptr, header->size);
        flash_write((uint32_t)flash_ptr, header->size, (uint32_t)src_ptr);

        /* update firmware information */
        {
            struct firmware_info info;
            memset(&info, 0, sizeof(info));
            info.addr  = layout->firmware_addr;
            info.magic = FIRMWARE_MAGIC;
            info.crc32 = header->crc32;
            info.size = header->size;
            memcpy(info.version, header->version, sizeof(header->version));

            flash_erase(layout->info_addr, sizeof(struct firmware_info));
            flash_write(layout->info_addr, sizeof(struct firmware_info), (uint32_t)&info);
        }
        flash_lock();
    } else if (image->type == IMAGE_TYPE_FILE) {
        /* TODO */
    }

__exit:
    return result;
}

static bool image_get_version(int type, char version[VERSION_SZ])
{
    bool result = true;
    const struct image_item *image = image_get_item(type);
    if (!image) {
        result = false;
        goto __exit;
    }

    if (image->type == IMAGE_TYPE_FLASH) {
        const struct image_header* header;

        header = (const struct image_header*)image->image.addr;
        /* check image valid */
        if (image_is_valid(header) == false) {
            result = false;
            goto __exit;
        }

        /* copy image version */
        memcpy(version, header->version, sizeof(header->version));
    } else if (image->type == IMAGE_TYPE_FILE) {
        /* TODO */
    }

__exit:
    return result;
}

static bool firmware_is_valid(void)
{
    const struct firmware_info *info;

    info = bootloader_get_firmware_info();
    if (info) {
        if (info->magic == FIRMWARE_MAGIC) {
            uint32_t crc32;

            /* calculate crc32 */
            crc32 = crc32_halfbyte((const void* )info->addr, info->size, 0);
            if (crc32 == info->crc32)
                return true;
        }
    }

    return false;
}

static bool firmware_get_version(char version[VERSION_SZ])
{
    const struct firmware_info *info;

    info = bootloader_get_firmware_info();
    if (info) {
        if (info->magic == FIRMWARE_MAGIC) {
            memcpy(version, info->version, sizeof(info->version));
            return true;
        }
    }

    return false;
}

static bool bootloader_upgrade(void)
{
    char fw_ver[VERSION_SZ];
    char im_ver[VERSION_SZ];

    /* check current firmware */
    if (firmware_is_valid() == false) {
        /* false, to upgrade */
        goto __upgrade;
    } else {
        /* current firmware is valid. */

        firmware_get_version(fw_ver);

        if (image_get_version(IMAGE_UPGRADE, im_ver) == false)
            return true; /* upgrade image invalid but current firmware is ok, not upgrade */

        /* check version with upgrade image */
        if (strcmp(fw_ver, im_ver) == 0) {
            /* ok, return */
            return true;
        }
        /* not the same version, continue to do upgrade */
    }

__upgrade:
    /* check upgrade image */
    if (image_verify(IMAGE_UPGRADE) == true) {
        /* do upgrade */
        if (image_unpack(IMAGE_UPGRADE) == false) {
            /* rollback to safe image */
            image_unpack(IMAGE_SAFE);
        }
    } else {
        /* rollback to safe image */
        image_unpack(IMAGE_SAFE);
    }

    return true;
}

bool bootloader_triple_area_prepare(void)
{
    return bootloader_upgrade();
}

uint32_t bootloader_triple_area_select(void)
{
    uint32_t addr = 0;
    const struct triple_area_layout *layout = 0;
    layout = bootloader_get_layout();
    if (layout && firmware_is_valid()) {
        addr = layout->firmware_addr;
    }
    return addr;
}

void bootloader_triple_area_go(uint32_t addr)
{
    /* boot to application firmware address */
    BOOT(addr);
    return;
}
