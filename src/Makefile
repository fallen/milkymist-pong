MMDIR?=../../milkymist
SERIALPORT?=/dev/ttyUSB0
include $(MMDIR)/software/include.mak

OBJECTS=crt0.o isr.o tick.o main.o plasma.o angle.o
SEGMENTS=-j .text -j .data -j .rodata

all: boot.bin

plasma.c: noarm.h

noarm.raw: noarm.png
	$(MMDIR)/tools/makeraw noarm.png

noarm.h: noarm.raw
	xxd -i noarm.raw noarm.h

%.bin: %.elf
	$(OBJCOPY) $(SEGMENTS) -O binary $< $@
	chmod -x $@
	$(MMDIR)/tools/crc32 $@

boot.elf: linker.ld $(OBJECTS)
	$(LD) $(LDFLAGS) -T linker.ld -N -o $@ -L$(MMDIR)/software/libbase -L$(MMDIR)/software/libmath -L$(MMDIR)/software/libbase -L$(MMDIR)/software/libhal --start-group $(OBJECTS) -lbase -lmath -lhal --end-group
	chmod -x $@

.PHONY: clean depend load

depend:
	makedepend -Y -- $(CFLAGS) -- *.c

clean:
	rm -f $(OBJECTS) boot.elf boot.bin .*~ *~ Makefile.bak noarm.h noarm.raw

load: boot.bin
	$(MMDIR)/tools/flterm --port $(SERIALPORT) --kernel boot.bin --kernel-adr 0x40000000
