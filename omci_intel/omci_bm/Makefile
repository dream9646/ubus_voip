# this variable must be defined before 'include ../makefile.inc'
DONT_LOAD_CONFIG_ARCH=y

include ../makefile.inc

ifeq ($(CONFIG_RTL9600_SERIES),y)
	BIND_MAC_CONFIGNAME=5vt2510
else ifeq ($(CONFIG_RTL9601B_SERIES),y)
	BIND_MAC_CONFIGNAME=5vt2510
else ifeq ($(CONFIG_RTL9602C_SERIES),y)
	BIND_MAC_CONFIGNAME=5vt2512
else ifeq ($(CONFIG_RTL9607C_SERIES),y)
	BIND_MAC_CONFIGNAME=5vt2518
else ifeq ($(CONFIG_TARGET_BRCM63XX),y)
	BIND_MAC_CONFIGNAME=bcm63xx
else 
$(error "chipset not supported? $(CONFIG_TARGET_BRCM63XX)")
endif

OBJS = omci_bm.o
OBJS += libheimdal.o

all : prepare_bind_mac omci_bm.a omci_bm_test

prepare_bind_mac:
	@[ -d bind_mac ] || git clone git://10.20.0.8/voip/bind_mac.git
	@[ -f bind_mac/build_$(BIND_MAC_CONFIGNAME)/lib/libheimdal.o ] || \
		(echo "building libheimdal.o for $(BIND_MAC_CONFIGNAME)..."; cd bind_mac; \
		 env -i PATH=/usr/local/bin:/usr/local/sbin:/usr/bin:/usr/sbin:/sbin:/bin sh makeall.sh $(BIND_MAC_CONFIGNAME) >build.log 2>build.log )
	@ln -sf bind_mac/bm_b64.c .
	@ln -sf bind_mac/bm_if.c .
	@ln -sf bind_mac/bm_key.c .
	@ln -sf bind_mac/bm_sum.c .
	@ln -sf bind_mac/key_pub.h .
	@ln -sf bind_mac/build_$(BIND_MAC_CONFIGNAME)/lib/libheimdal.o .

CFLAGS+= -I bind_mac/build_$(BIND_MAC_CONFIGNAME)/include -I bind_mac/heimdal/lib/

# always compiling with -Os and no debug info
CFLAGS:=$(subst -O0, -Os, $(CFLAGS))
CFLAGS:=$(subst -g ,, $(CFLAGS))

ifeq ($(REBUILD_LIBRARY), y)
omci_bm.a: $(OBJS) $(wildcard *.h) $(wildcard *.c)
	$(AR) rc omci_bm.a $(OBJS)
else
omci_bm.a:
	test -f $@
endif

omci_bm_test: omci_bm.a
	$(CC) $(LOCAL_CFLAGS) $(CFLAGS) -o omci_bm_test omci_bm_test.c omci_bm.a -ldl

clean:
	-rm -f $(SRC_RELATED_OBJS) omci_bm_test
	-rm -f *.bak core
	-rm -Rf bind_mac bm_b64.c bm_if.c bm_key.c bm_sum.c key_pub.h libheimdal.o 2>/dev/null
