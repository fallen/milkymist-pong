SDL_DEBUG?=0
MMDIR?=../../milkymist/milkymist-core
SERIALPORT?=/dev/ttyUSB0

ifeq ($(strip $(SDL_DEBUG)),1)
include ./makefile-sdl
OBJECTS=main.o vga_sdl.o sprites.o demo_1.o key.o transition.o tmu_sdl.o 
else
include $(MMDIR)/software/include.mak
#OBJECTS=crt0.o main.o vga.o sprites.o demo_1.o key.o transition.o 
OBJECTS=crt0.o isr.o brd.o time.o mem.o vga.o snd.o tmu.o pfpu.o line.o fontmb.o text.o intro.o main.o mod.o music.o sprites.o transition.o key.o intro.o demo_1.o demo_2.o
endif

DATA=badclouds.png badfactory.png
DATA_RAW=$(addsuffix .raw, $(basename $(DATA)) )
DATA_H=$(addsuffix .h, $(DATA) ) 
SEGMENTS=-j .text -j .data -j .rodata


MKRAW:=$(MMDIR)/tools/makeraw

all: boot.bin

ifeq ($(strip $(SDL_DEBUG)),1)
%.bin: %.elf
	cp $< $@

boot.elf: $(DATA_H) $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@
else
%.bin: %.elf
	$(OBJCOPY) $(SEGMENTS) -O binary $< $@
	$(MMDIR)/tools/crc32 $@

boot.elf: linker.ld $(DATA_H) $(OBJECTS) $(MMDIR)/software/baselib/baselib.a $(MMDIR)/software/mathlib/mathlib.a
	$(LD) $(LDFLAGS) -T linker.ld -N -o $@ $(OBJECTS) $(MMDIR)/software/baselib/baselib.a $(MMDIR)/software/mathlib/mathlib.a
endif

#PNG preprocessing
%.png.h: %.png
	@echo "DATA : $<"
	$(MKRAW) $<
	xxd -i $(basename $<).raw $(<).h

.PHONY: clean depend load

depend: $(DATA_H)
	makedepend -Y -- $(CFLAGS) -- *.c *.h

clean:
#	@echo "DATA_RAW = $(DATA_RAW)"
#	@echo "DATA_H = $(DATA_H)"
	rm -f $(OBJECTS) $(DATA_RAW) $(DATA_H) boot.elf boot.bin .*~ *~ Makefile.bak

load: boot.bin
	$(MMDIR)/tools/flterm --port $(SERIALPORT) --kernel boot.bin --kernel-adr 0x40000000
