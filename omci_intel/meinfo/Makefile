include ../makefile.inc

OBJS=meinfo.o meinfo_conv.o meinfo_instance.o meinfo_me_attr.o meinfo_me.o meinfo_config.o
all : meinfo.a

test_program:

ifeq ($(REBUILD_LIBRARY), y)
meinfo.a: $(OBJS) $(wildcard *.h)
	$(AR) rc meinfo.a $(OBJS)
else
meinfo.a: 
	test -f $@
endif

clean:
	-rm -f $(SRC_RELATED_OBJS)
	-rm -f  *.bak core
