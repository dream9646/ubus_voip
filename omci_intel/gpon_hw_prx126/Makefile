TOOLPREFIX?=rsdk-linux-

include ../makefile.inc

OBJS =	gpon_hw_prx126.o gpon_hw_prx126_conv.o gpon_hw_prx126_i2c.o \
	gpon_hw_prx126_onu.o gpon_hw_prx126_pm.o gpon_hw_prx126_tm.o \
	gpon_hw_prx126_util.o gpon_hw_prx126_history.o
	

#OBJS=gpon_hw_prx126.o

all : gpon_hw_prx126.a

test_program:

ifeq ($(REBUILD_LIBRARY), y)
gpon_hw_prx126.a: $(OBJS) $(wildcard *.h)
	$(AR) rc gpon_hw_prx126.a $(OBJS)
else
gpon_hw_prx126.a: 
	test -f $@
endif

clean:
	-rm -f $(SRC_RELATED_OBJS)
	-rm -f *.bak core
