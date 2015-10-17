#include <emscripten.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#include "gme.h"
#include "player_interface.h"
#include "xzdec.h"

#define SAMPLES_PER_FRAME 2

typedef struct
{
    int offset;
    int size;
} fileEntry;

typedef struct
{
    Music_Emu *emu;
    uint8_t *dataBuffer;
    int dataBufferSize;
    int voiceCount;
    int sampleRate;
    int trackCount;
    int isGameMusicArchive;
    fileEntry *entries;
} gmeContext;

int crPlayerContextSize()
{
    return sizeof(gmeContext);
}

int crPlayerInitialize(void *context, int sampleRate)
{
    gmeContext *gme = (gmeContext*)context;

    gme->emu = NULL;
    gme->dataBuffer = NULL;
    gme->dataBufferSize = 0;
    gme->voiceCount = 0;
    gme->sampleRate = sampleRate;
    gme->trackCount = 0;
    gme->isGameMusicArchive = 0;
    gme->entries = NULL;

    return 1;
}

int crPlayerLoadFile(void *context, const char *filename, unsigned char *data,
    int size)
{
    gmeContext *gme = (gmeContext*)context;
    int i;

    /* check if the data is compressed with XZ */
    if ((data[0] == 0xFD) &&
        (data[1] == '7') &&
        (data[2] == 'z') &&
        (data[3] == 'X') &&
        (data[4] == 'Z'))
    {
        /* compressed with XZ-embedded; decompress */
        if (!xz_decompress(data, size, &gme->dataBuffer, &gme->dataBufferSize))
        {
            free(gme->dataBuffer);
            return 0;
        }
    }
    else
    {
        /* not compressed; move to internal buffer */
        gme->dataBufferSize = size;
        gme->dataBuffer = (uint8_t*)malloc(gme->dataBufferSize);
        if (!gme->dataBuffer)
            return 0;
        memcpy(gme->dataBuffer, data, gme->dataBufferSize);
    }

    /* check if this is a .gamemusic file */
    if (strncmp((char *)gme->dataBuffer, "Game Music Files", 16) == 0)
    {
        /* it's a .gamemusic file; parse out a table of offsets and sizes */
        gme->isGameMusicArchive = 1;
        gme->trackCount =
            (gme->dataBuffer[16] << 24) |
            (gme->dataBuffer[17] << 16) |
            (gme->dataBuffer[18] <<  8) |
            (gme->dataBuffer[19] <<  0);
        gme->entries = (fileEntry*)malloc(gme->trackCount * sizeof(fileEntry));
        if (!gme->entries)
        {
            return 0;
        }

        /* gather up the offsets */
        for (i = 0; i < gme->trackCount; i++)
        {
            gme->entries[i].offset =
                (gme->dataBuffer[20 + i * 4 + 0] << 24) |
                (gme->dataBuffer[20 + i * 4 + 1] << 16) |
                (gme->dataBuffer[20 + i * 4 + 2] <<  8) |
                (gme->dataBuffer[20 + i * 4 + 3] <<  0);
        }

        /* derive the sizes based on the offsets */
        for (i = 0; i < gme->trackCount - 1; i++)
        {
            gme->entries[i].size =
                gme->entries[i + 1].offset - gme->entries[i].offset;
        }
        /* last size takes data size into account */
        gme->entries[gme->trackCount - 1].size =
            gme->dataBufferSize - gme->entries[gme->trackCount - 1].offset;
    }

    return 1;
}

int crPlayerSetTrack(void *context, int track)
{
    gmeContext *gme = (gmeContext*)context;
    gme_err_t status = NULL;
    int trueTrack;
    fileEntry *entry;

    /* initialize the engine */
    if (gme->isGameMusicArchive)
    {
        /* when it's time to start a track, it's always going to be
         * track 0 (first and only) of a single-song file */
        trueTrack = 0;

        /* if a file is already being played, free it first */
        if (gme->emu)
        {
            gme_delete(gme->emu);
            gme->emu = NULL;
        }

        entry = &gme->entries[track];
        status = gme_open_data(gme->dataBuffer + entry->offset, entry->size,
            &gme->emu, gme->sampleRate);
        if (status)
            return 0;
    }
    else
    {
        trueTrack = track;
        /* if the player isn't already open, do the initialization */
        if (!gme->emu)
        {
            status = gme_open_data(gme->dataBuffer, gme->dataBufferSize,
                &gme->emu, gme->sampleRate);
            if (status)
                return 0;
        }
    }

    /* set the track */
    status = gme_start_track(gme->emu, trueTrack);
    if (!status)
    {
        gme->voiceCount = gme_voice_count(gme->emu);
        return 2; /* stereo */
    }
    else
    {
        gme_delete(gme->emu);
        gme->emu = NULL;
        return 0;
    }
}

int crPlayerGenerateStereoFrames(void *context, int16_t *samples, int frameCount)
{
    gmeContext *gme = (gmeContext*)context;
    gme_err_t status;

    status = gme_play(gme->emu, frameCount * SAMPLES_PER_FRAME, samples);

    return (status == NULL);
}

int crPlayerVoicesCanBeToggled(void *context)
{
    return 1;
}

int crPlayerGetVoiceCount(void *context)
{
    gmeContext *gme = (gmeContext*)context;

    return gme->voiceCount;
}

const char *crPlayerGetVoiceName(void *context, int voice)
{
    gmeContext *gme = (gmeContext*)context;

    return gme_voice_name(gme->emu, voice);
}

void crPlayerSetVoiceState(void *context, int voice, int enabled)
{
    gmeContext *gme = (gmeContext*)context;

    if (voice < gme->voiceCount)
        gme_mute_voice(gme->emu, voice, !enabled);
}

void crPlayerCleanup(void *context)
{
    gmeContext *gme = (gmeContext*)context;

    if (gme->dataBuffer)
        free(gme->dataBuffer);
    gme->dataBuffer = NULL;

    if (gme->entries)
        free(gme->entries);
    gme->entries = NULL;

    if (gme->emu)
        gme_delete(gme->emu);
    gme->emu = NULL;
}

int main()
{
    EM_ASM(
        if (typeof(crPlayerIsReady) == "function")
            crPlayerIsReady();
    );
    return 0;
}
