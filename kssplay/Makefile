#kssplay Makefile

JS = cr-kssplay.js

OBJECTS = convert.o libkss.o kmz80.o device.o main.c
CC = emcc
OPT_LEVEL = -Os
EXPORT_LIST="['_crPlayerContextSize', '_crPlayerInitialize', '_crPlayerLoadFile', '_crPlayerSetTrack', '_crPlayerGenerateStereoFrames', '_crPlayerVoicesCanBeToggled', '_crPlayerGetVoiceCount', '_crPlayerGetVoiceName', '_crPlayerSetVoiceState', '_crPlayerCleanup']"

all: $(OBJECTS)
	$(CC) -I. $(OPT_LEVEL) -o $(JS) *.o ../kssplay_interface.c --memory-init-file 0 -s EXPORTED_FUNCTIONS=$(EXPORT_LIST) -s ASSERTIONS=2
	
convert.o: libkss/convert/*.c libkss/convert/*.h
	$(CC) $(OPT_LEVEL) -c libkss/convert/*.c
		
libkss.o: libkss/*.c libkss/*.h
	$(CC) $(OPT_LEVEL) -c libkss/*.c
	
kmz80.o: kmz80/*.c kmz80/*.h
	$(CC) $(OPT_LEVEL) -c kmz80/*.c
	
device.o: device/*.c device/*.h
	$(CC) $(OPT_LEVEL) -c device/*.c
