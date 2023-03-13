include ../makefile.inc

OBJS= classf_collect.o classf_join.o classf_determine.o classf_print.o classf_protocol_vlan.o

all : classf.a

test_program:

ifeq ($(REBUILD_LIBRARY), y)
classf.a: $(OBJS) $(wildcard *.h)
	$(AR) rc classf.a $(OBJS)
else
classf.a:
	test -f $@
endif

clean:
	-rm -f $(SRC_RELATED_OBJS)
	-rm -f  *.bak core
