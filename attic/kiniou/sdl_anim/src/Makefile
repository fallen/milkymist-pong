SDL:=1
MMDIR?=../../milkymist/milkymist-core
SERIALPORT?=/dev/ttyUSB0

ifeq ($(strip $(SDL)),1)
include ./makefile-sdl
else
include $(MMDIR)/software/include.mak
endif

#OBJECTS=crt0.o isr.o brd.o time.o mem.o vga.o snd.o tmu.o pfpu.o line.o fontmb.o text.o intro.o main.o mod.o music.o
OBJECTS=main.o vga.o sprites.o demo_1.o key.o transition.o 
DATA=badclouds.png.h
SEGMENTS=-j .text -j .data -j .rodata


MKRAW:=$(MMDIR)/tools/makeraw

all: boot.bin

ifeq ($(strip $(SDL)),1)
%.bin: %.elf
	cp $< $@

boot.elf: $(DATA) $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@
else
%.bin: %.elf
	$(OBJCOPY) $(SEGMENTS) -O binary $< $@
	$(MMDIR)/tools/crc32 $@

boot.elf: linker.ld $(OBJECTS) $(MMDIR)/software/baselib/baselib.a $(MMDIR)/software/mathlib/mathlib.a
	$(LD) $(LDFLAGS) -T linker.ld -N -o $@ $(OBJECTS) $(MMDIR)/software/baselib/baselib.a $(MMDIR)/software/mathlib/mathlib.a
endif

#PNG preprocessing
%.png.h: %.png
	@echo "DATA : $<"
	$(MKRAW) $<
	xxd -i $(basename $<).raw $(<).h

.PHONY: clean depend load

depend: $(DATA)
	makedepend -Y -- $(CFLAGS) -- *.c *.h

clean:
	rm -f $(OBJECTS) $(DATA) boot.elf boot.bin .*~ *~ Makefile.bak

load: boot.bin
	$(MMDIR)/tools/flterm --port $(SERIALPORT) --kernel boot.bin --kernel-adr 0x40000000
