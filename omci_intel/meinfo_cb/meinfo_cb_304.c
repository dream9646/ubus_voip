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
 * File    : meinfo_cb_304.c
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

//classid 304 9.3.24 Dot1ag MEP CCM database

static unsigned short 
meinfo_cb_304_meid_gen(unsigned short instance_num)
{
	struct me_t *parent_me=meinfo_me_get_by_instance_num(302, instance_num);
	if (!parent_me) {
		dbprintf(LOG_ERR, "unable to find parent me, classid=%u, instance=%u\n", 302, instance_num);	
		return instance_num;
	}
	return parent_me->meid;
}

static int 
meinfo_cb_304_get(struct me_t *me, unsigned char attr_mask[2])
{
	int i;
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
	// clear previous value 
	for (i = 1; i <= miptr->attr_total; i++) {
		meinfo_me_attr_clear_value(me->classid, i, &me->attr[i].value);
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
		int index = 0;
		cfm_pkt_rmep_entry_t *entry = NULL;
		list_for_each_entry(entry, &cfm_config->cfm_recv_rmep_list, node) {
			char *buf = NULL;
			int tlv_size = (entry->rmep_sender_id_tlv.type == 1) ? htons(entry->rmep_sender_id_tlv.len) + 3 : 1;
			int table_size = 16 + tlv_size;
			index++;
			if(index > 12) break;
			buf = malloc_safe(table_size);
			if(buf != NULL) {
				unsigned short rmep_id = htons(entry->rmep_id);
				unsigned int rmep_timestamp = htonl(entry->rmep_timestamp);
				unsigned char rmep_state = 4;
				int ptr = 0;
				
				switch (entry->rmep_state) {
					case CCM_RMEP_STATE_EXPECT:
						rmep_state = (cfm_mp_is_peer_mep_id(cfm_config, entry->rmep_id)) ? 3 : 1; // fail(static) or idle(dynamic)
						break;
					case CCM_RMEP_STATE_DISCOVER:
					default:
						rmep_state = 2; // start
						break;
					case CCM_RMEP_STATE_LOST:
						rmep_state = 3; // fail
						break;
					case CCM_RMEP_STATE_ACTIVE:
						rmep_state = 4; // ok
						break;
				}
				
				// RMep identifier (2 bytes)
				memcpy(buf+ptr, (char *) &rmep_id, sizeof(unsigned short));
				ptr += sizeof(unsigned short);
				// RMep state (1 byte)
				memcpy(buf+ptr, (char *) &rmep_state, sizeof(unsigned char));
				ptr += sizeof(unsigned char);
				// Failed-ok time (4 bytes)
				memcpy(buf+ptr, (char *) &rmep_timestamp, sizeof(unsigned int));
				ptr += sizeof(unsigned int);
				// MAC address (6 bytes)
				memcpy(buf+ptr, (char *) &entry->rmep_mac, IFHWADDRLEN);
				ptr += IFHWADDRLEN;
				// RDI (1 byte)
				memcpy(buf+ptr, (char *) &entry->rmep_rdi, sizeof(unsigned char));
				ptr += sizeof(unsigned char);
				// Port status (1 byte)
				memcpy(buf+ptr, (char *) &entry->rmep_port_state, sizeof(unsigned char));
				ptr += sizeof(unsigned char);
				// Interface status (1 byte)
				memcpy(buf+ptr, (char *) &entry->rmep_iface_state, sizeof(unsigned char));
				ptr += sizeof(unsigned char);
				// Sender ID TLV (1 byte or M bytes)
				memcpy(buf+ptr, (char *) &entry->rmep_sender_id_tlv, tlv_size);
				
				meinfo_me_attr_set_table_entry(me, index, buf, 0);
				free_safe(buf);
			}
		}
	}
	return 0;
}

struct meinfo_callback_t meinfo_cb_304 = {
	.meid_gen_cb	= meinfo_cb_304_meid_gen,
	.get_cb		= meinfo_cb_304_get,
};
