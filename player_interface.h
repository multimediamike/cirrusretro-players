#ifndef PLAYER_INTERFACE
#define PLAYER_INTERFACE

#include <inttypes.h>

#ifdef __cplusplus
extern "C"
{
#endif
    int crPlayerContextSize();
    int crPlayerInitialize(void *context, int sampleRate);
    int crPlayerLoadFile(void *context, const char *filename, unsigned char *data, int size);
    int crPlayerSetTrack(void *context, int track);
    int crPlayerGenerateStereoFrames(void *context, int16_t *samples, int frameCount);
    int crPlayerVoicesCanBeToggled(void *context);
    int crPlayerGetVoiceCount(void *context);
    const char *crPlayerGetVoiceName(void *context, int voice);
    void crPlayerSetVoiceState(void *context, int voice, int enabled);
    void crPlayerCleanup(void *context);
#ifdef __cplusplus
};
#endif

#endif  // PLAYER_INTERFACE
