include ../makefile.inc

OBJS=	batchtab.o batchtab_cb.o \
	batchtab_cb_filtering.o batchtab_cb_tagging.o \
	batchtab_cb_tm.o batchtab_cb_td.o batchtab_cb_gemflow.o batchtab_cb_classf.o \
	batchtab_cb_wan.o batchtab_cb_isolation.o batchtab_cb_mac_filter.o \
	batchtab_cb_linkready.o batchtab_cb_autouni.o batchtab_cb_autoveip.o \
	batchtab_cb_mcastmode.o batchtab_cb_mcastbw.o batchtab_cb_mcastconf.o\
	batchtab_cb_wanif.o batchtab_cb_wand2p.o batchtab_cb_pots_mkey.o batchtab_cb_lldp.o \
	batchtab_cb_hardbridge.o
#batchtab_cb_gem_td.o 

all : batchtab.a

test_program:

ifeq ($(REBUILD_LIBRARY), y)
batchtab.a: $(OBJS) $(wildcard *.h)
	$(AR) rc batchtab.a $(OBJS)
else
batchtab.a: 
	test -f $@
endif

clean:
	-rm -f $(SRC_RELATED_OBJS)
	-rm -f  *.bak core
