include ../makefile.inc

OBJS = cpuport_hw_prx126.o

all : cpuport_hw_prx126.a

test_program:

ifeq ($(REBUILD_LIBRARY), y)
cpuport_hw_prx126.a: $(OBJS) $(wildcard *.h) ../makefile.omci.inc
	$(AR) rc cpuport_hw_prx126.a $(OBJS)
else
cpuport_hw.a: 
	test -f $@
endif

clean:
	-rm -f $(SRC_RELATED_OBJS)
	-rm -f *.bak core
