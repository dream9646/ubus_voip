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
 * File    : er_group_hw_53_134_136_150_153.c
 *
 ******************************************************************/

#include <stdio.h>
#include <string.h>
#include "util.h"
#include "util_run.h"
#include "metacfg_adapter.h"
#include "meinfo.h"
#include "er_group.h"
#include "hwresource.h"
#include "er_group_hw.h"
#include "er_group_hw_util.h"
#include "batchtab.h"

char voip_interface_name_g[26]="";	//export for other function
#define SET_BIT(mask, bit)			\
	{					\
		unsigned char *data;		\
		int index;			\
		if ((bit) > 7){			\
			index = 15 - (bit);	\
			data = &(mask)[1];	\
		} else {			\
			index = 7 - (bit);	\
			data = &(mask)[0];	\
		}				\
		*data |= (1<<index);		\
	}

// 53 Physical_path_termination_point_POTS_UNI
// 134 IP_host_config_data
// 136 TCP/UDP_config_data
// 150 SIP_agent_config_data
// 153 SIP_user_data

//53@254,134@9,136@1,136@2,136@3,150@1,150@2,150@3,150@4,150@5,150@6,150@7,150@8,150@9,150@10,150@11
int
er_group_hw_sip_agent_config_data(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	unsigned short port_id;
	int ret;
	unsigned char iphost_attr_mask[2] = {0, 0};
	struct voip_hw_sip_agent_config_data_t sacd;
	struct me_t *temp_me1, *temp_me2;
	unsigned char large_string1[376], large_string2[376], large_string3[376], large_string4[376];
	struct me_t *ip_host_config_data_me;
	struct voip_hw_authentication_security_method_t authentication_security_method1;
	struct voip_hw_tcp_udp_config_data_t tcp_udp_config_data;
	struct voip_hw_ip_host_config_data_t ip_host_config_data;
	struct voip_hw_network_address_t network_address;
	struct attr_value_t attr;
	struct er_attr_group_instance_t *current_attr_inst;
#ifdef OMCI_METAFILE_ENABLE
	struct meinfo_instance_def_t *instance_def_ptr;
#endif

	if (attr_inst == NULL)
		return -1;

	dbprintf_voip(LOG_WARNING, "action:%d(%s), me=%d:0x%x, attr_mask=0x%02x%02x\n",
		action, er_group_hw_util_action_str(action), me->classid, me->meid, attr_mask[0], attr_mask[1]);

	port_id = attr_inst->er_attr_instance[0].attr_value.data;

	memset(&sacd, 0x00, sizeof(sacd));
	memset(&tcp_udp_config_data, 0x00, sizeof(tcp_udp_config_data));
	memset(&ip_host_config_data, 0x00, sizeof(ip_host_config_data));

	//ip address
	//refresh 134 me
	if ((ip_host_config_data_me = er_attr_group_util_attrinst2me(attr_inst, 1)) == NULL) {
		dbprintf_voip(LOG_ERR, "could not get ip host config me, classid=%u, meid=%u\n",
			attr_inst->er_attr_group_definition_ptr->er_attr[1].classid, attr_inst->er_attr_instance[1].meid);
		return -1;
	}
	util_attr_mask_set_bit(iphost_attr_mask, 9);
	if (meinfo_me_refresh(ip_host_config_data_me, iphost_attr_mask)< 0) {
		dbprintf_voip(LOG_ERR, "could not refresh ip host config me, classid=%u, meid=%u\n",
			ip_host_config_data_me->classid, ip_host_config_data_me->meid);
		return -1;
	}
	meinfo_me_attr_get(ip_host_config_data_me, 9, &attr);
	ip_host_config_data.ip_address = attr.data;

#ifdef OMCI_METAFILE_ENABLE
	instance_def_ptr = meinfo_instance_find_def(ip_host_config_data_me->classid, ip_host_config_data_me->instance_num);
        if (instance_def_ptr && strlen(instance_def_ptr->devname)>0) {
		char cmd[128], *p;
		struct metacfg_t kv_config;
		strcpy(voip_interface_name_g, instance_def_ptr->devname);
		snprintf(cmd, 127, "voip_ep%d_interface_name", port_id);
		memset(&kv_config, 0x0, sizeof(struct metacfg_t));
		metacfg_adapter_config_init(&kv_config);
		metacfg_adapter_config_file_load_part(&kv_config, "/etc/wwwctrl/metafile.dat", cmd, cmd);
		if (NULL != (p=metacfg_adapter_config_get_value(&kv_config, cmd, 0)) &&
		    0 != strcmp(p, instance_def_ptr->devname)) {
#ifdef OMCI_CMD_DISPATCHER_ENABLE
			snprintf(cmd, 127, "/etc/init.d/cmd_dispatcher.sh set voip_ep%d_interface_name=%s", port_id, instance_def_ptr->devname);
			util_run_by_thread(cmd, 1);
#endif
			util_fdprintf(util_dbprintf_get_console_fd(), "voip_ep%d_interface_name=%s\n", port_id, instance_def_ptr->devname);
		}
		metacfg_adapter_config_release(&kv_config);
	}
#endif
	//port id
	tcp_udp_config_data.port_id = (unsigned int) attr_inst->er_attr_instance[2].attr_value.data;

	//protocol
	tcp_udp_config_data.protocol = (unsigned char) attr_inst->er_attr_instance[3].attr_value.data;

	//tos diffserv field
	tcp_udp_config_data.tos_diffserv_field = (unsigned char) attr_inst->er_attr_instance[4].attr_value.data;

	//proxy server address server
	if ((temp_me1 = meinfo_me_get_by_meid(157, (unsigned short)attr_inst->er_attr_instance[5].attr_value.data)) == NULL) {
		sacd.proxy_server_address_pointer = NULL;
	} else {
		memset(large_string1, 0x00, sizeof(large_string1));
		meinfo_me_copy_from_large_string_me(temp_me1, large_string1, 375);
		sacd.proxy_server_address_pointer = large_string1;
	}

	//outbound proxy address server
	if ((temp_me1 = meinfo_me_get_by_meid(157, (unsigned short)attr_inst->er_attr_instance[6].attr_value.data)) == NULL) {
		sacd.outbound_proxy_address_pointer = NULL;
	} else {
		memset(large_string2, 0x00, sizeof(large_string2));
		meinfo_me_copy_from_large_string_me(temp_me1, large_string2, 375);
		sacd.outbound_proxy_address_pointer = large_string2;
	}

	sacd.primary_sip_dns = (unsigned int)attr_inst->er_attr_instance[7].attr_value.data;
	sacd.secondary_sip_dns = (unsigned int)attr_inst->er_attr_instance[8].attr_value.data;

	//tcp/udp pointer
	sacd.tcp_udp_pointer = &tcp_udp_config_data;

	//ip options
	tcp_udp_config_data.ip_host_pointer = &ip_host_config_data;

	sacd.sip_reg_exp_time = (unsigned int)attr_inst->er_attr_instance[10].attr_value.data;
	sacd.sip_rereg_head_start_time = (unsigned int)attr_inst->er_attr_instance[11].attr_value.data;

	//host part uri
	if ((temp_me1 = meinfo_me_get_by_meid(157, (unsigned short)attr_inst->er_attr_instance[12].attr_value.data)) == NULL) {
		sacd.host_part_uri = NULL;
	} else {
		memset(large_string3, 0x00, sizeof(large_string3));
		meinfo_me_copy_from_large_string_me(temp_me1, large_string3, 375);
		sacd.host_part_uri = large_string3;
	}

	sacd.sip_status = (unsigned short)attr_inst->er_attr_instance[13].attr_value.data;

	//sip registrar
	if ((temp_me1 = meinfo_me_get_by_meid(137, (unsigned short)attr_inst->er_attr_instance[14].attr_value.data)) == NULL) {
		sacd.sip_registrar = NULL;
	} else {
		memset(&network_address, 0x00, sizeof(network_address));

		//security_pointer
		meinfo_me_attr_get(temp_me1, 1, &attr);

		if ((temp_me2 = meinfo_me_get_by_meid(148, (unsigned short)attr.data)) == NULL) {
			network_address.security_pointer = NULL;
		} else {
			memset(&authentication_security_method1, 0x00, sizeof(authentication_security_method1));

			//caliddation scheme
			meinfo_me_attr_get(temp_me2, 1, &attr);
			authentication_security_method1.validation_scheme = (unsigned char) attr.data;

			//username 1
			attr.ptr = authentication_security_method1.username_1;
			meinfo_me_attr_get(temp_me2, 2, &attr);

			//password
			attr.ptr = authentication_security_method1.password;
			meinfo_me_attr_get(temp_me2, 3, &attr);

			//realm
			attr.ptr = authentication_security_method1.realm;
			meinfo_me_attr_get(temp_me2, 4, &attr);

			//username 2
			attr.ptr = authentication_security_method1.username_2;
			meinfo_me_attr_get(temp_me2, 5, &attr);

			network_address.security_pointer = &authentication_security_method1;
		}

		//address_pointer
		meinfo_me_attr_get(temp_me1, 2, &attr);

		if ((temp_me2 = meinfo_me_get_by_meid(157, (unsigned short)attr.data)) == NULL) {
			network_address.address_pointer = NULL;
		} else {
			memset(large_string4, 0x00, sizeof(large_string4));
			meinfo_me_copy_from_large_string_me(temp_me2, large_string4, 375);
			network_address.address_pointer = large_string4;
		}
		sacd.sip_registrar = &network_address;
	}

	//softswitch
	if(attr_inst->er_attr_instance[15].attr_value.ptr) {
		strncpy(sacd.softswitch, attr_inst->er_attr_instance[15].attr_value.ptr, 4);
	}
	switch(action) {
	case ER_ATTR_GROUP_HW_OP_ADD:
		er_attr_group_util_get_attr_mask_by_classid(attr_inst, 150, attr_mask);

	case ER_ATTR_GROUP_HW_OP_ADD_BY_UPDATE:
		if ((ret = er_group_hw_voip_wrapper_sip_agent_config_data(VOIP_HW_ACTION_SET, port_id, attr_mask, &sacd)) < 0) {
			dbprintf(LOG_ERR, "ret=%d\n", ret);
			return -1;
		}
		batchtab_omci_update("pots_mkey");
		break;

	case ER_ATTR_GROUP_HW_OP_UPDATE:
		//get current value
		if ((current_attr_inst = er_attr_group_util_gen_current_value_attrinst(attr_inst)) == NULL) {
			dbprintf(LOG_ERR, "fail to gen current value attr inst\n");
			return -1;
		}
		//for VoIP to make sure triger event no miss
		//if triger me's classid is 134 or 136, set mask bit 0,1,4,9
		if( me->classid	== 134 || me->classid == 136) {
			//use for me 150 attr 5: TCP/UDP_pointer
			SET_BIT(attr_mask, 0);
			SET_BIT(attr_mask, 1);
			SET_BIT(attr_mask, 4);
			SET_BIT(attr_mask, 9);
		}
		//triger me is 157, set mask as bit 0,1,7,9
		//me 137 conbind with me148 and me157
		//maybe useless now
		if( me->classid	== 157 ) {
			SET_BIT(attr_mask, 0);
			SET_BIT(attr_mask, 1);
			SET_BIT(attr_mask, 7);
			SET_BIT(attr_mask, 9);
		}

		//add
		if (er_group_hw_sip_agent_config_data(ER_ATTR_GROUP_HW_OP_ADD_BY_UPDATE, current_attr_inst, me, attr_mask) < 0) {
			er_attr_group_util_release_attrinst(current_attr_inst);
			dbprintf(LOG_ERR, "fail to update for adding\n");
			return -1;
		}
		er_attr_group_util_release_attrinst(current_attr_inst);
		batchtab_omci_update("pots_mkey");
		break;

	case ER_ATTR_GROUP_HW_OP_DEL:
		memset(&sacd, 0x00, sizeof(sacd));
		if ((ret = er_group_hw_voip_wrapper_sip_agent_config_data(VOIP_HW_ACTION_RELEASE, port_id, attr_mask, &sacd)) < 0) {
			dbprintf(LOG_ERR, "ret=%d\n", ret);
			return -1;
		}
		batchtab_omci_update("pots_mkey");
		break;

	default:
		dbprintf_voip(LOG_ERR,"Unknown operator\n");
	}
	return 0;
}
