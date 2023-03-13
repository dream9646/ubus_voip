CROSS=rsdk-linux-
CC=$(CROSS)gcc
LD=$(CROSS)ld
AR=$(CROSS)ar
RANLIB=$(CROSS)ranlib
STRIP=$(CROSS)strip

CFLAGS+= -I. -I../libweb 
CFLAGS+= -Wall -g
#CFLAGS+= -static

#LIBS=-lgnuregex	# required for freebsd
LIBS=

OBJS=cpuport.o cpuport_hw_fvt2510.o

all :	 cpuport

cpuport: $(OBJS)
	$(CC) $(CFLAGS) -o cpuport $(OBJS) $(LIBS)
	
clean:
	-rm -f *.a *.o *.bak core cpuport
