include ../makefile.inc

CFLAGS+= -I$(PLATFORM_DIR)/sdk/include
CFLAGS+= -I$(PLATFORM_DIR)/sdk/system/include
CFLAGS+= -I$(PLATFORM_DIR)/sdk/src/app/diag_rtk/include
CFLAGS+= -I$(PLATFORM_DIR)/sdk/include/hal
#CFLAGS+= -DX86
#CFLAGS+= -DACLTEST

OBJS = acl.o vacl.o vacl_util.o

all : acl.a

test_program: vacltest


ifeq ($(REBUILD_LIBRARY), y)
acl.a: $(OBJS) $(wildcard *.h)
	$(AR) rc acl.a $(OBJS)
else
acl.a:
	test -f $@
endif

vacltest: clean $(OBJS) vacltest.o vacltest_util.o
	$(CC) $(LOCAL_CFLAGS) $(CFLAGS) -o vacltest vacltest_util.o $(OBJS) ../util/conv.o ../util/util_inet.o vacltest.o

vacltest_hw: acl.a vacltest.o
	$(CC) $(LOCAL_CFLAGS) $(CFLAGS) -o vacltest_hw vacltest.o $(LIBS)
	$(STRIP) vacltest_hw

clean:
	-rm -f $(SRC_RELATED_OBJS)
	-rm -f  *.bak core vacltest
