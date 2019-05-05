#include "bootloader.h"

const struct ping_pong_layout _ping_pong_layout_table = {
    {
        IMAGE_TYPE_FLASH,
        {
            0x18010000,                    /* ping image at 0x18010000  */
        }
    },
    {
        IMAGE_TYPE_FLASH,
        {
            0x18080000,                    /* pong image at 0x18080000  */
        }
    }
};

const struct copy_layout copy_layout_table = {
    COPY_MODE_FW_INFO_ADDR,                    /*execute image information at 0x18010000*/
    COPY_MODE_FW_START_ADDR,                    /*execute image store address 0x18007000, it's also program start address*/
    COPY_MODE_FW_MAX_SIZE,                       /*firmware size at most 496KB*/
    {
        IMAGE_TYPE_FLASH,
        {
            COPY_MODE_UPGRADE_ADDR,               /*upgrade image store address, upgrade image program start address is 0x18011000*/
        }
    }
};

const struct triple_area_layout _triple_area_layout_table = {
    //FM_INFO_ADDR,                   /* firmware information at 0x10000 */
    0x00001000,
    0x40000,                        /* firmware begin at 0x40000 */

    {                               /* upgrade image at 0x80000  */
        IMAGE_TYPE_FLASH,
        {
            //0x80000,
            0xa0000,
        }
    },
    {
        IMAGE_TYPE_FLASH,           /* safe image at 0xc00000    */
        {
            //0xc0000,
            0xf0000,
        }
    }
};
