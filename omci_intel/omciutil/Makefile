include ../makefile.inc

OBJS= omciutil_vlan.o \
	omciutil_vlan_collect.o omciutil_vlan_compare.o omciutil_vlan_reverse.o \
	omciutil_vlan_pbitmap.o omciutil_vlan_str.o omciutil_vlan_valid.o \
	omciutil_stp.o omciutil_usage.o omciutil_misc.o

all : omciutil.a

test_program:

ifeq ($(REBUILD_LIBRARY), y)
omciutil.a: $(OBJS) $(wildcard *.h)
	$(AR) rc omciutil.a $(OBJS)
else
omciutil.a: 
	test -f $@
endif

clean:
	-rm -f $(SRC_RELATED_OBJS)
	-rm -f  *.bak core
