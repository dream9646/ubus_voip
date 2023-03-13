# include and lib definition shared by all omci modules
ifeq ($(OMCI_DIR),)
 $(error OMCI_DIR is not defined)
endif

OMCI_INCLUDE+= \
	-I$(OMCI_DIR)/cli/ \
	-I$(OMCI_DIR)/config/ \
	-I$(OMCI_DIR)/meinfo/ \
	-I$(OMCI_DIR)/meinfo_cb/ \
	-I$(OMCI_DIR)/me_related/ \
	-I$(OMCI_DIR)/fwk/ \
	-I$(OMCI_DIR)/notifychain/ \
	-I$(OMCI_DIR)/notifychain_cb/ \
	-I$(OMCI_DIR)/omciutil/ \
	-I$(OMCI_DIR)/omcidump/ \
	-I$(OMCI_DIR)/omcimsg/ \
	-I$(OMCI_DIR)/tasks/ \
	-I$(OMCI_DIR)/util/ \
	-I$(OMCI_DIR)/xmlparse/ \
	-I$(OMCI_DIR)/src/
OMCI_INCLUDE+= \
	-I$(OMCI_DIR)/hwresource/ \
	-I$(OMCI_DIR)/er_group/ \
	-I$(OMCI_DIR)/batchtab/ \
	-I$(OMCI_DIR)/cpuport/ \
	-I$(OMCI_DIR)/gpon/ \
	-I$(OMCI_DIR)/switch/ \
	-I$(OMCI_DIR)/acl/ \
	-I$(OMCI_DIR)/tm/ \
	-I$(OMCI_DIR)/classf/
OMCI_INCLUDE+= \
	-I$(OMCI_DIR)/cpuport_arp/ \
	-I$(OMCI_DIR)/cpuport_dhcp/ \
	-I$(OMCI_DIR)/cpuport_pppoe/ \
	-I$(OMCI_DIR)/igmp/ \
	-I$(OMCI_DIR)/cfm/ \
	-I$(OMCI_DIR)/lldp/ \
	-I$(OMCI_DIR)/rstplib/ \
	-I$(OMCI_DIR)/voip/ \
	-I$(OMCI_DIR)/extoam/ \
	-I$(OMCI_DIR)/meta_cfg/ \
	$(shell for d in $(OMCI_DIR)/proprietary_*; do echo -I$$d; done)

OMCI_LIB+= \
	$(OMCI_DIR)/cli/cli.a \
	$(OMCI_DIR)/config/omci_config.a \
	$(OMCI_DIR)/meinfo/meinfo.a \
	$(OMCI_DIR)/meinfo_cb/meinfo_cb.a \
	$(OMCI_DIR)/me_related/me_related.a \
	$(OMCI_DIR)/fwk/fwk.a \
	$(OMCI_DIR)/notifychain/notify_chain.a \
	$(OMCI_DIR)/notifychain_cb/notify_chain_cb.a \
	$(OMCI_DIR)/omciutil/omciutil.a \
	$(OMCI_DIR)/omcidump/omcidump.a \
	$(OMCI_DIR)/omcimsg/omcimsg.a \
	$(OMCI_DIR)/tasks/tasks.a \
	$(OMCI_DIR)/util/util.a \
	$(OMCI_DIR)/extoam/extoam.a \
	$(OMCI_DIR)/xmlparse/xmlparse.a \
	$(OMCI_DIR)/src/omcimain.a
OMCI_LIB+= \
	$(OMCI_DIR)/hwresource/hwresource.a \
	$(OMCI_DIR)/er_group/er_group.a \
	$(OMCI_DIR)/batchtab/batchtab.a \
	$(OMCI_DIR)/acl/acl.a \
	$(OMCI_DIR)/tm/tm.a \
	$(OMCI_DIR)/cpuport/cpuport.a \
	$(OMCI_DIR)/gpon/gpon.a \
	$(OMCI_DIR)/switch/switch.a \
	$(OMCI_DIR)/classf/classf.a 
OMCI_LIB+= \
	$(OMCI_DIR)/cpuport_arp/cpuport_arp.a \
	$(OMCI_DIR)/cpuport_dhcp/cpuport_dhcp.a \
	$(OMCI_DIR)/cpuport_pppoe/cpuport_pppoe.a \
	$(OMCI_DIR)/igmp/igmp_proxy.a \
	$(OMCI_DIR)/cfm/cfm.a \
	$(OMCI_DIR)/lldp/lldp.a \
	$(OMCI_DIR)/rstplib/rstp.a \
	$(OMCI_DIR)/voip/voip.a \
	$(OMCI_DIR)/meta_cfg/metacfg.a \
	$(shell for d in $(OMCI_DIR)/proprietary_*; do echo "$$d/$$(basename $$d).a"; done)

ifneq ($(TOOLPREFIX), /usr/bin/)
# hw related omci code
OMCI_INCLUDE+= \
	-I$(OMCI_DIR)/meinfo_hw/ \
	-I$(OMCI_DIR)/er_group_hw/ \
	-I$(OMCI_DIR)/voip_hw/ \
	-I$(OMCI_DIR)/voipclient/
OMCI_LIB+= \
	$(OMCI_DIR)/meinfo_hw/meinfo_hw.a \
	$(OMCI_DIR)/er_group_hw/er_group_hw.a \
	$(OMCI_DIR)/voip_hw/voip_hw.a \
	$(OMCI_DIR)/voipclient/voipclient.a
#	$(OMCI_DIR)/meinfo_hw/libm02098.a \

ifneq ($(CONFIG_TARGET_PRX126),)
OMCI_INCLUDE+= \
	-I$(OMCI_DIR)/gpon_hw_prx126/ \
	-I$(OMCI_DIR)/cpuport_hw_prx126/ \
	-I$(OMCI_DIR)/switch_hw_prx126/ \
	-I$(OMCI_DIR)/intel/  \
	-I$(STAGING_DIR)/usr/include/pon_adapter
OMCI_LIB+= \
	$(OMCI_DIR)/switch_hw_prx126/switch_hw_prx126.a \
	$(OMCI_DIR)/cpuport_hw_prx126/cpuport_hw_prx126.a \
	$(OMCI_DIR)/gpon_hw_prx126/gpon_hw_prx126.a \
	$(OMCI_DIR)/intel/intel_px126_util.a
endif

ifneq ($(CONFIG_TARGET_PRX120),)
OMCI_INCLUDE+= \
	-I$(OMCI_DIR)/gpon_hw_prx126/ \
	-I$(OMCI_DIR)/cpuport_hw_prx126/ \
	-I$(OMCI_DIR)/switch_hw_prx126/ \
	-I$(OMCI_DIR)/intel/  \
	-I$(STAGING_DIR)/usr/include/pon_adapter
OMCI_LIB+= \
	$(OMCI_DIR)/switch_hw_prx126/switch_hw_prx126.a \
	$(OMCI_DIR)/cpuport_hw_prx126/cpuport_hw_prx126.a \
	$(OMCI_DIR)/gpon_hw_prx126/gpon_hw_prx126.a \
	$(OMCI_DIR)/intel/intel_px126_util.a
endif

ifneq ($(CONFIG_TARGET_PRX321),)
OMCI_INCLUDE+= \
	-I$(OMCI_DIR)/gpon_hw_prx126/ \
	-I$(OMCI_DIR)/cpuport_hw_prx126/ \
	-I$(OMCI_DIR)/switch_hw_prx126/ \
	-I$(OMCI_DIR)/intel/  \
	-I$(STAGING_DIR)/usr/include/pon_adapter
OMCI_LIB+= \
	$(OMCI_DIR)/switch_hw_prx126/switch_hw_prx126.a \
	$(OMCI_DIR)/cpuport_hw_prx126/cpuport_hw_prx126.a \
	$(OMCI_DIR)/gpon_hw_prx126/gpon_hw_prx126.a \
	$(OMCI_DIR)/intel/intel_px126_util.a
endif

endif
