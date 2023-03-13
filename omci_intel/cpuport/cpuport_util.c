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
 * Module  : cpuport
 * File    : cpuport_util.c
 *
 ******************************************************************/

#include <sys/time.h>
#include <errno.h>
#include <sys/types.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>

#include "util.h"
#include "list.h"
#include "env.h"
#include "meinfo.h"
#include "hwresource.h"
#include "switch.h"
#include "cpuport.h"
#include "cpuport_util.h"
#include "cpuport_sniffer.h"

char *
cpuport_util_ip_proto_str(unsigned char proto)
{
	switch (proto) {
		case IPPROTO_ICMP: return("icmp");
		case IPPROTO_IGMP: return("igmp");
		case IPPROTO_IPIP: return("ip tunnel");
		case IPPROTO_TCP: return("tcp");
		case IPPROTO_EGP: return("egp");
		case IPPROTO_UDP: return("udp");
		case IPPROTO_RSVP: return("rsvp");
		case IPPROTO_GRE: return("cisco gre tunnel");
		case IPPROTO_IPV6: return("ipv6 over ipv4");
		case IPPROTO_ESP: return("esp");
		case IPPROTO_AH: return("ah");
		case IPPROTO_PIM: return("pim");
		case IPPROTO_RAW: return("raw");
	}
	return("");
}

int
cpuport_util_rx_port_type_is_wan(struct cpuport_info_t *pktinfo)
{
	if( pktinfo->rx_desc.logical_port_id == switch_get_wan_logical_portid())
		return 1;
	return 0;
}

// tci related  ////////////////////////////////////////////////

int
cpuport_util_get_pbit_from_tci(struct cpuport_tci_t *tci)
{
	if (tci->otci.tpid == 0 && tci->itci.tpid == 0)
		return 0;
	else if (tci->otci.tpid == 0)
		return (tci->itci.priority<=7) ? tci->itci.priority : 0; 
	else
		return (tci->otci.priority<=7) ? tci->otci.priority : 0;
	return 0;
}

int
cpuport_util_get_tagnum_from_tci(struct cpuport_tci_t *tci)
{
	if (tci == NULL)
		return 0;
	if ( (tci->otci.tpid | tci->otci.priority | tci->otci.de | tci->otci.vid ) == 0) {
		if ( (tci->itci.tpid | tci->itci.priority | tci->itci.de | tci->itci.vid ) == 0)
			return 0;
		else
			return 1;
	} else {
		return 2;
	}
}

static inline unsigned short
tci_value (struct tci_fileds_t *tci)
{
	return (tci->priority << 13 ) | (tci->de << 12) | tci->vid;
}

int
cpuport_util_remove_tag( struct cpuport_info_t *cpuport_info)
{
	unsigned char tag_num = cpuport_util_get_tagnum_from_tci(&cpuport_info->tci);
	struct cpuport_pkthdr_t *src, *dst;
	int i = 0;

	if (tag_num > 0)
	{
		src = (void*)cpuport_info->frame_ptr;
		cpuport_info->frame_ptr += CPUPORT_VTAG_LEN * tag_num;
		cpuport_info->frame_len -= CPUPORT_VTAG_LEN * tag_num;
		dst = (void*)cpuport_info->frame_ptr;

		// mac header is shift right, per byte one by one to avoid overlap issue
		for ( i = 11 ; i >= 0 ; i-- )
		{
			*(dst->dmac+i) = *(src->dmac+i);
		}
	}
	return 0;
}

int
cpuport_util_replace_tag( struct cpuport_info_t *cpuport_info, struct cpuport_tci_t *dst_tci )
{
	unsigned char src_tag_num = cpuport_util_get_tagnum_from_tci(&cpuport_info->tci);
	unsigned char dst_tag_num = cpuport_util_get_tagnum_from_tci(dst_tci);
	struct cpuport_pkthdr_t *src, *dst;
	int i = 0;
	src = (void*)cpuport_info->frame_ptr;
	cpuport_info->frame_ptr -= CPUPORT_VTAG_LEN * (dst_tag_num-src_tag_num);
	cpuport_info->frame_len += CPUPORT_VTAG_LEN * (dst_tag_num-src_tag_num);
	dst = (void*)cpuport_info->frame_ptr;

	if (dst_tag_num > src_tag_num) {	// mac header is shift left, per byte one by one to avoid overlap issue
		for ( i = 0 ; i < 12 ; i++ )
			*(dst->dmac+i) = *(src->dmac+i);
	} else if (dst_tag_num < src_tag_num) { // mac header is shift right, per byte one by one to avoid overlap issue
		for ( i = 11 ; i >= 0 ; i-- )
			*(dst->dmac+i) = *(src->dmac+i);
	}

	if (dst_tag_num == 1) {
		dst->tag_u.one.tpid0 = htons(dst_tci->itci.tpid);
		dst->tag_u.one.tci0  = htons(tci_value(&dst_tci->itci));
	} else if (dst_tag_num == 2) {
		dst->tag_u.two.tpid0 = htons(dst_tci->otci.tpid);
		dst->tag_u.two.tci0  = htons(tci_value(&dst_tci->otci));
		dst->tag_u.two.tpid1 = htons(dst_tci->itci.tpid);
		dst->tag_u.two.tci1  = htons(tci_value(&dst_tci->itci));
	} else if (dst_tag_num == 3) {	// wont happen
		dst->tag_u.three.tpid0 = htons(dst_tci->ext1_tci.tpid);
		dst->tag_u.three.tci0  = htons(tci_value(&dst_tci->ext1_tci));
		dst->tag_u.three.tpid1 = htons(dst_tci->otci.tpid);
		dst->tag_u.three.tci1  = htons(tci_value(&dst_tci->otci));
		dst->tag_u.three.tpid2 = htons(dst_tci->itci.tpid);
		dst->tag_u.three.tci2  = htons(tci_value(&dst_tci->itci));
	} else if (dst_tag_num == 4) {	// wont happen
		dst->tag_u.four.tpid0 = htons(dst_tci->ext2_tci.tpid);
		dst->tag_u.four.tci0  = htons(tci_value(&dst_tci->ext2_tci));
		dst->tag_u.four.tpid1 = htons(dst_tci->ext1_tci.tpid);
		dst->tag_u.four.tci1  = htons(tci_value(&dst_tci->ext1_tci));
		dst->tag_u.four.tpid2 = htons(dst_tci->otci.tpid);
		dst->tag_u.four.tci2  = htons(tci_value(&dst_tci->otci));
		dst->tag_u.four.tpid3 = htons(dst_tci->itci.tpid);
		dst->tag_u.four.tci3  = htons(tci_value(&dst_tci->itci));
	}

	cpuport_info->tci = *dst_tci;
	return 0;
}

// bridge port me related  ////////////////////////////////////////////////

// why not iterate veip 329 directly?
// assume whole system only has one veip?
unsigned short 
cpuport_util_get_veip_meid(void)
{
	struct me_t *veip=NULL;
	unsigned short meid =0xffff;
	struct meinfo_t *miptr= meinfo_util_miptr(422);

	struct me_t *me = NULL;
	struct me_t *me_47;

	if (miptr==NULL) {
		dbprintf_cpuport(LOG_ERR, "class=422, null miptr?\n");
		return 0xffff;
	}

	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		if ((1 == meinfo_util_me_attr_data( me, 1)) && //occupied
			(0 == meinfo_util_me_attr_data( me, 4)) && //bridge port type == 0 (veip)
			((me_47 = meinfo_me_get_by_meid(47, meinfo_util_me_attr_data(me, 2))) != NULL) && //veip meid == bridge port meid
			(me_47->is_private == 0))
		{
			veip = me;
		}
	}
	meid = meinfo_util_me_attr_data(veip, 2);
	return meid;
}

unsigned short 
cpuport_util_get_iphost_meid(void)
{
	struct me_t *iphost=NULL;
	unsigned short meid =0xffff;
	struct meinfo_t *miptr= meinfo_util_miptr(422);

	struct me_t *me = NULL;

	if (miptr==NULL) {
		dbprintf_cpuport(LOG_ERR, "class=422, null miptr?\n");
		return 0xffff;
	}

	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		if ( 3 == meinfo_util_me_attr_data(me, 4) ) //bridge port type == 3 (iphost)
			iphost = me;
	}
	meid = meinfo_util_me_attr_data(iphost, 2);
	return meid;
}
struct me_t *
cpuport_util_get_bridge_port_me_by_logical_port_id ( unsigned char logical_port_id )
{
	struct meinfo_t *miptr= meinfo_util_miptr(422);
	struct me_t *ibport_me, *bport_me;

	if (miptr==NULL) {
		dbprintf_cpuport(LOG_ERR, "class=422, null miptr?\n");
		return NULL;
	}

	list_for_each_entry(ibport_me, &miptr->me_instance_list, instance_node) {
		if ( logical_port_id == meinfo_util_me_attr_data( ibport_me, 6) ) {
			// there might be multiple ibport whose attr6 point to same logical port id
			// we iterate all ibport to find the one which could be mapped back to public bport
			if ((bport_me = hwresource_private2public(ibport_me)) != NULL)
				return bport_me;
		}
	}
	return NULL;
}

char
cpuport_util_get_logical_port_id_by_bridge_port_me ( struct me_t *me )
{
	struct me_t *internal_bport_me = hwresource_public2private(me);
	return (internal_bport_me) ? meinfo_util_me_attr_data(internal_bport_me, 6) : -1;
}

// routine for ctc internet bridge /////////////////////////////////////////////////

/* return 1 means yes, ctc_internet_bridge is enabled, 0 not */
int
cpuport_util_is_ctc_internet_bridge_enable(struct cpuport_info_t *pktinfo, char *prompt)
{
	int ret=0;

	cpuport_sniffer_dbprint_pkt(LOG_INFO, "cpu_ctc_inetbr", pktinfo->frame_ptr, pktinfo->frame_len, pktinfo->rx_desc.router_mode);
	dbprintf_cpuport(LOG_INFO, "%s check rx logical_port_id = 0x%x\n", prompt, pktinfo->rx_desc.logical_port_id);
	dbprintf_cpuport(LOG_INFO, "omci_env_g.ctc_internet_bridge_enabled_srcmask=0x%x\n", omci_env_g.ctc_internet_bridge_enabled_srcmask);
	if (omci_env_g.ctc_internet_bridge_enabled_srcmask & (1 << pktinfo->rx_desc.logical_port_id)) {
		if (cpuport_util_get_tagnum_from_tci(&pktinfo->tci) == 0) {
			dbprintf_cpuport(LOG_NOTICE, "%s ctc internet bridge is enabled on logical port id %d.\n", 
				prompt, pktinfo->rx_desc.logical_port_id);
			ret = RET_YES;
		}
		else {
			dbprintf_cpuport(LOG_INFO, "%s ctc internet bridge is not enabled for untagging packet on logical port id %d.\n", 
				prompt, pktinfo->rx_desc.logical_port_id);
			ret = RET_NO;
		}
	}
	else {
		dbprintf_cpuport(LOG_DEBUG, "%s ctc internet bridge is not enabled on logical port id %d.\n", 
			prompt, pktinfo->rx_desc.logical_port_id);
		ret = RET_NO;
	}
	return ret;
}

int
cpuport_util_is_ctc_internet_bridge_lan2lan(struct cpuport_info_t *pktinfo, char *prompt)
{
	int ret=0, port_rx_enable=0, port_tx_enable=0;

	cpuport_sniffer_dbprint_pkt(LOG_INFO, "cpu_ctc_lan2lan", pktinfo->frame_ptr, pktinfo->frame_len, pktinfo->rx_desc.router_mode);
	dbprintf_cpuport(LOG_INFO, "%s tx port=%d, rx port=%d\n", prompt, pktinfo->tx_desc.logical_port_id, pktinfo->rx_desc.logical_port_id);
	dbprintf_cpuport(LOG_INFO, "omci_env_g.ctc_internet_bridge_enabled_srcmask=0x%x\n", omci_env_g.ctc_internet_bridge_enabled_srcmask);
	if (omci_env_g.ctc_internet_bridge_enabled_srcmask == 0) {
		dbprintf_cpuport(LOG_DEBUG, "ctc internet bridge is not enabled.\n");
		return RET_NO;
	}

	port_tx_enable = (omci_env_g.ctc_internet_bridge_enabled_srcmask >> pktinfo->tx_desc.logical_port_id) & 1;
	port_rx_enable = (omci_env_g.ctc_internet_bridge_enabled_srcmask >> pktinfo->rx_desc.logical_port_id) & 1;
	dbprintf_cpuport(LOG_INFO, "port_tx_enable=%d port_rx_enable=%d\n", port_tx_enable, port_rx_enable);
	if (port_tx_enable != port_rx_enable) {
		dbprintf_cpuport(LOG_NOTICE, "%s ctc internet bridge is NOT same group for rx=%d tx=%d port, so deny traffic between.\n",
			 prompt, pktinfo->rx_desc.logical_port_id, pktinfo->tx_desc.logical_port_id);
		ret = RET_YES;
	}
	else {
		dbprintf_cpuport(LOG_NOTICE, "%s ctc internet bridge is same group for rx=%d tx=%d port.\n",
			 prompt, pktinfo->rx_desc.logical_port_id, pktinfo->tx_desc.logical_port_id);
		ret = RET_NO;
	}

	return ret;
}
