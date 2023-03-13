include ../makefile.inc

OBJS=fwk_cond.o fwk_msgq.o fwk_mutex.o fwk_rwlock.o fwk_sem.o fwk_thread.o fwk_timer.o

all : fwk.a

test_program:	msgqtest timertest

ifeq ($(REBUILD_LIBRARY), y)
fwk.a: $(OBJS) $(wildcard *.h)
	$(AR) rc fwk.a $(OBJS)
else
fwk.a: 
	test -f $@
endif

timertest: timertest.o fwk.a
	$(CC) $(CFLAGS) $(LOCAL_CFLAGS) -o timertest timertest.o $(LIBS)

msgqtest: msgqtest.o fwk.a
	$(CC) $(LOCAL_CFLAGS) $(CFLAGS) -o msgqtest msgqtest.o $(LIBS)

clean:
	-rm -f $(SRC_RELATED_OBJS)
	-rm -f  *.bak core timertest msgqtest
