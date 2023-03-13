include ../makefile.inc

LIB=	proprietary_ericsson.a

OBJS=	$(shell ls proprietary_*.[co] 2>/dev/null|sed -e"s/\.c$$/\.o/"|sort|uniq)
OBJS+=	$(shell ls me_related_*.[co] 2>/dev/null|sed -e"s/\.c$$/\.o/"|sort|uniq)
OBJS+=	$(shell ls meinfo_cb_*.[co] 2>/dev/null|sed -e"s/\.c$$/\.o/"|sort|uniq)
OBJS+=	$(shell ls batchtab_cb_*.[co] 2>/dev/null|sed -e"s/\.c$$/\.o/"|sort|uniq)
ifneq ($(TOOLPREFIX), /usr/bin/)
OBJS+=	$(shell ls meinfo_hw_*.[co] 2>/dev/null|sed -e"s/\.c$$/\.o/"|sort|uniq)
OBJS+=	$(shell ls er_group_hw_*.[co] 2>/dev/null|sed -e"s/\.c$$/\.o/"|sort|uniq)
endif

all: $(LIB)

test_program:

ifeq ($(REBUILD_LIBRARY), y)
$(LIB): $(OBJS) $(wildcard *.h)
	$(AR) rc $(LIB) $(OBJS)
else
$(LIB):
	test -f $@
endif

clean:
	-rm -f $(SRC_RELATED_OBJS)
	-rm -f  *.bak *.o core
