CROSS ?= /opt/crosstool/rsdk-1.5.6-5281-EB-2.6.30-0.9.30.3-110915/bin/mips-linux-

TARGET_CFLAGS ?= -I. -I$(INSTALL_INC_DIR) -Os -pipe -march=5281 -msoft-float
TARGET_ARFLAGS ?= csr

TARGET_CC:=$(CROSS)gcc
TARGET_CXX:=$(if $(CONFIG_INSTALL_LIBSTDCPP),$(CROSS)g++,no)

SED:=sed -i -e
CP:=cp -fpR
LN:=ln -sf

INSTALL_BIN:=install -m0755
INSTALL_DIR:=install -d -m0755
INSTALL_DATA:=install -m0644
INSTALL_CONF:=install -m0600

TARGET_CONFIGURE_OPTS = \
  AR=$(CROSS)ar \
  AS="$(TARGET_CC) -c $(TARGET_CFLAGS)" \
  LD=$(CROSS)ld \
  NM=$(CROSS)nm \
  CC="$(TARGET_CC)" \
  GCC="$(TARGET_CC)" \
  CXX="$(TARGET_CXX)" \
  RANLIB=$(CROSS)ranlib \
  STRIP=$(CROSS)strip \
  OBJCOPY=$(CROSS)objcopy \
  OBJDUMP=$(CROSS)objdump \
  SIZE=$(CROSS)size
