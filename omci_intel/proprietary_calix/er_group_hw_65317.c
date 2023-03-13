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
 * File    : er_group_hw_65317.c
 *
 ******************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"
#include "util_run.h"
#include "meinfo.h"
#include "er_group.h"
#include "batchtab.h"
#include "batchtab_cb.h"
#include "env.h"

// trigger batchtab_cb_wanif with proper attr_mask
int
er_group_hw_calix_rg_config_with_batchtab(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	switch (action) {
	case ER_ATTR_GROUP_HW_OP_ADD:
		batchtab_cb_wanif_me_update(me, attr_mask);
		batchtab_omci_update("wanif");		
		break;		
	case ER_ATTR_GROUP_HW_OP_UPDATE:		
		batchtab_cb_wanif_me_update(me, attr_mask);
		batchtab_omci_update("wanif");		
		break;
	case ER_ATTR_GROUP_HW_OP_DEL:
		break;
	default:
		dbprintf(LOG_ERR,"Unknown operator\n");
	}

	return 0;
}

//65317@7
int
er_group_hw_calix_rg_config_restore_default(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	if (action == ER_ATTR_GROUP_HW_OP_ADD || action == ER_ATTR_GROUP_HW_OP_UPDATE) {
		// reset to default
		unsigned char restore_default = (unsigned char)meinfo_util_me_attr_data(me, 7);
		if (restore_default == 1) {
			dbprintf(LOG_ERR, "calix_rg_config 0x%x, restore_default=%d, restore factory default & reboot...\n",
				me->meid, restore_default);
			dbprintf(LOG_ERR, "SYSTEM FACTORY RESET\n");
			util_run_by_system("rm -Rf /overlay/*; reboot");
		}
	}
	return 0;
}
//65317@9
int
er_group_hw_calix_rg_config_remote_gui_access_time(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	if (action == ER_ATTR_GROUP_HW_OP_ADD || action == ER_ATTR_GROUP_HW_OP_UPDATE) {
		// set remote_gui_access_time
		static long remote_gui_access_time_prev = 0;
		long remote_gui_access_time = (unsigned int)meinfo_util_me_attr_data(me, 9);

		if (remote_gui_access_time != remote_gui_access_time_prev) {
			if (remote_gui_access_time) {
				struct tm *t=(struct tm *)localtime(&remote_gui_access_time);
				dbprintf(LOG_ERR, "calix_rg_config 0x%x, remote_gui_access_time=0x%x (allowed before %04d/%02d/%02d %02d:%02d:%02d)\n",
			                me->meid, remote_gui_access_time, t->tm_year+1900, t->tm_mon+1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);
			} else {
				dbprintf(LOG_ERR, "calix_rg_config 0x%x, remote_gui_access_time=0 (disabled)\n", me->meid);
			}
			remote_gui_access_time_prev = remote_gui_access_time;

			// this feature is actually implemented in remote_gui_access_control.sh
			util_run_by_system("/etc/init.d/remote_gui_access_control.sh start");
		}
	}
	return 0;
}
//65317@meid,65317@1,65317@2,65317@6,65317@8,65317@3,65317@4,65317@5,65317@10,65317@11
int
er_group_hw_calix_rg_config(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	return er_group_hw_calix_rg_config_with_batchtab(action, attr_inst, me, attr_mask);
}
//65317@13
int
er_group_hw_calix_rg_registration_id(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	if (action == ER_ATTR_GROUP_HW_OP_ADD || action == ER_ATTR_GROUP_HW_OP_UPDATE) {
		// registration_id is actually the onu_passwd
		char *new_passwd = (char *)meinfo_util_me_attr_ptr(me, 13);
#ifdef OMCI_METAFILE_ENABLE
		struct metacfg_t kv_config;
#endif

		if (strlen(new_passwd) <= 0)
			return -1;

		if (gpon_hw_g.onu_password_set) {
			if (gpon_hw_g.onu_password_set(new_passwd) <0)
				return -1;
		}

#ifdef OMCI_METAFILE_ENABLE
		memset(&kv_config, 0x0, sizeof(struct metacfg_t));
		metacfg_adapter_config_init(&kv_config);
		if (metacfg_adapter_config_file_load_safe(&kv_config, GPON_DAT_FILE) !=0) {
			dbprintf(LOG_ERR, "load kv_config error %s\n", GPON_DAT_FILE);
			metacfg_adapter_config_release(&kv_config);
			return -1;
		}
			
		metacfg_adapter_config_entity_update(&kv_config, "onu_password", new_passwd);
		metacfg_adapter_config_file_save(&kv_config, GPON_DAT_FILE);
		metacfg_adapter_config_release(&kv_config);
		util_run_by_thread("/usr/sbin/nvram.sh commit gpon.dat", 0);	// commit in bg thread
#endif
	}

	return 0;
}
