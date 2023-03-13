include ../makefile.inc

OBJS =	cpuport_arp.o

#OBJS +=	igmp_ethctl.o 

all : cpuport_arp.a

test_program:

ifeq ($(REBUILD_LIBRARY), y)
cpuport_arp.a: $(OBJS) $(wildcard *.h)
	$(AR) rc cpuport_arp.a $(OBJS)
else
cpuport_arp.a: 
	test -f $@
endif

clean:
	-rm -f $(SRC_RELATED_OBJS)
	-rm -f  *.bak core
