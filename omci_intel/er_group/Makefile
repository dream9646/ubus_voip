include ../makefile.inc

OBJS=er_me_group.o er_attr_group.o er_attr_group_hw.o er_attr_group_util.o

all : er_group.a

test_program:	er_attr_group_hw_test

ifeq ($(REBUILD_LIBRARY), y)
er_group.a: $(OBJS) $(wildcard *.h)
	$(AR) rc $@ $(OBJS)
else
er_group.a:
	test -f $@
endif

er_attr_group_hw_test: er_attr_group_hw_test.o er_group.a
	$(CC) $(LOCAL_CFLAGS) $(CFLAGS) -o er_attr_group_hw_test er_attr_group_hw_test.o $(LIBS)

clean:
	-rm -f $(SRC_RELATED_OBJS)
	-rm -f  *.bak core er_attr_group_hw_test
