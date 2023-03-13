include ../makefile.inc

#OBJS=er_group_hw.o $(addsuffix .o,$(basename $(wildcard er_group_hw_[0-9]*.c)))
OBJS =	omci_config.o

all : omci_config.a

test_program:

#ifeq ($(REBUILD_LIBRARY), y)
omci_config.a: $(OBJS) $(wildcard *.h)
	$(AR) rc omci_config.a $(OBJS)
#else
#omci_config.a: 
#	test -f $@
#endif

clean:
	-rm -f $(SRC_RELATED_OBJS)
	-rm -f  *.bak core
