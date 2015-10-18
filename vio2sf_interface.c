#include <emscripten.h>

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "player_interface.h"
#include "xzdec.h"
#include "vio2sf.h"

#define SAMPLES_PER_FRAME 2
#define VIO2SF_VOICE_COUNT 1

#define PSFARCHIVE_SIG "PSF Song Archive"
#define PSFARCHIVE_SIG_LEN 16
#define TWOSF_VERSION 0x24
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
    int loaded2sf;
} vio2sfContext;

typedef struct
{
    int64_t firstOffset;
    int64_t currentOffset;
    int64_t lastOffset;
} fileHandle;

/* global is necessary since the file load callback doesn't take a user parm */
static vio2sfContext *g_vio2sf = NULL;

/* The 2sf library calls back to this function to retrieve a file */
int xsf_get_lib(char *pfilename, void **ppbuffer, unsigned int *plength)
{
    int i;
    int found = 0;
    unsigned int offset = 0;
    unsigned int size = 0;
    unsigned char *data = g_vio2sf->dataBuffer;
    unsigned char *dataCopy = NULL;
    fileEntry *entries = g_vio2sf->entries;

    for (i = 0; i < g_vio2sf->entryCount; i++)
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
    return sizeof(vio2sfContext);
}

/* Initialize the context data structure and save the requested sample rate */
int crPlayerInitialize(void *context, int sampleRate)
{
    vio2sfContext *vio2sf = (vio2sfContext*)context;

    vio2sf->dataBuffer = NULL;
    vio2sf->dataBufferSize = 0;
    vio2sf->sampleRate = sampleRate;
    vio2sf->entryCount = 0;
    vio2sf->entries = NULL;
    vio2sf->loaded2sf = 0;

    /* keep a global copy of the context in this file's namespace since
     * the vio2sf file load callback function doesn't offer a user parm */
    g_vio2sf = vio2sf;

    return 1;
}

/* copy the entire file to the player's memory space and parse the
 * out the boundaries */
int crPlayerLoadFile(void *context, const char *filename, unsigned char *data,
    int size)
{
    vio2sfContext *vio2sf = (vio2sfContext*)context;
    int i;
    unsigned char *fileTable;

    /* save the input */
    vio2sf->dataBuffer = data;
    vio2sf->dataBufferSize = size;

    /* minimally validate the file */
    if (strncmp((char *)vio2sf->dataBuffer, PSFARCHIVE_SIG, PSFARCHIVE_SIG_LEN) != 0)
        return 0;

    /* load the virtual file system within the psfarchive file */
    vio2sf->entryCount = GET_BE32(&vio2sf->dataBuffer[16]);
    vio2sf->entries = (fileEntry*)malloc(vio2sf->entryCount * sizeof(fileEntry));
    if (!vio2sf->entries)
        return 0;
    fileTable = vio2sf->dataBuffer + 20;
    for (i = 0; i < vio2sf->entryCount; i++)
    {
        vio2sf->entries[i].offset = GET_BE32(fileTable);
        fileTable += 4;
        vio2sf->entries[i].size = GET_BE32(fileTable);
        fileTable += 4;
        vio2sf->entries[i].name = (char *)(vio2sf->dataBuffer + GET_BE32(fileTable));
        fileTable += 4;
    }

    return 1;
}

/* initialize the player based on the specific track */
int crPlayerSetTrack(void *context, int track)
{
    vio2sfContext *vio2sf = (vio2sfContext*)context;
    unsigned int offset = 0;
    unsigned int size = 0;
    unsigned char *dataCopy = NULL;

    /* check that the track number is valid and fetch the root name */
    if (track >= vio2sf->entryCount)
        return 0;
    /* if a file is already loaded and playing, shut down the engine first */
    if (vio2sf->loaded2sf)
    {
        xsf_term();
        vio2sf->loaded2sf = 0;
    }
    offset = vio2sf->entries[track].offset;
    size = vio2sf->entries[track].size;
    dataCopy = (unsigned char*)malloc(size);
    if (!dataCopy)
        return 0;
    memcpy(dataCopy, &vio2sf->dataBuffer[offset], size);
    if (!xsf_start(dataCopy, size, vio2sf->sampleRate))
        return 0;  /* initialization failed */
    else
    {
        vio2sf->loaded2sf = 1;
        return 2;  /* initialized; indicate stereo */
    }
}

int crPlayerGenerateStereoFrames(void *context, int16_t *samples, int frameCount)
{
    xsf_gen(samples, frameCount);

    return 1;
}

int crPlayerVoicesCanBeToggled(void *context)
{
    /* no good reason to allow toggling of NDS channels */
    return 0;
}

int crPlayerGetVoiceCount(void *context)
{
    return VIO2SF_VOICE_COUNT;
}

const char *crPlayerGetVoiceName(void *context, int voice)
{
    return "VIO2SF Playback Engine";
}

void crPlayerSetVoiceState(void *context, int voice, int enabled)
{
    /* no-op */
}

void crPlayerCleanup(void *context)
{
    vio2sfContext *vio2sf = (vio2sfContext*)context;

    if (vio2sf->loaded2sf)
    {
        xsf_term();
        vio2sf->loaded2sf = 0;
    }

    if (vio2sf->dataBuffer)
        free(vio2sf->dataBuffer);
    vio2sf->dataBuffer = NULL;

    if (vio2sf->entries)
        free(vio2sf->entries);
    vio2sf->entries = NULL;
}

