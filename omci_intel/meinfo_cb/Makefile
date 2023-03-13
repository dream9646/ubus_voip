include ../makefile.inc

#OBJS=meinfo_cb.o $(addsuffix .o,$(basename $(wildcard meinfo_cb_[0-9]*.c)))
OBJS=meinfo_cb.o $(shell ls meinfo_cb_[0-9]*.[co]|sed -e"s/\.c$$/\.o/"|sort|uniq)

all : meinfo_cb.a

test_program:

ifeq ($(REBUILD_LIBRARY), y)
meinfo_cb.a: $(OBJS) $(wildcard *.h)
	$(AR) rc meinfo_cb.a $(OBJS)
else
meinfo_cb.a: 
	test -f $@
endif

clean:
	-rm -f $(SRC_RELATED_OBJS)
	-rm -f  *.bak core

