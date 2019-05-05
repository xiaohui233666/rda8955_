#ifndef BOOTLOADER_CONFIG_H__
#define BOOTLOADER_CONFIG_H__

/* CPU type */
#define CPU_CORTEX_M

/* The boot information address of application firmware */
//#define FM_INFO_ADDR        0x00001000
//#define FM_INFO_SECTOR_SZ   4096
#define COPY_MODE
#if defined(COPY_MODE)

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
#warning RDA unknown target, using default
#define COPY_MODE_FW_INFO_ADDR    0x18003000
#define COPY_MODE_FW_START_ADDR   0x18004000
#define COPY_MODE_UPGRADE_ADDR    0x18204000
#define COPY_MODE_FW_MAX_SIZE     0x0007D000

#endif

#endif

#endif

