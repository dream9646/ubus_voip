TOOLPREFIX?=rsdk-linux-

include ../makefile.inc

ifeq ($(TOOLKIT), y)
      
endif

#OBJS=er_group_hw.o $(addsuffix .o,$(basename $(wildcard er_group_hw_[0-9]*.c)))
OBJS =	mcast.o \
	mcast_cli.o \
	mcast_main.o \
	mcast_pkt.o \
	mcast_switch.o \
	mcast_timer.o \
	mcast_router.o \
	filter/preassign.o \
	filter/alu_p2p.o \
	filter/calix_txme.o \
	filter/calixlimit.o \
	filter/alu_signal.o \
	filter/mcast_version.o \
	filter/port_validator.o \
	filter/2511b.o \
	filter/igmp_omci.o \
	filter/pktdbg.o \
        counter/igmp_counter_simple.o \
	counter/igmp_counter_calix.o \
	counter/igmp_counter_calix_proxy.o \
	counter/igmp_counter_calix_eventlog.o \
	counter/igmp_counter_calix_proxy_eventlog.o \
	counter/igmp_counter_calix_15mins.o \
	mcast_proxy.o \
	mcast_snoop.o	\
	platform.o

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
