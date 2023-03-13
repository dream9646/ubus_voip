include ../makefile.inc

OBJS=omcimsg.o omcimsg_handler.o \
	omcimsg_alarm.o omcimsg_get_all_alarms.o \
	omcimsg_avc.o \
	omcimsg_create.o omcimsg_delete.o \
	omcimsg_get.o omcimsg_get_next.o omcimsg_set.o \
	omcimsg_mib_reset.o \
	omcimsg_mib_upload.o \
	omcimsg_get_current_data.o \
	omcimsg_reboot.o \
	omcimsg_sw_download.o \
	omcimsg_synchronize_time.o \
	omcimsg_test.o 

all : omcimsg.a

test_program:

ifeq ($(REBUILD_LIBRARY), y)
omcimsg.a: $(OBJS) $(wildcard *.h)
	$(AR) rc omcimsg.a $(OBJS)
else
omcimsg.a: 
	test -f $@
endif

clean:
	-rm -f $(SRC_RELATED_OBJS)
	-rm -f  *.bak core
