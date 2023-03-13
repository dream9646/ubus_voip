include ../makefile.inc

OBJS =	cpuport_dhcp.o packet_gen.o \

all : cpuport_dhcp.a

test_program:

ifeq ($(REBUILD_LIBRARY), y)
cpuport_dhcp.a: $(OBJS) $(wildcard *.h)
	$(AR) rc cpuport_dhcp.a $(OBJS)
else
cpuport_dhcp.a: 
	test -f $@
endif

clean:
	-rm -f $(SRC_RELATED_OBJS)
	-rm -f  *.bak core
