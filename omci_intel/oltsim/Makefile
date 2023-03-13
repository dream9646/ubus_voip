include ../makefile.inc

OBJS=oltsim.o omcilog.o logplay.o omcistr.o util.o

#SOURCES = $(wildcard *.c)

all :	oltsim

ifeq ($(REBUILD_LIBRARY), y)
oltsim: $(OBJS) $(wildcard *.h)
	$(CC) $(LOCAL_CFLAGS) $(CFLAGS) -o oltsim $(OBJS) -lpthread
else
oltsim: 
	test -f $@
endif

test_program:

clean:
	-rm -f $(SRC_RELATED_OBJS)
	-rm -f  *.bak core oltsim
