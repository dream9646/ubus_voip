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
 * Module  : classf
 * File    : classf_collect.c
 *
 ******************************************************************/

#include <string.h>

#include "classf.h"
#include "util.h"
#include "meinfo.h"
#include "me_related.h"
#include "switch.h"
#include "env.h"
#include "proprietary_alu.h"
#include "omciutil_misc.h"
#include "hwresource.h"
#include "wanif.h"

struct classf_filter_op_map_t filter_op_map_g[]={	//988
	{0x00, CLASSF_FILTER_OP_TYPE_PASS,		CLASSF_FILTER_OP_TYPE_PASS, 	CLASSF_FILTER_OP_TYPE_PASS, 	CLASSF_FILTER_OP_TYPE_PASS, 	CLASSF_FILTER_INSPECTION_TYPE_UNUSED},
	{0x01, CLASSF_FILTER_OP_TYPE_DISCARD, 	CLASSF_FILTER_OP_TYPE_DISCARD, 	CLASSF_FILTER_OP_TYPE_PASS, 	CLASSF_FILTER_OP_TYPE_PASS, 	CLASSF_FILTER_INSPECTION_TYPE_UNUSED},
	{0x02, CLASSF_FILTER_OP_TYPE_PASS, 		CLASSF_FILTER_OP_TYPE_PASS, 	CLASSF_FILTER_OP_TYPE_DISCARD, 	CLASSF_FILTER_OP_TYPE_DISCARD, 	CLASSF_FILTER_INSPECTION_TYPE_UNUSED},

	{0x03, CLASSF_FILTER_OP_TYPE_POSTIVE, 	CLASSF_FILTER_OP_TYPE_POSTIVE, 	CLASSF_FILTER_OP_TYPE_PASS, 	CLASSF_FILTER_OP_TYPE_PASS, 	CLASSF_FILTER_INSPECTION_TYPE_VID},	
	{0x04, CLASSF_FILTER_OP_TYPE_POSTIVE, 	CLASSF_FILTER_OP_TYPE_POSTIVE, 	CLASSF_FILTER_OP_TYPE_DISCARD, 	CLASSF_FILTER_OP_TYPE_DISCARD, 	CLASSF_FILTER_INSPECTION_TYPE_VID},	
	{0x05, CLASSF_FILTER_OP_TYPE_NEGATIVE, 	CLASSF_FILTER_OP_TYPE_PASS, 	CLASSF_FILTER_OP_TYPE_PASS, 	CLASSF_FILTER_OP_TYPE_PASS, 	CLASSF_FILTER_INSPECTION_TYPE_VID},
	{0x06, CLASSF_FILTER_OP_TYPE_NEGATIVE, 	CLASSF_FILTER_OP_TYPE_PASS, 	CLASSF_FILTER_OP_TYPE_DISCARD, 	CLASSF_FILTER_OP_TYPE_DISCARD, 	CLASSF_FILTER_INSPECTION_TYPE_VID},
	{0x07, CLASSF_FILTER_OP_TYPE_POSTIVE, 	CLASSF_FILTER_OP_TYPE_POSTIVE, 	CLASSF_FILTER_OP_TYPE_PASS, 	CLASSF_FILTER_OP_TYPE_PASS, 	CLASSF_FILTER_INSPECTION_TYPE_PRIORITY},
	{0x08, CLASSF_FILTER_OP_TYPE_POSTIVE, 	CLASSF_FILTER_OP_TYPE_POSTIVE, 	CLASSF_FILTER_OP_TYPE_DISCARD, 	CLASSF_FILTER_OP_TYPE_DISCARD, 	CLASSF_FILTER_INSPECTION_TYPE_PRIORITY},
	{0x09, CLASSF_FILTER_OP_TYPE_NEGATIVE, 	CLASSF_FILTER_OP_TYPE_PASS, 	CLASSF_FILTER_OP_TYPE_PASS, 	CLASSF_FILTER_OP_TYPE_PASS, 	CLASSF_FILTER_INSPECTION_TYPE_PRIORITY},
	{0x0a, CLASSF_FILTER_OP_TYPE_NEGATIVE, 	CLASSF_FILTER_OP_TYPE_PASS, 	CLASSF_FILTER_OP_TYPE_DISCARD, 	CLASSF_FILTER_OP_TYPE_DISCARD, 	CLASSF_FILTER_INSPECTION_TYPE_PRIORITY},
	{0x0b, CLASSF_FILTER_OP_TYPE_POSTIVE, 	CLASSF_FILTER_OP_TYPE_POSTIVE, 	CLASSF_FILTER_OP_TYPE_PASS, 	CLASSF_FILTER_OP_TYPE_PASS, 	CLASSF_FILTER_INSPECTION_TYPE_TCI},	
	{0x0c, CLASSF_FILTER_OP_TYPE_POSTIVE, 	CLASSF_FILTER_OP_TYPE_POSTIVE, 	CLASSF_FILTER_OP_TYPE_DISCARD, 	CLASSF_FILTER_OP_TYPE_DISCARD, 	CLASSF_FILTER_INSPECTION_TYPE_TCI},	
	{0x0d, CLASSF_FILTER_OP_TYPE_NEGATIVE, 	CLASSF_FILTER_OP_TYPE_PASS, 	CLASSF_FILTER_OP_TYPE_PASS, 	CLASSF_FILTER_OP_TYPE_PASS, 	CLASSF_FILTER_INSPECTION_TYPE_TCI},
	{0x0e, CLASSF_FILTER_OP_TYPE_NEGATIVE, 	CLASSF_FILTER_OP_TYPE_PASS, 	CLASSF_FILTER_OP_TYPE_DISCARD, 	CLASSF_FILTER_OP_TYPE_DISCARD, 	CLASSF_FILTER_INSPECTION_TYPE_TCI},

	{0x0f, CLASSF_FILTER_OP_TYPE_POSTIVE, 	CLASSF_FILTER_OP_TYPE_POSTIVE, 	CLASSF_FILTER_OP_TYPE_PASS, 	CLASSF_FILTER_OP_TYPE_PASS, 	CLASSF_FILTER_INSPECTION_TYPE_VID},	
	{0x10, CLASSF_FILTER_OP_TYPE_POSTIVE, 	CLASSF_FILTER_OP_TYPE_POSTIVE, 	CLASSF_FILTER_OP_TYPE_DISCARD, 	CLASSF_FILTER_OP_TYPE_DISCARD, 	CLASSF_FILTER_INSPECTION_TYPE_VID},	
	{0x11, CLASSF_FILTER_OP_TYPE_POSTIVE, 	CLASSF_FILTER_OP_TYPE_POSTIVE, 	CLASSF_FILTER_OP_TYPE_PASS, 	CLASSF_FILTER_OP_TYPE_PASS, 	CLASSF_FILTER_INSPECTION_TYPE_PRIORITY},
	{0x12, CLASSF_FILTER_OP_TYPE_POSTIVE, 	CLASSF_FILTER_OP_TYPE_POSTIVE, 	CLASSF_FILTER_OP_TYPE_DISCARD, 	CLASSF_FILTER_OP_TYPE_DISCARD, 	CLASSF_FILTER_INSPECTION_TYPE_PRIORITY},
	{0x13, CLASSF_FILTER_OP_TYPE_POSTIVE, 	CLASSF_FILTER_OP_TYPE_POSTIVE, 	CLASSF_FILTER_OP_TYPE_PASS, 	CLASSF_FILTER_OP_TYPE_PASS, 	CLASSF_FILTER_INSPECTION_TYPE_TCI},	
	{0x14, CLASSF_FILTER_OP_TYPE_POSTIVE, 	CLASSF_FILTER_OP_TYPE_POSTIVE, 	CLASSF_FILTER_OP_TYPE_DISCARD, 	CLASSF_FILTER_OP_TYPE_DISCARD, 	CLASSF_FILTER_INSPECTION_TYPE_TCI},

	{0x15, CLASSF_FILTER_OP_TYPE_PASS, 		CLASSF_FILTER_OP_TYPE_PASS, 	CLASSF_FILTER_OP_TYPE_DISCARD, 	CLASSF_FILTER_OP_TYPE_DISCARD, 	CLASSF_FILTER_INSPECTION_TYPE_UNUSED},
	{0x16, CLASSF_FILTER_OP_TYPE_PASS, 		CLASSF_FILTER_OP_TYPE_POSTIVE, 	CLASSF_FILTER_OP_TYPE_PASS, 	CLASSF_FILTER_OP_TYPE_PASS, 	CLASSF_FILTER_INSPECTION_TYPE_VID},
	{0x17, CLASSF_FILTER_OP_TYPE_PASS, 		CLASSF_FILTER_OP_TYPE_POSTIVE, 	CLASSF_FILTER_OP_TYPE_DISCARD, 	CLASSF_FILTER_OP_TYPE_DISCARD, 	CLASSF_FILTER_INSPECTION_TYPE_VID},
	{0x18, CLASSF_FILTER_OP_TYPE_PASS, 		CLASSF_FILTER_OP_TYPE_POSTIVE, 	CLASSF_FILTER_OP_TYPE_PASS, 	CLASSF_FILTER_OP_TYPE_PASS, 	CLASSF_FILTER_INSPECTION_TYPE_PRIORITY},
	{0x19, CLASSF_FILTER_OP_TYPE_PASS, 		CLASSF_FILTER_OP_TYPE_POSTIVE, 	CLASSF_FILTER_OP_TYPE_DISCARD, 	CLASSF_FILTER_OP_TYPE_DISCARD, 	CLASSF_FILTER_INSPECTION_TYPE_PRIORITY},
	{0x1a, CLASSF_FILTER_OP_TYPE_PASS, 		CLASSF_FILTER_OP_TYPE_POSTIVE, 	CLASSF_FILTER_OP_TYPE_PASS, 	CLASSF_FILTER_OP_TYPE_PASS, 	CLASSF_FILTER_INSPECTION_TYPE_TCI},
	{0x1b, CLASSF_FILTER_OP_TYPE_PASS, 		CLASSF_FILTER_OP_TYPE_POSTIVE, 	CLASSF_FILTER_OP_TYPE_DISCARD, 	CLASSF_FILTER_OP_TYPE_DISCARD, 	CLASSF_FILTER_INSPECTION_TYPE_TCI},

	{0x1c, CLASSF_FILTER_OP_TYPE_POSTIVE, 	CLASSF_FILTER_OP_TYPE_POSTIVE, 	CLASSF_FILTER_OP_TYPE_PASS, 	CLASSF_FILTER_OP_TYPE_PASS, 	CLASSF_FILTER_INSPECTION_TYPE_VID},
	{0x1d, CLASSF_FILTER_OP_TYPE_POSTIVE, 	CLASSF_FILTER_OP_TYPE_POSTIVE, 	CLASSF_FILTER_OP_TYPE_DISCARD, 	CLASSF_FILTER_OP_TYPE_DISCARD, 	CLASSF_FILTER_INSPECTION_TYPE_VID},
	{0x1e, CLASSF_FILTER_OP_TYPE_POSTIVE, 	CLASSF_FILTER_OP_TYPE_POSTIVE, 	CLASSF_FILTER_OP_TYPE_PASS, 	CLASSF_FILTER_OP_TYPE_PASS, 	CLASSF_FILTER_INSPECTION_TYPE_PRIORITY},
	{0x1f, CLASSF_FILTER_OP_TYPE_POSTIVE, 	CLASSF_FILTER_OP_TYPE_POSTIVE, 	CLASSF_FILTER_OP_TYPE_DISCARD, 	CLASSF_FILTER_OP_TYPE_DISCARD, 	CLASSF_FILTER_INSPECTION_TYPE_PRIORITY},
	{0x20, CLASSF_FILTER_OP_TYPE_POSTIVE, 	CLASSF_FILTER_OP_TYPE_POSTIVE, 	CLASSF_FILTER_OP_TYPE_PASS, 	CLASSF_FILTER_OP_TYPE_PASS, 	CLASSF_FILTER_INSPECTION_TYPE_TCI},
	{0x21, CLASSF_FILTER_OP_TYPE_POSTIVE, 	CLASSF_FILTER_OP_TYPE_POSTIVE, 	CLASSF_FILTER_OP_TYPE_DISCARD, 	CLASSF_FILTER_OP_TYPE_DISCARD, 	CLASSF_FILTER_INSPECTION_TYPE_TCI},
	{0xff, 0, 0, 0, 0, 0} //end
};

unsigned int rule_count_us = 0, rule_count_ds = 0;
unsigned int rule_candi_count_us = 0, rule_candi_count_ds = 0;

int omci_get_olt_vendor(void);


static int
classf_translate_filter_op(unsigned char op, struct classf_port_filter_t *port_filter)
{
	unsigned int i;

	if (port_filter == NULL)
	{
		dbprintf(LOG_ERR, "param error!!\n");
		return -1;
	}

	for (i = 0; filter_op_map_g[i].op != 0xff; i++)
	{
		if (filter_op_map_g[i].op == op)
		{
			port_filter->tag_rx = filter_op_map_g[i].tag_rx;
			port_filter->tag_tx = filter_op_map_g[i].tag_tx;
			port_filter->untag_rx = filter_op_map_g[i].untag_rx;
			port_filter->untag_tx = filter_op_map_g[i].untag_tx;
			port_filter->vlan_inspection_type = filter_op_map_g[i].inspection_type;
			
			return 0;
		}
	}

	dbprintf(LOG_ERR, "op not found!!\n");
	return -1;
}

//0: NOT multicast ds bridge wan port, 1: IS multicast ds bridge wan port
static int
classf_is_multicast_ds_port(struct me_t *bridge_port_me)
{
	struct attr_value_t attr_bridge_port_tp_type={0, NULL}, attr_bridge_port_tp_pointer={0, NULL}, attr_gem_ctp_pointer={0, NULL}, attr_gem_port_dir={0, NULL};
	struct me_t *me_268, *me_281;

	if (bridge_port_me == NULL)
	{
		dbprintf(LOG_ERR, "param error!!\n");
		return 0;
	}

	//according TP type, collect port gems
	meinfo_me_attr_get(bridge_port_me, 3, &attr_bridge_port_tp_type); //determine me type
	meinfo_me_attr_get(bridge_port_me, 4, &attr_bridge_port_tp_pointer); //tp pointer

	//tp type
	if (attr_bridge_port_tp_type.data ==6) //multicast wan port
	{
		if ((me_281 = meinfo_me_get_by_meid(281, attr_bridge_port_tp_pointer.data)) == NULL)
		{
			//no gem iwtp found, this pbit not allowed
		} else {
			meinfo_me_attr_get(me_281, 1, &attr_gem_ctp_pointer); //gem ctp pointer
			if ((me_268 = meinfo_me_get_by_meid(268, attr_gem_ctp_pointer.data)) == NULL)
			{
				//no gem ctp found, this pbit not allowed
			} else {
				//get gem port value
				meinfo_me_attr_get(me_268, 3, &attr_gem_port_dir);
				if (attr_gem_port_dir.data == CLASSF_GEM_DIR_DS)
				{
					return 1; //multicast ds wan port
				}
			}
		}
	}

	return 0;
}

static int
classf_collect_port_filter(struct classf_bridge_info_t *bridge_info,
	struct me_t *bridge_port_me,
	struct classf_port_filter_t *port_filter)
{
	struct meinfo_t *miptr=meinfo_util_miptr(84);
	struct me_t *filter_me;
	struct attr_value_t attr_filter_list={0, NULL};
	struct attr_value_t attr_filter_op={0, NULL};
	struct attr_value_t attr_filter_entry_total={0, NULL};
	unsigned char *bitmap;
	unsigned int i;
	unsigned int alu_workaround_flag = 0;

	if (bridge_info == NULL || bridge_port_me == NULL || port_filter == NULL)
	{
		dbprintf(LOG_ERR, "param error!!\n");
		return -1;
	}

	port_filter->bridge_me = bridge_info->me_bridge;
	port_filter->bridge_port_me = bridge_port_me;
	port_filter->filter_me = NULL; //init for no filter found

	//ALU FTTU Multicast GEM port VTF workaround
	//set op type for all pass if filter found
	if (omci_env_g.olt_workaround_enable == ENV_OLT_WORKAROUND_ALU &&
		(proprietary_alu_get_olt_version() == 0) &&
		classf_is_multicast_ds_port(bridge_port_me))
	{
		alu_workaround_flag = 1;
	}
	
	list_for_each_entry(filter_me, &miptr->me_instance_list, instance_node)
	{
		if (me_related(filter_me, bridge_port_me))
		{
			//retrive tci from filter me
			meinfo_me_attr_get(filter_me, 1, &attr_filter_list);
			meinfo_me_attr_get(filter_me, 2, &attr_filter_op);
			meinfo_me_attr_get(filter_me, 3, &attr_filter_entry_total);

			port_filter->vlan_tci_num = attr_filter_entry_total.data;

			if ((bitmap=(unsigned char *) attr_filter_list.ptr))
			{
				for (i=0; i<attr_filter_entry_total.data; i++)
				{
					port_filter->vlan_tci[i] = util_bitmap_get_value(bitmap, 24*8, i*16, 16);

					//alu workaround, vid=0xfff, priority=0x7, set op type for all pass
					if (omci_env_g.olt_workaround_enable == ENV_OLT_WORKAROUND_ALU &&
						(proprietary_alu_get_olt_version() == 1) &&
						(((port_filter->vlan_tci[i] & 0xe000) >> 13) == 0x7) &&
						((port_filter->vlan_tci[i] & 0x0fff) == 0xfff))
					{
						unsigned char attr_mask[2]={0, 0};
						alu_workaround_flag = 1;
						attr_filter_op.data = 0; // do not filter any vlan
						meinfo_me_attr_set(filter_me, 2, &attr_filter_op, 0);
						util_attr_mask_set_bit(attr_mask, 2);
						meinfo_me_flush(filter_me, attr_mask);

						//set low priority;
						port_filter->low_priority = 1;
					}
				}
			}

			//translate op to port_filter structure
			classf_translate_filter_op(attr_filter_op.data, port_filter);

			//alu workaround, reset op type for all pass
			if (alu_workaround_flag)
			{
				port_filter->untag_rx = CLASSF_FILTER_OP_TYPE_PASS;
				port_filter->untag_tx = CLASSF_FILTER_OP_TYPE_PASS;
				port_filter->tag_rx = CLASSF_FILTER_OP_TYPE_PASS;
				port_filter->tag_tx = CLASSF_FILTER_OP_TYPE_PASS;
			}
			
			port_filter->filter_me = filter_me;

			meinfo_me_attr_release(84, 1, &attr_filter_list);

			break;
		}
	}

	return 0;
}

/*
static int
classf_collect_port_gem_analyse_dscp2pbit_mask(char *dscp2pbit_bitmap, unsigned char *dscp2pbit_table, unsigned char *pbit_mask)
{
	unsigned int i;
	unsigned char pbit;

	if (dscp2pbit_bitmap == NULL || 
		dscp2pbit_table == NULL ||
		pbit_mask == NULL)
	{
		dbprintf(LOG_ERR, "param error!!\n");
		return -1;
	}

	*pbit_mask = 0;
	for (i = 0; i < 64; i++) //64 dscp value
	{
		pbit = util_bitmap_get_value((unsigned char *)dscp2pbit_bitmap, 8*24, i*3, 3);
		dscp2pbit_table[i] = pbit;
		*pbit_mask |= (0x1 << pbit);
	}

	return 0;
}
*/

static struct me_t *
classf_collect_port_gem_get_pq_me_us(struct me_t *me) //gem
{
	if (me == NULL)
	{
		dbprintf(LOG_ERR, "param error!!\n");
		return NULL;
	}

	//256's attr4 == 1 (Rate controlled) 
	if (omciutil_misc_traffic_management_option_is_rate_control())
	{ 
		struct me_t *me_tcont = meinfo_me_get_by_meid(262, (unsigned short)me->attr[4].value.data);
		struct me_t *private_me_gem=hwresource_public2private(me);
		struct me_t *pq_me;
		struct attr_value_t attr={0, NULL};
		
		if ((me_tcont == NULL) || (private_me_gem == NULL))
		{
			return NULL;
		}
		
		// pq pointer of igem me
		meinfo_me_attr_get(private_me_gem, 5, &attr);

		if ((pq_me = meinfo_me_get_by_meid(277,  (unsigned short)attr.data)) != NULL &&
			me_related(pq_me, me_tcont)) // igem pq pointer points to pq and && gem/pq belong to same me_tcont)
		{
			return pq_me;
		}
	} else { 
		//256's attr4 == 0 or 2 (Priority controlled or Priority and rate controlled)
		// (268's attr4 (Traffic_management_pointer_for_upstream) == 277's meid)
		return meinfo_me_get_by_meid(277, (unsigned short)me->attr[4].value.data);
	}
	
	return NULL;
}

static int
classf_collect_port_gem(struct classf_bridge_info_wan_port_t *wan_port, struct omciutil_vlan_dscp2pbit_info_t *dscp2pbit_info)
{
	struct attr_value_t attr_bridge_port_tp_type={0, NULL}, attr_bridge_port_tp_pointer={0, NULL}, attr_untag_option={0, NULL}, attr_default_pbit={0, NULL}, attr_gem_iwtp_pointer={0, NULL};
	struct attr_value_t attr_gem_ctp_pointer={0, NULL}, attr_gem_port_value={0, NULL}, attr_gem_iwtp_pointer2={0, NULL}, attr_gem_port_dir={0, NULL};
	struct attr_value_t attr_drop_thresholds_value={0, NULL}, attr_colour_marking={0, NULL};
	struct me_t *me_130, *me_266, *me_268, *me_281, *pq_me;
	unsigned int i, j;
	unsigned default_pbit;
	unsigned char checked_mask, gem_pbit_mask, untag_pbit_mask;
	unsigned short green_max, yellow_max;
	unsigned char enable_flows_distinguish;
	int found;
	unsigned char buf[8];

	if (wan_port == NULL || dscp2pbit_info == NULL)
	{
		dbprintf(LOG_ERR, "param error!!\n");
		return -1;
	}

	//according TP type, collect port gems
	meinfo_me_attr_get(wan_port->me_bridge_port, 3, &attr_bridge_port_tp_type); //determine me type
	meinfo_me_attr_get(wan_port->me_bridge_port, 4, &attr_bridge_port_tp_pointer); //tp pointer

	wan_port->port_gem.bridge_me = wan_port->me_bridge;
	wan_port->port_gem.bridge_port_me = wan_port->me_bridge_port;
	wan_port->port_gem.gem_index_total = 0; //means no gem ports found
	wan_port->port_gem.gem_num = 0; //means no gem ports found

	//tp type
	switch (attr_bridge_port_tp_type.data)
	{
	case 3: //802.1p(130)
		wan_port->port_gem.gem_type = 0; //regular data

		//get 8021p me
		if ((me_130 = meinfo_me_get_by_meid(130, attr_bridge_port_tp_pointer.data)) == NULL)
		{
			dbprintf(LOG_ERR, "class 130 me not found!!\n");
			return -1;
		}

		checked_mask = 0;

		for (i = 0; i < 8; i++)
		{
			if (!((checked_mask >> i) & 0x1))
			{
				gem_pbit_mask = 0;
				meinfo_me_attr_get(me_130, i+2, &attr_gem_iwtp_pointer); //gem iwtp pointer
				if ((me_266 = meinfo_me_get_by_meid(266, attr_gem_iwtp_pointer.data)) == NULL)
				{
					//no gem iwtp found, this pbit not allowed
				} else {
					meinfo_me_attr_get(me_266, 1, &attr_gem_ctp_pointer); //gem ctp pointer
					if ((me_268 = meinfo_me_get_by_meid(268, attr_gem_ctp_pointer.data)) == NULL)
					{
						//no gem ctp found, this pbit not allowed
					} else {
						//get gem port value
						meinfo_me_attr_get(me_268, 1, &attr_gem_port_value);
						meinfo_me_attr_get(me_268, 3, &attr_gem_port_dir);

						//get pq colour marking
						enable_flows_distinguish = 0;
						
						if ((pq_me = classf_collect_port_gem_get_pq_me_us(me_268)) != NULL)
						{
							//get drop pq thresholds, 8 bytes ptr.
							memset(buf, 0x00, sizeof(buf));
							attr_drop_thresholds_value.ptr = buf;
							meinfo_me_attr_get(pq_me, 13, &attr_drop_thresholds_value); //drop thresholds
							meinfo_me_attr_get(pq_me, 16, &attr_colour_marking); //colour_marking
							
							//check max value, green max > yellow max
							green_max = util_bitmap_get_value(attr_drop_thresholds_value.ptr, 8*8, 16, 16);
							yellow_max = util_bitmap_get_value(attr_drop_thresholds_value.ptr, 8*8, 48, 16);

							if (green_max != 0 && yellow_max != 0 && yellow_max < green_max &&
								attr_colour_marking.data >= 2 && attr_colour_marking.data <= 6) //marking by DEI, 8P0D, 7P1D, 6P2D, 5P3D
							{
								//enable flows distinguish process
								enable_flows_distinguish = 1;
							}
						}

						gem_pbit_mask |= (0x1 << i);

						//find other one
						for (j=i+1; j < 8; j++)
						{
							meinfo_me_attr_get(me_130, j+2, &attr_gem_iwtp_pointer2); //gem iwtp pointer
							if (attr_gem_iwtp_pointer2.data == attr_gem_iwtp_pointer.data)
							{
								gem_pbit_mask |= (0x1 << j);
								checked_mask |= (0x1 << j);
							}
						}
						
						wan_port->port_gem.gem_tagged[wan_port->port_gem.gem_index_total] = 1; //tagged
						wan_port->port_gem.gem_port[wan_port->port_gem.gem_index_total] = attr_gem_port_value.data;
						wan_port->port_gem.gem_ctp_me[wan_port->port_gem.gem_index_total] = me_268;
						wan_port->port_gem.pbit_mask[wan_port->port_gem.gem_index_total] = gem_pbit_mask;
						wan_port->port_gem.dir[wan_port->port_gem.gem_index_total] = attr_gem_port_dir.data;
						if (enable_flows_distinguish == 1)
						{
							//enable
							wan_port->port_gem.colour_marking_us[wan_port->port_gem.gem_index_total] = attr_colour_marking.data;
						}
						
						wan_port->port_gem.gem_index_total++;
						wan_port->port_gem.gem_num++;
					}
				}

				checked_mask |= (0x1 << i);
			}
		}

		//check untag pbit gem
		meinfo_me_attr_get(me_130, 10, &attr_untag_option);
		
		switch(attr_untag_option.data)
		{
		case 0: //from dscp
			//collect dscp2pbit info by this wan port
			if ((omciutil_vlan_collect_130_dscp2pbit_usage_by_port(wan_port->me_bridge_port, &(wan_port->port_gem.dscp2pbit_info)) < 0) ||
				wan_port->port_gem.dscp2pbit_info.enable == 0) //disbale
			{
				dbprintf(LOG_ERR, "collect wan port dscp-2-pbit error, bridge portme=0x%.4x!!\n", wan_port->me_bridge_port->meid);
			}

			wan_port->port_gem.untag_op = CLASSF_GEM_UNTAG_OP_DSCP;

			if (omci_env_g.classf_dscp2pbit_mode == 0)
			{
				untag_pbit_mask = wan_port->port_gem.dscp2pbit_info.pbit_mask;
			} else {
				untag_pbit_mask = dscp2pbit_info->pbit_mask;
			}

			//check with gem pbit mask
			for (i = 0; i < 8; i++)
			{
				if (untag_pbit_mask & (0x1 << i))
				{
					found = 0;
					for (j = 0; j < wan_port->port_gem.gem_num; j++)
					{
						if (wan_port->port_gem.pbit_mask[j] & (0x1 << i))
						{
							found = 1;
							break;
						}
					}
					if (found)
					{
						//add untag gem index
						wan_port->port_gem.gem_tagged[wan_port->port_gem.gem_index_total] = 0; //untagged
						wan_port->port_gem.gem_port[wan_port->port_gem.gem_index_total] = wan_port->port_gem.gem_port[j];
						wan_port->port_gem.gem_ctp_me[wan_port->port_gem.gem_index_total] = wan_port->port_gem.gem_ctp_me[j];
						wan_port->port_gem.pbit_mask[wan_port->port_gem.gem_index_total] = (0x1 << i); //untag
						wan_port->port_gem.dir[wan_port->port_gem.gem_index_total] = wan_port->port_gem.dir[j];

						wan_port->port_gem.gem_index_total++;
					}
				}
			}
	
			break;
		case 1: //set default pbit
			meinfo_me_attr_get(me_130, 12, &attr_default_pbit); //default pbit
			default_pbit = attr_default_pbit.data < 8? attr_default_pbit.data : 7;
			wan_port->port_gem.untag_op = CLASSF_GEM_UNTAG_OP_DEFAULT_PBIT;

			//check with gem pbit mask
			found = 0;
			for (j = 0; j < wan_port->port_gem.gem_num; j++)
			{
				if (wan_port->port_gem.pbit_mask[j] & (0x1 << default_pbit))
				{
					found = 1;
					break;
				}
			}
			if (found)
			{
				//add untag gem index
				wan_port->port_gem.gem_tagged[wan_port->port_gem.gem_index_total] = 0; //untagged
				wan_port->port_gem.gem_port[wan_port->port_gem.gem_index_total] = wan_port->port_gem.gem_port[j];
				wan_port->port_gem.gem_ctp_me[wan_port->port_gem.gem_index_total] = wan_port->port_gem.gem_ctp_me[j];
				wan_port->port_gem.pbit_mask[wan_port->port_gem.gem_index_total] = (0x1 << default_pbit); //untag
				wan_port->port_gem.dir[wan_port->port_gem.gem_index_total] = wan_port->port_gem.dir[j];

				wan_port->port_gem.gem_index_total++;
			}
			
			break;
		default: //unmark frame not allowed
			wan_port->port_gem.untag_op = CLASSF_GEM_UNTAG_OP_NO_PASS;			
		}

		break;
	case 5: //gem iwtp
		wan_port->port_gem.gem_type = 0; //regular data

		if ((me_266 = meinfo_me_get_by_meid(266, attr_bridge_port_tp_pointer.data)) == NULL)
		{
			//no gem iwtp found, this pbit not allowed
		} else {
			meinfo_me_attr_get(me_266, 1, &attr_gem_ctp_pointer); //gem ctp pointer
			if ((me_268 = meinfo_me_get_by_meid(268, attr_gem_ctp_pointer.data)) == NULL)
			{
				//no gem ctp found, this pbit not allowed
			} else {
				//get gem port value
				meinfo_me_attr_get(me_268, 1, &attr_gem_port_value);
				meinfo_me_attr_get(me_268, 3, &attr_gem_port_dir);

				//get pq colour marking
				enable_flows_distinguish = 0;

				if ((pq_me = classf_collect_port_gem_get_pq_me_us(me_268)) != NULL)
				{
					//get drop pq thresholds, 8 bytes ptr.
					memset(buf, 0x00, sizeof(buf));
					attr_drop_thresholds_value.ptr = buf;
					meinfo_me_attr_get(pq_me, 13, &attr_drop_thresholds_value); //drop thresholds
					meinfo_me_attr_get(pq_me, 16, &attr_colour_marking); //colour_marking
					
					//check max value, green max > yellow max
					green_max = util_bitmap_get_value(attr_drop_thresholds_value.ptr, 8*8, 16, 16);
					yellow_max = util_bitmap_get_value(attr_drop_thresholds_value.ptr, 8*8, 48, 16);

					if (green_max != 0 && yellow_max != 0 && yellow_max < green_max &&
						attr_colour_marking.data >= 2 && attr_colour_marking.data <= 6) //marking by DEI, 8P0D, 7P1D, 6P2D, 5P3D
					{
						//enable flows distinguish process
						enable_flows_distinguish = 1;
					}
				}

				wan_port->port_gem.gem_tagged[wan_port->port_gem.gem_index_total] = 1; //tagged
				wan_port->port_gem.gem_port[wan_port->port_gem.gem_index_total] = attr_gem_port_value.data;
				wan_port->port_gem.gem_ctp_me[wan_port->port_gem.gem_index_total] = me_268;
				wan_port->port_gem.pbit_mask[wan_port->port_gem.gem_index_total] = 0xff;
				wan_port->port_gem.dir[wan_port->port_gem.gem_index_total] = attr_gem_port_dir.data;
				if (enable_flows_distinguish == 1)
				{
					//enable
					wan_port->port_gem.colour_marking_us[wan_port->port_gem.gem_index_total] = attr_colour_marking.data;
				}

				wan_port->port_gem.gem_index_total++;
				wan_port->port_gem.gem_num++;

				//untag
				wan_port->port_gem.untag_op = CLASSF_GEM_UNTAG_OP_ALL_PASS;
				wan_port->port_gem.gem_tagged[wan_port->port_gem.gem_index_total] = 0; //untagged
				wan_port->port_gem.gem_port[wan_port->port_gem.gem_index_total] = attr_gem_port_value.data;
				wan_port->port_gem.gem_ctp_me[wan_port->port_gem.gem_index_total] = me_268;
				wan_port->port_gem.pbit_mask[wan_port->port_gem.gem_index_total] = 0xff;
				wan_port->port_gem.dir[wan_port->port_gem.gem_index_total] = attr_gem_port_dir.data;

				wan_port->port_gem.gem_index_total++;
			}
		}

		break;
	case 6: //multicast gem iwtp
		wan_port->port_gem.gem_type = 1; //multicast

		if ((me_281 = meinfo_me_get_by_meid(281, attr_bridge_port_tp_pointer.data)) == NULL)
		{
			//no gem iwtp found, this pbit not allowed
		} else {
			meinfo_me_attr_get(me_281, 1, &attr_gem_ctp_pointer); //gem ctp pointer
			if ((me_268 = meinfo_me_get_by_meid(268, attr_gem_ctp_pointer.data)) == NULL)
			{
				//no gem ctp found, this pbit not allowed
			} else {
				//get gem port value
				meinfo_me_attr_get(me_268, 1, &attr_gem_port_value);
				meinfo_me_attr_get(me_268, 3, &attr_gem_port_dir);

				wan_port->port_gem.gem_tagged[wan_port->port_gem.gem_index_total] = 1; //tagged
				wan_port->port_gem.gem_port[wan_port->port_gem.gem_index_total] = attr_gem_port_value.data;
				wan_port->port_gem.gem_ctp_me[wan_port->port_gem.gem_index_total] = me_268;
				wan_port->port_gem.pbit_mask[wan_port->port_gem.gem_index_total] = 0xff;
				wan_port->port_gem.dir[wan_port->port_gem.gem_index_total] = attr_gem_port_dir.data;

				wan_port->port_gem.gem_index_total++;
				wan_port->port_gem.gem_num++;

				//untag
				wan_port->port_gem.untag_op = CLASSF_GEM_UNTAG_OP_ALL_PASS;
				wan_port->port_gem.gem_tagged[wan_port->port_gem.gem_index_total] = 0; //untagged
				wan_port->port_gem.gem_port[wan_port->port_gem.gem_index_total] = attr_gem_port_value.data;
				wan_port->port_gem.gem_ctp_me[wan_port->port_gem.gem_index_total] = me_268;
				wan_port->port_gem.pbit_mask[wan_port->port_gem.gem_index_total] = 0xff;
				wan_port->port_gem.dir[wan_port->port_gem.gem_index_total] = attr_gem_port_dir.data;

				wan_port->port_gem.gem_index_total++;
			}
		}

		break;
	default:
		dbprintf(LOG_ERR, "incorrect tp type!!\n");
		return -1;
	}

	return 0;
}

#if 0 // Remove VEIP related workaround
static void
classf_add_rules_by_real_tci_on_veip_ports(struct list_head *us_tagging_rule_list, struct list_head *ds_tagging_rule_list,struct me_t *bridge_port_me, unsigned char port_index)
{
	struct omciutil_vlan_rule_fields_t *rule_fields;

	if (us_tagging_rule_list == NULL ||
		ds_tagging_rule_list == NULL ||
		bridge_port_me == NULL)
	{
		dbprintf(LOG_ERR, "param error!!\n");
		return;
	}

	//upstream rule, untag, add one expected tag
	rule_fields = malloc_safe(sizeof(struct omciutil_vlan_rule_fields_t));
	rule_fields->owner_me=NULL;
	rule_fields->bport_me=bridge_port_me;
	rule_fields->sort_priority = omci_env_g.classf_veip_real_tag_rule_priority;
	rule_fields->unreal_flag = 0; //real

	rule_fields->filter_outer.priority=15;
	rule_fields->filter_outer.vid=4096;
	rule_fields->filter_outer.tpid_de=0;
	rule_fields->filter_inner.priority=15;
	rule_fields->filter_inner.vid=4096;
	rule_fields->filter_inner.tpid_de=0;

	rule_fields->filter_ethertype=OMCIUTIL_VLAN_FILTER_ET_IGNORE;
	rule_fields->treatment_tag_to_remove=0;

	rule_fields->treatment_outer.priority=15;
	rule_fields->treatment_outer.vid=0;
	rule_fields->treatment_outer.tpid_de=0;

	rule_fields->treatment_inner.priority=((omci_env_g.classf_veip_real_tci_map[port_index] >> 13) & 0x7);
	if (((omci_env_g.classf_veip_real_tci_map[port_index] >> 12) & 0x1) == 1)
	{
		rule_fields->treatment_inner.tpid_de = OMCIUTIL_VLAN_TREATMENT_TPID_DE_1011;
	} else {
		rule_fields->treatment_inner.tpid_de = OMCIUTIL_VLAN_TREATMENT_TPID_DE_1010;
	}
	rule_fields->treatment_inner.vid = (omci_env_g.classf_veip_real_tci_map[port_index] & 0xfff);

	if (omciutil_vlan_insert_rule_fields_to_list(rule_fields, us_tagging_rule_list) < 0)
	{
		dbprintf(LOG_ERR, "can not insert expected us rule, bridge port meid=%u!!\n", bridge_port_me->meid);
		//release entry_fields
		free_safe(rule_fields);
	}

	//downstream rule, one tag, remove this tag
	rule_fields = malloc_safe(sizeof(struct omciutil_vlan_rule_fields_t));
	rule_fields->owner_me=NULL;
	rule_fields->bport_me=bridge_port_me;
	rule_fields->sort_priority = omci_env_g.classf_veip_real_tag_rule_priority;
	rule_fields->unreal_flag = 0; //real

	rule_fields->filter_outer.priority=15;
	rule_fields->filter_outer.vid=4096;
	rule_fields->filter_outer.tpid_de=0;

	rule_fields->filter_inner.priority=((omci_env_g.classf_veip_real_tci_map[port_index] >> 13) & 0x7);
	if (((omci_env_g.classf_veip_real_tci_map[port_index] >> 12) & 0x1) == 1)
	{
		rule_fields->filter_inner.tpid_de = OMCIUTIL_VLAN_FILTER_TPID_DE_1100;
	} else {
		rule_fields->filter_inner.tpid_de = OMCIUTIL_VLAN_FILTER_TPID_DE_1011;
	}
	rule_fields->filter_inner.vid = (omci_env_g.classf_veip_real_tci_map[port_index] & 0xfff);

	rule_fields->filter_ethertype=OMCIUTIL_VLAN_FILTER_ET_IGNORE;
	rule_fields->treatment_tag_to_remove=1; //remove

	rule_fields->treatment_outer.priority=15;
	rule_fields->treatment_outer.vid=0;
	rule_fields->treatment_outer.tpid_de=0;
	rule_fields->treatment_inner.priority=15;
	rule_fields->treatment_inner.vid=0;
	rule_fields->treatment_inner.tpid_de=0;

	if (omciutil_vlan_insert_rule_fields_to_list(rule_fields, ds_tagging_rule_list) < 0)
	{
		dbprintf(LOG_ERR, "can not insert expected ds rule, bridge port meid=%u!!\n", bridge_port_me->meid);
		//release entry_fields
		free_safe(rule_fields);
	}

	return;
}
#endif

static void
classf_assign_default_tagging_rules(struct list_head *tagging_rule_list, struct me_t *bridge_port_me, unsigned char mode, unsigned char tag_action) // tag_action: 0: 2 tags rules passthrough, 1: 2 tags rules remove 1 tag
{
	unsigned int count;
	struct omciutil_vlan_rule_fields_t *rule_fields;

	if (tagging_rule_list == NULL || bridge_port_me == NULL)
	{
		dbprintf(LOG_ERR, "param error!!\n");
		return;
	}

	//untag
	rule_fields = malloc_safe(sizeof(struct omciutil_vlan_rule_fields_t));
	rule_fields->owner_me=NULL;
	rule_fields->bport_me=bridge_port_me;
	rule_fields->sort_priority = OMCIUTIL_VLAN_SORT_PRIORITY_LOWEST; //lowest priority
	rule_fields->unreal_flag = 0; //real

	rule_fields->filter_outer.priority=15;
	rule_fields->filter_outer.vid=4096;
	rule_fields->filter_outer.tpid_de=0;
	rule_fields->filter_inner.priority=15;
	rule_fields->filter_inner.vid=4096;
	rule_fields->filter_inner.tpid_de=0;

	rule_fields->filter_ethertype=OMCIUTIL_VLAN_FILTER_ET_IGNORE;
	rule_fields->treatment_tag_to_remove=0;

	rule_fields->treatment_outer.priority=15;
	rule_fields->treatment_outer.vid=0;
	rule_fields->treatment_outer.tpid_de=0;
	rule_fields->treatment_inner.priority=15;
	rule_fields->treatment_inner.vid=0;
	rule_fields->treatment_inner.tpid_de=0;

	if (omciutil_vlan_insert_rule_fields_to_list(rule_fields, tagging_rule_list) < 0)
	{
		dbprintf(LOG_ERR, "can not insert default untag rule, bridge port meid=%u!!\n", bridge_port_me->meid);
		//release entry_fields
		free_safe(rule_fields);
	}

	//one tag
	rule_fields = malloc_safe(sizeof(struct omciutil_vlan_rule_fields_t));
	rule_fields->owner_me=NULL;
	rule_fields->bport_me=bridge_port_me;
	rule_fields->sort_priority = OMCIUTIL_VLAN_SORT_PRIORITY_LOWEST; //lowest priority
	rule_fields->unreal_flag = 0; //real

	rule_fields->filter_outer.priority=15;
	rule_fields->filter_outer.vid=4096;
	rule_fields->filter_outer.tpid_de=0;
	rule_fields->filter_inner.priority=14;
	rule_fields->filter_inner.vid=4096;
	rule_fields->filter_inner.tpid_de=0;

	rule_fields->filter_ethertype=OMCIUTIL_VLAN_FILTER_ET_IGNORE;
	rule_fields->treatment_tag_to_remove=0;

	rule_fields->treatment_outer.priority=15;
	rule_fields->treatment_outer.vid=0;
	rule_fields->treatment_outer.tpid_de=0;
	rule_fields->treatment_inner.priority=15;
	rule_fields->treatment_inner.vid=0;
	rule_fields->treatment_inner.tpid_de=0;

	if (omciutil_vlan_insert_rule_fields_to_list(rule_fields, tagging_rule_list) < 0)
	{
		dbprintf(LOG_ERR, "can not insert default one tag rule, bridge port meid=%u!!\n", bridge_port_me->meid);
		//release entry_fields
		free_safe(rule_fields);
	}

	if (mode == 0) //collect all rules(include 2 tags)
	{
		//two tags
		rule_fields = malloc_safe(sizeof(struct omciutil_vlan_rule_fields_t));
		rule_fields->owner_me=NULL;
		rule_fields->bport_me=bridge_port_me;
		rule_fields->sort_priority = OMCIUTIL_VLAN_SORT_PRIORITY_LOWEST; //lowest priority
		rule_fields->unreal_flag = 0; //real

		rule_fields->filter_outer.priority=14;
		rule_fields->filter_outer.vid=4096;
		rule_fields->filter_outer.tpid_de=0;
		rule_fields->filter_inner.priority=14;
		rule_fields->filter_inner.vid=4096;
		rule_fields->filter_inner.tpid_de=0;

		rule_fields->filter_ethertype=OMCIUTIL_VLAN_FILTER_ET_IGNORE;

		if (tag_action)
		{
			rule_fields->treatment_tag_to_remove=1;
		} else {
			rule_fields->treatment_tag_to_remove=0;
		}

		rule_fields->treatment_outer.priority=15;
		rule_fields->treatment_outer.vid=0;
		rule_fields->treatment_outer.tpid_de=0;
		rule_fields->treatment_inner.priority=15;
		rule_fields->treatment_inner.vid=0;
		rule_fields->treatment_inner.tpid_de=0;

		if (omciutil_vlan_insert_rule_fields_to_list(rule_fields, tagging_rule_list) < 0)
		{
			dbprintf(LOG_ERR, "can not insert default two tag rule, bridge port meid=%u!!\n", bridge_port_me->meid);
			//release entry_fields
			free_safe(rule_fields);
		}
	}

	//assign seq num to each rule_field
	count = 0;
	list_for_each_entry(rule_fields, tagging_rule_list, rule_node)
	{
		rule_fields->entry_id = count++;
	}

	return;
}

static void
classf_bridge_assign_global_seqnum_to_rule_candi(struct classf_bridge_info_t *bridge_info)
{
	struct classf_rule_candi_t *rule_candi_pos;

	if (bridge_info == NULL)
	{
		dbprintf(LOG_ERR, "param error!!\n");
		return;
	}

	list_for_each_entry(rule_candi_pos, &bridge_info->rule_candi_list_us, rc_node)
	{
		rule_candi_pos->entry_id = rule_candi_count_us++;
	}

	list_for_each_entry(rule_candi_pos, &bridge_info->rule_candi_list_ds, rc_node)
	{
		rule_candi_pos->entry_id = rule_candi_count_ds++;
	}

	return;
}

// 1. assign global sequence number
// 2. set uni port mask
static void
classf_bridge_set_additional_info_to_rule(struct classf_bridge_info_t *bridge_info, unsigned int uni_port_mask)
{
	struct classf_rule_t *rule_pos;

	if (bridge_info == NULL)
	{
		dbprintf(LOG_ERR, "param error!!\n");
		return;
	}

	//upstream
	list_for_each_entry(rule_pos, &bridge_info->rule_list_us, r_node)
	{
		rule_pos->entry_id = rule_count_us++;
		rule_pos->uni_port_mask = uni_port_mask;
	}

	//downstream
	list_for_each_entry(rule_pos, &bridge_info->rule_list_ds, r_node)
	{
		rule_pos->entry_id = rule_count_ds++;
		rule_pos->uni_port_mask = uni_port_mask;
	}

	return;
}

#if 0 // Remove VEIP related workaround
static void
classf_modify_rules_by_orig_tci_to_distinguish_veip_ports(struct list_head *us_tagging_rule_list, struct list_head *ds_tagging_rule_list, unsigned char port_index)
{
	struct omciutil_vlan_rule_fields_t *rule_fields_pos;
	int filter_tag_num, treatment_tag_num;

	if (us_tagging_rule_list == NULL ||
		ds_tagging_rule_list == NULL)
	{
		dbprintf(LOG_ERR, "param error!!\n");
		return;
	}

	//check upstream rules
	list_for_each_entry(rule_fields_pos, us_tagging_rule_list, rule_node)
	{
		//check for rule must have no filter, then add a port-distinguished tag in filter for different veip(pon) ports
		filter_tag_num = omciutil_vlan_get_rule_filter_tag_num(rule_fields_pos);
		if (filter_tag_num > 0) //not untag rule
		{
			dbprintf(LOG_ERR, "US distinguish_veip_ports error!! port_type_index=%u, entry_id=%u, tag_num=%u\n",
				port_index,
				rule_fields_pos->entry_id,
				filter_tag_num);
			continue;
		}

		//by classf_pon_tci_base and port_type_index to add the tag to inner filter
		rule_fields_pos->filter_inner.priority = (omci_env_g.classf_veip_orig_tci_map[port_index] >> 13) & 0x7;
		if (((omci_env_g.classf_veip_orig_tci_map[port_index] >> 12) & 0x1) == 1)
		{
			rule_fields_pos->filter_inner.tpid_de = OMCIUTIL_VLAN_FILTER_TPID_DE_1100;
		} else {
			rule_fields_pos->filter_inner.tpid_de = OMCIUTIL_VLAN_FILTER_TPID_DE_1011;
		}
		rule_fields_pos->filter_inner.vid = (omci_env_g.classf_veip_orig_tci_map[port_index] & 0xfff);

		//remove this tag
		rule_fields_pos->treatment_tag_to_remove = 1;
	}

	//check downstream rules
	list_for_each_entry(rule_fields_pos, ds_tagging_rule_list, rule_node)
	{
		//check for rule result must have no filter, then add a port-distinguished tag in treatment for different cpu ports
		filter_tag_num = omciutil_vlan_get_rule_filter_tag_num(rule_fields_pos);
		treatment_tag_num = omciutil_vlan_get_rule_treatment_tag_num(rule_fields_pos);
		if (filter_tag_num - rule_fields_pos->treatment_tag_to_remove + treatment_tag_num > 0) //not untag rule
		{
			dbprintf(LOG_ERR, "DS distinguish_veip_ports error!! port_type_index=%u, entry_id=%u, result_num=%u\n",
				port_index,
				rule_fields_pos->entry_id,
				filter_tag_num - rule_fields_pos->treatment_tag_to_remove + treatment_tag_num);
			continue;
		}

		//by classf_pon_tci_base and port_type_index to add the tag to inner treatment
		rule_fields_pos->treatment_inner.priority = (omci_env_g.classf_veip_orig_tci_map[port_index] >> 13) & 0x7;
		if (((omci_env_g.classf_veip_orig_tci_map[port_index] >> 12) & 0x1) == 1)
		{
			rule_fields_pos->treatment_inner.tpid_de = OMCIUTIL_VLAN_TREATMENT_TPID_DE_1011;
		} else {
			rule_fields_pos->treatment_inner.tpid_de = OMCIUTIL_VLAN_TREATMENT_TPID_DE_1010;
		}
		rule_fields_pos->treatment_inner.vid = (omci_env_g.classf_veip_orig_tci_map[port_index] & 0xfff);
	}	

	return;
}
#endif

/*
static int
classf_duplicate_protocol_rule_list(struct list_head *pvlan_rule_list_dst, struct list_head *pvlan_rule_list_src)
{
	struct classf_pvlan_rule_t *pvlan_rule_pos, *pvlan_rule_new;

	if (pvlan_rule_list_dst == NULL ||
		pvlan_rule_list_src == NULL)
	{
		dbprintf(LOG_ERR, "param error!!\n");
		return -1;
	}

	list_for_each_entry(pvlan_rule_pos, pvlan_rule_list_src, node)
	{
		pvlan_rule_new = malloc_safe(sizeof(struct classf_pvlan_rule_t));

		//copy content?
		pvlan_rule_new->pvlan_acl = pvlan_rule_pos->pvlan_acl;
		pvlan_rule_new->rule_fields = pvlan_rule_pos->rule_fields;

		//add to des list
		list_add_tail(&pvlan_rule_new->node, pvlan_rule_list_dst);
	}

	return 0;
}
*/

static int
classf_collect_reassign_veip_rules_to_uni(struct classf_bridge_info_t *bridge_info)
{
	unsigned int found, count;
	unsigned short vid;
	struct classf_bridge_info_lan_port_t *lan_port;
	struct omciutil_vlan_rule_fields_t *rule_fields, *rule_fields_n, *rule_fields_new;
	struct list_head tmp_rule_list_us, tmp_rule_list_ds;
	struct omciutil_vlan_rule_result_list_t rule_result_list;
	struct omciutil_vlan_rule_result_node_t *result_node;

	if (bridge_info == NULL)
	{
		dbprintf(LOG_ERR, "param error!!\n");
		return -1;
	}

	INIT_LIST_HEAD(&tmp_rule_list_us);
	INIT_LIST_HEAD(&tmp_rule_list_ds);

	//collect and remove rules
	list_for_each_entry(lan_port, &bridge_info->lan_port_list, lp_node)
	{
		if (lan_port->port_type == ENV_BRIDGE_PORT_TYPE_CPU)
		{
			//check upstream list
			list_for_each_entry_safe(rule_fields, rule_fields_n, &lan_port->port_tagging.us_tagging_rule_list, rule_node)
			{
				if (rule_fields->owner_me != NULL && rule_fields->owner_me->classid == 171)
				{
					if (omciutil_vlan_get_rule_filter_tag_num(rule_fields) == 0 || //untag rules
						(omciutil_vlan_get_rule_filter_tag_num(rule_fields) == 1 && //priority tag rules
						rule_fields->filter_inner.vid == 0) ||
						(omciutil_vlan_get_rule_filter_tag_num(rule_fields) == 1 && //1tag + 1tag rules
						rule_fields->treatment_tag_to_remove == 0 &&
						omciutil_vlan_get_rule_treatment_tag_num(rule_fields) == 1))
					{
						rule_fields->veip_flag = 1; //mark for veip bridge rules
						rule_fields_new = malloc_safe(sizeof(struct omciutil_vlan_rule_fields_t));
						*rule_fields_new = *rule_fields; //copy

						//add this to collect list
						list_add_tail(&rule_fields_new->rule_node, &tmp_rule_list_us);
					}
				}
			}

			//check downstream list
			list_for_each_entry_safe(rule_fields, rule_fields_n, &lan_port->port_tagging.ds_tagging_rule_list, rule_node)
			{
				if (rule_fields->owner_me != NULL && rule_fields->owner_me->classid == 171)
				{
					//generate result
					if (omciutil_vlan_generate_rule_result(rule_fields, &rule_result_list) < 0)
					{
						dbprintf(LOG_ERR, "could not get rule result\n");
						continue;
					}

					found = 0;
					//check result tag num
					switch (rule_result_list.tag_count)
					{
					case 0: //untag
						found = 1;//add to list
						break;
					case 1: //one tag
						//find for priority tag rules
						vid = 0xffff;
						//get result outer tag
						result_node = list_entry(rule_result_list.tag_list.prev, struct omciutil_vlan_rule_result_node_t, node);
						//check result_node to get vid
						switch (result_node->code)
						{
						case OMCIUTIL_VLAN_TAG_CODE_FILTER_OUTER: //impossible
							vid = rule_fields->filter_outer.vid;
							break;
						case OMCIUTIL_VLAN_TAG_CODE_FILTER_INNER:
							vid = rule_fields->filter_inner.vid;
							break;
						case OMCIUTIL_VLAN_TAG_CODE_TREATMENT_OUTER: //impossible
							if (rule_fields->treatment_outer.vid <= 4095)
							{
								vid = rule_fields->treatment_outer.vid;
							} else if (rule_fields->treatment_outer.vid == 4096) {
								vid = (rule_fields->filter_inner.priority < OMCIUTIL_VLAN_FILTER_PRIORITY_15 ? rule_fields->filter_inner.vid : 0);
							} else if (rule_fields->treatment_outer.vid == 4097) {
								vid = (rule_fields->filter_outer.priority < OMCIUTIL_VLAN_FILTER_PRIORITY_15 ? rule_fields->filter_outer.vid : 0);
							}
							break;
						case OMCIUTIL_VLAN_TAG_CODE_TREATMENT_INNER:
							if (rule_fields->treatment_inner.vid <= 4095)
							{
								vid = rule_fields->treatment_inner.vid;
							} else if (rule_fields->treatment_inner.vid == 4096) {
								vid = (rule_fields->filter_inner.priority < OMCIUTIL_VLAN_FILTER_PRIORITY_15 ? rule_fields->filter_inner.vid : 0);
							} else if (rule_fields->treatment_inner.vid == 4097) {
								vid = (rule_fields->filter_outer.priority < OMCIUTIL_VLAN_FILTER_PRIORITY_15 ? rule_fields->filter_outer.vid : 0);
							}
							break;
						default:
							dbprintf(LOG_ERR, "result code error\n");
						}
	
						//result means priority tag rule, treat as reassign rule
						if (vid == 0) // priority tag
						{
							found = 1;
						}

						//find for 1tag + 1tag rule
						if (!found &&
							omciutil_vlan_get_rule_filter_tag_num(rule_fields) == 2 && 
							rule_fields->treatment_tag_to_remove == 1 &&
							omciutil_vlan_get_rule_treatment_tag_num(rule_fields) == 0) //2tag - 1tag
						{
							found = 1;
						}

						break;
					default: // >= two tags, impossible
						; //do nothing
					}					

					if (found)
					{
						rule_fields->veip_flag = 1; //mark for veip bridge rules
						rule_fields_new = malloc_safe(sizeof(struct omciutil_vlan_rule_fields_t));
						*rule_fields_new = *rule_fields; //copy

						//add this to collect list
						list_add_tail(&rule_fields_new->rule_node, &tmp_rule_list_ds);
					}

					omciutil_vlan_release_rule_result(&rule_result_list);
				}
			}
		}
	}

	//add rules, check list empty
	if (omci_env_g.classf_veip_rules_reassign == 1 && //hw MARK and COPY, otherwise(2), do notthing
		(!list_empty(&tmp_rule_list_us) || !list_empty(&tmp_rule_list_ds)))
	{
		list_for_each_entry(lan_port, &bridge_info->lan_port_list, lp_node)
		{
			if (lan_port->port_type == ENV_BRIDGE_PORT_TYPE_UNI ||
				lan_port->port_type == CLASSF_BRIDGE_PORT_TYPE_UNI_DC)
			{
				if (!list_empty(&tmp_rule_list_us))
				{
					//us, if default rules, clean up then duplicate all to list
					if (lan_port->port_tagging.us_tagging_rule_default) //default rules, clean up
					{
						list_for_each_entry_safe(rule_fields, rule_fields_n, &lan_port->port_tagging.us_tagging_rule_list, rule_node)
						{
							list_del(&rule_fields->rule_node);
							free_safe(rule_fields);
						}
					}
					//duplicate to us list
					list_for_each_entry(rule_fields, &tmp_rule_list_us, rule_node)
					{
						rule_fields_new = malloc_safe(sizeof(struct omciutil_vlan_rule_fields_t));
						*rule_fields_new = *rule_fields;
						if (omciutil_vlan_insert_rule_fields_to_list(rule_fields_new, &lan_port->port_tagging.us_tagging_rule_list) < 0)
						{
							free_safe(rule_fields_new);
							dbprintf(LOG_ERR, "reassign us tagging rule error, lan_port_meid=0x%.4x\n", lan_port->me_bridge_port ? lan_port->me_bridge_port->meid : 0);
						}
					}
				}

				//resort
				count = 0;
				list_for_each_entry(rule_fields, &lan_port->port_tagging.us_tagging_rule_list, rule_node)
				{
					rule_fields->entry_id = count++;
				}

				if (!list_empty(&tmp_rule_list_ds))
				{
					//ds, if default rules, clean up then duplicate all to list
					if (lan_port->port_tagging.ds_tagging_rule_default) //default rules, clean up
					{
						list_for_each_entry_safe(rule_fields, rule_fields_n, &lan_port->port_tagging.ds_tagging_rule_list, rule_node)
						{
							list_del(&rule_fields->rule_node);
							free_safe(rule_fields);
						}
					}
					//duplicate to ds list
					list_for_each_entry(rule_fields, &tmp_rule_list_ds, rule_node)
					{
						rule_fields_new = malloc_safe(sizeof(struct omciutil_vlan_rule_fields_t));
						*rule_fields_new = *rule_fields;
						if (omciutil_vlan_insert_rule_fields_to_list(rule_fields_new, &lan_port->port_tagging.ds_tagging_rule_list) < 0)
						{
							free_safe(rule_fields_new);
							dbprintf(LOG_ERR, "reassign ds tagging rule error, lan_port_meid=0x%.4x\n", lan_port->me_bridge_port ? lan_port->me_bridge_port->meid : 0);
						}
					}
				}

				//resort
				count = 0;
				list_for_each_entry(rule_fields, &lan_port->port_tagging.ds_tagging_rule_list, rule_node)
				{
					rule_fields->entry_id = count++;
				}
			}
		}
	}

	//clean all in list us
	list_for_each_entry_safe(rule_fields, rule_fields_n, &tmp_rule_list_us, rule_node)
	{
		list_del(&rule_fields->rule_node);
		free_safe(rule_fields);
	}

	//clean all in list ds
	list_for_each_entry_safe(rule_fields, rule_fields_n, &tmp_rule_list_ds, rule_node)
	{
		list_del(&rule_fields->rule_node);
		free_safe(rule_fields);
	}

	return 0;
}

static void
classf_collect_bridge_process(struct classf_bridge_info_t *bridge_info,
	struct list_head *global_pvlan_rule_list,
	struct omciutil_vlan_dscp2pbit_info_t *dscp2pbit_info)
{
	struct classf_bridge_info_lan_port_t *lan_port;
	struct list_head *veip_rule_list = NULL;

	if (bridge_info == NULL ||
		global_pvlan_rule_list == NULL ||
		dscp2pbit_info == NULL)
	{
		dbprintf(LOG_ERR, "param error!!\n");
		return;
	}

	//get veip rules list first
	list_for_each_entry(lan_port, &bridge_info->lan_port_list, lp_node)
	{
		if (lan_port->port_type == ENV_BRIDGE_PORT_TYPE_CPU)
		{
			veip_rule_list = &lan_port->port_tagging.us_tagging_rule_list;
			break;
		}
	}

	//iterate for each lan port, find pvlan and join with veip tagging rule
	list_for_each_entry(lan_port, &bridge_info->lan_port_list, lp_node)
	{
		//protocol vlan join
		if (lan_port->port_type != ENV_BRIDGE_PORT_TYPE_IPHOST &&
			!(lan_port->port_type == ENV_BRIDGE_PORT_TYPE_CPU &&
			lan_port->me_bridge_port->is_private == 1)) //protocol vlan rules were unavaliable for autoveip
		{
			classf_protocol_vlan_rule_generate_by_port(lan_port,
				veip_rule_list,
				global_pvlan_rule_list,
				dscp2pbit_info);
		}

		//add default rules for us/ds list if empty, check pvlan list first
		if (omci_env_g.classf_add_default_rules == 2 || //force add
			(list_empty(&lan_port->port_protocol_vlan.us_pvlan_rule_list) &&
			list_empty(&lan_port->port_tagging.us_tagging_rule_list) &&
			omci_env_g.classf_add_default_rules == 1))
		{
			if (list_empty(&lan_port->port_protocol_vlan.us_pvlan_rule_list) &&
				list_empty(&lan_port->port_tagging.us_tagging_rule_list) &&
				omci_env_g.classf_add_default_rules == 1)
			{
				lan_port->port_tagging.us_tagging_rule_default = 1;
			}
			classf_assign_default_tagging_rules(&lan_port->port_tagging.us_tagging_rule_list, lan_port->me_bridge_port, omci_env_g.classf_tag_mode, 0);
		}
		if (omci_env_g.classf_add_default_rules == 2 || //force add
			(list_empty(&lan_port->port_protocol_vlan.ds_pvlan_rule_list) &&
			list_empty(&lan_port->port_tagging.ds_tagging_rule_list) &&
			omci_env_g.classf_add_default_rules == 1))
		{
			if (list_empty(&lan_port->port_protocol_vlan.ds_pvlan_rule_list) &&
				list_empty(&lan_port->port_tagging.ds_tagging_rule_list) &&
				omci_env_g.classf_add_default_rules == 1)
			{
				lan_port->port_tagging.ds_tagging_rule_default = 1;
			}
			classf_assign_default_tagging_rules(&lan_port->port_tagging.ds_tagging_rule_list, lan_port->me_bridge_port, 0, omci_env_g.classf_tag_mode); //ds, use tag_mode to control tag_action.
		}
	}	
	
	return;
}

static int
classf_collect_veip_hwnat_default_rules(struct me_t *bridge_port_me,
	struct list_head *rule_list_us,
	struct list_head *rule_list_ds)
{
	struct omciutil_vlan_rule_fields_t *rule_fields;

	if (bridge_port_me == NULL ||
		rule_list_us == NULL ||
		rule_list_ds == NULL)
	{
		dbprintf(LOG_ERR, "param error!!\n");
		return -1;
	}

	//one tag copy from inner rule for us
	rule_fields = malloc_safe(sizeof(struct omciutil_vlan_rule_fields_t));
	rule_fields->owner_me=NULL;
	rule_fields->bport_me=bridge_port_me;
	rule_fields->sort_priority = OMCIUTIL_VLAN_SORT_PRIORITY_DEFAULT; //default priority

	rule_fields->filter_outer.priority=15;
	rule_fields->filter_outer.vid=4096;
	rule_fields->filter_outer.tpid_de=0;
	rule_fields->filter_inner.priority=8;
	rule_fields->filter_inner.vid=4096;
	rule_fields->filter_inner.tpid_de=0;

	rule_fields->filter_ethertype=OMCIUTIL_VLAN_FILTER_ET_IGNORE;
	rule_fields->treatment_tag_to_remove=1;

	rule_fields->treatment_outer.priority=15;
	rule_fields->treatment_outer.vid=0;
	rule_fields->treatment_outer.tpid_de=0;
	rule_fields->treatment_inner.priority=OMCIUTIL_VLAN_TREATMENT_PRIORITY_8;
	rule_fields->treatment_inner.vid=4096;
	rule_fields->treatment_inner.tpid_de=0;

	if (omciutil_vlan_insert_rule_fields_to_list(rule_fields, rule_list_us) < 0)
	{
		dbprintf(LOG_ERR, "can not insert veip hwnat rule for us, bridge port meid=0x%.4x!!\n", bridge_port_me->meid);
		//release entry_fields
		free_safe(rule_fields);
	}

	//one tag copy from inner rule for ds, all the same with us
	rule_fields = malloc_safe(sizeof(struct omciutil_vlan_rule_fields_t));
	rule_fields->owner_me=NULL;
	rule_fields->bport_me=bridge_port_me;
	rule_fields->sort_priority = OMCIUTIL_VLAN_SORT_PRIORITY_DEFAULT; //default priority

	rule_fields->filter_outer.priority=15;
	rule_fields->filter_outer.vid=4096;
	rule_fields->filter_outer.tpid_de=0;
	rule_fields->filter_inner.priority=8;
	rule_fields->filter_inner.vid=4096;
	rule_fields->filter_inner.tpid_de=0;

	rule_fields->filter_ethertype=OMCIUTIL_VLAN_FILTER_ET_IGNORE;
	rule_fields->treatment_tag_to_remove=1;

	rule_fields->treatment_outer.priority=15;
	rule_fields->treatment_outer.vid=0;
	rule_fields->treatment_outer.tpid_de=0;
	rule_fields->treatment_inner.priority=OMCIUTIL_VLAN_TREATMENT_PRIORITY_8;
	rule_fields->treatment_inner.vid=4096;
	rule_fields->treatment_inner.tpid_de=0;

	if (omciutil_vlan_insert_rule_fields_to_list(rule_fields, rule_list_ds) < 0)
	{
		dbprintf(LOG_ERR, "can not insert veip hwnat rule for ds, bridge port meid=0x%.4x!!\n", bridge_port_me->meid);
		//release entry_fields
		free_safe(rule_fields);
	}

	return 0;
}

static int
classf_collect_default_rules_by_84(struct me_t *bridge_port_me,
	struct list_head *rule_list_us,
	struct list_head *rule_list_ds)
{
	struct omciutil_vlan_rule_fields_t *rule_fields;
	struct meinfo_t *miptr=meinfo_util_miptr(84);
	struct me_t *meptr_84 = NULL;
	unsigned int vid;
	int is_found = 0;


	if (bridge_port_me == NULL ||
		rule_list_us == NULL ||
		rule_list_ds == NULL)
	{
		dbprintf(LOG_ERR, "param error!!\n");
		return -1;
	}

	// 84
	list_for_each_entry(meptr_84, &miptr->me_instance_list, instance_node) {
		unsigned char *bitmap = meinfo_util_me_attr_ptr(meptr_84, 1);
		if((meinfo_util_me_attr_data(meptr_84, 3) != 0) ) {
		   vid = util_bitmap_get_value(bitmap, 24*8, 4, 12);
			is_found = 1;
			dbprintf_bat(LOG_NOTICE, "84=0x%04x\n", meptr_84->meid);
			break;
		}
	}

	if(is_found) {
		//untag add stag
		rule_fields = malloc_safe(sizeof(struct omciutil_vlan_rule_fields_t));
		rule_fields->owner_me=NULL;
		rule_fields->bport_me=bridge_port_me;
		rule_fields->sort_priority = OMCIUTIL_VLAN_SORT_PRIORITY_DEFAULT; //default priority

		rule_fields->filter_outer.priority=15;
		rule_fields->filter_outer.vid=4096;
		rule_fields->filter_outer.tpid_de=0;
		rule_fields->filter_inner.priority=15;
		rule_fields->filter_inner.vid=4096;
		rule_fields->filter_inner.tpid_de=0;

		rule_fields->filter_ethertype=OMCIUTIL_VLAN_FILTER_ET_IGNORE;
		rule_fields->treatment_tag_to_remove=0;

		rule_fields->treatment_outer.priority=15;
		rule_fields->treatment_outer.vid=0;
		rule_fields->treatment_outer.tpid_de=0;
		rule_fields->treatment_inner.priority=OMCIUTIL_VLAN_TREATMENT_PRIORITY_10;
		rule_fields->treatment_inner.vid=vid;
		rule_fields->treatment_inner.tpid_de=0;

		if (omciutil_vlan_insert_rule_fields_to_list(rule_fields, rule_list_us) < 0)
		{
			dbprintf(LOG_ERR, "can not insert default rule by me 84 for us, bridge port meid=0x%.4x!!\n", bridge_port_me->meid);
			//release entry_fields
			free_safe(rule_fields);
		}

		//one tag add stag
		rule_fields = malloc_safe(sizeof(struct omciutil_vlan_rule_fields_t));
		rule_fields->owner_me=NULL;
		rule_fields->bport_me=bridge_port_me;
		rule_fields->sort_priority = OMCIUTIL_VLAN_SORT_PRIORITY_DEFAULT; //default priority

		rule_fields->filter_outer.priority=15;
		rule_fields->filter_outer.vid=4096;
		rule_fields->filter_outer.tpid_de=0;
		rule_fields->filter_inner.priority=8;
		rule_fields->filter_inner.vid=4096;
		rule_fields->filter_inner.tpid_de=0;

		rule_fields->filter_ethertype=OMCIUTIL_VLAN_FILTER_ET_IGNORE;
		rule_fields->treatment_tag_to_remove=0;

		rule_fields->treatment_outer.priority=15;
		rule_fields->treatment_outer.vid=0;
		rule_fields->treatment_outer.tpid_de=0;
		rule_fields->treatment_inner.priority=OMCIUTIL_VLAN_TREATMENT_PRIORITY_10;
		rule_fields->treatment_inner.vid=vid;
		rule_fields->treatment_inner.tpid_de=0;

		if (omciutil_vlan_insert_rule_fields_to_list(rule_fields, rule_list_us) < 0)
		{
			dbprintf(LOG_ERR, "can not insert default rule by me 84 for us, bridge port meid=0x%.4x!!\n", bridge_port_me->meid);
			//release entry_fields
			free_safe(rule_fields);
		}

		//one tag remove one tag for ds
		rule_fields = malloc_safe(sizeof(struct omciutil_vlan_rule_fields_t));
		rule_fields->owner_me=NULL;
		rule_fields->bport_me=bridge_port_me;
		rule_fields->sort_priority = OMCIUTIL_VLAN_SORT_PRIORITY_DEFAULT; //default priority

		rule_fields->filter_outer.priority=15;
		rule_fields->filter_outer.vid=0;
		rule_fields->filter_outer.tpid_de=0;
		rule_fields->filter_inner.priority=8;
		rule_fields->filter_inner.vid=vid;
		rule_fields->filter_inner.tpid_de=0;

		rule_fields->filter_ethertype=OMCIUTIL_VLAN_FILTER_ET_IGNORE;
		rule_fields->treatment_tag_to_remove=1;

		rule_fields->treatment_outer.priority=OMCIUTIL_VLAN_TREATMENT_PRIORITY_15;
		rule_fields->treatment_outer.vid=0;
		rule_fields->treatment_outer.tpid_de=0;
		rule_fields->treatment_inner.priority=OMCIUTIL_VLAN_TREATMENT_PRIORITY_15;
		rule_fields->treatment_inner.vid=0;
		rule_fields->treatment_inner.tpid_de=0;

		if (omciutil_vlan_insert_rule_fields_to_list(rule_fields, rule_list_ds) < 0)
		{
			dbprintf(LOG_ERR, "can not insert default rule by me 84 for ds, bridge port meid=0x%.4x!!\n", bridge_port_me->meid);
			//release entry_fields
			free_safe(rule_fields);
		}

		// double tag remove one tag for ds
		rule_fields = malloc_safe(sizeof(struct omciutil_vlan_rule_fields_t));
		rule_fields->owner_me=NULL;
		rule_fields->bport_me=bridge_port_me;
		rule_fields->sort_priority = OMCIUTIL_VLAN_SORT_PRIORITY_DEFAULT; //default priority
		
		rule_fields->filter_outer.priority=8;
		rule_fields->filter_outer.vid=vid;
		rule_fields->filter_outer.tpid_de=0;
		rule_fields->filter_inner.priority=8;
		rule_fields->filter_inner.vid=4096;
		rule_fields->filter_inner.tpid_de=0;
		
		rule_fields->filter_ethertype=OMCIUTIL_VLAN_FILTER_ET_IGNORE;
		rule_fields->treatment_tag_to_remove=1;
		
		rule_fields->treatment_outer.priority=OMCIUTIL_VLAN_TREATMENT_PRIORITY_15;
		rule_fields->treatment_outer.vid=0;
		rule_fields->treatment_outer.tpid_de=0;
		rule_fields->treatment_inner.priority=OMCIUTIL_VLAN_TREATMENT_PRIORITY_15;
		rule_fields->treatment_inner.vid=0;
		rule_fields->treatment_inner.tpid_de=0;
		
		if (omciutil_vlan_insert_rule_fields_to_list(rule_fields, rule_list_ds) < 0)
		{
			dbprintf(LOG_ERR, "can not insert default rule by me 84 for ds, bridge port meid=0x%.4x!!\n", bridge_port_me->meid);
			//release entry_fields
			free_safe(rule_fields);
		}
	}

	return 0;
}


static void
_classf_rule_release(struct classf_info_t *classf_info)
{
	struct classf_bridge_info_t *bridge_info, *bridge_info_n;
	struct classf_bridge_info_lan_port_t *lan_port, *lan_port_n;
	struct classf_bridge_info_wan_port_t *wan_port, *wan_port_n;
	struct omciutil_vlan_rule_fields_t *rule_fields, *rule_fields_n;
	struct classf_rule_candi_t *rule_candi, *rule_candi_n;
	struct classf_rule_t *rule, *rule_n;
	struct classf_pvlan_rule_t *pvlan_rule, *pvlan_rule_n;

	if (classf_info == NULL)
	{
		dbprintf(LOG_ERR, "param error!!\n");
		return;
	}

	list_for_each_entry_safe(bridge_info, bridge_info_n, &classf_info->bridge_info_list, b_node)
	{
		//release rule candi us
		list_for_each_entry_safe(rule_candi, rule_candi_n, &bridge_info->rule_candi_list_us, rc_node)
		{
			list_del(&rule_candi->rc_node);
			free_safe(rule_candi);
		}
		//release rule candi ds
		list_for_each_entry_safe(rule_candi, rule_candi_n, &bridge_info->rule_candi_list_ds, rc_node)
		{
			list_del(&rule_candi->rc_node);
			free_safe(rule_candi);
		}
		//release rule us
		list_for_each_entry_safe(rule, rule_n, &bridge_info->rule_list_us, r_node)
		{
			list_del(&rule->r_node);
			classf_release_rule(rule);
			free_safe(rule);
		}
		//release rule ds
		list_for_each_entry_safe(rule, rule_n, &bridge_info->rule_list_ds, r_node)
		{
			list_del(&rule->r_node);
			classf_release_rule(rule);
			free_safe(rule);
		}
		//release lan bridge port
		list_for_each_entry_safe(lan_port, lan_port_n, &bridge_info->lan_port_list, lp_node)
		{
			//us
			list_for_each_entry_safe(rule_fields, rule_fields_n, &lan_port->port_tagging.us_tagging_rule_list, rule_node)
			{
				list_del(&rule_fields->rule_node);
				free_safe(rule_fields);
			}
			//ds
			list_for_each_entry_safe(rule_fields, rule_fields_n, &lan_port->port_tagging.ds_tagging_rule_list, rule_node)
			{
				list_del(&rule_fields->rule_node);
				free_safe(rule_fields);
			}
			//pvlan us
			list_for_each_entry_safe(pvlan_rule, pvlan_rule_n, &lan_port->port_protocol_vlan.us_pvlan_rule_list, pr_node)
			{
				list_del(&pvlan_rule->pr_node);
				free_safe(pvlan_rule);
			}
			//pvlan ds
			list_for_each_entry_safe(pvlan_rule, pvlan_rule_n, &lan_port->port_protocol_vlan.ds_pvlan_rule_list, pr_node)
			{
				list_del(&pvlan_rule->pr_node);
				free_safe(pvlan_rule);
			}

			list_del(&lan_port->lp_node);
			free_safe(lan_port);
		}
		//release wan bridge port
		list_for_each_entry_safe(wan_port, wan_port_n, &bridge_info->wan_port_list, wp_node)
		{
			list_del(&wan_port->wp_node);
			free_safe(wan_port);
		}

		list_del(&bridge_info->b_node);
		free_safe(bridge_info);
	}

	//check system protocol vlan list
	list_for_each_entry_safe(pvlan_rule, pvlan_rule_n, &classf_info->system_info.protocol_vlan_rule_list, pr_node)
	{
		list_del(&pvlan_rule->pr_node);
		free_safe(pvlan_rule);
	}

	// list in tlan_table
	list_for_each_entry_safe(rule_fields, rule_fields_n, &classf_info->system_info.tlan_info.uni_tlan_list, rule_node)
	{
		list_del(&rule_fields->rule_node);
		free_safe(rule_fields);
	}
	list_for_each_entry_safe(rule_fields, rule_fields_n, &classf_info->system_info.tlan_info.uni_nontlan_list, rule_node)
	{
		list_del(&rule_fields->rule_node);
		free_safe(rule_fields);
	}
	list_for_each_entry_safe(rule_fields, rule_fields_n, &classf_info->system_info.tlan_info.ani_tlan_list, rule_node)
	{
		list_del(&rule_fields->rule_node);
		free_safe(rule_fields);
	}
	list_for_each_entry_safe(rule_fields, rule_fields_n, &classf_info->system_info.tlan_info.ani_nontlan_list, rule_node)
	{
		list_del(&rule_fields->rule_node);
		free_safe(rule_fields);
	}

	return;
}

static int
classf_rule_collect_release_candi_rules_first(struct classf_info_t *classf_info)
{
	struct classf_bridge_info_t *bridge_info, *bridge_info_n;
	struct classf_rule_candi_t *rule_candi, *rule_candi_n;
	
	if (classf_info == NULL)
	{
		dbprintf(LOG_ERR, "param error!!\n");
		return -1;
	}

	list_for_each_entry_safe(bridge_info, bridge_info_n, &classf_info->bridge_info_list, b_node)
	{
		//release rule candi us
		list_for_each_entry_safe(rule_candi, rule_candi_n, &bridge_info->rule_candi_list_us, rc_node)
		{
			list_del(&rule_candi->rc_node);
			free_safe(rule_candi);
		}
		//release rule candi ds
		list_for_each_entry_safe(rule_candi, rule_candi_n, &bridge_info->rule_candi_list_ds, rc_node)
		{
			list_del(&rule_candi->rc_node);
			free_safe(rule_candi);
		}
	}
	
	return 0;
}

static void
classf_rule_collect_tlan_pattern(struct classf_bridge_info_t *bridge_info, struct omciutil_vlan_tlan_info_t *tlan_info)
{
	struct classf_rule_t *classf_rule_pos;
	struct omciutil_vlan_rule_fields_t *rule_fields_new;

	if (bridge_info == NULL ||
		tlan_info == NULL)
	{
		dbprintf(LOG_ERR, "param error!!\n");
		return;
	}
	
	//iterate upstream
	list_for_each_entry(classf_rule_pos, &bridge_info->rule_list_us, r_node)
	{
		//only check rules from class 171 and 249
		if (classf_rule_pos->rule_fields.owner_me != NULL &&
			(classf_rule_pos->rule_fields.owner_me->classid == 171 ||
			classf_rule_pos->rule_fields.owner_me->classid == 249 ||
			classf_rule_pos->rule_fields.owner_me->classid == 65313))
		{
			if (classf_rule_pos->tlan_info.flag == 1) //tlan rule
			{
				//allocate a new one rule_fields and insert to uni_tlan_list
				rule_fields_new = malloc_safe(sizeof(struct omciutil_vlan_rule_fields_t));
				*rule_fields_new = classf_rule_pos->rule_fields;
				if (omciutil_vlan_insert_rule_fields_to_list(rule_fields_new, &tlan_info->uni_tlan_list) < 0)
				{
					dbprintf(LOG_ERR, "inset to uni_tlan_list error!!\n");

					//release
					free_safe(rule_fields_new);
				}
			} else {
				//allocate a new one rule_fields and insert to uni_nontlan_list
				rule_fields_new = malloc_safe(sizeof(struct omciutil_vlan_rule_fields_t));
				*rule_fields_new = classf_rule_pos->rule_fields;
				if (omciutil_vlan_insert_rule_fields_to_list(rule_fields_new, &tlan_info->uni_nontlan_list) < 0)
				{
					dbprintf(LOG_ERR, "inset to uni_nontlan_list error!!\n");

					//release
					free_safe(rule_fields_new);
				}
			}
		}
	}

	//iterate downstream
	list_for_each_entry(classf_rule_pos, &bridge_info->rule_list_ds, r_node)
	{
		//only check rules from class 171 and 249
		if (classf_rule_pos->rule_fields.owner_me != NULL &&
			(classf_rule_pos->rule_fields.owner_me->classid == 171 ||
			classf_rule_pos->rule_fields.owner_me->classid == 249 ||
			classf_rule_pos->rule_fields.owner_me->classid == 65313))
		{
			if (classf_rule_pos->tlan_info.flag == 1) //tlan rule
			{
				//allocate a new one rule_fields and insert to ani_tlan_list
				rule_fields_new = malloc_safe(sizeof(struct omciutil_vlan_rule_fields_t));
				*rule_fields_new = classf_rule_pos->rule_fields;
				if (omciutil_vlan_insert_rule_fields_to_list(rule_fields_new, &tlan_info->ani_tlan_list) < 0)
				{
					dbprintf(LOG_ERR, "inset to ani_tlan_list error!!\n");

					//release
					free_safe(rule_fields_new);
				}
			} else {
				//allocate a new one rule_fields and insert to ani_nontlan_list
				rule_fields_new = malloc_safe(sizeof(struct omciutil_vlan_rule_fields_t));
				*rule_fields_new = classf_rule_pos->rule_fields;
				if (omciutil_vlan_insert_rule_fields_to_list(rule_fields_new, &tlan_info->ani_nontlan_list) < 0)
				{
					dbprintf(LOG_ERR, "inset to ani_nontlan_list error!!\n");

					//release
					free_safe(rule_fields_new);
				}
			}
		}
	}
}

static void
classf_collect_brwan_info(struct classf_brwan_info_t *brwan_info)
{
#ifdef OMCI_METAFILE_ENABLE
	struct metacfg_t kv_config;
	char key[128], *value_ptr;
	int i;
	
	if (brwan_info == NULL)
	{
		dbprintf(LOG_ERR, "param error!!\n");
		return;
	}

	memset(&kv_config, 0x0, sizeof(struct metacfg_t));
	metacfg_adapter_config_init(&kv_config);
	metacfg_adapter_config_file_load_part(&kv_config, "/etc/wwwctrl/metafile.dat", "dev_wan0_interface", "nat_dmz_enable");

	// brwan info
	for (i=0; i<BRWAN_INDEX_TOTAL; i++)
	{
		snprintf(key, 128, "brwan%d_enable", i);
		
		if (strlen(value_ptr = metacfg_adapter_config_get_value(&kv_config, key, 1)) > 0 &&
			(brwan_info->enable = util_atoi(value_ptr)) > 0) //enable
		{
			snprintf(key, 128, "brwan%d_vid", i);
			value_ptr = metacfg_adapter_config_get_value(&kv_config, key, 1);
			brwan_info->vid = util_atoi(value_ptr);

			snprintf(key, 128, "brwan%d_portmask", i);
			value_ptr = metacfg_adapter_config_get_value(&kv_config, key, 1);
			brwan_info->portmask = util_atoi(value_ptr);

			snprintf(key, 128, "brwan%d_wifimask", i);
			value_ptr = metacfg_adapter_config_get_value(&kv_config, key, 1);
			brwan_info->wifimask = util_atoi(value_ptr);

			//assume one and only one brwan enabled
			break;
		}
	}

	metacfg_adapter_config_release(&kv_config);
#endif
	return;
}

static void
classf_rule_collect_l2_rules_the_same_criterion(struct classf_info_t *classf_info)
{
	struct classf_bridge_info_t *bridge_info_pos, *bridge_info_pos_1;
	struct classf_rule_t *rule_pos;
	struct classf_bridge_info_lan_port_t *lan_port_pos;
	struct omciutil_vlan_rule_fields_t *rule_fields_pos;
	
	if (classf_info == NULL)
	{
		dbprintf(LOG_ERR, "param error!!\n");
		return;
	}
	
	list_for_each_entry(bridge_info_pos, &classf_info->bridge_info_list, b_node)
	{
		list_for_each_entry(rule_pos, &bridge_info_pos->rule_list_us, r_node)
		{
			if (rule_pos->lan_port != NULL &&
				rule_pos->lan_port->me_bridge_port != NULL &&
				rule_pos->rule_fields.l2_filter_enable)
			{
				//reset lan mask first
				rule_pos->rule_fields.l2_filter.lan_mask = 0x0;
				//mark self port
				rule_pos->rule_fields.l2_filter.lan_mask |= (1 << rule_pos->lan_port->logical_port_id);
				
				//find other lan ports l2 rules
				list_for_each_entry(bridge_info_pos_1, &classf_info->bridge_info_list, b_node)
				{
					list_for_each_entry(lan_port_pos, &bridge_info_pos_1->lan_port_list, lp_node)
					{
						if (lan_port_pos->me_bridge_port != NULL &&
							lan_port_pos->me_bridge_port->meid == rule_pos->lan_port->me_bridge_port->meid) //skip self
						{
							continue; //next one
						}

						//iterate us rule list
						list_for_each_entry(rule_fields_pos, &lan_port_pos->port_tagging.us_tagging_rule_list, rule_node)
						{
							if (rule_fields_pos->l2_filter_enable &&
								!memcmp(rule_fields_pos->l2_filter.mac, rule_pos->rule_fields.l2_filter.mac, sizeof(rule_fields_pos->l2_filter.mac)) &&
								!memcmp(rule_fields_pos->l2_filter.mac_mask, rule_pos->rule_fields.l2_filter.mac_mask, sizeof(rule_fields_pos->l2_filter.mac_mask)))
							{
								//the smae l2 smac, set lan mask
								rule_pos->rule_fields.l2_filter.lan_mask |= (1 << lan_port_pos->logical_port_id);
								//found, continue to next uni port
								break;
							}
						}
					}
				}
			}
		}
	}

	return;
}

static void
classf_rule_analyze_tlan_aggregation(struct classf_info_t *classf_info)
{
	struct classf_bridge_info_t *bridge_info_pos, *bridge_info_pos_1;
	struct classf_rule_t *rule_pos, *rule_pos_1;
	struct classf_bridge_info_wan_port_t *wan_port_pos;
	struct classf_bridge_info_lan_port_t *lan_port_pos;
	unsigned int wan_port_num, lan_port_num, cpu_port_num;
	int i;
	unsigned char found_pbit_mask, found_pbit_mask_1;
	unsigned short filter_vid;
	unsigned char priority_found, priority_found_1;
	unsigned char model;
	if (classf_info == NULL)
	{
		dbprintf(LOG_ERR, "param error!!\n");
		return;
	}

	if (classf_info->system_info.tlan_info.tlan_table.total == 0) //have no tlan
	{
		return;
	}

	//1. fully check the same tlan rules in different bridge,
	//2. could use for all tlan upstream rules to save acl usage
	list_for_each_entry(bridge_info_pos, &classf_info->bridge_info_list, b_node)
	{
		list_for_each_entry(rule_pos, &bridge_info_pos->rule_list_us, r_node)
		{
			if (rule_pos->lan_port != NULL &&
				rule_pos->lan_port->me_bridge_port != NULL &&
				rule_pos->tlan_info.flag) //tlan enbled rule
			{
				//reset lan mask first
				rule_pos->tlan_info.lan_mask = 0x0;
				//mark self port
				rule_pos->tlan_info.lan_mask |= (1 << rule_pos->lan_port->logical_port_id);
				
				//find other lan ports tlan rules
				list_for_each_entry(bridge_info_pos_1, &classf_info->bridge_info_list, b_node)
				{
					list_for_each_entry(rule_pos_1, &bridge_info_pos_1->rule_list_us, r_node)
					{
						if ((rule_pos_1->lan_port != NULL &&
							rule_pos_1->lan_port->me_bridge_port != NULL &&
							rule_pos_1->lan_port->me_bridge_port->meid == rule_pos->lan_port->me_bridge_port->meid) ||
							rule_pos_1->tlan_info.flag == 0) //skip self port rules, and not tlan rules
						{
							continue;
						}

						//compare two rules detail
						if (!memcmp(&rule_pos->rule_fields.filter_outer, &rule_pos_1->rule_fields.filter_outer, sizeof(rule_pos->rule_fields.filter_outer)) &&
							!memcmp(&rule_pos->rule_fields.filter_inner, &rule_pos_1->rule_fields.filter_inner, sizeof(rule_pos->rule_fields.filter_inner)) &&
							rule_pos->rule_fields.filter_ethertype == rule_pos_1->rule_fields.filter_ethertype &&
							rule_pos->rule_fields.treatment_tag_to_remove == rule_pos_1->rule_fields.treatment_tag_to_remove &&
							!memcmp(&rule_pos->rule_fields.treatment_outer, &rule_pos_1->rule_fields.treatment_outer, sizeof(rule_pos->rule_fields.treatment_outer)) &&
							!memcmp(&rule_pos->rule_fields.treatment_inner, &rule_pos_1->rule_fields.treatment_inner, sizeof(rule_pos->rule_fields.treatment_inner)) &&
							rule_pos->rule_fields.input_tpid == rule_pos_1->rule_fields.input_tpid &&
							rule_pos->rule_fields.output_tpid == rule_pos_1->rule_fields.output_tpid)
						{
							//the smae rule field for tagging, set lan mask
							rule_pos->tlan_info.lan_mask |= (1 << rule_pos_1->lan_port->logical_port_id);
						}
					}
				}
			}
		}
	}

	//1. one wan port, one lan port,
	//2. 4 gems for 802.1p, [012]->gem0, [3]->gem1, [4]->gem2, [567]->gem3
	//3. 1 vlan filter tci
	//4. check apply model
	//5. assign aggregation flag for each rules, do special handle while adding these rules.
	list_for_each_entry(bridge_info_pos, &classf_info->bridge_info_list, b_node)
	{
		//check wan/lan port number
		wan_port_num = lan_port_num = cpu_port_num = 0;
		list_for_each_entry(wan_port_pos, &bridge_info_pos->wan_port_list, wp_node)
		{
			wan_port_num++;
		}
		list_for_each_entry(lan_port_pos, &bridge_info_pos->lan_port_list, lp_node)
		{
			if (lan_port_pos->port_type == ENV_BRIDGE_PORT_TYPE_UNI)
			{
				lan_port_num++;
			} else if (lan_port_pos->port_type == ENV_BRIDGE_PORT_TYPE_CPU &&
				!list_empty(&lan_port_pos->port_tagging.us_tagging_rule_list)) { //veip have rules
				cpu_port_num++;
			}
		}
		if (wan_port_num != 1 || lan_port_num != 1 || cpu_port_num != 0) //for half bridge, must have one wan, one lan, and have no cpu port
		{
			continue; //next bridge
		}

		//check 8021p to gem port mapping
		wan_port_pos = list_first_entry(&bridge_info_pos->wan_port_list, struct classf_bridge_info_wan_port_t, wp_node);
		if (wan_port_pos->port_gem.gem_num == 4) //4 gem ports
		{
			found_pbit_mask = 0;;
			for (i = 0; i < wan_port_pos->port_gem.gem_index_total; i++)
			{
				if (wan_port_pos->port_gem.gem_tagged[i] == 1) //tagged
				{
					if (wan_port_pos->port_gem.pbit_mask[i] == 0x07) //pbit[012]
					{
						found_pbit_mask |= 0x07;
					}
					if (wan_port_pos->port_gem.pbit_mask[i] == 0x08) //pbit[3]
					{
						found_pbit_mask |= 0x08;
					}
					if (wan_port_pos->port_gem.pbit_mask[i] == 0x10) //pbit[4]
					{
						found_pbit_mask |= 0x10;
					}
					if (wan_port_pos->port_gem.pbit_mask[i] == 0xe0) //pbit[567]
					{
						found_pbit_mask |= 0xe0;
					}
				}
			}

			if (found_pbit_mask != 0xff)
			{
				continue; //next bridge
			}
		}else {
			continue; //next birdge
		}

		//get filter vid
		if (wan_port_pos->port_filter.vlan_tci_num != 1)
		{
			continue; //next bridge
		}
		filter_vid = wan_port_pos->port_filter.vlan_tci[0] & 0xfff;

		//get first one classf rule
		if (list_empty(&bridge_info_pos->rule_list_us) ||
			(rule_pos = list_first_entry(&bridge_info_pos->rule_list_us, struct classf_rule_t, r_node)) == NULL ||
			rule_pos->tlan_info.flag == 0) //non-tlan rule
		{
			continue; //next bridge
		}

		//accrodin the fisrt rule, check apply model
		if (omciutil_vlan_get_rule_filter_tag_num(&rule_pos->rule_fields) == 1 &&
			rule_pos->rule_fields.treatment_tag_to_remove == 0 &&
			omciutil_vlan_get_rule_treatment_tag_num(&rule_pos->rule_fields) == 1)
		{
			//analyze "add 2nd tag" model
			//a. seperate pbit0-7 to individual rule
			//b. add the tlan vid tag
			//c. pbit copy from inner

			//advance to determine two different models
			//1. 1 tag + 1 tag model
			//2. any tags + 1 tag model
			found_pbit_mask = 0;
			model = 0; //1 tag + 1 tag model
			list_for_each_entry(rule_pos, &bridge_info_pos->rule_list_us, r_node)
			{
				if (rule_pos->tlan_info.flag == 1 &&
					(omciutil_vlan_get_rule_filter_tag_num(&rule_pos->rule_fields) == 1 &&
					rule_pos->rule_fields.treatment_tag_to_remove == 0 &&
					omciutil_vlan_get_rule_treatment_tag_num(&rule_pos->rule_fields) == 1) && //1tag + 1tag
					(rule_pos->rule_fields.filter_inner.priority < OMCIUTIL_VLAN_FILTER_PRIORITY_8 && //pbit0-7
					rule_pos->rule_fields.treatment_inner.vid == filter_vid && //tlan vid
					rule_pos->rule_fields.treatment_inner.priority == OMCIUTIL_VLAN_TREATMENT_PRIORITY_8)) //copy from inner
				{
					found_pbit_mask |= (1 << rule_pos->rule_fields.filter_inner.priority);
				} else if (rule_pos->tlan_info.flag == 1 && //untag + 1 tag
					(omciutil_vlan_get_rule_filter_tag_num(&rule_pos->rule_fields) == 0 &&
					rule_pos->rule_fields.treatment_tag_to_remove == 0 &&
					omciutil_vlan_get_rule_treatment_tag_num(&rule_pos->rule_fields) == 1) && //untag + 1tag
					(rule_pos->rule_fields.tlan_workaround_flag == 2 && //force tpid to 0x9100 on condition untag adds 1 tag
					rule_pos->rule_fields.treatment_inner.vid == filter_vid)) {  //tlan vid
					model = 1; //any tags + 1 tag model
				} else {
					found_pbit_mask = 0;
					break;
				}
			}
			if (found_pbit_mask != 0xff) //all pbit
			{
				continue; //next bridge
			} else {
				if (model == 0)
				{
					//mark the aggregation flag for all us/ds rules
					list_for_each_entry(rule_pos, &bridge_info_pos->rule_list_us, r_node)
					{
						switch(rule_pos->rule_fields.filter_inner.priority)
						{
						case 0:
						case 6:
							rule_pos->tlan_info.aggregation_flag = CLASSF_RULE_AGGREGATION_1PBIT_MASK;
							break;
						case 1:
						case 7:
							rule_pos->tlan_info.aggregation_flag = CLASSF_RULE_AGGREGATION_SKIP;
							break;
						default:
							rule_pos->tlan_info.aggregation_flag = CLASSF_RULE_AGGREGATION_NORMAL;
						}
					}
					list_for_each_entry(rule_pos, &bridge_info_pos->rule_list_ds, r_node)
					{
						if (rule_pos->rule_fields.filter_inner.priority == OMCIUTIL_VLAN_FILTER_PRIORITY_8 &&
							rule_pos->rule_fields.filter_outer.priority == OMCIUTIL_VLAN_FILTER_PRIORITY_8)
						{
							rule_pos->tlan_info.aggregation_flag = CLASSF_RULE_AGGREGATION_NORMAL;
						} else {
							rule_pos->tlan_info.aggregation_flag = CLASSF_RULE_AGGREGATION_SKIP;
						}
					}
				}else { //model 1, any tags + 1 tag
					//mark the aggregation flag for all us/ds rules
					list_for_each_entry(rule_pos, &bridge_info_pos->rule_list_us, r_node)
					{
						// 1 tag rules
						if (omciutil_vlan_get_rule_filter_tag_num(&rule_pos->rule_fields) == 1)
						{
							switch(rule_pos->rule_fields.filter_inner.priority)
							{
							case 0:
							case 6:
								rule_pos->tlan_info.aggregation_flag = CLASSF_RULE_AGGREGATION_1PBIT_MASK;
								break;
							case 1:
							case 7:
								rule_pos->tlan_info.aggregation_flag = CLASSF_RULE_AGGREGATION_SKIP;
								break;
							default:
								rule_pos->tlan_info.aggregation_flag = CLASSF_RULE_AGGREGATION_NORMAL;
							}
						} else { //others, set normal
							rule_pos->tlan_info.aggregation_flag = CLASSF_RULE_AGGREGATION_NORMAL;
						}
					}
					list_for_each_entry(rule_pos, &bridge_info_pos->rule_list_ds, r_node)
					{
						//only the 1 tag - 1 tag and pbit wildcard rule sets to normal, otherwise set to skip
						//because tpid is 0x9100, have no 2nd stag definitely.
						if (omciutil_vlan_get_rule_filter_tag_num(&rule_pos->rule_fields) == 1 &&
							rule_pos->rule_fields.treatment_tag_to_remove == 1 &&
							omciutil_vlan_get_rule_treatment_tag_num(&rule_pos->rule_fields) == 0 &&
							rule_pos->rule_fields.filter_inner.priority == OMCIUTIL_VLAN_FILTER_PRIORITY_8)
						{
							rule_pos->tlan_info.aggregation_flag = CLASSF_RULE_AGGREGATION_NORMAL;
						} else {
							rule_pos->tlan_info.aggregation_flag = CLASSF_RULE_AGGREGATION_SKIP;
						}
					}
				}
			}
		} else if ((omciutil_vlan_get_rule_filter_tag_num(&rule_pos->rule_fields) == 1 ||
			omciutil_vlan_get_rule_filter_tag_num(&rule_pos->rule_fields) == 2) &&
			rule_pos->rule_fields.treatment_tag_to_remove == 1 &&
			omciutil_vlan_get_rule_treatment_tag_num(&rule_pos->rule_fields) == 1) {
			//analyze "add and change tags" model,
			//a. seperate pbit0-7 to individual rule
			//b. have 1tag and 2tag rules
			//c. chage the tlan vid tag
			//d. only one non-zero pbit rule assign to non-zero pbit the same, others assign to 0.
			found_pbit_mask = 0;
			found_pbit_mask_1 = 0;
			priority_found = 0;
			priority_found_1 = 0;
			list_for_each_entry(rule_pos, &bridge_info_pos->rule_list_us, r_node)
			{
				if (rule_pos->tlan_info.flag == 1 &&
					(omciutil_vlan_get_rule_filter_tag_num(&rule_pos->rule_fields) == 1 &&
					rule_pos->rule_fields.treatment_tag_to_remove == 1 &&
					omciutil_vlan_get_rule_treatment_tag_num(&rule_pos->rule_fields) == 1) && //1tag -1tag + 1tag
					(rule_pos->rule_fields.filter_inner.priority < OMCIUTIL_VLAN_FILTER_PRIORITY_8 && //pbit0-7
					rule_pos->rule_fields.treatment_inner.vid == filter_vid && //tlan vid
					rule_pos->rule_fields.treatment_inner.priority < OMCIUTIL_VLAN_TREATMENT_PRIORITY_8)) // assign
				{
					if (rule_pos->rule_fields.treatment_inner.priority != 0) //non-zrop
					{
						if (priority_found != 0)
						{
							break; //only one
						} else {
							priority_found = rule_pos->rule_fields.treatment_inner.priority;						
						}
					}
					found_pbit_mask |= (1 << rule_pos->rule_fields.filter_inner.priority);
				} else if (rule_pos->tlan_info.flag == 1 &&
					(omciutil_vlan_get_rule_filter_tag_num(&rule_pos->rule_fields) == 2 &&
					rule_pos->rule_fields.treatment_tag_to_remove == 1 &&
					omciutil_vlan_get_rule_treatment_tag_num(&rule_pos->rule_fields) == 1) && //2tag -1tag + 1tag
					(rule_pos->rule_fields.filter_outer.priority < OMCIUTIL_VLAN_FILTER_PRIORITY_8 && //pbit0-7
					rule_pos->rule_fields.treatment_inner.vid == filter_vid && //tlan vid
					rule_pos->rule_fields.treatment_inner.priority < OMCIUTIL_VLAN_TREATMENT_PRIORITY_8)) { // assign
					if (rule_pos->rule_fields.treatment_inner.priority != 0) //non-zrop
					{
						if (priority_found_1 != 0)
						{
							break; //only one
						} else {
							priority_found_1 = rule_pos->rule_fields.treatment_inner.priority;						
						}
					}					
					found_pbit_mask_1 |= (1 << rule_pos->rule_fields.filter_outer.priority);
				} else {
					found_pbit_mask = found_pbit_mask_1 = 0;
					break;
				}
			}

			if (found_pbit_mask != 0xff ||
				found_pbit_mask_1 != 0xff) //all pbit
			{
				continue; //next bridge
			} else {
				//mark the aggregation flag for all us/ds rules
				list_for_each_entry(rule_pos, &bridge_info_pos->rule_list_us, r_node)
				{
					if (rule_pos->rule_fields.treatment_inner.priority != 0)
					{
						rule_pos->tlan_info.aggregation_flag = CLASSF_RULE_AGGREGATION_PBIT_WILDCARD;
						for (i = 0; i < rule_pos->wan_port->port_gem.gem_index_total; i++)
						{
							if ((rule_pos->wan_port->port_gem.pbit_mask[i] & 0x1) != 0) //pbit 0
							{
								rule_pos->tlan_info.aggregation_data = (void *) i; //gem_index
								break;
							}
						}
					} else {
						rule_pos->tlan_info.aggregation_flag = CLASSF_RULE_AGGREGATION_SKIP;
					}
				}
				list_for_each_entry(rule_pos, &bridge_info_pos->rule_list_ds, r_node)
				{
					if (omciutil_vlan_get_rule_filter_tag_num(&rule_pos->rule_fields) == 1)
					{
						if (rule_pos->rule_fields.filter_inner.priority == OMCIUTIL_VLAN_FILTER_PRIORITY_8)
						{
							rule_pos->tlan_info.aggregation_flag = CLASSF_RULE_AGGREGATION_NORMAL;
						} else {
							rule_pos->tlan_info.aggregation_flag = CLASSF_RULE_AGGREGATION_SKIP;
						}
					} else if (omciutil_vlan_get_rule_filter_tag_num(&rule_pos->rule_fields) == 2) {
						if (rule_pos->rule_fields.filter_outer.priority == OMCIUTIL_VLAN_FILTER_PRIORITY_8)
						{
							rule_pos->tlan_info.aggregation_flag = CLASSF_RULE_AGGREGATION_NORMAL;
						} else {
							rule_pos->tlan_info.aggregation_flag = CLASSF_RULE_AGGREGATION_SKIP;
						}
					}
				}
			}
		}
	}

	return;
}

int
classf_rule_collect(struct classf_info_t *classf_info)
{
	unsigned int uni_port_mask;
	unsigned int logical_port_mask;
	struct meinfo_t *miptr=meinfo_util_miptr(45);
	struct meinfo_t *miptr_bridge_port=meinfo_util_miptr(47);
	struct me_t *bridge_me;
	struct me_t *bridge_port_me;
	struct classf_bridge_info_t *bridge_info;
	struct switch_port_info_t port_info;
	struct classf_bridge_info_lan_port_t *lan_port;
	struct classf_bridge_info_wan_port_t *wan_port;

	if (classf_info == NULL)
	{
		dbprintf(LOG_ERR, "param error!!\n");
		return -1;
	}

	fwk_mutex_lock(&classf_info->mutex);

	//release first
	_classf_rule_release(classf_info);

	rule_count_us = rule_count_ds = 0;
	rule_candi_count_us = rule_candi_count_ds = 0;

	//get all logical port mask without cpuext(wifi on cpu ext)
	logical_port_mask = switch_get_all_logical_portmask_without_cpuext();
	
	memset(&classf_info->system_info.dscp2pbit_info, 0x00, sizeof(struct omciutil_vlan_dscp2pbit_info_t));
	if (omci_env_g.classf_dscp2pbit_mode != 0) //dscp-2-pbit by global table
	{
		omciutil_vlan_collect_dscp2pbit_info_global(&classf_info->system_info.dscp2pbit_info);
	}

#ifdef OMCI_METAFILE_ENABLE
	//collect all protocol vlan rules first
	classf_protocol_vlan_collect_rules_from_metafile(&classf_info->system_info.protocol_vlan_rule_list);
#endif
	//collect tlan vid
	if(strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX)==0) //enable
	{
		omciutil_vlan_collect_tlan_vid(&classf_info->system_info.tlan_info.tlan_table);
	}

	//collect brwan info for sw bridge path
	classf_collect_brwan_info(&classf_info->system_info.brwan_info);

	//bridge
	list_for_each_entry(bridge_me, &miptr->me_instance_list, instance_node)
	{
		//create bridge info node
		bridge_info = malloc_safe(sizeof(struct classf_bridge_info_t));
		INIT_LIST_HEAD(&bridge_info->wan_port_list);
		INIT_LIST_HEAD(&bridge_info->lan_port_list);
		INIT_LIST_HEAD(&bridge_info->rule_candi_list_us);
		INIT_LIST_HEAD(&bridge_info->rule_candi_list_ds);
		INIT_LIST_HEAD(&bridge_info->rule_list_us);
		INIT_LIST_HEAD(&bridge_info->rule_list_ds);

		bridge_info->me_bridge = bridge_me;

		//add to brdige list tail
		list_add_tail(&bridge_info->b_node, &classf_info->bridge_info_list);

		//reset uni port mask in bridge
		uni_port_mask = 0x0;
		
		//collect info for each bridge port
		list_for_each_entry(bridge_port_me, &miptr_bridge_port->me_instance_list, instance_node)
		{
			if (!me_related(bridge_me, bridge_port_me)) //belong to it
			{
				continue; //check next one
			}

			//get port info
			if (switch_get_port_info_by_me(bridge_port_me, &port_info) != 0)
			{
				if(bridge_port_me != NULL)
					dbprintf(LOG_ERR, "can not get port info, meid=0x%.4x\n", bridge_port_me->meid);
				continue;
			}

			//skip bridge port was logical port num not defined
			if ((logical_port_mask & ( 1 << port_info.logical_port_id)) == 0) //not existed
			{
				if(bridge_port_me != NULL)
					dbprintf(LOG_ERR, "logical port num was not defined, skip, meid=0x%.4x, logical port id=%d\n", bridge_port_me->meid, port_info.logical_port_id);
				continue;
			}

			switch(port_info.port_type)
			{
			case ENV_BRIDGE_PORT_TYPE_CPU:
			case ENV_BRIDGE_PORT_TYPE_UNI:
			case ENV_BRIDGE_PORT_TYPE_IPHOST:
				//add a node to lan_port_list
				lan_port = malloc_safe(sizeof(struct classf_bridge_info_lan_port_t));
				lan_port->me_bridge = bridge_me;
				lan_port->me_bridge_port = bridge_port_me;
				if (port_info.port_type == ENV_BRIDGE_PORT_TYPE_UNI &&
					omci_env_g.port2port_enable)
				{
					lan_port->port_type = CLASSF_BRIDGE_PORT_TYPE_UNI_DC;
				} else {
					lan_port->port_type = port_info.port_type;
				}
				lan_port->logical_port_id = port_info.logical_port_id;
				lan_port->port_type_index = port_info.port_type_index;

				//set sorting elements
				if (port_info.port_type == ENV_BRIDGE_PORT_TYPE_CPU ||
					port_info.port_type == ENV_BRIDGE_PORT_TYPE_IPHOST)
				{
					lan_port->port_type_sort = ENV_BRIDGE_PORT_TYPE_CPU; //the highest priority port type
					lan_port->port_type_index_sort = 0; //the highest priority port type index

					//increase bridge cpu_port_num counter
					bridge_info->cpu_port_num++;
				} else {
					lan_port->port_type_sort = lan_port->port_type; //the highest priority port type
					lan_port->port_type_index_sort = lan_port->port_type_index; //the highest priority port type index
				}

				//collect uni port mask in the same bridge
				if (port_info.port_type == ENV_BRIDGE_PORT_TYPE_UNI)
				{
					uni_port_mask |= (1 << port_info.logical_port_id);
				}
				
				INIT_LIST_HEAD(&lan_port->port_tagging.us_tagging_rule_list);
				INIT_LIST_HEAD(&lan_port->port_tagging.ds_tagging_rule_list);
				INIT_LIST_HEAD(&lan_port->port_protocol_vlan.us_pvlan_rule_list);
				INIT_LIST_HEAD(&lan_port->port_protocol_vlan.ds_pvlan_rule_list);

				//add
				list_add_tail(&lan_port->lp_node, &bridge_info->lan_port_list);

				//collect port filter
				if (classf_collect_port_filter(bridge_info, bridge_port_me, &lan_port->port_filter) < 0)
				{
					dbprintf(LOG_ERR, "collect port filter error!!, meid=0x%.4x\n", bridge_port_me->meid);
				}
#ifndef X86	// kevin_tung, add ifndef for x86 compiling, to be fixed
				//collect port tagging rules
				if (omciutil_vlan_collect_rules_by_port(ER_ATTR_GROUP_HW_OP_NONE,
					bridge_port_me,
					NULL,
					&lan_port->port_tagging.us_tagging_rule_list,
					&lan_port->port_tagging.ds_tagging_rule_list,
					omci_env_g.classf_tag_mode) < 0)
				{ //tag_mode set to limit one tag on lan port most,
					dbprintf(LOG_ERR, "collect port tagging rules error!!, meid=0x%.4x\n", bridge_port_me->meid);
				}
#endif
				//collect dscp2pbit_info for this port
				omciutil_vlan_collect_171_dscp2pbit_usage_by_port(lan_port->me_bridge_port, &(lan_port->port_tagging.dscp2pbit_info), 0);
#if 0 // Remove VEIP related workaround
				//add veip(pon) port tagging rules by real tci
				if ((port_info.port_type == ENV_BRIDGE_PORT_TYPE_CPU ||
					port_info.port_type == ENV_BRIDGE_PORT_TYPE_IPHOST) &&
					port_info.port_type_index < ENV_SIZE_VEIP_PORT &&
					omci_env_g.classf_veip_real_tci_map[port_info.port_type_index] != 0)
				{
					classf_add_rules_by_real_tci_on_veip_ports(&lan_port->port_tagging.us_tagging_rule_list,
						&lan_port->port_tagging.ds_tagging_rule_list,
						bridge_port_me,
						port_info.port_type_index);
				}

				//modify veip(pon) port tagging rules to distinguish veip ports
				if ((port_info.port_type == ENV_BRIDGE_PORT_TYPE_CPU ||
					port_info.port_type == ENV_BRIDGE_PORT_TYPE_IPHOST) &&
					port_info.port_type_index < ENV_SIZE_VEIP_PORT &&
					omci_env_g.classf_veip_orig_tci_map[port_info.port_type_index] != 0)
				{
					classf_modify_rules_by_orig_tci_to_distinguish_veip_ports(&lan_port->port_tagging.us_tagging_rule_list,
						&lan_port->port_tagging.ds_tagging_rule_list,
						port_info.port_type_index);
				}
#endif
/*
				//collect protocol vlan by port, use global dscp2pbit info
				classf_protocol_vlan_collect_rules_by_port(&classf_info->system_info.protocol_vlan_rule_list,
					lan_port->me_bridge_port,
					&classf_info->system_info.dscp2pbit_info,
					&lan_port->port_protocol_vlan.us_pvlan_rule_list,
					&lan_port->port_protocol_vlan.ds_pvlan_rule_list);

				//add default rules for us/ds list if empty, check pvlan list first
				if (omci_env_g.classf_add_default_rules == 2 || //force add
					(list_empty(&lan_port->port_protocol_vlan.us_pvlan_rule_list) &&
					list_empty(&lan_port->port_tagging.us_tagging_rule_list) &&
					omci_env_g.classf_add_default_rules == 1))
				{
					classf_assign_default_tagging_rules(&lan_port->port_tagging.us_tagging_rule_list, bridge_port_me, omci_env_g.classf_tag_mode, 0); //us, no 2 tags rules
					if (list_empty(&lan_port->port_protocol_vlan.ds_pvlan_rule_list) &&
						list_empty(&lan_port->port_tagging.ds_tagging_rule_list) &&
						omci_env_g.classf_add_default_rules == 1)
					{
						lan_port->port_tagging.us_tagging_rule_default = 1;
					}
				}
				if (omci_env_g.classf_add_default_rules == 2 || //force add
					(list_empty(&lan_port->port_protocol_vlan.ds_pvlan_rule_list) &&
					list_empty(&lan_port->port_tagging.ds_tagging_rule_list) &&
					omci_env_g.classf_add_default_rules == 1))
				{
					classf_assign_default_tagging_rules(&lan_port->port_tagging.ds_tagging_rule_list, bridge_port_me, 0, 1); //ds, 2 tags rules remove 1 tag
					if (list_empty(&lan_port->port_protocol_vlan.ds_pvlan_rule_list) &&
						list_empty(&lan_port->port_tagging.ds_tagging_rule_list) &&
						omci_env_g.classf_add_default_rules == 1)
					{
						lan_port->port_tagging.ds_tagging_rule_default = 1;
					}
				}
*/

				//collect port veip hwnat default rules
				if (omci_env_g.classf_add_hwnat_1tag_rules && //enable
					lan_port->port_type == ENV_BRIDGE_PORT_TYPE_CPU)
				{
					classf_collect_veip_hwnat_default_rules(bridge_port_me, &lan_port->port_tagging.us_tagging_rule_list, &lan_port->port_tagging.ds_tagging_rule_list);
				}

				//collect default rules by me 84 for GWS
				if (lan_port->port_type == ENV_BRIDGE_PORT_TYPE_UNI)
				{	// Adtran or auto
					if ( (omci_env_g.classf_add_default_rules_by_me_84 == 1) ||
						 (omci_env_g.classf_add_default_rules_by_me_84 == 2 && omci_get_olt_vendor() == ENV_OLT_WORKAROUND_ADTRAN)	)
					{
						classf_collect_default_rules_by_84(bridge_port_me, &lan_port->port_tagging.us_tagging_rule_list, &lan_port->port_tagging.ds_tagging_rule_list);
					}
				}

				break;
			case ENV_BRIDGE_PORT_TYPE_WAN:
				//add a node to wan_port_list
				wan_port = malloc_safe(sizeof(struct classf_bridge_info_wan_port_t));
				wan_port->me_bridge = bridge_me;
				wan_port->me_bridge_port = bridge_port_me;
				wan_port->port_type_index = port_info.port_type_index;
				wan_port->logical_port_id = port_info.logical_port_id;
				list_add_tail(&wan_port->wp_node, &bridge_info->wan_port_list);

				//collect port gem
				if (classf_collect_port_gem(wan_port, &classf_info->system_info.dscp2pbit_info) < 0)
				{
					dbprintf(LOG_ERR, "collect port gem error!!, meid=0x%.4x\n", bridge_port_me->meid);
				}

				//collect port filter
				if (classf_collect_port_filter(bridge_info, bridge_port_me, &wan_port->port_filter) < 0)
				{
					dbprintf(LOG_ERR, "collect port filter error!!, meid=0x%.4x\n", bridge_port_me->meid);
				}

				break;
			case ENV_BRIDGE_PORT_TYPE_DS_BCAST:
				dbprintf(LOG_WARNING, "port type is downstream broadcast, don't care, meid=0x%.4x\n", bridge_port_me->meid);
				continue;
			default:
				dbprintf(LOG_ERR, "port type can not reg, meid=0x%.4x\n", bridge_port_me->meid);
				continue;
			}
		}

		//process protocol vlan, default rules...!!
		classf_collect_bridge_process(bridge_info,
			&classf_info->system_info.protocol_vlan_rule_list,
			&classf_info->system_info.dscp2pbit_info);

		//reassign veip(cpu) port tagging rules to uni ports, only for hw(classf) only part.
		//if for all(sw/hw), this reassign collection would been done in port rules collected of omci
		if (omci_env_g.classf_veip_rules_reassign == 1 || //1: hw MARK and COPY
			omci_env_g.classf_veip_rules_reassign == 2) //2: hw only MARK
		{
			classf_collect_reassign_veip_rules_to_uni(bridge_info);
		}

		//join
		classf_bridge_join(bridge_info);

		//assign sequence number
		classf_bridge_assign_global_seqnum_to_rule_candi(bridge_info);

		//check tagging, filter and 802.1p to elimate
		classf_bridge_rule_determine(bridge_info, &classf_info->system_info.dscp2pbit_info, &classf_info->system_info.tlan_info.tlan_table);

		//calix proprietary
		if(strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX)==0 &&
			omci_env_g.classf_tlan_pattern == 1) //enable
		{
			classf_rule_collect_tlan_pattern(bridge_info, &classf_info->system_info.tlan_info);
		}

		//release join result candidate rules usually after determination, if want to keep for debug or something else, enable below flag.
		if (omci_env_g.classf_keep_candi == 0)
		{
			classf_rule_collect_release_candi_rules_first(classf_info);
		}

		//assign global sequence number and uni port mask in the same bridge
		classf_bridge_set_additional_info_to_rule(bridge_info, uni_port_mask);
	}

	if(strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX)==0)
	{
		//find the same criterion(smac) for l2 rules(me249 for calix), to collect lan_mask of each classf rules 
		classf_rule_collect_l2_rules_the_same_criterion(classf_info);

		//analyze tlan condition, to save acl number usage as possible
		classf_rule_analyze_tlan_aggregation(classf_info);
	}

	fwk_mutex_unlock(&classf_info->mutex);

	return 0;
}

void
classf_rule_release(struct classf_info_t *classf_info)
{
	if (classf_info == NULL)
	{
		dbprintf(LOG_ERR, "param error!!\n");
		return;
	}

	fwk_mutex_lock(&classf_info->mutex);

	_classf_rule_release(classf_info);

	fwk_mutex_unlock(&classf_info->mutex);

	return;
}


