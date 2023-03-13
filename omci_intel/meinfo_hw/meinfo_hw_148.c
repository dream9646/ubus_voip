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
 * Module  : meinfo_hw
 * File    : meinfo_hw_148.c
 *
 ******************************************************************/

#include <string.h>

#include "voip_hw.h"
#include "util.h"
#include "meinfo_hw_util.h"
#include "er_group_hw.h"

static int
meinfo_hw_148_get_from_voip(struct me_t *issue_me, unsigned char attr_mask[2], struct voip_hw_authentication_security_method_t *authentication_security_method)
{
	int i;
	struct attr_value_t attr;

	if (issue_me == NULL || authentication_security_method == NULL)
	{
		return -1;
	}

	for (i = 1; i <= meinfo_util_get_attr_total(issue_me->classid); i++)
	{
		memset(&attr, 0x00, sizeof(attr));

		if (util_bitmap_get_value(attr_mask, 8*2, i - 1, 1))
		{
			switch (i)
			{
			case 1:
				attr.data = authentication_security_method->validation_scheme;
				break;
			case 2:
				attr.ptr = authentication_security_method->username_1;
				break;
			case 3:
				attr.ptr = authentication_security_method->password;
				break;
			case 4:
				attr.ptr = authentication_security_method->realm;
				break;
			case 5:
				attr.ptr = authentication_security_method->username_2;
				break;
			default:
				continue;//do nothing
			}

			meinfo_me_attr_set(issue_me, i, &attr, 1);
		}
	}

	return 0;
}

static int
meinfo_hw_148_get_from_voip_153(struct me_t *issue_me, unsigned char attr_mask[2], struct me_t *voip_me, unsigned char attr_order)
{
	struct voip_hw_sip_user_data_t sud;
	unsigned char large_string1[376], large_string2[376];
	struct voip_hw_authentication_security_method_t authentication_security_method1, authentication_security_method2;
	struct voip_hw_network_address_t network_address;
	unsigned char attr_mask_target[2], omci_attr_mask[2];

	if (issue_me == NULL || voip_me == NULL)
	{
		dbprintf(LOG_ERR, "input me error\n");
		return -1;
	}

	memset(&sud, 0x00, sizeof(sud));
	memset(&large_string1, 0x00, sizeof(large_string1));
	memset(&large_string2, 0x00, sizeof(large_string2));
	memset(&authentication_security_method1, 0x00, sizeof(authentication_security_method1));
	memset(&authentication_security_method2, 0x00, sizeof(authentication_security_method2));
	memset(&network_address, 0x00, sizeof(network_address));

	sud.user_part_aor = large_string1;
	sud.username_password = &authentication_security_method1;
	network_address.security_pointer = &authentication_security_method2;
	network_address.address_pointer = large_string2;
	sud.voicemail_server_sip_uri = &network_address;
	
	memset(attr_mask_target, 0x00, sizeof(attr_mask_target));
	util_attr_mask_set_bit(attr_mask_target, attr_order);

	if (meinfo_hw_util_get_voip_param_by_port(voip_me, &sud, attr_mask_target, omci_attr_mask) < 0)
	{
		dbprintf(LOG_ERR, "get param error: classid=%u, meid=%u\n", voip_me->classid, voip_me->meid);
		return -1;
	}

	if (!util_bitmap_get_value(omci_attr_mask, 8*2, attr_order - 1, 1))
	{
		dbprintf(LOG_ERR, "get from voip error: classid=%u, meid=%u, order=%u\n", issue_me->classid, issue_me->meid, attr_order);
		return -1;
	}

	switch (attr_order)
	{
	case 4:
		if (meinfo_hw_148_get_from_voip(issue_me, attr_mask, &authentication_security_method1) < 0)
		{
			dbprintf(LOG_ERR, "get from voip error: classid=%u, meid=%u, order=%u\n", issue_me->classid, issue_me->meid, attr_order);
			return -1;
		}
		break;
	case 5:
		if (meinfo_hw_148_get_from_voip(issue_me, attr_mask, &authentication_security_method2) < 0)
		{
			dbprintf(LOG_ERR, "get from voip error: classid=%u, meid=%u, order=%u\n", issue_me->classid, issue_me->meid, attr_order);
			return -1;
		}
		break;
	default:
		dbprintf(LOG_ERR, "get from voip error: classid=%u, meid=%u, order=%u\n", issue_me->classid, issue_me->meid, attr_order);
		return -1;
	}

	return 0;
}

static int
meinfo_hw_148_get_from_voip_150(struct me_t *issue_me, unsigned char attr_mask[2], struct me_t *voip_me, unsigned char attr_order)
{
	struct voip_hw_sip_agent_config_data_t sacd;
	unsigned char large_string1[376], large_string2[376], large_string3[376], large_string4[376];
	struct voip_hw_authentication_security_method_t authentication_security_method;
	struct voip_hw_tcp_udp_config_data_t tcp_udp_config_data;
	struct voip_hw_ip_host_config_data_t ip_host_config_data;
	struct voip_hw_network_address_t network_address;
	unsigned char attr_mask_target[2], omci_attr_mask[2];

	if (issue_me == NULL || voip_me == NULL)
	{
		dbprintf(LOG_ERR, "input me error\n");
		return -1;
	}

	memset(&sacd, 0x00, sizeof(sacd));
	memset(&large_string1, 0x00, sizeof(large_string1));
	memset(&large_string2, 0x00, sizeof(large_string2));
	memset(&large_string3, 0x00, sizeof(large_string3));
	memset(&large_string4, 0x00, sizeof(large_string4));
	memset(&authentication_security_method, 0x00, sizeof(authentication_security_method));
	memset(&network_address, 0x00, sizeof(network_address));
	memset(&tcp_udp_config_data, 0x00, sizeof(tcp_udp_config_data));
	memset(&ip_host_config_data, 0x00, sizeof(ip_host_config_data));

	//construct parm
	sacd.proxy_server_address_pointer = large_string1;
	sacd.outbound_proxy_address_pointer = large_string2;
	tcp_udp_config_data.ip_host_pointer = &ip_host_config_data;
	sacd.tcp_udp_pointer = &tcp_udp_config_data;
	sacd.host_part_uri = large_string3;
	network_address.address_pointer = large_string4;
	network_address.security_pointer = &authentication_security_method;
	sacd.sip_registrar = &network_address;

	memset(attr_mask_target, 0x00, sizeof(attr_mask_target));
	util_attr_mask_set_bit(attr_mask_target, attr_order);

	if (meinfo_hw_util_get_voip_param_by_port(voip_me, &sacd, attr_mask_target, omci_attr_mask) < 0)
	{
		dbprintf(LOG_ERR, "get param error: classid=%u, meid=%u\n", voip_me->classid, voip_me->meid);
		return -1;
	}

	if (!util_bitmap_get_value(omci_attr_mask, 8*2, attr_order - 1, 1))
	{
		dbprintf(LOG_ERR, "get from voip error: classid=%u, meid=%u, order=%u\n", issue_me->classid, issue_me->meid, attr_order);
		return -1;
	}

	switch (attr_order)
	{
	case 10:
		if (meinfo_hw_148_get_from_voip(issue_me, attr_mask, &authentication_security_method) < 0)
		{
			dbprintf(LOG_ERR, "get from voip error: classid=%u, meid=%u, order=%u\n", issue_me->classid, issue_me->meid, attr_order);
			return -1;
		}
		break;
	default:
		dbprintf(LOG_ERR, "get from voip error: classid=%u, meid=%u, order=%u\n", issue_me->classid, issue_me->meid, attr_order);
		return -1;
	}

	return 0;
}

static int
meinfo_hw_148_get_from_voip_146(struct me_t *issue_me, unsigned char attr_mask[2], struct me_t *voip_me, unsigned char attr_order)
{
	struct voip_hw_voip_application_service_profile_t vasp;
	unsigned char large_string1[376], large_string2[376], large_string3[376];
	struct voip_hw_authentication_security_method_t authentication_security_method1, authentication_security_method2, authentication_security_method3;
	struct voip_hw_network_address_t network_address1, network_address2, network_address3;
	unsigned char attr_mask_target[2], omci_attr_mask[2];

	if (issue_me == NULL || voip_me == NULL)
	{
		dbprintf(LOG_ERR, "input me error\n");
		return -1;
	}

	memset(&vasp, 0x00, sizeof(vasp));
	memset(&large_string1, 0x00, sizeof(large_string1));
	memset(&large_string2, 0x00, sizeof(large_string2));
	memset(&large_string3, 0x00, sizeof(large_string3));
	memset(&authentication_security_method1, 0x00, sizeof(authentication_security_method1));
	memset(&authentication_security_method2, 0x00, sizeof(authentication_security_method2));
	memset(&authentication_security_method3, 0x00, sizeof(authentication_security_method3));
	memset(&network_address1, 0x00, sizeof(network_address1));
	memset(&network_address2, 0x00, sizeof(network_address2));
	memset(&network_address3, 0x00, sizeof(network_address3));

	//construct
	network_address1.address_pointer = large_string1;
	network_address1.security_pointer = &authentication_security_method1;
	vasp.direct_connect_uri_pointer = &network_address1;
	network_address2.address_pointer = large_string2;
	network_address2.security_pointer = &authentication_security_method2;
	vasp.bridged_line_agent_uri_pointer = &network_address2;
	network_address3.address_pointer = large_string3;
	network_address3.security_pointer = &authentication_security_method3;
	vasp.conference_factory_uri_pointer = &network_address3;

	memset(attr_mask_target, 0x00, sizeof(attr_mask_target));
	util_attr_mask_set_bit(attr_mask_target, attr_order);

	if (meinfo_hw_util_get_voip_param_by_port(voip_me, &vasp, attr_mask_target, omci_attr_mask) < 0)
	{
		dbprintf(LOG_ERR, "get param error: classid=%u, meid=%u\n", voip_me->classid, voip_me->meid);
		return -1;
	}

	if (!util_bitmap_get_value(omci_attr_mask, 8*2, attr_order - 1, 1))
	{
		dbprintf(LOG_ERR, "get from voip error: classid=%u, meid=%u, order=%u\n", issue_me->classid, issue_me->meid, attr_order);
		return -1;
	}

	switch (attr_order)
	{
	case 6:
		if (meinfo_hw_148_get_from_voip(issue_me, attr_mask, &authentication_security_method1) < 0)
		{
			dbprintf(LOG_ERR, "get from voip error: classid=%u, meid=%u, order=%u\n", issue_me->classid, issue_me->meid, attr_order);
			return -1;
		}
		break;
	case 7:
		if (meinfo_hw_148_get_from_voip(issue_me, attr_mask, &authentication_security_method2) < 0)
		{
			dbprintf(LOG_ERR, "get from voip error: classid=%u, meid=%u, order=%u\n", issue_me->classid, issue_me->meid, attr_order);
			return -1;
		}
		break;
	case 8:
		if (meinfo_hw_148_get_from_voip(issue_me, attr_mask, &authentication_security_method3) < 0)
		{
			dbprintf(LOG_ERR, "get from voip error: classid=%u, meid=%u, order=%u\n", issue_me->classid, issue_me->meid, attr_order);
			return -1;
		}
		break;
	default:
		dbprintf(LOG_ERR, "get from voip error: classid=%u, meid=%u, order=%u\n", issue_me->classid, issue_me->meid, attr_order);
		return -1;
	}

	return 0;
}

static int
meinfo_hw_148_get_from_voip_138(struct me_t *issue_me, unsigned char attr_mask[2], struct me_t *voip_me, unsigned char attr_order)
{
	struct voip_hw_voip_config_data_t vcd;
	unsigned char large_string[376];
	struct voip_hw_authentication_security_method_t authentication_security_method;
	struct voip_hw_network_address_t network_address;
	unsigned char omci_attr_mask[2];
	unsigned char attr_mask_target[2];

	if (issue_me == NULL || voip_me == NULL)
	{
		dbprintf(LOG_ERR, "input me error\n");
		return -1;
	}

	memset(&vcd, 0x00, sizeof(vcd));
	memset(&large_string, 0x00, sizeof(large_string));
	memset(&authentication_security_method, 0x00, sizeof(authentication_security_method));
	memset(&network_address, 0x00, sizeof(network_address));
	
	//construct
	network_address.address_pointer = large_string;
	network_address.security_pointer = &authentication_security_method;
	vcd.voip_configuration_address_pointer = &network_address;

	memset(attr_mask_target, 0x00, sizeof(attr_mask_target));
	util_attr_mask_set_bit(attr_mask_target, attr_order);

	if (er_group_hw_voip_wrapper_voip_config_data(VOIP_HW_ACTION_GET, 0, attr_mask_target, &vcd) < 0)
	{
		dbprintf(LOG_ERR, "get voip me error, classid=%u, meid=%u\n", voip_me->classid, voip_me->meid);
		return -1;
	}

	omci_attr_mask[0] = attr_mask_target[0];
	omci_attr_mask[1] = attr_mask_target[1];

	if (!util_bitmap_get_value(omci_attr_mask, 8*2, attr_order - 1, 1))
	{
		dbprintf(LOG_ERR, "get from voip error: classid=%u, meid=%u, order=%u\n", issue_me->classid, issue_me->meid, attr_order);
		return -1;
	}

	switch (attr_order)
	{
	case 5:
		if (meinfo_hw_148_get_from_voip(issue_me, attr_mask, &authentication_security_method) < 0)
		{
			dbprintf(LOG_ERR, "get from voip error: classid=%u, meid=%u, order=%u\n", issue_me->classid, issue_me->meid, attr_order);
			return -1;
		}
		break;
	default:
		dbprintf(LOG_ERR, "get from voip error: classid=%u, meid=%u, order=%u\n", issue_me->classid, issue_me->meid, attr_order);
		return -1;
	}

	return 0;
}

static int
meinfo_hw_148_get_from_voip_58(struct me_t *issue_me, unsigned char attr_mask[2], struct me_t *voip_me, unsigned char attr_order)
{
	struct voip_hw_voice_service_profile_t vvsp;
	unsigned char large_string[376];
	struct voip_hw_authentication_security_method_t authentication_security_method;
	struct voip_hw_network_address_t network_address;
	unsigned char attr_mask_target[2], omci_attr_mask[2];

	if (issue_me == NULL || voip_me == NULL)
	{
		dbprintf(LOG_ERR, "input me error\n");
		return -1;
	}

	memset(&vvsp, 0x00, sizeof(vvsp));

	INIT_LIST_HEAD(&vvsp.tone_pattern_list);
	INIT_LIST_HEAD(&vvsp.tone_event_list);
	INIT_LIST_HEAD(&vvsp.ringing_pattern_list);
	INIT_LIST_HEAD(&vvsp.ringing_event_list);

	memset(&large_string, 0x00, sizeof(large_string));
	memset(&authentication_security_method, 0x00, sizeof(authentication_security_method));
	memset(&network_address, 0x00, sizeof(network_address));

	network_address.security_pointer = &authentication_security_method;
	network_address.address_pointer = large_string;
	vvsp.network_specific_extensions = &network_address;

	memset(attr_mask_target, 0x00, sizeof(attr_mask_target));
	util_attr_mask_set_bit(attr_mask_target, attr_order);

	if (meinfo_hw_util_get_voip_param_by_port(voip_me, &vvsp, attr_mask_target, omci_attr_mask) < 0)
	{
		dbprintf(LOG_ERR, "get param error: classid=%u, meid=%u\n", voip_me->classid, voip_me->meid);
		return -1;
	}

	if (!util_bitmap_get_value(omci_attr_mask, 8*2, attr_order - 1, 1))
	{
		dbprintf(LOG_ERR, "get from voip error: classid=%u, meid=%u, order=%u\n", issue_me->classid, issue_me->meid, attr_order);
		return -1;
	}

	switch (attr_order)
	{
	case 14:
		if (meinfo_hw_148_get_from_voip(issue_me, attr_mask, &authentication_security_method) < 0)
		{
			dbprintf(LOG_ERR, "get from voip error: classid=%u, meid=%u, order=%u\n", issue_me->classid, issue_me->meid, attr_order);
			return -1;
		}
		break;
	default:
		dbprintf(LOG_ERR, "get from voip error: classid=%u, meid=%u, order=%u\n", issue_me->classid, issue_me->meid, attr_order);
		return -1;
	}

	return 0;
}

//9.12.4 Authentication_security_method
static int
meinfo_hw_148_get(struct me_t *me, unsigned char attr_mask[2])
{
	struct meinfo_t *miptr;
	struct me_t *me_pos, *temp_me;
	struct attr_value_t attr;

	if (me == NULL)
	{
		return -1;
	}

	//find classid 153, attr_order 4 and 5
	miptr=meinfo_util_miptr(153);
	if (miptr==NULL) {
		dbprintf(LOG_ERR, "err=%d, classid=%u\n", MEINFO_ERROR_CLASSID_UNDEF, 153);
		return -1;	//class_id out of range
	}

	list_for_each_entry(me_pos, &miptr->me_instance_list, instance_node)
	{
		//get attr_order 4
		meinfo_me_attr_get(me_pos, 4, &attr);

		if (attr.data == me->meid)
		{
			if (meinfo_hw_148_get_from_voip_153(me, attr_mask, me_pos, 4) < 0)
			{
				dbprintf(LOG_ERR,
			 		"get from voip error, classid=%u, instance_num=%d, meid=%u\n",
					me_pos->classid, me_pos->instance_num, me_pos->meid);
			}
			return 0;
		}

		//get attr_order 5
		meinfo_me_attr_get(me_pos, 5, &attr);
		
		if ((temp_me = meinfo_me_get_by_meid(137, (unsigned short)attr.data)) != NULL)
		{
			//security pointer
			meinfo_me_attr_get(temp_me, 1, &attr);

			if (attr.data == me->meid)
			{
				if (meinfo_hw_148_get_from_voip_153(me, attr_mask, me_pos, 5) < 0)
				{
					dbprintf(LOG_ERR,
			 			"get from voip error, classid=%u, instance_num=%d, meid=%u\n",
						me_pos->classid, me_pos->instance_num, me_pos->meid);
				}
				return 0;
			}
		}
	}

	//find classid 150, attr_order 10
	miptr=meinfo_util_miptr(150);
	if (miptr==NULL) {
		dbprintf(LOG_ERR, "err=%d, classid=%u\n", MEINFO_ERROR_CLASSID_UNDEF, 150);
		return -1;	//class_id out of range
	}

	list_for_each_entry(me_pos, &miptr->me_instance_list, instance_node)
	{
		//get attr_order 10
		meinfo_me_attr_get(me_pos, 10, &attr);
		
		if ((temp_me = meinfo_me_get_by_meid(137, (unsigned short)attr.data)) != NULL)
		{
			//security pointer
			meinfo_me_attr_get(temp_me, 1, &attr);

			if (attr.data == me->meid)
			{
				if (meinfo_hw_148_get_from_voip_150(me, attr_mask, me_pos, 10) < 0)
				{
					dbprintf(LOG_ERR,
			 			"get from voip error, classid=%u, instance_num=%d, meid=%u\n",
						me_pos->classid, me_pos->instance_num, me_pos->meid);
				}
				return 0;
			}
		}
	}

	//find classid 146, attr_order 6,7 and 8
	miptr=meinfo_util_miptr(146);
	if (miptr==NULL) {
		dbprintf(LOG_ERR, "err=%d, classid=%u\n", MEINFO_ERROR_CLASSID_UNDEF, 146);
		return -1;	//class_id out of range
	}

	list_for_each_entry(me_pos, &miptr->me_instance_list, instance_node)
	{
		//get attr_order 6
		meinfo_me_attr_get(me_pos, 6, &attr);
		
		if ((temp_me = meinfo_me_get_by_meid(137, (unsigned short)attr.data)) != NULL)
		{
			//security pointer
			meinfo_me_attr_get(temp_me, 1, &attr);

			if (attr.data == me->meid)
			{
				if (meinfo_hw_148_get_from_voip_146(me, attr_mask, me_pos, 6) < 0)
				{
					dbprintf(LOG_ERR,
			 			"get from voip error, classid=%u, instance_num=%d, meid=%u\n",
						me_pos->classid, me_pos->instance_num, me_pos->meid);
				}
				return 0;
			}
		}

		//get attr_order 7
		meinfo_me_attr_get(me_pos, 7, &attr);
		
		if ((temp_me = meinfo_me_get_by_meid(137, (unsigned short)attr.data)) != NULL)
		{
			//security pointer
			meinfo_me_attr_get(temp_me, 1, &attr);

			if (attr.data == me->meid)
			{
				if (meinfo_hw_148_get_from_voip_146(me, attr_mask, me_pos, 7) < 0)
				{
					dbprintf(LOG_ERR,
			 			"get from voip error, classid=%u, instance_num=%d, meid=%u\n",
						me_pos->classid, me_pos->instance_num, me_pos->meid);
				}
				return 0;
			}
		}

		//get attr_order 8
		meinfo_me_attr_get(me_pos, 8, &attr);
		
		if ((temp_me = meinfo_me_get_by_meid(137, (unsigned short)attr.data)) != NULL)
		{
			//security pointer
			meinfo_me_attr_get(temp_me, 1, &attr);

			if (attr.data == me->meid)
			{
				if (meinfo_hw_148_get_from_voip_146(me, attr_mask, me_pos, 8) < 0)
				{
					dbprintf(LOG_ERR,
			 			"get from voip error, classid=%u, instance_num=%d, meid=%u\n",
						me_pos->classid, me_pos->instance_num, me_pos->meid);
				}
			}
			return 0;
		}
	}

	//find classid 138, attr_order 5
	miptr=meinfo_util_miptr(138);
	if (miptr==NULL) {
		dbprintf(LOG_ERR, "err=%d, classid=%u\n", MEINFO_ERROR_CLASSID_UNDEF, 138);
		return -1;	//class_id out of range
	}

	list_for_each_entry(me_pos, &miptr->me_instance_list, instance_node)
	{
		//get attr_order 5
		meinfo_me_attr_get(me_pos, 5, &attr);
		
		if ((temp_me = meinfo_me_get_by_meid(137, (unsigned short)attr.data)) != NULL)
		{
			//security pointer
			meinfo_me_attr_get(temp_me, 1, &attr);

			if (attr.data == me->meid)
			{
				if (meinfo_hw_148_get_from_voip_138(me, attr_mask, me_pos, 5) < 0)
				{
					dbprintf(LOG_ERR,
			 			"get from voip error, classid=%u, instance_num=%d, meid=%u\n",
						me_pos->classid, me_pos->instance_num, me_pos->meid);
				}
				return 0;
			}
		}
	}

	//find classid 58, attr_order 14
	miptr=meinfo_util_miptr(58);
	if (miptr==NULL) {
		dbprintf(LOG_ERR, "err=%d, classid=%u\n", MEINFO_ERROR_CLASSID_UNDEF, 58);
		return -1;	//class_id out of range
	}

	list_for_each_entry(me_pos, &miptr->me_instance_list, instance_node)
	{
		//get attr_order 14
		meinfo_me_attr_get(me_pos, 14, &attr);
		
		if ((temp_me = meinfo_me_get_by_meid(137, (unsigned short)attr.data)) != NULL)
		{
			//security pointer
			meinfo_me_attr_get(temp_me, 1, &attr);

			if (attr.data == me->meid)
			{
				if (meinfo_hw_148_get_from_voip_58(me, attr_mask, me_pos, 14) < 0)
				{
					dbprintf(LOG_ERR,
			 			"get from voip error, classid=%u, instance_num=%d, meid=%u\n",
						me_pos->classid, me_pos->instance_num, me_pos->meid);
				}
				return 0;
			}
		}
	}

	return 0;
}

struct meinfo_hardware_t meinfo_hw_148 = {
	.get_hw		= meinfo_hw_148_get
};

