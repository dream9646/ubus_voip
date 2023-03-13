include ../makefile.inc

#OBJS=me_related.o $(addsuffix .o,$(basename $(wildcard me_related_[0-9]*.c)))
OBJS=me_related.o $(shell ls me_related_[0-9]*.[co]|sed -e"s/\.c$$/\.o/"|sort|uniq)

all : me_related.a

test_program:	me_related_test

ifeq ($(REBUILD_LIBRARY), y)
me_related.a: $(OBJS) $(wildcard *.h)
	$(AR) rc me_related.a $(OBJS)
else
me_related.a: 
	test -f $@
endif

me_related_test: me_related_test.o me_related.a
	$(CC) $(LOCAL_CFLAGS) $(CFLAGS) -o me_related_test me_related_test.o $(LIBS)

clean:
	-rm -f $(SRC_RELATED_OBJS)
	-rm -f  *.bak core me_related_test
