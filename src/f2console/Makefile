
CROSS ?= mips-linux-gnu-
CC=$(CROSS)gcc
STRIP=$(CROSS)strip
AR=$(CROSS)ar
RANLIB=$(CROSS)ranlib
LD=$(CROSS)ld

CFLAGS ?= -I. -Os -Wno-pointer-sign -mips32r2 -mtune=24kec -muclibc -EL -msoft-float
CFLAGS += -Wall

OBJECT=f2console.o

default : all

all:f2console

f2console:$(OBJECT)
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ $^
	$(STRIP) $@

%.o:%.c
	$(CC) -c $(CFLAGS) $(INCLUDE) $<

clean:
	rm -f *.[oa] f2console

distclean:
	rm -f *.o *.a *.plan  *.valid f2console

