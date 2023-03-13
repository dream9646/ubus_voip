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
 * File    : meinfo_hw_65414.c
 *
 ******************************************************************/

#include <string.h>

#include "er_group.h"
#include "voip_hw.h"
#include "util.h"
#include "er_group_hw.h"
#include "omcimsg.h"
#include "proprietary_fiberhome.h"

//9.99.12 WRI_POS_config_data
static int 
meinfo_hw_65414_get(struct me_t *me, unsigned char attr_mask[2])
{
	unsigned short port_id;
	int ret, i;
	int entry_size;
	unsigned char omci_attr_mask[2];
	unsigned char *entry;
	struct voip_hw_wri_pos_config_data_t wpcd;
	struct voip_hw_pos_data_entry_t *pde_pos;
	struct attr_value_t attr;

	if (me == NULL)
	{
		dbprintf(LOG_ERR, "input error\n");
		return -1;
	}

	//port id
	port_id = VOIP_HW_GLOBAL_PORT_DEF;
	
	memset(&wpcd, 0x00, sizeof(wpcd));

	INIT_LIST_HEAD(&wpcd.pos_data_list);

	if ((ret = er_group_hw_voip_wrapper_wri_pos_config_data(VOIP_HW_ACTION_GET, port_id, attr_mask, &wpcd)) < 0)
	{
		dbprintf(LOG_ERR, "ret=%d\n", ret);
		return -1;
	}

	omci_attr_mask[0] = attr_mask[0];
	omci_attr_mask[1] = attr_mask[1];

	for (i = 1; i <= meinfo_util_get_attr_total(me->classid); i++)
	{
		memset(&attr, 0x00, sizeof(attr));

		if (util_bitmap_get_value(omci_attr_mask, 8*2, i - 1, 1))
		{
			switch (i)
			{
			case 1:
				attr.data = wpcd.item_number;
				break;
			case 2:
				//release table
				meinfo_me_attr_release_table_all_entries(me->attr[2].value.ptr);

				//add each entries from dial plan list
				if (!list_empty(&wpcd.pos_data_list) && (entry_size = meinfo_util_attr_get_table_entry_byte_size(65414, 2)) > 0)
				{
					list_for_each_entry(pde_pos, &wpcd.pos_data_list, pos_data_node)
					{
						entry = malloc_safe(entry_size);
						util_bitmap_set_value(entry, 8*entry_size, 0, 8, pde_pos->id); //id
						memcpy(&(entry[1]), pde_pos->tel_num, 20); //tel_num
						util_bitmap_set_value(entry, 8*entry_size, 8 * 21, 16, pde_pos->pos_type); //pos_type

						//set to table
						meinfo_me_attr_set_table_entry(me, 2, entry, 1);
						if (entry)
						{
							free_safe(entry);
						}
					}
				}

				//alread set to table, just continue
				continue;
			default:
				continue;//do nothing
			}

			meinfo_me_attr_set(me, i, &attr, 1);
		}
	}

	voip_hw_wri_pos_config_data_release_table(&wpcd);

	return 0;
}

struct meinfo_hardware_t meinfo_hw_fiberhome_65414 = {
	.get_hw		= meinfo_hw_65414_get
};

