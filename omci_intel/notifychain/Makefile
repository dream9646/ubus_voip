include ../makefile.inc

OBJS=notify_chain.o 

all : notify_chain.a

test_program:	notify_chain_test

ifeq ($(REBUILD_LIBRARY), y)
notify_chain.a: $(OBJS) $(wildcard *.h)
	$(AR) rc notify_chain.a $(OBJS)
else
notify_chain.a: 
	test -f $@
endif

notify_chain_test: notify_chain_test.o notify_chain.a
	$(CC) $(LOCAL_CFLAGS) $(CFLAGS) -o notify_chain_test notify_chain_test.o $(LIBS)

clean:
	-rm -f $(SRC_RELATED_OBJS)
	-rm -f  *.bak core notify_chain_test

