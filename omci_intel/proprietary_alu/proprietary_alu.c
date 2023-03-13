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
 * Module  : proprietary_alu
 * File    : proprietary_alu.c
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
#include "util.h"
#include "metacfg_adapter.h"
#include "proprietary_alu.h"
#include "proprietary_alu_remote_debug_session.h"

extern struct meinfo_callback_t meinfo_cb_alu_65281;
extern struct meinfo_callback_t meinfo_cb_alu_65282;
extern struct meinfo_callback_t meinfo_cb_alu_65286;
extern struct meinfo_callback_t meinfo_cb_alu_65292;
extern struct meinfo_callback_t meinfo_cb_alu_65295;
extern struct meinfo_callback_t meinfo_cb_alu_65296;
extern struct meinfo_callback_t meinfo_cb_alu_65297;
extern struct meinfo_callback_t meinfo_cb_alu_65299;
extern struct meinfo_callback_t meinfo_cb_alu_65300;
extern struct meinfo_callback_t meinfo_cb_alu_65301;
extern struct meinfo_callback_t meinfo_cb_alu_65302;
extern struct meinfo_callback_t meinfo_cb_alu_65305;
extern struct meinfo_callback_t meinfo_cb_alu_65306;
extern struct meinfo_callback_t meinfo_cb_alu_65308;
extern struct meinfo_callback_t meinfo_cb_alu_65309;
extern struct meinfo_callback_t meinfo_cb_alu_65310;
extern struct meinfo_callback_t meinfo_cb_alu_65312;
extern struct meinfo_callback_t meinfo_cb_alu_65313;
extern struct meinfo_callback_t meinfo_cb_alu_65323;
extern struct meinfo_callback_t meinfo_cb_alu_65530;
extern struct meinfo_callback_t meinfo_cb_alu_65531;

#ifndef X86
extern struct meinfo_hardware_t meinfo_hw_alu_65281;
extern struct meinfo_hardware_t meinfo_hw_alu_65282;
extern struct meinfo_hardware_t meinfo_hw_alu_65295;
extern struct meinfo_hardware_t meinfo_hw_alu_65296;
extern struct meinfo_hardware_t meinfo_hw_alu_65304;
extern struct meinfo_hardware_t meinfo_hw_alu_65312;
extern struct meinfo_hardware_t meinfo_hw_alu_65313;
#endif

//0: old version, 1: new version
unsigned int
proprietary_alu_get_olt_version(void)
{
	struct me_t *me_131;
	unsigned short olt_base;

	//get olt-g to check version
	if ((me_131 = meinfo_me_get_by_instance_num(131, 0)) == NULL ||
		me_131->attr[3].value.ptr == NULL)
	{
		dbprintf(LOG_ERR, "get 131 me error\n");
		return -1;
	}

	//get version byte 4 and 5
	olt_base = (unsigned short) util_bitmap_get_value(me_131->attr[3].value.ptr, 8*14, 8*4, 16);

	if (olt_base == 0x0000 || olt_base == 0x0001)
	{
		return 0; //old
	} else { //0x0002
		return 1; //new
	}
}

unsigned int
proprietary_alu_get_olt_release(void)
{
	struct me_t *me_131;

	//get olt-g to check version
	if ((me_131 = meinfo_me_get_by_instance_num(131, 0)) == NULL ||
		me_131->attr[3].value.ptr == NULL)
	{
		dbprintf(LOG_ERR, "get 131 me error\n");
		return -1;
	}

	//get version byte 0 to 3
	return ((unsigned int) util_bitmap_get_value(me_131->attr[3].value.ptr, 8*14, 8*0, 32));
}

//0: bridge mode, 1: router mode
unsigned int
proprietary_alu_get_ont_mode(void)
{
	unsigned char ont_mode = 1; // default is router mode
#ifdef OMCI_METAFILE_ENABLE
	struct metacfg_t kv_config;
	char *value_ptr;
	
	memset(&kv_config, 0x0, sizeof(struct metacfg_t));
	metacfg_adapter_config_init(&kv_config);
	
	if(metacfg_adapter_config_file_load_safe(&kv_config, "/etc/wwwctrl/metafile.dat") !=0)
	{
		dbprintf(LOG_ERR, "load kv_config error\n");
		metacfg_adapter_config_release(&kv_config);
		return -1;
	}
	
	if(strlen(value_ptr = metacfg_adapter_config_get_value(&kv_config, "ont_mode", 1)) > 0)
		ont_mode = util_atoi(value_ptr);
	
	metacfg_adapter_config_release(&kv_config);
#endif
	return ont_mode;
}

static int
proprietary_alu_me_related_init(void)
{
	me_related_add(ME_RELATED_FUNC_SPEC, 11, 65297, me_related_011_65297);
	me_related_add(ME_RELATED_FUNC_SPEC, 65297, 11, me_related_65297_011);
	
	me_related_add(ME_RELATED_FUNC_SPEC, 11, 65301, me_related_011_65301);
	me_related_add(ME_RELATED_FUNC_SPEC, 65301, 11, me_related_65301_011);
	
	me_related_add(ME_RELATED_FUNC_SPEC, 11, 65305, me_related_011_65305);
	me_related_add(ME_RELATED_FUNC_SPEC, 65305, 11, me_related_65305_011);
	
	me_related_add(ME_RELATED_FUNC_SPEC, 11, 65306, me_related_011_65306);
	me_related_add(ME_RELATED_FUNC_SPEC, 65306, 11, me_related_65306_011);
	
	me_related_add(ME_RELATED_FUNC_SPEC, 53, 65292, me_related_053_65292);
	me_related_add(ME_RELATED_FUNC_SPEC, 65292, 53, me_related_65292_053);
	
	me_related_add(ME_RELATED_FUNC_SPEC, 277, 65323, me_related_277_65323);
	me_related_add(ME_RELATED_FUNC_SPEC, 65323, 277, me_related_65323_277);
	
	me_related_add(ME_RELATED_FUNC_SPEC, 280, 65323, me_related_280_65323);
	me_related_add(ME_RELATED_FUNC_SPEC, 65323, 280, me_related_65323_280);
	
	me_related_add(ME_RELATED_FUNC_SPEC, 310, 65531, me_related_310_65531);
	me_related_add(ME_RELATED_FUNC_SPEC, 65531, 310, me_related_65531_310);
	
	me_related_add(ME_RELATED_FUNC_SPEC, 329, 65297, me_related_329_65297);
	me_related_add(ME_RELATED_FUNC_SPEC, 65297, 329, me_related_65297_329);
	
	me_related_add(ME_RELATED_FUNC_SPEC, 329, 65301, me_related_329_65301);
	me_related_add(ME_RELATED_FUNC_SPEC, 65301, 329, me_related_65301_329);
	
	me_related_add(ME_RELATED_FUNC_SPEC, 329, 65305, me_related_329_65305);
	me_related_add(ME_RELATED_FUNC_SPEC, 65305, 329, me_related_65305_329);
	
	me_related_add(ME_RELATED_FUNC_SPEC, 329, 65306, me_related_329_65306);
	me_related_add(ME_RELATED_FUNC_SPEC, 65306, 329, me_related_65306_329);
	
	return 0;
}

static int
proprietary_alu_meinfo_cb_init(void)
{
	meinfo_cb_register(65281, &meinfo_cb_alu_65281);
	meinfo_cb_register(65282, &meinfo_cb_alu_65282);
	meinfo_cb_register(65286, &meinfo_cb_alu_65286);
	meinfo_cb_register(65292, &meinfo_cb_alu_65292);
	meinfo_cb_register(65295, &meinfo_cb_alu_65295);
	meinfo_cb_register(65296, &meinfo_cb_alu_65296);
	meinfo_cb_register(65297, &meinfo_cb_alu_65297);
	meinfo_cb_register(65299, &meinfo_cb_alu_65299);
	meinfo_cb_register(65300, &meinfo_cb_alu_65300);
	meinfo_cb_register(65301, &meinfo_cb_alu_65301);
	meinfo_cb_register(65302, &meinfo_cb_alu_65302);
	meinfo_cb_register(65305, &meinfo_cb_alu_65305);
	meinfo_cb_register(65306, &meinfo_cb_alu_65306);
	meinfo_cb_register(65308, &meinfo_cb_alu_65308);
	meinfo_cb_register(65309, &meinfo_cb_alu_65309);
	meinfo_cb_register(65310, &meinfo_cb_alu_65310);
	meinfo_cb_register(65312, &meinfo_cb_alu_65312);
	meinfo_cb_register(65313, &meinfo_cb_alu_65313);
	meinfo_cb_register(65323, &meinfo_cb_alu_65323);
	meinfo_cb_register(65530, &meinfo_cb_alu_65530);
	meinfo_cb_register(65531, &meinfo_cb_alu_65531);
	
	return 0;
}

#ifndef X86
static int
proprietary_alu_meinfo_hw_init(void)
{
	meinfo_hw_register(65281, &meinfo_hw_alu_65281);
	meinfo_hw_register(65282, &meinfo_hw_alu_65282);
	meinfo_hw_register(65295, &meinfo_hw_alu_65295);
	meinfo_hw_register(65296, &meinfo_hw_alu_65296);
	meinfo_hw_register(65304, &meinfo_hw_alu_65304);
	meinfo_hw_register(65312, &meinfo_hw_alu_65312);
	meinfo_hw_register(65313, &meinfo_hw_alu_65313);
	
	return 0;
}

static int 
proprietary_alu_er_group_hw_init(void)
{
	/* er_group_hw_remote_debug_session.c */
	er_attr_group_hw_add("remote_debug_session", er_group_hw_remote_debug_session);

	/* er_group_hw_65296.c */
	er_attr_group_hw_add("port2port_enable", er_group_hw_port2port_enable);
	er_attr_group_hw_add("slid_capability", er_group_hw_slid_capability);
	er_attr_group_hw_add("invert_weight_enable", er_group_hw_invert_weight_enable);
	er_attr_group_hw_add("reset_to_default", er_group_hw_reset_to_default);
	
	/* er_group_hw_65301.c */
	er_attr_group_hw_add("anti_spoof_list", er_group_hw_anti_spoof_list);

	/* er_group_hw_65302.c */
	er_attr_group_hw_add("rf_filter", er_group_hw_rf_filter);
	
	/* er_group_hw_65304.c */
	er_attr_group_hw_add("current_time", er_group_hw_current_time);
	er_attr_group_hw_add("ntp_time", er_group_hw_ntp_time);
	er_attr_group_hw_add("l2s_tags_create_add01_multicast_65531", er_group_hw_l2s_tags_create_add01_multicast_65531);

	/* er_group_hw_11_47_65297.c */
	er_attr_group_hw_add("l2s_tags_create_add012_ds_supplemental_1_old", er_group_hw_l2s_tags_create_add012_ds_supplemental_1_old);

	/* er_group_hw_11_47_65305.c */
	er_attr_group_hw_add("l2s_tags_create_add012_ds_supplemental_1", er_group_hw_l2s_tags_create_add012_ds_supplemental_1);

	/* er_group_hw_47_329_65297.c */
	er_attr_group_hw_add("l2s_tags_create_add012_veip_ds_supplemental_1_old", er_group_hw_l2s_tags_create_add012_veip_ds_supplemental_1_old);

	/* er_group_hw_47_329_65305.c */
	er_attr_group_hw_add("l2s_tags_create_add012_veip_ds_supplemental_1", er_group_hw_l2s_tags_create_add012_veip_ds_supplemental_1);

	/*er_group_hw_65297*/
	er_attr_group_hw_add("igmp_access_control_method", er_group_hw_igmp_access_control_method);
	er_attr_group_hw_add("downstream_tag_handling_mode", er_group_hw_downstream_tag_handling_mode);

	/* er_group_hw_310.c */
	er_attr_group_hw_add("alu_old_olt_multicast_workaround",er_group_hw_vid_to_ethertype);

	/* er_group_hw_256.c */
	er_attr_group_hw_add("change_ont_mode", er_group_hw_change_ont_mode);
	
	/* er_group_hw_171.c */
	er_attr_group_hw_add("bundle_rate", er_group_hw_bundle_rate);

	/* er_group_hw_133.c */
	er_attr_group_hw_add("mac_limit_learning", er_group_hw_mac_limit_learning);
	
	/* er_group_hw_277_280_65323.c */
	er_attr_group_hw_add("mac_traffic_control_td_us_pq", er_group_hw_mac_traffic_control_td_us_pq);
	
	return 0;
}
#endif

int 
proprietary_alu(void)
{
	proprietary_alu_me_related_init();
	proprietary_alu_meinfo_cb_init();
#ifndef X86
	proprietary_alu_meinfo_hw_init();
	proprietary_alu_er_group_hw_init();
#endif

	return 0;
}
