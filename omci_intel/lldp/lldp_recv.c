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
 * Module  : lldp
 * File    : lldp_recv.c
 *
 ******************************************************************/

#include <stdio.h>
#include <string.h>
#include "util.h"
#include "switch.h"
#include "cpuport_frame.h"
#include "lldp_core.h"

int
lldp_pkt_recv_is_lldp(struct cpuport_info_t *pktinfo)
{
	if (pktinfo->rx_desc.router_mode)
		return CPUPORT_UNMATCH;
	if ( ntohs( *(unsigned short *)( pktinfo->frame_ptr+IFHWADDRLEN*2 ) ) == LLDP_ETHERTYPE ) 
		return CPUPORT_MATCH;
	else if ( ntohs( *(unsigned short *)( pktinfo->frame_ptr+IFHWADDRLEN*2+VLAN_TAG_OFFSET ) ) == LLDP_ETHERTYPE ) 
		return CPUPORT_MATCH;
	else if ( ntohs( *(unsigned short *)( pktinfo->frame_ptr+IFHWADDRLEN*2+VLAN_TAG_OFFSET*2 ) ) == LLDP_ETHERTYPE ) 
		return CPUPORT_MATCH;
	return CPUPORT_UNMATCH;
}

int
lldp_decode(lldp_pktinfo_t * lldp_pktinfo)
{
	extern lldp_pkt_t neighbor[];

	int length, gotend = 0;
	int tlv_size, tlv_type, offset = 0;
	int subtype;
	lldp_pkt_t* np;
	unsigned char *c = lldp_pktinfo->frame_ptr + IFHWADDRLEN*2 + ETHER_TYPE_LEN + lldp_pktinfo->vlan_num * 4;
	unsigned char oui[3];

	if (lldp_pktinfo->rx_desc.logical_port_id > SWITCH_LOGICAL_PORT_TOTAL)
		return -1;
	np = &neighbor[lldp_pktinfo->rx_desc.logical_port_id];

	memset(np, 0, sizeof(lldp_pkt_t));

	memcpy(np->src_mac, lldp_pktinfo->frame_ptr + IFHWADDRLEN, IFHWADDRLEN);

	length = lldp_pktinfo->frame_len - (IFHWADDRLEN*2 + ETHER_TYPE_LEN + lldp_pktinfo->vlan_num * 4);
	while ((offset < length) && (!gotend)) {
		tlv_size = ntohs( *(unsigned short *)(c + offset));
		tlv_type = tlv_size >> 9;
		tlv_size = tlv_size & 0x1ff;

		//dbprintf_lldp(LOG_ERR, "%02x %02x %d %d %d\n", *(c + offset), *(c + offset + 1), tlv_type, tlv_size, length);
		switch (tlv_type) {
		case LLDP_TLV_TYPE_END:
			gotend = 1;
			break;
		case LLDP_TLV_TYPE_CHASSIS_ID:
			np->chassis_id.subtype = (unsigned char)*(c + offset + 2);
			memcpy(np->chassis_id.chassis_id, (c + offset + 3),IFHWADDRLEN);
			break;
		case LLDP_TLV_TYPE_PORT_ID:
			np->port_id.subtype = (unsigned char)*(c + offset + 2);
			memcpy(np->port_id.port_id, (c + offset + 3),IFHWADDRLEN);
			break;
		case LLDP_TLV_TYPE_TIME_TO_LIVE:
			np->ttl.ttl = ntohs( *(unsigned short *)(c + offset + 2));
			break;
		case LLDP_TLV_TYPE_PORT_DESC:
			memcpy(np->port_desc.desc, (c + offset + 2), tlv_size);
			break;
		case LLDP_TLV_TYPE_SYSTEM_NAME:
			memcpy(np->system_name.name, (c + offset + 2), tlv_size);
			break;
		case LLDP_TLV_TYPE_SYSTEM_DESC:
			memcpy(np->system_desc.desc, (c + offset + 2), tlv_size);
			break;
		case LLDP_TLV_TYPE_SYSTEM_CAP:
			np->system_cap.cap = ntohs( *(unsigned short *)(c + offset + 2));;
			np->system_cap.en_cap = ntohs( *(unsigned short *)(c + offset + 4));;
			break;
		case LLDP_TLV_TYPE_MANAGE_ADDR:
			subtype = (unsigned char)*(c + offset + 3);
			if (subtype == 1) {
				/* Ipv4 */
				np->manage_addr_ipv4.length = (unsigned char)*(c + offset + 2);
				np->manage_addr_ipv4.subtype = 1;
				np->manage_addr_ipv4.addr = ntohl( *(unsigned int *)(c + offset + 4));
				np->manage_addr_ipv4.itf_subtype = (unsigned char)*(c + offset + 2 + np->manage_addr_ipv4.length);
				np->manage_addr_ipv4.itf_number = (unsigned char)*(c + offset + 2 + np->manage_addr_ipv4.length + 1);
				np->manage_addr_ipv4.oid_str_length = (unsigned char)*(c + offset + 2 + np->manage_addr_ipv4.length + 2);

				/* No ODI 8.5.9.8 object identifier at our strcut */
				/* TODO : if oid_str_length > 0 */
			} else if (subtype == 2){
				/* Ipv6 */
				np->manage_addr_ipv6.length = (unsigned char)*(c + offset + 2);
				np->manage_addr_ipv6.subtype = 2;
				np->manage_addr_ipv6.addr[0] = ntohl( *(unsigned int *)(c + offset + 4));
				np->manage_addr_ipv6.addr[1] = ntohl( *(unsigned int *)(c + offset + 8));
				np->manage_addr_ipv6.addr[2] = ntohl( *(unsigned int *)(c + offset + 12));
				np->manage_addr_ipv6.addr[3] = ntohl( *(unsigned int *)(c + offset + 16));

				np->manage_addr_ipv6.itf_subtype = (unsigned char)*(c + offset + 2 + np->manage_addr_ipv6.length);
				np->manage_addr_ipv6.itf_number = (unsigned char)*(c + offset + 2 + np->manage_addr_ipv6.length + 1);
				np->manage_addr_ipv6.oid_str_length = (unsigned char)*(c + offset + 2 + np->manage_addr_ipv6.length + 2);
			}
			break;
			case LLDP_TLV_TYPE_CUSTOM:
			oui[0] = (unsigned char)*(c + offset + 2);
			oui[1] = (unsigned char)*(c + offset + 3);
			oui[2] = (unsigned char)*(c + offset + 4);
			subtype = (unsigned char)*(c + offset + 5);
			if (oui[0] == 0 && oui[1] == 0x12 && oui[2] == 0xBB && subtype == 1) {
				memcpy(np->media_cap.oui, oui, sizeof(oui));
				np->media_cap.subtype = subtype;
				np->media_cap.cap = ntohs( *(unsigned short *)(c + offset + 6));
				np->media_cap.type = (unsigned char)*(c + offset + 8);
			}

			break;

		default:
			break;
		}

		offset += (2 + tlv_size);
	}

	return 0;
}

int
lldp_pkt_parse_pktinfo(struct cpuport_info_t *pktinfo, lldp_pktinfo_t * lldp_pktinfo)
{
	if ( pktinfo )
	{
		lldp_pktinfo->frame_ptr = pktinfo->frame_ptr;
		lldp_pktinfo->frame_len = pktinfo->frame_len;
		lldp_pktinfo->rx_desc = pktinfo->rx_desc;
		lldp_pktinfo->tx_desc = pktinfo->tx_desc;
	}
	
	if ( ntohs( *(unsigned short *)( lldp_pktinfo->frame_ptr+IFHWADDRLEN*2 ) ) == LLDP_ETHERTYPE ) 
		lldp_pktinfo->vlan_num = 0;
	else if ( ntohs( *(unsigned short *)( lldp_pktinfo->frame_ptr+IFHWADDRLEN*2+VLAN_TAG_OFFSET ) ) == LLDP_ETHERTYPE ) 
		lldp_pktinfo->vlan_num = 1;
	else if ( ntohs( *(unsigned short *)( lldp_pktinfo->frame_ptr+IFHWADDRLEN*2+VLAN_TAG_OFFSET*2 ) ) == LLDP_ETHERTYPE ) 
		lldp_pktinfo->vlan_num = 2;
	else
		return -1;

	lldp_pktinfo->dst_mac_ptr = lldp_pktinfo->frame_ptr;
	lldp_pktinfo->src_mac_ptr = lldp_pktinfo->frame_ptr + IFHWADDRLEN;

	switch (lldp_pktinfo->vlan_num) 
	{
		case 0:
			lldp_pktinfo->in_tci = NULL;
			lldp_pktinfo->out_tci = NULL;
			break;
		case 1:
			lldp_pktinfo->in_tci = lldp_pktinfo->frame_ptr + IFHWADDRLEN*2 + 2;
			lldp_pktinfo->out_tci = NULL;
			break;
		case 2:
			lldp_pktinfo->in_tci = lldp_pktinfo->frame_ptr + IFHWADDRLEN*2 + 2;
			lldp_pktinfo->out_tci = lldp_pktinfo->frame_ptr + IFHWADDRLEN*2 + VLAN_TAG_OFFSET + 2;
			break;
		default:
			lldp_pktinfo->in_tci = NULL;
			lldp_pktinfo->out_tci = NULL;
			break;
	}
	return 0;
}

int 
lldp_recv_pkt_process (lldp_pktinfo_t *lldp_pktinfo)
{
	extern lldp_parm_t lldp_parm[];
	int i, len=0;
	char buf[BUF_SIZE];

	if (lldp_pktinfo->rx_desc.logical_port_id > SWITCH_LOGICAL_PORT_TOTAL) {
		dbprintf_lldp(LOG_ERR, "lldp_recv_pkt_process error : logical_port_id=%d > %d\n", lldp_pktinfo->rx_desc.logical_port_id, SWITCH_LOGICAL_PORT_TOTAL);
		return -1;
	}

	if (lldp_decode(lldp_pktinfo) != 0) {
		dbprintf_lldp(LOG_ERR, "lldp decode fail logical_port_id=%d\n", lldp_pktinfo->rx_desc.logical_port_id);
		return -1;
	}

	lldp_parm[lldp_pktinfo->rx_desc.logical_port_id].pkt_c.rx++;
	lldp_parm[lldp_pktinfo->rx_desc.logical_port_id].sm.rcvFrame = TRUE;
	dbprintf_lldp(LOG_NOTICE, "lldp recv from %02x:%02x:%02x:%02x:%02x:%02x, logical_port_id=%d frame_len %d, \n", 
		lldp_pktinfo->src_mac_ptr[0], lldp_pktinfo->src_mac_ptr[1], lldp_pktinfo->src_mac_ptr[2], 
		lldp_pktinfo->src_mac_ptr[3], lldp_pktinfo->src_mac_ptr[4], lldp_pktinfo->src_mac_ptr[5], 
		lldp_pktinfo->rx_desc.logical_port_id, lldp_pktinfo->frame_len);
	if(omci_env_g.debug_level_lldp >= LOG_INFO) {
		for (i=0;i<(lldp_pktinfo->frame_len);i++) {
			if (i % 32 == 0) {
				len += snprintf(buf + len, BUF_SIZE - len, "\n");
			}
			len += snprintf(buf + len, BUF_SIZE - len, "%02x ", *(lldp_pktinfo->frame_ptr + i));
			if (len + 2 > BUF_SIZE) {
				dbprintf_lldp(LOG_ERR, "lldp decode buf too small\n");
				break;
			}

		}
		len += snprintf(buf + len, BUF_SIZE - len, "\n");
		dbprintf_lldp(LOG_INFO, "%s", buf);
	}


	return 0;
}
