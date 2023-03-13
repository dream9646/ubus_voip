include ../makefile.inc

OBJS=simplexml.o xmlparse.o xmlomci.o xmlomci_spec.o xmlomci_config.o xmlomci_er_group.o xmlomci_env.o

all : xmlparse.a

test_program:	xmltest

ifeq ($(REBUILD_LIBRARY), y)
xmlparse.a: $(OBJS) $(wildcard *.h)
	$(AR) rc xmlparse.a $(OBJS)
else
xmlparse.a: 
	test -f $@
endif

xmltest: xmltest.o xmlparse.a
	$(CC) $(LOCAL_CFLAGS) $(CFLAGS) -o xmltest xmltest.o $(LIBS)

clean:
	-rm -f $(SRC_RELATED_OBJS)
	-rm -f  *.bak core xmltest
