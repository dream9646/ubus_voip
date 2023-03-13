include ../makefile.inc

OBJS = voip.o

all : voip.a

test_program:

ifeq ($(REBUILD_LIBRARY), y)
voip.a: $(OBJS) $(wildcard *.h) ../makefile.omci.inc
	$(AR) rc voip.a $(OBJS)
else
voip.a: 
	test -f $@
endif

clean:
	-rm -f $(SRC_RELATED_OBJS)
	-rm -f *.bak core

