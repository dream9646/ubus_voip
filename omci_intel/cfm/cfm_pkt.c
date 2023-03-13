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
 * File    : cfm_pkt.c
 *
 ******************************************************************/
 
#include <arpa/inet.h>
#include <string.h>

#include "list.h"
#include "util.h"
#include "cpuport.h"
#include "cpuport_frame.h"
#include "cpuport_vtagging_vfilter.h"
#include "cfm_pkt.h"
#include "cfm_util.h"
#include "cfm_bport.h"

int
cfm_pkt_parse_pktinfo(struct cpuport_info_t *pktinfo, cfm_pktinfo_t * cfm_pktinfo)
{
	if ( pktinfo )
	{
		cfm_pktinfo->frame_ptr = pktinfo->frame_ptr;
		cfm_pktinfo->frame_len = pktinfo->frame_len;
		cfm_pktinfo->rx_desc = pktinfo->rx_desc;
		cfm_pktinfo->tx_desc = pktinfo->tx_desc;
	}
	if ( cfm_pktinfo->frame_ptr == NULL )
		return -1;
	else if ( ntohs( *(unsigned short *)( cfm_pktinfo->frame_ptr+IFHWADDRLEN*2 ) ) == CFM_ETHERTYPE ) 
		cfm_pktinfo->vlan_num = 0;
	else if ( ntohs( *(unsigned short *)( cfm_pktinfo->frame_ptr+IFHWADDRLEN*2+VLAN_TAG_OFFSET ) ) == CFM_ETHERTYPE ) 
		cfm_pktinfo->vlan_num = 1;
	else if ( ntohs( *(unsigned short *)( cfm_pktinfo->frame_ptr+IFHWADDRLEN*2+VLAN_TAG_OFFSET*2 ) ) == CFM_ETHERTYPE ) 
		cfm_pktinfo->vlan_num = 2;
	else
		return -1;

	cfm_pktinfo->dst_mac_ptr = cfm_pktinfo->frame_ptr;
	cfm_pktinfo->src_mac_ptr = cfm_pktinfo->frame_ptr + IFHWADDRLEN;

	switch (cfm_pktinfo->vlan_num) 
	{
		case 0:
			cfm_pktinfo->in_tci = NULL;
			cfm_pktinfo->out_tci = NULL;
			break;
		case 1:
			cfm_pktinfo->in_tci = cfm_pktinfo->frame_ptr + IFHWADDRLEN*2 + 2;
			cfm_pktinfo->out_tci = NULL;
			break;
		case 2:
			cfm_pktinfo->in_tci = cfm_pktinfo->frame_ptr + IFHWADDRLEN*2 + 2;
			cfm_pktinfo->out_tci = cfm_pktinfo->frame_ptr + IFHWADDRLEN*2 + VLAN_TAG_OFFSET + 2;
			break;
		default:
			cfm_pktinfo->in_tci = NULL;
			cfm_pktinfo->out_tci = NULL;
			break;
	}
	cfm_pktinfo->cfm_hdr = (cfm_hdr_t *) (cfm_pktinfo->frame_ptr+IFHWADDRLEN*2+ETHER_TYPE_LEN+cfm_pktinfo->vlan_num*4);
	return 0;
}

int
cfm_pkt_type(cfm_pktinfo_t * cfm_pktinfo)
{	
	if (!cfm_pktinfo->cfm_hdr)
		return CFM_PDU_INVALID;
		
	// Check multicast DA and MD_level consistency
	if (cfm_pktinfo->dst_mac_ptr[0] & 1) {
		unsigned char *m=cfm_pktinfo->dst_mac_ptr;
		// only accept mcast addr of 01:80:c2:00:00:3y 
		if (m[0]!=0x01 || m[1]!=0x80 || m[2]!=0xc2 || m[3]!=0x0 || m[4]!=0x0)
			return CFM_PDU_INVALID;			
		switch (cfm_pktinfo->cfm_hdr->opcode) {
		case CFM_PDU_CCM:
			if (m[5] != (0x30 | cfm_pktinfo->cfm_hdr->md_level))
				return CFM_PDU_INVALID;
			break;
		case CFM_PDU_LTM:
			if (m[5] != (0x38 | cfm_pktinfo->cfm_hdr->md_level))
				return CFM_PDU_INVALID;
			break;
		default:
			if (m[5] != (0x30 | cfm_pktinfo->cfm_hdr->md_level) &&
			    m[5] != (0x38 | cfm_pktinfo->cfm_hdr->md_level))
				return CFM_PDU_INVALID;
		}
	}

	return cfm_pktinfo->cfm_hdr->opcode;
}

int
cfm_pkt_check_mac(cfm_config_t *cfm_config, cfm_pktinfo_t *cfm_pktinfo)
{
	unsigned char mcast_mac[IFHWADDRLEN] = {0x01, 0x80, 0xC2, 0x00, 0x00, 0x30};
	unsigned char mcast_mac2[IFHWADDRLEN] = {0x01, 0x80, 0xC2, 0x00, 0x00, 0x38};

	mcast_mac[5] |= cfm_config->md_level;
	mcast_mac2[5] |= cfm_config->md_level;
		
	if(memcmp(cfm_pktinfo->dst_mac_ptr, cfm_config->macaddr, IFHWADDRLEN) == 0) {
		return 1;
	} else if(memcmp(cfm_pktinfo->dst_mac_ptr, mcast_mac, IFHWADDRLEN) == 0) {
		return 1;
	} else if(memcmp(cfm_pktinfo->dst_mac_ptr, mcast_mac2, IFHWADDRLEN) == 0) {
		if(cfm_pkt_type(cfm_pktinfo) == CFM_PDU_LTM) {
			unsigned char *p = (unsigned char *) cfm_pktinfo->cfm_hdr + sizeof(cfm_hdr_t);
			cfm_pdu_ltm_t *pdu_ltm = (cfm_pdu_ltm_t *) p;
			if(memcmp(pdu_ltm->target_addr, cfm_config->macaddr, IFHWADDRLEN) == 0)
	    			return 1;
	    		else
	    			return 0;
		} else
			return 1;
	}
	return 0;
}

int
cfm_pkt_check_vlan(cfm_config_t *cfm_config, cfm_pktinfo_t *cfm_pktinfo)
{
	unsigned short tci;
	struct cpuport_tci_t src_tci;
	struct me_t *bport_me = cfm_bport_find(cfm_config);
	
	memset(&src_tci, 0, sizeof(struct cpuport_tci_t));
	src_tci.ethertype = CFM_ETHERTYPE;
	
	switch(cfm_pktinfo->vlan_num) {
		case 0:
			if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX)==0) {
				// check  untag case if pri_vlan==0
				if(cfm_config->pri_vlan == 0) {
					int i;
					for(i=0; i<12; i++) {
						if(0 != cfm_config->assoc_vlans[i])
							return 0;
					}
					return 1;
				}
				break;
			}
			return 1;
		case 1:
			tci = htons(*(unsigned short *) cfm_pktinfo->in_tci);
			if((tci & 0xFFF) == cfm_config->pri_vlan) {
				if(cfm_config->pbit_filter)
					return ((((tci >> 13) & 0xFFF) == cfm_config->pri_pbit) ? 1 : 0);
				else
					return 1;
			} else {
				int i;
				for(i=0; i<12; i++) {
					if((tci & 0xFFF) == cfm_config->assoc_vlans[i])
						return 1;
				}
			}
			src_tci.itci.tpid = ntohs(*(unsigned short *)( cfm_pktinfo->frame_ptr+IFHWADDRLEN*2));
			src_tci.itci.priority = (tci >> 13) & 0x7;
			src_tci.itci.de = (tci >> 12) & 0x1;
			src_tci.itci.vid = tci & 0xFFF;
			break;
		case 2:
			tci = htons(*(unsigned short *) cfm_pktinfo->out_tci);
			if((tci & 0xFFF) == cfm_config->pri_vlan) {
				if(cfm_config->pbit_filter)
					return ((((tci >> 13) & 0xFFF) == cfm_config->pri_pbit) ? 1 : 0);
				else
					return 1;
			} else {
				int i;
				for(i=0; i<12; i++) {
					if((tci & 0xFFF) == cfm_config->assoc_vlans[i])
						return 1;
				}
			}
			src_tci.otci.tpid = ntohs(*(unsigned short *)( cfm_pktinfo->frame_ptr+IFHWADDRLEN*2));
			src_tci.otci.priority = (tci >> 13) & 0x7;
			src_tci.otci.de = (tci >> 12) & 0x1;
			src_tci.otci.vid = tci & 0xFFF;
			tci = htons(*(unsigned short *) cfm_pktinfo->in_tci);
			src_tci.itci.tpid = ntohs(*(unsigned short *)( cfm_pktinfo->frame_ptr+IFHWADDRLEN*2+VLAN_TAG_OFFSET));
			src_tci.itci.priority = (tci >> 13) & 0x7;
			src_tci.itci.de = (tci >> 12) & 0x1;
			src_tci.itci.vid = tci & 0xFFF;
			break;
		default:
			break;
	}
	if (cfm_util_get_mhf_creation(cfm_config) != MA_MHF_CREATION_NONE) { // MIP
		// for MIP, ingress VLAN may mismatch with MIP VLAN because of VLAN translation case
		return (cpuport_vfilter(bport_me, 1, &src_tci) == CPUPORT_VFILTER_PASS) ? 1 : 0;
	} else { // MEP
		return 0;
	}
}

int
cfm_pkt_recv_is_y1731(struct cpuport_info_t *pktinfo)
{
	int ret = CPUPORT_UNMATCH;
	cfm_pktinfo_t *cfm_pktinfo =(cfm_pktinfo_t *) malloc_safe(sizeof(cfm_pktinfo_t));
	if (cfm_pktinfo) {
		cfm_pkt_parse_pktinfo(pktinfo, cfm_pktinfo);
		cfm_pkt_type(cfm_pktinfo);
		if(cfm_pktinfo->cfm_hdr->opcode >= CFM_PDU_AIS)
			ret = CPUPORT_MATCH; // y.1731
		free_safe(cfm_pktinfo);
	}
	return ret;
}

int
cfm_pkt_recv_is_cfm(struct cpuport_info_t *pktinfo)
{
	if (pktinfo->rx_desc.router_mode || list_empty(&cfm_config_list))
		return CPUPORT_UNMATCH;
	if ( ntohs( *(unsigned short *)( pktinfo->frame_ptr+IFHWADDRLEN*2 ) ) == CFM_ETHERTYPE ) 
		return (!omci_env_g.cfm_y1731_enable && cfm_pkt_recv_is_y1731(pktinfo)==CPUPORT_MATCH) ? CPUPORT_UNMATCH : CPUPORT_MATCH;
	else if ( ntohs( *(unsigned short *)( pktinfo->frame_ptr+IFHWADDRLEN*2+VLAN_TAG_OFFSET ) ) == CFM_ETHERTYPE ) 
		return (!omci_env_g.cfm_y1731_enable && cfm_pkt_recv_is_y1731(pktinfo)==CPUPORT_MATCH) ? CPUPORT_UNMATCH : CPUPORT_MATCH;
	else if ( ntohs( *(unsigned short *)( pktinfo->frame_ptr+IFHWADDRLEN*2+VLAN_TAG_OFFSET*2 ) ) == CFM_ETHERTYPE ) 
		return (!omci_env_g.cfm_y1731_enable && cfm_pkt_recv_is_y1731(pktinfo)==CPUPORT_MATCH) ? CPUPORT_UNMATCH : CPUPORT_MATCH;
	return CPUPORT_UNMATCH;
}
