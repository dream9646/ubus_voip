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
 * File    : batchtab_cb_alu_ont_mode.c
 *
 ******************************************************************/

#include <stdio.h>
#include <string.h>
#include "meinfo.h"
#include "me_related.h"
#include "list.h"
#include "util.h"
#include "util_run.h"
#include "metacfg_adapter.h"
#include "batchtab.h"
#include "batchtab_cb.h"
#include "proprietary_alu.h"

#define METAFILE_PATH	"/etc/wwwctrl/metafile.dat"

static int alu_ont_mode_total_g = 0;

////////////////////////////////////////////////////

static int
batchtab_cb_alu_ont_mode_init(void)
{
	alu_ont_mode_total_g = 0;
	return 0;
}

static int
batchtab_cb_alu_ont_mode_finish(void)
{
	alu_ont_mode_total_g = 0;
	return 0;
}

static int
batchtab_cb_alu_ont_mode_gen(void **table_data)
{
	struct batchtab_t *batptr = batchtab_find_by_name("alu_ont_mode");
	// the tablegen may be triggered by omci update or batchtab unregister
	// however, we sync me256 attr6 to metafile only if me256 attr6 omci update appen
	if (batptr->omci_update_count == 0) {	
		dbprintf_bat(LOG_WARNING, "do nothing as there is no omci update\n");
		return 0;
	}

#ifdef OMCI_METAFILE_ENABLE
	// CHT workaround to switch "ont_mode" via ME256 attribute #6 "Battery_backup"
	if(omci_env_g.olt_workaround_enable == ENV_OLT_WORKAROUND_ALU) {
		struct me_t *meptr = meinfo_me_get_by_meid(256, 0);
#if 1
		struct metacfg_t metacfg_config;
		char *value_ptr;
		unsigned char value, meta_value = 1; // default is 1: router mode
		
		alu_ont_mode_total_g = 0;
		
		memset(&metacfg_config, 0x0, sizeof(struct metacfg_t));
		metacfg_adapter_config_init(&metacfg_config);
		metacfg_adapter_config_file_load_safe(&metacfg_config, METAFILE_PATH);
		if(strlen(value_ptr = metacfg_adapter_config_get_value(&metacfg_config, "ont_mode", 1)) > 0)
			meta_value = util_atoi(value_ptr);
		metacfg_adapter_config_release(&metacfg_config);
#else
		struct kv_config_t kv_config;
		char *value_ptr;
		unsigned char value, meta_value = 1; // default is 1: router mode
		
		alu_ont_mode_total_g = 0;
		
		kv_config_init(&kv_config);
		kv_config_file_load_safe(&kv_config, METAFILE_PATH);
		if(strlen(value_ptr = kv_config_get_value(&kv_config, "ont_mode", 1)) > 0)
			meta_value = util_atoi(value_ptr);
		kv_config_release(&kv_config);
#endif		
		value = meinfo_util_me_attr_data(meptr, 6); // 1: router, 0: bridge
		
		if(meta_value != value) {
			if(value==1) {
				dbprintf_bat(LOG_ERR, "Change ONT mode to router\n");
				util_run_by_system("ont_mode.sh router");
			} else {
				dbprintf_bat(LOG_ERR, "Change ONT mode to bridge\n");
				util_run_by_system("ont_mode.sh bridge");
			}
		}
		alu_ont_mode_total_g++;
	}
#endif
	*table_data = &alu_ont_mode_total_g;
	
	return 0;
}

static int
batchtab_cb_alu_ont_mode_release(void *table_data)
{
	return 0;
}

static int
batchtab_cb_alu_ont_mode_dump(int fd, void *table_data)
{
	int *total = table_data;
	struct me_t *me_131 = meinfo_me_get_by_instance_num(131, 0);
	struct me_t *me_256 = meinfo_me_get_by_meid(256, 0);

	util_fdprintf(fd, "update total = %d\n", (table_data) ? *total : 0);
		
	if (me_256) {
		
		
#if 1
#ifdef OMCI_METAFILE_ENABLE
		struct metacfg_t kv_config;
		unsigned char omci_value = meinfo_util_me_attr_data(me_256, 6); // 1: router, 0: bridge
		
		char *meta_value;

		memset(&kv_config, 0x0, sizeof(struct metacfg_t));
		metacfg_adapter_config_init(&kv_config);
		metacfg_adapter_config_file_load_part(&kv_config, METAFILE_PATH, "ont_mode", "ont_mode");
		meta_value = metacfg_adapter_config_get_value(&kv_config, "ont_mode", 1);
		util_fdprintf(fd, "metafile ont_mode=%s\n", meta_value);
		util_fdprintf(fd, "me256 attr6 ont_mode=%d\n", omci_value);
		metacfg_adapter_config_release(&kv_config);
#endif
#else
		struct kv_config_t kv_config;
		unsigned char omci_value = meinfo_util_me_attr_data(me_256, 6); // 1: router, 0: bridge
		
		char *meta_value;
		kv_config_init(&kv_config);
		kv_config_file_load_part(&kv_config, METAFILE_PATH, "ont_mode", "ont_mode");
		meta_value = kv_config_get_value(&kv_config, "ont_mode", 1);
		util_fdprintf(fd, "metafile ont_mode=%s\n", meta_value);
		util_fdprintf(fd, "me256 attr6 ont_mode=%d\n", omci_value);
		kv_config_release(&kv_config);
#endif		
	}
	//get olt-g to check version
	if (me_131 && me_131->attr[3].value.ptr) {
		unsigned short olt_base = (unsigned short) util_bitmap_get_value(me_131->attr[3].value.ptr, 8*14, 8*4, 16);	// byte 4,5
		unsigned int olt_release = (unsigned int) util_bitmap_get_value(me_131->attr[3].value.ptr, 8*14, 8*0, 32);	// byte 0,1,2,3
		char *olt_model = "unknown";
		if (olt_base == 0 || olt_base == 1)
			olt_model = "alu7342";
		else if (olt_base == 2)
			olt_model = "alu7360";
		util_fdprintf(fd, "olt base = %d (%s)\n", olt_base, olt_model);
		util_fdprintf(fd, "olt fw release = 0x%x\n", olt_release);
	}
	return 0;
}

static int
batchtab_cb_alu_ont_mode_hw_sync(void *table_data)
{
	batchtab_omci_update("tm");
	return 0;
}

// public register function /////////////////////////////////////
int
batchtab_cb_alu_ont_mode_register(void)
{
	return batchtab_register("alu_ont_mode", BATCHTAB_OMCI_UPDATE_NOAUTO, 3,
		batchtab_cb_alu_ont_mode_init,
		batchtab_cb_alu_ont_mode_finish,
		batchtab_cb_alu_ont_mode_gen,
		batchtab_cb_alu_ont_mode_release,
		batchtab_cb_alu_ont_mode_dump,
		batchtab_cb_alu_ont_mode_hw_sync);
}

