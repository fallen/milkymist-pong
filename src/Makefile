MMDIR?=../../milkymist/milkymist-core
SERIALPORT?=/dev/ttyUSB0
include $(MMDIR)/software/include.mak

OBJECTS=crt0.o isr.o brd.o time.o mem.o vga.o snd.o tmu.o pfpu.o line.o fontmb.o text.o intro.o main.o mod.o
SEGMENTS=-j .text -j .data -j .rodata

all: boot.bin

%.bin: %.elf
	$(OBJCOPY) $(SEGMENTS) -O binary $< $@
	$(MMDIR)/tools/crc32 $@

boot.elf: linker.ld $(OBJECTS) $(MMDIR)/software/baselib/baselib.a $(MMDIR)/software/mathlib/mathlib.a
	$(LD) $(LDFLAGS) -T linker.ld -N -o $@ $(OBJECTS) $(MMDIR)/software/baselib/baselib.a $(MMDIR)/software/mathlib/mathlib.a

.PHONY: clean depend load

depend:
	makedepend -Y -- $(CFLAGS) -- *.c

clean:
	rm -f $(OBJECTS) boot.elf boot.bin .*~ *~ Makefile.bak

load: boot.bin
	$(MMDIR)/tools/flterm --port $(SERIALPORT) --kernel boot.bin --kernel-adr 0x40000000
