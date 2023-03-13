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
 * File    : er_group_hw_11_130_266_268_277_401.c
 *
 ******************************************************************/

#include <stdio.h>
#include <sys/time.h>
#include "util.h"
#include "meinfo.h"
#include "me_related.h"
#include "omciutil_vlan.h"
#include "er_group.h"
#include "hwresource.h"
#include "switch.h"

int cli_tag(int fd, unsigned short bridge_meid);


// 11 Physical_path_termination_point_Ethernet_UNI 
// 130 802.1p_mapper_service_profile 
// 266 GEM_interworking_termination_point 
// 268 GEM_port_network_CTP 
// 277 Priority_queue_G 
// 401 Downstream pseudo obj 

//11@254,130@2,130@3,130@4,130@5,130@6,130@7,130@8,130@9,266@6,268@1,268@3,277@255
int
er_group_hw_l2s_unipqmap_802_1p(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	int i;
	//params
	unsigned short pq_meid;
	unsigned char pbit_bitmap=0;
	int ret = 0;
	struct meinfo_t *miptr=meinfo_util_miptr(47);
	struct er_attr_group_instance_t *current_attr_inst;
	struct me_t *private_ethernet_uni_me;
	struct switch_port_info_t port_info;

	struct me_t *me_uni, *me_mapper, *me_bport;
	struct me_t *me_bport_uni=NULL, *me_bport_mapper=NULL;
	char pbitmap_ds_uni[8], pbitmap_ds_mapper[8];

	if (attr_inst == NULL)
	{
		dbprintf(LOG_ERR, "attr instance error\n");
		return -1;
	}

	pq_meid = attr_inst->er_attr_instance[12].meid;
	private_ethernet_uni_me=er_attr_group_util_attrinst2private_me(attr_inst, 1);
	if (private_ethernet_uni_me==NULL)
		return -1;

	if (switch_get_port_info_by_me(private_ethernet_uni_me, &port_info) < 0)
	{
		dbprintf(LOG_ERR, "get port info error:classid=%u, meid=%u\n", private_ethernet_uni_me->classid, private_ethernet_uni_me->meid);
		return -1;
	}

	//pbit, check attr_inst member 1 to 8 to check the pointer equal with member 9's meid
	for (i = 0; i < 8; i++)
	{
		if (attr_inst->er_attr_instance[i+1].attr_value.data == attr_inst->er_attr_instance[9].meid)
		{
			pbit_bitmap |= 1 << i;
		}
	}
	if (pbit_bitmap == 0)
	{
		dbprintf(LOG_ERR, "pbit error: 802.1p mapper meid=0x%x\n", attr_inst->er_attr_instance[1].meid);
		return -1;
	}

	// find the pbitmap in uni side and 8021p mapper side
	me_uni=er_attr_group_util_attrinst2me(attr_inst, 0);
	me_mapper=er_attr_group_util_attrinst2me(attr_inst, 1);	
	
	if (me_uni==NULL || me_mapper ==NULL)
		return -1;
	list_for_each_entry(me_bport, &miptr->me_instance_list, instance_node) {
		if (me_related(me_bport, me_uni)) {
			me_bport_uni=me_bport;
		} else if (me_related(me_bport, me_mapper)) {
			me_bport_mapper=me_bport;
		}
	}
	// init pbitmap[8] to 0..7
	for (i=0; i<8; i++)
		pbitmap_ds_uni[i]=pbitmap_ds_mapper[i]=i;
	// get bport related pbit	
	if (me_bport_uni && me_bport_mapper) {
                omciutil_vlan_pbitmap_ds_get(me_bport_uni, pbitmap_ds_uni, 1);			
                omciutil_vlan_pbitmap_ds_get(me_bport_mapper, pbitmap_ds_mapper, 1);
	}	
	dbprintf(LOG_WARNING, "pbitmap_ds_uni[]   =%d,%d,%d,%d,%d,%d,%d,%d\n",
		pbitmap_ds_uni[0], pbitmap_ds_uni[1], pbitmap_ds_uni[2], pbitmap_ds_uni[3],
		pbitmap_ds_uni[4], pbitmap_ds_uni[5], pbitmap_ds_uni[6], pbitmap_ds_uni[7]);
	dbprintf(LOG_WARNING, "pbitmap_ds_mapper[]=%d,%d,%d,%d,%d,%d,%d,%d\n",
		pbitmap_ds_mapper[0], pbitmap_ds_mapper[1], pbitmap_ds_mapper[2], pbitmap_ds_mapper[3],
		pbitmap_ds_mapper[4], pbitmap_ds_mapper[5], pbitmap_ds_mapper[6], pbitmap_ds_mapper[7]);

	switch(action)
	{
	case ER_ATTR_GROUP_HW_OP_ADD:
		for (i = 0; i < 8; i++) 
		{
			if (pbit_bitmap & (1 << i))
			{
				int pbit=pbitmap_ds_uni[(int)pbitmap_ds_mapper[i]];		
				dbprintf(LOG_WARNING, "gem:0x%x, pbit %d -> %d\n", attr_inst->er_attr_instance[10].meid, i, pbit);

				if (switch_hw_g.qos_ds_pbit_pq_map_set == NULL || 
					(ret = switch_hw_g.qos_ds_pbit_pq_map_set(port_info.logical_port_id, pbit, pq_meid % 8) < 0))
				{
					dbprintf(LOG_ERR, "error, ret=%d\n", ret);
					return -1;
   				}
			}
		}
		break;
	case ER_ATTR_GROUP_HW_OP_DEL:
		for (i = 0; i < 8; i++) 
		{
			if (pbit_bitmap & (1 << i))
			{
				int pbit=pbitmap_ds_uni[(int)pbitmap_ds_mapper[i]];
				dbprintf(LOG_WARNING, "gem:0x%x, pbit %d -> %d\n", attr_inst->er_attr_instance[10].meid, i, pbit);

				if (switch_hw_g.qos_ds_pbit_pq_map_set == NULL || 
					(ret = switch_hw_g.qos_ds_pbit_pq_map_set(port_info.logical_port_id, pq_meid % 8, pq_meid % 8) < 0))
				{
					dbprintf(LOG_ERR, "error, ret=%d\n", ret);
					return -1;
   				}
			}
		}
		break;
	case ER_ATTR_GROUP_HW_OP_UPDATE:
		//delete
		if (er_group_hw_l2s_unipqmap_802_1p(ER_ATTR_GROUP_HW_OP_DEL, attr_inst, me, attr_mask) < 0)
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
		if (er_group_hw_l2s_unipqmap_802_1p(ER_ATTR_GROUP_HW_OP_ADD, current_attr_inst, me, attr_mask) < 0)
		{
			er_attr_group_util_release_attrinst(current_attr_inst);
			dbprintf(LOG_ERR, "fail to update for adding\n");
			return -1;
		}

		er_attr_group_util_release_attrinst(current_attr_inst);

		break;
	default:
		dbprintf(LOG_ERR, "unsupported er_group_hw method: %d\n", action);
		return -1;
	}

	return 0;
}
