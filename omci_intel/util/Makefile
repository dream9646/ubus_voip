include ../makefile.inc

OBJS=	util.o util_inet.o util_socket.o util_run.o \
	env.o conv.o crc.o prbs.o \
	fencrypt.o pingtest.o trtest.o \
	regexp.o memdbg.o iphost.o \
	url.o kv_config.o wanif.o 
	
ifeq ($(CONFIG_TARGET_PRX126), y)	
OBJS+= util_uci.o 
else ifeq ($(CONFIG_TARGET_PRX120), y)	
OBJS+= util_uci.o 
else ifeq ($(CONFIG_TARGET_PRX321), y)	
OBJS+= util_uci.o 
endif

all : util.a

test_program:

ifeq ($(REBUILD_LIBRARY), y)
util.a: $(OBJS) $(wildcard *.h)
	$(AR) rc util.a $(OBJS)
else
util.a: 
	test -f $@
endif

clean:
	-rm -f $(SRC_RELATED_OBJS)
	-rm -f  *.bak core
	if [ -f fencrypt.c ]; then \
		rm -f fencrypt.x86; \
	fi
