#try to use TOOLPREFIX defined by parent makefile or env
TOOLPREFIX?=/usr/bin/

export TOOLPREFIX

include ../makefile.inc

LIBDIRS= ../fwk/ \
           ../tasks/ \
           ../omcimsg/ \
           ../omcidump/ \
           ../omciutil/ \
	   ../meinfo/ \
           ../hwresource \
           ../meinfo_cb \
           ../me_related \
           ../er_group \
           ../notifychain/ \
           ../notifychain_cb/ \
           ../xmlparse/ \
           ../extoam/ \
           ../util/ \
           ../cli/ \
           ../config

LIBDIRS+=../batchtab \
	   ../switch/ \
           ../cpuport/ \
           ../gpon/ \
           ../classf \
           ../libkv \
           ../tm/ \
           ../acl/

LIBDIRS+=../cpuport_arp/ \
           ../cpuport_dhcp/ \
           ../cpuport_pppoe/ \
           ../cfm/ \
           ../lldp/ \
           ../igmp/ \
           ../rstplib \
           ../voip/ \
		   ../meta_cfg/ \
           $(shell for d in $(OMCI_DIR)/proprietary_*; do echo "../$$(basename $$d)"; done)

ifeq ($(OMCI_BM_ENABLE), y)
	LIBDIRS+= ../omci_bm
endif

ifneq ($(TOOLPREFIX),/usr/bin/)
ifneq ($(CONFIG_TARGET_PRX126),)
	LIBDIRS+= ../switch_hw_prx126
	LIBDIRS+= ../gpon_hw_prx126
	LIBDIRS+= ../cpuport_hw_prx126
	LIBDIRS+= ../acl_hw_prx126
	LIBDIRS+= ../intel
endif
ifneq ($(CONFIG_TARGET_PRX120),)
	LIBDIRS+= ../switch_hw_prx126
	LIBDIRS+= ../gpon_hw_prx126
	LIBDIRS+= ../cpuport_hw_prx126
	LIBDIRS+= ../acl_hw_prx126
	LIBDIRS+= ../intel
endif
ifneq ($(CONFIG_TARGET_PRX321),)
	LIBDIRS+= ../switch_hw_prx126
	LIBDIRS+= ../gpon_hw_prx126
	LIBDIRS+= ../cpuport_hw_prx126
	LIBDIRS+= ../acl_hw_prx126
	LIBDIRS+= ../intel
endif
	LIBDIRS+= ../meinfo_hw
	LIBDIRS+= ../er_group_hw
	LIBDIRS+= ../voip_hw
	LIBDIRS+= ../voipclient
endif

#$(info LIBDIRS=$(LIBDIRS))
#$(info OMCI_LIB=$(OMCI_LIB))
#$(info OMCI_INCLUDE=$(OMCI_INCLUDE))
#$(shell sleep 10)

#SOURCES = $(wildcard *.c)

all :	libdirs omcimain.a omcimain client_prog

libdirs:
	@for i in $(LIBDIRS); do \
		if [ -f $$i/Makefile ]; then \
			cd $$i; echo ""; make all || exit 1; cd ../src/ ;  \
		fi; \
	done

libdirs_test:
	@for i in $(LIBDIRS); do \
		if [ -f $$i/Makefile ]; then \
			cd $$i; echo ""; make test_program || exit 1; cd ../src/ ;  \
		fi; \
	done

libdirs_clean:
	@for i in $(LIBDIRS); do \
		if [ -f $$i/Makefile ]; then \
			cd $$i; echo ""; make clean ; cd ../src/ ;  \
		fi; \
	done

omcimain.a: omcimain.o $(wildcard *.h)
	$(AR) rc omcimain.a omcimain.o

omcimain: main.o omcimain.a $(OMCI_LIB) $(wildcard *.h)
	$(CC) $(LOCAL_CFLAGS) $(CFLAGS) -o omcimain main.o $(LIBS) 

client_prog: $(wildcard ../client_prog/*.c)
	make -C ../client_prog

update_cli:
	make -C ../client_prog update_cli

oltsim: $(wildcard ../oltsim/*.c)
	make -C ../oltsim

clean: libdirs_clean
	-rm -f $(SRC_RELATED_OBJS)
	-rm -f  *.bak core omcimain.a omcimain omcimain.tgz gpon_monitor.tgz
	-make -C ../client_prog clean
	-make -C ../oltsim clean

#rtk_api:
#	cat ../*/*.c|grep 'rtk_.*(' |sed -e "s/rtk_/\nrtk_/g"|grep '^rtk_[^ ]*('|cut -d '(' -f1|grep -v ')' |cut -d' ' -f1|sort|uniq > rtk_api.txt

