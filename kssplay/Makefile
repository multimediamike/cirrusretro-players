#kssplay Makefile

BIN = kssplay

OBJECTS = convert.o libkss.o kmz80.o device.o main.c
CC = gcc

all: $(OBJECTS)

	$(CC) *.o main.c -o $(BIN) -lm
	
convert.o: libkss/convert/*.c libkss/convert/*.h
	gcc -c libkss/convert/*.c 
		
libkss.o: libkss/*.c libkss/*.h
	gcc -c libkss/*.c
	
kmz80.o: kmz80/*.c kmz80/*.h
	gcc -c kmz80/*.c
	
device.o: device/*.c device/*.h
	gcc -c device/*.c
