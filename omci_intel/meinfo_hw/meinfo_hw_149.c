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
 * File    : meinfo_hw_149.c
 *
 ******************************************************************/

#include <string.h>

#include "voip_hw.h"
#include "util.h"
#include "meinfo_hw_util.h"
#include "er_group_hw.h"

//9.9.19 SIP_config_portal
static int 
meinfo_hw_149_get(struct me_t *me, unsigned char attr_mask[2])
{
	int i;
	struct voip_hw_sip_config_portal_t scp;
	struct attr_value_t attr;
	struct voip_hw_sip_config_portal_entry_t *scpe_pos;
	unsigned char *entry;
	int entry_size;
	unsigned char omci_attr_mask[2];

	
	if (me == NULL)
	{
		dbprintf(LOG_ERR, "input me error\n");
		return -1;
	}

	memset(&scp, 0x00, sizeof(scp));

	INIT_LIST_HEAD(&scp.sip_config_portal_list);
	
	if (er_group_hw_voip_wrapper_sip_config_portal(VOIP_HW_ACTION_GET, 0, attr_mask, &scp) < 0)
	{
		dbprintf(LOG_ERR, "get voip me error, classid=%u, meid=%u\n", me->classid, me->meid);
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
				//release table
				meinfo_me_attr_release_table_all_entries(me->attr[1].value.ptr);

				//add each entries from dial plan list
				if (!list_empty(&scp.sip_config_portal_list) && (entry_size = meinfo_util_attr_get_table_entry_byte_size(149, 1)) > 0)
				{
					list_for_each_entry(scpe_pos, &scp.sip_config_portal_list, sip_config_portal_node)
					{
						entry = malloc_safe(entry_size);
						memcpy(entry, scpe_pos->configuration_text, entry_size); //text

						//set to table
						meinfo_me_attr_set_table_entry(me, 1, entry, 1);
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

	voip_hw_sip_config_portal_release_table(&scp);

	return 0;
}

struct meinfo_hardware_t meinfo_hw_149 = {
	.get_hw		= meinfo_hw_149_get
};
