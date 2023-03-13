#TOOLPREFIX?=rsdk-linux-

include ../makefile.inc

OBJS=gpon.o 

all : gpon.a

test_program:

ifeq ($(REBUILD_LIBRARY), y)
gpon.a: $(OBJS) $(wildcard *.h)
	$(AR) rc gpon.a $(OBJS)
else
gpon.a: 
	test -f $@
endif

clean:
	-rm -f $(SRC_RELATED_OBJS)
	-rm -f  *.bak core
