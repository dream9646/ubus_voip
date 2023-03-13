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
 * Module  : cfm
 * File    : cfm_recv.c
 *
 ******************************************************************/

// Criteria of processing MD Level		MEP	MIP
// -------------------------------------------	------- -------
// cfm_hdr-> md_level >  cfm_config->md_level  	Pass	Pass
// cfm_hdr-> md_level == cfm_config->md_level  	handle	Pass
// cfm_hdr-> md_level <  cfm_config->md_level  	drop	drop
//
// check the following functions for detail
// 	valid_mep_direction_for_wan_recv()
// 	valid_mep_direction_for_lan_recv()
// 	cfm_recv_is_match_cfm_config()
// 	cfm_recv_get_cfm_config()
// 	cfm_recv_pkt_process()

#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <math.h>

#include "fwk_msgq.h"
#include "fwk_timer.h"
#include "list.h"
#include "util.h"
#include "switch.h"
#include "prbs.h"
#include "crc.h"
#include "cpuport.h"
#include "cpuport_pkt.h"
#include "cpuport_util.h"
#include "cpuport_util_send.h"
#include "cfm_pkt.h"
#include "cfm_mp.h"
#include "cfm_util.h"
#include "cfm_switch.h"
#include "cfm_bport.h"
#include "cfm_print.h"
#include "cfm_recv.h"
#include "cfm_send.h"
#include "cfm_core.h"

extern int cfm_timer_qid_g;

static int
cfm_calculate_ltr_length(unsigned char *frame_ptr, int vlan_num)
{
	cfm_hdr_t *cfm_hdr = (cfm_hdr_t *) (frame_ptr + IFHWADDRLEN*2 + vlan_num*VLAN_TAG_OFFSET + ETHER_TYPE_LEN);
	unsigned char *ltr_ptr = ((unsigned char *) cfm_hdr);
	unsigned short tlv_length = 0, ltr_length = sizeof(cfm_hdr_t) + cfm_hdr->first_tlv_offset;

	do {
		if(ltr_ptr[ltr_length] != 0) { // End TLV
			tlv_length = (ltr_ptr[ltr_length+1] << 4 | ltr_ptr[ltr_length+2]);
			if(tlv_length>32) break; // Ignore BAD TLV (assume TLV length is <32 bytes)
			ltr_length += sizeof(unsigned char) + sizeof(unsigned short) + tlv_length; // TLV type, TLV length, and actual length
		}
	} while(ltr_ptr[ltr_length] != 0); // End TLV
	ltr_length += sizeof(unsigned char); // Length of End TLV

	return ltr_length;
}

static int
cfm_calculate_lbr_length(unsigned char *frame_ptr, int vlan_num)
{
	cfm_hdr_t *cfm_hdr = (cfm_hdr_t *) (frame_ptr + IFHWADDRLEN*2 + vlan_num*VLAN_TAG_OFFSET + ETHER_TYPE_LEN);
	unsigned char *lbr_ptr = ((unsigned char *) cfm_hdr);
	unsigned short tlv_length = 0, lbr_length = sizeof(cfm_hdr_t) + cfm_hdr->first_tlv_offset;

	do {
		if(lbr_ptr[lbr_length] != 0) { // End TLV
			tlv_length = (lbr_ptr[lbr_length+1] << 4 | lbr_ptr[lbr_length+2]);
			if(tlv_length>32) break; // Ignore BAD TLV (assume TLV length is <32 bytes)
			lbr_length += sizeof(unsigned char) + sizeof(unsigned short) + tlv_length; // TLV type, TLV length, and actual length
		}
	} while(lbr_ptr[lbr_length] != 0); // End TLV
	lbr_length += sizeof(unsigned char); // Length of End TLV

	return lbr_length;
}

cfm_pkt_lbr_entry_t *
cfm_recv_lbr(cfm_config_t *cfm_config, cfm_pktinfo_t *cfm_pktinfo)
{
	cfm_hdr_t *cfm_hdr = (cfm_hdr_t *) (cfm_pktinfo->frame_ptr + IFHWADDRLEN*2 + cfm_pktinfo->vlan_num*VLAN_TAG_OFFSET + ETHER_TYPE_LEN);
	cfm_pdu_lb_t *pdu_lbr = (cfm_pdu_lb_t *) ((unsigned char *) cfm_pktinfo->cfm_hdr + sizeof(cfm_hdr_t));

	unsigned char *lbr_ptr = ((unsigned char *) cfm_hdr);
	unsigned short lbr_length = cfm_calculate_lbr_length(cfm_pktinfo->frame_ptr, cfm_pktinfo->vlan_num);

	unsigned int xid = ntohl(pdu_lbr->xid);
	unsigned long long now = cfm_util_get_time_in_us();

	unsigned long long resptime;
	cfm_pkt_lbr_entry_t *entry;
	int is_found = 0, count = 0;

	if (xid != cfm_config->nextLBMtransID) {	// wrong transaction id
		dbprintf_cfm(LOG_ERR, "Drop LBR, mdl=%d, xid=0x%x, from %s logical port %d %s (xid mismatch 0x%x)\n",
			cfm_pktinfo->cfm_hdr->md_level, xid,
			util_str_macaddr(cfm_pktinfo->src_mac_ptr),
			cfm_pktinfo->rx_desc.logical_port_id,
			cfm_print_get_vlan_str(cfm_pktinfo),
			cfm_config->nextLBMtransID);
		return NULL;
	}
	if (lbr_length > LBR_PAYLOAD_SIZE_MAX) {	// pkt too long?
		dbprintf_cfm(LOG_ERR, "Drop LBR, mdl=%d, xid=0x%x, from %s logical port %d %s (lbr %d too long?)\n",
			cfm_pktinfo->cfm_hdr->md_level, xid,
			util_str_macaddr(cfm_pktinfo->src_mac_ptr),
			cfm_pktinfo->rx_desc.logical_port_id,
			 cfm_print_get_vlan_str(cfm_pktinfo),
			lbr_length);
		return NULL;
	}

	list_for_each_entry(entry, &cfm_config->cfm_recv_lbr_list, node) {
		count++;
		if (memcmp(&(entry->lbr_mac), cfm_pktinfo->src_mac_ptr, IFHWADDRLEN) == 0) {
			is_found = 1;
			break;
		}
	}

	resptime = now - cfm_config->cfm_send_lbm_timestamp;

	util_dbprintf(omci_env_g.debug_level_cfm, LOG_ERR, 0, "%d bytes LBR from %s: xid=0x%x, time=%llu.%03llums%s\n",
		cfm_pktinfo->frame_len,
		util_str_macaddr(cfm_pktinfo->src_mac_ptr),
		xid, resptime/1000, resptime%1000, is_found?"(duplicated)":"");

	if (is_found) {
		return NULL;	// duplicated LBR from same rmep mac for same xid
	}

	if (count > CFM_MAX_TTL) cfm_mp_flush_lbr_list(cfm_config);
	entry = malloc_safe(sizeof(cfm_pkt_lbr_entry_t));
	if (entry != NULL) {
		entry->lbr_length = lbr_length;
		entry->lbr_timestamp = now;
		memcpy(&(entry->lbr_mac), cfm_pktinfo->src_mac_ptr, IFHWADDRLEN);
		memcpy(&(entry->u.lbr_payload), lbr_ptr, lbr_length);
		list_add_tail(&entry->node, &cfm_config->cfm_recv_lbr_list);
	}
	return entry;
}

cfm_pkt_ltr_entry_t *
cfm_recv_ltr(cfm_config_t *cfm_config, cfm_pktinfo_t *cfm_pktinfo)
{
	cfm_hdr_t *cfm_hdr = (cfm_hdr_t *) (cfm_pktinfo->frame_ptr + IFHWADDRLEN*2 + cfm_pktinfo->vlan_num*VLAN_TAG_OFFSET + ETHER_TYPE_LEN);
	cfm_pdu_ltr_t *pdu_ltr = (cfm_pdu_ltr_t *) ((unsigned char *) cfm_pktinfo->cfm_hdr + sizeof(cfm_hdr_t));

	unsigned char *ltr_ptr = ((unsigned char *) cfm_hdr);
	unsigned short ltr_length = cfm_calculate_ltr_length(cfm_pktinfo->frame_ptr, cfm_pktinfo->vlan_num);

	unsigned int xid = ntohl(pdu_ltr->xid);
	unsigned long long now = cfm_util_get_time_in_us();
	unsigned int resptime;
	cfm_pkt_ltr_entry_t *entry;
	int is_found = 0, count = 0;

	if (xid != cfm_config->nextLTMtransID) {	// wrong transaction id
		cfm_config->unexpected_ltr_count++;
		dbprintf_cfm(LOG_ERR, "Drop LTR, mdl=%d, xid=0x%x, ttl=%u, from %s logical port %d %s (xid mismatch 0x%x)\n",
			cfm_pktinfo->cfm_hdr->md_level, xid, pdu_ltr->ttl,
			util_str_macaddr(cfm_pktinfo->src_mac_ptr),
			cfm_pktinfo->rx_desc.logical_port_id,
			cfm_print_get_vlan_str(cfm_pktinfo),
			cfm_config->nextLTMtransID);
		return NULL;
	}
	if (ltr_length > LTR_PAYLOAD_SIZE_MAX) {	// pkt too long?
		cfm_config->unexpected_ltr_count++;
		dbprintf_cfm(LOG_ERR, "Drop LTR, mdl=%d, xid=0x%x, ttl=%u, from %s logical port %d %s (ltr %d too long?)\n",
			cfm_pktinfo->cfm_hdr->md_level, xid, pdu_ltr->ttl,
			util_str_macaddr(cfm_pktinfo->src_mac_ptr),
			cfm_pktinfo->rx_desc.logical_port_id,
			cfm_print_get_vlan_str(cfm_pktinfo),
			ltr_length);
		return NULL;
	}

	list_for_each_entry(entry, &cfm_config->cfm_recv_ltr_list, node) {
		count++;
		if (memcmp(&(entry->ltr_mac), cfm_pktinfo->src_mac_ptr, IFHWADDRLEN) == 0) {
			is_found = 1;
			break;
		}
	}

	now = cfm_util_get_time_in_us();
	resptime = now - cfm_config->cfm_send_ltm_timestamp;
	util_dbprintf(omci_env_g.debug_level_cfm, LOG_ERR, 0, "%d bytes LTR from %s: xid=0x%x, ttl=%u, time=%u.%03ums%s\n",
		cfm_pktinfo->frame_len,
		util_str_macaddr(cfm_pktinfo->src_mac_ptr),
		xid, pdu_ltr->ttl, resptime/1000, resptime%1000,
		is_found?"(duplicated)":"");

	if (is_found) {
		cfm_config->unexpected_ltr_count++;
		return NULL;	// duplicated LTR from same rmep mac for same xid
	}

	if (count > CFM_MAX_TTL) cfm_mp_flush_ltr_list(cfm_config);
	entry = malloc_safe(sizeof(cfm_pkt_ltr_entry_t));
	if (entry != NULL) {
		entry->ltr_length = ltr_length;
		entry->ltr_timestamp = now;
		memcpy(&(entry->ltr_mac), cfm_pktinfo->src_mac_ptr, IFHWADDRLEN);
		memcpy(&(entry->u.ltr_payload), ltr_ptr, ltr_length);
		list_add_tail(&entry->node, &cfm_config->cfm_recv_ltr_list);
	}
	return entry;
}

static int
cfm_recv_1sl(cfm_config_t *cfm_config, cfm_pktinfo_t *cfm_pktinfo)
{
	int is_found = 0;
	//cfm_hdr_t *cfm_hdr = (cfm_hdr_t *) (cfm_pktinfo->frame_ptr + IFHWADDRLEN*2 + cfm_pktinfo->vlan_num*VLAN_TAG_OFFSET + ETHER_TYPE_LEN);
	cfm_pdu_sl_t *ptr_1sl = (cfm_pdu_sl_t *) (cfm_pktinfo->frame_ptr + IFHWADDRLEN*2 + cfm_pktinfo->vlan_num*VLAN_TAG_OFFSET + ETHER_TYPE_LEN + sizeof(cfm_hdr_t));
	cfm_pkt_rmep_entry_t *entry;

	list_for_each_entry(entry, &cfm_config->cfm_recv_rmep_list, node) {
		if (entry->rmep_id == htons(ptr_1sl->source_mep_id) &&
		    memcmp(&entry->rmep_mac, cfm_pktinfo->src_mac_ptr, IFHWADDRLEN) == 0) {
		    	is_found = 1;
		    	break;
		}
	}
	if (!is_found)
		return 0;

	// stop or restart another 1SL loss measurement
	// when test id field in 1SL frame change to 0 or another test id
	entry->RxFCl_1sl++;
	entry->TxFCf_1sl = ptr_1sl->TxFCf;
	if ( ntohl( ptr_1sl->test_id) != entry->test_id_1sl ) {
		entry->test_id_1sl = ntohl( ptr_1sl->test_id );
		if (entry->eth_slm_dual_going_on == 0) {
			entry->TxFCf_p = ptr_1sl->TxFCf;
			entry->RxFCl_p = entry->RxFCl_1sl;
			entry->TxFCf_1sl_stop_check = entry->TxFCf_1sl;
			entry->eth_slm_dual_recv_timer =
				fwk_timer_create(cfm_timer_qid_g, CFM_TIMER_EVENT_ETH_SLM_DUAL_STOP_CHECK, 10000 , (void *)cfm_config);
			entry->eth_slm_dual_going_on = 1;
			memset(&(entry->eth_slm_dual_start_time), 0, sizeof(time_t));
			memset(&(entry->eth_slm_dual_stop_time), 0, sizeof(time_t));
			memset(&(entry->eth_slm_dual_fratio), 0, sizeof(unsigned int));
			memset(&(entry->eth_slm_dual_nratio), 0, sizeof(unsigned int));
			time(&entry->eth_slm_dual_start_time);
			entry->eth_slm_dual_expect_time = 0xffffffff;

		}else{
			entry->TxFCf_c = ptr_1sl->TxFCf;
			entry->RxFCl_c = entry->RxFCl_1sl;

			entry->eth_slm_dual_tx_nloss = entry->TxFCf_c - entry->TxFCf_p;
			entry->eth_slm_dual_rx_nloss = entry->RxFCl_c - entry->RxFCl_p;

			if (entry->eth_slm_dual_recv_timer > 0) {
				fwk_timer_delete(entry->eth_slm_dual_recv_timer );
				entry->eth_slm_dual_recv_timer = -1;
			}

			dbprintf_cfm(LOG_WARNING,"nearend tx=TxFCf_c(%d)-TxFCf_p(%d), rx=RxFCl_c(%d)-RxFCl_p(%d)\n",
				entry->TxFCf_c,	entry->TxFCf_p, entry->RxFCl_c, entry->RxFCl_p);
			{
				int nloss = entry->eth_slm_dual_tx_nloss - entry->eth_slm_dual_rx_nloss;

				if (nloss > 0 && entry->eth_slm_dual_tx_nloss > 0)
					entry->eth_slm_dual_nratio = nloss*10000/entry->eth_slm_dual_tx_nloss;
				util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0,
					"\nSynthetic Loss (dual):\n"
					"md_level %d, mep %d, rmep %d: testid %u\n"
					"loss nearend=%d (tx=%u rx=%u ratio=%u.%02u%%)\n",
					cfm_config->md_level, cfm_config->mep_id, entry->rmep_id, entry->test_id_1sl,
					nloss, entry->eth_slm_dual_tx_nloss, entry->eth_slm_dual_rx_nloss, entry->eth_slm_dual_nratio/100, entry->eth_slm_dual_nratio%100);
			}

			entry->TxFCf_p = ptr_1sl->TxFCf;
			entry->RxFCl_p = entry->RxFCl_1sl;
		}

	}
	return 0;
}

static int
is_valid_ccm_maid(cfm_config_t *cfm_config, char *maid)
{
	char ma_name[49] = {0};
	char md_name[49] = {0};
	int md_len, ma_len;

	if (maid[0] != cfm_config->md_format)
		return 0;

	strncpy(ma_name, cfm_config->ma_name_1, 25);
	ma_name[25]=0;
	strncat(ma_name, cfm_config->ma_name_2, 48-strlen(ma_name));
	ma_name[48]=0;

	if (cfm_config->md_format != MD_NAME_FORMAT_NONE) {	// MD format
		// 0:md_format, 1:md_len, 2..x-1:md_name, x:ma_format, x+1:ma_len, x+2:ma_name
		strncpy(md_name, cfm_config->md_name_1, 25);
		md_name[25]=0;
		strncat(md_name, cfm_config->md_name_2, 48-strlen(md_name));
		md_name[48]=0;

		switch(cfm_config->md_format) { 		// MD Length
			case MD_NAME_FORMAT_MACADDR_SHORTINT:	md_len = 8; break;
			case MD_NAME_FORMAT_DNS:
			case MD_NAME_FORMAT_STRING:
			case MD_NAME_FORMAT_ICC:
			default:				md_len = strlen(md_name); break;
		}
		if (maid[1] != md_len)
			return 0;
		if (memcmp(maid+2, md_name, md_len) != 0)
			return 0;

		if (maid[2+md_len] != cfm_config->ma_format) 		// MA Format
			return 0;
		switch(cfm_config->ma_format) {
			case MA_NAME_FORMAT_VID:
			case MA_NAME_FORMAT_SHORTINT:	ma_len = 2; break;
			case MA_NAME_FORMAT_STRING:
			default:			ma_len = strlen(ma_name); break;
			case MA_NAME_FORMAT_VPNID:	ma_len = 7; break;
			case MA_NAME_FORMAT_ICC:	ma_len = 13; break;
		}
		if (maid[2+md_len+1] != ma_len) 			// MA Length
			return 0;
		if (memcmp(maid+2+md_len+2, ma_name, ma_len) != 0)	// MA Name
			return 0;

	} else {
		// 0:md_format, 1:ma_format, 2:ma_len, 3..47:ma_name
		if (maid[1] != cfm_config->ma_format)		// MA format
			return 0;
		switch (cfm_config->ma_format) {
			case MA_NAME_FORMAT_VID:
			case MA_NAME_FORMAT_SHORTINT:	ma_len = 2; break;
			case MA_NAME_FORMAT_VPNID: 	ma_len = 7; break;
			case MA_NAME_FORMAT_STRING:
			default:			ma_len = strlen(ma_name); break;
			case MA_NAME_FORMAT_ICC:	ma_len = 13; break;
		}
		if (maid[2] != ma_len)				// MA len
			return 0;
		if (memcmp(maid+3, ma_name, ma_len) != 0)	// MA name
			return 0;

	}
	return 1;	// maid is valid
}

static cfm_pkt_rmep_entry_t *
cfm_recv_ccm(cfm_config_t *cfm_config, cfm_pktinfo_t *cfm_pktinfo)
{
	cfm_hdr_t *cfm_hdr = (cfm_hdr_t *) (cfm_pktinfo->frame_ptr + IFHWADDRLEN*2 + cfm_pktinfo->vlan_num*VLAN_TAG_OFFSET + ETHER_TYPE_LEN);
	cfm_pdu_ccm_t *ccm_ptr = (cfm_pdu_ccm_t *)((void *)cfm_hdr + sizeof(cfm_hdr_t));
	char *tlv_start=(unsigned char *)((void *)cfm_hdr + sizeof(cfm_hdr_t) + cfm_hdr->first_tlv_offset);
	int tlv_total_len = (cfm_pktinfo->frame_ptr+cfm_pktinfo->frame_len) - (unsigned char *)tlv_start;

	unsigned char rmep_interval =  cfm_hdr->flags & 0x7;
	unsigned char rmep_rdi = (cfm_hdr->flags >> 7)&1;
	unsigned long long now = cfm_util_get_time_in_us();
	cfm_pkt_rmep_entry_t *entry;
	int is_rmep_found = 0, port_state = 0, iface_state = 0;

	list_for_each_entry(entry, &cfm_config->cfm_recv_rmep_list, node) {
		if (entry->rmep_id == htons(ccm_ptr->mep_id) &&
		    (entry->rmep_state == CCM_RMEP_STATE_EXPECT || memcmp(&entry->rmep_mac, cfm_pktinfo->src_mac_ptr, IFHWADDRLEN) == 0)) {
		    	// entry created from expected mep list or entry collected from other rmep ccm pkt
		    	is_rmep_found = 1;
		    	break;
		}
	}

	// ccm defect check start ///////////////////////////////////////////////////////////
	// we record defect timestamp only for ccm from ACTIVE/LOST rmep
	// note: both ACTIVE/LOST state rmep need to be tracked,
	//       if we dont mark defect timestamp for LOST rmep,
	//       then the defect would be cleared soon as rmep is remained in LOST state

	// unexpected rmep, drop if auto_discovery isn't enabled
	if(!is_rmep_found && !cfm_config->auto_discovery) {
		cfm_config->defect_error_ccm_timestamp = now;
		return NULL;
	}
	// rmep has same mepid as us, invalid
	if (cfm_config->mep_id == ntohs(ccm_ptr->mep_id)) {
		if (is_rmep_found) {
			if (entry->rmep_state == CCM_RMEP_STATE_ACTIVE ||
			    entry->rmep_state == CCM_RMEP_STATE_LOST ||
			    entry->rmep_state == CCM_RMEP_STATE_EXPECT) {
				cfm_config->defect_error_ccm_timestamp = now;
			} else if (entry->rmep_state == CCM_RMEP_STATE_DISCOVER ||
				   entry->rmep_state == CCM_RMEP_STATE_HOLD) {
				entry->rmep_ccm_consecutive = 0;
				entry->rmep_state = CCM_RMEP_STATE_DISCOVER;
			}
		} else {
			cfm_config->defect_error_ccm_timestamp = now;
		}

		// save as last err_ccm pkt
		cfm_config->last_recv_err_ccm_msg_len = cfm_pktinfo->frame_len - ((void*)cfm_hdr - (void*)cfm_pktinfo->frame_ptr);
		if (cfm_config->last_recv_err_ccm_msg_len > 128)
			cfm_config->last_recv_err_ccm_msg_len = 128;
		memcpy(cfm_config->last_recv_err_ccm_msg, cfm_hdr, cfm_config->last_recv_err_ccm_msg_len);
		// print err
		dbprintf_cfm(LOG_NOTICE, "Drop CCM, mdl=%d, flags=0x%02x, seq=0x%x, rmep=%d, from %s logical port %d %s (mepid conflict)\n",
			cfm_pktinfo->cfm_hdr->md_level, cfm_pktinfo->cfm_hdr->flags,
			htonl(ccm_ptr->seq), htons(ccm_ptr->mep_id),
			util_str_macaddr(cfm_pktinfo->src_mac_ptr),
			cfm_pktinfo->rx_desc.logical_port_id,
			cfm_print_get_vlan_str(cfm_pktinfo));

		return NULL;
	}
	// rmep belong to different maid (md_name + ma_name), not for us
	if (is_valid_ccm_maid(cfm_config, ccm_ptr->ma_id) == 0) {
		if (is_rmep_found) {
			if (entry->rmep_state == CCM_RMEP_STATE_ACTIVE ||
			    entry->rmep_state == CCM_RMEP_STATE_LOST ||
			    entry->rmep_state == CCM_RMEP_STATE_EXPECT) {
				cfm_config->defect_xcon_ccm_timestamp = now;
			} else if (entry->rmep_state == CCM_RMEP_STATE_DISCOVER ||
				   entry->rmep_state == CCM_RMEP_STATE_HOLD) {
				entry->rmep_ccm_consecutive = 0;
				entry->rmep_state = CCM_RMEP_STATE_DISCOVER;
			}
		} else {
			cfm_config->defect_xcon_ccm_timestamp = now;
		}
		// save as last xcon_ccm pkt
		cfm_config->last_xcon_err_ccm_msg_len = cfm_pktinfo->frame_len - ((void*)cfm_hdr - (void*)cfm_pktinfo->frame_ptr);
		if (cfm_config->last_xcon_err_ccm_msg_len > 128)
			cfm_config->last_xcon_err_ccm_msg_len = 128;
		memcpy(cfm_config->last_xcon_err_ccm_msg, cfm_hdr, cfm_config->last_xcon_err_ccm_msg_len);
		// print err
		dbprintf_cfm(LOG_NOTICE, "Drop CCM, mdl=%d, flags=0x%02x, seq=0x%x, rmep=%d, from %s logical port %d %s (ma mismatch)\n",
			cfm_pktinfo->cfm_hdr->md_level, cfm_pktinfo->cfm_hdr->flags,
			htonl(ccm_ptr->seq), htons(ccm_ptr->mep_id),
			util_str_macaddr(cfm_pktinfo->src_mac_ptr),
			cfm_pktinfo->rx_desc.logical_port_id,
			cfm_print_get_vlan_str(cfm_pktinfo));
		return NULL;
	}
	// rmep send ccm with wrong ccm interval period
	if (rmep_interval == 0 || rmep_interval != cfm_config->ccm_interval) {
		if (is_rmep_found) {
			if (entry->rmep_state == CCM_RMEP_STATE_ACTIVE ||
			    entry->rmep_state == CCM_RMEP_STATE_LOST ||
			    entry->rmep_state == CCM_RMEP_STATE_EXPECT) {
				cfm_config->defect_unexp_period_timestamp = now;
			} else if (entry->rmep_state == CCM_RMEP_STATE_DISCOVER ||
				   entry->rmep_state == CCM_RMEP_STATE_HOLD) {
				entry->rmep_ccm_consecutive = 0;
				entry->rmep_state = CCM_RMEP_STATE_DISCOVER;
			}
		} else {
			cfm_config->defect_unexp_period_timestamp = now;
		}
		// save as last err_ccm pkt
		cfm_config->last_recv_err_ccm_msg_len = cfm_pktinfo->frame_len - ((void*)cfm_hdr - (void*)cfm_pktinfo->frame_ptr);
		if (cfm_config->last_recv_err_ccm_msg_len > 128)
			cfm_config->last_recv_err_ccm_msg_len = 128;
		memcpy(cfm_config->last_recv_err_ccm_msg, cfm_hdr, cfm_config->last_recv_err_ccm_msg_len);

		dbprintf_cfm(LOG_NOTICE, "Drop CCM, mdl=%d, flags=0x%02x, seq=0x%x, rmep=%d, from %s logical port %d %s (ccm interval mismatch)\n",
			cfm_pktinfo->cfm_hdr->md_level, cfm_pktinfo->cfm_hdr->flags,
			htonl(ccm_ptr->seq), htons(ccm_ptr->mep_id),
			util_str_macaddr(cfm_pktinfo->src_mac_ptr),
			cfm_pktinfo->rx_desc.logical_port_id,
			cfm_print_get_vlan_str(cfm_pktinfo));
		return NULL;
	}
	// rmep send ccm with rdi bit
	if (rmep_rdi) {
		if (is_rmep_found) {
			if (entry->rmep_state == CCM_RMEP_STATE_ACTIVE ||
			    entry->rmep_state == CCM_RMEP_STATE_LOST ||
			    entry->rmep_state == CCM_RMEP_STATE_EXPECT)
				cfm_config->defect_rdi_ccm_timestamp = now;
		} else {
			cfm_config->defect_rdi_ccm_timestamp = now;
		}
	}

	// rmep send ccm with tlv interface/port status != up
	{
		unsigned short tlv_len;
		unsigned char *tlv_val;
		int is_defect_mac = 0;

		if (cfm_util_find_tlv(tlv_start, tlv_total_len, CFM_TLV_TYPE_PORT_STATUS, &tlv_len, &tlv_val) >0) {
			if (tlv_val[0] !=2)	// port status != up
				is_defect_mac =1;
			port_state = tlv_val[0];
			if ((tlv_len != 1) || (tlv_val[0] != 1 && tlv_val[0] != 2)) {
				// note! invalid port_status tlv ccm is counted from ACTIVE rmep only
				if (entry->rmep_state == CCM_RMEP_STATE_ACTIVE) {
					cfm_config->invalid_port_status_ccm_count++;
				}
			}
		}
		if (cfm_util_find_tlv(tlv_start, tlv_total_len, CFM_TLV_TYPE_INTERFACE_STATUS, &tlv_len, &tlv_val) >0) {
			if (tlv_val[0] !=1)	// interface status != up
				is_defect_mac =1;
			iface_state = tlv_val[0];
			if ((tlv_len != 1) || (tlv_val[0] != 1 && tlv_val[0] != 2)) {
				// note! invalid interface_status tlv ccm is counted from ACTIVE rmep only
				if (entry->rmep_state == CCM_RMEP_STATE_ACTIVE) {
					cfm_config->invalid_interface_status_ccm_count++;
				}
			}
		}
		if (is_defect_mac) {
			if (is_rmep_found) {
				if (entry->rmep_state == CCM_RMEP_STATE_ACTIVE ||
				    entry->rmep_state == CCM_RMEP_STATE_LOST ||
				    entry->rmep_state == CCM_RMEP_STATE_EXPECT)
					cfm_config->defect_mac_status_timestamp = now;
			} else {
				cfm_config->defect_mac_status_timestamp = now;
			}
		}
	}
	// ccm defect check end ///////////////////////////////////////////////////////////

	if (!is_rmep_found) {
		entry = malloc_safe(sizeof(cfm_pkt_rmep_entry_t));
		if (entry == NULL)
			return entry;
		list_add_tail(&entry->node, &cfm_config->cfm_recv_rmep_list);

		memcpy(&entry->rmep_mac, cfm_pktinfo->src_mac_ptr, IFHWADDRLEN);
		entry->rmep_id = htons(ccm_ptr->mep_id);
		entry->rmep_ccm_consecutive = 1;
		entry->rmep_state = CCM_RMEP_STATE_DISCOVER;

	} else {
		if (entry->rmep_state == CCM_RMEP_STATE_EXPECT) {
 			// update mac for entry created from expected list
 			memcpy(&entry->rmep_mac, cfm_pktinfo->src_mac_ptr, IFHWADDRLEN);
			entry->rmep_ccm_consecutive = 1;
			entry->rmep_state = CCM_RMEP_STATE_DISCOVER;

		} else if (entry->rmep_state == CCM_RMEP_STATE_DISCOVER) {
			if (ccm_ptr->seq && ntohl(ccm_ptr->seq) <= entry->rmep_seq) {
				// maybe receive duplicate or out-of-order ccm due to network latency, ignore it
				dbprintf_cfm(LOG_WARNING, "Duplicated CCM (seq=%d rmep_seq=%d)\n", ntohl(ccm_ptr->seq), entry->rmep_seq);
			} else if (ccm_ptr->seq && ntohl(ccm_ptr->seq) != entry->rmep_seq+1) {
				entry->rmep_ccm_consecutive = 1;
				// remain in DISCOVER state
			} else {
				entry->rmep_ccm_consecutive++;
				if (entry->rmep_ccm_consecutive >= omci_env_g.cfm_rmep_ccm_before_hold) {
					entry->rmep_ccm_consecutive = 1;
					entry->rmep_state = CCM_RMEP_STATE_HOLD;
					if (omci_env_g.cfm_rmep_holdtime_before_active == 0) {
						entry->rmep_state = CCM_RMEP_STATE_ACTIVE;
						dbprintf_cfm(LOG_ERR, "%s CFM alarm NEW_PEER is set (rMEPid=%d)\n",
							cfm_print_get_cfm_config_str(cfm_config), entry->rmep_id);
					}
				}
			}

		} else if (entry->rmep_state == CCM_RMEP_STATE_HOLD) {
			if (ccm_ptr->seq && ntohl(ccm_ptr->seq) <= entry->rmep_seq) {
				// maybe receive duplicate or out-of-order ccm due to network latency, ignore it
				dbprintf_cfm(LOG_WARNING, "Duplicated CCM (seq=%d rmep_seq=%d)\n", ntohl(ccm_ptr->seq), entry->rmep_seq);
			} else if (ccm_ptr->seq && ntohl(ccm_ptr->seq) != entry->rmep_seq+1) {
				entry->rmep_ccm_consecutive = 1;
				entry->rmep_state = CCM_RMEP_STATE_DISCOVER;
			} else {
				entry->rmep_ccm_consecutive++;
				// for HOLD rmep is free of defect for more than holdtime, turn state into ACTIVE
				// note: as n consecutive would have only n-1 interval, so use >, not >=
				if (entry->rmep_ccm_consecutive >
				    omci_env_g.cfm_rmep_holdtime_before_active/cfm_util_ccm_interval_to_ms(cfm_config->ccm_interval)) {
					entry->rmep_ccm_consecutive = 1;
					entry->rmep_state = CCM_RMEP_STATE_ACTIVE;
					dbprintf_cfm(LOG_ERR, "%s CFM alarm NEW_PEER is set (rMEPid=%d)\n",
						cfm_print_get_cfm_config_str(cfm_config), entry->rmep_id);
				}
			}

		} else if (entry->rmep_state == CCM_RMEP_STATE_ACTIVE) {
			if (ccm_ptr->seq && ntohl(ccm_ptr->seq) <= entry->rmep_seq) {
				// maybe receive duplicate or out-of-order ccm due to network latency, ignore it
				dbprintf_cfm(LOG_WARNING, "Duplicated CCM (seq=%d rmep_seq=%d)\n", ntohl(ccm_ptr->seq), entry->rmep_seq);
			} else if (ccm_ptr->seq && ntohl(ccm_ptr->seq) != entry->rmep_seq+1) {
				entry->rmep_ccm_consecutive = 1;
				// note! out of seq ccm is counted from ACTIVE rmep only
				cfm_config->out_of_seqence_ccm_count++;
			} else {
				entry->rmep_ccm_consecutive++;
			}

		} else if (entry->rmep_state == CCM_RMEP_STATE_INACTIVE) {
			if (ccm_ptr->seq && ntohl(ccm_ptr->seq) <= entry->rmep_seq) {
				// maybe receive duplicate or out-of-order ccm due to network latency, ignore it
				dbprintf_cfm(LOG_WARNING, "Duplicated CCM (seq=%d rmep_seq=%d)\n", ntohl(ccm_ptr->seq), entry->rmep_seq);
			} else if (ccm_ptr->seq && ntohl(ccm_ptr->seq) != entry->rmep_seq+1) {
				entry->rmep_ccm_consecutive = 1;
			} else {
				entry->rmep_ccm_consecutive++;
			}

		} else if (entry->rmep_state == CCM_RMEP_STATE_LOST) {
			entry->rmep_ccm_consecutive = 1;
			entry->rmep_state = CCM_RMEP_STATE_ACTIVE;

		} // else: NONE or REMOVE, do nothing with counter or state
	}

	entry->rmep_timestamp = now;	// unit: us
	entry->rmep_clear_timestamp = 0;
	entry->rmep_interval = rmep_interval;
	entry->rmep_rdi = rmep_rdi;
	entry->rmep_port_state = port_state;
	entry->rmep_iface_state = iface_state;
	entry->rmep_seq = ntohl(ccm_ptr->seq);
	entry->RxFCl_ccm++;

	if (cfm_config->lmm_dual_test_enable) {
		if (cfm_config->lm_dual_test_rmep == 0 ) { //get into eth-lm dual test mode
			if ( ccm_ptr->y_1731[0] != 0 || ccm_ptr->y_1731[1] != 0 || ccm_ptr->y_1731[1] != 0 ) {
				cfm_config->lm_dual_test_rmep = entry->rmep_id;
			}
		}else{
			if ( ccm_ptr->y_1731[0] != 0 || ccm_ptr->y_1731[1] != 0 || ccm_ptr->y_1731[1] != 0 ) {
				if ( cfm_config->lm_dual_test_rmep == entry->rmep_id ){
					entry->TxFCf_ccm = ntohl(ccm_ptr->y_1731[0]) ;
					entry->RxFCb_ccm = ntohl(ccm_ptr->y_1731[1]) ;
					entry->TxFCb_ccm = ntohl(ccm_ptr->y_1731[2]) ;
				}else{
					// another eth-lm dual test is running
					dbprintf_cfm(LOG_ERR,"Another eth-lm dual test is running.\n");
				}
			}
		}
	}

	// copy sender_id to ccm db entry
	{
		unsigned short tlv_len;
		unsigned char *tlv_val;

		if (cfm_util_find_tlv(tlv_start, tlv_total_len, CFM_TLV_TYPE_SENDER_ID, &tlv_len, &tlv_val) >0) {
			memcpy(&entry->rmep_sender_id_tlv, tlv_val, sizeof(cfm_tlv_sender_id_t));
			if(tlv_len == 0) {
				// note! invalid senderid tlv ccm is counted from ACTIVE rmep only
				if (entry->rmep_state == CCM_RMEP_STATE_ACTIVE) {
					cfm_config->invalid_senderid_ccm_count++;
				}
			}
		} else {
			memset(&entry->rmep_sender_id_tlv, 0, sizeof(cfm_tlv_sender_id_t));
		}
	}

	return entry;
}

static int
valid_mep_direction_for_wan_recv(cfm_config_t *cfm_config)
{
	if (!omci_env_g.cfm_reverse_mep_direction) {
		// definition from 802.1ag
		switch (cfm_config->port_type) {	// cfm recv on wan is
			case ENV_BRIDGE_PORT_TYPE_WAN:
			case ENV_BRIDGE_PORT_TYPE_DS_BCAST:	// for down mep on wan
				return !(cfm_config->mep_control & MEP_CONTROL_IS_UP_MEP);
			case ENV_BRIDGE_PORT_TYPE_UNI:
			case ENV_BRIDGE_PORT_TYPE_CPU:
			case ENV_BRIDGE_PORT_TYPE_IPHOST:	// for up mep on lan
				return (cfm_config->mep_control & MEP_CONTROL_IS_UP_MEP);
		}
	} else {
		// definition for alu, which is 802.1ag reversed
		switch (cfm_config->port_type) {
			case ENV_BRIDGE_PORT_TYPE_WAN:
			case ENV_BRIDGE_PORT_TYPE_DS_BCAST:
				return (cfm_config->mep_control & MEP_CONTROL_IS_UP_MEP);
			case ENV_BRIDGE_PORT_TYPE_UNI:
			case ENV_BRIDGE_PORT_TYPE_CPU:
			case ENV_BRIDGE_PORT_TYPE_IPHOST:
				return !(cfm_config->mep_control & MEP_CONTROL_IS_UP_MEP);
		}
	}
	return 0;
}

static int
valid_mep_direction_for_lan_recv(cfm_config_t *cfm_config)
{
	if (!omci_env_g.cfm_reverse_mep_direction) {
		// definition from 802.1ag
		switch (cfm_config->port_type) {	// cfm recv on lan is
			case ENV_BRIDGE_PORT_TYPE_WAN:
			case ENV_BRIDGE_PORT_TYPE_DS_BCAST:	// for up mep on wan
				return (cfm_config->mep_control & MEP_CONTROL_IS_UP_MEP);
			case ENV_BRIDGE_PORT_TYPE_UNI:
			case ENV_BRIDGE_PORT_TYPE_CPU:
			case ENV_BRIDGE_PORT_TYPE_IPHOST:	// for down mep on lan
				return !(cfm_config->mep_control & MEP_CONTROL_IS_UP_MEP);
		}
	} else {
		// definition for alu, which is 802.1ag reversed
		switch (cfm_config->port_type) {
			case ENV_BRIDGE_PORT_TYPE_WAN:
			case ENV_BRIDGE_PORT_TYPE_DS_BCAST:
				return !(cfm_config->mep_control & MEP_CONTROL_IS_UP_MEP);
			case ENV_BRIDGE_PORT_TYPE_UNI:
			case ENV_BRIDGE_PORT_TYPE_CPU:
			case ENV_BRIDGE_PORT_TYPE_IPHOST:
				return (cfm_config->mep_control & MEP_CONTROL_IS_UP_MEP);
		}
	}
	return 0;
}

static int
cfm_recv_is_match_cfm_config(cfm_config_t *cfm_config, unsigned char rx_logical_port_id)
{
	// Check incoming port
	if(rx_logical_port_id == switch_get_wan_logical_portid()) { // WAN
		// direction up/down check for mep
		if (cfm_util_get_mhf_creation(cfm_config) == MA_MHF_CREATION_NONE) {
			if (!valid_mep_direction_for_wan_recv(cfm_config))
				return 0;
		}
		// same bridge check
		if (cfm_config->port_type == ENV_BRIDGE_PORT_TYPE_WAN) {
			// mep/mip port match cfm ingress port
			if (cfm_config->logical_port_id == rx_logical_port_id)
				return 1;
		} else if (cfm_config->port_type == ENV_BRIDGE_PORT_TYPE_UNI ||
			cfm_config->port_type == ENV_BRIDGE_PORT_TYPE_CPU) {
			// the peer of mep/mip port match cfm ingress port
			if (cfm_bport_find_peer_logical_portmask(cfm_config) & (1<<rx_logical_port_id))
				return 1;
		}
	} else { // UNI
		// direction up/down check for mep
		if (cfm_util_get_mhf_creation(cfm_config) == MA_MHF_CREATION_NONE) { 	// MEP
			if (!valid_mep_direction_for_lan_recv(cfm_config))
				return 0;
		}
		// same bridge check
		if (cfm_config->port_type == ENV_BRIDGE_PORT_TYPE_UNI) {
			// mep/mip port match cfm ingress port
			if (cfm_config->logical_port_id ==  rx_logical_port_id)
				return 1;
			// mep/mip port match uni0 (if port2port enabled)
			else if(omci_env_g.port2port_enable && cfm_config->logical_port_id == 0)
				return 1;
		} else if (cfm_config->port_type == ENV_BRIDGE_PORT_TYPE_WAN) {
			// the peer of mep/mip port match cfm ingress port
			if (cfm_bport_find_peer_logical_portmask(cfm_config) & (1<<rx_logical_port_id))
				return 1;
			// the peer of mep/mip port match uni0 (if port2port enabled)
			else if (omci_env_g.port2port_enable && (cfm_bport_find_peer_logical_portmask(cfm_config) & 1)) // 1 means the bit for port0
				return 1;
		}
	}
	return 0;
}

// for specific cfm pkt, only one cfm config would be located to process the pkt
// if multiple mep/mip defined in the device
//   md level exact match mep will be choosed first (for reply),
//   then md level exact match mip will be choosed (for fwd/reply),
//   mep/mip with lowest md level and the md level < pkt md level will be chosen (for fwd)
//   mep/mip with highest md level and the md level > pkt md level will be chosen (for alarm&drop)
// note: the admin_state is checked outside the routine,
//       or we might issue XCON alarm incorrectly when a pkt is recv by a admin locked mep
cfm_config_t *
cfm_recv_get_cfm_config (cfm_pktinfo_t *cfm_pktinfo)
{
	unsigned char rx_logical_port_id = cfm_pktinfo->rx_desc.logical_port_id;
	unsigned char pkt_md_level = cfm_pktinfo->cfm_hdr->md_level;
	cfm_config_t *cfm_config;
	int md_level;

	// locate MEP whose md_level == cfm pkt md_level, for reply
	list_for_each_entry(cfm_config, &cfm_config_list, node) {
		if (cfm_util_get_mhf_creation(cfm_config) == MA_MHF_CREATION_NONE) {  // MEP
			if (cfm_config->md_level == pkt_md_level) {
			    	if (cfm_pkt_check_vlan(cfm_config, cfm_pktinfo) == 0)
			    		continue;
				if (cfm_recv_is_match_cfm_config(cfm_config, rx_logical_port_id) == 0)
			    		continue;
		   		dbprintf_cfm(LOG_INFO, "pkt %s MATCH MEP %s\n",
		    			cfm_print_get_cfm_pkt_str(cfm_pktinfo->cfm_hdr), cfm_print_get_cfm_config_str(cfm_config));
				return cfm_config;
			}
		}
	}

	// locate MIP whose md_level == cfm pkt md_level, for reply/fwd
	list_for_each_entry(cfm_config, &cfm_config_list, node) {
		if (cfm_util_get_mhf_creation(cfm_config) != MA_MHF_CREATION_NONE) {  // MIP
			if (cfm_config->md_level == pkt_md_level) {
			    	if (cfm_pkt_check_vlan(cfm_config, cfm_pktinfo) == 0)
			    		continue;
				if (cfm_recv_is_match_cfm_config(cfm_config, rx_logical_port_id) == 0)
			    		continue;
		   		dbprintf_cfm(LOG_INFO, "pkt %s MATCH MIP %s\n",
		    			cfm_print_get_cfm_pkt_str(cfm_pktinfo->cfm_hdr), cfm_print_get_cfm_config_str(cfm_config));
				return cfm_config;
			}
		}
	}

	// locate MEP/MIP with minimal md level and this md_level < cfm pkt md_level, for fwd
	for(md_level=0; md_level< pkt_md_level; md_level++) {
		list_for_each_entry(cfm_config, &cfm_config_list, node) {
			if (cfm_config->md_level == md_level) {
			    	if (cfm_pkt_check_vlan(cfm_config, cfm_pktinfo) == 0)
			    		continue;
				if (cfm_recv_is_match_cfm_config(cfm_config, rx_logical_port_id) == 0)
			    		continue;
		   		dbprintf_cfm(LOG_INFO, "pkt %s MATCH FWD MEP/MIP %s\n",
		    			cfm_print_get_cfm_pkt_str(cfm_pktinfo->cfm_hdr), cfm_print_get_cfm_config_str(cfm_config));
			     	return cfm_config;
			}
		}
	}

	// locate MEP/MIP with minimal md level and this md_level < cfm pkt md_level, for xcon alarm&drop
	for(md_level=7; md_level> pkt_md_level; md_level--) {
		list_for_each_entry(cfm_config, &cfm_config_list, node) {
			if (cfm_config->md_level == md_level) {
			    	if (cfm_pkt_check_vlan(cfm_config, cfm_pktinfo) == 0)
			    		continue;
				if (cfm_recv_is_match_cfm_config(cfm_config, rx_logical_port_id) == 0)
			    		continue;
		   		dbprintf_cfm(LOG_INFO, "pkt %s MATCH ALARM MEP/MIP %s\n",
		    			cfm_print_get_cfm_pkt_str(cfm_pktinfo->cfm_hdr), cfm_print_get_cfm_config_str(cfm_config));
			     	return cfm_config;
			}
		}
	}

	return NULL;
}

static int bitcount8(unsigned char b)
{
	b = (b & 0x55u) + ((b >> 1) & 0x55u);
	b = (b & 0x33u) + ((b >> 2) & 0x33u);
	b = (b & 0x0fu) + ((b >> 4) & 0x0fu);

	return (b);
}

// mep action					when
//-------------------------------------------	--------------------
// update defect time				pkt recv
// cfm_mp_update_defect_status()		alarm loop, ccm send or pkt recv
// cfm_mp_check_interval_and_send_ais_lck	alarm loop, ccm send or pkt recv
// timer_event_alarm_check			alarm loop

// we do cfm_mp_update_defect_status(), cfm_mp_check_interval_and_send_ais_lck() in
// a. alarm loop: for quick response on mep with very long interval (eg: 1min, 10min)
// b. ccm send: for normal mep with very short interval (eg: 3.3ms)
// c. pkt recv: for server mep which has no ccm send

int
cfm_recv_pkt_process (struct cpuport_info_t *pktinfo, cfm_pktinfo_t *cfm_pktinfo)
{
	unsigned char *p = (unsigned char *) cfm_pktinfo->cfm_hdr + sizeof(cfm_hdr_t);
	int type = cfm_pkt_type(cfm_pktinfo) ;
	cfm_config_t *cfm_config;

	unsigned char cfm_mmac[IFHWADDRLEN] = {0x01, 0x80, 0xC2, 0x00, 0x00, 0x30};
	cfm_pdu_ltm_t *pdu_ltm;
	cfm_pkt_rmep_entry_t *entry = NULL;

	// Check if this CFM packet is valid
	if (type == CFM_PDU_INVALID) {
		server_mep_cfm_config_g.cfm_pkt_pm[CFM_PDU_INVALID].drop++;
		if(cfm_pktinfo->rx_desc.logical_port_id == switch_get_wan_logical_portid())
			server_mep_cfm_config_g.cfm_pkt_pm[CFM_PDU_INVALID].drop_ani++;
		else
			server_mep_cfm_config_g.cfm_pkt_pm[CFM_PDU_INVALID].drop_uni++;
		dbprintf_cfm(LOG_WARNING,
			"Drop %s from %s logical port %d %s (invalid CFM or dmac)\n",
			cfm_print_get_cfm_pkt_str(cfm_pktinfo->cfm_hdr),
			util_str_macaddr(cfm_pktinfo->src_mac_ptr),
			cfm_pktinfo->rx_desc.logical_port_id,
			cfm_print_get_vlan_str(cfm_pktinfo));
		return -1;
	}

	// Check if this CFM packet belongs to any MEP/MIP
	if ((cfm_config = cfm_recv_get_cfm_config(cfm_pktinfo)) == NULL) {
		cfm_config = &server_mep_cfm_config_g;

		// Calix case: bypass cfm stack if it doesn't belong to any MEP/MIP
		if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX)==0) {
			// check rma mask if it's multicast, otherwise forward unconditionally
			int forward = (pktinfo->dst_mac[0]&1) ? (cpuport_pkt_is_rma_forward(pktinfo) ? 1 : 0) : 1;
			cfm_config->cfm_pkt_pm[(type<=CFM_PDU_MAX)?type:CFM_PDU_INVALID].fwd++;
			if(cfm_pktinfo->rx_desc.logical_port_id == switch_get_wan_logical_portid()) { // WAN -> UNI
				if(forward) {
					cfm_config->cfm_pkt_pm[(type<=CFM_PDU_MAX)?type:CFM_PDU_INVALID].fwd_uni++;
					return (cpuport_util_send_multi_lan(pktinfo, "cfm-to-lan", NULL, NULL)==1) ? 0 : -1;
				}
			} else { // UNI -> WAN
				if(forward) {
					cfm_config->cfm_pkt_pm[(type<=CFM_PDU_MAX)?type:CFM_PDU_INVALID].fwd_ani++;
					return (cpuport_util_send_multi_wan(pktinfo, "cfm-to-wan", NULL, NULL)==1) ? 0 : -1;
				}
			}
		}

		cfm_config->cfm_pkt_pm[(type<=CFM_PDU_MAX)?type:CFM_PDU_INVALID].drop++;
		if(cfm_pktinfo->rx_desc.logical_port_id == switch_get_wan_logical_portid())
			cfm_config->cfm_pkt_pm[(type<=CFM_PDU_MAX)?type:CFM_PDU_INVALID].drop_ani++;
		else
			cfm_config->cfm_pkt_pm[(type<=CFM_PDU_MAX)?type:CFM_PDU_INVALID].drop_uni++;
		if (type == CFM_PDU_CCM) {	// ccm pkt not handled by mep/mip should be treated by server_mep
			// server mep
			if (omci_env_g.cfm_y1731_enable &&
			    cfm_config->mep_control & MEP_CONTROL_SERVER_MEP_ENABLE) {	// server_mep enabled

				cfm_config->defect_xcon_ccm_timestamp = cfm_util_get_time_in_us();
				{	// save as last xcon_ccm pkt
					cfm_config->last_xcon_err_ccm_msg_len = cfm_pktinfo->frame_len - ((void*)cfm_pktinfo->cfm_hdr - (void*)cfm_pktinfo->frame_ptr);
					if (cfm_config->last_xcon_err_ccm_msg_len > 128)
						cfm_config->last_xcon_err_ccm_msg_len = 128;
					memcpy(cfm_config->last_xcon_err_ccm_msg, cfm_pktinfo->cfm_hdr, cfm_config->last_xcon_err_ccm_msg_len);
				}
				// record where the defect pkt come from, so we can send ais in opposite direction for server mep
				server_mep_logical_portmask_g |= (1<<cfm_pktinfo->rx_desc.logical_port_id);

				// cfm_mp_update_defect_status() in done pkt recv, ccm send & alarm loop
				if (cfm_mp_update_defect_status(cfm_config)>0) {
					// send ais ASAP if any defect detected
					cfm_mp_check_interval_and_send_ais_lck(cfm_config);
				}
			}

		}
		dbprintf_cfm((type==CFM_PDU_CCM)? LOG_NOTICE:LOG_WARNING,
			"Drop %s from %s logical port %d %s (no mep/mip found)\n",
			cfm_print_get_cfm_pkt_str(cfm_pktinfo->cfm_hdr),
			util_str_macaddr(cfm_pktinfo->src_mac_ptr),
			cfm_pktinfo->rx_desc.logical_port_id,
			cfm_print_get_vlan_str(cfm_pktinfo));
		return -1;
	}
	// check if CFM pkt owner is admin locked
	if (cfm_config->admin_state) {
		cfm_config->cfm_pkt_pm[(type<=CFM_PDU_MAX)?type:CFM_PDU_INVALID].drop++;
		if(cfm_pktinfo->rx_desc.logical_port_id == switch_get_wan_logical_portid())
			cfm_config->cfm_pkt_pm[(type<=CFM_PDU_MAX)?type:CFM_PDU_INVALID].drop_ani++;
		else
			cfm_config->cfm_pkt_pm[(type<=CFM_PDU_MAX)?type:CFM_PDU_INVALID].drop_uni++;

		dbprintf_cfm((type==CFM_PDU_CCM)? LOG_NOTICE:LOG_WARNING,
			"Drop %s from %s logical port %d %s (mdl %d, %s %d is admLocked)\n",
			cfm_print_get_cfm_pkt_str(cfm_pktinfo->cfm_hdr),
			util_str_macaddr(cfm_pktinfo->src_mac_ptr),
			cfm_pktinfo->rx_desc.logical_port_id,
			cfm_print_get_vlan_str(cfm_pktinfo),
			cfm_config->md_level,
			(cfm_util_get_mhf_creation(cfm_config) == MA_MHF_CREATION_NONE)?"mep":"mip",
			cfm_config->mep_id);
		return -1;
	}

	cfm_print_recv_frame((type==CFM_PDU_CCM)?LOG_NOTICE:LOG_WARNING, cfm_config, cfm_pktinfo);

	if (cfm_pktinfo->cfm_hdr->md_level > cfm_config->md_level) {
		dbprintf_cfm(LOG_INFO, "%s(%d) md level %d > mp index %d md level %d, forward\n",
			cfm_print_get_opcode_str(type), type, cfm_pktinfo->cfm_hdr->md_level, cfm_config->index, cfm_config->md_level);
		//cfm_config->cfm_pkt_pm[(type<=CFM_PDU_MAX)?type:CFM_PDU_INVALID].rx++;
		if(cfm_pktinfo->rx_desc.logical_port_id == switch_get_wan_logical_portid())
			cfm_config->cfm_pkt_pm[(type<=CFM_PDU_MAX)?type:CFM_PDU_INVALID].rx_ani++;
		else
			cfm_config->cfm_pkt_pm[(type<=CFM_PDU_MAX)?type:CFM_PDU_INVALID].rx_uni++;
		cfm_forward_cfm(cfm_config, pktinfo, cfm_pktinfo, type);
		return 0;
	} else if (cfm_pktinfo->cfm_hdr->md_level < cfm_config->md_level) {
		dbprintf_cfm(LOG_INFO, "%s(%d) md level %d < mp index %d md level %d, drop\n",
			cfm_print_get_opcode_str(type), type, cfm_pktinfo->cfm_hdr->md_level, cfm_config->index, cfm_config->md_level);
		cfm_config->cfm_pkt_pm[(type<=CFM_PDU_MAX)?type:CFM_PDU_INVALID].drop++;
		if(cfm_pktinfo->rx_desc.logical_port_id == switch_get_wan_logical_portid())
			cfm_config->cfm_pkt_pm[(type<=CFM_PDU_MAX)?type:CFM_PDU_INVALID].drop_ani++;
		else
			cfm_config->cfm_pkt_pm[(type<=CFM_PDU_MAX)?type:CFM_PDU_INVALID].drop_uni++;
		if (type==CFM_PDU_CCM) cfm_config->defect_xcon_ccm_timestamp = cfm_util_get_time_in_us();
		return 0;
	}

	// MD level is equal below

	cfm_mmac[5] |= cfm_pktinfo->cfm_hdr->md_level;
	switch (type)
	{
	case CFM_PDU_CCM:
		if (cfm_util_get_mhf_creation(cfm_config) == MA_MHF_CREATION_NONE) { 	// MEP
			if (cfm_recv_ccm(cfm_config, cfm_pktinfo) != NULL) {
				int is_rdi = cfm_pktinfo->cfm_hdr->flags >> 7;
				if(is_rdi)
					cfm_config->cfm_pkt_pm[CFM_PDU_CCM].rx_rdi++;
				else
					cfm_config->cfm_pkt_pm[CFM_PDU_CCM].rx++;
				if(cfm_pktinfo->rx_desc.logical_port_id == switch_get_wan_logical_portid())
					cfm_config->cfm_pkt_pm[CFM_PDU_CCM].rx_ani++;
				else
					cfm_config->cfm_pkt_pm[CFM_PDU_CCM].rx_uni++;
			} else {
				cfm_config->cfm_pkt_pm[CFM_PDU_CCM].drop++;	// invalid ccm
				if(cfm_pktinfo->rx_desc.logical_port_id == switch_get_wan_logical_portid())
					cfm_config->cfm_pkt_pm[CFM_PDU_CCM].drop_ani++;
				else
					cfm_config->cfm_pkt_pm[CFM_PDU_CCM].drop_uni++;
			}
		} else {								// MIP
			cfm_config->cfm_pkt_pm[CFM_PDU_CCM].rx++;
			if(cfm_pktinfo->rx_desc.logical_port_id == switch_get_wan_logical_portid())
				cfm_config->cfm_pkt_pm[CFM_PDU_CCM].rx_ani++;
			else
				cfm_config->cfm_pkt_pm[CFM_PDU_CCM].rx_uni++;
			cfm_forward_cfm(cfm_config, pktinfo, cfm_pktinfo, CFM_PDU_CCM);
		}
		break;

	case CFM_PDU_LBR:
		if (memcmp(cfm_pktinfo->dst_mac_ptr, cfm_config->macaddr, IFHWADDRLEN) == 0) {
			// add to link list
			if (cfm_recv_lbr(cfm_config, cfm_pktinfo) != NULL) {
				cfm_config->cfm_pkt_pm[CFM_PDU_LBR].rx++;
				if(cfm_pktinfo->rx_desc.logical_port_id == switch_get_wan_logical_portid())
					cfm_config->cfm_pkt_pm[CFM_PDU_LBR].rx_ani++;
				else
					cfm_config->cfm_pkt_pm[CFM_PDU_LBR].rx_uni++;
			} else { // wrong xid, duplicated reply, pkt too long
				cfm_config->cfm_pkt_pm[CFM_PDU_LBR].drop++;
				if(cfm_pktinfo->rx_desc.logical_port_id == switch_get_wan_logical_portid())
					cfm_config->cfm_pkt_pm[CFM_PDU_LBR].drop_ani++;
				else
					cfm_config->cfm_pkt_pm[CFM_PDU_LBR].drop_uni++;
			}
		} else {
			if (cfm_util_get_mhf_creation(cfm_config) != MA_MHF_CREATION_NONE) { // MIP
				cfm_config->cfm_pkt_pm[CFM_PDU_LBR].rx++;
				if(cfm_pktinfo->rx_desc.logical_port_id == switch_get_wan_logical_portid())
					cfm_config->cfm_pkt_pm[CFM_PDU_LBR].rx_ani++;
				else
					cfm_config->cfm_pkt_pm[CFM_PDU_LBR].rx_uni++;
				cfm_forward_cfm(cfm_config, pktinfo, cfm_pktinfo, CFM_PDU_LBR);
			} else {
				cfm_config->cfm_pkt_pm[CFM_PDU_LBR].drop++;
				if(cfm_pktinfo->rx_desc.logical_port_id == switch_get_wan_logical_portid())
					cfm_config->cfm_pkt_pm[CFM_PDU_LBR].drop_ani++;
				else
					cfm_config->cfm_pkt_pm[CFM_PDU_LBR].drop_uni++;
			}
		}
		break;

	case CFM_PDU_LBM:
		if (memcmp(cfm_pktinfo->dst_mac_ptr, cfm_config->macaddr, IFHWADDRLEN) == 0) {
			cfm_config->cfm_pkt_pm[CFM_PDU_LBM].rx++;
			if(cfm_pktinfo->rx_desc.logical_port_id == switch_get_wan_logical_portid())
				cfm_config->cfm_pkt_pm[CFM_PDU_LBM].rx_ani++;
			else
				cfm_config->cfm_pkt_pm[CFM_PDU_LBM].rx_uni++;
			//release lbr link list in cfm_send_lbr()
			cfm_send_lbr(cfm_config, cfm_pktinfo);

		} else if (memcmp(cfm_pktinfo->dst_mac_ptr, cfm_mmac, IFHWADDRLEN) == 0) {
			cfm_config->cfm_pkt_pm[CFM_PDU_LBM].rx++;
			if(cfm_pktinfo->rx_desc.logical_port_id == switch_get_wan_logical_portid())
				cfm_config->cfm_pkt_pm[CFM_PDU_LBM].rx_ani++;
			else
				cfm_config->cfm_pkt_pm[CFM_PDU_LBM].rx_uni++;
			if (cfm_util_get_mhf_creation(cfm_config) == MA_MHF_CREATION_NONE) { // MEP
				if (cfm_config->mep_control2 & MEP_CONTROL2_LBR_ENABLE)
					cfm_send_lbr(cfm_config, cfm_pktinfo);
				else {
					cfm_config->cfm_pkt_pm[CFM_PDU_LBM].drop++;
					if(cfm_pktinfo->rx_desc.logical_port_id == switch_get_wan_logical_portid())
						cfm_config->cfm_pkt_pm[CFM_PDU_LBM].drop_ani++;
					else
						cfm_config->cfm_pkt_pm[CFM_PDU_LBM].drop_uni++;
				}
			} else { //MIP
				cfm_forward_cfm(cfm_config, pktinfo, cfm_pktinfo, CFM_PDU_LBM);
			}
		} else {
			if (cfm_util_get_mhf_creation(cfm_config) != MA_MHF_CREATION_NONE) { // MIP
				cfm_config->cfm_pkt_pm[CFM_PDU_LBM].rx++;
				if(cfm_pktinfo->rx_desc.logical_port_id == switch_get_wan_logical_portid())
					cfm_config->cfm_pkt_pm[CFM_PDU_LBM].rx_ani++;
				else
					cfm_config->cfm_pkt_pm[CFM_PDU_LBM].rx_uni++;
				cfm_forward_cfm(cfm_config, pktinfo, cfm_pktinfo, CFM_PDU_LBM);
			} else {
				cfm_config->cfm_pkt_pm[CFM_PDU_LBM].drop++;
				if(cfm_pktinfo->rx_desc.logical_port_id == switch_get_wan_logical_portid())
					cfm_config->cfm_pkt_pm[CFM_PDU_LBM].drop_ani++;
				else
					cfm_config->cfm_pkt_pm[CFM_PDU_LBM].drop_uni++;
			}
		}
		break;

	case CFM_PDU_LTR:
		if (memcmp(cfm_pktinfo->dst_mac_ptr, cfm_config->macaddr, IFHWADDRLEN) == 0) {
			if (cfm_util_get_mhf_creation(cfm_config) != MA_MHF_CREATION_NONE) { // MIP
				cfm_config->cfm_pkt_pm[CFM_PDU_LTR].rx++;
				if(cfm_pktinfo->rx_desc.logical_port_id == switch_get_wan_logical_portid())
					cfm_config->cfm_pkt_pm[CFM_PDU_LTR].rx_ani++;
				else
					cfm_config->cfm_pkt_pm[CFM_PDU_LTR].rx_uni++;
				cfm_forward_cfm(cfm_config, pktinfo, cfm_pktinfo, CFM_PDU_LTR);
			} else {
				// add to link list
				if (cfm_recv_ltr(cfm_config, cfm_pktinfo)!= NULL) {
					cfm_config->cfm_pkt_pm[CFM_PDU_LTR].rx++;
					if(cfm_pktinfo->rx_desc.logical_port_id == switch_get_wan_logical_portid())
						cfm_config->cfm_pkt_pm[CFM_PDU_LTR].rx_ani++;
					else
						cfm_config->cfm_pkt_pm[CFM_PDU_LTR].rx_uni++;
				} else{ // wrong xid, duplicated reply, pkt too long
					cfm_config->cfm_pkt_pm[CFM_PDU_LTR].drop++;
					if(cfm_pktinfo->rx_desc.logical_port_id == switch_get_wan_logical_portid())
						cfm_config->cfm_pkt_pm[CFM_PDU_LTR].drop_ani++;
					else
						cfm_config->cfm_pkt_pm[CFM_PDU_LTR].drop_uni++;
				}
			}
		} else {
			if (cfm_util_get_mhf_creation(cfm_config) != MA_MHF_CREATION_NONE) { // MIP
				cfm_config->cfm_pkt_pm[CFM_PDU_LTR].rx++;
				if(cfm_pktinfo->rx_desc.logical_port_id == switch_get_wan_logical_portid())
					cfm_config->cfm_pkt_pm[CFM_PDU_LTR].rx_ani++;
				else
					cfm_config->cfm_pkt_pm[CFM_PDU_LTR].rx_uni++;
				cfm_forward_cfm(cfm_config, pktinfo, cfm_pktinfo, CFM_PDU_LTR);
			}  else {
				cfm_config->cfm_pkt_pm[CFM_PDU_LTR].drop++;
				if(cfm_pktinfo->rx_desc.logical_port_id == switch_get_wan_logical_portid())
					cfm_config->cfm_pkt_pm[CFM_PDU_LTR].drop_ani++;
				else
					cfm_config->cfm_pkt_pm[CFM_PDU_LTR].drop_uni++;
			}
		}
		break;

	case CFM_PDU_LTM:
		pdu_ltm = (cfm_pdu_ltm_t *) p;
		if (pdu_ltm->ttl == 0)  {
			cfm_config->cfm_pkt_pm[CFM_PDU_LTM].drop++;
			if(cfm_pktinfo->rx_desc.logical_port_id == switch_get_wan_logical_portid())
				cfm_config->cfm_pkt_pm[CFM_PDU_LTM].drop_ani++;
			else
				cfm_config->cfm_pkt_pm[CFM_PDU_LTM].drop_uni++;
			break;
		}
		if (memcmp(pdu_ltm->target_addr, cfm_config->macaddr, IFHWADDRLEN) == 0) { // (target_addr == cfm_config->macaddr)
			cfm_config->cfm_pkt_pm[CFM_PDU_LTM].rx++;
			if(cfm_pktinfo->rx_desc.logical_port_id == switch_get_wan_logical_portid())
				cfm_config->cfm_pkt_pm[CFM_PDU_LTM].rx_ani++;
			else
				cfm_config->cfm_pkt_pm[CFM_PDU_LTM].rx_uni++;
			/*// 802.1ag: LTR ttl = LTM ttl
			// y.1731:  LTR ttl = LTM ttl-1
			if (omci_env_g.cfm_y1731_enable)*/
				pdu_ltm->ttl--;
			if (cfm_config->mep_control2 & MEP_CONTROL2_LTR_ENABLE)
				cfm_send_ltr(cfm_config, cfm_pktinfo);
			else {
				cfm_config->cfm_pkt_pm[CFM_PDU_LTM].drop++;
				if(cfm_pktinfo->rx_desc.logical_port_id == switch_get_wan_logical_portid())
					cfm_config->cfm_pkt_pm[CFM_PDU_LTM].drop_ani++;
				else
					cfm_config->cfm_pkt_pm[CFM_PDU_LTM].drop_uni++;
			}
		} else {
			struct switch_mac_tab_entry_t mac_tab_entry;
			if (cfm_util_get_mhf_creation(cfm_config) == MA_MHF_CREATION_NONE) { // not MIP
				cfm_config->cfm_pkt_pm[CFM_PDU_LTM].drop++;
				if(cfm_pktinfo->rx_desc.logical_port_id == switch_get_wan_logical_portid())
					cfm_config->cfm_pkt_pm[CFM_PDU_LTM].drop_ani++;
				else
					cfm_config->cfm_pkt_pm[CFM_PDU_LTM].drop_uni++;
				break;
			}
			memset(&mac_tab_entry, 0, sizeof(struct switch_mac_tab_entry_t));
			memcpy(mac_tab_entry.mac_addr, pdu_ltm->target_addr, IFHWADDRLEN);
			// Check if target MAC is in MAC table, if it is, forward LTM, otherwise drop LTM
			if (switch_hw_g.mac_entry_get == NULL ||
			    switch_hw_g.mac_entry_get(&mac_tab_entry) <0 ||
			    mac_tab_entry.port_bitmap == 0) {
				cfm_config->cfm_pkt_pm[CFM_PDU_LTM].drop++;
				if(cfm_pktinfo->rx_desc.logical_port_id == switch_get_wan_logical_portid())
					cfm_config->cfm_pkt_pm[CFM_PDU_LTM].drop_ani++;
				else
					cfm_config->cfm_pkt_pm[CFM_PDU_LTM].drop_uni++;
				break;
			}

			cfm_config->cfm_pkt_pm[CFM_PDU_LTM].rx++;
			if(cfm_pktinfo->rx_desc.logical_port_id == switch_get_wan_logical_portid())
				cfm_config->cfm_pkt_pm[CFM_PDU_LTM].rx_ani++;
			else
				cfm_config->cfm_pkt_pm[CFM_PDU_LTM].rx_uni++;

			if (cfm_config->mep_control2 & MEP_CONTROL2_LTR_ENABLE) {
				if (pdu_ltm->ttl > 0) {
					int is_found = 0, ltm_count = 0;
					cfm_pkt_ltm_entry_t *ltm_entry;
					// Update LTM table if record is there
					list_for_each_entry(ltm_entry, &cfm_config->cfm_recv_ltm_list, node) {
						ltm_count++;
						if ((ltm_entry->xid == ntohl(pdu_ltm->xid)) && (memcmp(&(ltm_entry->ltm_mac), cfm_pktinfo->src_mac_ptr, IFHWADDRLEN) == 0)) {
							is_found = 1;
							break;
						} else if (ltm_entry->xid == ntohl(pdu_ltm->xid)) {
							memcpy(&(ltm_entry->ltm_mac), cfm_pktinfo->src_mac_ptr, IFHWADDRLEN);
							is_found = 1;
							break;
						} else if (memcmp(&(ltm_entry->ltm_mac), cfm_pktinfo->src_mac_ptr, IFHWADDRLEN) == 0) {
							ltm_entry->xid = ntohl(pdu_ltm->xid);
							is_found = 1;
							break;
						}
					}
					// Add entry to LTM table for LTR mac lookup
					if (!is_found) {
						if (ltm_count > CFM_MAX_TTL) cfm_mp_flush_ltm_list(cfm_config);
						ltm_entry = malloc_safe(sizeof(cfm_pkt_ltm_entry_t));
						if (ltm_entry != NULL) {
							memcpy(&(ltm_entry->ltm_mac), cfm_pktinfo->src_mac_ptr, IFHWADDRLEN);
							ltm_entry->xid = ntohl(pdu_ltm->xid);
							list_add_tail(&ltm_entry->node, &cfm_config->cfm_recv_ltm_list);
						}
					}

					cfm_forward_cfm(cfm_config, pktinfo, cfm_pktinfo, CFM_PDU_LTM);
					/*if (!omci_env_g.cfm_y1731_enable) {
						// y.1731:  LTR ttl = LTM ttl-1
						// 802.1ag: LTR ttl = LTM ttl
						// add ttl back since it's -1 when forwarded ltm
						pdu_ltm->ttl++;
					}*/
				}
				cfm_send_ltr(cfm_config, cfm_pktinfo);
			} else {
				// forwrd without changing TTL if LTR is disabled
				cfm_forward_cfm(cfm_config, pktinfo, cfm_pktinfo, CFM_PDU_LTM);
			}
		}
		break;

	case CFM_PDU_AIS:
		if (((memcmp(cfm_pktinfo->dst_mac_ptr, cfm_config->macaddr, IFHWADDRLEN) == 0) ||
		    (memcmp(cfm_pktinfo->dst_mac_ptr, cfm_mmac, IFHWADDRLEN) == 0)) &&
		    omci_env_g.cfm_y1731_enable) {
			cfm_config->cfm_pkt_pm[CFM_PDU_AIS].rx++;
			if(cfm_pktinfo->rx_desc.logical_port_id == switch_get_wan_logical_portid())
				cfm_config->cfm_pkt_pm[CFM_PDU_AIS].rx_ani++;
			else
				cfm_config->cfm_pkt_pm[CFM_PDU_AIS].rx_uni++;
			dbprintf_cfm(LOG_INFO, "CFM_PDU_AIS received\n");
			// mark for AIS defect
			cfm_config->defect_ais_timestamp = cfm_util_get_time_in_us();
		} else {
			if (cfm_util_get_mhf_creation(cfm_config) != MA_MHF_CREATION_NONE) { // MIP
				cfm_config->cfm_pkt_pm[CFM_PDU_AIS].rx++;
				if(cfm_pktinfo->rx_desc.logical_port_id == switch_get_wan_logical_portid())
					cfm_config->cfm_pkt_pm[CFM_PDU_AIS].rx_ani++;
				else
					cfm_config->cfm_pkt_pm[CFM_PDU_AIS].rx_uni++;
				cfm_forward_cfm(cfm_config, pktinfo, cfm_pktinfo, CFM_PDU_AIS);
			} else {
				cfm_config->cfm_pkt_pm[CFM_PDU_AIS].drop++;
				if(cfm_pktinfo->rx_desc.logical_port_id == switch_get_wan_logical_portid())
					cfm_config->cfm_pkt_pm[CFM_PDU_AIS].drop_ani++;
				else
					cfm_config->cfm_pkt_pm[CFM_PDU_AIS].drop_uni++;
			}
		}
		break;

	case CFM_PDU_LCK:
		if (((memcmp(cfm_pktinfo->dst_mac_ptr, cfm_config->macaddr, IFHWADDRLEN) == 0) ||
		    (memcmp(cfm_pktinfo->dst_mac_ptr, cfm_mmac, IFHWADDRLEN) == 0)) &&
		    omci_env_g.cfm_y1731_enable) {
			cfm_config->cfm_pkt_pm[CFM_PDU_LCK].rx++;
			if(cfm_pktinfo->rx_desc.logical_port_id == switch_get_wan_logical_portid())
				cfm_config->cfm_pkt_pm[CFM_PDU_LCK].rx_ani++;
			else
				cfm_config->cfm_pkt_pm[CFM_PDU_LCK].rx_uni++;
			dbprintf_cfm(LOG_INFO, "CFM_PDU_LCK received\n");
			// mark for LCK recv
			cfm_config->recv_lck_timestamp = cfm_util_get_time_in_us();
		} else {
			if (cfm_util_get_mhf_creation(cfm_config) != MA_MHF_CREATION_NONE) { // MIP
				cfm_config->cfm_pkt_pm[CFM_PDU_LCK].rx++;
				if(cfm_pktinfo->rx_desc.logical_port_id == switch_get_wan_logical_portid())
					cfm_config->cfm_pkt_pm[CFM_PDU_LCK].rx_ani++;
				else
					cfm_config->cfm_pkt_pm[CFM_PDU_LCK].rx_uni++;
				cfm_forward_cfm(cfm_config, pktinfo, cfm_pktinfo, CFM_PDU_LCK);
			} else {
				cfm_config->cfm_pkt_pm[CFM_PDU_LCK].drop++;
				if(cfm_pktinfo->rx_desc.logical_port_id == switch_get_wan_logical_portid())
					cfm_config->cfm_pkt_pm[CFM_PDU_LCK].drop_ani++;
				else
					cfm_config->cfm_pkt_pm[CFM_PDU_LCK].drop_uni++;
			}
		}
		break;

	case CFM_PDU_LMR:
		if ((memcmp(cfm_pktinfo->dst_mac_ptr, cfm_config->macaddr, IFHWADDRLEN) == 0) &&
		omci_env_g.cfm_y1731_enable) {
			cfm_pkt_rmep_entry_t *entry;
			int found = 0;
			list_for_each_entry(entry, &cfm_config->cfm_recv_rmep_list, node) {
				if (memcmp(cfm_pktinfo->src_mac_ptr, entry->rmep_mac, IFHWADDRLEN) == 0) {
					found = 1;	// Check target_mac
					cfm_config->cfm_pkt_pm[CFM_PDU_LMR].rx++;
					if(cfm_pktinfo->rx_desc.logical_port_id == switch_get_wan_logical_portid())
						cfm_config->cfm_pkt_pm[CFM_PDU_LMR].rx_ani++;
					else
						cfm_config->cfm_pkt_pm[CFM_PDU_LMR].rx_uni++;
					break;
				}
			}
			if (!found) {
				cfm_config->cfm_pkt_pm[CFM_PDU_LMR].drop++;
				if(cfm_pktinfo->rx_desc.logical_port_id == switch_get_wan_logical_portid())
					cfm_config->cfm_pkt_pm[CFM_PDU_LMR].drop_ani++;
				else
					cfm_config->cfm_pkt_pm[CFM_PDU_LMR].drop_uni++;
				break;
			}
			cfm_pdu_lm_t *pdu_lmr = (cfm_pdu_lm_t *)p;
			entry->RxFCl_lmr++;
			if (entry->TxFCf_p == 0 && entry->RxFCf_p == 0
					&& entry->TxFCb_p == 0 ) {
				//the first LMR to start loss measurement
				entry->TxFCf_p = ntohl(pdu_lmr->TxFCf);
				entry->RxFCf_p = ntohl(pdu_lmr->RxFCf);
				entry->TxFCb_p = ntohl(pdu_lmr->TxFCb);
				entry->RxFCl_p = entry->RxFCl_ccm;
			}else if ( entry->TxFCf_c == 0 && entry->RxFCf_c == 0
					&& entry->TxFCb_c == 0 ) {

				//the second LMR to end loss measurement
				entry->TxFCf_c = ntohl(pdu_lmr->TxFCf);
				entry->RxFCf_c = ntohl(pdu_lmr->RxFCf);
				entry->TxFCb_c = ntohl(pdu_lmr->TxFCb);
				entry->RxFCl_c = entry->RxFCl_ccm;

				entry->eth_lm_single_tx_floss = entry->TxFCf_c - entry->TxFCf_p;
				entry->eth_lm_single_rx_floss = entry->RxFCf_c - entry->RxFCf_p;
				entry->eth_lm_single_tx_nloss = entry->TxFCb_c - entry->TxFCb_p;
				entry->eth_lm_single_rx_nloss = entry->RxFCl_c - entry->RxFCl_p;

				dbprintf_cfm(LOG_WARNING," farend tx=TxFCf_c(%d)-TxFCf_p(%d), rx=RxFCf_c(%d)-RxFCf_p(%d)\n",
						entry->TxFCf_c, entry->TxFCf_p,
						entry->RxFCf_c, entry->RxFCf_p);
				dbprintf_cfm(LOG_WARNING,"nearend tx=TxFCb_c(%d)-TxFCb_p(%d), rx=RxFCl_c(%d)-RxFCl_p(%d)\n",
						entry->TxFCb_c, entry->TxFCb_p,
						entry->RxFCl_c, entry->RxFCl_p);
				if ( entry->eth_lm_single_force_stop == 0 ) {
					int floss = entry->eth_lm_single_tx_floss - entry->eth_lm_single_rx_floss;
					int nloss = entry->eth_lm_single_tx_nloss - entry->eth_lm_single_rx_nloss;

					if (floss > 0 && entry->eth_lm_single_tx_floss > 0)
						entry->eth_lm_single_fratio = floss*10000/entry->eth_lm_single_tx_floss;
					if (nloss > 0 && entry->eth_lm_single_tx_nloss > 0)
						entry->eth_lm_single_nratio = nloss*10000/entry->eth_lm_single_tx_nloss;
					util_dbprintf(omci_env_g.debug_level, LOG_WARNING, 0,
						"\nLoss Mesaurement (single):\n"
						"md_level %d, mep %d, rmep %d:\n"
						"loss farend=%d (tx=%u rx=%u ratio=%u.%02u%%), nearend=%d (tx=%u rx=%u ratio=%u.%02u%%)\n",
						cfm_config->md_level, cfm_config->mep_id, entry->rmep_id,
						floss, entry->eth_lm_single_tx_floss, entry->eth_lm_single_rx_floss, entry->eth_lm_single_fratio/100, entry->eth_lm_single_fratio%100,
						nloss, entry->eth_lm_single_tx_nloss, entry->eth_lm_single_rx_nloss, entry->eth_lm_single_nratio/100, entry->eth_lm_single_nratio%100);
				}
				entry->eth_lm_single_going_on = 0;
				time( &entry->eth_lm_single_stop_time );
			}else{
				entry->TxFCf_p = ntohl(pdu_lmr->TxFCf);
				entry->RxFCf_p = ntohl(pdu_lmr->RxFCf);
				entry->TxFCb_p = ntohl(pdu_lmr->TxFCb);
				entry->RxFCf_p = entry->RxFCl_ccm;
				entry->TxFCf_c = 0;
				entry->RxFCf_c = 0;
				entry->TxFCb_c = 0;
				entry->RxFCl_c = 0;
			}
		} else {
			if (cfm_util_get_mhf_creation(cfm_config) != MA_MHF_CREATION_NONE) { // MIP
				cfm_config->cfm_pkt_pm[CFM_PDU_LMR].rx++;
				if(cfm_pktinfo->rx_desc.logical_port_id == switch_get_wan_logical_portid())
					cfm_config->cfm_pkt_pm[CFM_PDU_LMR].rx_ani++;
				else
					cfm_config->cfm_pkt_pm[CFM_PDU_LMR].rx_uni++;
				cfm_forward_cfm(cfm_config, pktinfo, cfm_pktinfo, CFM_PDU_LMR);
			} else {
				cfm_config->cfm_pkt_pm[CFM_PDU_LMR].drop++;
				if(cfm_pktinfo->rx_desc.logical_port_id == switch_get_wan_logical_portid())
					cfm_config->cfm_pkt_pm[CFM_PDU_LMR].drop_ani++;
				else
					cfm_config->cfm_pkt_pm[CFM_PDU_LMR].drop_uni++;
			}
		}
		break;

	case CFM_PDU_LMM:
		if (((memcmp(cfm_pktinfo->dst_mac_ptr, cfm_config->macaddr, IFHWADDRLEN) == 0) ||
		    (memcmp(cfm_pktinfo->dst_mac_ptr, cfm_mmac, IFHWADDRLEN) == 0)) &&
		    omci_env_g.cfm_y1731_enable) {
			cfm_pkt_rmep_entry_t *entry = NULL;
			//cfm_pdu_lm_t *pdu_lmm = (cfm_pdu_lm_t *) p;
			cfm_config->cfm_pkt_pm[CFM_PDU_LMM].rx++;
			if(cfm_pktinfo->rx_desc.logical_port_id == switch_get_wan_logical_portid())
				cfm_config->cfm_pkt_pm[CFM_PDU_LMM].rx_ani++;
			else
				cfm_config->cfm_pkt_pm[CFM_PDU_LMM].rx_uni++;
			list_for_each_entry(entry, &cfm_config->cfm_recv_rmep_list, node) {
				if (memcmp(entry->rmep_mac, cfm_pktinfo->src_mac_ptr, IFHWADDRLEN) == 0) {
					entry->RxFCl_lmm++;
					cfm_send_lmr(cfm_config, entry, cfm_pktinfo);
					break;
				}
			}
		} else {
			if (cfm_util_get_mhf_creation(cfm_config) != MA_MHF_CREATION_NONE) { // MIP
				cfm_config->cfm_pkt_pm[CFM_PDU_LMM].rx++;
				if(cfm_pktinfo->rx_desc.logical_port_id == switch_get_wan_logical_portid())
					cfm_config->cfm_pkt_pm[CFM_PDU_LMM].rx_ani++;
				else
					cfm_config->cfm_pkt_pm[CFM_PDU_LMM].rx_uni++;
				cfm_forward_cfm(cfm_config, pktinfo, cfm_pktinfo, CFM_PDU_LMM);
			} else {
				cfm_config->cfm_pkt_pm[CFM_PDU_LMM].drop++;
				if(cfm_pktinfo->rx_desc.logical_port_id == switch_get_wan_logical_portid())
					cfm_config->cfm_pkt_pm[CFM_PDU_LMM].drop_ani++;
				else
					cfm_config->cfm_pkt_pm[CFM_PDU_LMM].drop_uni++;
			}
		}
		break;

	case CFM_PDU_1DM:
		if (((memcmp(cfm_pktinfo->dst_mac_ptr, cfm_config->macaddr, IFHWADDRLEN) == 0) ||
		   (memcmp(cfm_pktinfo->dst_mac_ptr, cfm_mmac, IFHWADDRLEN) == 0)) &&
		   omci_env_g.cfm_y1731_enable) {
			char *signstr;
			cfm_pdu_1dm_t *pdu_1dm = (cfm_pdu_1dm_t *) p;
			cfm_config->cfm_pkt_pm[CFM_PDU_1DM].rx++;
			if(cfm_pktinfo->rx_desc.logical_port_id == switch_get_wan_logical_portid())
				cfm_config->cfm_pkt_pm[CFM_PDU_1DM].rx_ani++;
			else
				cfm_config->cfm_pkt_pm[CFM_PDU_1DM].rx_uni++;
			//network byte order to us
			cfm_util_ieee1588_64bit_to_us(pdu_1dm->TxTimestampf, &cfm_config->TxTimestampf);
			cfm_config->RxTimestampf = cfm_util_get_time_in_us();

			if (cfm_config->RxTimestampf > cfm_config->TxTimestampf) {
				signstr="";
				cfm_config->frame_delay=(cfm_config->RxTimestampf-cfm_config->TxTimestampf);
			} else {
				signstr="-";
				cfm_config->frame_delay=(cfm_config->TxTimestampf-cfm_config->RxTimestampf);
			}

			util_dbprintf(omci_env_g.debug_level_cfm, LOG_ERR, 0, "\nOneway Dely Measurement:\n");

			util_dbprintf(omci_env_g.debug_level_cfm, LOG_ERR, 0, "%d bytes 1DM from %s for mdl %d, mep %d\n",
				cfm_pktinfo->frame_len, util_str_macaddr(cfm_pktinfo->src_mac_ptr),
				cfm_pktinfo->cfm_hdr->md_level, cfm_config->mep_id);

			util_dbprintf(omci_env_g.debug_level_cfm, LOG_ERR, 0, "delay time=%s%llu.%03llums\n",
				signstr, cfm_config->frame_delay/1000, cfm_config->frame_delay%1000);

			util_dbprintf(omci_env_g.debug_level_cfm, LOG_WARNING, 0, "\nRxTimestampb %llusec,%lluus, TxTimestampf:%llusec,%lluus\n",
				cfm_config->RxTimestampf/1000000, cfm_config->RxTimestampf%1000000,
				cfm_config->TxTimestampf/1000000, cfm_config->TxTimestampf%1000000);

		} else {
			if (cfm_util_get_mhf_creation(cfm_config) != MA_MHF_CREATION_NONE) { // MIP
				cfm_config->cfm_pkt_pm[CFM_PDU_1DM].rx++;
				if(cfm_pktinfo->rx_desc.logical_port_id == switch_get_wan_logical_portid())
					cfm_config->cfm_pkt_pm[CFM_PDU_1DM].rx_ani++;
				else
					cfm_config->cfm_pkt_pm[CFM_PDU_1DM].rx_uni++;
				cfm_forward_cfm(cfm_config, pktinfo, cfm_pktinfo, CFM_PDU_1DM);
			} else {
				cfm_config->cfm_pkt_pm[CFM_PDU_1DM].drop++;
				if(cfm_pktinfo->rx_desc.logical_port_id == switch_get_wan_logical_portid())
					cfm_config->cfm_pkt_pm[CFM_PDU_1DM].drop_ani++;
				else
					cfm_config->cfm_pkt_pm[CFM_PDU_1DM].drop_uni++;
			}
		}
		break;

	case CFM_PDU_TST:
		if(((memcmp(cfm_pktinfo->dst_mac_ptr, cfm_config->macaddr, IFHWADDRLEN) == 0) ||
		   (memcmp(cfm_pktinfo->dst_mac_ptr, cfm_mmac, IFHWADDRLEN) == 0)) &&
		   omci_env_g.cfm_y1731_enable) {

			unsigned int crc32_offset=0, err=0, pkt_offset=0;
			float err_rate=0;
			unsigned short prbs_data_len=0;
			unsigned int i, calculate_crc32, crc32_value_len=0, append_crc32=0;
			unsigned char *pattern_type_str;
			unsigned char *payload_str;

			cfm_pdu_tst_t *pdu_tst = (cfm_pdu_tst_t *) p;
			cfm_tlv_test_t *cfm_tlv_test;

			cfm_tlv_test= ( cfm_tlv_test_t *) (p + sizeof(cfm_pdu_tst_t));

			cfm_config->cfm_pkt_pm[CFM_PDU_TST].rx++;
			if(cfm_pktinfo->rx_desc.logical_port_id == switch_get_wan_logical_portid())
				cfm_config->cfm_pkt_pm[CFM_PDU_TST].rx_ani++;
			else
				cfm_config->cfm_pkt_pm[CFM_PDU_TST].rx_uni++;
			crc32_offset=IFHWADDRLEN*2+cfm_pktinfo->vlan_num*VLAN_TAG_OFFSET+ETHER_TYPE_LEN+sizeof(cfm_hdr_t)+sizeof(cfm_pdu_tst_t);
			cfm_config->tst_sequence_number=ntohl(pdu_tst->sequence_number);

			//prepare for prbs31 check
			// type size=1, crc32 value size=4
			switch(cfm_tlv_test->pattern_type) {
			case 0:	//null without crc32
				pattern_type_str="null data only";
				payload_str="null data len";
				prbs_data_len=cfm_tlv_test->length-1;
				crc32_value_len=0;

				//check null data
				pkt_offset=crc32_offset + sizeof(cfm_tlv_test_t);
				for(i=0; i < prbs_data_len; i++) {
					err+=bitcount8(*(cfm_pktinfo->frame_ptr + pkt_offset +i));
				}

				if(err) {
					//(err*100)/(prbs_data_len*8);
					err_rate=(err*12.5)/prbs_data_len;
				}
			break;
			case 1:	//null with crc32
				pattern_type_str="null data + crc32";
				payload_str="null data len";
				prbs_data_len=cfm_tlv_test->length-5;
				crc32_value_len=4;

				//check null data
				pkt_offset=crc32_offset + sizeof(cfm_tlv_test_t);
				for(i=0; i < prbs_data_len; i++) {
					err+=bitcount8(*(cfm_pktinfo->frame_ptr + pkt_offset +i));
				}

				if(err) {
					//(err*100)/(prbs_data_len*8);
					err_rate=(err*12.5)/prbs_data_len;
				}
			break;
			case 2:	//prbs without crc32
				pattern_type_str="prbs only";
				payload_str="prbs data len";
				prbs_data_len=cfm_tlv_test->length-1;
				crc32_value_len=0;

				//check prbs31
				err=prbs31_checker(prbs_data_len, cfm_pktinfo->frame_ptr + crc32_offset + sizeof(cfm_tlv_test_t));
				if(err) {
					//(err*100)/(prbs_data_len*8);
					err_rate=(err*12.5)/prbs_data_len;
				}
			break;
			case 3: //prbs with crc32
				pattern_type_str="prbs + crc32";
				payload_str="prbs data len";
				prbs_data_len=cfm_tlv_test->length-5;
				crc32_value_len=4;

				//check prbs31
				err=prbs31_checker(prbs_data_len, cfm_pktinfo->frame_ptr+crc32_offset+sizeof(cfm_tlv_test_t));
				if(err) {
					//(err*100)/(prbs_data_len*8);
					err_rate=(err*12.5)/prbs_data_len;
				}
			break;

			default:
				pattern_type_str="parameter error";
				payload_str="null data len";
				prbs_data_len=cfm_tlv_test->length-1;
				crc32_value_len=0;

				//check null data
				//memset(frame_ptr+pkt_offset, 0, prbs_data_len);
				dbprintf_cfm(LOG_ERR, "Input parameter error, should be 0~3\n");
			}

			util_dbprintf(omci_env_g.debug_level_cfm, LOG_ERR, 0,
				"\nTST (%s):\n", pattern_type_str);

			util_dbprintf(omci_env_g.debug_level_cfm, LOG_ERR, 0,
				"%d bytes TST from %s for mdl %d, mep %d\n",
				cfm_pktinfo->frame_len, util_str_macaddr(cfm_pktinfo->src_mac_ptr),
				cfm_pktinfo->cfm_hdr->md_level, cfm_config->mep_id);

			util_dbprintf(omci_env_g.debug_level_cfm, LOG_ERR, 0,
				"seq=0x%04X, pattern=%d, %s=%d, biterr=%d (rate=%f%%)\n",
				cfm_config->tst_sequence_number, cfm_tlv_test->pattern_type, payload_str, prbs_data_len, err, err_rate);

			//check crc32
			if(crc32_value_len) {
				//ethernet crc32
				calculate_crc32=crc_le_update(0xFFFFFFFF, cfm_pktinfo->frame_ptr+crc32_offset, cfm_tlv_test->length-4+3);
				calculate_crc32=htonl(~calculate_crc32);

				memcpy(&append_crc32, cfm_pktinfo->frame_ptr+(crc32_offset+cfm_tlv_test->length-1), sizeof(calculate_crc32));
				append_crc32=ntohl(append_crc32);
				if (memcmp(&calculate_crc32, cfm_pktinfo->frame_ptr+(crc32_offset+cfm_tlv_test->length-1), sizeof(calculate_crc32)) != 0) {
					util_dbprintf(omci_env_g.debug_level_cfm, LOG_ERR, 0, "crc32 mismatch (calculated=0x%04X, pdu=0x%04X)\n",
						calculate_crc32, append_crc32);
				} else {
					util_dbprintf(omci_env_g.debug_level_cfm, LOG_ERR, 0, "crc32 match (calculated=0x%04X, pdu=0x%04X)\n",
						calculate_crc32, append_crc32);
				}
				dbprintf_cfm(LOG_WARNING, "crc32_offset=%d\n", crc32_offset);
			}

		} else {
			if (cfm_util_get_mhf_creation(cfm_config) != MA_MHF_CREATION_NONE) { // MIP
				cfm_config->cfm_pkt_pm[CFM_PDU_TST].rx++;
				if(cfm_pktinfo->rx_desc.logical_port_id == switch_get_wan_logical_portid())
					cfm_config->cfm_pkt_pm[CFM_PDU_TST].rx_ani++;
				else
					cfm_config->cfm_pkt_pm[CFM_PDU_TST].rx_uni++;
				cfm_forward_cfm(cfm_config, pktinfo, cfm_pktinfo, CFM_PDU_TST);
			} else {
				cfm_config->cfm_pkt_pm[CFM_PDU_TST].drop++;
				if(cfm_pktinfo->rx_desc.logical_port_id == switch_get_wan_logical_portid())
					cfm_config->cfm_pkt_pm[CFM_PDU_TST].drop_ani++;
				else
					cfm_config->cfm_pkt_pm[CFM_PDU_TST].drop_uni++;
			}
		}
		break;

	case CFM_PDU_DMR:
		if ((memcmp(cfm_pktinfo->dst_mac_ptr, cfm_config->macaddr, IFHWADDRLEN) == 0) &&
		omci_env_g.cfm_y1731_enable) {
			if (memcmp(cfm_pktinfo->src_mac_ptr, cfm_config->cfm_send_dmm_mac, IFHWADDRLEN) == 0) { // Check target_mac
				unsigned long long round_trip_time=0, response_time=0;

				cfm_pdu_dm_t *pdu_dmr = (cfm_pdu_dm_t *) p;
				cfm_config->cfm_pkt_pm[CFM_PDU_DMR].rx++;
				if(cfm_pktinfo->rx_desc.logical_port_id == switch_get_wan_logical_portid())
					cfm_config->cfm_pkt_pm[CFM_PDU_DMR].rx_ani++;
				else
					cfm_config->cfm_pkt_pm[CFM_PDU_DMR].rx_uni++;
				memcpy(&cfm_config->cfm_recv_dmr_mac, cfm_pktinfo->src_mac_ptr, IFHWADDRLEN);

				//network byte order to us
				cfm_util_ieee1588_64bit_to_us(pdu_dmr->TxTimestampf, &cfm_config->TxTimestampf);
				cfm_util_ieee1588_64bit_to_us(pdu_dmr->RxTimestampf, &cfm_config->RxTimestampf);
				cfm_util_ieee1588_64bit_to_us(pdu_dmr->TxTimestampb, &cfm_config->TxTimestampb);
				cfm_config->RxTimestampb = cfm_util_get_time_in_us();

				if (cfm_config->TxTimestampb && cfm_config->RxTimestampf) {

					if (cfm_config->RxTimestampb > cfm_config->TxTimestampf)
						round_trip_time=cfm_config->RxTimestampb-cfm_config->TxTimestampf;
					else
						round_trip_time=cfm_config->TxTimestampf-cfm_config->RxTimestampb;
					util_dbprintf(omci_env_g.debug_level_cfm, LOG_WARNING, 0,
						"\nround_trip_time=%llusec,%lluus, RxTimestampb %llusec,%lluus, TxTimestampf:%llusec,%lluus\n",
						round_trip_time/1000000, round_trip_time%1000000,
						cfm_config->RxTimestampb/1000000, cfm_config->RxTimestampb%1000000,
						cfm_config->TxTimestampf/1000000, cfm_config->TxTimestampf%1000000);

					if ( cfm_config->RxTimestampf > cfm_config->TxTimestampb)
						response_time=cfm_config->RxTimestampf-cfm_config->TxTimestampb;
					else
						response_time=cfm_config->TxTimestampb-cfm_config->RxTimestampf;
					util_dbprintf(omci_env_g.debug_level_cfm, LOG_WARNING, 0,
						"response_time=%llusec,%lluus, RxTimestampf %llusec,%lluus, TxTimestampb:%llusec,%lluus\n",
						response_time/1000000, response_time%1000000,
						cfm_config->RxTimestampf/1000000, cfm_config->RxTimestampf%1000000,
						cfm_config->TxTimestampb/1000000, cfm_config->TxTimestampb%1000000);

					cfm_config->frame_delay=round_trip_time-response_time;

					if(round_trip_time<cfm_config->round_trip_min || !cfm_config->round_trip_min) cfm_config->round_trip_min = round_trip_time;
					if(round_trip_time>cfm_config->round_trip_max) cfm_config->round_trip_max = round_trip_time;
					cfm_config->round_trip_avg = (cfm_config->round_trip_avg+round_trip_time)/((cfm_config->round_trip_avg)?2:1);
					if(response_time<cfm_config->response_time_min || !cfm_config->response_time_min) cfm_config->response_time_min = response_time;
					if(response_time>cfm_config->response_time_max) cfm_config->response_time_max = response_time;
					cfm_config->response_time_avg = (cfm_config->response_time_avg+response_time)/((cfm_config->response_time_avg)?2:1);
				} else {
					if (cfm_config->RxTimestampb > cfm_config->TxTimestampf)
						cfm_config->frame_delay=(cfm_config->RxTimestampb-cfm_config->TxTimestampf);
					else
						cfm_config->frame_delay=(cfm_config->TxTimestampf-cfm_config->RxTimestampb);
				}
				if(cfm_config->frame_delay<cfm_config->frame_delay_min || !cfm_config->frame_delay_min) cfm_config->frame_delay_min = cfm_config->frame_delay;
				if(cfm_config->frame_delay>cfm_config->frame_delay_max) cfm_config->frame_delay_max = cfm_config->frame_delay;
				cfm_config->frame_delay_avg = (cfm_config->frame_delay_avg+cfm_config->frame_delay)/((cfm_config->frame_delay_avg)?2:1);

				util_dbprintf(omci_env_g.debug_level_cfm, LOG_WARNING, 0, "%d bytes DMR from %s: time=%llu.%03llums ",
					cfm_pktinfo->frame_len,
					util_str_macaddr(cfm_pktinfo->src_mac_ptr),
					cfm_config->frame_delay/1000, cfm_config->frame_delay%1000);

				{
					unsigned long long near_delay, far_delay;
					unsigned char *near_sign="", *far_sign="";
					if ( cfm_config->RxTimestampf > cfm_config->TxTimestampf) {
						far_delay = cfm_config->RxTimestampf-cfm_config->TxTimestampf;
					} else {
						far_delay = cfm_config->TxTimestampf-cfm_config->RxTimestampf;
						far_sign="-";
					}
					if ( cfm_config->RxTimestampb > cfm_config->TxTimestampb) {
						near_delay = cfm_config->RxTimestampb-cfm_config->TxTimestampb;
					} else {
						near_delay = cfm_config->TxTimestampb-cfm_config->RxTimestampb;
						near_sign="-";
					}
					util_dbprintf(omci_env_g.debug_level_cfm, LOG_WARNING, 0,
						"(far delay=%s%llu.%03llums, near delay=%s%llu.%03llums)\n",
						far_sign, far_delay/1000, far_delay%1000,
						near_sign, near_delay/1000, near_delay%1000);
				}

			} else {
				cfm_config->cfm_pkt_pm[CFM_PDU_DMR].drop++;
				if(cfm_pktinfo->rx_desc.logical_port_id == switch_get_wan_logical_portid())
					cfm_config->cfm_pkt_pm[CFM_PDU_DMR].drop_ani++;
				else
					cfm_config->cfm_pkt_pm[CFM_PDU_DMR].drop_uni++;
			}
		} else {
			if (cfm_util_get_mhf_creation(cfm_config) != MA_MHF_CREATION_NONE) { // MIP
				cfm_config->cfm_pkt_pm[CFM_PDU_DMR].rx++;
				if(cfm_pktinfo->rx_desc.logical_port_id == switch_get_wan_logical_portid())
					cfm_config->cfm_pkt_pm[CFM_PDU_DMR].rx_ani++;
				else
					cfm_config->cfm_pkt_pm[CFM_PDU_DMR].rx_uni++;
				cfm_forward_cfm(cfm_config, pktinfo, cfm_pktinfo, CFM_PDU_DMR);
			} else {
				cfm_config->cfm_pkt_pm[CFM_PDU_DMR].drop++;
				if(cfm_pktinfo->rx_desc.logical_port_id == switch_get_wan_logical_portid())
					cfm_config->cfm_pkt_pm[CFM_PDU_DMR].drop_ani++;
				else
					cfm_config->cfm_pkt_pm[CFM_PDU_DMR].drop_uni++;
			}
		}
		break;

	case CFM_PDU_DMM:
		if (((memcmp(cfm_pktinfo->dst_mac_ptr, cfm_config->macaddr, IFHWADDRLEN) == 0) ||
		    (memcmp(cfm_pktinfo->dst_mac_ptr, cfm_mmac, IFHWADDRLEN) == 0)) &&
		    omci_env_g.cfm_y1731_enable) {
			cfm_pdu_dm_t *pdu_dmm = (cfm_pdu_dm_t *) p;

			cfm_config->cfm_pkt_pm[CFM_PDU_DMM].rx++;
			if(cfm_pktinfo->rx_desc.logical_port_id == switch_get_wan_logical_portid())
				cfm_config->cfm_pkt_pm[CFM_PDU_DMM].rx_ani++;
			else
				cfm_config->cfm_pkt_pm[CFM_PDU_DMM].rx_uni++;

			//network byte order to us
			cfm_util_ieee1588_64bit_to_us(pdu_dmm->TxTimestampf, &cfm_config->TxTimestampf);
			cfm_config->RxTimestampf = cfm_util_get_time_in_us();
			cfm_config->TxTimestampb = 0;
			cfm_config->RxTimestampb = 0;
			cfm_send_dmr(cfm_config, cfm_pktinfo);
		} else {
			if (cfm_util_get_mhf_creation(cfm_config) != MA_MHF_CREATION_NONE) { // MIP
				cfm_config->cfm_pkt_pm[CFM_PDU_DMM].rx++;
				if(cfm_pktinfo->rx_desc.logical_port_id == switch_get_wan_logical_portid())
					cfm_config->cfm_pkt_pm[CFM_PDU_DMM].rx_ani++;
				else
					cfm_config->cfm_pkt_pm[CFM_PDU_DMM].rx_uni++;
				cfm_forward_cfm(cfm_config, pktinfo, cfm_pktinfo, CFM_PDU_DMM);
			} else {
				cfm_config->cfm_pkt_pm[CFM_PDU_DMM].drop++;
				if(cfm_pktinfo->rx_desc.logical_port_id == switch_get_wan_logical_portid())
					cfm_config->cfm_pkt_pm[CFM_PDU_DMM].drop_ani++;
				else
					cfm_config->cfm_pkt_pm[CFM_PDU_DMM].drop_uni++;
			}
		}
		break;

	case CFM_PDU_1SL:
		if (((memcmp(cfm_pktinfo->dst_mac_ptr, cfm_config->macaddr, IFHWADDRLEN) == 0) ||
		    (memcmp(cfm_pktinfo->dst_mac_ptr, cfm_mmac, IFHWADDRLEN) == 0)) &&
		    omci_env_g.cfm_y1731_enable) {
		   	int found = 0;
			list_for_each_entry(entry, &cfm_config->cfm_recv_rmep_list, node) {
				if (memcmp(entry->rmep_mac, cfm_pktinfo->src_mac_ptr, IFHWADDRLEN) == 0) {
					found = 1;
					break;
				}
			}
			if (found)	{ // pkt sender is on our rmep list
				cfm_config->cfm_pkt_pm[CFM_PDU_1SL].rx++;
				if(cfm_pktinfo->rx_desc.logical_port_id == switch_get_wan_logical_portid())
					cfm_config->cfm_pkt_pm[CFM_PDU_1SL].rx_ani++;
				else
					cfm_config->cfm_pkt_pm[CFM_PDU_1SL].rx_uni++;
				cfm_recv_1sl(cfm_config, cfm_pktinfo);
			} else {
				cfm_config->cfm_pkt_pm[CFM_PDU_1SL].drop++;
				if(cfm_pktinfo->rx_desc.logical_port_id == switch_get_wan_logical_portid())
					cfm_config->cfm_pkt_pm[CFM_PDU_1SL].drop_ani++;
				else
					cfm_config->cfm_pkt_pm[CFM_PDU_1SL].drop_uni++;
			}
		} else {
			if (cfm_util_get_mhf_creation(cfm_config) != MA_MHF_CREATION_NONE) { // MIP
				cfm_config->cfm_pkt_pm[CFM_PDU_1SL].rx++;
				if(cfm_pktinfo->rx_desc.logical_port_id == switch_get_wan_logical_portid())
					cfm_config->cfm_pkt_pm[CFM_PDU_1SL].rx_ani++;
				else
					cfm_config->cfm_pkt_pm[CFM_PDU_1SL].rx_uni++;
				cfm_forward_cfm(cfm_config, pktinfo, cfm_pktinfo, CFM_PDU_1SL);
			} else {
				cfm_config->cfm_pkt_pm[CFM_PDU_1SL].drop++;
				if(cfm_pktinfo->rx_desc.logical_port_id == switch_get_wan_logical_portid())
					cfm_config->cfm_pkt_pm[CFM_PDU_1SL].drop_ani++;
				else
					cfm_config->cfm_pkt_pm[CFM_PDU_1SL].drop_uni++;
			}
		}
		break;

	case CFM_PDU_SLR:
		if ((memcmp(cfm_pktinfo->dst_mac_ptr, cfm_config->macaddr, IFHWADDRLEN) == 0) &&
		omci_env_g.cfm_y1731_enable) {
			int found = 0;
			list_for_each_entry(entry, &cfm_config->cfm_recv_rmep_list, node) {
				if (memcmp(cfm_pktinfo->src_mac_ptr, entry->rmep_mac, IFHWADDRLEN) == 0) {
					found = 1;
					break;
				}
			}
			if (found) {	// pkt sender is on our rmep list
				cfm_pdu_sl_t *pdu_slr = (cfm_pdu_sl_t *) p;
				if ( entry->eth_slm_single_going_on == 0 ) {
					dbprintf_cfm(LOG_ERR,"SLM single test is not going on.\n");
					break;
				}
				if ((cfm_config->mep_id != htons(pdu_slr->source_mep_id)) || (entry->test_id_slm != ntohl(pdu_slr->test_id))) {
					cfm_config->cfm_pkt_pm[CFM_PDU_SLR].drop++;
					if(cfm_pktinfo->rx_desc.logical_port_id == switch_get_wan_logical_portid())
						cfm_config->cfm_pkt_pm[CFM_PDU_SLR].drop_ani++;
					else
						cfm_config->cfm_pkt_pm[CFM_PDU_SLR].drop_uni++;
				} else {
					cfm_config->cfm_pkt_pm[CFM_PDU_SLR].rx++;
					if(cfm_pktinfo->rx_desc.logical_port_id == switch_get_wan_logical_portid())
						cfm_config->cfm_pkt_pm[CFM_PDU_SLR].rx_ani++;
					else
						cfm_config->cfm_pkt_pm[CFM_PDU_SLR].rx_uni++;
					//cfm_pkt_slm_entry_t *entry = NULL;
					entry->RxFCl_slr++;
					entry->TxFCf_slr = ntohl(pdu_slr->TxFCf);
					entry->TxFCb_slr = ntohl(pdu_slr->TxFCb);
					if (entry->TxFCf_p == 0
							&& entry->TxFCb_p == 0
							&& entry->RxFCl_p == 0) {
						entry->TxFCf_p = entry->TxFCf_slr;
						entry->TxFCb_p = entry->TxFCb_slr;
						entry->RxFCl_p = entry->RxFCl_slr;
					}
				}
			} else {
				cfm_config->cfm_pkt_pm[CFM_PDU_SLR].drop++;
				if(cfm_pktinfo->rx_desc.logical_port_id == switch_get_wan_logical_portid())
					cfm_config->cfm_pkt_pm[CFM_PDU_SLR].drop_ani++;
				else
					cfm_config->cfm_pkt_pm[CFM_PDU_SLR].drop_uni++;
			}
		} else {
			if (cfm_util_get_mhf_creation(cfm_config) != MA_MHF_CREATION_NONE) { // MIP
				cfm_config->cfm_pkt_pm[CFM_PDU_SLR].rx++;
				if(cfm_pktinfo->rx_desc.logical_port_id == switch_get_wan_logical_portid())
					cfm_config->cfm_pkt_pm[CFM_PDU_SLR].rx_ani++;
				else
					cfm_config->cfm_pkt_pm[CFM_PDU_SLR].rx_uni++;
				cfm_forward_cfm(cfm_config, pktinfo, cfm_pktinfo, CFM_PDU_SLR);
			} else {
				cfm_config->cfm_pkt_pm[CFM_PDU_SLR].drop++;
				if(cfm_pktinfo->rx_desc.logical_port_id == switch_get_wan_logical_portid())
					cfm_config->cfm_pkt_pm[CFM_PDU_SLR].drop_ani++;
				else
					cfm_config->cfm_pkt_pm[CFM_PDU_SLR].drop_uni++;
			}
		}
		break;

	case CFM_PDU_SLM:
		if (((memcmp(cfm_pktinfo->dst_mac_ptr, cfm_config->macaddr, IFHWADDRLEN) == 0) ||
		    (memcmp(cfm_pktinfo->dst_mac_ptr, cfm_mmac, IFHWADDRLEN) == 0)) &&
		    omci_env_g.cfm_y1731_enable) {
		    	int found = 0;
			cfm_pkt_rmep_entry_t *entry = NULL;
			list_for_each_entry(entry, &cfm_config->cfm_recv_rmep_list, node) {
				if (memcmp(entry->rmep_mac, cfm_pktinfo->src_mac_ptr, IFHWADDRLEN) == 0) {
					found = 1;
					break;
				}
			}
			if (found) {	// pkt sender is on our rmep list
				cfm_config->cfm_pkt_pm[CFM_PDU_SLM].rx++;
				if(cfm_pktinfo->rx_desc.logical_port_id == switch_get_wan_logical_portid())
					cfm_config->cfm_pkt_pm[CFM_PDU_SLM].rx_ani++;
				else
					cfm_config->cfm_pkt_pm[CFM_PDU_SLM].rx_uni++;
				entry->RxFCl_slm++;
				entry->TxFCl_slr++;
				cfm_send_slr(cfm_config, entry, cfm_pktinfo);
			} else {
				cfm_config->cfm_pkt_pm[CFM_PDU_SLM].drop++;
				if(cfm_pktinfo->rx_desc.logical_port_id == switch_get_wan_logical_portid())
					cfm_config->cfm_pkt_pm[CFM_PDU_SLM].drop_ani++;
				else
					cfm_config->cfm_pkt_pm[CFM_PDU_SLM].drop_uni++;
			}
		} else {
			if (cfm_util_get_mhf_creation(cfm_config) != MA_MHF_CREATION_NONE) { // MIP
				cfm_config->cfm_pkt_pm[CFM_PDU_SLM].rx++;
				if(cfm_pktinfo->rx_desc.logical_port_id == switch_get_wan_logical_portid())
					cfm_config->cfm_pkt_pm[CFM_PDU_SLM].rx_ani++;
				else
					cfm_config->cfm_pkt_pm[CFM_PDU_SLM].rx_uni++;
				cfm_forward_cfm(cfm_config, pktinfo, cfm_pktinfo, CFM_PDU_SLM);
			} else {
				cfm_config->cfm_pkt_pm[CFM_PDU_SLM].drop++;
				if(cfm_pktinfo->rx_desc.logical_port_id == switch_get_wan_logical_portid())
					cfm_config->cfm_pkt_pm[CFM_PDU_SLM].drop_ani++;
				else
					cfm_config->cfm_pkt_pm[CFM_PDU_SLM].drop_uni++;
			}
		}
		break;

	default:
		if ((memcmp(cfm_pktinfo->dst_mac_ptr, cfm_config->macaddr, IFHWADDRLEN) == 0) ||
		    (memcmp(cfm_pktinfo->dst_mac_ptr, cfm_mmac, IFHWADDRLEN) == 0)) {
			dbprintf_cfm(LOG_ERR, "Unsupported CFM frame received: %s(%d)\n", cfm_print_get_opcode_str(type), type);
			cfm_config->cfm_pkt_pm[(type <= CFM_PDU_MAX)?type:CFM_PDU_INVALID].rx++;
			if(cfm_pktinfo->rx_desc.logical_port_id == switch_get_wan_logical_portid())
				cfm_config->cfm_pkt_pm[(type <= CFM_PDU_MAX)?type:CFM_PDU_INVALID].rx_ani++;
			else
				cfm_config->cfm_pkt_pm[(type <= CFM_PDU_MAX)?type:CFM_PDU_INVALID].rx_uni++;
		} else {
			if (cfm_util_get_mhf_creation(cfm_config) != MA_MHF_CREATION_NONE) { // MIP
				cfm_config->cfm_pkt_pm[(type <= CFM_PDU_MAX)?type:CFM_PDU_INVALID].rx++;
				if(cfm_pktinfo->rx_desc.logical_port_id == switch_get_wan_logical_portid())
					cfm_config->cfm_pkt_pm[(type <= CFM_PDU_MAX)?type:CFM_PDU_INVALID].rx_ani++;
				else
					cfm_config->cfm_pkt_pm[(type <= CFM_PDU_MAX)?type:CFM_PDU_INVALID].rx_uni++;
				cfm_forward_cfm(cfm_config, pktinfo, cfm_pktinfo, type);
			} else {
				cfm_config->cfm_pkt_pm[(type <= CFM_PDU_MAX)?type:CFM_PDU_INVALID].drop++;
				if(cfm_pktinfo->rx_desc.logical_port_id == switch_get_wan_logical_portid())
					cfm_config->cfm_pkt_pm[(type <= CFM_PDU_MAX)?type:CFM_PDU_INVALID].drop_ani++;
				else
					cfm_config->cfm_pkt_pm[(type <= CFM_PDU_MAX)?type:CFM_PDU_INVALID].drop_uni++;
			}
		}
		break;
	}
	return 0;
}
