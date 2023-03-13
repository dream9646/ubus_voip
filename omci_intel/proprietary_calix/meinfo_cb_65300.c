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
 * File    : meinfo_cb_65300.c
 *
 ******************************************************************/

#include <string.h>
#include "meinfo_cb.h"
#include "util.h"
#include "hwresource.h"
#include "omciutil_misc.h"
#include "cfm_pkt.h"

//classid 65300 9.99.36 OamMepStatsExt

static int 
meinfo_cb_65300_get(struct me_t *me, unsigned char attr_mask[2])
{
	unsigned char md_level = 0xFF, tp_type = 0;
	unsigned short meg_id = 0, mep_id = 0, meid = 0;
	cfm_config_t *cfm_config = NULL;
	struct me_t *parent_me = NULL, *meptr = NULL, *bridgeport_me = NULL, *ibridgeport_me = NULL;
	int port_type = -1, port_idx = -1;
	
	// Reset all attributes first in case CFM can't be found and leave previous records
	if (util_attr_mask_get_bit(attr_mask, 1)) { // LTMsgsRecv
		struct attr_value_t attr;
		attr.data = 0;
		meinfo_me_attr_set(me, 1, &attr, 0);
	}
	if (util_attr_mask_get_bit(attr_mask, 2)) { // LTMsgsSent
		struct attr_value_t attr;
		attr.data = 0;
		meinfo_me_attr_set(me, 2, &attr, 0);
	}
	if (util_attr_mask_get_bit(attr_mask, 4)) { // LTRespRecv
		struct attr_value_t attr;
		attr.data = 0;
		meinfo_me_attr_set(me, 4, &attr, 0);
	}
	if (util_attr_mask_get_bit(attr_mask, 5)) { // LTRespSent
		struct attr_value_t attr;
		attr.data = 0;
		meinfo_me_attr_set(me, 5, &attr, 0);
	}
	if (util_attr_mask_get_bit(attr_mask, 6)) { // LTRespRxUnexp
		struct attr_value_t attr;
		attr.data = 0;
		meinfo_me_attr_set(me, 6, &attr, 0);
	}
	if (util_attr_mask_get_bit(attr_mask, 7)) { // LMMsgsRecv
		struct attr_value_t attr;
		attr.data = 0;
		meinfo_me_attr_set(me, 7, &attr, 0);
	}
	if (util_attr_mask_get_bit(attr_mask, 8)) { // LMMsgsSent
		struct attr_value_t attr;
		attr.data = 0;
		meinfo_me_attr_set(me, 8, &attr, 0);
	}
	if (util_attr_mask_get_bit(attr_mask, 9)) { // LMRespRx
		struct attr_value_t attr;
		attr.data = 0;
		meinfo_me_attr_set(me, 9, &attr, 0);
	}
	if (util_attr_mask_get_bit(attr_mask, 10)) { // LMRespSent
		struct attr_value_t attr;
		attr.data = 0;
		meinfo_me_attr_set(me, 10, &attr, 0);
	}
	if (util_attr_mask_get_bit(attr_mask, 11)) { // DMRespRx
		struct attr_value_t attr;
		attr.data = 0;
		meinfo_me_attr_set(me, 11, &attr, 0);
	}
	if (util_attr_mask_get_bit(attr_mask, 12)) { // DMRespSent
		struct attr_value_t attr;
		attr.data = 0;
		meinfo_me_attr_set(me, 12, &attr, 0);
	}
	if (util_attr_mask_get_bit(attr_mask, 13)) { // SenderIdInvalid
		struct attr_value_t attr;
		attr.data = 0;
		meinfo_me_attr_set(me, 13, &attr, 0);
	}
	if (util_attr_mask_get_bit(attr_mask, 14)) { // PortStatusInvalid
		struct attr_value_t attr;
		attr.data = 0;
		meinfo_me_attr_set(me, 14, &attr, 0);
	}
	if (util_attr_mask_get_bit(attr_mask, 15)) { // IntfStatusInvalid
		struct attr_value_t attr;
		attr.data = 0;
		meinfo_me_attr_set(me, 15, &attr, 0);
	}
	if (util_attr_mask_get_bit(attr_mask, 16)) { // SeqErrors
		struct attr_value_t attr;
		attr.data = 0;
		meinfo_me_attr_set(me, 16, &attr, 0);
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
				if (util_attr_mask_get_bit(attr_mask, 1)) { // LTMsgsRecv
					struct attr_value_t attr;
					attr.data = cfm_config->cfm_pkt_pm[CFM_PDU_LTM].rx;
					meinfo_me_attr_set(me, 1, &attr, 0);
				}
				if (util_attr_mask_get_bit(attr_mask, 2)) { // LTMsgsSent
					struct attr_value_t attr;
					attr.data = cfm_config->cfm_pkt_pm[CFM_PDU_LTM].tx;
					meinfo_me_attr_set(me, 2, &attr, 0);
				}
				if (util_attr_mask_get_bit(attr_mask, 4)) { // LTRespRecv
					struct attr_value_t attr;
					attr.data = cfm_config->cfm_pkt_pm[CFM_PDU_LTR].rx;
					meinfo_me_attr_set(me, 4, &attr, 0);
				}
				if (util_attr_mask_get_bit(attr_mask, 5)) { // LTRespSent
					struct attr_value_t attr;
					attr.data = cfm_config->cfm_pkt_pm[CFM_PDU_LTR].tx;
					meinfo_me_attr_set(me, 5, &attr, 0);
				}
				if (util_attr_mask_get_bit(attr_mask, 6)) { // LTRespRxUnexp
					struct attr_value_t attr;
					attr.data = cfm_config->unexpected_ltr_count;
					meinfo_me_attr_set(me, 6, &attr, 0);
				}
				if (util_attr_mask_get_bit(attr_mask, 7)) { // LMMsgsRecv
					struct attr_value_t attr;
					attr.data = cfm_config->cfm_pkt_pm[CFM_PDU_LMM].rx;
					meinfo_me_attr_set(me, 7, &attr, 0);
				}
				if (util_attr_mask_get_bit(attr_mask, 8)) { // LMMsgsSent
					struct attr_value_t attr;
					attr.data = cfm_config->cfm_pkt_pm[CFM_PDU_LMM].tx;
					meinfo_me_attr_set(me, 8, &attr, 0);
				}
				if (util_attr_mask_get_bit(attr_mask, 9)) { // LMRespRx
					struct attr_value_t attr;
					attr.data = cfm_config->cfm_pkt_pm[CFM_PDU_LMR].rx;
					meinfo_me_attr_set(me, 9, &attr, 0);
				}
				if (util_attr_mask_get_bit(attr_mask, 10)) { // LMRespSent
					struct attr_value_t attr;
					attr.data = cfm_config->cfm_pkt_pm[CFM_PDU_LMR].tx;
					meinfo_me_attr_set(me, 10, &attr, 0);
				}
				if (util_attr_mask_get_bit(attr_mask, 11)) { // DMRespRx
					struct attr_value_t attr;
					attr.data = cfm_config->cfm_pkt_pm[CFM_PDU_DMR].rx;
					meinfo_me_attr_set(me, 11, &attr, 0);
				}
				if (util_attr_mask_get_bit(attr_mask, 12)) { // DMRespSent
					struct attr_value_t attr;
					attr.data = cfm_config->cfm_pkt_pm[CFM_PDU_DMR].tx;
					meinfo_me_attr_set(me, 12, &attr, 0);
				}
				if (util_attr_mask_get_bit(attr_mask, 13)) { // SenderIdInvalid
					struct attr_value_t attr;
					attr.data = cfm_config->invalid_senderid_ccm_count;
					meinfo_me_attr_set(me, 13, &attr, 0);
				}
				if (util_attr_mask_get_bit(attr_mask, 14)) { // PortStatusInvalid
					struct attr_value_t attr;
					attr.data = cfm_config->invalid_port_status_ccm_count;
					meinfo_me_attr_set(me, 14, &attr, 0);
				}
				if (util_attr_mask_get_bit(attr_mask, 15)) { // IntfStatusInvalid
					struct attr_value_t attr;
					attr.data = cfm_config->invalid_interface_status_ccm_count;
					meinfo_me_attr_set(me, 15, &attr, 0);
				}
				if (util_attr_mask_get_bit(attr_mask, 16)) { // SeqErrors
					struct attr_value_t attr;
					attr.data = cfm_config->out_of_seqence_ccm_count;
					meinfo_me_attr_set(me, 16, &attr, 0);
				}
			}
		}
	}
	
	return 0;
}

struct meinfo_callback_t meinfo_cb_calix_65300 = {
	.get_cb		= meinfo_cb_65300_get,
};
