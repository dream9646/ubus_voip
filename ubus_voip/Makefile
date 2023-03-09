#
# Copyright (C) 2013-2016 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=util_voip
PKG_RELEASE:=mark

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)
PKG_INSTALL_DIR:=$(PKG_BUILD_DIR)/ipkg-install

PKG_BUILD_PARALLEL:=1

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

#define Build/InstallDev
#	$(INSTALL_DIR) $(1)/usr/include
#	$(CP) $(PKG_BUILD_DIR)/include/rpcd $(1)/usr/include/
#endef

define Package/$(PKG_NAME)/default
  CATEGORY:=5VT Packages
  TITLE:=util_voip
  DEPENDS:=+libubus +libubox
endef

define Package/$(PKG_NAME)
  $(Package/$(PKG_NAME)/default)
  DEPENDS+= +libuci +libblobmsg-json +libjson-c
endef

define Package/$(PKG_NAME)/description
 util_ubus
endef

#define Package/$(PKG_NAME)/conffiles
#/etc/config/voip
#endef

TARGET_LDFLAGS += -lcrypt

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/fvt_evoip.init $(1)/etc/init.d/fvt_evoip
	$(INSTALL_DIR) $(1)/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/fvt_evoip $(1)/bin/fvt_evoip
	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_CONF) ./files/fvt_evoip.config $(1)/etc/config/fvt_evoip
	$(INSTALL_DIR) $(1)/etc/voip
	$(INSTALL_CONF) ./files/fv_voip_uci.config $(1)/etc/voip/fv_voip_uci.config
	$(INSTALL_DATA) ./files/cmd_list $(1)/etc/voip/cmd_list
endef
define Package/$(PKG_NAME)/init
    /etc/init.d/fvt_evoip start
endef


$(eval $(call BuildPackage,$(PKG_NAME)))
