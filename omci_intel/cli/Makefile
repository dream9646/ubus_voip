include ../makefile.inc

OBJS=	cli_version.o cli.o cli_env.o cli_history.o cli_misc.o \
	cli_config.o cli_spec.o cli_me.o cli_attr.o \
	cli_hwresource.o cli_er.o cli_tag.o \
	cli_bridge.o cli_pots.o cli_pots_mkey.o cli_unrelated.o \
	cli_cpuport.o cli_portinfo.o \
	cli_tcont.o cli_cfm.o cli_lldp.o cli_arp.o cli_stp.o cli_dhcp.o \
	cli_gfast.o cli_classf.o cli_bat.o cli_gpon.o \
	cli_acl.o cli_igmp.o cli_switch.o cli_extoam.o cli_nat.o cli_session.o

#REVISION=$(shell export LANG=C; svn info .. 2> /dev/null | grep Revision:|cut -d: -f2)
ifeq ($(shell test -e ../.git && echo -n yes),yes)
REVISION=$(shell export LANG=C; git log 2>/dev/null | head -1 | cut -c8-14)
else
REVISION=$(shell export LANG=C; basename $${PWD%/cli}|cut -c6-12)
endif

all : cli.a

test_program:

ifeq ($(REBUILD_LIBRARY), y)
cli.a: mk_revision $(OBJS) $(wildcard *.h)
	$(AR) rc cli.a $(OBJS)
else
cli.a: 
	test -f $@
endif

mk_revision:
	@if [ ! -f revision.h ]; then \
	if [ -z $(REVISION) ]; then \
		(echo "#define TAG_REVISION \"0\"" > revision.h); \
	else \
		(echo "#define TAG_REVISION \"$(REVISION)\"" > revision.h); \
	fi \
	fi
	-@[ -f cli_version.o ] && rm cli_version.o

clean:
	-rm -f $(SRC_RELATED_OBJS)
	-rm -f  *.bak core revision.h
