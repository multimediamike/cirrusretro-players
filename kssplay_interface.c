#include <emscripten.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#include "libkss/kssplay.h"
#include "player_interface.h"
#include "xzdec.h"

#define BITS_PER_SAMPLE 16

typedef struct
{
    KSS *kss;
    KSSPLAY *kssplay;
    uint8_t *dataBuffer;
    int dataBufferSize;
    int sampleRate;
} kssContext;

int crPlayerContextSize()
{
    return sizeof(kssContext);
}

int crPlayerInitialize(void *context, int sampleRate)
{
    kssContext *kss = (kssContext*)context;

    kss->kss = NULL;
    kss->kssplay = NULL;
    kss->dataBuffer = NULL;
    kss->dataBufferSize = 0;
    kss->sampleRate = sampleRate;

    return 1;
}

int crPlayerLoadFile(void *context, const char *filename, unsigned char *data,
    int size)
{
    kssContext *kss = (kssContext*)context;

    /* check if the data is compressed with XZ */
#if 0
    if ((data[0] == 0xFD) &&
        (data[1] == '7') &&
        (data[2] == 'z') &&
        (data[3] == 'X') &&
        (data[4] == 'Z'))
    {
        /* compressed with XZ-embedded; decompress */
        if (!xz_decompress(data, size, &kss->dataBuffer, &kss->dataBufferSize))
        {
            free(kss->dataBuffer);
            return 0;
        }
    }
    else
#endif
    {
        /* not compressed; move to internal buffer */
        kss->dataBufferSize = size;
        kss->dataBuffer = (uint8_t*)malloc(kss->dataBufferSize);
        if (!kss->dataBuffer)
            return 0;
        memcpy(kss->dataBuffer, data, kss->dataBufferSize);
    }

    return 1;
}

int crPlayerSetTrack(void *context, int track)
{
    kssContext *kss = (kssContext*)context;

    /* load the data if this is the first time through */
    if (!kss->kss)
        kss->kss = KSS_bin2kss(kss->dataBuffer, kss->dataBufferSize, NULL);

    /* delete any existing player */
    if (kss->kssplay)
        KSSPLAY_delete(kss->kssplay);

    /* initialize the player */
    kss->kssplay = KSSPLAY_new(kss->sampleRate, 2, BITS_PER_SAMPLE);
    KSSPLAY_set_data(kss->kssplay, kss->kss);
    KSSPLAY_reset(kss->kssplay, track, 0);

    /* mono data */
    return 1;
}

int crPlayerGenerateStereoFrames(void *context, int16_t *samples, int frameCount)
{
    kssContext *kss = (kssContext*)context;

    KSSPLAY_calc(kss->kssplay, samples, frameCount);

    return 1;
}

int crPlayerVoicesCanBeToggled(void *context)
{
    return 0;
}

int crPlayerGetVoiceCount(void *context)
{
    return 1;
}

const char *crPlayerGetVoiceName(void *context, int voice)
{
    return "KSSplay engine";
}

void crPlayerSetVoiceState(void *context, int voice, int enabled)
{
}

void crPlayerCleanup(void *context)
{
    kssContext *kss = (kssContext*)context;

    KSSPLAY_delete(kss->kssplay);
}

