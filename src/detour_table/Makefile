
CROSS ?= mips-linux-gnu-
CC=$(CROSS)gcc
STRIP=$(CROSS)strip
AR=$(CROSS)ar
RANLIB=$(CROSS)ranlib
LD=$(CROSS)ld

CFLAGS ?= -I. -Os -Wno-pointer-sign -mips32r2 -mtune=24kec -muclibc -EL -msoft-float
CFLAGS += -Wall

OBJECT=detour_table.o

default : all

all:detour_table

detour_table:$(OBJECT)
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ $^
	$(STRIP) $@

%.o:%.c
	$(CC) -c $(CFLAGS) $(INCLUDE) $<

clean:
	rm -f *.[oa] detour_table

distclean:
	rm -f *.o *.a *.plan  *.valid detour_table

