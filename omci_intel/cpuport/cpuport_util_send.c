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
 * File    : cpuport_util_send.c
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
#include "omciutil_vlan.h"
#include "batchtab_cb.h"
#include "batchtab.h"
#include "hwresource.h"
#include "switch.h"
#include "classf.h"
#include "cpuport.h"
#include "cpuport_frame.h"
#include "cpuport_util.h"
#include "cpuport_vtagging_vfilter.h"
#include "cpuport_ipv6.h"

// one(lan) -> one(wan) with vtagging/vfilter///////////////////////////////////////////////////////////////////////////

void
dbprintf_cpuport_send_wan_handle_tag(struct cpuport_info_t *pktinfo, struct me_t *equiv_ingress_bport_me, char *prompt, char *msg)
{
	if (omci_env_g.debug_level_cpuport >= LOG_NOTICE) {
		char *src_name, *src2_name, *dst_name;	
		src_name = switch_get_logical_portname(pktinfo->rx_desc.logical_port_id);
		src2_name = switch_get_logical_portname(cpuport_util_get_logical_port_id_by_bridge_port_me(equiv_ingress_bport_me));
		dst_name = switch_get_logical_portname(pktinfo->tx_desc.logical_port_id);
		if (msg && strlen(msg)>0) {
			dbprintf_cpuport(LOG_NOTICE, "%s, %s->%s, bport 0x%x(%s)->0x%x(%s), %s\n",
				prompt, src_name, dst_name, equiv_ingress_bport_me->meid, src2_name, pktinfo->tx_desc.bridge_port_me->meid, dst_name, msg);
		} else {
			dbprintf_cpuport(LOG_NOTICE, "%s, %s->%s, bport 0x%x(%s)->0x%x(%s)\n",
				prompt, src_name, dst_name, equiv_ingress_bport_me->meid, src2_name, pktinfo->tx_desc.bridge_port_me->meid, dst_name);
		}
	}
}

// pkt from uni to wan :  protocol-vlan vtagging -> omci uni vtagging -> omci uni filter -> omci wan filter 
// only pkt matched pbit_filter_mask would be sent
// return value: 1=pass, 0=drop, -1=error
static int
cpuport_util_send_wan_handle_tag(unsigned char pbit_filter_mask, struct cpuport_info_t *pktinfo, char *prompt, int (send_deny_check)(struct cpuport_info_t *, char *))
{
	struct me_t *uni0_port_me = cpuport_util_get_bridge_port_me_by_logical_port_id(0);
	struct me_t *veip_bridge_port = NULL, *ingress_port_me = NULL;
	struct cpuport_tci_t dst_tci;
	unsigned short veip_meid = cpuport_util_get_veip_meid();
	int pbit;

	if ((send_deny_check != NULL) &&
	    (RET_YES == send_deny_check(pktinfo, prompt))) {
		dbprintf_cpuport(LOG_NOTICE, "send deny for %s\n", prompt);
		return -1;
	}

	if ( veip_meid != 0xffff )
		veip_bridge_port = meinfo_me_get_by_meid(47, veip_meid );
	
	// find equiv bport me if port2port_enable	
	ingress_port_me = pktinfo->rx_desc.bridge_port_me;
	if (omci_env_g.port2port_enable) { // refer uni0 first, then veip
		if(uni0_port_me)
			ingress_port_me = uni0_port_me;
		else if(veip_bridge_port)
			ingress_port_me = veip_bridge_port;
	}	
	// for wifi port, apply veip's vlan translation
	if (omci_env_g.veip_for_wifi_enable) {
		if ( (pktinfo->rx_desc.logical_port_id == switch_get_wifi_logical_portid(0) || 
		      pktinfo->rx_desc.logical_port_id == switch_get_wifi_logical_portid(1)) && 
		      (pktinfo->rx_desc.bridge_port_me && pktinfo->rx_desc.bridge_port_me->is_private) ) {
		      	if(veip_bridge_port)
				ingress_port_me = veip_bridge_port;
		}
	}
	if(!ingress_port_me)
		return -1;
	// if there is no rx bport me, use uni0 or veip instead to eliminate possible null pointer crash issue
	if(!pktinfo->rx_desc.bridge_port_me)
		pktinfo->rx_desc.bridge_port_me = ingress_port_me;
		
	if (cpuport_vtagging_vfilter(ingress_port_me, pktinfo->tx_desc.bridge_port_me, &(pktinfo->tci), &dst_tci, pktinfo) == CPUPORT_VTAGGING_VFILTER_BLOCK ) {
		dbprintf_cpuport_send_wan_handle_tag(pktinfo, ingress_port_me, prompt, "vtagging & vfilter DROP!");
		return 0;
	}
	
	// prevent mismatch pbit_filter pkt passes to invalid dst bport
	pbit=cpuport_util_get_pbit_from_tci(&dst_tci);
	if (((1<<pbit) & pbit_filter_mask) == 0) {
		static char line[64];
		snprintf(line, 64, "pkt p%d not covered by wanflow pbit_mask 0x%x, DROP!", pbit, pbit_filter_mask);
		dbprintf_cpuport_send_wan_handle_tag(pktinfo, ingress_port_me, prompt, line);
		return 0;
	}
	
	if (cpuport_util_replace_tag( pktinfo, &dst_tci) != RET_OK) {
		dbprintf_cpuport_send_wan_handle_tag(pktinfo, ingress_port_me, prompt, "cpuport_util_replace_tag() ERR!");
		return -1;
	}

	dbprintf_cpuport_send_wan_handle_tag(pktinfo, ingress_port_me, prompt, "");
	cpuport_frame_send(pktinfo);
	return 1;
}

// one(wan) -> one(lan) with vtagging/vfilter ///////////////////////////////////////////////////////////////////////////

void
dbprintf_cpuport_send_lan_handle_tag(struct cpuport_info_t *pktinfo, struct me_t *equiv_egress_bport_me, char *prompt, char *msg)
{
	if (omci_env_g.debug_level_cpuport >= LOG_NOTICE) {
		char *src_name, *dst_name, *dst2_name;
		src_name = switch_get_logical_portname(pktinfo->rx_desc.logical_port_id);
		dst_name = switch_get_logical_portname(pktinfo->tx_desc.logical_port_id);
		dst2_name = switch_get_logical_portname(cpuport_util_get_logical_port_id_by_bridge_port_me(equiv_egress_bport_me));
		if (msg && strlen(msg)>0) {
			dbprintf_cpuport(LOG_NOTICE, "%s, %s->%s, bport 0x%x(%s)->0x%x(%s), %s\n",
				prompt, src_name, dst_name, pktinfo->rx_desc.bridge_port_me->meid, src_name, equiv_egress_bport_me->meid, dst2_name, msg);
		} else {
			dbprintf_cpuport(LOG_NOTICE, "%s, %s->%s, bport 0x%x(%s)->0x%x(%s)\n",
				prompt, src_name, dst_name, pktinfo->rx_desc.bridge_port_me->meid, src_name, equiv_egress_bport_me->meid, dst2_name);
		}
	}
}

// pkt from wan to uni :  omci wan filter -> omci uni filter -> protocol-vlan vtagging -> omci uni vtagging

// return value: 1=pass, 0=drop, -1=error
static int
cpuport_util_send_lan_handle_tag(struct cpuport_info_t *pktinfo, char *prompt, int (send_deny_check)(struct cpuport_info_t *, char *))
{
	struct me_t *uni0_port_me = cpuport_util_get_bridge_port_me_by_logical_port_id(0);
	struct me_t *veip_bridge_port = NULL, *egress_port_me = NULL;
	struct cpuport_tci_t dst_tci;
	unsigned short veip_meid = cpuport_util_get_veip_meid();
	
	if ((send_deny_check != NULL) &&
	    (RET_YES == send_deny_check(pktinfo, prompt))) {
		dbprintf_cpuport(LOG_NOTICE, "send deny for %s\n", prompt);
		return -1;
	}
	
	if ( omci_env_g.cpuport_ds_passthrough && (cpuport_util_get_tagnum_from_tci(&pktinfo->tci) ==0)) {
		dbprintf_cpuport_send_lan_handle_tag(pktinfo, egress_port_me, prompt, "send");
		cpuport_frame_send(pktinfo);
		return 1;
	}
	
	if ( veip_meid != 0xffff )
		veip_bridge_port = meinfo_me_get_by_meid(47, veip_meid );
	
	egress_port_me = pktinfo->tx_desc.bridge_port_me;
	if (omci_env_g.port2port_enable) { // refer uni0 first, then veip
		if(uni0_port_me)
			egress_port_me = uni0_port_me;
		else if (veip_bridge_port)
			egress_port_me = veip_bridge_port;
	}
	// for wifi port, apply veip's vlan translation
	if (omci_env_g.veip_for_wifi_enable) {
		if ( (pktinfo->tx_desc.logical_port_id == switch_get_wifi_logical_portid(0) || 
		      pktinfo->tx_desc.logical_port_id == switch_get_wifi_logical_portid(1)) && 
		      (pktinfo->tx_desc.bridge_port_me && pktinfo->tx_desc.bridge_port_me->is_private) ) {
		      	if (veip_bridge_port)
				egress_port_me = veip_bridge_port;
		}
	}
	if(!egress_port_me) {
		dbprintf_cpuport(LOG_NOTICE, "tx logical port id %d maps to NULL bridge port me?\n", pktinfo->tx_desc.logical_port_id);
		return -1;
	}
	
	//pass through wan port filter
	int rtn = cpuport_vtagging_vfilter(pktinfo->rx_desc.bridge_port_me, egress_port_me , &(pktinfo->tci), &dst_tci, pktinfo);		
	if (rtn == CPUPORT_VTAGGING_VFILTER_BLOCK) {
		dbprintf_cpuport_send_lan_handle_tag(pktinfo, egress_port_me, prompt, "vtagging & vfilter DROP!");
		return 0;
	} else if ( rtn == CPUPORT_VTAGGING_RULE_UNMATCH ) {
		if ( omci_env_g.classf_add_default_rules == 0 ){
			dbprintf_cpuport_send_lan_handle_tag(pktinfo, egress_port_me, prompt, "no rule found, default DROP!");
			return 0;
		} else {
			dbprintf_cpuport_send_lan_handle_tag(pktinfo, egress_port_me, prompt, "no rule found, default PASS!");
			memcpy(&dst_tci, &(pktinfo->tci),  sizeof(struct cpuport_tci_t));
		}
	}
	
	if (cpuport_util_replace_tag(pktinfo, &dst_tci) != RET_OK) {
		dbprintf_cpuport_send_lan_handle_tag(pktinfo, egress_port_me, prompt, "cpuport_util_replace_tag() ERR!");
		return -1;
	}

	dbprintf_cpuport_send_lan_handle_tag(pktinfo, egress_port_me, prompt, "");
	cpuport_frame_send(pktinfo);
	return 1;
}

// one(lan) to many(wan) with vtagging/vfilter ///////////////////////////////////////////////////////////////////////////

// return value: 1=pass, 0=drop, -1=error
int
cpuport_util_send_multi_wan(struct cpuport_info_t *pktinfo, char *prompt, 
  int (send_deny_check)(struct cpuport_info_t *, char *),
  int (lan_to_wan_cb)(unsigned char pbit_filter_mask, struct cpuport_info_t *pktinfo, char *prompt, int (send_deny_check)(struct cpuport_info_t *, char *)))
{
	static unsigned char cloned_pkt[2048];
	struct cpuport_info_t cloned_pktinfo;
	int total_send=0, total_err=0;
	int p, bridge_id, id, ret;
	struct batchtab_cb_wan_t *t = batchtab_table_data_get("wan");
	unsigned int da_portmask;

	if (t == NULL) {
		dbprintf_cpuport(LOG_NOTICE, "batch table wan is NULL\n");
		return -1;
	}

	bridge_id = meinfo_util_me_attr_data(pktinfo->rx_desc.bridge_port_me,1);
	//dbprintf_cpuport(LOG_DEBUG, "bridge_id=0x%x(%d)\n", bridge_id, bridge_id);

#if 0
	// skip routing only src uni, this might be not always true
	if ((1<<pktinfo->rx_desc.logical_port_id) & batchtab_cb_isolation_get_router_mode_uni_portmask()) {
		dbprintf_cpuport(LOG_NOTICE, "logical port %d bport(0x%x) is routing only, skip L2 forwarding\n", 
			pktinfo->rx_desc.logical_port_id, pktinfo->rx_desc.bridge_port_me->meid);
		batchtab_table_data_put("wan");	
		return 0;
	}
#endif
	// skip linkdown port
	pktinfo->tx_desc.logical_port_id = switch_get_wan_logical_portid();
	if (((1<<pktinfo->tx_desc.logical_port_id) & switch_get_linkup_logical_portmask()) == 0) {
		batchtab_table_data_put("wan");	
		return 0;
	}
	// skip if da found in mactab but not on the tx dest port
	da_portmask = switch_get_dmac_logical_portmask(pktinfo->dst_mac);
	if (da_portmask && ((1<<pktinfo->tx_desc.logical_port_id) & da_portmask) == 0) {
		batchtab_table_data_put("wan");
		dbprintf_cpuport(LOG_INFO, "mac[%.2x:%.2x:%.2x:%.2x:%.2x:%.2x] is not on wan ports, DROP!!\n",
			pktinfo->dst_mac[0],
			pktinfo->dst_mac[1],
			pktinfo->dst_mac[2],
			pktinfo->dst_mac[3],
			pktinfo->dst_mac[4],
			pktinfo->dst_mac[5]);
		return 0;
	}

	for ( p = 0; p < 8; p++) {
		struct batchtab_cb_wan_node_t *w;
		struct me_t *internal_bport_me;
		
		list_for_each_entry(w, &t->wan_list[p], node) {
			struct me_t *bport_me = meinfo_me_get_by_meid(47, w->bport_meid); 
		
			if (bport_me == NULL)
				continue;
			internal_bport_me = hwresource_public2private(bport_me);
			if ( internal_bport_me == NULL) //skip null wan port
				continue;
			if ( meinfo_util_me_attr_data(internal_bport_me, 4) != 2 ) //skip non wan port
				continue;
			if ( meinfo_util_me_attr_data(bport_me, 3) == 6 ) //skip multicast gem port
				continue;			
			if ( omci_env_g.port2port_enable == 0 ) {
				// check if the wan port of the flow i is on the same bridge as the uni
				id = meinfo_util_me_attr_data(bport_me, 1);
				if ( id != bridge_id ) {
					dbprintf_cpuport(LOG_DEBUG, "pkt enters bridge me 0x%x, flow %d is on bridge me 0x%x, flow skipped\n", bridge_id, w->stream_id, id);
					continue;
				}
			}			

			pktinfo->tx_desc.bridge_port_me = bport_me;

			// clone pktinfo for multiple send
			//memset(&cloned_pkt[pktinfo->frame_len], 0, sizeof(cloned_pkt)-pktinfo->frame_len);
			memcpy(&cloned_pkt, pktinfo->frame_ptr, pktinfo->frame_len);
			memcpy(&cloned_pktinfo, pktinfo, sizeof(struct cpuport_info_t));
			cloned_pktinfo.frame_ptr = cloned_pkt;
			// recalculate nh
			cloned_pktinfo.nh = pktinfo->nh ? (void *)cloned_pktinfo.frame_ptr + (pktinfo->nh - (void *)pktinfo->frame_ptr) : 0; //offset

			if (lan_to_wan_cb)
				ret = lan_to_wan_cb((1<<p), &cloned_pktinfo, prompt, send_deny_check);
			else
				ret = cpuport_util_send_wan_handle_tag((1<<p), &cloned_pktinfo, prompt, send_deny_check);
			if (ret >0)
				total_send++;
			else if (ret <0)
				total_err++;
		}
	}
	batchtab_table_data_put("wan");
	//util_dbprintf(omci_env_g.debug_level_cpuport, LOG_NOTICE, 0, "\n");
	if (total_send)
		return 1;
	if (total_err)
		return -1;
	return 0;
}

static int //0: not match, 1: match
cpuport_util_match_ani_tci_vid(struct cpuport_tci_t *src_tci, unsigned short ani_vid)
{
	unsigned char tag_num;

	if (src_tci == NULL)
	{
		dbprintf(LOG_ERR, "param error!!\n");
		return 0;
	}
	
	tag_num	 = cpuport_util_get_tagnum_from_tci(src_tci);

	switch (tag_num)
	{
		case 1:
			if (src_tci->itci.vid == ani_vid)
			{
				return 1;
			}
			break;
		case 2:
			if (src_tci->otci.vid == ani_vid)
			{
				return 1;
			}
			break;
		default:
			;
	}
	
	return 0;
}

static int //0: not brwan wifi, 1: brwan wifi
cpuport_util_is_brwan_wifi(struct cpuport_info_t *pktinfo)
{
	struct classf_info_t *t1;
	struct batchtab_cb_wanif_t *t2;
	unsigned short vid;
	int i;

	if (pktinfo == NULL)
	{
		dbprintf(LOG_ERR, "param error!!\n");
		return 0;
	}

	//check brwan_info for enabled and wifimask
	t1 = batchtab_table_data_get("classf");

	if (t1 == NULL ||
		t1->system_info.brwan_info.enable == 0 ||
		t1->system_info.brwan_info.wifimask == 0)
	{
		batchtab_table_data_put("classf");
		return 0;
	}

	vid = t1->system_info.brwan_info.vid;

	batchtab_table_data_put("classf");	

	//check ani_vid = pktinfo tci vid, then rg_vid = brwan_info vid
	if ((t2 = batchtab_table_data_get("wanif")) == NULL &&
		t2->veip_rg_tag_total == 0)
	{
		batchtab_table_data_put("wanif");
		return 0;
	}

	for (i = 0; i < t2->veip_rg_tag_total && i < RG2ANI_TAGLIST_TOTAL; i++)
	{
		if (cpuport_util_match_ani_tci_vid(&pktinfo->tci, (t2->veip_ani_tag_list[i] & 0xfff)) && //ani vid match
			(t2->veip_rg_tag_list[i] & 0xfff) == vid) //rg vid match
		{
			dbprintf_cpuport(LOG_NOTICE, "this is brwan wifi packet, ani_vid=%d, rg_vid=%d\n", (t2->veip_ani_tag_list[i] & 0xfff), (t2->veip_rg_tag_list[i] & 0xfff));
			batchtab_table_data_put("wanif");
			return 1;
		}
	}

	batchtab_table_data_put("wanif");

	return 0;
}

// one(wan) to many(lan) with vtagging/vfilter ///////////////////////////////////////////////////////////////////////////

// return value: 1=pass, 0=drop, -1=error
static int
_cpuport_util_send_multi_lan(struct cpuport_info_t *pktinfo, char *prompt, 	// for single wan to multiple lan
  int (send_deny_check)(struct cpuport_info_t *, char *),
  int (wan_to_lan_cb)(struct cpuport_info_t *pktinfo, char *prompt, int (send_deny_check)(struct cpuport_info_t *, char *)))
{
	static unsigned char cloned_pkt[2048];
	struct cpuport_info_t cloned_pktinfo;
	int total_send=0, total_err=0;
	int i, bridge_id, ret;
	int trunk_group_send = 0;
	unsigned int da_portmask;

	struct me_t *uni0_port_me = cpuport_util_get_bridge_port_me_by_logical_port_id(0);
	struct me_t *veip_bridge_port = meinfo_me_get_by_meid(47, cpuport_util_get_veip_meid());
	
	// drop wan-to-uni packet if no wan bport me
	if ( !pktinfo->rx_desc.bridge_port_me )
		return -1;

	bridge_id = meinfo_util_me_attr_data(pktinfo->rx_desc.bridge_port_me,1);
	da_portmask = switch_get_dmac_logical_portmask(pktinfo->dst_mac);

	for ( i=0; i< SWITCH_LOGICAL_PORT_TOTAL ; i++ ) {
		struct me_t *internal_bport_me;
		
		if (((1<<i) & switch_get_all_logical_portmask()) == 0)
			continue;
		
		// the iphost/veip should be just ignored
		// traffic to iphost/veip would be done in router path by ifsw, so there is no need to do them in software bridge path,
		// on the other hand, cpuport_util_send_lan_handle_tag() is unable to send pkt to linux veip/iphost wan interface
		if (i == switch_get_wan_logical_portid() ||	// ignore pon port
		    i == switch_get_cpu_logical_portid(0)) 	// ignore iphost/veip port
			continue;

#if 0
        	// skip routing only dst uni, this might be not always true
        	if ((1<<i) & batchtab_cb_isolation_get_router_mode_uni_portmask()) {
			dbprintf_cpuport(LOG_NOTICE, "logical port %d bport(0x%x) is routing only, skip L2 forwarding\n", 
				i, pktinfo->rx_desc.bridge_port_me->meid);
        	        continue;
		}
#endif		
		// skip linkdown port
		if (((1<<i) & switch_get_linkup_logical_portmask()) == 0)
			continue;
		// skip if da found in mactab but not on the tx dest port
		if (da_portmask && ((1<<i) & da_portmask) == 0)	
			continue;

		if (((1<<i) & switch_get_wifi_logical_portmask()) != 0)
		{
			unsigned char *value_ptr;
			
			//calix, skip broadcast, multicast, unknown unicast to WIFI
			//dmac bc
			if ((pktinfo->dst_mac[0] == 0xff &&
				pktinfo->dst_mac[1] == 0xff &&
				pktinfo->dst_mac[2] == 0xff &&
				pktinfo->dst_mac[3] == 0xff &&
				pktinfo->dst_mac[4] == 0xff &&
				pktinfo->dst_mac[5] == 0xff) &&
				(omci_env_g.cpuport_wifi_block_mask & ENV_CPUPORT_WIFI_BLOCK_BC) &&
				!cpuport_util_is_brwan_wifi(pktinfo))
			{
				continue; //skip without forward
			} else if (((pktinfo->dst_mac[0] & 0x81) == 0x01) &&
				(omci_env_g.cpuport_wifi_block_mask & ENV_CPUPORT_WIFI_BLOCK_MC)) {
				//dmac mc
				continue; //skip without forward
			} else if (da_portmask == 0 && //unknown unicast
				(omci_env_g.cpuport_wifi_block_mask & ENV_CPUPORT_WIFI_BLOCK_UC)){
				//dmac unknown uc
				continue; //skip without forward
			}

			//calix, for blocking ipv6 downstream RA to wifi, force wifi all ssid could not get bridge ipv6 address.
			//but need to fix it in the future,
			value_ptr = (unsigned char *)pktinfo->nh; //assignment
			if (omci_env_g.cpuport_wifi_block_ra &&
				pktinfo->dst_mac[0] == 0x33 &&
				pktinfo->dst_mac[1] == 0x33 &&
				pktinfo->tci.ethertype == 0x86dd &&
				value_ptr[6] == NEXTHDR_ICMP && //icmpv6, assume have no any other extension header
				value_ptr[40] == NDISC_ROUTER_ADVERTISEMENT)
			{
				continue; //skip without forward
			}
			
		}

		internal_bport_me = hwresource_public2private(pktinfo->rx_desc.bridge_port_me);

		pktinfo->tx_desc.bridge_port_me = cpuport_util_get_bridge_port_me_by_logical_port_id(i);
		pktinfo->tx_desc.logical_port_id = i;
		if ((1<<i) & omci_env_g.trunk_group_portmask) {
			if (trunk_group_send > 0)
				continue;	// dont fwd more than once to same group trunck ports 
		}
		
		// check if dst port is on the same bridge as src port
		if(meinfo_util_me_attr_data(internal_bport_me, 4) != 4) { // skip broadcast gem port since it shall broadcast to all ports
			// TO BE FIX, tricky! kevin
			// the following code tries to determine the equiv egress port me used in cpuport_util_send_lan_handle_tag() (this is ugly)
			// and judge if src and dst port both belong to same bridge for L2fwd (this is strange)
			// Normally, the real ingress/egress port should be used for same bridge judgement, not the equiv port,
			// however, if port2port_enable on alu olt, even uni1..uni3 are on bridge without wan, the ds traffic from wan should be fwd to them
			// todo:
			// the the same bridge judgement for L2fwd should be moved into cpuport_util_send_lan_handle_tag()
			if (omci_env_g.port2port_enable) { // refer uni0 first, then veip
				if(uni0_port_me) {
					if(meinfo_util_me_attr_data(uni0_port_me, 1) != bridge_id) {
						dbprintf_cpuport(LOG_NOTICE, "port2port_enable, but uni0 not in the same bridge(%d) bport(0x%x)\n", bridge_id, pktinfo->rx_desc.bridge_port_me->meid);
						continue;
					}
				} else if(veip_bridge_port) {
					if(meinfo_util_me_attr_data(veip_bridge_port, 1) != bridge_id) {
						dbprintf_cpuport(LOG_NOTICE, "port2port_enable, but veip not in the same bridge(%d) bport(0x%x)\n", bridge_id, pktinfo->rx_desc.bridge_port_me->meid);
						continue;
					}
				} else {
					dbprintf_cpuport(LOG_NOTICE, "logical port %d port ME not found\n", i);
					continue;
				}
			} else if (pktinfo->tx_desc.bridge_port_me == NULL) { // maybe created by autouni
				if(veip_bridge_port) { // maybe autouni
					if(meinfo_util_me_attr_data(veip_bridge_port, 1) != bridge_id) {
						dbprintf_cpuport(LOG_NOTICE, "logical port %d port ME not found or not in the same bridge(%d) bport(0x%x)\n", i, bridge_id, pktinfo->rx_desc.bridge_port_me->meid);
						continue;
					}
				} else {
					dbprintf_cpuport(LOG_NOTICE, "logical port %d port ME not found\n", i);
					continue;
				}
			} else {
				 if(meinfo_util_me_attr_data(pktinfo->tx_desc.bridge_port_me,1) != bridge_id ) {
					dbprintf_cpuport(LOG_NOTICE, "logical port %d port ME (0x%0x) not in the same bridge(%d) bport(0x%x)\n", i, pktinfo->tx_desc.bridge_port_me->meid, bridge_id, pktinfo->rx_desc.bridge_port_me->meid);
					continue;
				}
			}
		}
		
		// clone pktinfo for multiple send
		//memset(&cloned_pkt[pktinfo->frame_len], 0, sizeof(cloned_pkt)-pktinfo->frame_len);
		memcpy(&cloned_pkt, pktinfo->frame_ptr, pktinfo->frame_len);
		memcpy(&cloned_pktinfo, pktinfo, sizeof(struct cpuport_info_t));
		cloned_pktinfo.frame_ptr = cloned_pkt;
		// recalculate nh
		cloned_pktinfo.nh = pktinfo->nh ? (void *)cloned_pktinfo.frame_ptr + (pktinfo->nh - (void *)pktinfo->frame_ptr) : 0; //offset
		
		if (wan_to_lan_cb) {
			ret = wan_to_lan_cb(&cloned_pktinfo, prompt, send_deny_check);
		} else {
			ret = cpuport_util_send_lan_handle_tag(&cloned_pktinfo, prompt, send_deny_check);
		}		
		if (ret >0) {
			total_send++;
			if ((1<<cloned_pktinfo.tx_desc.logical_port_id) & omci_env_g.trunk_group_portmask)
				trunk_group_send++;
		} else if (ret <0) {
			total_err++;
		}
	}
	//util_dbprintf(omci_env_g.debug_level_cpuport, LOG_NOTICE, 0, "\n");
	if (total_send)
		return 1;
	if (total_err)
		return -1;
	return 0;
}
int
cpuport_util_send_multi_lan(struct cpuport_info_t *pktinfo, char *prompt, 	// for multiple wan to multiple lan
  int (send_deny_check)(struct cpuport_info_t *, char *),
  int (wan_to_lan_cb)(struct cpuport_info_t *pktinfo, char *prompt, int (send_deny_check)(struct cpuport_info_t *, char *)))
{
	int total_send=0, total_err=0;
	int i, ret;

	// drop wan-to-uni packet if no wan bport me
	if ( !pktinfo->rx_desc.bridge_port_me )
		return -1;

	if (pktinfo->rx_desc.rx_wan_total <= 1)
		return _cpuport_util_send_multi_lan(pktinfo, prompt, send_deny_check, wan_to_lan_cb);

	for (i =0; i< pktinfo->rx_desc.rx_wan_total; i++) {
		pktinfo->rx_desc.bridge_port_me = pktinfo->rx_desc.rx_wan_me[i];
		ret = _cpuport_util_send_multi_lan(pktinfo, prompt, send_deny_check, wan_to_lan_cb);
		if (ret > 0)
			total_send++;
		if (ret < 0)
			total_err++;
	}
	if (total_send >0)
		return total_send;
	if (total_err >0)
		return -1;
	return 0;
}

// one(lan) to many(lan) passthrough ///////////////////////////////////////////////////////////////////////////

// used for uni to uni local bridging
int
cpuport_util_send_multi_lan_from_lan(struct cpuport_info_t *pktinfo, char *prompt, int (send_deny_check)(struct cpuport_info_t *, char *))
{
	int trunk_group_send = 0;
	unsigned int da_portmask = switch_get_dmac_logical_portmask(pktinfo->dst_mac);
	unsigned int isolation_portmask;
	int i;	
	unsigned char tag_num;

        if (switch_hw_g.port_isolation_get(pktinfo->rx_desc.logical_port_id, &isolation_portmask) != 0) {
                // in case any error, set isolation portmask to all logical portmask so it wont filter any dst port
                isolation_portmask = switch_get_all_logical_portmask();
        }        

	dbprintf_cpuport(LOG_INFO, "%s rx logical_port_id = %d\n", prompt, pktinfo->rx_desc.logical_port_id);
	for ( i=0; i<SWITCH_LOGICAL_PORT_TOTAL; i++ ) {
	
		if (((1<<i) & switch_get_all_logical_portmask()) == 0)
			continue;
		// skip dst port not found in the isolation set of srcport
		if (((1<<i) & isolation_portmask) == 0)
			continue;		
		// ignore wan & veip/iphost port
		if ( i == switch_get_wan_logical_portid() || i == switch_get_cpu_logical_portid(0) )
			continue;
		if ( i == pktinfo->rx_desc.logical_port_id )
			continue;
		if ( strlen(omci_env_g.wifi_if)  && (i == switch_get_wifi_logical_portid(0) || i == switch_get_wifi_logical_portid(1))) {
			if (!(( strcmp(omci_env_g.wifi_if,"wlan0") == 0 && i==switch_get_wifi_logical_portid(0) ) || 
			       ( strcmp(omci_env_g.wifi_if,"wlan1") == 0 && i==switch_get_wifi_logical_portid(1) ) ||
			       ( strcmp(omci_env_g.wifi_if,"both") == 0 && 
			       (i==switch_get_wifi_logical_portid(0) || i==switch_get_wifi_logical_portid(1)) )))
				continue;
		}

		// skip linkdown port
		if (((1<<i) & switch_get_linkup_logical_portmask()) == 0)
			continue;		
		// skip if da found in mactab but not on the tx dest port
		if (da_portmask && ((1<<i) & da_portmask) == 0)	
			continue;

		pktinfo->tx_desc.logical_port_id = i;		
		if ((1<<i) & omci_env_g.trunk_group_portmask) {
			if ((1<<pktinfo->rx_desc.logical_port_id) & omci_env_g.trunk_group_portmask)
				continue;	// dont fwd between trunk ports
			if (trunk_group_send > 0)
				continue;	// dont fwd more than once to same group trunck ports
		}				

		if ((send_deny_check != NULL) &&
		    (RET_YES == send_deny_check(pktinfo, prompt))) {
			char *src_name, *dst_name;
			src_name = switch_get_logical_portname(pktinfo->rx_desc.logical_port_id);
			dst_name = switch_get_logical_portname(pktinfo->tx_desc.logical_port_id);		    
			dbprintf_cpuport(LOG_NOTICE, "send deny for %s, %s->%s\n", prompt, src_name, dst_name);
			continue;
		}
		
		if (omci_env_g.debug_level_cpuport >= LOG_NOTICE) {
			char *src_name, *dst_name;
			src_name = switch_get_logical_portname(pktinfo->rx_desc.logical_port_id);
			dst_name = switch_get_logical_portname(pktinfo->tx_desc.logical_port_id);
			dbprintf_cpuport(LOG_NOTICE, "%s, %s->%s\n", prompt, src_name, dst_name);
		}
		
		//remove tag from wifi to uni, the tag was for distingulishing different ssid, unnecessary for uni.
		if (((1<<pktinfo->rx_desc.logical_port_id) & switch_get_wifi_logical_portmask()) && //from wifi port
			((1<<pktinfo->tx_desc.logical_port_id) & switch_get_uni_logical_portmask()) && //to uni port
			(tag_num = cpuport_util_get_tagnum_from_tci(&pktinfo->tci) > 0))
		{
			static unsigned char cloned_pkt[2048];
			struct cpuport_info_t cloned_pktinfo;
	
			// clone pktinfo for multiple send
			//memset(&cloned_pkt[pktinfo->frame_len], 0, sizeof(cloned_pkt)-pktinfo->frame_len);
			memcpy(&cloned_pkt, pktinfo->frame_ptr, pktinfo->frame_len);
			memcpy(&cloned_pktinfo, pktinfo, sizeof(struct cpuport_info_t));
			cloned_pktinfo.frame_ptr = cloned_pkt;
			// recalculate nh
			cloned_pktinfo.nh = pktinfo->nh ? (void *)cloned_pktinfo.frame_ptr + (pktinfo->nh - (void *)pktinfo->frame_ptr) : 0; //offset
			cpuport_util_remove_tag(&cloned_pktinfo);
			cpuport_frame_send(&cloned_pktinfo);
		} else {
			cpuport_frame_send(pktinfo);
		}
		
		if ((1<<pktinfo->tx_desc.logical_port_id) & omci_env_g.trunk_group_portmask)
			trunk_group_send++;
	}
	//util_dbprintf(omci_env_g.debug_level_cpuport, LOG_NOTICE, 0, "\n");
	return 0;
}

