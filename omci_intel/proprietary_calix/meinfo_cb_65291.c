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
 * File    : meinfo_cb_65291.c
 *
 ******************************************************************/

#include <string.h>
#include "meinfo_cb.h"
#include "util.h"
#include "hwresource.h"
#include "omciutil_misc.h"
#include "cfm_pkt.h"

//classid 65291 9.99.29 OamLinkTrace

static int 
meinfo_cb_65291_get(struct me_t *me, unsigned char attr_mask[2])
{
	unsigned char md_level = 0xFF, tp_type = 0;
	unsigned short meg_id = 0, mep_id = 0, meid = 0;
	cfm_config_t *cfm_config = NULL;
	struct me_t *parent_me = NULL, *meptr = NULL, *bridgeport_me = NULL, *ibridgeport_me = NULL;
	int port_type = -1, port_idx = -1;
	
	// Reset all attributes first in case CFM can't be found and leave previous records
	if (util_attr_mask_get_bit(attr_mask, 5)) { // TransId
		struct attr_value_t attr;
		attr.data = 0;
		meinfo_me_attr_set(me, 5, &attr, 0);
	}
	if (util_attr_mask_get_bit(attr_mask, 6)) { // ResultTransId
		struct attr_value_t attr;
		attr.data = 0;
		meinfo_me_attr_set(me, 6, &attr, 0);
	}
	if (util_attr_mask_get_bit(attr_mask, 7)) { // ResultStart
		struct attr_value_t attr;
		attr.data = 0;
		meinfo_me_attr_set(me, 7, &attr, 0);
	}
	if (util_attr_mask_get_bit(attr_mask, 8)) { // ResultTSize
		struct attr_value_t attr;
		attr.data = 0;
		meinfo_me_attr_set(me, 8, &attr, 0);
	}
	if (util_attr_mask_get_bit(attr_mask, 9)) { // ResultTable
		struct attr_table_header_t *tab_header = (struct attr_table_header_t *)meinfo_util_me_attr_ptr(me, 9);
		meinfo_me_attr_release_table_all_entries(tab_header);
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
		port_type = meinfo_util_me_attr_data(ibridgeport_me, 4);
		port_idx = meinfo_util_me_attr_data(ibridgeport_me, 5);

		if (port_type < 0 || port_idx < 0)
			return -1;

		if((cfm_config = cfm_mp_find(md_level, mep_id, port_type, port_idx)) != NULL) {
			if (!cfm_config->admin_state) {
				if (util_attr_mask_get_bit(attr_mask, 5)) { // TransId
					struct attr_value_t attr;
					attr.data = cfm_config->nextLTMtransID;
					meinfo_me_attr_set(me, 5, &attr, 0);
				}
				if (util_attr_mask_get_bit(attr_mask, 6)) { // ResultTransId
					struct attr_value_t attr;
					attr.data = cfm_config->nextLTMtransID;
					meinfo_me_attr_set(me, 6, &attr, 0);
				}
				if (util_attr_mask_get_bit(attr_mask, 7)) { // ResultStart
					struct attr_value_t attr;
					cfm_pkt_ltr_entry_t *entry = NULL;
					attr.data = 0;
					list_for_each_entry(entry, &cfm_config->cfm_recv_ltr_list, node) {
						attr.data = (unsigned int) (entry->ltr_timestamp/1000000);
						break;
					}
					meinfo_me_attr_set(me, 7, &attr, 0);
				}
				if (util_attr_mask_get_bit(attr_mask, 9)) { // ResultTable
					struct attr_table_header_t *tab_header = (struct attr_table_header_t *)meinfo_util_me_attr_ptr(me, 9);
					void *tab_entry = malloc_safe(8);
					cfm_pkt_ltr_entry_t *entry = NULL;
					meinfo_me_attr_release_table_all_entries(tab_header);
					list_for_each_entry(entry, &cfm_config->cfm_recv_ltr_list, node) {
						int i;
						memset(tab_entry, 0, 8);
						util_bitmap_set_value(tab_entry, 8*8, 0, 8, entry->u.ltr.ltr_pdu.ttl);
						for (i=0 ;i<IFHWADDRLEN ;i++)   // macaddr
							util_bitmap_set_value(tab_entry, 8*8, 8+i*8, 8, entry->ltr_mac[i]);
						if(entry->u.ltr.ltr_hdr.flags & 0x20) // TerminalMEP (bit5)
							util_bitmap_set_value(tab_entry, 8*8, 56, 8, 2);
						else
							util_bitmap_set_value(tab_entry, 8*8, 56, 8, 1);
						meinfo_me_attr_set_table_entry(me, 9, tab_entry, 0);
					}
					free_safe(tab_entry);
				}
				if (util_attr_mask_get_bit(attr_mask, 8)) { // ResultTSize
					struct attr_value_t attr;
					attr.data = meinfo_me_attr_get_table_entry_count(me, 9);
					meinfo_me_attr_set(me, 8, &attr, 0);
				}
				if (util_attr_mask_get_bit(attr_mask, 11)) { // HistoryTable
					void *tab_entry = malloc_safe(16);
					cfm_pkt_ltr_entry_t *entry = NULL;
					list_for_each_entry(entry, &cfm_config->cfm_recv_ltr_list, node) {
						cfm_pkt_rmep_entry_t *entry2 = NULL;
						struct get_attr_table_context_t table_context;
						int is_duplicated = 0;
						if (meinfo_me_attr_get_table_entry(me, 11, &table_context, tab_entry) == MEINFO_RW_OK) {
							do {
								unsigned int xid = (unsigned int)util_bitmap_get_value(tab_entry, 16*8, 0, 32);
								if(entry->u.ltr.ltr_pdu.xid == xid) {
									is_duplicated = 1;
									break;
								}
							} while (meinfo_me_attr_get_table_entry_next(me, 11, &table_context, tab_entry) == MEINFO_RW_OK);
							if(is_duplicated) continue;
						}
						list_for_each_entry(entry2, &cfm_config->cfm_recv_rmep_list, node) {
							if(memcmp(entry->ltr_mac, entry2->rmep_mac, IFHWADDRLEN)==0) {
								int i;
								memset(tab_entry, 0, 16);
								util_bitmap_set_value(tab_entry, 16*8, 0, 32, htons(entry->u.ltr.ltr_pdu.xid));
								util_bitmap_set_value(tab_entry, 16*8, 32, 16, htons(entry2->rmep_id));
								for (i=0 ;i<IFHWADDRLEN ;i++)   // macaddr
									util_bitmap_set_value(tab_entry, 16*8, 48+i*8, 8, entry->ltr_mac[i]);
								util_bitmap_set_value(tab_entry, 16*8, 96, 32, htonl((unsigned int) (entry->ltr_timestamp/1000000)));
								meinfo_me_attr_set_table_entry(me, 11, tab_entry, 0);
								break;
							}
						}
					}
					free_safe(tab_entry);
					if(meinfo_me_attr_get_table_entry_count(me, 11) > 16) { // olt supports upto 16 entries
						struct attr_table_header_t *tab_header = (struct attr_table_header_t *)meinfo_util_me_attr_ptr(me, 11);
						meinfo_me_attr_release_table_all_entries(tab_header);
					}
				}
				if (util_attr_mask_get_bit(attr_mask, 10)) { // HistoryTSize
					struct attr_value_t attr;
					attr.data = meinfo_me_attr_get_table_entry_count(me, 11);
					meinfo_me_attr_set(me, 10, &attr, 0);
				}
			}
		}
	}
	
	return 0;
}

struct meinfo_callback_t meinfo_cb_calix_65291 = {
	.get_cb		= meinfo_cb_65291_get,
};
