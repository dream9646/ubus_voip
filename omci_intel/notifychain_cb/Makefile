include ../makefile.inc

OBJS=me_createdel.o me_createdel_table.o

all : notify_chain_cb.a

test_program:

ifeq ($(REBUILD_LIBRARY), y)
notify_chain_cb.a: $(OBJS) $(wildcard *.h)
	$(AR) rc notify_chain_cb.a $(OBJS)
else
notify_chain_cb.a: 
	test -f $@
endif

clean:
	-rm -f $(SRC_RELATED_OBJS)
	-rm -f  *.bak core
