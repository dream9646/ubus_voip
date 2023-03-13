include ../makefile.inc

OBJS=clitask.o coretask.o recvtask.o tranxmod.o alarmtask.o pmtask.o avctask.o testtask.o tasks.o restore.o

all : tasks.a

test_program:

ifeq ($(REBUILD_LIBRARY), y)
tasks.a: $(OBJS) $(wildcard *.h)
	$(AR) rc tasks.a $(OBJS)
else
tasks.a: 
	test -f $@
endif

clean:
	-rm -f $(SRC_RELATED_OBJS)
	-rm -f  *.bak core
