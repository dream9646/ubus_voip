#!/usr/bin/perl
while (<>) {
	$line=$_;
	$line =~ s/rtk_stp_init *\(/rtk_rg_stp_init\(/g;
	$line =~ s/rtk_stp_mstpState_get *\(/rtk_rg_stp_mstpState_get\(/g;
	$line =~ s/rtk_stp_mstpState_set *\(/rtk_rg_stp_mstpState_set\(/g;
	$line =~ s/rtk_gpon_activate *\(/rtk_rg_gpon_activate\(/g;
	$line =~ s/rtk_gpon_aesKeySwitch_get *\(/rtk_rg_gpon_aesKeySwitch_get\(/g;
	$line =~ s/rtk_gpon_alarmStatus_get *\(/rtk_rg_gpon_alarmStatus_get\(/g;
	$line =~ s/rtk_gpon_autoBoh_get *\(/rtk_rg_gpon_autoBoh_get\(/g;
	$line =~ s/rtk_gpon_autoBoh_set *\(/rtk_rg_gpon_autoBoh_set\(/g;
	$line =~ s/rtk_gpon_autoTcont_get *\(/rtk_rg_gpon_autoTcont_get\(/g;
	$line =~ s/rtk_gpon_autoTcont_set *\(/rtk_rg_gpon_autoTcont_set\(/g;
	$line =~ s/rtk_gpon_berInterval_get *\(/rtk_rg_gpon_berInterval_get\(/g;
	$line =~ s/rtk_gpon_broadcastPass_get *\(/rtk_rg_gpon_broadcastPass_get\(/g;
	$line =~ s/rtk_gpon_broadcastPass_set *\(/rtk_rg_gpon_broadcastPass_set\(/g;
	$line =~ s/rtk_gpon_callbackExtMsgGetHandle_reg *\(/rtk_rg_gpon_callbackExtMsgGetHandle_reg\(/g;
	$line =~ s/rtk_gpon_callbackExtMsgSetHandle_reg *\(/rtk_rg_gpon_callbackExtMsgSetHandle_reg\(/g;
	$line =~ s/rtk_gpon_callbackQueryAesKey_reg *\(/rtk_rg_gpon_callbackQueryAesKey_reg\(/g;
	$line =~ s/rtk_gpon_deActivate *\(/rtk_rg_gpon_deActivate\(/g;
	$line =~ s/rtk_gpon_debug_set *\(/rtk_rg_gpon_debug_set\(/g;
	$line =~ s/rtk_gpon_deinitial *\(/rtk_rg_gpon_deinitial\(/g;
	$line =~ s/rtk_gpon_device_deInitialize *\(/rtk_rg_gpon_device_deInitialize\(/g;
	$line =~ s/rtk_gpon_device_initialize *\(/rtk_rg_gpon_device_initialize\(/g;
	$line =~ s/rtk_gpon_devInfo_show *\(/rtk_rg_gpon_devInfo_show\(/g;
	$line =~ s/rtk_gpon_driver_deInitialize *\(/rtk_rg_gpon_driver_deInitialize\(/g;
	$line =~ s/rtk_gpon_driver_initialize *\(/rtk_rg_gpon_driver_initialize\(/g;
	$line =~ s/rtk_gpon_dsFecSts_get *\(/rtk_rg_gpon_dsFecSts_get\(/g;
	$line =~ s/rtk_gpon_dsFlow_get *\(/rtk_rg_gpon_dsFlow_get\(/g;
	$line =~ s/rtk_gpon_dsFlow_set *\(/rtk_rg_gpon_dsFlow_set\(/g;
	$line =~ s/rtk_gpon_dsFlow_show *\(/rtk_rg_gpon_dsFlow_show\(/g;
	$line =~ s/rtk_gpon_eqdOffset_get *\(/rtk_rg_gpon_eqdOffset_get\(/g;
	$line =~ s/rtk_gpon_eqdOffset_set *\(/rtk_rg_gpon_eqdOffset_set\(/g;
	$line =~ s/rtk_gpon_evtHdlAlarm_reg *\(/rtk_rg_gpon_evtHdlAlarm_reg\(/g;
	$line =~ s/rtk_gpon_evtHdlDsFecChange_reg *\(/rtk_rg_gpon_evtHdlDsFecChange_reg\(/g;
	$line =~ s/rtk_gpon_evtHdlOmci_reg *\(/rtk_rg_gpon_evtHdlOmci_reg\(/g;
	$line =~ s/rtk_gpon_evtHdlPloam_dreg *\(/rtk_rg_gpon_evtHdlPloam_dreg\(/g;
	$line =~ s/rtk_gpon_evtHdlPloam_reg *\(/rtk_rg_gpon_evtHdlPloam_reg\(/g;
	$line =~ s/rtk_gpon_evtHdlStateChange_reg *\(/rtk_rg_gpon_evtHdlStateChange_reg\(/g;
	$line =~ s/rtk_gpon_evtHdlUsFecChange_reg *\(/rtk_rg_gpon_evtHdlUsFecChange_reg\(/g;
	$line =~ s/rtk_gpon_evtHdlUsPloamNrmEmpty_reg *\(/rtk_rg_gpon_evtHdlUsPloamNrmEmpty_reg\(/g;
	$line =~ s/rtk_gpon_evtHdlUsPloamUrgEmpty_reg *\(/rtk_rg_gpon_evtHdlUsPloamUrgEmpty_reg\(/g;
	$line =~ s/rtk_gpon_extMsg_get *\(/rtk_rg_gpon_extMsg_get\(/g;
	$line =~ s/rtk_gpon_extMsg_set *\(/rtk_rg_gpon_extMsg_set\(/g;
	$line =~ s/rtk_gpon_flowCounter_get *\(/rtk_rg_gpon_flowCounter_get\(/g;
	$line =~ s/rtk_gpon_flowCounter_show *\(/rtk_rg_gpon_flowCounter_show\(/g;
	$line =~ s/rtk_gpon_globalCounter_get *\(/rtk_rg_gpon_globalCounter_get\(/g;
	$line =~ s/rtk_gpon_globalCounter_show *\(/rtk_rg_gpon_globalCounter_show\(/g;
	$line =~ s/rtk_gpon_gtc_show *\(/rtk_rg_gpon_gtc_show\(/g;
	$line =~ s/rtk_gpon_initial *\(/rtk_rg_gpon_initial\(/g;
	$line =~ s/rtk_gpon_isr_entry *\(/rtk_rg_gpon_isr_entry\(/g;
	$line =~ s/rtk_gpon_macEntry_add *\(/rtk_rg_gpon_macEntry_add\(/g;
	$line =~ s/rtk_gpon_macEntry_del *\(/rtk_rg_gpon_macEntry_del\(/g;
	$line =~ s/rtk_gpon_macEntry_get *\(/rtk_rg_gpon_macEntry_get\(/g;
	$line =~ s/rtk_gpon_macFilterMode_get *\(/rtk_rg_gpon_macFilterMode_get\(/g;
	$line =~ s/rtk_gpon_macFilterMode_set *\(/rtk_rg_gpon_macFilterMode_set\(/g;
	$line =~ s/rtk_gpon_macTable_show *\(/rtk_rg_gpon_macTable_show\(/g;
	$line =~ s/rtk_gpon_mcForceMode_get *\(/rtk_rg_gpon_mcForceMode_get\(/g;
	$line =~ s/rtk_gpon_mcForceMode_set *\(/rtk_rg_gpon_mcForceMode_set\(/g;
	$line =~ s/rtk_gpon_multicastAddrCheck_get *\(/rtk_rg_gpon_multicastAddrCheck_get\(/g;
	$line =~ s/rtk_gpon_multicastAddrCheck_set *\(/rtk_rg_gpon_multicastAddrCheck_set\(/g;
	$line =~ s/rtk_gpon_nonMcastPass_get *\(/rtk_rg_gpon_nonMcastPass_get\(/g;
	$line =~ s/rtk_gpon_nonMcastPass_set *\(/rtk_rg_gpon_nonMcastPass_set\(/g;
	$line =~ s/rtk_gpon_omci_rx *\(/rtk_rg_gpon_omci_rx\(/g;
	$line =~ s/rtk_gpon_omci_tx *\(/rtk_rg_gpon_omci_tx\(/g;
	$line =~ s/rtk_gpon_parameter_get *\(/rtk_rg_gpon_parameter_get\(/g;
	$line =~ s/rtk_gpon_parameter_set *\(/rtk_rg_gpon_parameter_set\(/g;
	$line =~ s/rtk_gpon_password_get *\(/rtk_rg_gpon_password_get\(/g;
	$line =~ s/rtk_gpon_password_set *\(/rtk_rg_gpon_password_set\(/g;
	$line =~ s/rtk_gpon_ploam_send *\(/rtk_rg_gpon_ploam_send\(/g;
	$line =~ s/rtk_gpon_ponStatus_get *\(/rtk_rg_gpon_ponStatus_get\(/g;
	$line =~ s/rtk_gpon_powerLevel_get *\(/rtk_rg_gpon_powerLevel_get\(/g;
	$line =~ s/rtk_gpon_powerLevel_set *\(/rtk_rg_gpon_powerLevel_set\(/g;
	$line =~ s/rtk_gpon_rdi_get *\(/rtk_rg_gpon_rdi_get\(/g;
	$line =~ s/rtk_gpon_rdi_set *\(/rtk_rg_gpon_rdi_set\(/g;
	$line =~ s/rtk_gpon_serialNumber_get *\(/rtk_rg_gpon_serialNumber_get\(/g;
	$line =~ s/rtk_gpon_serialNumber_set *\(/rtk_rg_gpon_serialNumber_set\(/g;
	$line =~ s/rtk_gpon_tcontCounter_get *\(/rtk_rg_gpon_tcontCounter_get\(/g;
	$line =~ s/rtk_gpon_tcontCounter_show *\(/rtk_rg_gpon_tcontCounter_show\(/g;
	$line =~ s/rtk_gpon_tcont_create *\(/rtk_rg_gpon_tcont_create\(/g;
	$line =~ s/rtk_gpon_tcont_destroy *\(/rtk_rg_gpon_tcont_destroy\(/g;
	$line =~ s/rtk_gpon_tcont_get *\(/rtk_rg_gpon_tcont_get\(/g;
	$line =~ s/rtk_gpon_tcont_show *\(/rtk_rg_gpon_tcont_show\(/g;
	$line =~ s/rtk_gpon_txForceIdle_get *\(/rtk_rg_gpon_txForceIdle_get\(/g;
	$line =~ s/rtk_gpon_txForceIdle_set *\(/rtk_rg_gpon_txForceIdle_set\(/g;
	$line =~ s/rtk_gpon_txForceLaser_get *\(/rtk_rg_gpon_txForceLaser_get\(/g;
	$line =~ s/rtk_gpon_txForceLaser_set *\(/rtk_rg_gpon_txForceLaser_set\(/g;
	$line =~ s/rtk_gpon_unit_test *\(/rtk_rg_gpon_unit_test\(/g;
	$line =~ s/rtk_gpon_usFlow_get *\(/rtk_rg_gpon_usFlow_get\(/g;
	$line =~ s/rtk_gpon_usFlow_set *\(/rtk_rg_gpon_usFlow_set\(/g;
	$line =~ s/rtk_gpon_usFlow_show *\(/rtk_rg_gpon_usFlow_show\(/g;
	$line =~ s/rtk_gpon_version_get *\(/rtk_rg_gpon_version_get\(/g;
	$line =~ s/rtk_gpon_version_show *\(/rtk_rg_gpon_version_show\(/g;
	$line =~ s/rtk_epon_churningKey_get *\(/rtk_rg_epon_churningKey_get\(/g;
	$line =~ s/rtk_epon_churningKey_set *\(/rtk_rg_epon_churningKey_set\(/g;
	$line =~ s/rtk_epon_dsFecState_get *\(/rtk_rg_epon_dsFecState_get\(/g;
	$line =~ s/rtk_epon_dsFecState_set *\(/rtk_rg_epon_dsFecState_set\(/g;
	$line =~ s/rtk_epon_fecState_get *\(/rtk_rg_epon_fecState_get\(/g;
	$line =~ s/rtk_epon_fecState_set *\(/rtk_rg_epon_fecState_set\(/g;
	$line =~ s/rtk_epon_forceLaserState_get *\(/rtk_rg_epon_forceLaserState_get\(/g;
	$line =~ s/rtk_epon_forceLaserState_set *\(/rtk_rg_epon_forceLaserState_set\(/g;
	$line =~ s/rtk_epon_init *\(/rtk_rg_epon_init\(/g;
	$line =~ s/rtk_epon_intrMask_get *\(/rtk_rg_epon_intrMask_get\(/g;
	$line =~ s/rtk_epon_intrMask_set *\(/rtk_rg_epon_intrMask_set\(/g;
	$line =~ s/rtk_epon_intr_disableAll *\(/rtk_rg_epon_intr_disableAll\(/g;
	$line =~ s/rtk_epon_intr_get *\(/rtk_rg_epon_intr_get\(/g;
	$line =~ s/rtk_epon_laserTime_get *\(/rtk_rg_epon_laserTime_get\(/g;
	$line =~ s/rtk_epon_laserTime_set *\(/rtk_rg_epon_laserTime_set\(/g;
	$line =~ s/rtk_epon_llidEntryNum_get *\(/rtk_rg_epon_llidEntryNum_get\(/g;
	$line =~ s/rtk_epon_llid_entry_get *\(/rtk_rg_epon_llid_entry_get\(/g;
	$line =~ s/rtk_epon_llid_entry_set *\(/rtk_rg_epon_llid_entry_set\(/g;
	$line =~ s/rtk_epon_losState_get *\(/rtk_rg_epon_losState_get\(/g;
	$line =~ s/rtk_epon_mibCounter_get *\(/rtk_rg_epon_mibCounter_get\(/g;
	$line =~ s/rtk_epon_mibGlobal_reset *\(/rtk_rg_epon_mibGlobal_reset\(/g;
	$line =~ s/rtk_epon_mibLlidIdx_reset *\(/rtk_rg_epon_mibLlidIdx_reset\(/g;
	$line =~ s/rtk_epon_mpcpTimeoutVal_get *\(/rtk_rg_epon_mpcpTimeoutVal_get\(/g;
	$line =~ s/rtk_epon_mpcpTimeoutVal_set *\(/rtk_rg_epon_mpcpTimeoutVal_set\(/g;
	$line =~ s/rtk_epon_opticalPolarity_get *\(/rtk_rg_epon_opticalPolarity_get\(/g;
	$line =~ s/rtk_epon_opticalPolarity_set *\(/rtk_rg_epon_opticalPolarity_set\(/g;
	$line =~ s/rtk_epon_registerReq_get *\(/rtk_rg_epon_registerReq_get\(/g;
	$line =~ s/rtk_epon_registerReq_set *\(/rtk_rg_epon_registerReq_set\(/g;
	$line =~ s/rtk_epon_syncTime_get *\(/rtk_rg_epon_syncTime_get\(/g;
	$line =~ s/rtk_epon_thresholdReport_get *\(/rtk_rg_epon_thresholdReport_get\(/g;
	$line =~ s/rtk_epon_thresholdReport_set *\(/rtk_rg_epon_thresholdReport_set\(/g;
	$line =~ s/rtk_epon_usFecState_get *\(/rtk_rg_epon_usFecState_get\(/g;
	$line =~ s/rtk_epon_usFecState_set *\(/rtk_rg_epon_usFecState_set\(/g;
	$line =~ s/rtk_i2c_clock_get *\(/rtk_rg_i2c_clock_get\(/g;
	$line =~ s/rtk_i2c_clock_set *\(/rtk_rg_i2c_clock_set\(/g;
	$line =~ s/rtk_i2c_eepMirror_get *\(/rtk_rg_i2c_eepMirror_get\(/g;
	$line =~ s/rtk_i2c_eepMirror_read *\(/rtk_rg_i2c_eepMirror_read\(/g;
	$line =~ s/rtk_i2c_eepMirror_set *\(/rtk_rg_i2c_eepMirror_set\(/g;
	$line =~ s/rtk_i2c_eepMirror_write *\(/rtk_rg_i2c_eepMirror_write\(/g;
	$line =~ s/rtk_i2c_enable_get *\(/rtk_rg_i2c_enable_get\(/g;
	$line =~ s/rtk_i2c_enable_set *\(/rtk_rg_i2c_enable_set\(/g;
	$line =~ s/rtk_i2c_init *\(/rtk_rg_i2c_init\(/g;
	$line =~ s/rtk_i2c_read *\(/rtk_rg_i2c_read\(/g;
	$line =~ s/rtk_i2c_width_get *\(/rtk_rg_i2c_width_get\(/g;
	$line =~ s/rtk_i2c_width_set *\(/rtk_rg_i2c_width_set\(/g;
	$line =~ s/rtk_i2c_write *\(/rtk_rg_i2c_write\(/g;
	$line =~ s/rtk_intr_gphyStatus_clear *\(/rtk_rg_intr_gphyStatus_clear\(/g;
	$line =~ s/rtk_intr_gphyStatus_get *\(/rtk_rg_intr_gphyStatus_get\(/g;
	$line =~ s/rtk_intr_imr_get *\(/rtk_rg_intr_imr_get\(/g;
	$line =~ s/rtk_intr_imr_restore *\(/rtk_rg_intr_imr_restore\(/g;
	$line =~ s/rtk_intr_imr_set *\(/rtk_rg_intr_imr_set\(/g;
	$line =~ s/rtk_intr_ims_clear *\(/rtk_rg_intr_ims_clear\(/g;
	$line =~ s/rtk_intr_ims_get *\(/rtk_rg_intr_ims_get\(/g;
	$line =~ s/rtk_intr_init *\(/rtk_rg_intr_init\(/g;
	$line =~ s/rtk_intr_linkdownStatus_clear *\(/rtk_rg_intr_linkdownStatus_clear\(/g;
	$line =~ s/rtk_intr_linkdownStatus_get *\(/rtk_rg_intr_linkdownStatus_get\(/g;
	$line =~ s/rtk_intr_linkupStatus_clear *\(/rtk_rg_intr_linkupStatus_clear\(/g;
	$line =~ s/rtk_intr_linkupStatus_get *\(/rtk_rg_intr_linkupStatus_get\(/g;
	$line =~ s/rtk_intr_polarity_get *\(/rtk_rg_intr_polarity_get\(/g;
	$line =~ s/rtk_intr_polarity_set *\(/rtk_rg_intr_polarity_set\(/g;
	$line =~ s/rtk_intr_speedChangeStatus_clear *\(/rtk_rg_intr_speedChangeStatus_clear\(/g;
	$line =~ s/rtk_intr_speedChangeStatus_get *\(/rtk_rg_intr_speedChangeStatus_get\(/g;
	$line =~ s/rtk_ponmac_flow2Queue_get *\(/rtk_rg_ponmac_flow2Queue_get\(/g;
	$line =~ s/rtk_ponmac_flow2Queue_set *\(/rtk_rg_ponmac_flow2Queue_set\(/g;
	$line =~ s/rtk_ponmac_init *\(/rtk_rg_ponmac_init\(/g;
	$line =~ s/rtk_ponmac_losState_get *\(/rtk_rg_ponmac_losState_get\(/g;
	$line =~ s/rtk_ponmac_mode_get *\(/rtk_rg_ponmac_mode_get\(/g;
	$line =~ s/rtk_ponmac_mode_set *\(/rtk_rg_ponmac_mode_set\(/g;
	$line =~ s/rtk_ponmac_opticalPolarity_get *\(/rtk_rg_ponmac_opticalPolarity_get\(/g;
	$line =~ s/rtk_ponmac_opticalPolarity_set *\(/rtk_rg_ponmac_opticalPolarity_set\(/g;
	$line =~ s/rtk_ponmac_queueDrainOut_set *\(/rtk_rg_ponmac_queueDrainOut_set\(/g;
	$line =~ s/rtk_ponmac_queue_add *\(/rtk_rg_ponmac_queue_add\(/g;
	$line =~ s/rtk_ponmac_queue_del *\(/rtk_rg_ponmac_queue_del\(/g;
	$line =~ s/rtk_ponmac_queue_get *\(/rtk_rg_ponmac_queue_get\(/g;
	$line =~ s/rtk_ponmac_transceiver_get *\(/rtk_rg_ponmac_transceiver_get\(/g;
	$line =~ s/rtk_port_adminEnable_get *\(/rtk_rg_port_adminEnable_get\(/g;
	$line =~ s/rtk_port_adminEnable_set *\(/rtk_rg_port_adminEnable_set\(/g;
	$line =~ s/rtk_port_cpuPortId_get *\(/rtk_rg_port_cpuPortId_get\(/g;
	$line =~ s/rtk_port_enhancedFid_get *\(/rtk_rg_port_enhancedFid_get\(/g;
	$line =~ s/rtk_port_enhancedFid_set *\(/rtk_rg_port_enhancedFid_set\(/g;
	$line =~ s/rtk_port_flowctrl_get *\(/rtk_rg_port_flowctrl_get\(/g;
	$line =~ s/rtk_port_gigaLiteEnable_get *\(/rtk_rg_port_gigaLiteEnable_get\(/g;
	$line =~ s/rtk_port_gigaLiteEnable_set *\(/rtk_rg_port_gigaLiteEnable_set\(/g;
	$line =~ s/rtk_port_greenEnable_get *\(/rtk_rg_port_greenEnable_get\(/g;
	$line =~ s/rtk_port_greenEnable_set *\(/rtk_rg_port_greenEnable_set\(/g;
	$line =~ s/rtk_port_init *\(/rtk_rg_port_init\(/g;
	$line =~ s/rtk_port_isolationCtagPktConfig_get *\(/rtk_rg_port_isolationCtagPktConfig_get\(/g;
	$line =~ s/rtk_port_isolationCtagPktConfig_set *\(/rtk_rg_port_isolationCtagPktConfig_set\(/g;
	$line =~ s/rtk_port_isolationEntryExt_get *\(/rtk_rg_port_isolationEntryExt_get\(/g;
	$line =~ s/rtk_port_isolationEntryExt_set *\(/rtk_rg_port_isolationEntryExt_set\(/g;
	$line =~ s/rtk_port_isolationEntry_get *\(/rtk_rg_port_isolationEntry_get\(/g;
	$line =~ s/rtk_port_isolationEntry_set *\(/rtk_rg_port_isolationEntry_set\(/g;
	$line =~ s/rtk_port_isolationExtL34_get *\(/rtk_rg_port_isolationExtL34_get\(/g;
	$line =~ s/rtk_port_isolationExtL34_set *\(/rtk_rg_port_isolationExtL34_set\(/g;
	$line =~ s/rtk_port_isolationExt_get *\(/rtk_rg_port_isolationExt_get\(/g;
	$line =~ s/rtk_port_isolationExt_set *\(/rtk_rg_port_isolationExt_set\(/g;
	$line =~ s/rtk_port_isolationIpmcLeaky_get *\(/rtk_rg_port_isolationIpmcLeaky_get\(/g;
	$line =~ s/rtk_port_isolationIpmcLeaky_set *\(/rtk_rg_port_isolationIpmcLeaky_set\(/g;
	$line =~ s/rtk_port_isolationL34PktConfig_get *\(/rtk_rg_port_isolationL34PktConfig_get\(/g;
	$line =~ s/rtk_port_isolationL34PktConfig_set *\(/rtk_rg_port_isolationL34PktConfig_set\(/g;
	$line =~ s/rtk_port_isolationL34_get *\(/rtk_rg_port_isolationL34_get\(/g;
	$line =~ s/rtk_port_isolationL34_set *\(/rtk_rg_port_isolationL34_set\(/g;
	$line =~ s/rtk_port_isolationLeaky_get *\(/rtk_rg_port_isolationLeaky_get\(/g;
	$line =~ s/rtk_port_isolationLeaky_set *\(/rtk_rg_port_isolationLeaky_set\(/g;
	$line =~ s/rtk_port_isolationPortLeaky_get *\(/rtk_rg_port_isolationPortLeaky_get\(/g;
	$line =~ s/rtk_port_isolationPortLeaky_set *\(/rtk_rg_port_isolationPortLeaky_set\(/g;
	$line =~ s/rtk_port_isolation_get *\(/rtk_rg_port_isolation_get\(/g;
	$line =~ s/rtk_port_isolation_set *\(/rtk_rg_port_isolation_set\(/g;
	$line =~ s/rtk_port_link_get *\(/rtk_rg_port_link_get\(/g;
	$line =~ s/rtk_port_macExtMode_get *\(/rtk_rg_port_macExtMode_get\(/g;
	$line =~ s/rtk_port_macExtMode_set *\(/rtk_rg_port_macExtMode_set\(/g;
	$line =~ s/rtk_port_macExtRgmiiDelay_get *\(/rtk_rg_port_macExtRgmiiDelay_get\(/g;
	$line =~ s/rtk_port_macExtRgmiiDelay_set *\(/rtk_rg_port_macExtRgmiiDelay_set\(/g;
	$line =~ s/rtk_port_macForceAbilityState_get *\(/rtk_rg_port_macForceAbilityState_get\(/g;
	$line =~ s/rtk_port_macForceAbilityState_set *\(/rtk_rg_port_macForceAbilityState_set\(/g;
	$line =~ s/rtk_port_macForceAbility_get *\(/rtk_rg_port_macForceAbility_get\(/g;
	$line =~ s/rtk_port_macForceAbility_set *\(/rtk_rg_port_macForceAbility_set\(/g;
	$line =~ s/rtk_port_macLocalLoopbackEnable_get *\(/rtk_rg_port_macLocalLoopbackEnable_get\(/g;
	$line =~ s/rtk_port_macLocalLoopbackEnable_set *\(/rtk_rg_port_macLocalLoopbackEnable_set\(/g;
	$line =~ s/rtk_port_macRemoteLoopbackEnable_get *\(/rtk_rg_port_macRemoteLoopbackEnable_get\(/g;
	$line =~ s/rtk_port_macRemoteLoopbackEnable_set *\(/rtk_rg_port_macRemoteLoopbackEnable_set\(/g;
	$line =~ s/rtk_port_phyAutoNegoAbility_get *\(/rtk_rg_port_phyAutoNegoAbility_get\(/g;
	$line =~ s/rtk_port_phyAutoNegoAbility_set *\(/rtk_rg_port_phyAutoNegoAbility_set\(/g;
	$line =~ s/rtk_port_phyAutoNegoEnable_get *\(/rtk_rg_port_phyAutoNegoEnable_get\(/g;
	$line =~ s/rtk_port_phyAutoNegoEnable_set *\(/rtk_rg_port_phyAutoNegoEnable_set\(/g;
	$line =~ s/rtk_port_phyCrossOverMode_get *\(/rtk_rg_port_phyCrossOverMode_get\(/g;
	$line =~ s/rtk_port_phyCrossOverMode_set *\(/rtk_rg_port_phyCrossOverMode_set\(/g;
	$line =~ s/rtk_port_phyForceModeAbility_get *\(/rtk_rg_port_phyForceModeAbility_get\(/g;
	$line =~ s/rtk_port_phyForceModeAbility_set *\(/rtk_rg_port_phyForceModeAbility_set\(/g;
	$line =~ s/rtk_port_phyMasterSlave_get *\(/rtk_rg_port_phyMasterSlave_get\(/g;
	$line =~ s/rtk_port_phyMasterSlave_set *\(/rtk_rg_port_phyMasterSlave_set\(/g;
	$line =~ s/rtk_port_phyReg_get *\(/rtk_rg_port_phyReg_get\(/g;
	$line =~ s/rtk_port_phyReg_set *\(/rtk_rg_port_phyReg_set\(/g;
	$line =~ s/rtk_port_phyTestMode_get *\(/rtk_rg_port_phyTestMode_get\(/g;
	$line =~ s/rtk_port_phyTestMode_set *\(/rtk_rg_port_phyTestMode_set\(/g;
	$line =~ s/rtk_port_rtctResult_get *\(/rtk_rg_port_rtctResult_get\(/g;
	$line =~ s/rtk_port_rtct_start *\(/rtk_rg_port_rtct_start\(/g;
	$line =~ s/rtk_port_specialCongestStatus_clear *\(/rtk_rg_port_specialCongestStatus_clear\(/g;
	$line =~ s/rtk_port_specialCongestStatus_get *\(/rtk_rg_port_specialCongestStatus_get\(/g;
	$line =~ s/rtk_port_specialCongest_get *\(/rtk_rg_port_specialCongest_get\(/g;
	$line =~ s/rtk_port_specialCongest_set *\(/rtk_rg_port_specialCongest_set\(/g;
	$line =~ s/rtk_port_speedDuplex_get *\(/rtk_rg_port_speedDuplex_get\(/g;
	$line =~ s/rtk_rate_egrBandwidthCtrlIncludeIfg_get *\(/rtk_rg_rate_egrBandwidthCtrlIncludeIfg_get\(/g;
	$line =~ s/rtk_rate_egrBandwidthCtrlIncludeIfg_set *\(/rtk_rg_rate_egrBandwidthCtrlIncludeIfg_set\(/g;
	$line =~ s/rtk_rate_egrQueueBwCtrlEnable_get *\(/rtk_rg_rate_egrQueueBwCtrlEnable_get\(/g;
	$line =~ s/rtk_rate_egrQueueBwCtrlEnable_set *\(/rtk_rg_rate_egrQueueBwCtrlEnable_set\(/g;
	$line =~ s/rtk_rate_egrQueueBwCtrlMeterIdx_get *\(/rtk_rg_rate_egrQueueBwCtrlMeterIdx_get\(/g;
	$line =~ s/rtk_rate_egrQueueBwCtrlMeterIdx_set *\(/rtk_rg_rate_egrQueueBwCtrlMeterIdx_set\(/g;
	$line =~ s/rtk_rate_init *\(/rtk_rg_rate_init\(/g;
	$line =~ s/rtk_rate_portEgrBandwidthCtrlIncludeIfg_get *\(/rtk_rg_rate_portEgrBandwidthCtrlIncludeIfg_get\(/g;
	$line =~ s/rtk_rate_portEgrBandwidthCtrlIncludeIfg_set *\(/rtk_rg_rate_portEgrBandwidthCtrlIncludeIfg_set\(/g;
	$line =~ s/rtk_rate_portEgrBandwidthCtrlRate_get *\(/rtk_rg_rate_portEgrBandwidthCtrlRate_get\(/g;
	$line =~ s/rtk_rate_portEgrBandwidthCtrlRate_set *\(/rtk_rg_rate_portEgrBandwidthCtrlRate_set\(/g;
	$line =~ s/rtk_rate_portIgrBandwidthCtrlIncludeIfg_get *\(/rtk_rg_rate_portIgrBandwidthCtrlIncludeIfg_get\(/g;
	$line =~ s/rtk_rate_portIgrBandwidthCtrlIncludeIfg_set *\(/rtk_rg_rate_portIgrBandwidthCtrlIncludeIfg_set\(/g;
	$line =~ s/rtk_rate_portIgrBandwidthCtrlRate_get *\(/rtk_rg_rate_portIgrBandwidthCtrlRate_get\(/g;
	$line =~ s/rtk_rate_portIgrBandwidthCtrlRate_set *\(/rtk_rg_rate_portIgrBandwidthCtrlRate_set\(/g;
	$line =~ s/rtk_rate_shareMeterBucket_get *\(/rtk_rg_rate_shareMeterBucket_get\(/g;
	$line =~ s/rtk_rate_shareMeterBucket_set *\(/rtk_rg_rate_shareMeterBucket_set\(/g;
	$line =~ s/rtk_rate_shareMeterExceed_clear *\(/rtk_rg_rate_shareMeterExceed_clear\(/g;
	$line =~ s/rtk_rate_shareMeterExceed_get *\(/rtk_rg_rate_shareMeterExceed_get\(/g;
	$line =~ s/rtk_rate_shareMeterMode_get *\(/rtk_rg_rate_shareMeterMode_get\(/g;
	$line =~ s/rtk_rate_shareMeterMode_set *\(/rtk_rg_rate_shareMeterMode_set\(/g;
	$line =~ s/rtk_rate_shareMeter_get *\(/rtk_rg_rate_shareMeter_get\(/g;
	$line =~ s/rtk_rate_shareMeter_set *\(/rtk_rg_rate_shareMeter_set\(/g;
	$line =~ s/rtk_rate_stormBypass_get *\(/rtk_rg_rate_stormBypass_get\(/g;
	$line =~ s/rtk_rate_stormBypass_set *\(/rtk_rg_rate_stormBypass_set\(/g;
	$line =~ s/rtk_rate_stormControlEnable_get *\(/rtk_rg_rate_stormControlEnable_get\(/g;
	$line =~ s/rtk_rate_stormControlEnable_set *\(/rtk_rg_rate_stormControlEnable_set\(/g;
	$line =~ s/rtk_rate_stormControlMeterIdx_get *\(/rtk_rg_rate_stormControlMeterIdx_get\(/g;
	$line =~ s/rtk_rate_stormControlMeterIdx_set *\(/rtk_rg_rate_stormControlMeterIdx_set\(/g;
	$line =~ s/rtk_rate_stormControlPortEnable_get *\(/rtk_rg_rate_stormControlPortEnable_get\(/g;
	$line =~ s/rtk_rate_stormControlPortEnable_set *\(/rtk_rg_rate_stormControlPortEnable_set\(/g;
	$line =~ s/rtk_rldp_config_get *\(/rtk_rg_rldp_config_get\(/g;
	$line =~ s/rtk_rldp_config_set *\(/rtk_rg_rldp_config_set\(/g;
	$line =~ s/rtk_rldp_init *\(/rtk_rg_rldp_init\(/g;
	$line =~ s/rtk_rldp_portConfig_get *\(/rtk_rg_rldp_portConfig_get\(/g;
	$line =~ s/rtk_rldp_portConfig_set *\(/rtk_rg_rldp_portConfig_set\(/g;
	$line =~ s/rtk_rldp_portStatus_clear *\(/rtk_rg_rldp_portStatus_clear\(/g;
	$line =~ s/rtk_rldp_portStatus_get *\(/rtk_rg_rldp_portStatus_get\(/g;
	$line =~ s/rtk_rldp_status_get *\(/rtk_rg_rldp_status_get\(/g;
	$line =~ s/rtk_rlpp_init *\(/rtk_rg_rlpp_init\(/g;
	$line =~ s/rtk_rlpp_trapType_get *\(/rtk_rg_rlpp_trapType_get\(/g;
	$line =~ s/rtk_rlpp_trapType_set *\(/rtk_rg_rlpp_trapType_set\(/g;
	$line =~ s/rtk_stat_global_get *\(/rtk_rg_stat_global_get\(/g;
	$line =~ s/rtk_stat_global_getAll *\(/rtk_rg_stat_global_getAll\(/g;
	$line =~ s/rtk_stat_global_reset *\(/rtk_rg_stat_global_reset\(/g;
	$line =~ s/rtk_stat_init *\(/rtk_rg_stat_init\(/g;
	$line =~ s/rtk_stat_logCtrl_get *\(/rtk_rg_stat_logCtrl_get\(/g;
	$line =~ s/rtk_stat_logCtrl_set *\(/rtk_rg_stat_logCtrl_set\(/g;
	$line =~ s/rtk_stat_log_get *\(/rtk_rg_stat_log_get\(/g;
	$line =~ s/rtk_stat_log_reset *\(/rtk_rg_stat_log_reset\(/g;
	$line =~ s/rtk_stat_mibCntMode_get *\(/rtk_rg_stat_mibCntMode_get\(/g;
	$line =~ s/rtk_stat_mibCntMode_set *\(/rtk_rg_stat_mibCntMode_set\(/g;
	$line =~ s/rtk_stat_mibCntTagLen_get *\(/rtk_rg_stat_mibCntTagLen_get\(/g;
	$line =~ s/rtk_stat_mibCntTagLen_set *\(/rtk_rg_stat_mibCntTagLen_set\(/g;
	$line =~ s/rtk_stat_mibLatchTimer_get *\(/rtk_rg_stat_mibLatchTimer_get\(/g;
	$line =~ s/rtk_stat_mibLatchTimer_set *\(/rtk_rg_stat_mibLatchTimer_set\(/g;
	$line =~ s/rtk_stat_mibSyncMode_get *\(/rtk_rg_stat_mibSyncMode_get\(/g;
	$line =~ s/rtk_stat_mibSyncMode_set *\(/rtk_rg_stat_mibSyncMode_set\(/g;
	$line =~ s/rtk_stat_pktInfo_get *\(/rtk_rg_stat_pktInfo_get\(/g;
	$line =~ s/rtk_stat_port_get *\(/rtk_rg_stat_port_get\(/g;
	$line =~ s/rtk_stat_port_getAll *\(/rtk_rg_stat_port_getAll\(/g;
	$line =~ s/rtk_stat_port_reset *\(/rtk_rg_stat_port_reset\(/g;
	$line =~ s/rtk_stat_rstCntValue_get *\(/rtk_rg_stat_rstCntValue_get\(/g;
	$line =~ s/rtk_stat_rstCntValue_set *\(/rtk_rg_stat_rstCntValue_set\(/g;
	$line =~ s/rtk_switch_allExtPortMask_set *\(/rtk_rg_switch_allExtPortMask_set\(/g;
	$line =~ s/rtk_switch_allPortMask_set *\(/rtk_rg_switch_allPortMask_set\(/g;
	$line =~ s/rtk_switch_chip_reset *\(/rtk_rg_switch_chip_reset\(/g;
	$line =~ s/rtk_switch_deviceInfo_get *\(/rtk_rg_switch_deviceInfo_get\(/g;
	$line =~ s/rtk_switch_init *\(/rtk_rg_switch_init\(/g;
	$line =~ s/rtk_switch_logicalPort_get *\(/rtk_rg_switch_logicalPort_get\(/g;
	$line =~ s/rtk_switch_maxPktLenLinkSpeed_get *\(/rtk_rg_switch_maxPktLenLinkSpeed_get\(/g;
	$line =~ s/rtk_switch_maxPktLenLinkSpeed_set *\(/rtk_rg_switch_maxPktLenLinkSpeed_set\(/g;
	$line =~ s/rtk_switch_mgmtMacAddr_get *\(/rtk_rg_switch_mgmtMacAddr_get\(/g;
	$line =~ s/rtk_switch_mgmtMacAddr_set *\(/rtk_rg_switch_mgmtMacAddr_set\(/g;
	$line =~ s/rtk_switch_nextPortInMask_get *\(/rtk_rg_switch_nextPortInMask_get\(/g;
	$line =~ s/rtk_switch_phyPortId_get *\(/rtk_rg_switch_phyPortId_get\(/g;
	$line =~ s/rtk_switch_port2PortMask_clear *\(/rtk_rg_switch_port2PortMask_clear\(/g;
	$line =~ s/rtk_switch_port2PortMask_set *\(/rtk_rg_switch_port2PortMask_set\(/g;
	$line =~ s/rtk_switch_portIdInMask_check *\(/rtk_rg_switch_portIdInMask_check\(/g;
	$line =~ s/rtk_switch_portMask_Clear *\(/rtk_rg_switch_portMask_Clear\(/g;
	$line =~ s/rtk_switch_version_get *\(/rtk_rg_switch_version_get\(/g;
	$line =~ s/rtk_trap_igmpCtrlPkt2CpuEnable_get *\(/rtk_rg_trap_igmpCtrlPkt2CpuEnable_get\(/g;
	$line =~ s/rtk_trap_igmpCtrlPkt2CpuEnable_set *\(/rtk_rg_trap_igmpCtrlPkt2CpuEnable_set\(/g;
	$line =~ s/rtk_trap_init *\(/rtk_rg_trap_init\(/g;
	$line =~ s/rtk_trap_ipMcastPkt2CpuEnable_get *\(/rtk_rg_trap_ipMcastPkt2CpuEnable_get\(/g;
	$line =~ s/rtk_trap_ipMcastPkt2CpuEnable_set *\(/rtk_rg_trap_ipMcastPkt2CpuEnable_set\(/g;
	$line =~ s/rtk_trap_l2McastPkt2CpuEnable_get *\(/rtk_rg_trap_l2McastPkt2CpuEnable_get\(/g;
	$line =~ s/rtk_trap_l2McastPkt2CpuEnable_set *\(/rtk_rg_trap_l2McastPkt2CpuEnable_set\(/g;
	$line =~ s/rtk_trap_mldCtrlPkt2CpuEnable_get *\(/rtk_rg_trap_mldCtrlPkt2CpuEnable_get\(/g;
	$line =~ s/rtk_trap_mldCtrlPkt2CpuEnable_set *\(/rtk_rg_trap_mldCtrlPkt2CpuEnable_set\(/g;
	$line =~ s/rtk_trap_oamPduAction_get *\(/rtk_rg_trap_oamPduAction_get\(/g;
	$line =~ s/rtk_trap_oamPduAction_set *\(/rtk_rg_trap_oamPduAction_set\(/g;
	$line =~ s/rtk_trap_oamPduPri_get *\(/rtk_rg_trap_oamPduPri_get\(/g;
	$line =~ s/rtk_trap_oamPduPri_set *\(/rtk_rg_trap_oamPduPri_set\(/g;
	$line =~ s/rtk_trap_portIgmpMldCtrlPktAction_get *\(/rtk_rg_trap_portIgmpMldCtrlPktAction_get\(/g;
	$line =~ s/rtk_trap_portIgmpMldCtrlPktAction_set *\(/rtk_rg_trap_portIgmpMldCtrlPktAction_set\(/g;
	$line =~ s/rtk_trap_reasonTrapToCpuPriority_get *\(/rtk_rg_trap_reasonTrapToCpuPriority_get\(/g;
	$line =~ s/rtk_trap_reasonTrapToCpuPriority_set *\(/rtk_rg_trap_reasonTrapToCpuPriority_set\(/g;
	$line =~ s/rtk_trap_rmaAction_get *\(/rtk_rg_trap_rmaAction_get\(/g;
	$line =~ s/rtk_trap_rmaAction_set *\(/rtk_rg_trap_rmaAction_set\(/g;
	$line =~ s/rtk_trap_rmaPri_get *\(/rtk_rg_trap_rmaPri_get\(/g;
	$line =~ s/rtk_trap_rmaPri_set *\(/rtk_rg_trap_rmaPri_set\(/g;
	print $line;
}
