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
 * Module  : proprietary_fiberhome
 * File    : meinfo_hw_157.c
 *
 ******************************************************************/

#include <string.h>

#include "voip_hw.h"
#include "util.h"
#include "meinfo_hw_util.h"
#include "er_group_hw.h"
#include "proprietary_fiberhome.h"

extern int meinfo_hw_157_get_from_voip(struct me_t *issue_me, unsigned char attr_mask[2], unsigned char *large_string);

int
meinfo_hw_157_get_from_voip_65416(struct me_t *issue_me, unsigned char attr_mask[2], struct me_t *voip_me, unsigned char attr_order)
{
	struct voip_hw_wri_mg_port_register_control_t wmprc;
	unsigned char large_string1[376];
	unsigned char attr_mask_target[2], omci_attr_mask[2];

	if (issue_me == NULL || voip_me == NULL)
	{
		dbprintf(LOG_ERR, "input me error\n");
		return -1;
	}

	memset(&wmprc, 0x00, sizeof(wmprc));
	memset(&large_string1, 0x00, sizeof(large_string1));

	//construct parm
	wmprc.mg_ip_pointer = large_string1;

	memset(attr_mask_target, 0x00, sizeof(attr_mask_target));
	util_attr_mask_set_bit(attr_mask_target, attr_order);

	if (meinfo_hw_util_get_voip_param_by_port(voip_me, &wmprc, attr_mask_target, omci_attr_mask) < 0)
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
	case 1:
		if (meinfo_hw_157_get_from_voip(issue_me, attr_mask, large_string1) < 0)
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

int
meinfo_hw_157_get_from_voip_65419(struct me_t *issue_me, unsigned char attr_mask[2], struct me_t *voip_me, unsigned char attr_order)
{
	unsigned short port_id;
	int ret;
	struct voip_hw_wri_sip_config_t wsc;
	unsigned char large_string1[376];
	unsigned char large_string2[376];
	unsigned char large_string3[376];
	unsigned char attr_mask_target[2], omci_attr_mask[2];

	if (issue_me == NULL || voip_me == NULL)
	{
		dbprintf(LOG_ERR, "input me error\n");
		return -1;
	}

	//port id
	port_id = VOIP_HW_GLOBAL_PORT_DEF;

	memset(&wsc, 0x00, sizeof(wsc));
	memset(&large_string1, 0x00, sizeof(large_string1));
	memset(&large_string2, 0x00, sizeof(large_string2));
	memset(&large_string3, 0x00, sizeof(large_string3));

	//construct parm
	wsc.secondary_sip_registrar_server_address_pointer = large_string1;
	wsc.secondary_proxy_server_address_pointer = large_string2;
	wsc.current_sip_registrar_pointer = large_string3;

	memset(attr_mask_target, 0x00, sizeof(attr_mask_target));
	util_attr_mask_set_bit(attr_mask_target, attr_order);

	if ((ret = er_group_hw_voip_wrapper_wri_sip_config(VOIP_HW_ACTION_GET, port_id, attr_mask_target, &wsc)) < 0)
	{
		dbprintf(LOG_ERR, "ret=%d\n", ret);
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
	case 1:
		if (meinfo_hw_157_get_from_voip(issue_me, attr_mask, large_string1) < 0)
		{
			dbprintf(LOG_ERR, "get from voip error: classid=%u, meid=%u, order=%u\n", issue_me->classid, issue_me->meid, attr_order);
			return -1;
		}
		break;
	case 2:
		if (meinfo_hw_157_get_from_voip(issue_me, attr_mask, large_string2) < 0)
		{
			dbprintf(LOG_ERR, "get from voip error: classid=%u, meid=%u, order=%u\n", issue_me->classid, issue_me->meid, attr_order);
			return -1;
		}
		break;
	case 7:
		if (meinfo_hw_157_get_from_voip(issue_me, attr_mask, large_string3) < 0)
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

