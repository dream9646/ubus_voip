/******************************************************************
 *
 * Copyright (C) 2011 5V Technologies Ltd.
 * All Rights Reserved.
 *
 * This program is the proprietary software of 5V Technologies Ltd
 * and/or its licensors, and only be used, duplicated,
 * modified or distributed under the authorized license from 5VT.
 *
 * ANY USE OF THE SOFTWARE OTHER THAN AS AUTHORIZED UNDER
 * THIS LICENSE OR COPYRIGHT LAW IS PROHIBITED.
 *
 * Purpose : 5VT OMCI protocol stack
 * Module  : proprietary_calix
 * File    : proprietary_calix.h
 *
 ******************************************************************/

#ifndef __PROPRIETARY_CALIX_H__
#define __PROPRIETARY_CALIX_H__

#include "er_group.h"

#define CALIX_DAT_FILE	"/nvram/calix.dat"

#ifndef X86
/*er_group_hw_11_47_249.c*/
int er_group_hw_l2s_l2_tags_create_add012_pptp_uni(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);
/*er_group_hw_11_264.c*/
int er_group_hw_rg_group(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);
/*er_group_hw_47_130_249.c*/
int er_group_hw_l2s_l2_tags_create_add012_802_1p(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);
/*er_group_hw_47_134_249.c*/
int er_group_hw_l2s_l2_tags_create_add012_ip_host(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);
/*er_group_hw_47_249.c*/
int er_group_hw_l2s_l2_tags_create_add012_bridge(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);
/*er_group_hw_47_249_329.c*/
int er_group_hw_l2s_l2_tags_create_add012_veip(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);
/* er_group_hw_47_148_329_65317.c */
int er_group_hw_calix_rg_config_pppauth(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);
/* er_group_hw_47_329_65317.c */
int er_group_hw_calix_rg_config_restore_default(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);
int er_group_hw_calix_rg_config_remote_gui_access_time(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);
int er_group_hw_calix_rg_config(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);
int er_group_hw_calix_rg_registration_id(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);
/* er_group_hw_65500.c */
int er_group_hw_full_bridge_enable(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);
/* er_group_hw_65503.c */
int er_group_hw_restore_to_default(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);
/* er_group_hw_241.c */
int er_group_hw_date_time(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);
int er_group_hw_date_timezone(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);
int er_group_hw_telnet_enable(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);
int er_group_hw_ookla_disable(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);
/* er_group_hw_242.c */
int er_group_hw_VideoTxMode(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);
int er_group_hw_IfCountReset_242(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);
int er_group_hw_ForceOperMode(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);
/* er_group_hw_243.c */
int er_group_hw_primaryconfigserverpointer(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);
/* er_group_hw_248.c */
int er_group_hw_video_agc_enable(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);
/* er_group_hw_45_47_49_79_251.c */
int er_group_hw_l2cp_enable(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);
/* er_group_hw_251_171_280.c */
int er_group_hw_uni_policing(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);
/* er_group_hw_53_65283.c */
int er_group_hw_voip_call_statistics_extension4(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);
/* er_group_hw_65287.c */
int er_group_hw_oam_config(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);
/* er_group_hw_65288_65289.c */
int er_group_hw_oam_mep(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);
/* er_group_hw_65288_65290.c */
int er_group_hw_oam_mip(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);
/* er_group_hw_65288_65289_65291.c */
int er_group_hw_oam_mep_linktrace(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);
/* er_group_hw_65288_65289_65292.c */
int er_group_hw_oam_mep_loopback(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);
/* er_group_hw_65288_65289_65294.c */
int er_group_hw_oam_mep_pm_macaddr(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);
/* er_group_hw_65288_65289_65299.c */
int er_group_hw_oam_mep_clear_counters(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);
/* er_group_hw_65288_65289_65302.c */
int er_group_hw_oam_lm_clear_counters(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);
/* er_group_hw_65288_65289_65303.c */
int er_group_hw_oam_dm_clear_counters(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);
/* er_group_hw_65288_65290_65301.c */
int er_group_hw_oam_mip_clear_counters(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);
/* er_group_hw_65311.c */
int er_group_hw_IfCountReset_65311(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);
/* er_group_hw_65313.c */
int er_group_hw_calix_vlan_tlan(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);
/* er_group_hw_65318.c */
int er_group_hw_max_power_budget(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);
/* er_group_hw_53_134_136_150_153_65315 */
int er_group_hw_calix_voip_fqdn(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);
/* er_group_hw_65318.c */
int er_group_hw_lldp_admin_state(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);
int er_group_hw_lldp_network_policy(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);
int er_group_hw_lldp_vid(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);
int er_group_hw_lldp_priority(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);
int er_group_hw_lldp_dscp(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);
#endif

/* me_related_011.c */
int me_related_011_242(struct me_t *me1, struct me_t *me2);
int me_related_011_251(struct me_t *me1, struct me_t *me2);
int me_related_11_65311(struct me_t *me1, struct me_t *me2);
int me_related_011_65289(struct me_t *me1, struct me_t *me2);
int me_related_011_65290(struct me_t *me1, struct me_t *me2);
/* me_related_045.c */
int me_related_045_251(struct me_t *me1, struct me_t *me2);
/* me_related_053.c */
int me_related_053_244(struct me_t *me1, struct me_t *me2);
int me_related_053_65283(struct me_t *me1, struct me_t *me2);
/* me_related_130.c */
int me_related_130_65289(struct me_t *me1, struct me_t *me2);
int me_related_130_65290(struct me_t *me1, struct me_t *me2);
/* me_related_134.c */
int me_related_134_65289(struct me_t *me1, struct me_t *me2);
int me_related_134_65290(struct me_t *me1, struct me_t *me2);
int me_related_134_65315(struct me_t *me1, struct me_t *me2);
/* me_related_148.c */
int me_related_148_65317(struct me_t *me1, struct me_t *me2);
/* me_related_242.c */
int me_related_242_011(struct me_t *me1, struct me_t *me2);
/* me_related_244.c */
int me_related_244_053(struct me_t *me1, struct me_t *me2);
/* me_related_249.c */
int me_related_249_047(struct me_t *me1, struct me_t *me2);
int me_related_249_130(struct me_t *me1, struct me_t *me2);
int me_related_249_011(struct me_t *me1, struct me_t *me2);
int me_related_249_266(struct me_t *me1, struct me_t *me2);
int me_related_249_281(struct me_t *me1, struct me_t *me2);
int me_related_249_329(struct me_t *me1, struct me_t *me2);
/* me_related_251.c */
int me_related_251_011(struct me_t *me1, struct me_t *me2);
int me_related_251_045(struct me_t *me1, struct me_t *me2);
int me_related_251_329(struct me_t *me1, struct me_t *me2);
/* me_related_266.c */
int me_related_266_65289(struct me_t *me1, struct me_t *me2);
int me_related_266_65290(struct me_t *me1, struct me_t *me2);
/* me_related_281.c */
int me_related_281_65289(struct me_t *me1, struct me_t *me2);
int me_related_281_65290(struct me_t *me1, struct me_t *me2);
/* me_related_329.c */
int me_related_329_251(struct me_t *me1, struct me_t *me2);
int me_related_329_65289(struct me_t *me1, struct me_t *me2);
int me_related_329_65290(struct me_t *me1, struct me_t *me2);
int me_related_329_65317(struct me_t *me1, struct me_t *me2);
/* me_related_65283.c */
int me_related_65283_053(struct me_t *me1, struct me_t *me2);
/* me_related_65287.c */
int me_related_65287_65289(struct me_t *me1, struct me_t *me2);
int me_related_65287_65290(struct me_t *me1, struct me_t *me2);
/* me_related_65288.c */
int me_related_65288_65289(struct me_t *me1, struct me_t *me2);
int me_related_65288_65290(struct me_t *me1, struct me_t *me2);
/* me_related_65289.c */
int me_related_65289_011(struct me_t *me1, struct me_t *me2);
int me_related_65289_130(struct me_t *me1, struct me_t *me2);
int me_related_65289_134(struct me_t *me1, struct me_t *me2);
int me_related_65289_266(struct me_t *me1, struct me_t *me2);
int me_related_65289_281(struct me_t *me1, struct me_t *me2);
int me_related_65289_329(struct me_t *me1, struct me_t *me2);
int me_related_65289_65287(struct me_t *me1, struct me_t *me2);
int me_related_65289_65288(struct me_t *me1, struct me_t *me2);
int me_related_65289_65291(struct me_t *me1, struct me_t *me2);
int me_related_65289_65292(struct me_t *me1, struct me_t *me2);
int me_related_65289_65294(struct me_t *me1, struct me_t *me2);
int me_related_65289_65299(struct me_t *me1, struct me_t *me2);
int me_related_65289_65300(struct me_t *me1, struct me_t *me2);
int me_related_65289_65302(struct me_t *me1, struct me_t *me2);
int me_related_65289_65303(struct me_t *me1, struct me_t *me2);
int me_related_65289_65304(struct me_t *me1, struct me_t *me2);
/* me_related_65290.c */
int me_related_65290_011(struct me_t *me1, struct me_t *me2);
int me_related_65290_130(struct me_t *me1, struct me_t *me2);
int me_related_65290_134(struct me_t *me1, struct me_t *me2);
int me_related_65290_266(struct me_t *me1, struct me_t *me2);
int me_related_65290_281(struct me_t *me1, struct me_t *me2);
int me_related_65290_329(struct me_t *me1, struct me_t *me2);
int me_related_65290_65287(struct me_t *me1, struct me_t *me2);
int me_related_65290_65288(struct me_t *me1, struct me_t *me2);
int me_related_65290_65301(struct me_t *me1, struct me_t *me2);
/* me_related_65291.c */
int me_related_65291_65289(struct me_t *me1, struct me_t *me2);
/* me_related_65292.c */
int me_related_65292_65289(struct me_t *me1, struct me_t *me2);
/* me_related_65294.c */
int me_related_65294_65289(struct me_t *me1, struct me_t *me2);
/* me_related_65299.c */
int me_related_65299_65289(struct me_t *me1, struct me_t *me2);
/* me_related_65300.c */
int me_related_65300_65289(struct me_t *me1, struct me_t *me2);
/* me_related_65301.c */
int me_related_65301_65290(struct me_t *me1, struct me_t *me2);
/* me_related_65302.c */
int me_related_65302_65289(struct me_t *me1, struct me_t *me2);
/* me_related_65303.c */
int me_related_65303_65289(struct me_t *me1, struct me_t *me2);
/* me_related_65304.c */
int me_related_65304_65289(struct me_t *me1, struct me_t *me2);
/* me_related_65311.c */
int me_related_65311_11(struct me_t *me1, struct me_t *me2);
/* me_related_65315.c */
int me_related_65315_134(struct me_t *me1, struct me_t *me2);
/* me_related_65317.c */
int me_related_65317_329(struct me_t *me1, struct me_t *me2);
int me_related_65317_148(struct me_t *me1, struct me_t *me2);
/* batchtab_cb_calix_tlan.c */
int batchtab_cb_calix_tlan_register(void);
/* batchtab_cb_calix_l2cp.c */
int batchtab_cb_calix_l2cp_register(void);
/* batchtab_cb_calix_uni_policing.c */
int batchtab_cb_calix_uni_policing_register(void);
/* batchtab_cb_calix_apas.c */
int batchtab_cb_calix_apas_is_enabled(void);
int batchtab_cb_calix_apas_register(void);
/* proprietary_calix.c */
unsigned int proprietary_calix_get_olt_version(void);
int proprietary_calix(void);
#endif
