CC = gcc
CFLAGS = -Os -Wall -Werror --std=gnu99 -g3 -Wmissing-declarations -DINSTALL_PREFIX="${CMAKE_INSTALL_PREFIX}"
LDFLAGS = 
INCLUDES = -I include
LIBS = -lubox -lubus -luci -lblobmsg_json -ljson-c -lcrypt -ldl

ifeq ($(shell uname), Darwin)
	INCLUDES += -I /opt/local/include
	LDFLAGS += -L /opt/local/lib
endif

ifeq ($(shell uname -s), Linux)
	ifeq ($(shell uname -m), x86_64)
		LDFLAGS += -L /usr/lib64
	else
		LDFLAGS += -L /usr/lib
	endif
endif

ifdef FILE_SUPPORT
	CFLAGS += -DFILE_SUPPORT
endif
ifdef IWINFO_SUPPORT
	CFLAGS += -DIWINFO_SUPPORT
endif
ifdef RPCSYS_SUPPORT
	CFLAGS += -DRPCSYS_SUPPORT
endif
ifdef UCODE_SUPPORT
	CFLAGS += -DUCODE_SUPPORT
endif

ifdef HAVE_SHADOW
	CFLAGS += -DHAVE_SHADOW
endif

ifeq ($(shell pkg-config --exists libubox && echo 1 || echo 0), 1)
	LIBS += $(shell pkg-config --libs libubox)
	INCLUDES += $(shell pkg-config --cflags libubox)
endif
ifeq ($(shell pkg-config --exists libubus && echo 1 || echo 0), 1)
	LIBS += $(shell pkg-config --libs libubus)
	INCLUDES += $(shell pkg-config --cflags libubus)
endif
ifeq ($(shell pkg-config --exists uci && echo 1 || echo 0), 1)
	LIBS += $(shell pkg-config --libs uci)
	INCLUDES += $(shell pkg-config --cflags uci)
endif

SRCS = ubus_voip.c
OBJS = $(SRCS:.c=.o)

.PHONY: all clean

all: fvt_evoip

fvt_evoip: $(OBJS)
	$(CC) $(LDFLAGS) $^ $(LIBS) -o $@

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f fvt_evoip $(OBJS)

install: fvt_evoip
	mkdir -p $(DESTDIR)/usr/sbin $(DESTDIR)/usr/lib/fvt_evoip
	install -m 755 fvt_evoip $(DESTDIR)/usr/sbin/
	install -m 755 $(PLUGINS) $(DESTDIR)/usr/lib/fvt_evoip/
