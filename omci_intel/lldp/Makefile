include ../makefile.inc

OBJS =	lldp_core.o lldp_send.o lldp_recv.o lldp_sm.o \

all : lldp.a

test_program:

ifeq ($(REBUILD_LIBRARY), y)
lldp.a: $(OBJS) $(wildcard *.h)
	$(AR) rc lldp.a $(OBJS)
else
lldp.a: 
	test -f $@
endif

clean:
	-rm -f $(SRC_RELATED_OBJS)
	-rm -f  *.bak core
