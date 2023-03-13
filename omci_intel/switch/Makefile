include ../makefile.inc

OBJS=switch.o switch_null.o

all : switch.a

test_program:

ifeq ($(REBUILD_LIBRARY), y)
switch.a: $(OBJS) $(wildcard *.h)
	$(AR) rc switch.a $(OBJS)
else
switch.a: 
	test -f $@
endif

clean:
	-rm -f $(SRC_RELATED_OBJS)
	-rm -f  *.bak core
