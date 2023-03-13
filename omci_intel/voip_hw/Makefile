TOOLPREFIX?=rsdk-linux-

include ../makefile.inc

OBJS = voip_hw.o voip_hw_util.o

all : voip_hw.a

test_program:

ifeq ($(REBUILD_LIBRARY), y)
voip_hw.a: $(OBJS) $(wildcard *.h) ../makefile.omci.inc
	$(AR) rc voip_hw.a $(OBJS)
else
voip_hw.a: 
	test -f $@
endif

clean:
	-rm -f $(SRC_RELATED_OBJS)
	-rm -f *.bak core

