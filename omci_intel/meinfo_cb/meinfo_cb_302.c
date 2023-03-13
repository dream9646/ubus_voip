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
 * File    : meinfo_cb_302.c
 *
 ******************************************************************/

#include <string.h>
#include "meinfo_cb.h"
#include "omcimsg.h"
#include "util.h"
#include "hwresource.h"

#include "cfm_pkt.h"
#include "cfm_mp.h"
#include "cfm_util.h"
#include "cfm_send.h"

//classid 302 9.3.22 Dot1ag_MEP

static int
get_classid_by_302_tp_type(unsigned char tp_type)
{
	switch (tp_type) {
	case 0: return 47;	// mac bridge port config data
	case 1: return 130;	// 802.1p mapper service
	}
	dbprintf(LOG_ERR, "tp_type %d is undefined\n", tp_type);	
	return -1;
}

static int 
meinfo_cb_302_get_pointer_classid(struct me_t *me, unsigned char attr_order)
{
	if (attr_order==1) {
		return get_classid_by_302_tp_type(meinfo_util_me_attr_data(me, 2));
	}
	return -1;
}

static int 
meinfo_cb_302_is_attr_valid(struct me_t *me, unsigned char attr_order, struct attr_value_t *attr)
{
	if (attr_order==2) {		// tp type
		if (get_classid_by_302_tp_type(attr->data)<0)
			return 0;
	}
	return 1;
}

static int
meinfo_cb_302_set_admin_lock(struct me_t *me, int lockval)
{
	struct attr_value_t attr={ .data=lockval?1:0, .ptr=NULL, };	
	meinfo_me_attr_set(me, 7, &attr, 0);
	return 0;
}

static int 
meinfo_cb_302_is_admin_locked(struct me_t *me)
{
	return me?meinfo_util_me_attr_data(me, 7):0;
}

static int 
meinfo_cb_302_set(struct me_t *me, unsigned char attr_mask[2])
{	
	struct attr_value_t attr;
	cfm_config_t *cfm_config = NULL;
	unsigned short mep_id = meinfo_util_me_attr_data(me, 4);
	unsigned char md_level = 0xFF;
	struct me_t *bp = NULL, *ma = meinfo_me_get_by_meid(300, meinfo_util_me_attr_data(me, 3));
	unsigned char tp_type = meinfo_util_me_attr_data(me, 2);
	int port_type = -1, port_idx = -1;
	
	if (omci_env_g.cfm_enable == 0) {
       		dbprintf(LOG_ERR,"cfm_enable==0\n");
		return -1;
	}
	if(ma) {
		struct me_t *md = meinfo_me_get_by_meid(299, meinfo_util_me_attr_data(ma, 1));
		if(md) md_level = meinfo_util_me_attr_data(md, 1);
	}
	
	if(tp_type==0) {
		bp = meinfo_me_get_by_meid(47, meinfo_util_me_attr_data(me, 1));
	} else {
		struct meinfo_t *miptr = meinfo_util_miptr(47);
		struct me_t *meptr_47 = NULL;
		list_for_each_entry(meptr_47, &miptr->me_instance_list, instance_node) {
			if((meinfo_util_me_attr_data(meptr_47, 3)==3) && (meinfo_util_me_attr_data(meptr_47, 4)==meinfo_util_me_attr_data(me, 1))) {
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
	if (port_type < 0 || port_idx < 0)
		return -1;
	
	if((md_level != 0xFF) && (cfm_config = cfm_mp_find(md_level, mep_id, port_type, port_idx)) != NULL) {
		if (util_attr_mask_get_bit(attr_mask, 7)) { // AdminState
			meinfo_me_attr_get(me, 7, &attr);
			cfm_config->admin_state = (unsigned short)attr.data;
		}
		
		if (util_attr_mask_get_bit(attr_mask, 10)) { // Peer MEP IDs
			int i;
			attr.ptr = meinfo_util_me_attr_ptr(me , 10);
			for(i=0; i<12; i++)
				cfm_config->peer_mep_ids[i] = htons(((unsigned short *) attr.ptr)[i]);
		}
		
		if (util_attr_mask_get_bit(attr_mask, 12)) { // Fault alarm threshold
			meinfo_me_attr_get(me, 12, &attr);
			cfm_config->fault_alarm_threshold = (unsigned char)attr.data;
		}
		
		if (util_attr_mask_get_bit(attr_mask, 13)) { // Alarm declaration soak time
			meinfo_me_attr_get(me, 13, &attr);
			cfm_config->alarm_declaration_soak_time = (unsigned short)attr.data;
		}
		
		if (util_attr_mask_get_bit(attr_mask, 14)) { // Alarm clear soak time
			meinfo_me_attr_get(me, 14, &attr);
			cfm_config->alarm_clear_soak_time = (unsigned short)attr.data;
		}
	}
	
	return 0;
}

static int 
meinfo_cb_302_test_is_supported(struct me_t *me, unsigned char *req)
{
	struct omcimsg_test_dot1ag_mep_t *test_req = (struct omcimsg_test_dot1ag_mep_t *) req;
	if (test_req->select_test==0 ||		// Ethernet loopback test
	    test_req->select_test==1)		// 802.1ag link trace test
		return 1;
	return 0;
}

static int
meinfo_cb_302_test_result_lt(cfm_config_t *cfm_config, unsigned short pointer_to_a_general_purpose_buffer_me)
{
	struct me_t *meptr_308 = meinfo_me_get_by_meid(308, pointer_to_a_general_purpose_buffer_me);
	
	if(meptr_308 != NULL) {
		int size = 0;
		cfm_pkt_ltr_entry_t *entry = NULL;
		struct timeval time;
		time.tv_sec = omci_env_g.cfm_send_timeout;
		time.tv_usec = 0;
		select(0, NULL, NULL, NULL, &time);
		list_for_each_entry(entry, &cfm_config->cfm_recv_ltr_list, node) {
			size += (sizeof(unsigned short) + entry->ltr_length);
			select(0, NULL, NULL, NULL, &time);
		}
		if(size) {
			char *buf = malloc_safe(size);
			if(buf != NULL) {
				int offset = 0;
				list_for_each_entry(entry, &cfm_config->cfm_recv_ltr_list, node) {
					unsigned short ltr_length = htons(entry->ltr_length);
					memcpy(buf+offset, (char *) &ltr_length, sizeof(unsigned short));
					memcpy(buf+offset+sizeof(unsigned short), (char *) &entry->u.ltr_payload, entry->ltr_length);
					offset += (sizeof(unsigned short)+entry->ltr_length);
				}
				meinfo_me_attr_set_table_all_entries_by_mem(meptr_308, 2, size, buf, 1);
				free_safe(buf);
			} else
				return -1;
		} else
			return -1;
	}
	return 0;
}

static int 
meinfo_cb_302_test(struct me_t *me, unsigned char *req, unsigned char *result)
{
	struct omcimsg_test_dot1ag_mep_t *test_req = (struct omcimsg_test_dot1ag_mep_t *) req;
	struct omcimsg_test_result_dot1ag_mep_t *test_result=(struct omcimsg_test_result_dot1ag_mep_t *)result;
	cfm_config_t *cfm_config = NULL;
	unsigned char null_mac[IFHWADDRLEN] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	unsigned short mep_id = meinfo_util_me_attr_data(me, 4);
	unsigned char md_level = 0xFF;
	struct me_t *bp = NULL, *ma = meinfo_me_get_by_meid(300, meinfo_util_me_attr_data(me, 3));
	unsigned char tp_type = meinfo_util_me_attr_data(me, 2);
	int port_type = -1, port_idx = -1;
	
	if (omci_env_g.cfm_enable == 0) {
       		dbprintf(LOG_ERR,"cfm_enable==0\n");
		return -1;
	}
	if(tp_type==0) {
		bp = meinfo_me_get_by_meid(47, meinfo_util_me_attr_data(me, 1));
	} else {
		struct meinfo_t *miptr = meinfo_util_miptr(47);
		struct me_t *meptr_47 = NULL;
		list_for_each_entry(meptr_47, &miptr->me_instance_list, instance_node) {
			if((meinfo_util_me_attr_data(meptr_47, 3)==3) && (meinfo_util_me_attr_data(meptr_47, 4)==meinfo_util_me_attr_data(me, 1))) {
				bp = meptr_47;
				break;
			}
		}
	}
	
	if(ma) {
		struct me_t *md = meinfo_me_get_by_meid(299, meinfo_util_me_attr_data(ma, 1));
		if(md) md_level = meinfo_util_me_attr_data(md, 1);
	}
	
	if(bp) {
		struct me_t *ibridgeport_me = hwresource_public2private(bp);
		if(ibridgeport_me && (meinfo_util_me_attr_data(ibridgeport_me, 1) == 1)) { // Occupied
			port_type = meinfo_util_me_attr_data(ibridgeport_me, 4);
			port_idx = meinfo_util_me_attr_data(ibridgeport_me, 5);
		}
	}
	if (port_type < 0 || port_idx < 0)
		return -1;
	
	if (test_req->select_test==0) {	// loopback test
		cfm_pkt_lbr_entry_t *entry;
		if((md_level != 0xFF) && (cfm_config = cfm_mp_find(md_level, mep_id, port_type, port_idx)) != NULL) {
			unsigned long long lbr_timestamp = 0;
			int i;
			if(memcmp(test_req->u.lb.target_mac, null_mac, IFHWADDRLEN) == 0) { // Find MAC by MEP_ID
				cfm_pkt_rmep_entry_t *entry = NULL;
				list_for_each_entry(entry, &cfm_config->cfm_recv_rmep_list, node) {
					if(entry->rmep_id == htons(test_req->u.lb.target_mep_id)) {
						memcpy(test_req->u.lb.target_mac, entry->rmep_mac, IFHWADDRLEN);
						break;
					}
				}
			}
			cfm_config->cfm_pkt_pm[CFM_PDU_LBR].rx = 0; // Reset LBR RX counter before send
			cfm_config->cfm_pkt_pm[CFM_PDU_LBR].drop = 0; // Reset LBR Drop counter before send
			for(i=0; i<htons(test_req->u.lb.repeat_count); i++) {
				struct timeval time;
				time.tv_sec = omci_env_g.cfm_send_timeout;
				time.tv_usec = 0;
				if(cfm_config == NULL) {
					test_result->test_result = OMCIMSG_TEST_RESULT_DOT1AG_MEP_TIMEOUT;
					return 0;
				}
				if(((test_req->u.lb.priority >> 4) & 1) == 0) // c == 0
					cfm_send_lbm(cfm_config, test_req->u.lb.target_mac, (test_req->u.lb.priority>>1)&0x0F, test_req->u.lb.priority&1);
				else // c == 1
					cfm_send_lbm(cfm_config, test_req->u.lb.target_mac, -1, 0);
				select(0, NULL, NULL, NULL, &time);
			}

			test_result->test_result = OMCIMSG_TEST_RESULT_DOT1AG_MEP_TIMEOUT;
			list_for_each_entry(entry, &cfm_config->cfm_recv_lbr_list, node) {			
				if(memcmp(entry->lbr_mac, test_req->u.lb.target_mac, IFHWADDRLEN) == 0) {
					test_result->test_result = OMCIMSG_TEST_RESULT_DOT1AG_MEP_SUCCESS;
					lbr_timestamp = entry->lbr_timestamp;
				}					
			}
			test_result->u.lb.lbr_count_valid = htons(cfm_config->cfm_pkt_pm[CFM_PDU_LBR].rx);
			test_result->u.lb.lbr_count_out_of_order = 0;
			test_result->u.lb.lbr_count_mismatch = htons(cfm_config->cfm_pkt_pm[CFM_PDU_LBR].drop);

			if (cfm_config->cfm_send_lbm_timestamp != 0 && 
			    cfm_config->cfm_send_lbm_timestamp != 0xFFFFFFFF &&
			   lbr_timestamp != 0 && lbr_timestamp != 0xFFFFFFFF)
				test_result->u.lb.lbr_delay =	(lbr_timestamp > cfm_config->cfm_send_lbm_timestamp) ?
							htonl(lbr_timestamp - cfm_config->cfm_send_lbm_timestamp) : 0;
			else
				test_result->u.lb.lbr_delay =	0;
		} else {
			test_result->test_result = OMCIMSG_TEST_RESULT_DOT1AG_MEP_TIMEOUT;
		}
		return 0;

	} else if (test_req->select_test==1) { // linktrace test
		if((md_level != 0xFF) && (cfm_config = cfm_mp_find(md_level, mep_id,  port_type, port_idx)) != NULL) {
			if(memcmp(test_req->u.lt.target_mac, null_mac, IFHWADDRLEN) == 0) { // Find MAC by MEP_ID
				cfm_pkt_rmep_entry_t *entry = NULL;
				list_for_each_entry(entry, &cfm_config->cfm_recv_rmep_list, node) {
					if(entry->rmep_id == htons(test_req->u.lt.target_mep_id)) {
						memcpy(test_req->u.lb.target_mac, entry->rmep_mac, IFHWADDRLEN);
						break;
					}
				}
			}
			cfm_send_ltm(cfm_config, test_req->u.lt.target_mac, -1, 0, test_req->u.lt.ttl, ((test_req->u.lt.flags >> 7) & 1));
			test_result->u.lt.ltm_xid = htonl(cfm_config->nextLTMtransID);
			mep_id = htons(mep_id);
			memcpy(test_result->u.lt.ltm_tlv, &mep_id, sizeof(unsigned short));
			memcpy(test_result->u.lt.ltm_tlv+sizeof(unsigned short), cfm_config->macaddr, IFHWADDRLEN);
			if(meinfo_cb_302_test_result_lt(cfm_config, htons(test_req->u.lt.pointer_to_a_general_purpose_buffer_me)) == 0)
				test_result->test_result = OMCIMSG_TEST_RESULT_DOT1AG_MEP_SUCCESS;
			else
				test_result->test_result = OMCIMSG_TEST_RESULT_DOT1AG_MEP_TIMEOUT;
		} else {
			test_result->test_result = OMCIMSG_TEST_RESULT_DOT1AG_MEP_TIMEOUT;
		}
		return 0;
	}
	test_result->test_result=OMCIMSG_TEST_RESULT_DOT1AG_MEP_SUCCESS;
	return 0;
}

struct meinfo_callback_t meinfo_cb_302 = {
	.set_admin_lock_cb	= meinfo_cb_302_set_admin_lock,
	.is_admin_locked_cb	= meinfo_cb_302_is_admin_locked,
	.get_pointer_classid_cb	= meinfo_cb_302_get_pointer_classid,
	.is_attr_valid_cb	= meinfo_cb_302_is_attr_valid,
	.set_cb	= meinfo_cb_302_set,
	.test_is_supported_cb	= meinfo_cb_302_test_is_supported,
	.test_cb		= meinfo_cb_302_test,
};
