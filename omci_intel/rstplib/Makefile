include ../makefile.inc

CFLAGS += -D__LINUX__ -Wall

OBJS =	bridge.o \
	edge.o\
	migrate.o\
	p2p.o\
	pcost.o\
	port.o\
	portinfo.o\
	rolesel.o\
	roletrns.o\
	statmch.o\
	stp_in.o\
	stpm.o\
	stpmgmt.o\
	stp_to.o\
	sttrans.o\
	times.o\
	topoch.o\
	transmit.o\
	uid_sock.o\
	vector.o\
	rstp_cfg.o\
	rstp_omci.o\
	rstp.o\
	rstp_port.o\
	rstp_cfg.o\
	rstp_info.o\
	rstp_cmd_handler.o

all : rstp.a

test_program:

ifeq ($(REBUILD_LIBRARY), y)
rstp.a: $(OBJS) $(wildcard *.h)
	$(AR) rc rstp.a $(OBJS)
else
rstp.a: 
	test -f $@
endif

clean:
	-rm -f $(SRC_RELATED_OBJS)
	-rm -f  *.bak core
