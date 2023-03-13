
# default build omci for trex

ifeq ($(OMCI_DIR),)
OMCI_DIR?=$(realpath ..)
endif
export OMCI_DIR
# try to load parameters specified in last mk_omcimain.sh execution, 
# so if user does 'make' in any subdir without calling mk_omcimain_tgz.sh, the same parameters would be used
-include $(OMCI_DIR)/src/mk_omcimain_tgz.sh.config

# uncomment to build omci for x86
#TOOLPREFIX=/usr/bin/

# uncomment to enable bind_mac check
#OMCI_BM_ENABLE=y

# uncomment to enable omci authoriation protect
#OMCI_AUTHORIZATION_PROTECT_ENABLE=y

### include platform config #################

#$(info TOOLPREFIX=$(TOOLPREFIX) OMCI_BM_ENABLE=$(OMCI_BM_ENABLE) OMCI_AUTHORIZATION_PROTECT_ENABLE=$(OMCI_AUTHORIZATION_PROTECT_ENABLE))

ifeq ($(TOOLPREFIX),/usr/bin/)
include $(OMCI_DIR)/makefile.inc.x86
else ifeq ($(CONFIG_TARGET_PRX126),y)
include $(OMCI_DIR)/makefile.inc.prx126
else ifeq ($(CONFIG_TARGET_PRX120),y)
include $(OMCI_DIR)/makefile.inc.prx120
else ifeq ($(CONFIG_TARGET_PRX321),y)
include $(OMCI_DIR)/makefile.inc.prx321
endif

### toolchain ###############################
#TOOLPREFIX?=arm-openwrt-linux-muslgnueabi-
CC=$(TOOLPREFIX)gcc
LD=$(TOOLPREFIX)ld
AR=$(TOOLPREFIX)ar
RANLIB=$(TOOLPREFIX)ranlib
STRIP=$(TOOLPREFIX)strip

### platform independent flag ###############
CFLAGS+= -O0
CFLAGS+= -g
#CFLAGS+= -static
#CFLAGS+= -DDEBUG

### omci include and lib ####################

# omci authorization protect support
ifeq ($(OMCI_AUTHORIZATION_PROTECT_ENABLE), y)
LOCAL_CFLAGS+= -DOMCI_AUTHORIZATION_PROTECT_ENABLE
endif

# bind_mac check support
ifeq ($(OMCI_BM_ENABLE), y)
LOCAL_CFLAGS+= -DOMCI_BM_ENABLE
endif

# required for pthread functions
LOCAL_CFLAGS+= -D_GNU_SOURCE

ifeq ($(CONFIG_TARGET_PRX126), y)
LOCAL_CFLAGS+= -DCONFIG_TARGET_PRX126 -Wno-format-truncation -Wno-stringop-overflow -Wno-sizeof-pointer-memaccess
else ifeq ($(CONFIG_TARGET_PRX120), y)
LOCAL_CFLAGS+= -DCONFIG_TARGET_PRX120 -Wno-format-truncation -Wno-stringop-overflow -Wno-sizeof-pointer-memaccess
else ifeq ($(CONFIG_TARGET_PRX321), y)
LOCAL_CFLAGS+= -DCONFIG_TARGET_PRX321 -Wno-format-truncation -Wno-stringop-overflow -Wno-sizeof-pointer-memaccess
endif

# treat warning as error, warn all, with some execeptions (these strict options are used for local code only)
LOCAL_CFLAGS+= -Werror -Wall -Wno-pointer-sign -Wno-int-to-pointer-cast -Wno-pointer-to-int-cast -Wno-format-security
#-Wno-unused-but-set-variable
-include $(OMCI_DIR)/makefile.omci.inc
LOCAL_CFLAGS+= $(OMCI_INCLUDE)

# since omci libs refer to each other, grouping is required for linking
LIBS+=	-Wl,--start-group $(OMCI_LIB) -Wl,--end-group
# libm for log10() which is used in mw<->db conversion
LIBS+= -lm
# librt for sys_clock_gettime(), sys uptime in usec precision
LIBS+= -lrt 
LIBS+= -lpthread
# bind_mac uses dlopen functions
ifeq ($(OMCI_BM_ENABLE), y)
LIBS+= -ldl
endif

### platform dependent ######################
CFLAGS+= $(PLATFORM_CFLAGS)
LIBS+= $(PLATFORM_LIBS)

### output msg ##############################
# show CFLAGS, LOCAL_CFLAGS whenever this file is included
#$(info $(MKNAME) CFLAGS=$(CFLAGS))
#$(info $(MKNAME) LOCAL_CFLAGS=$(LOCAL_CFLAGS))
#$(shell sleep 10)

### misc ####################################
SRC_FILES = $(wildcard *.c */*.c)
SRC_RELATED_OBJS = $(SRC_FILES:.c=.o)
ifneq ($(wildcard *.[co] */*.[co]),)
	REBUILD_LIBRARY=y
	SRC_RELATED_OBJS += $(wildcard *.a)
endif

CCNAME = $(shell basename $(CC))

%.o : %.c
#	$(CC) -c $(LOCAL_CFLAGS) $(CFLAGS) $(CPPFLAGS) $< -o $@
	@echo [$(CCNAME)] $< && $(CC) -c $(LOCAL_CFLAGS) $(CFLAGS) $(CPPFLAGS) $< -o $@
