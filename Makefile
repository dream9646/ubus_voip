-include ./rules.mk
-include ./common.mk

TARGET_CFLAGS ?= -I. -I$(INSTALL_INC_DIR) -Os -pipe -march=5281 -msoft-float
ARFLAGS ?= csr

UTILDIRS+=$(SRC_DIR)/memdbg

ifeq ($(BUILD_DETOUR_TABLE),1)
	UTILDIRS+=$(SRC_DIR)/detour_table
endif
ifeq ($(BUILD_F2CONSOLE),1)
	UTILDIRS+=$(SRC_DIR)/f2console
endif

all: util libvoiputils.a

default: all

clean: util_clean

libvoiputils.a:
	$(CROSS)ar $(TARGET_ARFLAGS) $@ $(SRC_DIR)/memdbg/*.o

dump:
	@echo TOPDIR=$(TOPDIR)
	@echo CROSS=$(CROSS)

#### build rule for src/packages ####

util:
	@mkdir -p $(INSTALL_INC_DIR)
	@for i in $(UTILDIRS); do \
		if [ -f $$i/Makefile ]; then \
			$(MAKE) -C $$i $(TARGET_CONFIGURE_OPTS) CFLAGS="$(TARGET_CFLAGS) -I."; \
		fi; \
	done

util_clean:
	@$(shell [ -d $(INSTALL_INC_DIR) ] && find $(INSTALL_INC_DIR) -name "*.h" -exec rm -f {} \;)
	-rm -f libvoiputils.a
	@for i in $(UTILDIRS); do \
		if [ -f $$i/Makefile ]; then \
			$(MAKE) -C $$i clean; \
		fi; \
	done

