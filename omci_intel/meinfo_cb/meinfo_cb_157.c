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
 * Module  : meinfo_cb
 * File    : meinfo_cb_157.c
 *
 ******************************************************************/

#include <string.h>

#include "meinfo_cb.h"
#include "me_related.h"
#include "er_group.h"
#include "util.h"

//classid 157 9.12.5 Large_string

static int
meinfo_cb_157_check_update(struct me_t *me)
{
	unsigned char attr_mask[2];
	unsigned short ls_meid, ls_meid1;
	struct meinfo_t *miptr;
	struct me_t *me_pos, *temp_me;
	struct attr_value_t attr;
	struct attr_table_header_t *table_header;
	struct attr_table_entry_t *table_entry_pos;

	if (me == NULL)
	{
		return -1;
	}

	//find classid 153, attr_order 2 and 5
	miptr=meinfo_util_miptr(153);
	if (miptr==NULL) {
		dbprintf(LOG_ERR, "err=%d, classid=%u\n", MEINFO_ERROR_CLASSID_UNDEF, 153);
		return -1;	//class_id out of range
	}

	list_for_each_entry(me_pos, &miptr->me_instance_list, instance_node)
	{
		//get attr_order 2
		meinfo_me_attr_get(me_pos, 2, &attr);

		if (attr.data == me->meid)
		{
			memset(attr_mask, 0x00, sizeof(attr_mask));
			util_attr_mask_set_bit(attr_mask, 2);
			//call er_attr_group_me_update
			if (meinfo_me_issue_self_er_attr_group(me_pos, ER_ATTR_GROUP_HW_OP_UPDATE, attr_mask) < 0)
			{
				dbprintf(LOG_ERR,
			 		"issue self er_attr_group error, classid=%u, instance_num=%d, meid=%u\n",
					me_pos->classid, me_pos->instance_num, me_pos->meid);
			}
		}

		//get attr_order 5
		meinfo_me_attr_get(me_pos, 5, &attr);
		
		//network address
		if ((temp_me = meinfo_me_get_by_meid(137, (unsigned short)attr.data)) != NULL)
		{
			//address pointer
			meinfo_me_attr_get(temp_me, 2, &attr);

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
	}

	//find classid 150, attr_order 1, 2, 8 and 10
	miptr=meinfo_util_miptr(150);
	if (miptr==NULL) {
		dbprintf(LOG_ERR, "err=%d, classid=%u\n", MEINFO_ERROR_CLASSID_UNDEF, 150);
		return -1;	//class_id out of range
	}

	list_for_each_entry(me_pos, &miptr->me_instance_list, instance_node)
	{
		//get attr_order 1
		meinfo_me_attr_get(me_pos, 1, &attr);

		if (attr.data == me->meid)
		{
			memset(attr_mask, 0x00, sizeof(attr_mask));
			util_attr_mask_set_bit(attr_mask, 1);
			//call er_attr_group_me_update
			if (meinfo_me_issue_self_er_attr_group(me_pos, ER_ATTR_GROUP_HW_OP_UPDATE, attr_mask) < 0)
			{
				dbprintf(LOG_ERR,
			 		"issue self er_attr_group error, classid=%u, instance_num=%d, meid=%u\n",
					me_pos->classid, me_pos->instance_num, me_pos->meid);
			}
		}

		//get attr_order 2
		meinfo_me_attr_get(me_pos, 2, &attr);
		
		if (attr.data == me->meid)
		{
			memset(attr_mask, 0x00, sizeof(attr_mask));
			util_attr_mask_set_bit(attr_mask, 2);
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

		//get attr_order 10
		meinfo_me_attr_get(me_pos, 10, &attr);
		
		//network address
		if ((temp_me = meinfo_me_get_by_meid(137, (unsigned short)attr.data)) != NULL)
		{
			//address pointer
			meinfo_me_attr_get(temp_me, 2, &attr);

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
	}

	//find classid 146, attr_order 6, 7 and 8
	miptr=meinfo_util_miptr(146);
	if (miptr==NULL) {
		dbprintf(LOG_ERR, "err=%d, classid=%u\n", MEINFO_ERROR_CLASSID_UNDEF, 146);
		return -1;	//class_id out of range
	}

	list_for_each_entry(me_pos, &miptr->me_instance_list, instance_node)
	{
		//get attr_order 6
		meinfo_me_attr_get(me_pos, 6, &attr);
		
		//network address
		if ((temp_me = meinfo_me_get_by_meid(137, (unsigned short)attr.data)) != NULL)
		{
			//address pointer
			meinfo_me_attr_get(temp_me, 2, &attr);

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
		}

		//get attr_order 7
		meinfo_me_attr_get(me_pos, 7, &attr);
		
		//network address
		if ((temp_me = meinfo_me_get_by_meid(137, (unsigned short)attr.data)) != NULL)
		{
			//address pointer
			meinfo_me_attr_get(temp_me, 2, &attr);

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
		}

		//get attr_order 8
		meinfo_me_attr_get(me_pos, 8, &attr);
		
		//network address
		if ((temp_me = meinfo_me_get_by_meid(137, (unsigned short)attr.data)) != NULL)
		{
			//address pointer
			meinfo_me_attr_get(temp_me, 2, &attr);

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
		
		//network address
		if ((temp_me = meinfo_me_get_by_meid(137, (unsigned short)attr.data)) != NULL)
		{
			//address pointer
			meinfo_me_attr_get(temp_me, 2, &attr);

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
	}

	//find classid 58, attr_order 11 and 13
	miptr=meinfo_util_miptr(58);
	if (miptr==NULL) {
		dbprintf(LOG_ERR, "err=%d, classid=%u\n", MEINFO_ERROR_CLASSID_UNDEF, 58);
		return -1;	//class_id out of range
	}

	list_for_each_entry(me_pos, &miptr->me_instance_list, instance_node)
	{
		//traverse attr 11 table
		if ((table_header = (struct attr_table_header_t *) me_pos->attr[11].value.ptr) == NULL) {
			dbprintf(LOG_ERR, "attr 11 value was not table\n");
			return (-1);
		}

		list_for_each_entry(table_entry_pos, &table_header->entry_list, entry_node)
		{
			ls_meid = (unsigned short) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 8*7, 16, 16);
			if (ls_meid == me->meid)
			{
				memset(attr_mask, 0x00, sizeof(attr_mask));
				util_attr_mask_set_bit(attr_mask, 11);
				//call er_attr_group_me_update
				if (meinfo_me_issue_self_er_attr_group(me_pos, ER_ATTR_GROUP_HW_OP_UPDATE, attr_mask) < 0)
				{
					dbprintf(LOG_ERR,
			 			"issue self er_attr_group error, classid=%u, instance_num=%d, meid=%u\n",
						me_pos->classid, me_pos->instance_num, me_pos->meid);
				}
				break;
			}
		}

		//traverse attr 13 table
		if ((table_header = (struct attr_table_header_t *) me_pos->attr[13].value.ptr) == NULL) {
			dbprintf(LOG_ERR, "attr 13 value was not table\n");
			return (-1);
		}

		list_for_each_entry(table_entry_pos, &table_header->entry_list, entry_node)
		{
			ls_meid = (unsigned short) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 8*7, 16, 16);
			ls_meid1 = (unsigned short) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 8*7, 40, 16);
			if (ls_meid == me->meid || ls_meid1 == me->meid)
			{
				memset(attr_mask, 0x00, sizeof(attr_mask));
				util_attr_mask_set_bit(attr_mask, 13);
				//call er_attr_group_me_update
				if (meinfo_me_issue_self_er_attr_group(me_pos, ER_ATTR_GROUP_HW_OP_UPDATE, attr_mask) < 0)
				{
					dbprintf(LOG_ERR,
			 			"issue self er_attr_group error, classid=%u, instance_num=%d, meid=%u\n",
						me_pos->classid, me_pos->instance_num, me_pos->meid);
				}
				break;
			}
		}

		//get attr_order 14
		meinfo_me_attr_get(me_pos, 14, &attr);
		
		//network address
		if ((temp_me = meinfo_me_get_by_meid(137, (unsigned short)attr.data)) != NULL)
		{
			//address pointer
			meinfo_me_attr_get(temp_me, 2, &attr);

			if (attr.data == me->meid)
			{
				memset(attr_mask, 0x00, sizeof(attr_mask));
				util_attr_mask_set_bit(attr_mask, 14);
				//call er_attr_group_me_update
				if (meinfo_me_issue_self_er_attr_group(me_pos, ER_ATTR_GROUP_HW_OP_UPDATE, attr_mask) < 0)
				{
					dbprintf(LOG_ERR,
			 			"issue self er_attr_group error, classid=%u, instance_num=%d, meid=%u\n",
						me_pos->classid, me_pos->instance_num, me_pos->meid);
				}
			}
		}
	}

	//find classid 340, attr_order 2
	miptr=meinfo_util_miptr(340);
	if (miptr==NULL) {
		dbprintf(LOG_ERR, "err=%d, classid=%u\n", MEINFO_ERROR_CLASSID_UNDEF, 340);
		return -1;	//class_id out of range
	}

	list_for_each_entry(me_pos, &miptr->me_instance_list, instance_node)
	{
		//get attr_order 2
		meinfo_me_attr_get(me_pos, 2, &attr);
		
		//network address
		if ((temp_me = meinfo_me_get_by_meid(137, (unsigned short)attr.data)) != NULL)
		{
			//address pointer
			meinfo_me_attr_get(temp_me, 2, &attr);

			if (attr.data == me->meid)
			{
				memset(attr_mask, 0x00, sizeof(attr_mask));
				util_attr_mask_set_bit(attr_mask, 2);
				//call er_attr_group_me_update
				if (meinfo_me_issue_self_er_attr_group(me_pos, ER_ATTR_GROUP_HW_OP_UPDATE, attr_mask) < 0)
				{
					dbprintf(LOG_ERR,
			 			"issue self er_attr_group error, classid=%u, instance_num=%d, meid=%u\n",
						me_pos->classid, me_pos->instance_num, me_pos->meid);
				}
			}
		}
	}

	if(strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_FIBERHOME)==0) {
		//find classid 65416, attr_order 1
		miptr=meinfo_util_miptr(65416);
		if (miptr==NULL) {
			dbprintf(LOG_ERR, "err=%d, classid=%u\n", MEINFO_ERROR_CLASSID_UNDEF, 65416);
			return -1;	//class_id out of range
		}
		
		list_for_each_entry(me_pos, &miptr->me_instance_list, instance_node)
		{
			//get attr_order 1
			meinfo_me_attr_get(me_pos, 1, &attr);
	
			if (attr.data == me->meid)
			{
				memset(attr_mask, 0x00, sizeof(attr_mask));
				util_attr_mask_set_bit(attr_mask, 1);
				//call er_attr_group_me_update
				if (meinfo_me_issue_self_er_attr_group(me_pos, ER_ATTR_GROUP_HW_OP_UPDATE, attr_mask) < 0)
				{
					dbprintf(LOG_ERR,
				 		"issue self er_attr_group error, classid=%u, instance_num=%d, meid=%u\n",
						me_pos->classid, me_pos->instance_num, me_pos->meid);
				}
			}
		}
			
		//find classid 65419, attr_order 1 and 2
		miptr=meinfo_util_miptr(65419);
		if (miptr==NULL) {
			dbprintf(LOG_ERR, "err=%d, classid=%u\n", MEINFO_ERROR_CLASSID_UNDEF, 65419);
			return -1;	//class_id out of range
		}
	
		list_for_each_entry(me_pos, &miptr->me_instance_list, instance_node)
		{
			//get attr_order 1
			meinfo_me_attr_get(me_pos, 1, &attr);
	
			if (attr.data == me->meid)
			{
				memset(attr_mask, 0x00, sizeof(attr_mask));
				util_attr_mask_set_bit(attr_mask, 1);
				//call er_attr_group_me_update
				if (meinfo_me_issue_self_er_attr_group(me_pos, ER_ATTR_GROUP_HW_OP_UPDATE, attr_mask) < 0)
				{
					dbprintf(LOG_ERR,
				 		"issue self er_attr_group error, classid=%u, instance_num=%d, meid=%u\n",
						me_pos->classid, me_pos->instance_num, me_pos->meid);
				}
			}
	
			//get attr_order 2
			meinfo_me_attr_get(me_pos, 2, &attr);
	
			if (attr.data == me->meid)
			{
				memset(attr_mask, 0x00, sizeof(attr_mask));
				util_attr_mask_set_bit(attr_mask, 2);
				//call er_attr_group_me_update
				if (meinfo_me_issue_self_er_attr_group(me_pos, ER_ATTR_GROUP_HW_OP_UPDATE, attr_mask) < 0)
				{
					dbprintf(LOG_ERR,
				 		"issue self er_attr_group error, classid=%u, instance_num=%d, meid=%u\n",
						me_pos->classid, me_pos->instance_num, me_pos->meid);
				}
			}
		}
	}

	if(strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX)==0) {
		//find classid 65315(ExtIpHostConfigData), attr_order 1(FQDN)
		miptr=meinfo_util_miptr(65315);
		if (miptr==NULL) {
			dbprintf(LOG_ERR, "err=%d, classid=%u\n", MEINFO_ERROR_CLASSID_UNDEF, 65315);
			return -1;	//class_id out of range
		}
		
		list_for_each_entry(me_pos, &miptr->me_instance_list, instance_node)
		{
			//get attr_order 1
			meinfo_me_attr_get(me_pos, 1, &attr);
	
			if (attr.data == me->meid)
			{
				memset(attr_mask, 0x00, sizeof(attr_mask));
				util_attr_mask_set_bit(attr_mask, 1);
				//call er_attr_group_me_update
				if (meinfo_me_issue_self_er_attr_group(me_pos, ER_ATTR_GROUP_HW_OP_UPDATE, attr_mask) < 0)
				{
					dbprintf(LOG_ERR,
				 		"issue self er_attr_group error, classid=%u, instance_num=%d, meid=%u\n",
						me_pos->classid, me_pos->instance_num, me_pos->meid);
				}
			}
		}
	}

	return 0;
}

static int 
meinfo_cb_157_create(struct me_t *me, unsigned char attr_mask[2])
{
	return meinfo_cb_157_check_update(me);
}

static int 
meinfo_cb_157_set(struct me_t *me, unsigned char attr_mask[2])
{
	return meinfo_cb_157_check_update(me);
}

struct meinfo_callback_t meinfo_cb_157 = {
	.create_cb	= meinfo_cb_157_create,
	.set_cb		= meinfo_cb_157_set
};

