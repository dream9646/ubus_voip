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
 * File    : meinfo_cb_303.c
 *
 ******************************************************************/

#include <string.h>
#include "meinfo_cb.h"
#include "util.h"
#include "hwresource.h"

#include "cfm_pkt.h"
#include "cfm_mp.h"
#include "cfm_util.h"
#include "cfm_send.h"

//classid 303 9.3.23 Dot1ag MEP status

static unsigned short 
meinfo_cb_303_meid_gen(unsigned short instance_num)
{
	struct me_t *parent_me=meinfo_me_get_by_instance_num(302, instance_num);
	if (!parent_me) {
		dbprintf(LOG_ERR, "unable to find parent me, classid=%u, instance=%u\n", 302, instance_num);	
		return instance_num;
	}
	return parent_me->meid;
}

static int 
meinfo_cb_303_get(struct me_t *me, unsigned char attr_mask[2])
{
	unsigned char md_level = 0xFF;
	unsigned short mep_id = 0;
	cfm_config_t *cfm_config = NULL;
	struct meinfo_t *miptr=meinfo_util_miptr(me->classid);
	struct me_t *parent_me=meinfo_me_get_by_meid(302, me->meid);
	int port_type = -1, port_idx = -1;
	
	if (omci_env_g.cfm_enable == 0) {
       		dbprintf(LOG_ERR,"cfm_enable==0\n");
		return -1;
	}
	if (!parent_me) {
		dbprintf(LOG_ERR, "unable to find parent me, classid=%u, meid=%u\n", 302, me->meid);	
		return -1;
	} else {
		struct me_t *bp = NULL, *ma = meinfo_me_get_by_meid(300, meinfo_util_me_attr_data(parent_me, 3));
		unsigned char tp_type = meinfo_util_me_attr_data(parent_me, 2);
		
		if(ma) {
			struct me_t *md = meinfo_me_get_by_meid(299, meinfo_util_me_attr_data(ma, 1));
			if(md) md_level = meinfo_util_me_attr_data(md, 1);
		}
		
		if(tp_type==0) {
			bp = meinfo_me_get_by_meid(47, meinfo_util_me_attr_data(parent_me, 1));
		} else {
			struct meinfo_t *miptr = meinfo_util_miptr(47);
			struct me_t *meptr_47 = NULL;
			list_for_each_entry(meptr_47, &miptr->me_instance_list, instance_node) {
				if((meinfo_util_me_attr_data(meptr_47, 3)==3) && (meinfo_util_me_attr_data(meptr_47, 4)==meinfo_util_me_attr_data(parent_me, 1))) {
					bp = meptr_47;
					break;
				}
			}
		}
		
		if(bp) {
			struct me_t *ibridgeport_me = hwresource_public2private(bp);
			if(ibridgeport_me && (meinfo_util_me_attr_data(ibridgeport_me, 1) == 1)) { // Occupied
				port_type = meinfo_util_me_attr_data(ibridgeport_me, 4);
				port_idx = meinfo_util_me_attr_data(ibridgeport_me, 5);
			}
		}
	}
	if (port_type < 0 || port_idx < 0)
		return -1;
	
	mep_id = meinfo_util_me_attr_data(parent_me, 4);
	if((md_level != 0xFF) && (cfm_config = cfm_mp_find(md_level, mep_id, port_type, port_idx)) != NULL) {
		if (util_attr_mask_get_bit(attr_mask, 1)) { // MEP MAC address
			struct attr_value_t attr;
			attr.ptr=malloc_safe(miptr->attrinfo[1].byte_size);
			memcpy(attr.ptr, (char *) cfm_config->macaddr, miptr->attrinfo[1].byte_size);
			meinfo_me_attr_set(me, 1, &attr, 0);
			free_safe(attr.ptr);
		}
		if (util_attr_mask_get_bit(attr_mask, 8)) { // CCM transmitted count
			struct attr_value_t attr;
			attr.data = cfm_config->cfm_pkt_pm[CFM_PDU_CCM].tx + cfm_config->cfm_pkt_pm[CFM_PDU_CCM].tx_rdi;
			meinfo_me_attr_set(me, 8, &attr, 0);
		}
		if (util_attr_mask_get_bit(attr_mask, 10)) { // LBRs transmitted count
			struct attr_value_t attr;
			attr.data = cfm_config->cfm_pkt_pm[CFM_PDU_LBR].tx;
			meinfo_me_attr_set(me, 10, &attr, 0);
		}
		if (util_attr_mask_get_bit(attr_mask, 11)) { // Next loopback transaction identifier
			struct attr_value_t attr;
			attr.data = cfm_config->nextLBMtransID;
			meinfo_me_attr_set(me, 11, &attr, 0);
		}
		if (util_attr_mask_get_bit(attr_mask, 12)) { // Next link trace transaction identifier
			struct attr_value_t attr;
			attr.data = cfm_config->nextLTMtransID;
			meinfo_me_attr_set(me, 12, &attr, 0);
		}
	}
	
	return 0;
}

struct meinfo_callback_t meinfo_cb_303 = {
	.meid_gen_cb	= meinfo_cb_303_meid_gen,
	.get_cb		= meinfo_cb_303_get,
};
