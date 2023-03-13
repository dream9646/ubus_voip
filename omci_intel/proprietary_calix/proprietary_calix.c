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
 * File    : proprietary_calix.c
 *
 ******************************************************************/

#include <stdio.h>
#include <string.h>
#include "me_related.h"
#include "meinfo_cb.h"
#include "er_group.h"
#ifndef X86
#include "meinfo_hw.h"
#include "er_group_hw.h"
#endif
#include "proprietary_calix.h"

extern struct meinfo_callback_t meinfo_cb_calix_241;
extern struct meinfo_callback_t meinfo_cb_calix_242;
extern struct meinfo_callback_t meinfo_cb_calix_244;
extern struct meinfo_callback_t meinfo_cb_calix_249;
extern struct meinfo_callback_t meinfo_cb_calix_65283;
extern struct meinfo_callback_t meinfo_cb_calix_65288;
extern struct meinfo_callback_t meinfo_cb_calix_65289;
extern struct meinfo_callback_t meinfo_cb_calix_65291;
extern struct meinfo_callback_t meinfo_cb_calix_65292;
extern struct meinfo_callback_t meinfo_cb_calix_65294;
extern struct meinfo_callback_t meinfo_cb_calix_65299;
extern struct meinfo_callback_t meinfo_cb_calix_65300;
extern struct meinfo_callback_t meinfo_cb_calix_65301;
extern struct meinfo_callback_t meinfo_cb_calix_65302;
extern struct meinfo_callback_t meinfo_cb_calix_65303;
extern struct meinfo_callback_t meinfo_cb_calix_65304;
extern struct meinfo_callback_t meinfo_cb_calix_65311;
extern struct meinfo_callback_t meinfo_cb_calix_65313;
extern struct meinfo_callback_t meinfo_cb_calix_65317;
extern struct meinfo_callback_t meinfo_cb_calix_65501;

#ifndef X86
extern struct meinfo_hardware_t meinfo_hw_calix_241;
extern struct meinfo_hardware_t meinfo_hw_calix_242;
extern struct meinfo_hardware_t meinfo_hw_calix_244;
extern struct meinfo_hardware_t meinfo_hw_calix_248;
extern struct meinfo_hardware_t meinfo_hw_calix_249;
extern struct meinfo_hardware_t meinfo_hw_calix_65280;
extern struct meinfo_hardware_t meinfo_hw_calix_65281;
extern struct meinfo_hardware_t meinfo_hw_calix_65282;
extern struct meinfo_hardware_t meinfo_hw_calix_65283;
extern struct meinfo_hardware_t meinfo_hw_calix_65311;
extern struct meinfo_hardware_t meinfo_hw_calix_65315;
extern struct meinfo_hardware_t meinfo_hw_calix_65317;
extern struct meinfo_hardware_t meinfo_hw_calix_65318;
extern struct meinfo_hardware_t meinfo_hw_calix_65319;
extern struct meinfo_hardware_t meinfo_hw_calix_65501;
extern struct meinfo_hardware_t meinfo_hw_calix_65502;
extern struct meinfo_hardware_t meinfo_hw_calix_65503;
#endif

//0: old version, 1: new version
unsigned int
proprietary_calix_get_olt_version(void)
{
	struct me_t *me_131;

	//get olt-g to check version
	if ((me_131 = meinfo_me_get_by_instance_num(131, 0)) == NULL ||
		me_131->attr[3].value.ptr == NULL)
	{
		return -1;
	}

	//get equipment id
	if (strncmp(me_131->attr[2].value.ptr, "AXOS", 4) == 0)
		return 1; // AXOS
	else
		return 0; // EXA
}

static int
proprietary_calix_me_related_init(void)
{
	me_related_add(ME_RELATED_FUNC_SPEC, 11, 242, me_related_011_242);
	me_related_add(ME_RELATED_FUNC_SPEC, 242, 11, me_related_242_011);

	me_related_add(ME_RELATED_FUNC_SPEC, 11, 251, me_related_011_251);
	me_related_add(ME_RELATED_FUNC_SPEC, 251, 11, me_related_251_011);
	me_related_add(ME_RELATED_FUNC_SPEC, 329, 251, me_related_329_251);
	me_related_add(ME_RELATED_FUNC_SPEC, 251, 329, me_related_251_329);
	me_related_add(ME_RELATED_FUNC_SPEC, 45, 251, me_related_045_251);
	me_related_add(ME_RELATED_FUNC_SPEC, 251, 45, me_related_251_045);

	me_related_add(ME_RELATED_FUNC_SPEC, 53, 244, me_related_053_244);
	me_related_add(ME_RELATED_FUNC_SPEC, 244, 53, me_related_244_053);
	me_related_add(ME_RELATED_FUNC_SPEC, 53, 65283, me_related_053_65283);
	me_related_add(ME_RELATED_FUNC_SPEC, 65283, 53, me_related_65283_053);

	me_related_add(ME_RELATED_FUNC_SPEC, 249, 47, me_related_249_047);
	me_related_add(ME_RELATED_FUNC_SPEC, 249, 130, me_related_249_130);
	me_related_add(ME_RELATED_FUNC_SPEC, 249, 11, me_related_249_011);
	me_related_add(ME_RELATED_FUNC_SPEC, 249, 266, me_related_249_266);
	me_related_add(ME_RELATED_FUNC_SPEC, 249, 281, me_related_249_281);
	me_related_add(ME_RELATED_FUNC_SPEC, 249, 329, me_related_249_329);
	
	me_related_add(ME_RELATED_FUNC_SPEC, 65317, 148, me_related_65317_148);
	me_related_add(ME_RELATED_FUNC_SPEC, 148, 65317, me_related_148_65317);
	me_related_add(ME_RELATED_FUNC_SPEC, 65317, 329, me_related_65317_329);
	me_related_add(ME_RELATED_FUNC_SPEC, 329, 65317, me_related_329_65317);
    
	me_related_add(ME_RELATED_FUNC_SPEC, 65311, 11, me_related_65311_11);
	me_related_add(ME_RELATED_FUNC_SPEC, 11, 65311, me_related_11_65311);
    
	me_related_add(ME_RELATED_FUNC_SPEC, 65315, 134, me_related_65315_134);
	me_related_add(ME_RELATED_FUNC_SPEC, 134, 65315, me_related_134_65315);
    
	me_related_add(ME_RELATED_FUNC_SPEC, 11, 65289, me_related_011_65289);
	me_related_add(ME_RELATED_FUNC_SPEC, 65289, 11, me_related_65289_011);
	me_related_add(ME_RELATED_FUNC_SPEC, 130, 65289, me_related_130_65289);
	me_related_add(ME_RELATED_FUNC_SPEC, 65289, 130, me_related_65289_130);
	me_related_add(ME_RELATED_FUNC_SPEC, 134, 65289, me_related_134_65289);
	me_related_add(ME_RELATED_FUNC_SPEC, 65289, 134, me_related_65289_134);
	me_related_add(ME_RELATED_FUNC_SPEC, 266, 65289, me_related_266_65289);
	me_related_add(ME_RELATED_FUNC_SPEC, 65289, 266, me_related_65289_266);
	me_related_add(ME_RELATED_FUNC_SPEC, 281, 65289, me_related_281_65289);
	me_related_add(ME_RELATED_FUNC_SPEC, 65289, 281, me_related_65289_281);
	me_related_add(ME_RELATED_FUNC_SPEC, 329, 65289, me_related_329_65289);
	me_related_add(ME_RELATED_FUNC_SPEC, 65289, 329, me_related_65289_329);
	me_related_add(ME_RELATED_FUNC_SPEC, 65288, 65289, me_related_65288_65289);
	me_related_add(ME_RELATED_FUNC_SPEC, 65289, 65288, me_related_65289_65288);
	me_related_add(ME_RELATED_FUNC_SPEC, 65289, 65291, me_related_65289_65291);
	me_related_add(ME_RELATED_FUNC_SPEC, 65291, 65289, me_related_65291_65289);
	me_related_add(ME_RELATED_FUNC_SPEC, 65289, 65292, me_related_65289_65292);
	me_related_add(ME_RELATED_FUNC_SPEC, 65292, 65289, me_related_65292_65289);
	me_related_add(ME_RELATED_FUNC_SPEC, 65289, 65294, me_related_65289_65294);
	me_related_add(ME_RELATED_FUNC_SPEC, 65294, 65289, me_related_65294_65289);
	me_related_add(ME_RELATED_FUNC_SPEC, 65289, 65299, me_related_65289_65299);
	me_related_add(ME_RELATED_FUNC_SPEC, 65299, 65289, me_related_65299_65289);
	me_related_add(ME_RELATED_FUNC_SPEC, 65289, 65300, me_related_65289_65300);
	me_related_add(ME_RELATED_FUNC_SPEC, 65300, 65289, me_related_65300_65289);
	me_related_add(ME_RELATED_FUNC_SPEC, 65289, 65302, me_related_65289_65302);
	me_related_add(ME_RELATED_FUNC_SPEC, 65302, 65289, me_related_65302_65289);
	me_related_add(ME_RELATED_FUNC_SPEC, 65289, 65303, me_related_65289_65303);
	me_related_add(ME_RELATED_FUNC_SPEC, 65303, 65289, me_related_65303_65289);
	me_related_add(ME_RELATED_FUNC_SPEC, 65289, 65304, me_related_65289_65304);
	me_related_add(ME_RELATED_FUNC_SPEC, 65304, 65289, me_related_65304_65289);

	me_related_add(ME_RELATED_FUNC_SPEC, 11, 65290, me_related_011_65290);
	me_related_add(ME_RELATED_FUNC_SPEC, 65290, 11, me_related_65290_011);
	me_related_add(ME_RELATED_FUNC_SPEC, 130, 65290, me_related_130_65290);
	me_related_add(ME_RELATED_FUNC_SPEC, 65290, 130, me_related_65290_130);
	me_related_add(ME_RELATED_FUNC_SPEC, 134, 65290, me_related_134_65290);
	me_related_add(ME_RELATED_FUNC_SPEC, 65290, 134, me_related_65290_134);
	me_related_add(ME_RELATED_FUNC_SPEC, 266, 65290, me_related_266_65290);
	me_related_add(ME_RELATED_FUNC_SPEC, 65290, 266, me_related_65290_266);
	me_related_add(ME_RELATED_FUNC_SPEC, 281, 65290, me_related_281_65290);
	me_related_add(ME_RELATED_FUNC_SPEC, 65290, 281, me_related_65290_281);
	me_related_add(ME_RELATED_FUNC_SPEC, 329, 65290, me_related_329_65290);
	me_related_add(ME_RELATED_FUNC_SPEC, 65290, 329, me_related_65290_329);
	me_related_add(ME_RELATED_FUNC_SPEC, 65288, 65290, me_related_65288_65290);
	me_related_add(ME_RELATED_FUNC_SPEC, 65290, 65288, me_related_65290_65288);
	me_related_add(ME_RELATED_FUNC_SPEC, 65290, 65301, me_related_65290_65301);
	me_related_add(ME_RELATED_FUNC_SPEC, 65301, 65290, me_related_65301_65290);

	return 0;
}

static int
proprietary_calix_meinfo_cb_init(void)
{
	meinfo_cb_register(241, &meinfo_cb_calix_241);
	meinfo_cb_register(242, &meinfo_cb_calix_242);
	meinfo_cb_register(244, &meinfo_cb_calix_244);
	meinfo_cb_register(249, &meinfo_cb_calix_249);
	meinfo_cb_register(65283, &meinfo_cb_calix_65283);
	meinfo_cb_register(65288, &meinfo_cb_calix_65288);
	meinfo_cb_register(65289, &meinfo_cb_calix_65289);
	meinfo_cb_register(65291, &meinfo_cb_calix_65291);
	meinfo_cb_register(65292, &meinfo_cb_calix_65292);
	meinfo_cb_register(65294, &meinfo_cb_calix_65294);
	meinfo_cb_register(65299, &meinfo_cb_calix_65299);
	meinfo_cb_register(65300, &meinfo_cb_calix_65300);
	meinfo_cb_register(65301, &meinfo_cb_calix_65301);
	meinfo_cb_register(65302, &meinfo_cb_calix_65302);
	meinfo_cb_register(65303, &meinfo_cb_calix_65303);
	meinfo_cb_register(65304, &meinfo_cb_calix_65304);
	meinfo_cb_register(65311, &meinfo_cb_calix_65311);
	meinfo_cb_register(65313, &meinfo_cb_calix_65313);
	meinfo_cb_register(65317, &meinfo_cb_calix_65317);
	meinfo_cb_register(65501, &meinfo_cb_calix_65501);
	return 0;
}

#ifndef X86
static int
proprietary_calix_meinfo_hw_init(void)
{
	meinfo_hw_register(241, &meinfo_hw_calix_241);
	meinfo_hw_register(242, &meinfo_hw_calix_242);
	meinfo_hw_register(244, &meinfo_hw_calix_244);
	meinfo_hw_register(248, &meinfo_hw_calix_248);
	meinfo_hw_register(249, &meinfo_hw_calix_249);
	meinfo_hw_register(65280, &meinfo_hw_calix_65280);
	meinfo_hw_register(65281, &meinfo_hw_calix_65281);
	meinfo_hw_register(65282, &meinfo_hw_calix_65282);
	meinfo_hw_register(65283, &meinfo_hw_calix_65283);
	meinfo_hw_register(65311, &meinfo_hw_calix_65311);
	meinfo_hw_register(65315, &meinfo_hw_calix_65315);
	meinfo_hw_register(65317, &meinfo_hw_calix_65317);
	meinfo_hw_register(65318, &meinfo_hw_calix_65318);
	meinfo_hw_register(65319, &meinfo_hw_calix_65319);
	meinfo_hw_register(65501, &meinfo_hw_calix_65501);
	meinfo_hw_register(65502, &meinfo_hw_calix_65502);
	meinfo_hw_register(65503, &meinfo_hw_calix_65503);
	
	return 0;
}

static int 
proprietary_calix_er_group_hw_init(void)
{
	/*er_group_hw_11_47_249.c*/
	er_attr_group_hw_add("l2s_l2_tags_create_add012_pptp_uni", er_group_hw_l2s_l2_tags_create_add012_pptp_uni);
	
	/* er_group_hw_11_264.c */
	er_attr_group_hw_add("rg_group", er_group_hw_rg_group);
	
	/*er_group_hw_47_130_249.c*/
	er_attr_group_hw_add("l2s_l2_tags_create_add012_802_1p", er_group_hw_l2s_l2_tags_create_add012_802_1p);
	
	/*er_group_hw_47_134_249.c*/
	er_attr_group_hw_add("l2s_l2_tags_create_add012_ip_host", er_group_hw_l2s_l2_tags_create_add012_ip_host);
	
	/*er_group_hw_47_249.c*/
	er_attr_group_hw_add("l2s_l2_tags_create_add012_bridge", er_group_hw_l2s_l2_tags_create_add012_bridge);
	
	/*er_group_hw_47_249_329.c*/
	er_attr_group_hw_add("l2s_l2_tags_create_add012_veip", er_group_hw_l2s_l2_tags_create_add012_veip);

	/* er_group_hw_47_148_329_65317.c */
	er_attr_group_hw_add("calix_rg_config_pppauth", er_group_hw_calix_rg_config_pppauth);

	/* er_group_hw_47_329_65317.c */
	er_attr_group_hw_add("calix_rg_config_restore_default", er_group_hw_calix_rg_config_restore_default);
	er_attr_group_hw_add("calix_rg_config_remote_gui_access_time", er_group_hw_calix_rg_config_remote_gui_access_time);
	er_attr_group_hw_add("calix_rg_config", er_group_hw_calix_rg_config);
	er_attr_group_hw_add("calix_rg_registration_id", er_group_hw_calix_rg_registration_id);

	/* er_group_hw_241.c */
	er_attr_group_hw_add("date_time", er_group_hw_date_time);
	er_attr_group_hw_add("date_timezone", er_group_hw_date_timezone);
	er_attr_group_hw_add("telnet_enable", er_group_hw_telnet_enable);
	er_attr_group_hw_add("ookla_disable", er_group_hw_ookla_disable);

	/* er_group_hw_242.c */
	er_attr_group_hw_add("VideoTxMode", er_group_hw_VideoTxMode);
	er_attr_group_hw_add("IfCountReset_242", er_group_hw_IfCountReset_242);
	er_attr_group_hw_add("ForceOperMode", er_group_hw_ForceOperMode);

	/* er_group_hw_243.c */
	er_attr_group_hw_add("primaryconfigserverpointer", er_group_hw_primaryconfigserverpointer);

	/* er_group_hw_248.c */
	er_attr_group_hw_add("video_agc_enable", er_group_hw_video_agc_enable);

	/* er_group_hw_45_47_49_79_251.c */
	er_attr_group_hw_add("l2cp_enable", er_group_hw_l2cp_enable);

	/*er_group_hw_251_171_280.c*/
	er_attr_group_hw_add("uni_policing", er_group_hw_uni_policing);

	/* er_group_hw_53_65283.c */
	er_attr_group_hw_add("voip_call_statistics_extension4", er_group_hw_voip_call_statistics_extension4);

	/* er_group_hw_65287.c */
	er_attr_group_hw_add("oam_config", er_group_hw_oam_config);
	
	/* er_group_hw_65288_65289.c */
	er_attr_group_hw_add("oam_mep", er_group_hw_oam_mep);

	/* er_group_hw_65288_65290.c */
	er_attr_group_hw_add("oam_mip", er_group_hw_oam_mip);

	/* er_group_hw_65288_65289_65291.c */
	er_attr_group_hw_add("oam_mep_linktrace", er_group_hw_oam_mep_linktrace);

	/* er_group_hw_65288_65289_65292.c */
	er_attr_group_hw_add("oam_mep_loopback", er_group_hw_oam_mep_loopback);

	/* er_group_hw_65288_65289_65294.c */
	er_attr_group_hw_add("oam_mep_pm_macaddr", er_group_hw_oam_mep_pm_macaddr);

	/* er_group_hw_65288_65289_65299.c */
	er_attr_group_hw_add("oam_mep_clear_counters", er_group_hw_oam_mep_clear_counters);

	/* er_group_hw_65288_65289_65302.c */
	er_attr_group_hw_add("oam_lm_clear_counters", er_group_hw_oam_lm_clear_counters);

	/* er_group_hw_65288_65289_65303.c */
	er_attr_group_hw_add("oam_dm_clear_counters", er_group_hw_oam_dm_clear_counters);

	/* er_group_hw_65288_65290_65301.c */
	er_attr_group_hw_add("oam_mip_clear_counters", er_group_hw_oam_mip_clear_counters);

	/* er_group_hw_65311.c */
	er_attr_group_hw_add("IfCountReset_65311", er_group_hw_IfCountReset_65311);	

	/* er_group_hw_65313.c */
	er_attr_group_hw_add("calix_vlan_tlan", er_group_hw_calix_vlan_tlan);

	/* er_group_hw_65315.c */
	er_attr_group_hw_add("calix_voip_fqdn", er_group_hw_calix_voip_fqdn);
	
	/* er_group_hw_65318.c */
	er_attr_group_hw_add("max_power_budget", er_group_hw_max_power_budget);
	
	/* er_group_hw_65500.c */
	er_attr_group_hw_add("full_bridge_enable", er_group_hw_full_bridge_enable);
        
	/* er_group_hw_65503.c */
	er_attr_group_hw_add("restore_to_default", er_group_hw_restore_to_default);

	/* er_group_hw_65319.c */
	er_attr_group_hw_add("lldp_admin_state", er_group_hw_lldp_admin_state);
	er_attr_group_hw_add("lldp_network_policy", er_group_hw_lldp_network_policy);
	er_attr_group_hw_add("lldp_vid", er_group_hw_lldp_vid);
	er_attr_group_hw_add("lldp_priority", er_group_hw_lldp_priority);
	er_attr_group_hw_add("lldp_dscp", er_group_hw_lldp_dscp);

	return 0;
}
#endif

int 
proprietary_calix(void)
{
	proprietary_calix_me_related_init();	
	proprietary_calix_meinfo_cb_init();
#ifndef X86
	proprietary_calix_meinfo_hw_init();
	proprietary_calix_er_group_hw_init();
#endif
	return 0;
}
