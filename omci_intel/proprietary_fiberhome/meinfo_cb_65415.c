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
 * File    : meinfo_cb_65415.c
 *
 ******************************************************************/

#include "meinfo_cb.h"
#include "util.h"

//classid 65415 9.99.13 WRI_IPT_config_data

static int 
meinfo_cb_65415_create(struct me_t *me, unsigned char attr_mask[2])
{
	if (me == NULL || omci_env_g.voip_enable == ENV_VOIP_DISABLE)
	{
		return -1;
	}
	
	return 0;
}

static int
meinfo_cb_65415_entry_isdel(unsigned short classid, unsigned char attr_order, unsigned char *entry)
{
	int i;
	struct attrinfo_table_t *table_ptr;
	struct meinfo_t *miptr=meinfo_util_miptr(classid);

	if (miptr==NULL)
		return -1;

	table_ptr = miptr->attrinfo[attr_order].attrinfo_table_ptr;
	if( table_ptr == NULL){
		dbprintf(LOG_ERR, "classid=%u, null attrinfo_table_ptr?\n", classid);
		return -1;       
	}
	//2: IPT_table
	if( attr_order == 2){
		//check all bytes excepted first one were zero
		for (i = 1; i < table_ptr->entry_byte_size; i++)
		{
			if (entry[i] != 0)
			{
				return MEINFO_CB_DELETE_NONE;
			}
		}
		
		return MEINFO_CB_DELETE_ENTRY;
	}
	dbprintf(LOG_ERR, "Attr order error classid=%u, attr_order=%d\n", classid, attr_order);
	return -1;       
}

struct meinfo_callback_t meinfo_cb_fiberhome_65415 = {
	.create_cb			= meinfo_cb_65415_create,
	.table_entry_isdel_cb	= meinfo_cb_65415_entry_isdel
};

