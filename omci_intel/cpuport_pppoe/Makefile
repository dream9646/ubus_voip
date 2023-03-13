include ../makefile.inc

#OBJS=er_group_hw.o $(addsuffix .o,$(basename $(wildcard er_group_hw_[0-9]*.c)))
OBJS =	cpuport_pppoe.o \

all : cpuport_pppoe.a

test_program:

ifeq ($(REBUILD_LIBRARY), y)
cpuport_pppoe.a: $(OBJS) $(wildcard *.h)
	$(AR) rc cpuport_pppoe.a $(OBJS)
else
cpuport_pppoe.a: 
	test -f $@
endif

clean:
	-rm -f $(SRC_RELATED_OBJS)
	-rm -f  *.bak core
