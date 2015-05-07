#ifndef PLAYER_INTERFACE
#define PLAYER_INTERFACE

extern "C"
{
    int crPlayerContextSize();
    int crPlayerInitialize(void *context, int sampleRate);
    int crPlayerLoadFile(void *context, const char *filename, unsigned char *data, int size, int decompressedSize);
    int crPlayerSetTrack(void *context, int track);
    int crPlayerGenerateStereoFrames(void *context, int16_t *samples, int frameCount);
    int crPlayerVoicesCanBeToggled(void *context);
    int crPlayerGetVoiceCount(void *context);
    const char *crPlayerGetVoiceName(void *context, int voice);
    void crPlayerSetVoiceState(void *context, int voice, int enabled);
    void crPlayerCleanup(void *context);
};

#endif  // PLAYER_INTERFACE
