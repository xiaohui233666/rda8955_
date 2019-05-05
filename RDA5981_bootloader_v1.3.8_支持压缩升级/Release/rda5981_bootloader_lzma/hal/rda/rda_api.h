#ifndef RDA_API_H
#define RDA_API_H

#include <board.h>

#define TARGET_RDA
#define TARGET_RDA5981

#ifdef TARGET_RDA

//#define IAPDEBUG

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

//#if defined(TARGET_RDA5981)
#if defined (UNO_81C_U02) || defined (UNO_81C_U04)
#define SECTOR_SIZE     4096
#define FLASH_SIZE 0x400000

#elif defined (UNO_81A_U02) || defined (UNO_81A_U04) || defined (UNO_81AM_U02) || defined (UNO_81AM_U04)
#define SECTOR_SIZE     4096
#define FLASH_SIZE 0x100000

#else
#warning RDA unknown target, using default
#define SECTOR_SIZE     4096
#define FLASH_SIZE 0x100000

#endif

typedef enum _IAPCode {
    BoundaryError = -99,    //Commands may not span several sectors
    AlignError,             //Data must be aligned on longword (two LSBs zero)
    ProtectionError,        //Flash sector is protected
    AccessError,            //Something went wrong
    CollisionError,         //During writing something tried to flash which was written to
    LengthError,            //The length must be multiples of 4
    RuntimeError,
    EraseError,             //The flash was not erased before writing to it
    Success = 0
}IAPCode;

#define FLASH_CTL_REG_BASE 0x17fff000
#define FLASH_CTL_TX_CMD_ADDR_REG (FLASH_CTL_REG_BASE + 0x00)
#define CMD_64KB_BLOCK_ERASE (0x000000d8UL)
#define WRITE_REG32(REG, VAL)    ((*(volatile unsigned int*)(REG)) = (unsigned int)(VAL))

IAPCode erase_flash(int address, uint32_t len);

/** Erase a flash sector
 *
 * The size erased depends on the used device
 *
 * @param address address in the sector which needs to be erased
 * @param return Success if no errors were encountered, otherwise one of the error states
 */
IAPCode erase_sector(int address);

/** Program flash
 *
 * Before programming the used area needs to be erased. The erase state is checked
 * before programming, and will return an error if not erased.
 *
 * @param address starting address where the data needs to be programmed (must be longword alligned: two LSBs must be zero)
 * @param data pointer to array with the data to program
 * @param length number of bytes to program (must be a multiple of 4. must be a multiple of 8 when K64F)
 * @param return Success if no errors were encountered, otherwise one of the error states
 */
IAPCode program_flash(int address, char *data, unsigned int length);


IAPCode read_flash(int address, char *data, unsigned int length);


#endif

#endif /*RDA_API_H*/
