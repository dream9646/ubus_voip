include ../makefile.inc

OBJS=tm.o gemflow.o td.o
all : tm.a

test_program:

ifeq ($(REBUILD_LIBRARY), y)
tm.a: $(OBJS) $(wildcard *.h)
	$(AR) rc tm.a $(OBJS)
else
tm.a: 
	test -f $@
endif

clean:
	-rm -f $(SRC_RELATED_OBJS)
	-rm -f  *.bak core
