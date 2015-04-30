#include <emscripten.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#include "gme.h"
#include "player_interface.h"

#define SAMPLES_PER_FRAME 2

typedef struct
{
    Music_Emu *emu;
    uint8_t *dataBuffer;
    int dataBufferSize;
    int voiceCount;
    int sampleRate;
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

    return 1;
}

int crPlayerLoadFile(void *context, const char *filename, unsigned char *data,
    int size)
{
    gmeContext *gme = (gmeContext*)context;

    gme->dataBufferSize = size;
    gme->dataBuffer = (uint8_t*)malloc(gme->dataBufferSize);
    if (!gme->dataBuffer)
        return 0;
    memcpy(gme->dataBuffer, data, gme->dataBufferSize);

    return 1;
}

int crPlayerSetTrack(void *context, int track)
{
    gmeContext *gme = (gmeContext*)context;
    gme_err_t status = NULL;

    /* initialize the engine */
    status = gme_open_data(gme->dataBuffer, gme->dataBufferSize,
        &gme->emu, gme->sampleRate);
    if (status)
        return 0;

    /* set the track */
    status = gme_start_track(gme->emu, track);
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
