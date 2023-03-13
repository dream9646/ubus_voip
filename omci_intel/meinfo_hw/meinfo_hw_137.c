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
 * File    : meinfo_hw_137.c
 *
 ******************************************************************/

#include <string.h>

#include "voip_hw.h"
#include "util.h"
#include "meinfo_hw_util.h"
#include "batchtab_cb.h"

//9.12.3 Network_address
static int
meinfo_hw_137_get(struct me_t *me, unsigned char attr_mask[2])
{
	int ret;
	if (me == NULL) {
		dbprintf(LOG_ERR, "input me error\n");
		return -1;
	}
	if (BATCHTAB_POTS_MKEY_HW_SYNC_DONE != (ret=batchtab_cb_pots_is_hw_sync())) {
		dbprintf(LOG_NOTICE, "pots_mkey is not hw_sync (%d)\n", ret);
		return 0;
	}
	dbprintf_voip(LOG_INFO, "attr_mask[0](0x%02x) attr_mask[1](0x%02x)\n", attr_mask[0], attr_mask[1]);
#if 0
	struct meinfo_t *miptr;
	struct me_t *me_pos;
	struct attr_value_t attr;
	unsigned char attr_mask[2];

	if (me == NULL)
	{
		return -1;
	}

	//find classid 153, attr_order 5
	miptr=meinfo_util_miptr(153);
	if (miptr==NULL) {
		dbprintf(LOG_ERR, "err=%d, classid=%u\n", MEINFO_ERROR_CLASSID_UNDEF, 153);
		return -1;	//class_id out of range
	}

	list_for_each_entry(me_pos, &miptr->me_instance_list, instance_node)
	{
		//get attr_order 5
		meinfo_me_attr_get(me_pos, 5, &attr);

		if (attr.data == me->meid)
		{
			memset(attr_mask, 0x00, sizeof(attr_mask));
			util_attr_mask_set_bit(attr_mask, 5);
			//call er_attr_group_me_update
			if (meinfo_me_issue_self_er_attr_group(me_pos, ER_ATTR_GROUP_HW_OP_UPDATE, attr_mask) < 0)
			{
				dbprintf(LOG_ERR,
			 		"issue self er_attr_group error, classid=%u, instance_num=%d, meid=%u\n",
					me_pos->classid, me_pos->instance_num, me_pos->meid);
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

		if (attr.data == me->meid)
		{
			memset(attr_mask, 0x00, sizeof(attr_mask));
			util_attr_mask_set_bit(attr_mask, 10);
			//call er_attr_group_me_update
			if (meinfo_me_issue_self_er_attr_group(me_pos, ER_ATTR_GROUP_HW_OP_UPDATE, attr_mask) < 0)
			{
				dbprintf(LOG_ERR,
			 		"issue self er_attr_group error, classid=%u, instance_num=%d, meid=%u\n",
					me_pos->classid, me_pos->instance_num, me_pos->meid);
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

		if (attr.data == me->meid)
		{
			memset(attr_mask, 0x00, sizeof(attr_mask));
			util_attr_mask_set_bit(attr_mask, 6);
			//call er_attr_group_me_update
			if (meinfo_me_issue_self_er_attr_group(me_pos, ER_ATTR_GROUP_HW_OP_UPDATE, attr_mask) < 0)
			{
				dbprintf(LOG_ERR,
			 		"issue self er_attr_group error, classid=%u, instance_num=%d, meid=%u\n",
					me_pos->classid, me_pos->instance_num, me_pos->meid);
			}
		}

		//get attr_order 7
		meinfo_me_attr_get(me_pos, 7, &attr);

		if (attr.data == me->meid)
		{
			memset(attr_mask, 0x00, sizeof(attr_mask));
			util_attr_mask_set_bit(attr_mask, 7);
			//call er_attr_group_me_update
			if (meinfo_me_issue_self_er_attr_group(me_pos, ER_ATTR_GROUP_HW_OP_UPDATE, attr_mask) < 0)
			{
				dbprintf(LOG_ERR,
			 		"issue self er_attr_group error, classid=%u, instance_num=%d, meid=%u\n",
					me_pos->classid, me_pos->instance_num, me_pos->meid);
			}
		}

		//get attr_order 8
		meinfo_me_attr_get(me_pos, 8, &attr);

		if (attr.data == me->meid)
		{
			memset(attr_mask, 0x00, sizeof(attr_mask));
			util_attr_mask_set_bit(attr_mask, 8);
			//call er_attr_group_me_update
			if (meinfo_me_issue_self_er_attr_group(me_pos, ER_ATTR_GROUP_HW_OP_UPDATE, attr_mask) < 0)
			{
				dbprintf(LOG_ERR,
			 		"issue self er_attr_group error, classid=%u, instance_num=%d, meid=%u\n",
					me_pos->classid, me_pos->instance_num, me_pos->meid);
			}
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

		if (attr.data == me->meid)
		{
			memset(attr_mask, 0x00, sizeof(attr_mask));
			util_attr_mask_set_bit(attr_mask, 5);
			//call er_attr_group_me_update
			if (meinfo_me_issue_self_er_attr_group(me_pos, ER_ATTR_GROUP_HW_OP_UPDATE, attr_mask) < 0)
			{
				dbprintf(LOG_ERR,
			 		"issue self er_attr_group error, classid=%u, instance_num=%d, meid=%u\n",
					me_pos->classid, me_pos->instance_num, me_pos->meid);
			}
		}
	}
#endif
	return 0;
}

struct meinfo_hardware_t meinfo_hw_137 = {
	.get_hw		= meinfo_hw_137_get
};

