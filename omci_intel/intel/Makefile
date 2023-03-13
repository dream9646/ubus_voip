include ../makefile.inc

OBJS= intel_px126_adapter.o intel_px126_util.o intel_px126_omcc_util.o intel_px126_mcc_util.o intel_px126_tm.o intel_px126_pon.o intel_px126_port.o intel_px126_pm.o intel_px126_sw_image.o
	
CPROTO_SRC = $(OBJS:.o=.c)

all : cproto intel_px126_util.a

test_program:

ifeq ($(REBUILD_LIBRARY), y)
intel_px126_util.a: $(OBJS) $(wildcard *.h) ../makefile.omci.inc
	$(AR) rc intel_px126_util.a $(OBJS)
else
intel_px126_util.a: 
	test -f $@
endif

clean:
	-rm -f $(SRC_RELATED_OBJS)
	-rm -f  *.bak core
	
	
# since intel_px126_util_proto.h is included in almost switch_hw_prx126_*.c, 
# we create a dummy intel_px126_util_proto.h first so cproto wont error when parsing include directive of per switch_hw_prx126_*.c
cproto:
	touch intel_px126_util_proto.h; cproto -E "$(CC) -E $(LOCAL_CFLAGS) $(CFLAGS) $(CPPFLAGS)" $(CPROTO_SRC) 2>/dev/null |grep -v ^__ > .proto.h; mv .proto.h intel_px126_util_proto.h	