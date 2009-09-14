
SOURCES=main.c pcm.c mod.c file.c
TARGET=main

NAMES=$(basename $(SOURCES))
OBJECTS=$(addsuffix .o, $(NAMES))
HEADERS=$(addsuffix .h, $(filter-out $(TARGET),$(NAMES)))

CC=gcc

CFLAGS=-Wall -DUSE_MMAP -DLITTLE_ENDIAN -D_DEBUG=0 -ggdb
LDFLAGS=

DEPFILE=depends.source

$(TARGET): $(OBJECTS) $(HEADERS) $(DEPFILE)
	gcc $(LDFLAGS) -o $@ $(OBJECTS)

$(DEPFILE): $(SOURCES) $(CSOURCES)
	$(shell touch $(DEPFILE))
	$(CC) $(CFLAGS) -M $(SOURCES) > $(DEPFILE)

.c.o:
	gcc -c $(CFLAGS) $< -o $@



clean:	$(DEPFILE)
	rm -f $(OBJECTS) $(TARGET) $(DEPFILE)


dep:	$(DEPFILE)

run:
	./main ../data/8.mod | sox -L -r 48000 -t raw -b 2 -s -2 -c 2 - -t wav out.wav 
	ffplay out.wav

play:
	./main ../data/8.mod
	sox -L -r 48000 -t raw -b 2 -s -2 -c 2 output.raw -t wav out.wav 
	ffplay out.wav


-include $(DEPFILE)
