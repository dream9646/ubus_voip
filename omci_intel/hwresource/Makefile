include ../makefile.inc

OBJS=hwresource.o hwresource_cb.o

all : hwresource.a

test_program:

ifeq ($(REBUILD_LIBRARY), y)
hwresource.a: $(OBJS) $(wildcard *.h)
	$(AR) rc hwresource.a $(OBJS)
else
hwresource.a: 
	test -f $@
endif

clean:
	-rm -f $(SRC_RELATED_OBJS)
	-rm -f  *.bak core
