import os
import sys
import struct
import zlib
import time
	
"""
#define IMAGE_MAGIC         0xAEAE
struct image_header
{
    uint16_t magic;
    uint8_t  encrypt_algo;
    uint8_t  resv[1];

    uint8_t  version[VERSION_SZ];

    uint32_t crc32;
    uint32_t size;
	//uint8_t  padding[4060];
};
"""

def insert(original, new, pos):
	'''Inserts new inside original at pos.'''
	return original[:pos] + new + original[pos:]
	
def pack_image_lzma(filename, version):
    try:
	    import pylzma
    except ImportError:
        print 'could import pylzma module'
        exit(0)

    fname = os.path.splitext(filename)

    print 'firmware:', filename

    f = file(filename, 'rb')
    data = f.read()
    f.close()

    magic = 0xAEAE
    encrypt_algo = 1
    rescv = 0

    #ISOTIMEFORMAT='%Y-%m-%d %X'
    #version = time.strftime( ISOTIMEFORMAT, time.localtime() )

    crc32 = zlib.crc32(data, 0) & 0xFFFFFFFF
    crc32 ^= 0xFFFFFFFF
    size = len(data)
	
    lzma_data = pylzma.compress(data, dictionary=12, eos=0)	
    uncompressed_size = struct.pack("LL", size&0xffffffff, size>>32)
    lzma_data = insert(lzma_data, uncompressed_size, 5)
	
    f = file(fname[0] + '.bin.lzma', "wb")
    f.write(lzma_data)
    f.close()
	
    lzma_crc32 = zlib.crc32(lzma_data, 0) & 0xFFFFFFFF
    lzma_crc32 ^= 0xFFFFFFFF
    lzma_size = len(lzma_data)	
    print '    size:', lzma_size
    print '   version:', version
    print '   crc32: %08x' % lzma_crc32

    print '   uncompress size:', size
    print '   uncompress crc32: %08x' % crc32
	
    header = struct.pack("<HBB16sLLLL", magic, encrypt_algo, rescv, version, crc32, size, lzma_crc32, lzma_size)

    f = file(fname[0] + '_otapackage_lzma.bin', "wb")
    f.write(header)
    f.write(lzma_data)
    f.close()

def pack_image(filename, version):

    fname = os.path.splitext(filename)

    print 'firmware:', filename

    f = file(filename, 'rb')
    data = f.read()
    f.close()

    magic = 0xAEAE
    encrypt_algo = 0
    rescv = 0

    #ISOTIMEFORMAT='%Y-%m-%d %X'
    #version = time.strftime( ISOTIMEFORMAT, time.localtime() )

    crc32 = zlib.crc32(data, 0) & 0xFFFFFFFF
    crc32 ^= 0xFFFFFFFF
    size = len(data)
    print '    size:', size
    print '   version:', version
    print '   crc32: %08x' % crc32

    header = struct.pack("<HBB24sLL", magic, encrypt_algo, rescv, version, crc32, size)

    f = file(fname[0] + '_otapackage.bin', "wb")
    f.write(header)
    f.write(data)
    f.close()

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print sys.argv[0], "filename"
        exit(0)

    if len(sys.argv) == 3:
        pack_image(sys.argv[1], sys.argv[2])
    elif sys.argv[3] == 'lzma':
        pack_image_lzma(sys.argv[1], sys.argv[2])
    elif sys.argv[3] == 'LZMA':
        pack_image_lzma(sys.argv[1], sys.argv[2])