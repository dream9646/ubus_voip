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
 * File    : meinfo_cb_65303.c
 *
 ******************************************************************/

#include <string.h>
#include "meinfo_cb.h"
#include "util.h"
#include "hwresource.h"
#include "omciutil_misc.h"
#include "cfm_pkt.h"

//classid 65303 9.99.39 OamDmMepStats

static int 
meinfo_cb_65303_get(struct me_t *me, unsigned char attr_mask[2])
{
	unsigned char md_level = 0xFF, tp_type = 0;
	unsigned short meg_id = 0, mep_id = 0, meid = 0;
	cfm_config_t *cfm_config = NULL;
	struct me_t *parent_me = NULL, *meptr = NULL, *bridgeport_me = NULL, *ibridgeport_me = NULL;
	int port_type = -1, port_idx = -1;
	
	// Reset all attributes first in case CFM can't be found and leave previous records
	if (util_attr_mask_get_bit(attr_mask, 1)) { // MinRTDDelay
		struct attr_value_t attr;
		attr.data = 0;
		meinfo_me_attr_set(me, 1, &attr, 0);
	}
	if (util_attr_mask_get_bit(attr_mask, 2)) { // MaxRTDDelay
		struct attr_value_t attr;
		attr.data = 0;
		meinfo_me_attr_set(me, 2, &attr, 0);
	}
	if (util_attr_mask_get_bit(attr_mask, 3)) { // AvgRTDDelay
		struct attr_value_t attr;
		attr.data = 0;
		meinfo_me_attr_set(me, 3, &attr, 0);
	}
	if (util_attr_mask_get_bit(attr_mask, 4)) { // MinRTDVar
		struct attr_value_t attr;
		attr.data = 0;
		meinfo_me_attr_set(me, 4, &attr, 0);
	}
	if (util_attr_mask_get_bit(attr_mask, 5)) { // MaxRTDVar
		struct attr_value_t attr;
		attr.data = 0;
		meinfo_me_attr_set(me, 5, &attr, 0);
	}
	if (util_attr_mask_get_bit(attr_mask, 6)) { // AvgRTDVar
		struct attr_value_t attr;
		attr.data = 0;
		meinfo_me_attr_set(me, 6, &attr, 0);
	}
	if (util_attr_mask_get_bit(attr_mask, 7)) { // MinFEPDelay
		struct attr_value_t attr;
		attr.data = 0;
		meinfo_me_attr_set(me, 7, &attr, 0);
	}
	if (util_attr_mask_get_bit(attr_mask, 8)) { // MaxFEPDelay
		struct attr_value_t attr;
		attr.data = 0;
		meinfo_me_attr_set(me, 8, &attr, 0);
	}
	if (util_attr_mask_get_bit(attr_mask, 9)) { // AvgFEPDelay
		struct attr_value_t attr;
		attr.data = 0;
		meinfo_me_attr_set(me, 9, &attr, 0);
	}
	if (util_attr_mask_get_bit(attr_mask, 10)) { // RemoteMep
		struct attr_value_t attr;
		attr.data = 0;
		meinfo_me_attr_set(me, 10, &attr, 0);
	}
	if (util_attr_mask_get_bit(attr_mask, 11)) { // RemoteMac
		struct attr_value_t attr;
		attr.ptr= malloc_safe(IFHWADDRLEN);
		memset(attr.ptr, 0, IFHWADDRLEN);
		meinfo_me_attr_set(me, 11, &attr, 0);
		if (attr.ptr) free_safe(attr.ptr);
	}
	if (util_attr_mask_get_bit(attr_mask, 12)) { // ClearCounters
		struct attr_value_t attr;
		attr.data = 1;
		meinfo_me_attr_set(me, 12, &attr, 0);
	}
	
	if (omci_env_g.cfm_enable == 0) {
       		dbprintf(LOG_INFO,"cfm_enable==0\n");
		return 0;
	}
	
	parent_me = meinfo_me_get_by_meid(65289, me->meid);
	if (!parent_me) {
		dbprintf(LOG_ERR, "unable to find parent me, classid=%u, meid=%u\n", 65289, me->meid);	
		return -1;
	} else {
		meg_id = meinfo_util_me_attr_data(parent_me, 1);
		mep_id = meinfo_util_me_attr_data(parent_me, 2);
		tp_type = meinfo_util_me_attr_data(parent_me, 4);
		meid = meinfo_util_me_attr_data(parent_me, 5);
	}
	
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
			return -1;
	}

	bridgeport_me = omciutil_misc_find_me_related_bport(meptr);
	ibridgeport_me = hwresource_public2private(bridgeport_me);

	if(ibridgeport_me) {
		struct me_t *pm_me = meinfo_me_get_by_meid(65294, me->meid);
		unsigned char mac[IFHWADDRLEN];
		int i;
		port_type = meinfo_util_me_attr_data(ibridgeport_me, 4);
		port_idx = meinfo_util_me_attr_data(ibridgeport_me, 5);

		if (port_type < 0 || port_idx < 0 || !pm_me)
			return -1;

		for (i=0; i<IFHWADDRLEN; i++)
			mac[i] = util_bitmap_get_value(meinfo_util_me_attr_ptr(pm_me, 2), 6*8, i*8, 8);

		if((cfm_config = cfm_mp_find(md_level, mep_id, port_type, port_idx)) != NULL) {
			if (!cfm_config->admin_state) {
				if (util_attr_mask_get_bit(attr_mask, 1)) { // MinRTDDelay
					struct attr_value_t attr;
					attr.data = (unsigned int) cfm_config->round_trip_min;
					meinfo_me_attr_set(me, 1, &attr, 0);
				}
				if (util_attr_mask_get_bit(attr_mask, 2)) { // MaxRTDDelay
					struct attr_value_t attr;
					attr.data = (unsigned int) cfm_config->round_trip_max;
					meinfo_me_attr_set(me, 2, &attr, 0);
				}
				if (util_attr_mask_get_bit(attr_mask, 3)) { // AvgRTDDelay
					struct attr_value_t attr;
					attr.data = (unsigned int) cfm_config->round_trip_avg;
					meinfo_me_attr_set(me, 3, &attr, 0);
				}
				if (util_attr_mask_get_bit(attr_mask, 4)) { // MinRTDVar
					struct attr_value_t attr;
					attr.data = (unsigned int) cfm_config->frame_delay_min;
					meinfo_me_attr_set(me, 4, &attr, 0);
				}
				if (util_attr_mask_get_bit(attr_mask, 5)) { // MaxRTDVar
					struct attr_value_t attr;
					attr.data = (unsigned int) cfm_config->frame_delay_max;
					meinfo_me_attr_set(me, 5, &attr, 0);
				}
				if (util_attr_mask_get_bit(attr_mask, 6)) { // AvgRTDVar
					struct attr_value_t attr;
					attr.data = (unsigned int) cfm_config->frame_delay_avg;
					meinfo_me_attr_set(me, 6, &attr, 0);
				}
				if (util_attr_mask_get_bit(attr_mask, 7)) { // MinFEPDelay
					struct attr_value_t attr;
					attr.data = (unsigned int) cfm_config->response_time_min;
					meinfo_me_attr_set(me, 7, &attr, 0);
				}
				if (util_attr_mask_get_bit(attr_mask, 8)) { // MaxFEPDelay
					struct attr_value_t attr;
					attr.data = (unsigned int) cfm_config->response_time_max;
					meinfo_me_attr_set(me, 8, &attr, 0);
				}
				if (util_attr_mask_get_bit(attr_mask, 9)) { // AvgFEPDelay
					struct attr_value_t attr;
					attr.data = (unsigned int) cfm_config->response_time_avg;
					meinfo_me_attr_set(me, 9, &attr, 0);
				}
				if (util_attr_mask_get_bit(attr_mask, 10)) { // RemoteMep
					struct attr_value_t attr;
					cfm_pkt_rmep_entry_t *entry = NULL;
					list_for_each_entry(entry, &cfm_config->cfm_recv_rmep_list, node) {
						if(memcmp(entry->rmep_mac, mac, IFHWADDRLEN)==0) {
							attr.data = (unsigned short) entry->rmep_id;
							meinfo_me_attr_set(me, 10, &attr, 0);
							break;
						}
					}
				}
				if (util_attr_mask_get_bit(attr_mask, 11)) { // RemoteMac
					struct attr_value_t attr;
					attr.ptr= malloc_safe(IFHWADDRLEN);
					memcpy(attr.ptr, mac, sizeof(mac));
					meinfo_me_attr_set(me, 11, &attr, 0);
					if (attr.ptr) free_safe(attr.ptr);
				}
				if (util_attr_mask_get_bit(attr_mask, 12)) { // ClearCounters
					struct attr_value_t attr;
					attr.data = 1;
					meinfo_me_attr_set(me, 12, &attr, 0);
				}
			}
		}
	}
	
	return 0;
}

struct meinfo_callback_t meinfo_cb_calix_65303 = {
	.get_cb		= meinfo_cb_65303_get,
};
