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
 * File    : meinfo_hw_136.c
 *
 ******************************************************************/

#include <string.h>

#include "voip_hw.h"
#include "util.h"
#include "meinfo_hw_util.h"

static int
meinfo_hw_136_get_from_voip(struct me_t *issue_me, unsigned char attr_mask[2], struct voip_hw_tcp_udp_config_data_t *tcp_udp_config_data)
{
	int i;
	struct attr_value_t attr;

	if (issue_me == NULL || tcp_udp_config_data == NULL)
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
				attr.data = tcp_udp_config_data->port_id;
				break;
			case 2:
				attr.data = tcp_udp_config_data->protocol;
				break;
			case 3:
				attr.data = tcp_udp_config_data->tos_diffserv_field;
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
meinfo_hw_136_get_from_voip_150(struct me_t *issue_me, unsigned char attr_mask[2], struct me_t *voip_me, unsigned char attr_order)
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
	case 5:
		if (meinfo_hw_136_get_from_voip(issue_me, attr_mask, &tcp_udp_config_data) < 0)
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

//classid 136 9.4.14 TCP/UDP_config_data
static int
meinfo_hw_136_get(struct me_t *me, unsigned char attr_mask[2])
{
	struct meinfo_t *miptr;
	struct me_t *me_pos;
	struct attr_value_t attr;

	if (me == NULL)
	{
		return -1;
	}

	//find classid 150, attr_order 5
	miptr=meinfo_util_miptr(150);
	if (miptr==NULL) {
		dbprintf(LOG_ERR, "err=%d, classid=%u\n", MEINFO_ERROR_CLASSID_UNDEF, 150);
		return -1;	//class_id out of range
	}

	list_for_each_entry(me_pos, &miptr->me_instance_list, instance_node)
	{
		//get attr_order 5
		meinfo_me_attr_get(me_pos, 5, &attr);

		if (attr.data == me->meid)
		{
			if (meinfo_hw_136_get_from_voip_150(me, attr_mask, me_pos, 5) < 0)
			{
				dbprintf(LOG_ERR,
			 		"get from voip error, classid=%u, instance_num=%d, meid=%u\n",
					me_pos->classid, me_pos->instance_num, me_pos->meid);
			}
		}
	}

	return 0;
}

struct meinfo_hardware_t meinfo_hw_136 = {
	.get_hw		= meinfo_hw_136_get
};

