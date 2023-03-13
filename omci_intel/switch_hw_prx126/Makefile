TOOLPREFIX?=rsdk-linux-

include ../makefile.inc

OBJS = switch_hw_prx126_conv.o
OBJS += switch_hw_prx126_hook.o
OBJS += switch_hw_prx126_acl.o
OBJS += switch_hw_prx126_init.o
OBJS += switch_hw_prx126_int.o
OBJS += switch_hw_prx126_ipmc.o
OBJS += switch_hw_prx126_mac.o
OBJS += switch_hw_prx126_misc.o
OBJS += switch_hw_prx126_pm.o
OBJS += switch_hw_prx126_history.o
OBJS += switch_hw_prx126_port.o
OBJS += switch_hw_prx126_qos.o
OBJS += switch_hw_prx126_rate.o
OBJS += switch_hw_prx126_rldp.o
OBJS += switch_hw_prx126_stp.o
OBJS += switch_hw_prx126_svlan.o
OBJS += switch_hw_prx126_switch.o
OBJS += switch_hw_prx126_vacl.o
OBJS += switch_hw_prx126_vlan.o
OBJS += switch_hw_prx126_classf.o
OBJS += switch_hw_prx126_trunk.o
OBJS += switch_hw_prx126_nat.o
OBJS += switch_hw_prx126_sw_image.o

CPROTO_SRC = $(OBJS:.o=.c)

all : cproto switch_hw_prx126.a

test_program:

ifeq ($(REBUILD_LIBRARY), y)
switch_hw_prx126.a: $(OBJS) $(wildcard *.h) ../makefile.omci.inc
	$(AR) rc switch_hw_prx126.a $(OBJS)
else
switch_hw.a: 
	test -f $@
endif

clean:
	-rm -f $(SRC_RELATED_OBJS)
	-rm -f *.bak core

# since switch_hw_prx126_proto.h is included in almost switch_hw_prx126_*.c, 
# we create a dummy switch_hw_prx126_proto.h first so cproto wont error when parsing include directive of per switch_hw_prx126_*.c
cproto:
	touch switch_hw_prx126_proto.h; cproto -E "$(CC) -E $(LOCAL_CFLAGS) $(CFLAGS) $(CPPFLAGS)" $(CPROTO_SRC) 2>/dev/null |grep -v ^__ > .proto.h; mv .proto.h switch_hw_prx126_proto.h
