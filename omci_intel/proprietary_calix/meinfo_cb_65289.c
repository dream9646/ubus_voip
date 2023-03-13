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
 * Module  : proprietary_calix
 * File    : meinfo_cb_65289.c
 *
 ******************************************************************/

#include <string.h>
#include "meinfo_cb.h"
#include "util.h"
#include "hwresource.h"
#include "omciutil_misc.h"
#include "cfm_pkt.h"

//classid 65289 9.99.27 OamMep

static int 
meinfo_cb_65289_get(struct me_t *me, unsigned char attr_mask[2])
{
	unsigned char md_level = 0xFF, tp_type = 0;
	unsigned short meg_id = 0, mep_id = 0, meid = 0;
	cfm_config_t *cfm_config = NULL;
	struct me_t *parent_me = NULL, *meptr = NULL, *bridgeport_me = NULL, *ibridgeport_me = NULL;
	int port_type = -1, port_idx = -1;
	
	// Reset all attributes first in case CFM can't be found and leave previous records
	if (util_attr_mask_get_bit(attr_mask, 14)) { // DynamicMepTSize
		struct attr_value_t attr;
		attr.data = 0;
		meinfo_me_attr_set(me, 14, &attr, 0);
	}
	if (util_attr_mask_get_bit(attr_mask, 15)) { // DynamicMepIds
		struct attr_table_header_t *tab_header = (struct attr_table_header_t *)meinfo_util_me_attr_ptr(me, 15);
		//clear all old entries from table
		meinfo_me_attr_release_table_all_entries(tab_header);
	}
	
	if (omci_env_g.cfm_enable == 0) {
       		dbprintf(LOG_INFO,"cfm_enable==0\n");
		return 0;
	}
	
	meg_id = meinfo_util_me_attr_data(me, 1);
	mep_id = meinfo_util_me_attr_data(me, 2);
	tp_type = meinfo_util_me_attr_data(me, 4);
	meid = meinfo_util_me_attr_data(me, 5);

	parent_me = meinfo_me_get_by_meid(65288, meg_id);
	if (!parent_me) {
		dbprintf(LOG_ERR, "unable to find parent me, classid=%u, meid=%u\n", 65288, me->meid);	
		return -1;
	} else {
		md_level = meinfo_util_me_attr_data(parent_me, 3);
	}

	switch(tp_type) {
		case 1: // pptp eth uni
			if ((meptr = meinfo_me_get_by_meid(11, meid)) == NULL) {
				dbprintf(LOG_ERR, "could not get bridge port me, classid=11, meid=%u\n", meid);
				return -1;
			}
			break;
		case 3: // 802.1p mapper service
			if ((meptr = meinfo_me_get_by_meid(130, meid)) == NULL) {
				dbprintf(LOG_ERR, "could not get bridge port me, classid=130, meid=%u\n", meid);
				return -1;
			}
			break;
		case 4: // ip host
			if ((meptr = meinfo_me_get_by_meid(134, meid)) == NULL) {
				dbprintf(LOG_ERR, "could not get bridge port me, classid=134, meid=%u\n", meid);
				return -1;
			}
			break;
		case 5: // gem iwtp
			if ((meptr = meinfo_me_get_by_meid(266, meid)) == NULL) {
				dbprintf(LOG_ERR, "could not get bridge port me, classid=266, meid=%u\n", meid);
				return -1;
			}
			break;
		case 6: // gem mcast iwtp
			if ((meptr = meinfo_me_get_by_meid(281, meid)) == NULL) {
				dbprintf(LOG_ERR, "could not get bridge port me, classid=281, meid=%u\n", meid);
				return -1;
			}
			break;
		case 11: // veip
			if ((meptr = meinfo_me_get_by_meid(329, meid)) == NULL) {
				dbprintf(LOG_ERR, "could not get bridge port me, classid=329, meid=%u\n", meid);
				return -1;
			}
			break;
		default:
			return 0;
	}

	bridgeport_me = omciutil_misc_find_me_related_bport(meptr);
	ibridgeport_me = hwresource_public2private(bridgeport_me);

	if(ibridgeport_me) {
		port_type = meinfo_util_me_attr_data(ibridgeport_me, 4);
		port_idx = meinfo_util_me_attr_data(ibridgeport_me, 5);

		if (port_type < 0 || port_idx < 0)
			return -1;

		if((cfm_config = cfm_mp_find(md_level, mep_id, port_type, port_idx)) != NULL) {
			if (!cfm_config->admin_state) {
				if (util_attr_mask_get_bit(attr_mask, 15)) { // DynamicMepIds
					struct attr_table_header_t *tab_header = (struct attr_table_header_t *)meinfo_util_me_attr_ptr(me, 15);
					void *tab_entry = malloc_safe(13);
					cfm_pkt_rmep_entry_t *entry = NULL;
					//clear all old entries from table
					meinfo_me_attr_release_table_all_entries(tab_header);
					list_for_each_entry(entry, &cfm_config->cfm_recv_rmep_list, node) {
						int i, rmep_state;
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
						memset(tab_entry, 0, 13);
						util_bitmap_set_value(tab_entry, 13*8, 0, 16, htons(entry->rmep_id));
						for (i=0 ; i<6 ; i++)	// macaddr
							util_bitmap_set_value(tab_entry, 13*8, 16+i*8, 8, entry->rmep_mac[i]);
						util_bitmap_set_value(tab_entry, 13*8, 64, 32, htonl((unsigned int) (entry->rmep_timestamp/1000000))); // us->sec
						util_bitmap_set_value(tab_entry, 13*8, 96, 3, rmep_state);
						util_bitmap_set_value(tab_entry, 13*8, 99, 1, entry->rmep_rdi);
						util_bitmap_set_value(tab_entry, 13*8, 100, 2, entry->rmep_port_state);
						util_bitmap_set_value(tab_entry, 13*8, 102, 2, 0); // reserved
						meinfo_me_attr_set_table_entry(me, 15, tab_entry, 0);
					}
					free_safe(tab_entry);
				}
				if (util_attr_mask_get_bit(attr_mask, 14)) { // DynamicMepTSize
					struct attr_value_t attr;
					attr.data = meinfo_me_attr_get_table_entry_count(me, 15);
					meinfo_me_attr_set(me, 14, &attr, 0);
				}
			}
		}
	}
	
	return 0;
}

static int
meinfo_cb_65289_alarm(struct me_t *me, unsigned char alarm_mask[28])
{
	unsigned char md_level = 0xFF, tp_type = 0;
	unsigned short meg_id = 0, mep_id = 0, meid = 0;
	cfm_config_t *cfm_config = NULL;
	struct me_t *parent_me = NULL, *meptr = NULL, *bridgeport_me = NULL, *ibridgeport_me = NULL;
	int port_type = -1, port_idx = -1;
	
	if (omci_env_g.cfm_enable == 0) {
       		dbprintf(LOG_INFO,"cfm_enable==0\n");
		return 0;
	}
	
	meg_id = meinfo_util_me_attr_data(me, 1);
	mep_id = meinfo_util_me_attr_data(me, 2);
	tp_type = meinfo_util_me_attr_data(me, 4);
	meid = meinfo_util_me_attr_data(me, 5);

	parent_me = meinfo_me_get_by_meid(65288, meg_id);
	if (!parent_me) {
		dbprintf(LOG_ERR, "unable to find parent me, classid=%u, meid=%u\n", 65288, me->meid);	
		return -1;
	} else {
		md_level = meinfo_util_me_attr_data(parent_me, 3);
	}

	switch(tp_type) {
		case 1: // pptp eth uni
			if ((meptr = meinfo_me_get_by_meid(11, meid)) == NULL) {
				dbprintf(LOG_WARNING, "could not get bridge port me, classid=11, meid=%u\n", meid);
				return -1;
			}
			break;
		case 3: // 802.1p mapper service
			if ((meptr = meinfo_me_get_by_meid(130, meid)) == NULL) {
				dbprintf(LOG_WARNING, "could not get bridge port me, classid=130, meid=%u\n", meid);
				return -1;
			}
			break;
		case 4: // ip host
			if ((meptr = meinfo_me_get_by_meid(134, meid)) == NULL) {
				dbprintf(LOG_WARNING, "could not get bridge port me, classid=134, meid=%u\n", meid);
				return -1;
			}
			break;
		case 5: // gem iwtp
			if ((meptr = meinfo_me_get_by_meid(266, meid)) == NULL) {
				dbprintf(LOG_WARNING, "could not get bridge port me, classid=266, meid=%u\n", meid);
				return -1;
			}
			break;
		case 6: // gem mcast iwtp
			if ((meptr = meinfo_me_get_by_meid(281, meid)) == NULL) {
				dbprintf(LOG_WARNING, "could not get bridge port me, classid=281, meid=%u\n", meid);
				return -1;
			}
			break;
		case 11: // veip
			if ((meptr = meinfo_me_get_by_meid(329, meid)) == NULL) {
				dbprintf(LOG_WARNING, "could not get bridge port me, classid=329, meid=%u\n", meid);
				return -1;
			}
			break;
		default:
			return 0;
	}

	bridgeport_me = omciutil_misc_find_me_related_bport(meptr);
	ibridgeport_me = hwresource_public2private(bridgeport_me);

	if(ibridgeport_me) {
		port_type = meinfo_util_me_attr_data(ibridgeport_me, 4);
		port_idx = meinfo_util_me_attr_data(ibridgeport_me, 5);

		if (port_type < 0 || port_idx < 0)
			return -1;
	
		if((cfm_config = cfm_mp_find(md_level, mep_id, port_type, port_idx)) != NULL) {
			struct me_t *ext_me = meinfo_me_get_by_meid(65294, me->meid);
			char alarm_threshold_bitmap[] = { 	ALARM_THRESHOLD_0, ALARM_THRESHOLD_1, 
							ALARM_THRESHOLD_2, ALARM_THRESHOLD_3, 
							ALARM_THRESHOLD_4, ALARM_THRESHOLD_5, 
							ALARM_THRESHOLD_6 }; // refer to cfm alarm threshold
			unsigned char alarm_status = cfm_config->alarm_status;
			if(ext_me) {
				struct me_t *lm_me = meinfo_me_get_by_meid(65302, me->meid);
				struct me_t *dm_me = meinfo_me_get_by_meid(65303, me->meid);
				struct me_t *pm_me1 = meinfo_me_get_by_meid(65295, (unsigned short) meinfo_util_me_attr_data(ext_me, 1));
				struct me_t *pm_me2 = meinfo_me_get_by_meid(65296, (unsigned short) meinfo_util_me_attr_data(ext_me, 1));
				if(lm_me && dm_me && pm_me1 && pm_me2) {
					if (!cfm_config->admin_state && meinfo_util_me_attr_data(lm_me, 2) > meinfo_util_me_attr_data(pm_me1, 5)) // NearEndMaxLoss
						util_alarm_mask_set_bit(alarm_mask, 0);
					else if (cfm_config->admin_state || meinfo_util_me_attr_data(lm_me, 2) <= meinfo_util_me_attr_data(pm_me1, 6))
						util_alarm_mask_clear_bit(alarm_mask, 0);
					
					if (!cfm_config->admin_state && meinfo_util_me_attr_data(lm_me, 3) > meinfo_util_me_attr_data(pm_me1, 7)) // NearEndAvgLoss
						util_alarm_mask_set_bit(alarm_mask, 1);
					else if (cfm_config->admin_state || meinfo_util_me_attr_data(lm_me, 3) <= meinfo_util_me_attr_data(pm_me1, 8))
						util_alarm_mask_clear_bit(alarm_mask, 1);
					
					if (!cfm_config->admin_state && meinfo_util_me_attr_data(lm_me, 6) > meinfo_util_me_attr_data(pm_me1, 9)) // FarEndMaxLoss
						util_alarm_mask_set_bit(alarm_mask, 2);
					else if (cfm_config->admin_state || meinfo_util_me_attr_data(lm_me, 6) <= meinfo_util_me_attr_data(pm_me1, 10))
						util_alarm_mask_clear_bit(alarm_mask, 2);
					
					if (!cfm_config->admin_state && meinfo_util_me_attr_data(lm_me, 7) > meinfo_util_me_attr_data(pm_me1, 11)) // FarEndAvgLoss
						util_alarm_mask_set_bit(alarm_mask, 3);
					else if (cfm_config->admin_state || meinfo_util_me_attr_data(lm_me, 7) <= meinfo_util_me_attr_data(pm_me1, 12))
						util_alarm_mask_clear_bit(alarm_mask, 3);
					
					if (!cfm_config->admin_state && meinfo_util_me_attr_data(dm_me, 2) > meinfo_util_me_attr_data(pm_me2, 2)) // MaxDelay
						util_alarm_mask_set_bit(alarm_mask, 4);
					else if (cfm_config->admin_state || meinfo_util_me_attr_data(dm_me, 2) <= meinfo_util_me_attr_data(pm_me2, 3))
						util_alarm_mask_clear_bit(alarm_mask, 4);
					
					if (!cfm_config->admin_state && meinfo_util_me_attr_data(dm_me, 3) > meinfo_util_me_attr_data(pm_me2, 4)) // AvgDelay
						util_alarm_mask_set_bit(alarm_mask, 5);
					else if (cfm_config->admin_state || meinfo_util_me_attr_data(dm_me, 3) <= meinfo_util_me_attr_data(pm_me2, 5))
						util_alarm_mask_clear_bit(alarm_mask, 5);
					
					if (!cfm_config->admin_state && meinfo_util_me_attr_data(dm_me, 5) > meinfo_util_me_attr_data(pm_me2, 6)) // MaxDelayVar
						util_alarm_mask_set_bit(alarm_mask, 6);
					else if (cfm_config->admin_state || meinfo_util_me_attr_data(dm_me, 5) <= meinfo_util_me_attr_data(pm_me2, 7))
						util_alarm_mask_clear_bit(alarm_mask, 6);
					
					if (!cfm_config->admin_state && meinfo_util_me_attr_data(dm_me, 6) > meinfo_util_me_attr_data(pm_me1, 13)) // AvgDelayVar
						util_alarm_mask_set_bit(alarm_mask, 13);
					else if (cfm_config->admin_state || meinfo_util_me_attr_data(dm_me, 6) <= meinfo_util_me_attr_data(pm_me1, 14))
						util_alarm_mask_clear_bit(alarm_mask, 13);
				}
			}
			// sort the priority and only report higher priority alarm
			if (alarm_status & (1<<DEFECT_XCON_CCM)) {
				alarm_status = 1<<DEFECT_XCON_CCM;
			} else if (alarm_status & (1<<DEFECT_ERROR_CCM)) {
				alarm_status = 1<<DEFECT_ERROR_CCM;
			} else if (alarm_status & (1<<DEFECT_UNEXP_PERIOD)) {
				alarm_status = 1<<DEFECT_UNEXP_PERIOD;
			} else if (alarm_status & (1<<DEFECT_REMOTE_CCM)) {
				alarm_status = 1<<DEFECT_REMOTE_CCM;
			} else if (alarm_status & (1<<DEFECT_MAC_STATUS)) {
				alarm_status = 1<<DEFECT_MAC_STATUS;
			} else if (alarm_status & (1<<DEFECT_RDI_CCM)) {
				alarm_status = 1<<DEFECT_RDI_CCM;
			}
			if (!cfm_config->admin_state && (alarm_status & (1<<DEFECT_REMOTE_CCM)) && 
			    (alarm_threshold_bitmap[cfm_config->fault_alarm_threshold] & (1<<DEFECT_REMOTE_CCM))) // CCM LOC
				util_alarm_mask_set_bit(alarm_mask, 7);
			else
				util_alarm_mask_clear_bit(alarm_mask, 7);
			
			if (!cfm_config->admin_state && (alarm_status & (1<<DEFECT_MAC_STATUS)) && 
			    (alarm_threshold_bitmap[cfm_config->fault_alarm_threshold] & (1<<DEFECT_MAC_STATUS))) // CCM MAC
				util_alarm_mask_set_bit(alarm_mask, 8);
			else
				util_alarm_mask_clear_bit(alarm_mask, 8);
			
			if (!cfm_config->admin_state && (alarm_status & (1<<DEFECT_XCON_CCM)) && 
			    (alarm_threshold_bitmap[cfm_config->fault_alarm_threshold] & (1<<DEFECT_XCON_CCM))) // CCM MEG
				util_alarm_mask_set_bit(alarm_mask, 9);
			else
				util_alarm_mask_clear_bit(alarm_mask, 9);
			
			if (!cfm_config->admin_state && (alarm_status & (1<<DEFECT_ERROR_CCM)) && 
			    (alarm_threshold_bitmap[cfm_config->fault_alarm_threshold] & (1<<DEFECT_ERROR_CCM))) // CCM MEP
				util_alarm_mask_set_bit(alarm_mask, 10);
			else
				util_alarm_mask_clear_bit(alarm_mask, 10);
			
			if (!cfm_config->admin_state && (alarm_status & (1<<DEFECT_UNEXP_PERIOD)) && 
			    (alarm_threshold_bitmap[cfm_config->fault_alarm_threshold] & (1<<DEFECT_UNEXP_PERIOD))) // CCM Period
				util_alarm_mask_set_bit(alarm_mask, 11);
			else
				util_alarm_mask_clear_bit(alarm_mask, 11);
			
			if (!cfm_config->admin_state && (alarm_status & (1<<DEFECT_RDI_CCM)) && 
			    (alarm_threshold_bitmap[cfm_config->fault_alarm_threshold] & (1<<DEFECT_RDI_CCM))) // CCM RDI
				util_alarm_mask_set_bit(alarm_mask, 12);
			else
				util_alarm_mask_clear_bit(alarm_mask, 12);
		}
	}
	
	return 0;
}

struct meinfo_callback_t meinfo_cb_calix_65289 = {
	.alarm_cb	= meinfo_cb_65289_alarm,
	.get_cb		= meinfo_cb_65289_get,
};
