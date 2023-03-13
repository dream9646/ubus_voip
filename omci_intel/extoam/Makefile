#TOOLPREFIX?=rsdk-linux-

include ../makefile.inc

#OBJS=er_group_hw.o $(addsuffix .o,$(basename $(wildcard er_group_hw_[0-9]*.c)))
OBJS =	extoam_entry.o \
	extoam_cmd.o \
	extoam_handle.o \
	extoam_link.o \
	extoam_msg.o \
	extoam_queue.o \
	extoam_swdl.o \
	extoam_loopback.o \
	extoam_object.o \
	extoam_event.o \

#OBJS +=	igmp_ethctl.o 

all : extoam.a

test_program:

ifeq ($(REBUILD_LIBRARY), y)
extoam.a: $(OBJS) $(wildcard *.h)
	$(AR) rc extoam.a $(OBJS)
else
extoam.a: 
	test -f $@
endif

clean:
	-rm -f $(SRC_RELATED_OBJS)
	-rm -f  *.bak core
