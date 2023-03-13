TOOLPREFIX?=mips-linux-gnu-

include ../makefile.inc

OBJS= voipclient_util.o voipclient_comm.o voipclient_cmd.o

all : voipclient.a

test_program: voipclient_example

ifeq ($(REBUILD_LIBRARY), y)
voipclient.a: $(OBJS) $(wildcard *.h) ../makefile.omci.inc
	$(AR) rc voipclient.a $(OBJS)
else
voipclient.a: 
	test -f $@
endif

clean:
	-rm -f $(SRC_RELATED_OBJS)
	-rm -f *.bak core voipclient_example

