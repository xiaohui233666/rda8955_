#ifndef BOOTLOADER_CONFIG_H__
#define BOOTLOADER_CONFIG_H__

/* CPU type */
#define CPU_CORTEX_M

/* The boot information address of application firmware */
//#define FM_INFO_ADDR        0x00001000
//#define FM_INFO_SECTOR_SZ   4096
#define COPY_MODE
#if defined(COPY_MODE)

#ifndef OTA_LAMZ_COMPRESS_SUPPORT

#if defined (UNO_81C_U02) || defined (UNO_81C_U04)
#define COPY_MODE_FW_INFO_ADDR    0x18003000
#define COPY_MODE_FW_START_ADDR   0x18004000
#define COPY_MODE_UPGRADE_ADDR    0x181F8000
#define COPY_MODE_FW_MAX_SIZE     0x001F4000
#elif defined (UNO_81A_U02) || defined (UNO_81A_U04) || defined (UNO_81AM_U02) || defined (UNO_81AM_U04)
#define COPY_MODE_FW_INFO_ADDR    0x18003000
#define COPY_MODE_FW_START_ADDR   0x18004000
#define COPY_MODE_UPGRADE_ADDR    0x18081000
#define COPY_MODE_FW_MAX_SIZE     0x0007D000
#else
#error "unsupport chip version"
#endif

#else /*OTA_LAMZ_COMPRESS_SUPPORT*/

#if defined (UNO_81C_U02) || defined (UNO_81C_U04)
#define COPY_MODE_FW_INFO_ADDR    0x18005000
#define COPY_MODE_FW_START_ADDR   0x18006000
#define COPY_MODE_UPGRADE_ADDR    0x181F8000
#define COPY_MODE_FW_MAX_SIZE     0x001F2000
#elif defined (UNO_81A_U02) || defined (UNO_81A_U04) || defined (UNO_81AM_U02) || defined (UNO_81AM_U04)
#define COPY_MODE_FW_INFO_ADDR    0x18005000
#define COPY_MODE_FW_START_ADDR   0x18006000
#define COPY_MODE_UPGRADE_ADDR    0x1809C000
#define COPY_MODE_FW_MAX_SIZE     0x00096000
#else
#error "unsupport chip version"
#endif

#endif/*OTA_LAMZ_COMPRESS_SUPPORT*/

#endif

#endif

