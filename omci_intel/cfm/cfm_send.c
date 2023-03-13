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
 * File    : cfm_send.c
 *
 ******************************************************************/

#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "list.h"
#include "util.h"
#include "meinfo.h"
#include "me_related.h"
#include "hwresource.h"
#include "fwk_msgq.h"
#include "fwk_timer.h"
#include "switch.h"
#include "cpuport.h"
#include "cpuport_util.h"
#include "cpuport_util_send.h"
#include "cpuport_frame.h"
#ifndef X86
#if defined(CONFIG_TARGET_PRX126) || defined CONFIG_SDK_PRX120 || defined CONFIG_SDK_PRX321
#include "cpuport_hw_prx126.h"
#endif
#endif
#include "prbs.h"
#include "crc.h"
#include "proprietary_alu.h"

#include "cfm_pkt.h"
#include "cfm_mp.h"
#include "cfm_util.h"
#include "cfm_switch.h"
#include "cfm_bport.h"
#include "cfm_print.h"
#include "cfm_core.h"
#include "cfm_send.h"

int
cfm_send_frame(cfm_config_t *cfm_config, struct cpuport_info_t *pktinfo, int send_type)
{
	unsigned int uni_portmask = switch_get_uni_logical_portmask();
	unsigned int tx_portmask = 0;
	struct me_t *tx_port_me = NULL;
	int trunk_group_send = 0;
	int i, num, err;

	if(send_type == CFM_SEND_FORWARD) {
		if(pktinfo->rx_desc.logical_port_id == switch_get_wan_logical_portid()) { // WAN -> UNI
			if (cfm_config->port_type == ENV_BRIDGE_PORT_TYPE_UNI) {
				tx_port_me = cfm_bport_find(cfm_config);
				tx_portmask = 1 << cfm_config->logical_port_id;
			} else {
				tx_port_me = cfm_bport_find_peer(cfm_config);
				tx_portmask = cfm_bport_find_peer_logical_portmask(cfm_config);
			}
			return (cpuport_util_send_multi_lan(pktinfo, "cfm-forward-to-lan", NULL, NULL)>0) ? 0 : -1;
		} else { // UNI -> WAN
			if (cfm_config->port_type == ENV_BRIDGE_PORT_TYPE_WAN) {
				tx_port_me = cfm_bport_find(cfm_config);
				tx_portmask = 1 << cfm_config->logical_port_id;
			} else {
				tx_port_me = cfm_bport_find_peer(cfm_config);
				tx_portmask = cfm_bport_find_peer_logical_portmask(cfm_config);
			}
			return (cpuport_util_send_multi_wan(pktinfo, "cfm-forward-to-wan", NULL, NULL)>0) ? 0 : -1;
		}

	} else if(send_type == CFM_SEND_NORMAL) { // Send (CCM/LBM/LTM/LMM/DMM/1DM)
		pktinfo->rx_desc.bridge_port_me = NULL;
		pktinfo->rx_desc.logical_port_id = 6; // CPU port

		if (!omci_env_g.cfm_reverse_mep_direction) {	// follow up/down definition in 802.1ag
			if (cfm_config->mep_control & MEP_CONTROL_IS_UP_MEP) {
				tx_port_me = cfm_bport_find_peer(cfm_config);
				tx_portmask = cfm_bport_find_peer_logical_portmask(cfm_config);
			} else {
				tx_port_me = cfm_bport_find(cfm_config);
				tx_portmask = 1 << cfm_config->logical_port_id;
			}
		} else {					// follow up/down definition in alu?
			if (cfm_config->mep_control & MEP_CONTROL_IS_UP_MEP) {
				tx_port_me = cfm_bport_find(cfm_config);
				tx_portmask = 1 << cfm_config->logical_port_id;
			} else {
				tx_port_me = cfm_bport_find_peer(cfm_config);
				tx_portmask = cfm_bport_find_peer_logical_portmask(cfm_config);
			}
		}

 	} else if (send_type == CFM_SEND_OPPOSITE) { // Send (AIS/LCK)
		pktinfo->rx_desc.bridge_port_me = NULL;
		pktinfo->rx_desc.logical_port_id = 6; // CPU port

		if (!omci_env_g.cfm_reverse_mep_direction) {	// follow up/down definition in 802.1ag
			if (cfm_config->mep_control & MEP_CONTROL_IS_UP_MEP) {
				tx_port_me = cfm_bport_find(cfm_config);
				tx_portmask = 1 << cfm_config->logical_port_id;
			} else {
				tx_port_me = cfm_bport_find_peer(cfm_config);
				tx_portmask = cfm_bport_find_peer_logical_portmask(cfm_config);
			}
		} else {					// follow up/down definition in alu?
			if (cfm_config->mep_control & MEP_CONTROL_IS_UP_MEP) {
				tx_port_me = cfm_bport_find_peer(cfm_config);
				tx_portmask = cfm_bport_find_peer_logical_portmask(cfm_config);
			} else {
				tx_port_me = cfm_bport_find(cfm_config);
				tx_portmask = 1 << cfm_config->logical_port_id;
			}
		}

	} else if (send_type == CFM_SEND_REPLY) { // Response (LBR/LTR/LMR/DMR)
		tx_port_me = pktinfo->rx_desc.bridge_port_me;
		tx_portmask = 1 << pktinfo->rx_desc.logical_port_id;

	} else {	// invalid send_type
		return -1;
	}

	// when port2port enable, mep/mip is only created on uni0, but that mep/mip needs to send/recv pkt for all uni
	if (omci_env_g.port2port_enable) {
		if (tx_portmask & uni_portmask )	// if any uni found, add all uni to portmask
			tx_portmask |= uni_portmask;
	}

	// cfm_portmask & linkup_logical_portmask check
	if ((tx_portmask & omci_env_g.cfm_portmask & switch_get_linkup_logical_portmask()) == 0) {
		dbprintf_cfm(LOG_NOTICE, "mdl %d mp %d tx_portmask 0x%x canceled because of cfm_portmak(0x%x) & switch_linkup_logical_portmask(0x%x)\n",
			cfm_config->md_level, cfm_config->mep_id, tx_portmask, omci_env_g.cfm_portmask, switch_get_linkup_logical_portmask());
		return -1;
	}
	// filter unnecessary ports by cfm_portmask, basically, only uplink & uni need cfm
	tx_portmask &= omci_env_g.cfm_portmask;
	// only send to linkup port
	tx_portmask &= switch_get_linkup_logical_portmask();

	// eliminate unnecessary flooding by da lookup
	num = 0;
	for (i=0; i<SWITCH_LOGICAL_PORT_TOTAL; i++) {
		if (tx_portmask & (1<<i))
			num++;
	}
	if (num >1) {
		unsigned int da_portmask = switch_get_dmac_logical_portmask(pktinfo->dst_mac);
		if (tx_portmask & da_portmask)
			tx_portmask = tx_portmask & da_portmask;
	}

	// send pkt based on tx portmask
	err = 0;
	for (i=0; i<SWITCH_LOGICAL_PORT_TOTAL; i++) {
		if (tx_portmask & (1 <<i)) {
			pktinfo->tx_desc.logical_port_id = i;
			if ((1<<i) & omci_env_g.trunk_group_portmask) {
				if (trunk_group_send > 0)
					continue;	// dont send more than once to same group of trunk ports
			}
			if (uni_portmask & (1<<i)) {
				// the tx_portmask would have multi-bit set only if they are uni ports
				// so we try to find precise tx bport for specific uni
				struct switch_port_info_t port_info;
				switch_get_port_info_by_uni_num(i, &port_info);
				pktinfo->tx_desc.bridge_port_me = port_info.bridge_port_me;
			} else {
				pktinfo->tx_desc.bridge_port_me = tx_port_me;
			}
			// fill pktinfo's tci
			memset(&pktinfo->tci, 0, sizeof(struct cpuport_tci_t));
			if ( ntohs( *(unsigned short *)( pktinfo->frame_ptr+IFHWADDRLEN*2+VLAN_TAG_OFFSET ) ) == CFM_ETHERTYPE ) {
				unsigned short tci = ntohs( *(unsigned short *)( pktinfo->frame_ptr+IFHWADDRLEN*2+2 ));
				pktinfo->tci.itci.tpid = ntohs( *(unsigned short *)( pktinfo->frame_ptr+IFHWADDRLEN*2 ));
				pktinfo->tci.itci.priority = tci >> 13;
				pktinfo->tci.itci.de = (tci >> 12) & 1;
				pktinfo->tci.itci.vid = tci & 0xfff;
				pktinfo->tci.ethertype = CFM_ETHERTYPE;
			} else if ( ntohs( *(unsigned short *)( pktinfo->frame_ptr+IFHWADDRLEN*2+VLAN_TAG_OFFSET*2 ) ) == CFM_ETHERTYPE ) {
				unsigned short tci = ntohs( *(unsigned short *)( pktinfo->frame_ptr+IFHWADDRLEN*2+2 ));
				pktinfo->tci.otci.tpid = ntohs( *(unsigned short *)( pktinfo->frame_ptr+IFHWADDRLEN*2 ));
				pktinfo->tci.otci.priority = tci >> 13;
				pktinfo->tci.otci.de = (tci >> 12) & 1;
				pktinfo->tci.otci.vid = tci & 0xfff;
				tci = ntohs( *(unsigned short *)( pktinfo->frame_ptr+IFHWADDRLEN*2+VLAN_TAG_OFFSET+2 ));
				pktinfo->tci.itci.tpid = ntohs( *(unsigned short *)( pktinfo->frame_ptr+IFHWADDRLEN*2+VLAN_TAG_OFFSET ));
				pktinfo->tci.itci.priority = tci >> 13;
				pktinfo->tci.itci.de = (tci >> 12) & 1;
				pktinfo->tci.itci.vid = tci & 0xfff;
				pktinfo->tci.ethertype = CFM_ETHERTYPE;
			}
			// per calix's request, response ltr with its priority
			if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX)==0) {
				cfm_hdr_t *cfm_hdr = NULL;
				if (ntohs( *(unsigned short *)( pktinfo->frame_ptr+IFHWADDRLEN*2+VLAN_TAG_OFFSET)) == CFM_ETHERTYPE) {
					cfm_hdr = (cfm_hdr_t *)(pktinfo->frame_ptr + IFHWADDRLEN*2 + ETHER_TYPE_LEN + VLAN_TAG_OFFSET * 1);
					if((cfm_hdr->opcode == CFM_PDU_LTR) && (cfm_util_get_mhf_creation(cfm_config) == MA_MHF_CREATION_NONE)) {
						unsigned char *pbit = (unsigned char *)( pktinfo->frame_ptr+IFHWADDRLEN*2+2 );
						*pbit = ((cfm_config->priority&0x7)<<5) | (*pbit&0x1f);
						pktinfo->tci.itci.priority = cfm_config->priority;
					}
				} else if (ntohs( *(unsigned short *)( pktinfo->frame_ptr+IFHWADDRLEN*2+VLAN_TAG_OFFSET*2)) == CFM_ETHERTYPE) {
					cfm_hdr = (cfm_hdr_t *)(pktinfo->frame_ptr + IFHWADDRLEN*2 + ETHER_TYPE_LEN + VLAN_TAG_OFFSET * 2);
					if((cfm_hdr->opcode == CFM_PDU_LTR) && (cfm_util_get_mhf_creation(cfm_config) == MA_MHF_CREATION_NONE)) {
						unsigned char *pbit = (unsigned char *)( pktinfo->frame_ptr+IFHWADDRLEN*2+2 );
						*pbit = ((cfm_config->priority&0x7)<<5) | (*pbit&0x1f);
						pbit = (unsigned char *)( pktinfo->frame_ptr+IFHWADDRLEN*2+VLAN_TAG_OFFSET+2 );
						*pbit = ((cfm_config->priority&0x7)<<5) | (*pbit&0x1f);
						pktinfo->tci.otci.priority = cfm_config->priority;
						pktinfo->tci.itci.priority = cfm_config->priority;
					}
				}
			}
			// pktinfo->tx_desc.bridge_port_me is actually not used by cpuport_frame_send()
			// it would be required only if we want to do vtagging/vfiltering for cfm pkt
			if (omci_env_g.debug_level_cfm >= LOG_WARNING) {
				cfm_hdr_t *cfm_hdr;
				if (ntohs( *(unsigned short *)( pktinfo->frame_ptr+IFHWADDRLEN*2+VLAN_TAG_OFFSET)) == CFM_ETHERTYPE)
					cfm_hdr = (cfm_hdr_t *)(pktinfo->frame_ptr + IFHWADDRLEN*2 + ETHER_TYPE_LEN + VLAN_TAG_OFFSET * 1);
				else if (ntohs( *(unsigned short *)( pktinfo->frame_ptr+IFHWADDRLEN*2+VLAN_TAG_OFFSET*2)) == CFM_ETHERTYPE)
					cfm_hdr = (cfm_hdr_t *)(pktinfo->frame_ptr + IFHWADDRLEN*2 + ETHER_TYPE_LEN + VLAN_TAG_OFFSET * 2);
				else // if (ntohs( *(unsigned short *)( pktinfo->frame_ptr+IFHWADDRLEN*2 ) ) == CFM_ETHERTYPE)
					cfm_hdr = (cfm_hdr_t *)(pktinfo->frame_ptr + IFHWADDRLEN*2 + ETHER_TYPE_LEN + VLAN_TAG_OFFSET * 0);
				cfm_print_send_frame((cfm_hdr->opcode==CFM_PDU_CCM)?LOG_NOTICE:LOG_WARNING, cfm_config, pktinfo, send_type);
				if(pktinfo->tx_desc.logical_port_id == switch_get_wan_logical_portid()) // WAN
					cfm_config->cfm_pkt_pm[(cfm_hdr->opcode <= CFM_PDU_MAX)?cfm_hdr->opcode:CFM_PDU_INVALID].tx_ani++;
				else // LAN
					cfm_config->cfm_pkt_pm[(cfm_hdr->opcode <= CFM_PDU_MAX)?cfm_hdr->opcode:CFM_PDU_INVALID].tx_uni++;
			}
			if (cpuport_frame_send(pktinfo) < 0) {
				err++;
			} else {
				if ((1<<i) & omci_env_g.trunk_group_portmask)
					trunk_group_send++;
			}
		}
	}
	if (err)
		return -1;
	return 0;
}

static int
fill_data_tlv(cfm_config_t *cfm_config, char *frame_ptr, unsigned int pkt_offset)
{
	int i;
	cfm_tlv_data_t cfm_tlv_data;

	cfm_tlv_data.type = 3;
	cfm_tlv_data.length = htons(cfm_config->tlv_data_length);
	memcpy(frame_ptr+pkt_offset, &cfm_tlv_data, sizeof(cfm_tlv_data));
	pkt_offset += sizeof(cfm_tlv_data);

	//fill payload_pattern into data tlv
	for(i=0; i<cfm_config->tlv_data_length; i++) {
		unsigned char pattern = ((unsigned int) cfm_config->payload_pattern) >> (32-(((i%4)+1)*8));
		memset(frame_ptr+pkt_offset+i, pattern, sizeof(unsigned char));
	}
	pkt_offset += cfm_config->tlv_data_length;

	return pkt_offset;
}

static int
fill_test_tlv(cfm_config_t *cfm_config, char *frame_ptr, unsigned int pkt_offset)
{
	cfm_tlv_test_t cfm_tlv_test;
	unsigned short prbs_data_len=0;
	unsigned int crc32_value=0, crc32_value_len=0;
	unsigned int crc32_offset=0;

	//prepare for crc32 generator
	crc32_offset=pkt_offset;

	// Fill TST TLV
	cfm_tlv_test.type = 32;
	cfm_tlv_test.length = htons(cfm_config->tlv_tst_length);
	cfm_tlv_test.pattern_type= cfm_config->tlv_tst_pattern_type;

	memcpy(frame_ptr+pkt_offset, &cfm_tlv_test, sizeof(cfm_tlv_test));
	pkt_offset += sizeof(cfm_tlv_test);

	// type size=1, crc32 size=4
	switch(cfm_tlv_test.pattern_type) {
	case 0:	//null without crc32
		prbs_data_len=cfm_config->tlv_tst_length-1;
		crc32_value_len=0;

		//fill null data
		memset(frame_ptr+pkt_offset, 0, prbs_data_len);
	break;
	case 1:	//null with crc32
		prbs_data_len=cfm_config->tlv_tst_length-5;
		crc32_value_len=4;

		//fill null data
		memset(frame_ptr+pkt_offset, 0, prbs_data_len);
	break;
	case 2:	//prbs without crc32
		prbs_data_len=cfm_config->tlv_tst_length-1;
		crc32_value_len=0;

		//fill prbs31
		prbs31(cfm_config->tst_sequence_number, prbs_data_len, frame_ptr+pkt_offset);
	break;
	case 3: //prbs with crc32
		prbs_data_len=cfm_config->tlv_tst_length-5;
		crc32_value_len=4;

		//fill prbs31
		prbs31(cfm_config->tst_sequence_number, prbs_data_len, frame_ptr+pkt_offset);
	break;

	default:
		prbs_data_len=cfm_config->tlv_tst_length-1;
		crc32_value_len=0;
		//fill null data
		memset(frame_ptr+pkt_offset, 0, prbs_data_len);
		dbprintf_cfm(LOG_ERR, "Input parameter error, should be 0~3\n");
	}
	pkt_offset += prbs_data_len;

	//fill crc32
	if(crc32_value_len) {
		//ethernet crc32
		crc32_value=crc_le_update(0xFFFFFFFF, frame_ptr+crc32_offset, cfm_config->tlv_tst_length-4+3);
		util_dbprintf(omci_env_g.debug_level_cfm, LOG_ERR, 0, "crc32_value=0x%04X\n", ~crc32_value);

		crc32_value=htonl(~crc32_value);
		memcpy(frame_ptr+pkt_offset, &crc32_value, crc32_value_len);
		pkt_offset += crc32_value_len;
	}

	return pkt_offset;
}


static int
cfm_is_send_tlv_sender_id(cfm_config_t *cfm_config, int *is_chassis, int *is_manage)
{
	switch (cfm_util_get_sender_id_permission(cfm_config)) {
		case MA_SENDER_ID_PERMISSION_NONE:
			*is_chassis = FALSE;
			*is_manage = FALSE;
			break;
		case MA_SENDER_ID_PERMISSION_CHASSIS:
			*is_chassis = TRUE;
			*is_manage = FALSE;
			break;
		case MA_SENDER_ID_PERMISSION_MANAGE:
			*is_chassis = FALSE;
			*is_manage = TRUE;
			break;
		case MA_SENDER_ID_PERMISSION_CHASSIS_MANAGE:
			*is_chassis = TRUE;
			*is_manage = TRUE;
	}

	dbprintf_cfm(LOG_DEBUG, "mdl %d mep %d, md_sender_id_permission=%d, ma_sender_id_permission=%d, is_chassis=%d, is_manage=%d\n",
		cfm_config->md_level, cfm_config->mep_id, cfm_config->md_sender_id_permission, cfm_config->ma_sender_id_permission, *is_chassis, *is_manage);

	if((*is_chassis == FALSE) && (*is_manage == FALSE))
		return FALSE;
	else
		return TRUE;
}

static int
cfm_tlv_end(cfm_tlv_end_t *end_tlv)
{
	end_tlv->type = 0;

	return 0;
}

static int
cfm_tlv_sender_id(cfm_config_t *cfm_config, cfm_tlv_sender_id_t *sender_id_tlv)
{
	int is_chassis, is_manage;
	unsigned short length = 0;
	extern cfm_chassis_t cfm_chassis_g;

	sender_id_tlv->type = 1;

	cfm_is_send_tlv_sender_id(cfm_config, &is_chassis, &is_manage);

	if(is_chassis == TRUE) {
		sender_id_tlv->sender_id[length++] = cfm_chassis_g.chassis_id_length; // Chassis ID Length
		if(cfm_chassis_g.chassis_id_length > 0) {
			sender_id_tlv->sender_id[length++] = cfm_chassis_g.chassis_id_subtype; // Chassis ID Subtype
			memcpy(sender_id_tlv->sender_id+length, cfm_chassis_g.chassis_id, cfm_chassis_g.chassis_id_length); // Chassis ID
			length += cfm_chassis_g.chassis_id_length;
		}
	}

	if(is_manage == TRUE) {
		sender_id_tlv->sender_id[length++] = cfm_chassis_g.mgmtaddr_domain_length; // MA Domain Length
		if(cfm_chassis_g.mgmtaddr_domain_length > 0) {
			memcpy(sender_id_tlv->sender_id+length, cfm_chassis_g.mgmtaddr_domain, cfm_chassis_g.mgmtaddr_domain_length); // MA Domain ID
			length += cfm_chassis_g.mgmtaddr_domain_length;
			sender_id_tlv->sender_id[length++] = cfm_chassis_g.mgmtaddr_length; // MA Length
			memcpy(sender_id_tlv->sender_id+length, cfm_chassis_g.mgmtaddr, cfm_chassis_g.mgmtaddr_length); // MA
			length += cfm_chassis_g.mgmtaddr_length;
		}
	}

	sender_id_tlv->len = htons(length);

	return 0;
}

static int
cfm_tlv_port_status(cfm_config_t *cfm_config, cfm_tlv_port_status_t *port_status_tlv, int is_up)
{
	port_status_tlv->type = 2;
	port_status_tlv->len = htons(1);
	if (is_up)
		port_status_tlv->port_status = 2;	// psUP
	else
		port_status_tlv->port_status = 1;	// psBlocked
	return 0;
}

static int
cfm_tlv_interface_status(cfm_config_t *cfm_config, cfm_tlv_interface_status_t *interface_status_tlv, int is_up)
{
	interface_status_tlv->type = 4;
	interface_status_tlv->len = htons(1);
	if (is_up)
		interface_status_tlv->interface_status = 1;	// up
	else
		interface_status_tlv->interface_status = 2;	// down
	return 0;
}

static int
cfm_tlv_reply_ingress(cfm_config_t *cfm_config, cfm_tlv_reply_ingress_t *reply_ingress_tlv, unsigned char ttl)
{
	reply_ingress_tlv->type = 5;
	reply_ingress_tlv->len = htons(7);
	//reply_ingress_tlv->ingress_action = (ttl > 0) ? 1 : 2; // IngrOK or IngrDown
	reply_ingress_tlv->ingress_action = 1; // IngrOK
	memcpy(reply_ingress_tlv->ingress_mac, cfm_config->macaddr, IFHWADDRLEN);
	return 0;
}

static int
cfm_tlv_reply_egress(cfm_config_t *cfm_config, cfm_tlv_reply_egress_t *reply_egress_tlv, unsigned char ttl)
{
	reply_egress_tlv->type = 6;
	reply_egress_tlv->len = htons(7);
	//reply_egress_tlv->egress_action = (ttl > 0) ? 1 : 2; // EgrOK or EgrDown
	reply_egress_tlv->egress_action = 1; // EgrOK
	memcpy(reply_egress_tlv->egress_mac, cfm_config->macaddr, IFHWADDRLEN);
	return 0;
}

static int
cfm_tlv_ltm_egress_id(cfm_config_t *cfm_config, cfm_tlv_ltm_egress_id_t *ltm_egress_id_tlv)
{
	ltm_egress_id_tlv->type = 7;
	ltm_egress_id_tlv->len = htons(8);
	ltm_egress_id_tlv->egress_id = htons((cfm_config->egress_id[6] << 8) | cfm_config->egress_id[7]);
	memcpy(ltm_egress_id_tlv->egress_mac, cfm_config->macaddr, IFHWADDRLEN);
	return 0;
}

static int
cfm_tlv_ltr_egress_id(cfm_config_t *cfm_config, cfm_tlv_ltr_egress_id_t *ltr_egress_id_tlv, cfm_tlv_ltm_egress_id_t *ltm_egress_id_tlv)
{
	ltr_egress_id_tlv->type = 8;
	ltr_egress_id_tlv->len = htons(16);
	ltr_egress_id_tlv->last_egress_id = ltm_egress_id_tlv->egress_id;
	memcpy(ltr_egress_id_tlv->last_egress_mac, ltm_egress_id_tlv->egress_mac, IFHWADDRLEN);
	ltr_egress_id_tlv->next_egress_id = htons((cfm_config->egress_id[6] << 8) | cfm_config->egress_id[7]);
	memcpy(ltr_egress_id_tlv->next_egress_mac, cfm_config->macaddr, IFHWADDRLEN);
	return 0;
}

int
cfm_send_lbr(cfm_config_t *cfm_config, cfm_pktinfo_t *cfm_pktinfo)
{
	struct cpuport_info_t pktinfo;

	pktinfo.frame_ptr = cfm_pktinfo->frame_ptr;
	pktinfo.rx_desc = cfm_pktinfo->rx_desc;
	pktinfo.tx_desc = cfm_pktinfo->tx_desc;
	pktinfo.frame_len = cfm_pktinfo->frame_len;
	pktinfo.frame_len = (pktinfo.frame_len < MIN_ETH_FRAME_SIZE) ? MIN_ETH_FRAME_SIZE : pktinfo.frame_len;

	memcpy(cfm_pktinfo->dst_mac_ptr, cfm_pktinfo->src_mac_ptr, IFHWADDRLEN);
	memcpy(cfm_pktinfo->src_mac_ptr, cfm_config->macaddr, IFHWADDRLEN);
	cfm_pktinfo->cfm_hdr->opcode = CFM_PDU_LBR;

	if(cfm_send_frame(cfm_config, &pktinfo, CFM_SEND_REPLY) < 0) {
		return -1;
	}
	cfm_config->cfm_pkt_pm[CFM_PDU_LBR].tx++;
	return 0;
}

int
cfm_send_lbm(cfm_config_t *cfm_config, unsigned char *target_mac, char priority, char dei)
{
	struct cpuport_info_t pktinfo;
	cfm_hdr_t cfm_hdr;
	cfm_pdu_lb_t pdu_lbm;
	int is_sender_id, is_chassis, is_manage;
	cfm_tlv_sender_id_t sender_id_tlv;
	cfm_tlv_end_t end_tlv;
	unsigned int pkt_offset;
	unsigned char frame_ptr[BUF_SIZE];
	unsigned char vlan[VLAN_TAG_OFFSET] = {0x81, 0x00, 0x00, 0x00};
	unsigned char ethtype[2] = {0x89, 0x02};

	// Fill DA
	memcpy(&frame_ptr, target_mac, IFHWADDRLEN);
	pkt_offset = IFHWADDRLEN;

	// Fill SA
	memcpy(frame_ptr+pkt_offset, cfm_config->macaddr, IFHWADDRLEN);
	pkt_offset += IFHWADDRLEN;

	// Fill VLAN Tag
	{
		unsigned short equiv_vlan = (cfm_config->pri_vlan)?cfm_config->pri_vlan:cfm_config->assoc_vlans[0];
		unsigned char equiv_pri = (priority == -1)?cfm_config->priority:priority;
		if (equiv_vlan !=0 || equiv_pri !=0) {
			vlan[2] = (equiv_pri << 5) & 0xE0;
			vlan[2] |= (dei << 4) & 0x10;
			vlan[2] |= (equiv_vlan >> 8) & 0x1F;
			vlan[3] = equiv_vlan & 0xFF;
			memcpy(frame_ptr+pkt_offset, &vlan, sizeof(vlan));
			pkt_offset += sizeof(vlan);
		}
	}

	// Fill Ethertype
	memcpy(frame_ptr+pkt_offset, &ethtype, sizeof(ethtype));
	pkt_offset += sizeof(ethtype);

	// Fill CFM Common PDU
	cfm_hdr.md_level = cfm_config->md_level;
	cfm_hdr.version = 0;
	cfm_hdr.opcode = CFM_PDU_LBM;
	cfm_hdr.flags = 0x00;
	cfm_hdr.first_tlv_offset = sizeof(cfm_pdu_lb_t);
	memcpy(frame_ptr+pkt_offset, &cfm_hdr, sizeof(cfm_hdr));
	pkt_offset += sizeof(cfm_hdr);

	// Fill CFM LBM PDU
	pdu_lbm.xid = htonl(++(cfm_config->nextLBMtransID));
	memcpy(frame_ptr+pkt_offset, &pdu_lbm, sizeof(pdu_lbm));
	pkt_offset += sizeof(pdu_lbm);

	// Fill Sender ID TLV
	is_sender_id = cfm_is_send_tlv_sender_id(cfm_config, &is_chassis, &is_manage);
	if(is_sender_id == TRUE && cfm_config->send_lb_senderid == 1) {
		cfm_tlv_sender_id(cfm_config, &sender_id_tlv);
		memcpy(frame_ptr+pkt_offset, &sender_id_tlv, htons(sender_id_tlv.len)+3);
		pkt_offset += (htons(sender_id_tlv.len)+3);
	}

	// Fill Port Status TLV
	if (cfm_config->send_lb_port_status == 1) {
		int is_linkup = switch_get_linkup_logical_portmask() & (1 << cfm_config->logical_port_id);
		if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX)==0) {
			// if port_type is WAN, find its peer UNI state
			if (cfm_config->port_type == ENV_BRIDGE_PORT_TYPE_WAN) {
				struct me_t *peer_me = cfm_bport_find_peer(cfm_config);
				if(peer_me) {
					struct me_t *ibridgeport_me = hwresource_public2private(peer_me);
					if (ibridgeport_me) {
						unsigned char logical_port_id = meinfo_util_me_attr_data(ibridgeport_me, 6);
						is_linkup = switch_get_linkup_logical_portmask() & (1 << logical_port_id);
					}
				}
			}
		}
		cfm_tlv_port_status_t port_status_tlv;
		cfm_tlv_port_status(cfm_config, &port_status_tlv, is_linkup);
		memcpy(frame_ptr+pkt_offset, &port_status_tlv, 4);
		pkt_offset += 4;
	}

	// Fill Interface Status TLV
	if (cfm_config->send_lb_interface_status == 1) {
		int is_linkup = switch_get_linkup_logical_portmask() & (1 << cfm_config->logical_port_id);
		if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX)==0) {
			// if port_type is WAN, find its peer UNI state
			if (cfm_config->port_type == ENV_BRIDGE_PORT_TYPE_WAN) {
				struct me_t *peer_me = cfm_bport_find_peer(cfm_config);
				if(peer_me) {
					struct me_t *ibridgeport_me = hwresource_public2private(peer_me);
					if (ibridgeport_me) {
						unsigned char logical_port_id = meinfo_util_me_attr_data(ibridgeport_me, 6);
						is_linkup = switch_get_linkup_logical_portmask() & (1 << logical_port_id);
					}
				}
			}
		}
		cfm_tlv_interface_status_t interface_status_tlv;
		cfm_tlv_interface_status(cfm_config, &interface_status_tlv, is_linkup);
		memcpy(frame_ptr+pkt_offset, &interface_status_tlv, 4);
		pkt_offset += 4;
	}

	// Fill DATA TLV
	if( cfm_config->tlv_tst_length > 0 && cfm_config->payload_format == OPTION_TLV_TEST) {
		pkt_offset=fill_test_tlv(cfm_config, frame_ptr, pkt_offset);
	} else if( cfm_config->tlv_data_length > 0 && cfm_config->payload_format == OPTION_TLV_DATA && cfm_config->send_lb_data_status == 1) {
		pkt_offset=fill_data_tlv(cfm_config, frame_ptr, pkt_offset);
	}

	// Fill End TLV
	cfm_tlv_end(&end_tlv);
	memcpy(frame_ptr+pkt_offset, &end_tlv, sizeof(end_tlv));
	pkt_offset += sizeof(end_tlv);

	pktinfo.frame_ptr = frame_ptr;
	pktinfo.buf_len = BUF_SIZE;
	pktinfo.frame_len = (pkt_offset < MIN_ETH_FRAME_SIZE) ? MIN_ETH_FRAME_SIZE : pkt_offset;

	// Before sending LBM, flush LBR related stru
	cfm_mp_flush_lbr_list(cfm_config);
	cfm_config->cfm_send_lbm_timestamp = cfm_util_get_time_in_us();

	if(cfm_send_frame(cfm_config, &pktinfo, CFM_SEND_NORMAL) < 0) {
		return -1;
	}
	cfm_config->cfm_pkt_pm[CFM_PDU_LBM].tx++;
	return 0;
}

int
cfm_send_ltr(cfm_config_t *cfm_config, cfm_pktinfo_t *cfm_pktinfo)
{
	unsigned char *p;
	int is_sender_id, is_chassis, is_manage, flags;
	struct cpuport_info_t pktinfo;
	cfm_pdu_ltm_t *pdu_ltm;
	cfm_tlv_ltm_egress_id_t *ltm_egress_id_tlv;
	cfm_pdu_ltr_t pdu_ltr;
	cfm_tlv_ltr_egress_id_t ltr_egress_id_tlv;
	cfm_tlv_reply_ingress_t reply_ingress_tlv;
	cfm_tlv_reply_egress_t reply_egress_tlv;
	cfm_tlv_sender_id_t sender_id_tlv;
	cfm_tlv_end_t end_tlv;

	p = (unsigned char *) cfm_pktinfo->cfm_hdr + sizeof(cfm_hdr_t);
	pdu_ltm = (cfm_pdu_ltm_t *) p;
	p += sizeof(cfm_pdu_ltm_t);
	ltm_egress_id_tlv = (cfm_tlv_ltm_egress_id_t *) p;

	memcpy(cfm_pktinfo->src_mac_ptr, cfm_config->macaddr, IFHWADDRLEN);
	memcpy(cfm_pktinfo->dst_mac_ptr, pdu_ltm->origin_addr, IFHWADDRLEN);
	flags = cfm_pktinfo->cfm_hdr->flags;
	flags |= (memcmp(pdu_ltm->target_addr, cfm_config->macaddr, IFHWADDRLEN) == 0) ? 0x20 : 0x00; // TerminalMEP?
	if((memcmp(pdu_ltm->target_addr, cfm_config->macaddr, IFHWADDRLEN) != 0) &&
	   (pdu_ltm->ttl > 1) &&
	   (cfm_util_get_mhf_creation(cfm_config) != MA_MHF_CREATION_NONE)) // Not TerminalMEP and TTL>1 and MIP
		flags |= 0x40; // FwdYes

	// Fill CFM LTR PDU
	pdu_ltr.xid = pdu_ltm->xid;
	pdu_ltr.ttl = pdu_ltm->ttl;
	pdu_ltr.relay_action = (memcmp(pdu_ltm->target_addr, cfm_config->macaddr, IFHWADDRLEN)) ? 2 : 1; // RlyFDB or RelayHit
	p = (unsigned char *) cfm_pktinfo->cfm_hdr + sizeof(cfm_hdr_t);
	memcpy(p, &pdu_ltr, sizeof(cfm_pdu_ltr_t));
	p += sizeof(cfm_pdu_ltr_t);

	// Fill LTR Egress ID TLV
	cfm_tlv_ltr_egress_id(cfm_config, &ltr_egress_id_tlv, ltm_egress_id_tlv);
	memcpy(p, &ltr_egress_id_tlv, sizeof(cfm_tlv_ltr_egress_id_t));
	p += sizeof(cfm_tlv_ltr_egress_id_t);

	// Fill Reply Ingress TLV
	cfm_tlv_reply_ingress(cfm_config, &reply_ingress_tlv, pdu_ltm->ttl);
	memcpy(p, &reply_ingress_tlv, sizeof(cfm_tlv_reply_ingress_t));
	p += sizeof(cfm_tlv_reply_ingress_t);

	// Fill Reply Egress TLV
	cfm_tlv_reply_egress(cfm_config, &reply_egress_tlv, pdu_ltm->ttl);
	memcpy(p, &reply_egress_tlv, sizeof(cfm_tlv_reply_egress_t));
	p += sizeof(cfm_tlv_reply_egress_t);

	// Fill Sender ID TLV
	is_sender_id = cfm_is_send_tlv_sender_id(cfm_config, &is_chassis, &is_manage);
	if(is_sender_id == TRUE && cfm_config->send_lt_senderid == 1) {
		cfm_tlv_sender_id(cfm_config, &sender_id_tlv);
		memcpy(p, &sender_id_tlv, sizeof(cfm_tlv_sender_id_t));
		p += (htons(sender_id_tlv.len)+3);
	}

	// Fill End TLV
	cfm_tlv_end(&end_tlv);

	memcpy(p, &end_tlv, sizeof(cfm_tlv_end_t));

	cfm_pktinfo->cfm_hdr->opcode = CFM_PDU_LTR;
	cfm_pktinfo->cfm_hdr->first_tlv_offset = sizeof(cfm_pdu_ltr_t);
	cfm_pktinfo->cfm_hdr->flags = flags;

	pktinfo.frame_ptr = cfm_pktinfo->frame_ptr;
	pktinfo.rx_desc = cfm_pktinfo->rx_desc;
	pktinfo.tx_desc = cfm_pktinfo->tx_desc;
	pktinfo.frame_len = IFHWADDRLEN * 2
			+ VLAN_TAG_OFFSET * cfm_pktinfo->vlan_num
			+ ETHER_TYPE_LEN
			+ sizeof(cfm_hdr_t)
			+ sizeof(cfm_pdu_ltr_t)
			+ sizeof(cfm_tlv_ltr_egress_id_t)
			+ sizeof(cfm_tlv_reply_ingress_t)
			+ sizeof(cfm_tlv_reply_egress_t)
			+ ((is_sender_id == TRUE) ? (htons(sender_id_tlv.len) + 3) : 0)
			+ sizeof(cfm_tlv_end_t);
	pktinfo.frame_len = (pktinfo.frame_len < MIN_ETH_FRAME_SIZE) ? MIN_ETH_FRAME_SIZE : pktinfo.frame_len;

	if(cfm_send_frame(cfm_config, &pktinfo, CFM_SEND_REPLY) < 0) {
		return -1;
	}
	cfm_config->cfm_pkt_pm[CFM_PDU_LTR].tx++;
	return 0;
}

int
cfm_send_ltm(cfm_config_t *cfm_config, unsigned char *target_mac, char priority, char dei, unsigned char ttl, unsigned char is_fdbonly)
{
	int is_sender_id, is_chassis, is_manage;
	struct cpuport_info_t pktinfo;
	cfm_hdr_t cfm_hdr;
	cfm_pdu_ltm_t pdu_ltm;
	cfm_tlv_ltm_egress_id_t ltm_egress_id_tlv;
	cfm_tlv_sender_id_t sender_id_tlv;
	cfm_tlv_end_t end_tlv;
	unsigned int pkt_offset;
	unsigned char frame_ptr[BUF_SIZE];
	unsigned char d_mac[IFHWADDRLEN] = {0x01, 0x80, 0xC2, 0x00, 0x00, 0x38};
	unsigned char vlan[VLAN_TAG_OFFSET] = {0x81, 0x00, 0x00, 0x00};
	unsigned char ethtype[2] = {0x89, 0x02};

	// Fill DA
	d_mac[5] |= cfm_config->md_level;
	memcpy(frame_ptr, &d_mac, IFHWADDRLEN);
	pkt_offset = IFHWADDRLEN;

	// Fill SA
	memcpy(frame_ptr+pkt_offset, cfm_config->macaddr, IFHWADDRLEN);
	pkt_offset += IFHWADDRLEN;

	// Fill VLAN Tag
	{
		unsigned short equiv_vlan = (cfm_config->pri_vlan)?cfm_config->pri_vlan:cfm_config->assoc_vlans[0];
		unsigned char equiv_pri = (priority == -1)?cfm_config->priority:priority;
		if (equiv_vlan !=0 || equiv_pri !=0) {
			vlan[2] = (equiv_pri << 5) & 0xE0;
			vlan[2] |= (dei << 4) & 0x10;
			vlan[2] |= (equiv_vlan >> 8) & 0x1F;
			vlan[3] = equiv_vlan & 0xFF;
			memcpy(frame_ptr+pkt_offset, &vlan, sizeof(vlan));
			pkt_offset += sizeof(vlan);
		}
	}

	// Fill Ethertype
	memcpy(frame_ptr+pkt_offset, &ethtype, sizeof(ethtype));
	pkt_offset += sizeof(ethtype);

	// Fill CFM Common PDU
	cfm_hdr.md_level = cfm_config->md_level;
	cfm_hdr.version = 0;
	cfm_hdr.opcode = CFM_PDU_LTM;
	cfm_hdr.flags = (is_fdbonly == 1) ? 0x80 : 0x00;
	cfm_hdr.first_tlv_offset = sizeof(cfm_pdu_ltm_t);
	memcpy(frame_ptr+pkt_offset, &cfm_hdr, sizeof(cfm_hdr));
	pkt_offset += sizeof(cfm_hdr);

	// Fill CFM LTM PDU
	pdu_ltm.xid = htonl(++(cfm_config->nextLTMtransID));
	pdu_ltm.ttl = ttl;
	memcpy(&pdu_ltm.origin_addr, cfm_config->macaddr, IFHWADDRLEN);
	memcpy(&pdu_ltm.target_addr, target_mac, IFHWADDRLEN);
	memcpy(frame_ptr+pkt_offset, &pdu_ltm, sizeof(pdu_ltm));
	pkt_offset += sizeof(pdu_ltm);

	// Fill LTM Egress ID TLV
	cfm_tlv_ltm_egress_id(cfm_config, &ltm_egress_id_tlv);
	memcpy(frame_ptr+pkt_offset, &ltm_egress_id_tlv, sizeof(ltm_egress_id_tlv));
	pkt_offset += sizeof(ltm_egress_id_tlv);

	// Fill Sender ID TLV
	is_sender_id = cfm_is_send_tlv_sender_id(cfm_config, &is_chassis, &is_manage);
	if(is_sender_id == TRUE && cfm_config->send_lt_senderid == 1) {
		cfm_tlv_sender_id(cfm_config, &sender_id_tlv);
		memcpy(frame_ptr+pkt_offset, &sender_id_tlv, htons(sender_id_tlv.len)+3);
		pkt_offset += (htons(sender_id_tlv.len)+3);
	}

	// Fill Port Status TLV
	if (cfm_config->send_lt_port_status == 1) {
		int is_linkup = switch_get_linkup_logical_portmask() & (1 << cfm_config->logical_port_id);
		if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX)==0) {
			// if port_type is WAN, find its peer UNI state
			if (cfm_config->port_type == ENV_BRIDGE_PORT_TYPE_WAN) {
				struct me_t *peer_me = cfm_bport_find_peer(cfm_config);
				if(peer_me) {
					struct me_t *ibridgeport_me = hwresource_public2private(peer_me);
					if (ibridgeport_me) {
						unsigned char logical_port_id = meinfo_util_me_attr_data(ibridgeport_me, 6);
						is_linkup = switch_get_linkup_logical_portmask() & (1 << logical_port_id);
					}
				}
			}
		}
		cfm_tlv_port_status_t port_status_tlv;
		cfm_tlv_port_status(cfm_config, &port_status_tlv, is_linkup);
		memcpy(frame_ptr+pkt_offset, &port_status_tlv, 4);
		pkt_offset += 4;
	}

	// Fill Interface Status TLV
	if (cfm_config->send_lt_interface_status == 1) {
		int is_linkup = switch_get_linkup_logical_portmask() & (1 << cfm_config->logical_port_id);
		if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX)==0) {
			// if port_type is WAN, find its peer UNI state
			if (cfm_config->port_type == ENV_BRIDGE_PORT_TYPE_WAN) {
				struct me_t *peer_me = cfm_bport_find_peer(cfm_config);
				if(peer_me) {
					struct me_t *ibridgeport_me = hwresource_public2private(peer_me);
					if (ibridgeport_me) {
						unsigned char logical_port_id = meinfo_util_me_attr_data(ibridgeport_me, 6);
						is_linkup = switch_get_linkup_logical_portmask() & (1 << logical_port_id);
					}
				}
			}
		}
		cfm_tlv_interface_status_t interface_status_tlv;
		cfm_tlv_interface_status(cfm_config, &interface_status_tlv, is_linkup);
		memcpy(frame_ptr+pkt_offset, &interface_status_tlv, 4);
		pkt_offset += 4;
	}

	// Fill DATA TLV
	if( cfm_config->tlv_data_length > 0 && cfm_config->payload_format == OPTION_TLV_DATA && cfm_config->send_lt_data_status == 1) {
		pkt_offset = fill_data_tlv(cfm_config, frame_ptr, pkt_offset);
	}

	// Fill End TLV
	cfm_tlv_end(&end_tlv);
	memcpy(frame_ptr+pkt_offset, &end_tlv, sizeof(end_tlv));
	pkt_offset += sizeof(end_tlv);

	pktinfo.frame_ptr = frame_ptr;
	pktinfo.buf_len = BUF_SIZE;
	pktinfo.frame_len = (pkt_offset < MIN_ETH_FRAME_SIZE) ? MIN_ETH_FRAME_SIZE : pkt_offset;

	// Before sending LTM, flush LTR related stru
	cfm_mp_flush_ltr_list(cfm_config);
	cfm_config->cfm_send_ltm_timestamp = cfm_util_get_time_in_us();

	if(cfm_send_frame(cfm_config, &pktinfo, CFM_SEND_NORMAL) < 0) {
		return -1;
	}
	cfm_config->cfm_pkt_pm[CFM_PDU_LTM].tx++;
	return 0;
}

// find adjacent upper md level of the defect_md_level
// so mep@defect_md_ldevl can send AIS to opposition direction on adjacent md level
// ps: if defect is a ccm pkt not received by any mep
//     a pseudo mep(the server mep) a md_level_1 is assumed, the defect_md_level is assume to be 1
static int
cfm_find_adjacent_md_level (int defect_md_level)
{
	cfm_config_t *cfm_config = NULL;
	int md_level;

	// locate next md level where at least one mip/mep has been defined
	for(md_level=defect_md_level+1; md_level<=7; md_level++) {
		list_for_each_entry(cfm_config, &cfm_config_list, node) {
			if (cfm_config->md_level == md_level)
				return md_level;
		}
	}
	return -1;
}

// send ais/lck to ais_vlan for mep/mip@ais_md_level
static int
cfm_send_aislck_one(unsigned char opcode, cfm_config_t *cfm_config, unsigned char *target_mac, char ais_pri, char dei, char flags, char ais_md_level, unsigned short ais_vlan)
{
	struct cpuport_info_t pktinfo;
	cfm_hdr_t cfm_hdr;
	cfm_tlv_end_t end_tlv;
	unsigned int pkt_offset;
	unsigned char frame_ptr[BUF_SIZE];
	unsigned char vlan[VLAN_TAG_OFFSET] = {0x81, 0x00, 0x00, 0x00};
	unsigned char ethtype[2] = {0x89, 0x02};
	unsigned char cfm_mmac[IFHWADDRLEN] = {0x01, 0x80, 0xC2, 0x00, 0x00, 0x30};

	// Fill DA
	cfm_mmac[5] |= ais_md_level;
	if(strcmp(target_mac, "NULL")==0)
		memcpy(&frame_ptr, cfm_mmac, IFHWADDRLEN);
	else
		memcpy(&frame_ptr, target_mac, IFHWADDRLEN);
	pkt_offset = IFHWADDRLEN;

	// Fill SA
	memcpy(frame_ptr+pkt_offset, cfm_config->macaddr, IFHWADDRLEN);
	pkt_offset += IFHWADDRLEN;

	// Fill VLAN Tag
	if(ais_vlan != 0 || ais_pri !=0) {
		vlan[2] = (ais_pri << 5) & 0xE0;
		vlan[2] |= (dei << 4) & 0x10;
		vlan[2] |= (ais_vlan >> 8) & 0x1F;
		vlan[3] = ais_vlan & 0xFF;
		memcpy(frame_ptr+pkt_offset, &vlan, sizeof(vlan));
		pkt_offset += sizeof(vlan);
	}

	// Fill Ethertype
	memcpy(frame_ptr+pkt_offset, &ethtype, sizeof(ethtype));
	pkt_offset += sizeof(ethtype);

	// Fill CFM Common PDU
	cfm_hdr.md_level = ais_md_level;
	cfm_hdr.version = 0;
	cfm_hdr.opcode = opcode;
	cfm_hdr.flags = flags;
	cfm_hdr.first_tlv_offset = sizeof(cfm_pdu_aislck_t);
	memcpy(frame_ptr+pkt_offset, &cfm_hdr, sizeof(cfm_hdr));
	pkt_offset += sizeof(cfm_hdr);

	// Fill End TLV
	cfm_tlv_end(&end_tlv);
	memcpy(frame_ptr+pkt_offset, &end_tlv, sizeof(end_tlv));
	pkt_offset += sizeof(end_tlv);

	pktinfo.frame_ptr = frame_ptr;
	pktinfo.buf_len = BUF_SIZE;
	pktinfo.frame_len = (pkt_offset < MIN_ETH_FRAME_SIZE) ? MIN_ETH_FRAME_SIZE : pkt_offset;

	if (opcode == CFM_PDU_AIS) {
		if (cfm_config->md_level == server_mep_cfm_config_g.md_level &&
		    cfm_config->mep_id == server_mep_cfm_config_g.mep_id) {
			// server mep, send ais in both direction
			if (server_mep_logical_portmask_g & switch_get_wan_logical_portmask()) {	// defect happen at wan side
				// server mep is a downMEP@wan0, so opposite mean lan
				if(cfm_send_frame(cfm_config, &pktinfo, CFM_SEND_OPPOSITE) < 0)		// send ais in to lan
					return -1;
			}
			if (server_mep_logical_portmask_g & (~switch_get_wan_logical_portmask())) {	// defect happen at lan side
				// server mep is a downMEP@wan0, so normal mean wan
				if(cfm_send_frame(cfm_config, &pktinfo, CFM_SEND_NORMAL) < 0)		// send ais in to wan
					return -1;
			}
		} else {
			// normal mep, always send ais in opposite direction
			if(cfm_send_frame(cfm_config, &pktinfo, CFM_SEND_OPPOSITE) < 0)
  				return -1;
  		}
	} else if (opcode == CFM_PDU_LCK) {	// always send lck in both direction
		if(cfm_send_frame(cfm_config, &pktinfo, CFM_SEND_NORMAL) < 0)
			return -1;
  		if(cfm_send_frame(cfm_config, &pktinfo, CFM_SEND_OPPOSITE) < 0)
  			return -1;
	} else {
		dbprintf_cfm(LOG_ERR, "wrong opcode %d(%s) used?\n", opcode, cfm_print_get_opcode_str(opcode));
		return -2;
  	}
	return 0;
}

// send ais/lck to possible untag/vlans at adjacent md level (by localateing all meps at adjacent md level)
int
cfm_send_aislck(unsigned char opcode, cfm_config_t *cfm_config, unsigned char *target_mac, char priority, char dei, char flags)
{
	int ais_md_level = (cfm_config->eth_ais_control>>4) & 7;	// client meps md level
	cfm_config_t *client;
	unsigned short ais_vlan[32];
	int ais_vlan_total = 0;
	char ais_pri = (priority==-1)?cfm_config->priority:priority;
	int i;

	if (ais_md_level == 0) {
		ais_md_level =  cfm_find_adjacent_md_level(cfm_config->md_level);
		if (ais_md_level < 0)
			dbprintf_cfm(LOG_WARNING, "mdl %d mep %d send ais/lck canceled (unable to locate client mdl)\n",
				cfm_config->md_level, cfm_config->mep_id);
	}
	if (ais_md_level < 0 || ais_md_level > 7)
		return -1;

	// find all untag/vlans used by client meps at adjacent md level
	list_for_each_entry(client, &cfm_config_list, node) {
		if (client->md_level == ais_md_level) {
			unsigned short client_vlan;
			int found = 0;
			if (client->pri_vlan) {
				client_vlan = client->pri_vlan;
			} else if (client->assoc_vlans[0]) {
				client_vlan = client->assoc_vlans[0];
			} else {
				client_vlan = 0;
			}
			for (i=0; i<ais_vlan_total; i++) {
				if (ais_vlan[i] == client_vlan) {
					found = 1;
					break;
				}
			}
			if (!found) {
				ais_vlan[ais_vlan_total] = client_vlan;
				ais_vlan_total++;
			}
		}
	}

	if (ais_vlan_total == 0) {
		dbprintf_cfm(LOG_WARNING, "mdl %d mep %d send ais/lck canceled (no mep/mip found on client mdl %d)\n",
			cfm_config->md_level, cfm_config->mep_id, ais_md_level);
		return -1;
	}
	for (i = 0; i< ais_vlan_total; i++) {
		if (cfm_send_aislck_one(opcode, cfm_config, target_mac, ais_pri, dei, flags, ais_md_level, ais_vlan[i]) == 0) {
			if (opcode == CFM_PDU_AIS)
				cfm_config->cfm_pkt_pm[CFM_PDU_AIS].tx++;
			else if (opcode == CFM_PDU_LCK)
				cfm_config->cfm_pkt_pm[CFM_PDU_LCK].tx++;
		}
	}

	return 0;
}

int
cfm_send_ais(cfm_config_t *cfm_config, unsigned char *target_mac, char priority, char dei, char flags)
{
	return cfm_send_aislck(CFM_PDU_AIS, cfm_config, target_mac, priority, dei, flags);
}

int
cfm_send_lck(cfm_config_t *cfm_config, unsigned char *target_mac, char priority, char dei, char flags)
{
	return cfm_send_aislck(CFM_PDU_LCK, cfm_config, target_mac, priority, dei, flags);
}

int
cfm_send_lmr(cfm_config_t *cfm_config, cfm_pkt_rmep_entry_t *entry, cfm_pktinfo_t *cfm_pktinfo)
{
	unsigned char mac[IFHWADDRLEN], *p;
	struct cpuport_info_t pktinfo;
	cfm_pdu_lm_t *pdu_lmm, pdu_lmr;
	cfm_tlv_end_t end_tlv;

	p = (unsigned char *) cfm_pktinfo->cfm_hdr + sizeof(cfm_hdr_t);
	pdu_lmm = (cfm_pdu_lm_t *) p;

	pktinfo.frame_ptr = cfm_pktinfo->frame_ptr;
	pktinfo.rx_desc = cfm_pktinfo->rx_desc;
	pktinfo.tx_desc = cfm_pktinfo->tx_desc;
	pktinfo.frame_len = cfm_pktinfo->frame_len;
	pktinfo.frame_len = (pktinfo.frame_len < MIN_ETH_FRAME_SIZE) ? MIN_ETH_FRAME_SIZE : pktinfo.frame_len;

	memcpy(&mac, cfm_pktinfo->dst_mac_ptr, sizeof(mac));
	memcpy(cfm_pktinfo->dst_mac_ptr, cfm_pktinfo->src_mac_ptr, sizeof(mac));
	memcpy(cfm_pktinfo->src_mac_ptr, &mac, sizeof(mac));
	cfm_pktinfo->cfm_hdr->opcode = CFM_PDU_LMR;
	cfm_pktinfo->cfm_hdr->first_tlv_offset = sizeof(cfm_pdu_lm_t);
	cfm_pktinfo->cfm_hdr->flags = 0x00;

	// Fill End TLV
	cfm_tlv_end(&end_tlv);

	// Fill CFM LMR PDU
	pdu_lmr.TxFCf = pdu_lmm->TxFCf;
	pdu_lmr.RxFCf = htonl(entry->RxFCl_ccm);
	pdu_lmr.TxFCb = htonl(cfm_config->CCIsentCCMs);

	// Copy PDU/TLV into CFM packet
	p = (unsigned char *) cfm_pktinfo->cfm_hdr + sizeof(cfm_hdr_t);
	memcpy(p, &pdu_lmr, sizeof(cfm_pdu_lm_t));
	p += sizeof(cfm_pdu_lm_t);
	memcpy(p, &end_tlv, sizeof(cfm_tlv_end_t));

	if(cfm_send_frame(cfm_config, &pktinfo, CFM_SEND_REPLY) < 0) {
		return -1;
	}
	cfm_config->cfm_pkt_pm[CFM_PDU_LMR].tx++;
	return 0;
}

int
cfm_send_lmm(cfm_config_t *cfm_config, cfm_pkt_rmep_entry_t *rmep_entry, char priority, char dei)
{
	struct cpuport_info_t pktinfo;
	cfm_hdr_t cfm_hdr;
	cfm_pdu_lm_t pdu_lmm;
	cfm_tlv_end_t end_tlv;
	unsigned int pkt_offset;
	unsigned char frame_ptr[BUF_SIZE];
	unsigned char vlan[VLAN_TAG_OFFSET] = {0x81, 0x00, 0x00, 0x00};
	unsigned char ethtype[2] = {0x89, 0x02};

	// Fill DA
	memcpy(&frame_ptr, rmep_entry->rmep_mac, IFHWADDRLEN);
	pkt_offset = IFHWADDRLEN;

	// Fill SA
	memcpy(frame_ptr+pkt_offset, cfm_config->macaddr, IFHWADDRLEN);
	pkt_offset += IFHWADDRLEN;

	// Fill VLAN Tag
	{
		unsigned short equiv_vlan = (cfm_config->pri_vlan)?cfm_config->pri_vlan:cfm_config->assoc_vlans[0];
		unsigned char equiv_pri = (priority == -1)?cfm_config->priority:priority;
		if (equiv_vlan !=0 || equiv_pri !=0) {
			vlan[2] = (equiv_pri << 5) & 0xE0;
			vlan[2] |= (dei << 4) & 0x10;
			vlan[2] |= (equiv_vlan >> 8) & 0x1F;
			vlan[3] = equiv_vlan & 0xFF;
			memcpy(frame_ptr+pkt_offset, &vlan, sizeof(vlan));
			pkt_offset += sizeof(vlan);
		}
	}

	// Fill Ethertype
	memcpy(frame_ptr+pkt_offset, &ethtype, sizeof(ethtype));
	pkt_offset += sizeof(ethtype);

	// Fill CFM Common PDU
	cfm_hdr.md_level = cfm_config->md_level;
	cfm_hdr.version = 0;
	cfm_hdr.opcode = CFM_PDU_LMM;
	cfm_hdr.flags = 0x00;
	cfm_hdr.first_tlv_offset = sizeof(cfm_pdu_lm_t);
	memcpy(frame_ptr+pkt_offset, &cfm_hdr, sizeof(cfm_hdr));
	pkt_offset += sizeof(cfm_hdr);

	// Fill CFM LMM PDU
	pdu_lmm.TxFCf = htonl(cfm_config->CCIsentCCMs);
	pdu_lmm.RxFCf = 0;
	pdu_lmm.TxFCb = 0;
	memcpy(frame_ptr+pkt_offset, &pdu_lmm, sizeof(pdu_lmm));
	pkt_offset += sizeof(pdu_lmm);

	// Fill End TLV
	cfm_tlv_end(&end_tlv);
	memcpy(frame_ptr+pkt_offset, &end_tlv, sizeof(end_tlv));
	pkt_offset += sizeof(end_tlv);

	pktinfo.frame_ptr = frame_ptr;
	pktinfo.buf_len = BUF_SIZE;
	pktinfo.frame_len = (pkt_offset < MIN_ETH_FRAME_SIZE) ? MIN_ETH_FRAME_SIZE : pkt_offset;

	if(cfm_send_frame(cfm_config, &pktinfo, CFM_SEND_NORMAL) < 0) {
		return -1;
	}
	cfm_config->cfm_pkt_pm[CFM_PDU_LMM].tx++;
	return 0;
}

int
cfm_send_dmr(cfm_config_t *cfm_config, cfm_pktinfo_t *cfm_pktinfo)
{
	unsigned char *p;
	struct cpuport_info_t pktinfo;
	unsigned char mac[IFHWADDRLEN];
	cfm_pdu_dm_t *pdu_dmm, pdu_dmr;
	cfm_tlv_end_t end_tlv;

	p = (unsigned char *) cfm_pktinfo->cfm_hdr + sizeof(cfm_hdr_t);
	pdu_dmm = (cfm_pdu_dm_t *) p;

	pktinfo.frame_ptr = cfm_pktinfo->frame_ptr;
	pktinfo.rx_desc = cfm_pktinfo->rx_desc;
	pktinfo.tx_desc = cfm_pktinfo->tx_desc;
	pktinfo.frame_len = cfm_pktinfo->frame_len;
	pktinfo.frame_len = (pktinfo.frame_len < MIN_ETH_FRAME_SIZE) ? MIN_ETH_FRAME_SIZE : pktinfo.frame_len;

	memcpy(&mac, cfm_pktinfo->dst_mac_ptr, sizeof(mac));
	memcpy(cfm_pktinfo->dst_mac_ptr, cfm_pktinfo->src_mac_ptr, sizeof(mac));
	memcpy(cfm_pktinfo->src_mac_ptr, &mac, sizeof(mac));
	cfm_pktinfo->cfm_hdr->opcode = CFM_PDU_DMR;
	cfm_pktinfo->cfm_hdr->first_tlv_offset = sizeof(cfm_pdu_dm_t);
	cfm_pktinfo->cfm_hdr->flags = 0x00;

	// Fill End TLV
	cfm_tlv_end(&end_tlv);

	// Fill CFM DMR PDU
	pdu_dmr.TxTimestampf = pdu_dmm->TxTimestampf;	//timestamp from DMM field

	// cfm_config
	// First 32-bits are seconds, second 32-bits are nano-seconds, and network byte order
	cfm_util_us_to_64bit_ieee1588(cfm_config->RxTimestampf, &pdu_dmr.RxTimestampf);
	cfm_config->TxTimestampb = cfm_util_get_time_in_us();
	cfm_util_us_to_64bit_ieee1588(cfm_config->TxTimestampb, &pdu_dmr.TxTimestampb);
	pdu_dmr.RxTimestampb.sec = 0;
	pdu_dmr.RxTimestampb.nanosec = 0;

	// Copy PDU/TLV into CFM packet
	p = (unsigned char *) cfm_pktinfo->cfm_hdr + sizeof(cfm_hdr_t);
	memcpy(p, &pdu_dmr, sizeof(cfm_pdu_dm_t));
	p += sizeof(cfm_pdu_dm_t);
	memcpy(p, &end_tlv, sizeof(cfm_tlv_end_t));

	if(cfm_send_frame(cfm_config, &pktinfo, CFM_SEND_REPLY) < 0) {
		return -1;
	}
	cfm_config->cfm_pkt_pm[CFM_PDU_DMR].tx++;
	return 0;
}

int
cfm_send_dmm(cfm_config_t *cfm_config, unsigned char *target_mac, char priority, char dei)
{
	struct cpuport_info_t pktinfo;
	cfm_hdr_t cfm_hdr;
	cfm_pdu_dm_t pdu_dmm;
	cfm_tlv_end_t end_tlv;
	unsigned int pkt_offset;
	unsigned char frame_ptr[BUF_SIZE];
	unsigned char vlan[VLAN_TAG_OFFSET] = {0x81, 0x00, 0x00, 0x00};
	unsigned char ethtype[2] = {0x89, 0x02};

	// Fill DA
	memcpy(&frame_ptr, target_mac, IFHWADDRLEN);
	pkt_offset = IFHWADDRLEN;

	// Fill SA
	memcpy(frame_ptr+pkt_offset, cfm_config->macaddr, IFHWADDRLEN);
	pkt_offset += IFHWADDRLEN;

	// Fill VLAN Tag
	{
		unsigned short equiv_vlan = (cfm_config->pri_vlan)?cfm_config->pri_vlan:cfm_config->assoc_vlans[0];
		unsigned char equiv_pri = (priority == -1)?cfm_config->priority:priority;
		if (equiv_vlan !=0 || equiv_pri !=0) {
			vlan[2] = (equiv_pri << 5) & 0xE0;
			vlan[2] |= (dei << 4) & 0x10;
			vlan[2] |= (equiv_vlan >> 8) & 0x1F;
			vlan[3] = equiv_vlan & 0xFF;
			memcpy(frame_ptr+pkt_offset, &vlan, sizeof(vlan));
			pkt_offset += sizeof(vlan);
		}
	}

	// Fill Ethertype
	memcpy(frame_ptr+pkt_offset, &ethtype, sizeof(ethtype));
	pkt_offset += sizeof(ethtype);

	// Fill CFM Common PDU
	cfm_hdr.md_level = cfm_config->md_level;
	cfm_hdr.version = 0;
	cfm_hdr.opcode = CFM_PDU_DMM;
	cfm_hdr.flags = 0x00;
	cfm_hdr.first_tlv_offset = sizeof(cfm_pdu_dm_t);
	memcpy(frame_ptr+pkt_offset, &cfm_hdr, sizeof(cfm_hdr));
	pkt_offset += sizeof(cfm_hdr);

	// Fill CFM DMM PDU
	// First 32-bits are seconds, second 32-bits are nano-seconds, and network byte order
	cfm_config->TxTimestampf = cfm_util_get_time_in_us();
	cfm_util_us_to_64bit_ieee1588(cfm_config->TxTimestampf, &pdu_dmm.TxTimestampf);

	pdu_dmm.RxTimestampf.sec = 0;
	pdu_dmm.RxTimestampf.nanosec = 0;
	pdu_dmm.TxTimestampb.sec = 0;
	pdu_dmm.TxTimestampb.nanosec = 0;
	pdu_dmm.RxTimestampb.sec = 0;
	pdu_dmm.RxTimestampb.nanosec = 0;
	memcpy(frame_ptr+pkt_offset, &pdu_dmm, sizeof(pdu_dmm));
	pkt_offset += sizeof(pdu_dmm);

	// Fill DATA TLV
	if( cfm_config->tlv_data_length > 0 && cfm_config->payload_format == OPTION_TLV_DATA) {
		pkt_offset=fill_data_tlv(cfm_config, frame_ptr, pkt_offset);
	}

	// Fill End TLV
	cfm_tlv_end(&end_tlv);
	memcpy(frame_ptr+pkt_offset, &end_tlv, sizeof(end_tlv));
	pkt_offset += sizeof(end_tlv);

	// Before sending DMM, flush DMR MAC first
	memcpy(&cfm_config->cfm_send_dmm_mac, target_mac, IFHWADDRLEN);
	memset(&cfm_config->cfm_recv_dmr_mac, 0, IFHWADDRLEN);
	cfm_config->RxTimestampf = 0;
	cfm_config->TxTimestampb = 0;
	cfm_config->RxTimestampb = 0;

	pktinfo.frame_ptr = frame_ptr;
	pktinfo.buf_len = BUF_SIZE;
	pktinfo.frame_len = (pkt_offset < MIN_ETH_FRAME_SIZE) ? MIN_ETH_FRAME_SIZE : pkt_offset;

	if(cfm_send_frame(cfm_config, &pktinfo, CFM_SEND_NORMAL) < 0) {
		return -1;
	}
	cfm_config->cfm_pkt_pm[CFM_PDU_DMM].tx++;
	return 0;
}

int
cfm_send_1dm(cfm_config_t *cfm_config, unsigned char *target_mac, char priority, char dei)
{
	struct cpuport_info_t pktinfo;
	cfm_hdr_t cfm_hdr;
	cfm_pdu_1dm_t pdu_1dm;
	cfm_tlv_end_t end_tlv;
	unsigned int pkt_offset;
	unsigned char frame_ptr[BUF_SIZE];
	unsigned char vlan[VLAN_TAG_OFFSET] = {0x81, 0x00, 0x00, 0x00};
	unsigned char ethtype[2] = {0x89, 0x02};

	// Fill DA
	memcpy(&frame_ptr, target_mac, IFHWADDRLEN);
	pkt_offset = IFHWADDRLEN;

	// Fill SA
	memcpy(frame_ptr+pkt_offset, cfm_config->macaddr, IFHWADDRLEN);
	pkt_offset += IFHWADDRLEN;

	// Fill VLAN Tag
	{
		unsigned short equiv_vlan = (cfm_config->pri_vlan)?cfm_config->pri_vlan:cfm_config->assoc_vlans[0];
		unsigned char equiv_pri = (priority == -1)?cfm_config->priority:priority;
		if (equiv_vlan !=0 || equiv_pri !=0) {
			vlan[2] = (equiv_pri << 5) & 0xE0;
			vlan[2] |= (dei << 4) & 0x10;
			vlan[2] |= (equiv_vlan >> 8) & 0x1F;
			vlan[3] = equiv_vlan & 0xFF;
			memcpy(frame_ptr+pkt_offset, &vlan, sizeof(vlan));
			pkt_offset += sizeof(vlan);
		}
	}

	// Fill Ethertype
	memcpy(frame_ptr+pkt_offset, &ethtype, sizeof(ethtype));
	pkt_offset += sizeof(ethtype);

	// Fill CFM Common PDU
	cfm_hdr.md_level = cfm_config->md_level;
	cfm_hdr.version = 0;
	cfm_hdr.opcode = CFM_PDU_1DM;
	cfm_hdr.flags = 0x00;
	cfm_hdr.first_tlv_offset = sizeof(cfm_pdu_1dm_t);
	memcpy(frame_ptr+pkt_offset, &cfm_hdr, sizeof(cfm_hdr));
	pkt_offset += sizeof(cfm_hdr);

	// Fill CFM DMM PDU
	// First 32-bits are seconds, second 32-bits are nano-seconds, and network byte order
	cfm_config->TxTimestampf = cfm_util_get_time_in_us();
	cfm_util_us_to_64bit_ieee1588(cfm_config->TxTimestampf, &pdu_1dm.TxTimestampf);

	pdu_1dm.RxTimestampf.sec = 0;
	pdu_1dm.RxTimestampf.nanosec = 0;

	memcpy(frame_ptr+pkt_offset, &pdu_1dm, sizeof(pdu_1dm));
	pkt_offset += sizeof(pdu_1dm);

	// Fill DATA TLV
	if( cfm_config->tlv_data_length > 0 && cfm_config->payload_format == OPTION_TLV_DATA) {
		pkt_offset=fill_data_tlv(cfm_config, frame_ptr, pkt_offset);
	}
	// Fill End TLV
	cfm_tlv_end(&end_tlv);
	memcpy(frame_ptr+pkt_offset, &end_tlv, sizeof(end_tlv));
	pkt_offset += sizeof(end_tlv);

	pktinfo.frame_ptr = frame_ptr;
	pktinfo.buf_len = BUF_SIZE;
	pktinfo.frame_len = (pkt_offset < MIN_ETH_FRAME_SIZE) ? MIN_ETH_FRAME_SIZE : pkt_offset;

	if(cfm_send_frame(cfm_config, &pktinfo, CFM_SEND_NORMAL) < 0) {
		return -1;
	}
	cfm_config->cfm_pkt_pm[CFM_PDU_1DM].tx++;
	return 0;
}

int
cfm_send_tst(cfm_config_t *cfm_config, unsigned char *target_mac, char priority, char dei)
{
	struct cpuport_info_t pktinfo;
	cfm_hdr_t cfm_hdr;
	cfm_pdu_tst_t pdu_tst;
	cfm_tlv_end_t end_tlv;
	unsigned int pkt_offset;
	unsigned char frame_ptr[BUF_SIZE];
	unsigned char vlan[VLAN_TAG_OFFSET] = {0x81, 0x00, 0x00, 0x00};
	unsigned char ethtype[2] = {0x89, 0x02};

	// Fill DA
	memcpy(&frame_ptr, target_mac, IFHWADDRLEN);
	pkt_offset = IFHWADDRLEN;

	// Fill SA
	memcpy(frame_ptr+pkt_offset, cfm_config->macaddr, IFHWADDRLEN);
	pkt_offset += IFHWADDRLEN;

	// Fill VLAN Tag
	{
		unsigned short equiv_vlan = (cfm_config->pri_vlan)?cfm_config->pri_vlan:cfm_config->assoc_vlans[0];
		unsigned char equiv_pri = (priority == -1)?cfm_config->priority:priority;
		if (equiv_vlan !=0 || equiv_pri !=0) {
			vlan[2] = (equiv_pri << 5) & 0xE0;
			vlan[2] |= (dei << 4) & 0x10;
			vlan[2] |= (equiv_vlan >> 8) & 0x1F;
			vlan[3] = equiv_vlan & 0xFF;
			memcpy(frame_ptr+pkt_offset, &vlan, sizeof(vlan));
			pkt_offset += sizeof(vlan);
		}
	}

	// Fill Ethertype
	memcpy(frame_ptr+pkt_offset, &ethtype, sizeof(ethtype));
	pkt_offset += sizeof(ethtype);

	// Fill CFM Common PDU
	cfm_hdr.md_level = cfm_config->md_level;
	cfm_hdr.version = 0;
	cfm_hdr.opcode = CFM_PDU_TST;
	cfm_hdr.flags = 0x00;
	cfm_hdr.first_tlv_offset = sizeof(cfm_pdu_tst_t);	//4
	memcpy(frame_ptr+pkt_offset, &cfm_hdr, sizeof(cfm_hdr));
	pkt_offset += sizeof(cfm_hdr);

	// Fill CFM TST PDU
	pdu_tst.sequence_number= htonl(++(cfm_config->tst_sequence_number));
	memcpy(frame_ptr+pkt_offset, &pdu_tst, sizeof(pdu_tst));
	pkt_offset += sizeof(pdu_tst);

	util_dbprintf(omci_env_g.debug_level_cfm, LOG_ERR, 0, "\nSend TST sequence_number=0x%04X\n", cfm_config->tst_sequence_number);
	pkt_offset=fill_test_tlv(cfm_config, frame_ptr, pkt_offset);

	// Fill End TLV
	cfm_tlv_end(&end_tlv);
	memcpy(frame_ptr+pkt_offset, &end_tlv, sizeof(end_tlv));
	pkt_offset += sizeof(end_tlv);

	pktinfo.frame_ptr = frame_ptr;
	pktinfo.frame_len = (pkt_offset < MIN_ETH_FRAME_SIZE) ? MIN_ETH_FRAME_SIZE : pkt_offset;
	pktinfo.buf_len = BUF_SIZE;

	if(cfm_send_frame(cfm_config, &pktinfo, CFM_SEND_NORMAL) < 0) {
		return -1;
	}
	cfm_config->cfm_pkt_pm[CFM_PDU_TST].tx++;
	return 0;
}

int
cfm_send_1sl(cfm_config_t *cfm_config,cfm_pkt_rmep_entry_t *rmep_entry, char priority, char dei)
{
	struct cpuport_info_t pktinfo;
	cfm_hdr_t cfm_hdr;
	cfm_pdu_sl_t pdu_1sl;
	cfm_tlv_end_t end_tlv;
	unsigned int pkt_offset;
	unsigned char frame_ptr[BUF_SIZE];
	unsigned char vlan[VLAN_TAG_OFFSET] = {0x81, 0x00, 0x00, 0x00};
	unsigned char ethtype[2] = {0x89, 0x02};

	// Fill DA
	memcpy(&frame_ptr, rmep_entry->rmep_mac , IFHWADDRLEN);
	pkt_offset = IFHWADDRLEN;

	// Fill SA
	memcpy(frame_ptr+pkt_offset, cfm_config->macaddr, IFHWADDRLEN);
	pkt_offset += IFHWADDRLEN;

	// Fill VLAN Tag
	{
		unsigned short equiv_vlan = (cfm_config->pri_vlan)?cfm_config->pri_vlan:cfm_config->assoc_vlans[0];
		unsigned char equiv_pri = (priority == -1)?cfm_config->priority:priority;
		if (equiv_vlan !=0 || equiv_pri !=0) {
			vlan[2] = (equiv_pri << 5) & 0xE0;
			vlan[2] |= (dei << 4) & 0x10;
			vlan[2] |= (equiv_vlan >> 8) & 0x1F;
			vlan[3] = equiv_vlan & 0xFF;
			memcpy(frame_ptr+pkt_offset, &vlan, sizeof(vlan));
			pkt_offset += sizeof(vlan);
		}
	}

	// Fill Ethertype
	memcpy(frame_ptr+pkt_offset, &ethtype, sizeof(ethtype));
	pkt_offset += sizeof(ethtype);

	// Fill CFM Common PDU
	cfm_hdr.md_level = cfm_config->md_level;
	cfm_hdr.version = 0;
	cfm_hdr.opcode = CFM_PDU_1SL;
	cfm_hdr.flags = 0x00;
	cfm_hdr.first_tlv_offset = sizeof(cfm_pdu_sl_t);
	memcpy(frame_ptr+pkt_offset, &cfm_hdr, sizeof(cfm_hdr));
	pkt_offset += sizeof(cfm_hdr);

	// Fill CFM 1SL PDU
	pdu_1sl.source_mep_id = htons(cfm_config->mep_id);
	pdu_1sl.responder_mep_id = 0;
	pdu_1sl.test_id = htonl(rmep_entry->test_id_1sl);
	pdu_1sl.TxFCf = htonl(rmep_entry->TxFCl_1sl);
	pdu_1sl.TxFCb = 0;
	memcpy(frame_ptr+pkt_offset, &pdu_1sl, sizeof(pdu_1sl));
	pkt_offset += sizeof(pdu_1sl);

	// Fill DATA TLV
	if( cfm_config->tlv_data_length > 0 && cfm_config->payload_format == OPTION_TLV_DATA) {
		pkt_offset=fill_data_tlv(cfm_config, frame_ptr, pkt_offset);
	}

	// Fill End TLV
	cfm_tlv_end(&end_tlv);
	memcpy(frame_ptr+pkt_offset, &end_tlv, sizeof(end_tlv));
	pkt_offset += sizeof(end_tlv);

	pktinfo.frame_ptr = frame_ptr;
	pktinfo.buf_len = BUF_SIZE;
	pktinfo.frame_len = (pkt_offset < MIN_ETH_FRAME_SIZE) ? MIN_ETH_FRAME_SIZE : pkt_offset;

	if(cfm_send_frame(cfm_config, &pktinfo, CFM_SEND_NORMAL) < 0) {
		return -1;
	}
	cfm_config->cfm_pkt_pm[CFM_PDU_1SL].tx++;
	return 0;
}

int
cfm_send_slr(cfm_config_t *cfm_config, cfm_pkt_rmep_entry_t *rmep_entry, cfm_pktinfo_t *cfm_pktinfo)
{
	unsigned char *p;
	struct cpuport_info_t pktinfo;
	unsigned char mac[IFHWADDRLEN];
	cfm_pdu_sl_t *pdu_slm, pdu_slr;
	cfm_tlv_end_t end_tlv;

	p = (unsigned char *) cfm_pktinfo->cfm_hdr + sizeof(cfm_hdr_t);
	pdu_slm = (cfm_pdu_sl_t *) p;

	pktinfo.frame_ptr = cfm_pktinfo->frame_ptr;
	pktinfo.rx_desc = cfm_pktinfo->rx_desc;
	pktinfo.tx_desc = cfm_pktinfo->tx_desc;
	pktinfo.frame_len = cfm_pktinfo->frame_len;
	pktinfo.frame_len = (pktinfo.frame_len < MIN_ETH_FRAME_SIZE) ? MIN_ETH_FRAME_SIZE : pktinfo.frame_len;

	memcpy(&mac, cfm_pktinfo->dst_mac_ptr, sizeof(mac));
	memcpy(cfm_pktinfo->dst_mac_ptr, cfm_pktinfo->src_mac_ptr, sizeof(mac));
	memcpy(cfm_pktinfo->src_mac_ptr, &mac, sizeof(mac));
	cfm_pktinfo->cfm_hdr->opcode = CFM_PDU_SLR;
	cfm_pktinfo->cfm_hdr->first_tlv_offset = sizeof(cfm_pdu_sl_t);
	cfm_pktinfo->cfm_hdr->flags = 0x00;

	// Fill End TLV
	cfm_tlv_end(&end_tlv);

	// Fill CFM SLR PDU
	pdu_slr.source_mep_id = pdu_slm->source_mep_id;
	pdu_slr.responder_mep_id = htons(cfm_config->mep_id);
	pdu_slr.test_id = pdu_slm->test_id;
	pdu_slr.TxFCf = pdu_slm->TxFCf;
	pdu_slr.TxFCb = htonl(rmep_entry->TxFCl_slr);

	// Copy PDU/TLV into CFM packet
	p = (unsigned char *) cfm_pktinfo->cfm_hdr + sizeof(cfm_hdr_t);
	memcpy(p, &pdu_slr, sizeof(cfm_pdu_sl_t));
	p += sizeof(cfm_pdu_sl_t);
	memcpy(p, &end_tlv, sizeof(cfm_tlv_end_t));

	if(cfm_send_frame(cfm_config, &pktinfo, CFM_SEND_REPLY) < 0) {
		return -1;
	}
	return 0;
}

int
cfm_send_slm(cfm_config_t *cfm_config, cfm_pkt_rmep_entry_t *rmep_entry, char priority, char dei)
{
	struct cpuport_info_t pktinfo;
	cfm_hdr_t cfm_hdr;
	cfm_pdu_sl_t pdu_slm;
	cfm_tlv_end_t end_tlv;
	unsigned int pkt_offset;
	unsigned char frame_ptr[BUF_SIZE];
	unsigned char vlan[VLAN_TAG_OFFSET] = {0x81, 0x00, 0x00, 0x00};
	unsigned char ethtype[2] = {0x89, 0x02};

	// Fill DA
	memcpy(&frame_ptr, rmep_entry->rmep_mac , IFHWADDRLEN);
	pkt_offset = IFHWADDRLEN;

	// Fill SA
	memcpy(frame_ptr+pkt_offset, cfm_config->macaddr, IFHWADDRLEN);
	pkt_offset += IFHWADDRLEN;

	// Fill VLAN Tag
	{
		unsigned short equiv_vlan = (cfm_config->pri_vlan)?cfm_config->pri_vlan:cfm_config->assoc_vlans[0];
		unsigned char equiv_pri = (priority == -1)?cfm_config->priority:priority;
		if (equiv_vlan !=0 || equiv_pri !=0) {
			vlan[2] = (equiv_pri << 5) & 0xE0;
			vlan[2] |= (dei << 4) & 0x10;
			vlan[2] |= (equiv_vlan >> 8) & 0x1F;
			vlan[3] = equiv_vlan & 0xFF;
			memcpy(frame_ptr+pkt_offset, &vlan, sizeof(vlan));
			pkt_offset += sizeof(vlan);
		}
	}

	// Fill Ethertype
	memcpy(frame_ptr+pkt_offset, &ethtype, sizeof(ethtype));
	pkt_offset += sizeof(ethtype);

	// Fill CFM Common PDU
	cfm_hdr.md_level = cfm_config->md_level;
	cfm_hdr.version = 0;
	cfm_hdr.opcode = CFM_PDU_SLM;
	cfm_hdr.flags = 0x00;
	cfm_hdr.first_tlv_offset = sizeof(cfm_pdu_sl_t);
	memcpy(frame_ptr+pkt_offset, &cfm_hdr, sizeof(cfm_hdr));
	pkt_offset += sizeof(cfm_hdr);

	// Fill CFM SLM PDU
	pdu_slm.source_mep_id = htons(cfm_config->mep_id);
	pdu_slm.responder_mep_id = 0;
	pdu_slm.test_id = htonl(rmep_entry->test_id_slm);
	pdu_slm.TxFCf = htonl(rmep_entry->TxFCl_slm);
	pdu_slm.TxFCb = 0;
	memcpy(frame_ptr+pkt_offset, &pdu_slm, sizeof(pdu_slm));
	pkt_offset += sizeof(pdu_slm);

	// Fill DATA TLV
	if( cfm_config->tlv_data_length > 0 && cfm_config->payload_format == OPTION_TLV_DATA) {
		pkt_offset=fill_data_tlv(cfm_config, frame_ptr, pkt_offset);
	}
	// Fill End TLV
	cfm_tlv_end(&end_tlv);
	memcpy(frame_ptr+pkt_offset, &end_tlv, sizeof(end_tlv));
	pkt_offset += sizeof(end_tlv);

	pktinfo.frame_ptr = frame_ptr;
	pktinfo.buf_len = BUF_SIZE;
	pktinfo.frame_len = (pkt_offset < MIN_ETH_FRAME_SIZE) ? MIN_ETH_FRAME_SIZE : pkt_offset;

	if(cfm_send_frame(cfm_config, &pktinfo, CFM_SEND_NORMAL) < 0) {
		return -1;
	}
	return 0;
}

int
cfm_send_ccm(cfm_config_t *cfm_config, char priority, char dei)
{
	int is_sender_id, is_chassis, is_manage, is_rdi;
	unsigned int md_len, ma_len;
	char md_name[49] = {0};
	char ma_name[49] = {0};
	struct cpuport_info_t pktinfo;
	cfm_hdr_t cfm_hdr;
	cfm_pdu_ccm_t pdu_ccm;
	cfm_tlv_sender_id_t sender_id_tlv;
	cfm_tlv_end_t end_tlv;
	unsigned int pkt_offset;
	unsigned char d_mac[IFHWADDRLEN] = {0x01, 0x80, 0xC2, 0x00, 0x00, 0x30};
	unsigned char vlan[VLAN_TAG_OFFSET] = {0x81, 0x00, 0x00, 0x00};
	unsigned char ethtype[2] = {0x89, 0x02};
	unsigned char frame_ptr[BUF_SIZE];

	is_rdi = 0;
	// by default, all defect except the DEFECT_RDI_CCM are counted in cfm_defect_mask_in_rdi_gen
	// DEFECT_RDI_CCM is not counted is to avoid mutual-rdi set but actually no defect exist
	if (cfm_config->defect_status) {
		unsigned char defect = cfm_config->defect_status;
		unsigned long long now = cfm_util_get_time_in_us();
		unsigned int ais_interval_us = ((cfm_config->eth_ais_control&1)?60:1) *1000 *1000;
		if (cfm_config->defect_status & (1<<DEFECT_AIS))			// ais received, remove defect supressed by ais
			defect &= (~omci_env_g.cfm_defect_mask_supressed_by_ais);
		if (now - cfm_config->recv_lck_timestamp <= ais_interval_us*3.5)	// lck received, remove defect supressed by lck
			defect &= (~omci_env_g.cfm_defect_mask_supressed_by_lck);	// while recv lck doesnt mean defect, it also applies 3.5*ais_interval as timeout
		if (defect & omci_env_g.cfm_defect_mask_in_rdi_gen) {
			is_rdi = 1;
			dbprintf_cfm(LOG_INFO, "%s ccm is_rdi=%d (defect=0x%x)\n", cfm_print_get_cfm_config_str(cfm_config), is_rdi, defect);
		}
	}

	// Fill DA
	d_mac[5] |= cfm_config->md_level;
	memcpy(&frame_ptr, &d_mac, IFHWADDRLEN);
	pkt_offset = IFHWADDRLEN;

	// Fill SA
	memcpy(frame_ptr+pkt_offset, cfm_config->macaddr, IFHWADDRLEN);
	pkt_offset += IFHWADDRLEN;

	// Fill VLAN Tag
	{
		unsigned short equiv_vlan = (cfm_config->pri_vlan)?cfm_config->pri_vlan:cfm_config->assoc_vlans[0];
		unsigned char equiv_pri = (priority == -1)?cfm_config->priority:priority;
		if (equiv_vlan !=0 || equiv_pri !=0) {
			vlan[2] = (equiv_pri << 5) & 0xE0;
			vlan[2] |= (dei << 4) & 0x10;
			vlan[2] |= (equiv_vlan >> 8) & 0x1F;
			vlan[3] = equiv_vlan & 0xFF;
			memcpy(frame_ptr+pkt_offset, &vlan, sizeof(vlan));
			pkt_offset += sizeof(vlan);
		}
	}

	// Fill Ethertype
	memcpy(frame_ptr+pkt_offset, &ethtype, sizeof(ethtype));
	pkt_offset += sizeof(ethtype);

	// Fill CFM Common PDU
	cfm_hdr.md_level = cfm_config->md_level;
	cfm_hdr.version = 0;
	cfm_hdr.opcode = CFM_PDU_CCM;
	cfm_hdr.flags = cfm_config->ccm_interval | (is_rdi?(1<<7):0);
	cfm_hdr.first_tlv_offset = sizeof(cfm_pdu_ccm_t);
	memcpy(frame_ptr+pkt_offset, &cfm_hdr, sizeof(cfm_hdr));
	pkt_offset += sizeof(cfm_hdr);

	// Fill CFM CCM PDU
	pdu_ccm.seq = htonl(cfm_config->CCIsentCCMs);
	pdu_ccm.mep_id = htons(cfm_config->mep_id);

	memset(pdu_ccm.ma_id, 0, sizeof(pdu_ccm.ma_id));
	pdu_ccm.ma_id[0] = cfm_config->md_format; // MD Format

	strncpy(ma_name, cfm_config->ma_name_1, 25);
	ma_name[25]=0;
	strncat(ma_name, cfm_config->ma_name_2, 48-strlen(ma_name));
	ma_name[48]=0;
	if(cfm_config->md_format != MD_NAME_FORMAT_NONE) {
		strncpy(md_name, cfm_config->md_name_1, 25);
		md_name[25]=0;
		strncat(md_name, cfm_config->md_name_2, 48-strlen(md_name));
		md_name[48]=0;

		switch(cfm_config->md_format) { // MD Length
			case MD_NAME_FORMAT_MACADDR_SHORTINT:
				pdu_ccm.ma_id[1] = md_len = 8;
				memcpy(pdu_ccm.ma_id+2, md_name, md_len);
				break;
			case MD_NAME_FORMAT_ICC:
				pdu_ccm.ma_id[1] = md_len = 13;
				memcpy(pdu_ccm.ma_id+2, md_name, md_len);
				break;
			case MD_NAME_FORMAT_DNS:
			case MD_NAME_FORMAT_STRING:
			default:
				pdu_ccm.ma_id[1] = md_len = strlen(md_name);
				memcpy(pdu_ccm.ma_id+2, md_name, md_len);
				break;
		}

		pdu_ccm.ma_id[2+md_len] = cfm_config->ma_format; // MA Format
		switch(cfm_config->ma_format) { // MA Length
			case MA_NAME_FORMAT_VID:
				pdu_ccm.ma_id[2+md_len+1] = ma_len = 2;
				*(unsigned short*)(pdu_ccm.ma_id+2+md_len+2) = htons(cfm_config->pri_vlan);
				break;
			case MA_NAME_FORMAT_SHORTINT:
				pdu_ccm.ma_id[2+md_len+1] = ma_len = 2;
				memcpy(pdu_ccm.ma_id+2+md_len+2, ma_name, ma_len); // MA Name_1
				break;
			case MA_NAME_FORMAT_STRING:
			default:
				pdu_ccm.ma_id[2+md_len+1] = ma_len = strlen(ma_name);
				memcpy(pdu_ccm.ma_id+2+md_len+2, ma_name, ma_len); // MA Name_1
				break;
			case MA_NAME_FORMAT_VPNID:
				pdu_ccm.ma_id[2+md_len+1] = ma_len = 7;
				memcpy(pdu_ccm.ma_id+2+md_len+2, ma_name, ma_len); // MA Name_1
				break;
			case MA_NAME_FORMAT_ICC:
				pdu_ccm.ma_id[2+md_len+1] = ma_len = 13;
				memcpy(pdu_ccm.ma_id+2+md_len+2, ma_name, ma_len); // MA Name_1
				break;
		}
	} else {
		pdu_ccm.ma_id[1] = cfm_config->ma_format; // MA Format
		switch(cfm_config->ma_format) { // MA Length
			case MA_NAME_FORMAT_VID:
				pdu_ccm.ma_id[2] = ma_len = 2;
				*(unsigned short*)(pdu_ccm.ma_id+3) = htons(cfm_config->pri_vlan);
				break;
			case MA_NAME_FORMAT_SHORTINT:
				pdu_ccm.ma_id[2] = ma_len = 2;
				memcpy(pdu_ccm.ma_id+3, ma_name, ma_len); // MA Name
				break;
			case MA_NAME_FORMAT_STRING:
			default:
				pdu_ccm.ma_id[2] = ma_len = strlen(ma_name); // MA Length
				memcpy(pdu_ccm.ma_id+3, ma_name, ma_len); // MA Name
				break;
			case MA_NAME_FORMAT_VPNID:
				pdu_ccm.ma_id[2] = ma_len = 7; // MA Length
				memcpy(pdu_ccm.ma_id+3, ma_name, ma_len); // MA Name
				break;
			case MA_NAME_FORMAT_ICC:
				pdu_ccm.ma_id[2] = ma_len = 13; // MA Length
				memcpy(pdu_ccm.ma_id+3, ma_name, ma_len); // MA Name
				break;
		}
	}

	if ( cfm_config->lmm_dual_test_enable == 0 ) {
		pdu_ccm.y_1731[0] = 0;
		pdu_ccm.y_1731[1] = 0;
		pdu_ccm.y_1731[2] = 0;
		pdu_ccm.y_1731[3] = 0;

	}else {
		if ( cfm_config->lm_dual_test_rmep ) {
			cfm_pkt_rmep_entry_t *rmep_entry;
			list_for_each_entry(rmep_entry, &cfm_config->cfm_recv_rmep_list, node) {
				if ( rmep_entry->rmep_id == cfm_config->lm_dual_test_rmep )  {
					rmep_entry->TxFCl_ccm = cfm_config->CCIsentCCMs;
					pdu_ccm.y_1731[0] = htonl(rmep_entry->TxFCl_ccm);
					pdu_ccm.y_1731[1] = htonl(rmep_entry->RxFCl_ccm);
					pdu_ccm.y_1731[2] = htonl(rmep_entry->TxFCf_ccm);
				}
			}
		}else{
			pdu_ccm.y_1731[0] = htonl(cfm_config->CCIsentCCMs);
			pdu_ccm.y_1731[1] = 0;
			pdu_ccm.y_1731[2] = 0;
		}
	}
	pdu_ccm.y_1731[3] = 0;
	memcpy(frame_ptr+pkt_offset, &pdu_ccm, sizeof(pdu_ccm));
	pkt_offset += sizeof(pdu_ccm);

	// Fill Sender ID TLV
	is_sender_id = cfm_is_send_tlv_sender_id(cfm_config, &is_chassis, &is_manage);
	if(is_sender_id == TRUE && cfm_config->send_cc_senderid == 1) {
		cfm_tlv_sender_id(cfm_config, &sender_id_tlv);
		memcpy(frame_ptr+pkt_offset, &sender_id_tlv, htons(sender_id_tlv.len)+3);
		pkt_offset += (htons(sender_id_tlv.len)+3);
	}

	// Fill Port Status TLV
	if (cfm_config->send_cc_port_status == 1) {
		int is_linkup = switch_get_linkup_logical_portmask() & (1 << cfm_config->logical_port_id);
		if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX)==0) {
			// if port_type is WAN, find its peer UNI state
			if (cfm_config->port_type == ENV_BRIDGE_PORT_TYPE_WAN) {
				struct me_t *peer_me = cfm_bport_find_peer(cfm_config);
				if(peer_me) {
					struct me_t *ibridgeport_me = hwresource_public2private(peer_me);
					if (ibridgeport_me) {
						unsigned char logical_port_id = meinfo_util_me_attr_data(ibridgeport_me, 6);
						is_linkup = switch_get_linkup_logical_portmask() & (1 << logical_port_id);
					}
				}
			}
		}
		cfm_tlv_port_status_t port_status_tlv;
		cfm_tlv_port_status(cfm_config, &port_status_tlv, is_linkup);
		memcpy(frame_ptr+pkt_offset, &port_status_tlv, 4);
		pkt_offset += 4;
	}

	// Fill Interface Status TLV
	if (cfm_config->send_cc_interface_status == 1) {
		int is_linkup = switch_get_linkup_logical_portmask() & (1 << cfm_config->logical_port_id);
		if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX)==0) {
			// if port_type is WAN, find its peer UNI state
			if (cfm_config->port_type == ENV_BRIDGE_PORT_TYPE_WAN) {
				struct me_t *peer_me = cfm_bport_find_peer(cfm_config);
				if(peer_me) {
					struct me_t *ibridgeport_me = hwresource_public2private(peer_me);
					if (ibridgeport_me) {
						unsigned char logical_port_id = meinfo_util_me_attr_data(ibridgeport_me, 6);
						is_linkup = switch_get_linkup_logical_portmask() & (1 << logical_port_id);
					}
				}
			}
		}
		cfm_tlv_interface_status_t interface_status_tlv;
		cfm_tlv_interface_status(cfm_config, &interface_status_tlv, is_linkup);
		memcpy(frame_ptr+pkt_offset, &interface_status_tlv, 4);
		pkt_offset += 4;
	}

	// Fill DATA TLV
	if( cfm_config->tlv_data_length > 0 && cfm_config->payload_format == OPTION_TLV_DATA && cfm_config->send_cc_data_status == 1) {
		pkt_offset = fill_data_tlv(cfm_config, frame_ptr, pkt_offset);
	}

	// Fill End TLV
	cfm_tlv_end(&end_tlv);
	memcpy(frame_ptr+pkt_offset, &end_tlv, sizeof(end_tlv));
	pkt_offset += sizeof(end_tlv);

	pktinfo.frame_ptr = frame_ptr;
	pktinfo.buf_len = BUF_SIZE;
	pktinfo.frame_len = (pkt_offset < MIN_ETH_FRAME_SIZE) ? MIN_ETH_FRAME_SIZE : pkt_offset;
#if 0 // Calix JIRA GI-2188
	// Workaround for Calix, when uni link is down, do not send ccm
	if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX)==0 && omci_env_g.olt_workaround_enable == ENV_OLT_WORKAROUND_CALIX) {
		if (cfm_config->send_ccm_port_status == 0 && cfm_config->send_ccm_interface_status == 0) {
			int is_linkup = switch_get_linkup_logical_portmask() & (1 << cfm_config->logical_port_id);
			// if port_type is WAN, find its peer UNI state
			if (cfm_config->port_type == ENV_BRIDGE_PORT_TYPE_WAN) {
				struct me_t *peer_me = cfm_bport_find_peer(cfm_config);
				if(peer_me) {
					struct me_t *ibridgeport_me = hwresource_public2private(peer_me);
					if (ibridgeport_me) {
						unsigned char logical_port_id = meinfo_util_me_attr_data(ibridgeport_me, 6);
						is_linkup = switch_get_linkup_logical_portmask() & (1 << logical_port_id);
					}
				}
			}
			if (!is_linkup) {
				dbprintf_cfm(LOG_WARNING, "Do not send CCM due to link down (md_level=%d mep_id=%d)\n", cfm_config->md_level, cfm_config->mep_id);
				return 0;
			}
		}
	}
#endif
	if(cfm_send_frame(cfm_config, &pktinfo, CFM_SEND_NORMAL) < 0) {
		return -1;
	}
	if(is_rdi)
		cfm_config->cfm_pkt_pm[CFM_PDU_CCM].tx_rdi++;
	else
		cfm_config->cfm_pkt_pm[CFM_PDU_CCM].tx++;
	return 0;
}

int
cfm_forward_cfm(cfm_config_t *cfm_config, struct cpuport_info_t *pktinfo, cfm_pktinfo_t *cfm_pktinfo, unsigned char cfm_pdu)
{
	if(cfm_pdu == CFM_PDU_LTM) {
		unsigned char *p;
		cfm_pdu_ltm_t *pdu_ltm;
		cfm_tlv_ltm_egress_id_t *ltm_egress_id_tlv;

		p = (unsigned char *) cfm_pktinfo->cfm_hdr + sizeof(cfm_hdr_t);
		pdu_ltm = (cfm_pdu_ltm_t *) p;
		//memcpy(pdu_ltm->origin_addr, &mac, IFHWADDRLEN);
		pdu_ltm->ttl--;
		p += sizeof(cfm_pdu_ltm_t);
		ltm_egress_id_tlv = (cfm_tlv_ltm_egress_id_t *) p;
		ltm_egress_id_tlv->egress_id = htons((cfm_config->egress_id[6] << 8) | cfm_config->egress_id[7]);
		memcpy(ltm_egress_id_tlv->egress_mac, cfm_config->macaddr, IFHWADDRLEN);
		memcpy(cfm_pktinfo->src_mac_ptr, cfm_config->macaddr, IFHWADDRLEN);
		memcpy(pktinfo->src_mac, cfm_config->macaddr, IFHWADDRLEN);
	} else if(cfm_pdu == CFM_PDU_LTR) {
		unsigned char *p;
		cfm_pdu_ltr_t *pdu_ltr;
		//cfm_tlv_ltr_egress_id_t *ltr_egress_id_tlv;

		p = (unsigned char *) cfm_pktinfo->cfm_hdr + sizeof(cfm_hdr_t);
		pdu_ltr = (cfm_pdu_ltr_t *) p;
		// replace destination mac if it is ont mac
		if(memcmp(cfm_pktinfo->dst_mac_ptr, cfm_config->macaddr, IFHWADDRLEN) == 0) {
			cfm_pkt_ltm_entry_t *ltm_entry;
			list_for_each_entry(ltm_entry, &cfm_config->cfm_recv_ltm_list, node) {
				if (ltm_entry->xid == ntohl(pdu_ltr->xid)) {
					memcpy(cfm_pktinfo->dst_mac_ptr, ltm_entry->ltm_mac, IFHWADDRLEN);
					memcpy(pktinfo->dst_mac, ltm_entry->ltm_mac, IFHWADDRLEN);
					break;
				}
			}
		}
		p += sizeof(cfm_pdu_ltr_t);
		//ltr_egress_id_tlv = (cfm_tlv_ltr_egress_id_t *) p;
#if 0 // Do not modify LTR payload according to ALU request
		ltr_egress_id_tlv->next_egress_id = htons((cfm_config->egress_id[6] << 8) | cfm_config->egress_id[7]);
		memcpy(ltr_egress_id_tlv->last_egress_mac, cfm_pktinfo->src_mac_ptr, IFHWADDRLEN);
		memcpy(ltr_egress_id_tlv->next_egress_mac, cfm_config->macaddr, IFHWADDRLEN);
		memcpy(cfm_pktinfo->src_mac_ptr, cfm_config->macaddr, IFHWADDRLEN);
		memcpy(pktinfo->src_mac, cfm_config->macaddr, IFHWADDRLEN);
		//memcpy(cfm_pktinfo->dst_mac_ptr, ltr_egress_id_tlv->last_egress_mac, IFHWADDRLEN);
		cfm_pktinfo->cfm_hdr->flags |= 0x40; // FwdYes
#endif
	}

	pktinfo->frame_ptr = cfm_pktinfo->frame_ptr;
	//pktinfo->rx_desc = cfm_pktinfo->rx_desc;
	//pktinfo->tx_desc = cfm_pktinfo->tx_desc;
	pktinfo->frame_len = cfm_pktinfo->frame_len;
	pktinfo->frame_len = (pktinfo->frame_len < MIN_ETH_FRAME_SIZE) ? MIN_ETH_FRAME_SIZE : pktinfo->frame_len;

	if(cfm_send_frame(cfm_config, pktinfo, CFM_SEND_FORWARD) == 0) {
		cfm_config->cfm_pkt_pm[(cfm_pdu <= CFM_PDU_MAX)?cfm_pdu:CFM_PDU_INVALID].fwd++;
		if(pktinfo->rx_desc.logical_port_id == switch_get_wan_logical_portid()) // WAN -> UNI
			cfm_config->cfm_pkt_pm[(cfm_pdu <= CFM_PDU_MAX)?cfm_pdu:CFM_PDU_INVALID].fwd_uni++;
		else
			cfm_config->cfm_pkt_pm[(cfm_pdu <= CFM_PDU_MAX)?cfm_pdu:CFM_PDU_INVALID].fwd_ani++;
		return 0;
	} else {
		cfm_config->cfm_pkt_pm[(cfm_pdu <= CFM_PDU_MAX)?cfm_pdu:CFM_PDU_INVALID].drop++;
		return -1;
	}
}
