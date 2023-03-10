CROSS ?= mips-linux-gnu-
CC = $(CROSS)gcc
LD = $(CROSS)ld
AR = $(CROSS)ar
STRIP = $(CROSS)strip


CFLAGS  ?= -I. -Os -Wno-pointer-sign -mips32r2 -mtune=24kec -muclibc -EL -msoft-float

default : all

all: memdbg


%.o:%.c
	$(CC) -c memdbg.c $(CFLAGS) -L. -lpthread

memdbg: memdbg.o
	$(AR) -csr libmemdbg.a memdbg.o


clean:
	rm -f *.[oa] memdbg
