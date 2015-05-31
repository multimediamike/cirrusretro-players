#ifndef _XZDEC_H_
#define _XZDEC_H_

#ifdef __cplusplus
extern "C"
{
#endif
    /* Decompress a buffer that is compressed with the XZ algorithm.
     * Note that this leverages the XZ-embedded library which can only
     * handle CRC32 integrity checks, e.g., data that was encoded with
     * 'xz --check=crc32 ...' which is NOT the default for xz.
     *
     * Input parameters:
     *  encoded: pointer to compressed bytestream
     *  encoded_size: length of the encoded bytestream
     *
     * Output parameters:
     *  decoded: pointer to decompressed bytestream
     *  decoded_size: pointer to int indicating length of decoded bytestream
     *
     * Returns non-zero on success, 0 on failure.
     */
    int xz_decompress(unsigned char *encoded, int encoded_size,
        unsigned char **decoded, int *decoded_size);
#ifdef __cplusplus
};
#endif
#endif  // _XZDEC_H_
