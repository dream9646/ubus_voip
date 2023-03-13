TOOLPREFIX?=rsdk-linux-

include ../makefile.inc

#OBJS=er_group_hw.o $(addsuffix .o,$(basename $(wildcard er_group_hw_[0-9]*.c)))
OBJS =	mcast.o \
	mcast_cli.o \
	mcast_main.o \
	mcast_switch.o \
	igmp_mbship.o \
	mcast_timer.o \
	mcast_pkt.o \
	igmp_omci.o \
	mcast_router.o \
        counter/igmp_counter_simple.o \
	counter/igmp_counter_calix.o \
	counter/igmp_counter_calix_proxy.o \
	counter/igmp_counter_calix_eventlog.o \
	counter/igmp_counter_calix_proxy_eventlog.o \
	counter/igmp_counter_calix_15mins.o \
        igmp_v2.o \
	mcast_proxy.o \
	igmp_v3_lw_omci.o \
	mcast_snoop.o

#OBJS +=	igmp_ethctl.o 

all : igmp_proxy.a

test_program:

ifeq ($(REBUILD_LIBRARY), y)
igmp_proxy.a: $(OBJS) $(wildcard *.h)
	$(AR) rc igmp_proxy.a $(OBJS)
else
igmp_proxy.a: 
	test -f $@
endif

clean:
	-rm -f $(SRC_RELATED_OBJS)
	-rm -f  *.bak core
