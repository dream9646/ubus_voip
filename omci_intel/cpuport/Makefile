include ../makefile.inc

#OBJS=er_group_hw.o $(addsuffix .o,$(basename $(wildcard er_group_hw_[0-9]*.c)))
OBJS =	cpuport_frame.o \
	cpuport_recv.o cpuport_pkt.o \
	cpuport_extract_filter.o \
	cpuport_vtagging_vfilter.o \
	cpuport_pvlan.o \
	cpuport_util_send.o \
	cpuport_util.o \
	cpuport_null.o cpuport_sniffer.o cpuport_history.o \
	cpuport.o 

#OBJS +=	igmp_ethctl.o 

all : cpuport.a

test_program:

ifeq ($(REBUILD_LIBRARY), y)
cpuport.a: $(OBJS) $(wildcard *.h)
	$(AR) rc cpuport.a $(OBJS)
else
pkt.a: 
	test -f $@
endif

clean:
	-rm -f $(SRC_RELATED_OBJS)
	-rm -f  *.bak core
