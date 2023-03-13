include ../makefile.inc

OBJS= metacfg_adapter.o metacfg_uci.o 
	
CPROTO_SRC = $(OBJS:.o=.c)

all : cproto metacfg.a

test_program:

ifeq ($(REBUILD_LIBRARY), y)
metacfg.a: $(OBJS) $(wildcard *.h) ../makefile.omci.inc
	$(AR) rc metacfg.a $(OBJS)
else
metacfg.a: 
	test -f $@
endif

clean:
	-rm -f $(SRC_RELATED_OBJS)
	-rm -f  *.bak core
	
	
# since intel_px126_util_proto.h is included in almost switch_hw_prx126_*.c, 
# we create a dummy intel_px126_util_proto.h first so cproto wont error when parsing include directive of per switch_hw_prx126_*.c
cproto:
	touch intel_px126_util_proto.h; cproto -E "$(CC) -E $(LOCAL_CFLAGS) $(CFLAGS) $(CPPFLAGS)" $(CPROTO_SRC) 2>/dev/null |grep -v ^__ > .proto.h; mv .proto.h intel_px126_util_proto.h	