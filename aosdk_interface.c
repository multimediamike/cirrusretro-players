#include <emscripten.h>

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "player_interface.h"
#include "xzdec.h"
#include "ao.h"
#include "eng_protos.h"

#define SAMPLES_PER_FRAME 2
#define VOICE_COUNT 1

#define PSFARCHIVE_SIG "PSF Song Archive"
#define PSFARCHIVE_SIG_LEN 16
#define PSF_XZ_SIG "psf"
#define PSF_XZ_SIG_LEN 3
#define GET_LE32(x) ((((uint8_t*)(x))[3] << 24) | \
                     (((uint8_t*)(x))[2] << 16) | \
                     (((uint8_t*)(x))[1] << 8) | \
                      ((uint8_t*)(x))[0])
#define GET_BE32(x) ((((uint8_t*)(x))[0] << 24) | \
                     (((uint8_t*)(x))[1] << 16) | \
                     (((uint8_t*)(x))[2] << 8) | \
                      ((uint8_t*)(x))[3])

typedef struct
{
    int offset;
    int size;
    char *name;
} fileEntry;

typedef struct
{
    uint8_t *dataBuffer;
    int dataBufferSize;
    int sampleRate;
    int entryCount;
    fileEntry *entries;
    int isPsfArchive;
} aosdkContext;

typedef struct
{
    int64_t firstOffset;
    int64_t currentOffset;
    int64_t lastOffset;
} fileHandle;

/* global is necessary since the file load callback doesn't take a user parm */
static aosdkContext *g_aosdk = NULL;

/* The AOSDK library calls back to this function to retrieve a file */
int ao_get_lib(char *pfilename, uint8 **ppbuffer, uint64 *plength)
{
    int i;
    int found = 0;
    unsigned int offset = 0;
    unsigned int size = 0;
    unsigned char *data = g_aosdk->dataBuffer;
    unsigned char *dataCopy = NULL;
    fileEntry *entries = g_aosdk->entries;

    for (i = 0; i < g_aosdk->entryCount; i++)
    {
        /* should be a binary search, ideally */
        if (strcasecmp(entries[i].name, pfilename) == 0)
        {
            found = 1;
            offset = entries[i].offset;
            size = entries[i].size;
            break;
        }
    }

    if (found)
    {
        dataCopy = (unsigned char*)malloc(size);
        if (!dataCopy)
            return 0;
        memcpy(dataCopy, &data[offset], size);
    }

    *ppbuffer = dataCopy;
    *plength = size;

    return found;
}

int crPlayerContextSize()
{
    return sizeof(aosdkContext);
}

/* Initialize the context data structure and save the requested sample rate */
int crPlayerInitialize(void *context, int sampleRate)
{
    aosdkContext *aosdk = (aosdkContext*)context;

    aosdk->dataBuffer = NULL;
    aosdk->dataBufferSize = 0;
    aosdk->sampleRate = sampleRate;
    aosdk->entryCount = 0;
    aosdk->entries = NULL;
    aosdk->isPsfArchive = 0;

    /* keep a global copy of the context in this file's namespace since
     * the AOSDK file load callback function doesn't offer a user parm */
    g_aosdk = aosdk;

    return 1;
}

/* copy the entire file to the player's memory space and parse the
 * out the boundaries */
int crPlayerLoadFile(void *context, const char *filename, unsigned char *data,
    int size)
{
    aosdkContext *aosdk = (aosdkContext*)context;
    int i;
    unsigned char *fileTable;

    /* save the input */
    aosdk->dataBuffer = data;
    aosdk->dataBufferSize = size;

    /* minimally validate the file */
    if (strncmp((char *)aosdk->dataBuffer, PSFARCHIVE_SIG, PSFARCHIVE_SIG_LEN) == 0)
        aosdk->isPsfArchive = 1;
    else if (strncmp((char *)aosdk->dataBuffer, PSF_XZ_SIG, PSF_XZ_SIG_LEN) == 0)
    {
#if defined(AOSDK_SSF)
        if (aosdk->dataBuffer[3] != 0x11)
#elif defined(AOSDK_DSF)
        if (aosdk->dataBuffer[3] != 0x12)
#else
    #error No valid PSF type specified during compilation
#endif
            return 0;

        aosdk->isPsfArchive = 0;
        return 1;
    }
    else
        return 0;

    /* load the virtual file system within the psfarchive file */
    aosdk->entryCount = GET_BE32(&aosdk->dataBuffer[16]);
    aosdk->entries = (fileEntry*)malloc(aosdk->entryCount * sizeof(fileEntry));
    if (!aosdk->entries)
        return 0;
    fileTable = aosdk->dataBuffer + 20;
    for (i = 0; i < aosdk->entryCount; i++)
    {
        aosdk->entries[i].offset = GET_BE32(fileTable);
        fileTable += 4;
        aosdk->entries[i].size = GET_BE32(fileTable);
        fileTable += 4;
        aosdk->entries[i].name = (char *)(aosdk->dataBuffer + GET_BE32(fileTable));
        fileTable += 4;
    }

    return 1;
}

/* initialize the player based on the specific track */
int crPlayerSetTrack(void *context, int track)
{
    aosdkContext *aosdk = (aosdkContext*)context;
    unsigned int offset = 0;
    unsigned int size = 0;
    unsigned char *dataCopy = NULL;

    if (aosdk->isPsfArchive)
    {
        /* check that the track number is valid and fetch the root name */
        if (track >= aosdk->entryCount)
            return 0;
        offset = aosdk->entries[track].offset;
        size = aosdk->entries[track].size;
        dataCopy = (unsigned char*)malloc(size);
        if (!dataCopy)
            return 0;
        memcpy(dataCopy, &aosdk->dataBuffer[offset], size);
    }
    else
    {
        size = aosdk->dataBufferSize;
        dataCopy = (unsigned char*)malloc(size);
        if (!dataCopy)
            return 0;
        memcpy(dataCopy, aosdk->dataBuffer, size);
    }

#if defined(AOSDK_DSF)
    if (!dsf_start(dataCopy, size))
#elif defined(AOSDK_SSF)
    if (!ssf_start(dataCopy, size))
#else
    #error No valid PSF type specified during compilation
#endif
        return 0;  /* initialization failed */
    else
        return 2;  /* initialized; indicate stereo */
}

int crPlayerGenerateStereoFrames(void *context, int16_t *samples, int frameCount)
{
#if defined(AOSDK_DSF)
    dsf_gen(samples, frameCount);
#elif defined(AOSDK_SSF)
    ssf_gen(samples, frameCount);
#else
    #error No valid PSF type specified during compilation
#endif

    return 1;
}

int crPlayerVoicesCanBeToggled(void *context)
{
    /* no good reason to allow toggling of NDS channels */
    return 0;
}

int crPlayerGetVoiceCount(void *context)
{
    return VOICE_COUNT;
}

const char *crPlayerGetVoiceName(void *context, int voice)
{
    return "AOSDK Playback Engine";
}

void crPlayerSetVoiceState(void *context, int voice, int enabled)
{
    /* no-op */
}

void crPlayerCleanup(void *context)
{
    aosdkContext *aosdk = (aosdkContext*)context;

    if (aosdk->dataBuffer)
        free(aosdk->dataBuffer);
    aosdk->dataBuffer = NULL;

    if (aosdk->entries)
        free(aosdk->entries);
    aosdk->entries = NULL;
}

