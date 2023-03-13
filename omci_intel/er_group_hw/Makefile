TOOLPREFIX?=rsdk-linux-

include ../makefile.inc

#OBJS=er_group_hw.o $(addsuffix .o,$(basename $(wildcard er_group_hw_[0-9]*.c)))
OBJS=	er_group_hw.o \
	er_group_hw_util.o \
	er_group_hw_6_47_48.o \
	er_group_hw_7.o \
	er_group_hw_11.o \
	er_group_hw_11_47.o \
	er_group_hw_11_47_78.o \
	er_group_hw_11_47_171.o \
	er_group_hw_11_130_266_268_277_78_401.o \
	er_group_hw_11_130_266_268_277_171_401.o \
	er_group_hw_11_130_266_268_277_401.o \
	er_group_hw_11_290.o \
	er_group_hw_45.o \
	er_group_hw_47.o \
	er_group_hw_45_47.o \
	er_group_hw_45_47_49.o \
	er_group_hw_45_47_79.o \
	er_group_hw_45_47_84.o \
	er_group_hw_45_280_298.o \
	er_group_hw_47_78.o \
	er_group_hw_47_78_130.o \
	er_group_hw_47_78_134.o \
	er_group_hw_47_78_329.o \
	er_group_hw_47_84_299_300.o \
	er_group_hw_47_130.o \
	er_group_hw_47_130_171.o \
	er_group_hw_47_130_262_266_268_277_280_402.o \
	er_group_hw_47_130_262_266_268_277_402.o \
	er_group_hw_47_130_266_268_277_280_402.o \
	er_group_hw_47_130_266_268_277_402.o \
	er_group_hw_47_130_299_300_302.o \
	er_group_hw_47_134_171.o \
	er_group_hw_47_171.o \
	er_group_hw_47_171_329.o \
	er_group_hw_47_262_266_268_277_280_402.o \
	er_group_hw_47_262_266_268_277_402.o \
	er_group_hw_47_266_268_277_280_402.o \
	er_group_hw_47_266_268_277_402.o \
	er_group_hw_47_266_268_401.o \
	er_group_hw_47_280_403.o \
	er_group_hw_47_280_404.o \
	er_group_hw_47_281_268.o \
	er_group_hw_47_299_300_302.o \
	er_group_hw_47_309_310.o \
	er_group_hw_47_329.o \
	er_group_hw_53.o \
	er_group_hw_53_58_139_142.o \
	er_group_hw_53_134_136_150_153.o \
	er_group_hw_53_139.o \
	er_group_hw_53_139_142.o \
	er_group_hw_53_139_142_143.o \
	er_group_hw_53_140.o \
	er_group_hw_53_141.o \
	er_group_hw_53_144.o \
	er_group_hw_53_145_153.o \
	er_group_hw_53_146_153.o \
	er_group_hw_53_147_153.o \
	er_group_hw_53_150_151_153.o \
	er_group_hw_53_150_152_153.o \
	er_group_hw_53_153.o \
	er_group_hw_130_280_298.o \
	er_group_hw_131.o \
	er_group_hw_134.o \
	er_group_hw_137_148_157_340.o \
	er_group_hw_138.o \
	er_group_hw_149.o \
	er_group_hw_256.o \
	er_group_hw_262.o \
	er_group_hw_262_277_278_402.o \
	er_group_hw_262_278.o \
	er_group_hw_263.o \
	er_group_hw_268_277.o \
	er_group_hw_268_280_401.o \
	er_group_hw_277_402.o \
	er_group_hw_290_291.o \
	er_group_hw_300.o \
	er_group_hw_158.o \
	er_group_hw_334.o \
	er_group_hw_425.o \
	er_group_hw_82.o \
	er_group_hw_83.o \
	er_group_hw_90.o \
	er_group_hw_349.o \
	er_group_hw_153_150.o \
	er_group_hw_65283.o \
	er_group_hw_voip_wrapper.o

all : er_group_hw.a

test_program:

ifeq ($(REBUILD_LIBRARY), y)
er_group_hw.a: $(OBJS) $(wildcard *.h)
	$(AR) rc er_group_hw.a $(OBJS)
else
er_group_hw.a: 
	test -f $@
endif

clean:
	-rm -f $(SRC_RELATED_OBJS)
	-rm -f *.bak core
