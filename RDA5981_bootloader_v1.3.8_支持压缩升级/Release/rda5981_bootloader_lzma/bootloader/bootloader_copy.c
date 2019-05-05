#include "bootloader_copy.h"
#include "bootloader.h"
#include "crc32.h"
#include "flash.h"

static bool firmware_is_valid(void)
{
    extern const struct copy_layout copy_layout_table;
    bool valid = false;
    const struct firmware_info *info;

    info = (struct firmware_info*)copy_layout_table.info_addr;
    if (info
            && info->magic == FIRMWARE_MAGIC
            && (info->addr == copy_layout_table.firmware_addr)
            && (info->size < copy_layout_table.image_size_max)) {
        uint32_t crc32;

        /* calculate crc32 */
        crc32 = crc32_halfbyte((const void* )info->addr, info->size, 0);
        valid = (crc32 == info->crc32);
    }
    printf("firmware image %s\n", valid?"valid":"invalid");
    return valid;
}

static bool firmware_get_version(char version[VERSION_SZ])
{
    extern const struct copy_layout copy_layout_table;
    const struct firmware_info *info;

    info = (struct firmware_info*)copy_layout_table.info_addr;
    memset(version, 0, VERSION_SZ);
    if (info) {
        memcpy(version, info->version, sizeof(info->version));
        return true;
    }

    return false;
}

static bool upgrade_is_valid()
{
    extern const struct copy_layout copy_layout_table;
    bool result = false;
    const struct image_item *image = &copy_layout_table.upgrade;

    if (image && image->type == IMAGE_TYPE_FLASH) {
        uint32_t crc32;
        const struct image_header *header;

        header = (const struct image_header*)image->image.addr;
        if (header
                && (header->magic == IMAGE_MAGIC)
                && (header->size < copy_layout_table.image_size_max)) {
            crc32 = crc32_halfbyte((uint8_t *)(header + 1), header->size, 0);

            result = (crc32 == header->crc32);
        }
    }

    printf("upgrade image %s\n", result?"valid":"invalid");
    return result;
}

static bool upgrade_get_version(char version[VERSION_SZ])
{
    extern const struct copy_layout copy_layout_table;
    bool result = false;
    const struct image_item *image = &copy_layout_table.upgrade;
    memset(version, 0, VERSION_SZ);
    if (image && image->type == IMAGE_TYPE_FLASH) {
        const struct image_header* header;

        header = (const struct image_header*)image->image.addr;
        /* check image valid */
        if ((header) && (header->magic == IMAGE_MAGIC)) {
            /* copy image version */
            memcpy(version, header->version, sizeof(header->version));
            result = true;
        }
    }

    return result;
}

static bool upgrade()
{
    extern const struct copy_layout copy_layout_table;
    bool result = false;
    const struct image_item *image = &copy_layout_table.upgrade;
    flash_unlock();
    do {
        if (image && image->type == IMAGE_TYPE_FLASH) {
            int status = 0;
            const struct image_header* header;
            uint8_t *src_ptr, *flash_ptr;

            /* unpack firmware */
            header = (const struct image_header*)(image->image.addr);
            src_ptr = (uint8_t *)(header + 1);
            flash_ptr = (uint8_t *)copy_layout_table.firmware_addr;

			if (header->encrypt_algo == 0) {
	            /* erase flash sector */
	            if (0 == (status = flash_erase((uint32_t)flash_ptr, header->size))) {
	                if (0 == (status = flash_write((uint32_t)flash_ptr, header->size, (uint32_t)src_ptr))) {
	                    /*verify crc*/
	                    uint32_t crc32;
	                    crc32 = crc32_halfbyte((uint8_t *)(flash_ptr), header->size, 0);
	                    status = (crc32 == header->crc32)?0:1;
	                }
	            }
#ifdef OTA_LAMZ_COMPRESS_SUPPORT
			} else if (header->encrypt_algo == 1) {//lzma
	            if (0 == (status = flash_erase((uint32_t)flash_ptr, header->real_size))) {
	                if (0 == (status = lzma_update((uint32_t)src_ptr, (uint32_t)header->size,
						(uint32_t)flash_ptr, (uint32_t)header->real_size))) {
	                    /*verify crc*/
	                    uint32_t crc32;
	                    crc32 = crc32_halfbyte((uint8_t *)(flash_ptr), header->real_size, 0);
	                    status = (crc32 == header->real_crc32)?0:1;
	                }
	            }
#endif /*OTA_LAMZ_COMPRESS_SUPPORT*/
			} else {
				printf("unsupport encrypt algo:%d\n", header->encrypt_algo);
			}

            if(status != 0) {
                printf("upgrade flash operation failed in copy upgrade firmware\n");
                break;
            } else {
                printf("upgrade complete\n");
            }

            /* update firmware information */
            {
                struct firmware_info info;
                memset(&info, 0, sizeof(info));
                info.magic = FIRMWARE_MAGIC;
                memcpy(info.version, header->version, sizeof(header->version));
                info.addr  = copy_layout_table.firmware_addr;
#ifdef OTA_LAMZ_COMPRESS_SUPPORT
				if (header->encrypt_algo == 1) {//lzma
	                info.size = header->real_size;
	                info.crc32 = header->real_crc32;
				} else {
#endif /*OTA_LAMZ_COMPRESS_SUPPORT*/
	                info.size = header->size;
	                info.crc32 = header->crc32;
#ifdef OTA_LAMZ_COMPRESS_SUPPORT
				}
#endif /*OTA_LAMZ_COMPRESS_SUPPORT*/

                if (0 == (status = flash_erase(copy_layout_table.info_addr, sizeof(struct firmware_info)))) {
                    status = flash_write(copy_layout_table.info_addr, sizeof(struct firmware_info), (uint32_t)&info);
                }

                if (status != 0) {
                    printf("upgrade flash operation failed in update firmware information\n");
                    break;
                } else {
                    printf("update firmware information complete\n");
                }
            }

            result = true;

        }
    } while(0);

    flash_lock();
    return result;
}

static void bootloader_prepare_boot_addr(void)
{
    int status = 0;
    extern const struct copy_layout copy_layout_table;
    uint32_t addr;
    struct firmware_info info;

    memcpy((void *)(&info), (void *)(copy_layout_table.info_addr), sizeof(info));
    if ((info.bootmagic == FIRMWARE_MAGIC)
        && ((info.bootaddr >= 0x18001000) && (info.bootaddr < 0x18000000 + FLASH_SIZE)))
    {
        addr = info.bootaddr;
        printf("boot specified addr:0x%8x\n", addr);
        info.bootaddr = 0;

        if (0 == (status = flash_erase(copy_layout_table.info_addr, sizeof(struct firmware_info)))) {
            status = flash_write(copy_layout_table.info_addr, sizeof(struct firmware_info), (uint32_t)&info);
        }

        if (status != 0) {
            printf("upgrade flash operation failed in update firmware information\n");
        } else {
            BOOT(addr);
            return;
        }

    }
}


static bool bootloader_upgrade(void)
{
    char fw_ver[VERSION_SZ] = {'\0'};
    char upgrade_ver[VERSION_SZ] = {'\0'};

    bool firmware_valid = firmware_is_valid();
    bool upgrade_firmware_valid = upgrade_is_valid();
    /* check current firmware */
    if (true == firmware_valid) {
        /* current firmware is valid. */
        firmware_get_version(fw_ver);
        printf("current firmware is valid(ver:%s)\n", fw_ver);
        /* upgrade image invalid but current firmware is ok, not upgrade */
        if (false == upgrade_firmware_valid)
            return true;

        upgrade_get_version(upgrade_ver);
        printf("upgrade is valid(ver:%s)\n", upgrade_ver);
        /* check version with upgrade image */
        if (strcmp(fw_ver, upgrade_ver) == 0) {
            /* ok, return */
            return true;
        }
    }

    /* check upgrade image */
    if (true == upgrade_firmware_valid) {
        return upgrade();
    }

    return false;
}

bool bootloader_copy_prepare(void)
{
    bootloader_prepare_boot_addr();
    return bootloader_upgrade();
}

uint32_t bootloader_copy_select(void)
{
    extern const struct copy_layout copy_layout_table;
    return copy_layout_table.firmware_addr;
}

void bootloader_copy_go(uint32_t addr)
{
    BOOT(addr);
    return;
}
