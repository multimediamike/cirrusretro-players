#ifndef _XZDEC_H_
#define _XZDEC_H_

extern "C"
{
    /* Decompress a buffer that is compressed with the XZ algorithm.
     * Note that this leverages the XZ-embedded library which can only
     * handle CRC32 integrity checks, e.g., data that was encoded with
     * 'xz --check=crc32 ...' which is NOT the default for xz.
     *
     * Size of decompressed data needs to be known in advance since this
     * function expects that the 'decoded' buffer will point to enough
     * space to hold all the data.
     *
     * Returns non-zero on success, 0 on failure.
     */
    int xz_decompress(unsigned char *encoded, int encoded_size,
        unsigned char *decoded, int decoded_size);
};

#endif  // _XZDEC_H_
