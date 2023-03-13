TOOLPREFIX?=rsdk-linux-

include ../makefile.inc

#OBJS=meinfo_hw.o $(addsuffix .o,$(basename $(wildcard meinfo_hw_[0-9]*.c)))
OBJS=meinfo_hw.o \
	meinfo_hw_util.o \
	meinfo_hw_rf.o \
	meinfo_hw_poe.o \
	meinfo_hw_anig.o \
	meinfo_hw_anig_fh.o \
	meinfo_hw_11.o \
	meinfo_hw_48.o \
	meinfo_hw_47.o \
	meinfo_hw_46.o \
	meinfo_hw_45.o \
	meinfo_hw_134.o \
	meinfo_hw_136.o \
	meinfo_hw_138.o \
	meinfo_hw_139.o \
	meinfo_hw_140.o \
	meinfo_hw_141.o \
	meinfo_hw_142.o \
	meinfo_hw_143.o \
	meinfo_hw_144.o \
	meinfo_hw_145.o \
	meinfo_hw_146.o \
	meinfo_hw_147.o \
	meinfo_hw_148.o \
	meinfo_hw_149.o \
	meinfo_hw_150.o \
	meinfo_hw_151.o \
	meinfo_hw_152.o \
	meinfo_hw_153.o \
	meinfo_hw_157.o \
	meinfo_hw_171.o \
	meinfo_hw_24.o \
	meinfo_hw_256.o \
	meinfo_hw_257.o \
	meinfo_hw_262.o \
	meinfo_hw_263.o \
	meinfo_hw_267.o \
	meinfo_hw_268.o \
	meinfo_hw_276.o \
	meinfo_hw_277.o \
	meinfo_hw_290.o \
	meinfo_hw_296.o \
	meinfo_hw_312.o \
	meinfo_hw_321.o \
	meinfo_hw_322.o \
	meinfo_hw_330.o \
	meinfo_hw_341.o \
	meinfo_hw_349.o \
	meinfo_hw_53.o \
	meinfo_hw_52.o \
	meinfo_hw_50.o \
	meinfo_hw_58.o \
	meinfo_hw_6.o \
	meinfo_hw_7.o \
	meinfo_hw_89.o \
	meinfo_hw_82.o \
	meinfo_hw_90.o \
	meinfo_hw_131.o \
        meinfo_hw_425.o \
        meinfo_hw_137.o \
        meinfo_hw_334.o

all : meinfo_hw.a libm02098.a

test_program:

ifeq ($(REBUILD_LIBRARY), y)
meinfo_hw.a: $(OBJS) $(wildcard *.h)
	$(AR) rc meinfo_hw.a $(OBJS)
else
meinfo_hw.a: 
	test -f $@
endif

libm02098.a:
	cp libm02098.a.mips libm02098.a

clean:
	-rm -f $(SRC_RELATED_OBJS)
	-rm -f  *.bak core
