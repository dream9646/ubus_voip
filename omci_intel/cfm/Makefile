include ../makefile.inc

#OBJS=er_group_hw.o $(addsuffix .o,$(basename $(wildcard er_group_hw_[0-9]*.c)))
OBJS =	cfm_switch.o \
	cfm_bport.o \
	cfm_util.o \
	cfm_mp.o \
	cfm_send.o \
	cfm_recv.o \
	cfm_pkt.o \
	cfm_core.o \
	cfm_print.o \

all : cfm.a

test_program:

ifeq ($(REBUILD_LIBRARY), y)
cfm.a: $(OBJS) $(wildcard *.h)
	$(AR) rc cfm.a $(OBJS)
else
cfm.a: 
	test -f $@
endif

clean:
	-rm -f $(SRC_RELATED_OBJS)
	-rm -f  *.bak core
