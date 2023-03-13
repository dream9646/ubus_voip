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
 * Module  : er_group_hw
 * File    : er_group_hw_290_291.c
 *
 ******************************************************************/
#include <string.h>

#include "util.h"
#include "util_run.h"
#include "switch.h"
#include "meinfo.h"
#include "er_group.h"
#include "hwresource.h"
#include "er_group_hw_util.h"

// 290 Dot1X_port_extension_package
// 291 Dot1X_configuration_profile

//290@inst,1 [0~1]
//291@3,4,5,6,7,8 [2~7]

int
er_group_hw_dot1x_config(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	struct er_attr_group_instance_t *current_attr_inst;
	unsigned char instance_num, enable;
	unsigned char *server1 = NULL, *server2 = NULL, *server3 = NULL;
	unsigned char secret1[25], secret2[25], secret3[25];
	unsigned char large_string1[376], large_string2[376], large_string3[376];
	unsigned char devname[16];
#ifdef OMCI_METAFILE_ENABLE
	unsigned char *p, meta_start[32], meta_end[32], key[32], cmd[128];
#endif
	struct me_t *str_me1 = NULL, *str_me2 = NULL, *str_me3 = NULL;
	struct me_t *dot1x_me = NULL, *parent_me = NULL;
	struct switch_port_info_t port_info;
#ifdef OMCI_METAFILE_ENABLE
	struct metacfg_t kv_config;
	int value = -1;
#endif
	
	if (attr_inst == NULL) {
       		dbprintf(LOG_ERR,"attr_inst NULL\n");
		return -1;
	}

	dot1x_me = er_attr_group_util_attrinst2me(attr_inst, 0);
	if(dot1x_me==NULL)
		return -1;
	parent_me = meinfo_me_get_by_meid(11, dot1x_me->meid);
	if (parent_me==NULL)
		return -1;
	if (switch_get_port_info_by_me(parent_me, &port_info) < 0) {
		if (parent_me!=NULL)
			dbprintf(LOG_WARNING, "get port info error:classid=%u, meid=%u\n", parent_me->classid, parent_me->meid);
		return -1;
	}
	// only apply to uni port
	if(!((1<<port_info.logical_port_id) & switch_get_uni_logical_portmask()))
		return -1;
	
	switch (port_info.logical_port_id) {
		case 0:
			strcpy(devname, "eth0.2");
			break;
		case 1:
			strcpy(devname, "eth0.3");
			break;
		case 2:
			strcpy(devname, "eth0.4");
			break;
		case 3:
			strcpy(devname, "eth0.5");
			break;
		default:
			memset(devname, 0, sizeof(devname));
			break;
	}
	
	instance_num = (unsigned short) attr_inst->er_attr_instance[0].attr_value.data;
	enable = (unsigned char) attr_inst->er_attr_instance[1].attr_value.data;
	memcpy(secret1, (unsigned char *) attr_inst->er_attr_instance[3].attr_value.ptr, sizeof(secret1));
	memcpy(secret2, (unsigned char *) attr_inst->er_attr_instance[5].attr_value.ptr, sizeof(secret2));
	memcpy(secret3, (unsigned char *) attr_inst->er_attr_instance[7].attr_value.ptr, sizeof(secret3));
	
	if ((str_me1 = meinfo_me_get_by_meid(157, (unsigned short)attr_inst->er_attr_instance[2].attr_value.data)) == NULL) {
		server1 = NULL;
	} else {
		memset(large_string1, 0x00, sizeof(large_string1));
		meinfo_me_copy_from_large_string_me(str_me1, large_string1, 375);
		server1 = large_string1;
	}
	
	if ((str_me2 = meinfo_me_get_by_meid(157, (unsigned short)attr_inst->er_attr_instance[4].attr_value.data)) == NULL) {
		server2 = NULL;
	} else {
		memset(large_string2, 0x00, sizeof(large_string2));
		meinfo_me_copy_from_large_string_me(str_me2, large_string2, 375);
		server2 = large_string2;
	}
	
	if ((str_me3 = meinfo_me_get_by_meid(157, (unsigned short)attr_inst->er_attr_instance[6].attr_value.data)) == NULL) {
		server3 = NULL;
	} else {
		memset(large_string3, 0x00, sizeof(large_string3));
		meinfo_me_copy_from_large_string_me(str_me3, large_string3, 375);
		server3 = large_string3;
	}
	
	switch(action)
	{
	case ER_ATTR_GROUP_HW_OP_ADD:
#ifdef OMCI_METAFILE_ENABLE
		snprintf(meta_start, 31, "dot1x_management_capability");
		snprintf(meta_end, 31, "dot1x3_server_shared_secret");
		memset(&kv_config, 0x0, sizeof(struct metacfg_t));
		metacfg_adapter_config_init(&kv_config);
		metacfg_adapter_config_file_load_part(&kv_config, "/etc/wwwctrl/metafile.dat", meta_start, meta_end);	
		strcpy(key, "dot1x_management_capability");
		p = metacfg_adapter_config_get_value(&kv_config, key, 1);
		if(p) value = util_atoi(p);
		if(value == 0) {
			sprintf(key, "dot1x%d_enable", instance_num);
			p = metacfg_adapter_config_get_value(&kv_config, key, 0);
			if(p) {
#ifdef OMCI_CMD_DISPATCHER_ENABLE
				snprintf(cmd, 127, "/etc/init.d/cmd_dispatcher.sh set dot1x%d_enable=%d", instance_num, enable);
				util_run_by_thread(cmd, 1);
#endif
				util_fdprintf(util_dbprintf_get_console_fd(), "dot1x%d_enable=%d\n", instance_num, enable);
			}
			sprintf(key, "dot1x%d_interface", instance_num);
			p = metacfg_adapter_config_get_value(&kv_config, key, 0);
			if(p && enable) {
#ifdef OMCI_CMD_DISPATCHER_ENABLE
				snprintf(cmd, 127, "/etc/init.d/cmd_dispatcher.sh set dot1x%d_interface=%s", instance_num, devname);
				util_run_by_thread(cmd, 1);
#endif
				util_fdprintf(util_dbprintf_get_console_fd(), "dot1x%d_interface=%s\n", instance_num, devname);
			}
			strcpy(key, "dot1x0_server_addr");
			p = metacfg_adapter_config_get_value(&kv_config, key, 0);
			if (p && server1) {
				char *ip = strtok(server1, ":");
				char *port = strtok(NULL, "");

#ifdef OMCI_CMD_DISPATCHER_ENABLE
				snprintf(cmd, 127, "/etc/init.d/cmd_dispatcher.sh set dot1x0_server_addr=%s", ip);
				util_run_by_thread(cmd, 1);
#endif
				util_fdprintf(util_dbprintf_get_console_fd(), "dot1x0_server_addr=%s\n", ip);
				if(port) {
#ifdef OMCI_CMD_DISPATCHER_ENABLE
					snprintf(cmd, 127, "/etc/init.d/cmd_dispatcher.sh set dot1x0_server_port=%s", port);
					util_run_by_thread(cmd, 1);
#endif
					util_fdprintf(util_dbprintf_get_console_fd(), "dot1x0_server_port=%s\n", port);
				}
			}
			strcpy(key, "dot1x0_server_shared_secret");
			p = metacfg_adapter_config_get_value(&kv_config, key, 0);
			if (p && secret1[0] != 0) {
#ifdef OMCI_CMD_DISPATCHER_ENABLE
				snprintf(cmd, 127, "/etc/init.d/cmd_dispatcher.sh set dot1x0_server_shared_secret=%s", secret1);
				util_run_by_thread(cmd, 1);
#endif
				util_fdprintf(util_dbprintf_get_console_fd(), "dot1x0_server_shared_secret=%s\n", secret1);
			}
			strcpy(key, "dot1x1_server_addr");
			p = metacfg_adapter_config_get_value(&kv_config, key, 0);
			if (p && server2) {
				char *ip = strtok(server2, ":");
				char *port = strtok(NULL, "");
#ifdef OMCI_CMD_DISPATCHER_ENABLE
				snprintf(cmd, 127, "/etc/init.d/cmd_dispatcher.sh set dot1x1_server_addr=%s", ip);
				util_run_by_thread(cmd, 1);
#endif
				util_fdprintf(util_dbprintf_get_console_fd(), "dot1x1_server_addr=%s\n", ip);
				if(port) {
#ifdef OMCI_CMD_DISPATCHER_ENABLE
					snprintf(cmd, 127, "/etc/init.d/cmd_dispatcher.sh set dot1x1_server_port=%s", port);
					util_run_by_thread(cmd, 1);
#endif
					util_fdprintf(util_dbprintf_get_console_fd(), "dot1x1_server_port=%s\n", port);
				}
			}
			strcpy(key, "dot1x1_server_shared_secret");
			p = metacfg_adapter_config_get_value(&kv_config, key, 0);
			if (p && secret2[0] != 0) {
#ifdef OMCI_CMD_DISPATCHER_ENABLE
				snprintf(cmd, 127, "/etc/init.d/cmd_dispatcher.sh set dot1x1_server_shared_secret=%s", secret2);
				util_run_by_thread(cmd, 1);
#endif
				util_fdprintf(util_dbprintf_get_console_fd(), "dot1x1_server_shared_secret=%s\n", secret2);
			}
			strcpy(key, "dot1x2_server_addr");
			p = metacfg_adapter_config_get_value(&kv_config, key, 0);
			if (p && server3) {
				char *ip = strtok(server3, ":");
				char *port = strtok(NULL, "");
#ifdef OMCI_CMD_DISPATCHER_ENABLE
				snprintf(cmd, 127, "/etc/init.d/cmd_dispatcher.sh set dot1x2_server_addr=%s", ip);
				util_run_by_thread(cmd, 1);
#endif
				util_fdprintf(util_dbprintf_get_console_fd(), "dot1x2_server_addr=%s\n", ip);
				if(port) {
#ifdef OMCI_CMD_DISPATCHER_ENABLE
					snprintf(cmd, 127, "/etc/init.d/cmd_dispatcher.sh set dot1x2_server_port=%s", port);
					util_run_by_thread(cmd, 1);
#endif
					util_fdprintf(util_dbprintf_get_console_fd(), "dot1x2_server_port=%s\n", port);
				}
			}
			strcpy(key, "dot1x2_server_shared_secret");
			p = metacfg_adapter_config_get_value(&kv_config, key, 0);
			if (p && secret3[0] != 0) {
				snprintf(cmd, 127, "/etc/init.d/cmd_dispatcher.sh set dot1x2_server_shared_secret=%s", secret3);
				util_run_by_thread(cmd, 1);
				util_fdprintf(util_dbprintf_get_console_fd(), "dot1x2_server_shared_secret=%s\n", secret3);
			}
		}
		metacfg_adapter_config_release(&kv_config);
#endif
		break;
	case ER_ATTR_GROUP_HW_OP_DEL:
		break;
	case ER_ATTR_GROUP_HW_OP_UPDATE:
		//delete
		if (er_group_hw_dot1x_config(ER_ATTR_GROUP_HW_OP_DEL, attr_inst, me, attr_mask) < 0)
		{
			dbprintf(LOG_ERR, "fail to update for deleting\n");
			return -1;
		}

		//get current value 
		if ((current_attr_inst = er_attr_group_util_gen_current_value_attrinst(attr_inst)) == NULL)
		{
			dbprintf(LOG_ERR, "fail to gen current value attr inst\n");
			return -1;
		}

		//add
		if (er_group_hw_dot1x_config(ER_ATTR_GROUP_HW_OP_ADD, current_attr_inst, me, attr_mask) < 0)
		{
			er_attr_group_util_release_attrinst(current_attr_inst);
			dbprintf(LOG_ERR, "fail to update for adding\n");
			return -1;
		}

		er_attr_group_util_release_attrinst(current_attr_inst);

		break;
	default:
       		dbprintf(LOG_ERR,"Unknown operator\n");
		break;
	}
	return 0;
}
