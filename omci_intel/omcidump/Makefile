include ../makefile.inc

OBJS=	omcidump.o \
	omcidump_create.o \
	omcidump_alarm.o \
	omcidump_get_all_alarms.o \
	omcidump_delete.o \
	omcidump_get.o \
	omcidump_get_next.o \
	omcidump_set.o \
	omcidump_mib_reset.o \
	omcidump_mib_upload.o \
	omcidump_get_current_data.o \
	omcidump_avc.o \
	omcidump_synchronize_time.o \
	omcidump_reboot.o \
	omcidump_sw_download.o \
	omcidump_test.o

all : omcidump.a

test_program:

ifeq ($(REBUILD_LIBRARY), y)
omcidump.a: $(OBJS) $(wildcard *.h)
	$(AR) rc omcidump.a $(OBJS)
else
omcidump.a: 
	test -f $@
endif

clean:
	-rm -f $(SRC_RELATED_OBJS)
	-rm -f  *.bak core
