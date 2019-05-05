#ifndef CRC32_H_INCLUDED
#define CRC32_H_INCLUDED

#include <stdint.h>

extern uint32_t crc32_halfbyte(const void* data,
                               uint32_t length,
                               uint32_t previousCrc32);


#endif // CRC32_H_INCLUDED
