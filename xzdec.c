#include "xz.h"

int xz_decompress(unsigned char *encoded, int encoded_size,
    unsigned char *decoded, int decoded_size)
{
    static int xz_initialized = 0;
    enum xz_ret ret;
    struct xz_dec *xz;
    struct xz_buf buf;

    if (!encoded || !decoded)
        return 0;

    if (!xz_initialized)
    {
        xz_crc32_init();
        xz_initialized = 1;
    }

    buf.in = encoded;
    buf.in_pos = 0;
    buf.in_size = encoded_size;

    buf.out = decoded;
    buf.out_pos = 0;
    buf.out_size = decoded_size;

    xz = xz_dec_init(XZ_SINGLE, 0);
    if (!xz)
    {
        return 0;
    }

    ret = xz_dec_run(xz, &buf);
    if (ret != XZ_STREAM_END)
        return 0;
    else
        return 1;
}
