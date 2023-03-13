include ../../makefile.inc

OBJS =	complete.o \
	editline.o \
	sysunix.o

CFLAGS+= -I.

all : editline.a 

editline.a: $(OBJS) $(wildcard *.h)
	$(AR) rc editline.a $(OBJS)

clean:
	-rm -f $(OBJS) editline.a
	-rm -f  *.bak core
