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
 * File    : cpuport_pvlan.c
 *
 ******************************************************************/

#include <sys/time.h>
#include <errno.h>
#include <sys/types.h>
#include <net/if_arp.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>

#include "util.h"
#include "util_inet.h"
#include "list.h"
#include "env.h"
#include "meinfo.h"
#include "omciutil_vlan.h"
#include "batchtab_cb.h"
#include "batchtab.h"
#include "classf.h"
#include "cpuport.h"
#include "cpuport_util.h"
#include "cpuport_vtagging_vfilter.h"
#include "cpuport_ipv6.h"

static int
cpuport_pvlan_acl_info_from_pkt(struct cpuport_info_t *pktinfo, struct classf_pvlan_acl_t *pvlan_acl_info)
{
	if (pktinfo == NULL || pvlan_acl_info == NULL) {
		dbprintf_cpuport(LOG_NOTICE, "parameter is NULL\n");
		return -1;
	}

	memset(pvlan_acl_info, 0, sizeof(struct classf_pvlan_acl_t));

	pvlan_acl_info->ethertype = pktinfo->tci.ethertype;
	*(unsigned int *)&pvlan_acl_info->smac[0] = *(unsigned int*)&pktinfo->src_mac[0];
	*(unsigned short *)&pvlan_acl_info->smac[4] = *(unsigned short*)&pktinfo->src_mac[4];
	*(unsigned int *)&pvlan_acl_info->dmac[0] = *(unsigned int*)&pktinfo->dst_mac[0];
	*(unsigned short *)&pvlan_acl_info->dmac[4] = *(unsigned short*)&pktinfo->dst_mac[4];

	if (pvlan_acl_info->ethertype == 0x0800) {
		struct iphdr *ip_hdr= (struct iphdr *)(pktinfo->nh);
		pvlan_acl_info->sip.s_addr = ip_hdr->saddr;
		pvlan_acl_info->dip.s_addr = ip_hdr->daddr;
		pvlan_acl_info->protocol = ip_hdr->protocol;
		pvlan_acl_info->sport = ntohs(*(unsigned short *)(ip_hdr+(ip_hdr->ihl)*4));
		pvlan_acl_info->dport = ntohs(*(unsigned short *)(ip_hdr+(ip_hdr->ihl)*4+2));

	}else if (pvlan_acl_info->ethertype == 0x0806) {
		char sender_ip[32];
		char target_ip[32];
		
		if(omci_env_g.cpuport_pvlan_clone_ipv4_to_arp) {
			// fake arp ip as ip so it can be matched by ipv4 acl rule
			struct arphdr *arp=(struct arphdr*)(pktinfo->nh);
			unsigned char *smac=(void *)arp + sizeof(struct arphdr);
			unsigned char *s=smac+6;
			unsigned char *dmac=s+4;
			unsigned char *d=dmac+6;
			struct in_addr *sip = (struct in_addr *)(s);
			struct in_addr *dip = (struct in_addr *)(d);
			pvlan_acl_info->sip.s_addr = sip->s_addr;
			pvlan_acl_info->dip.s_addr = dip->s_addr;
			pvlan_acl_info->fake_arp_to_ip = 1;
		}
	
		inet_ntop(AF_INET,&pvlan_acl_info->sip, sender_ip, 32);
		inet_ntop(AF_INET,&pvlan_acl_info->dip, target_ip, 32);
		dbprintf_cpuport(LOG_NOTICE, "ARP pkt acl info: smac:%02x:%02x:%02x:%02x:%02x:%02x, dmac:%02x:%02x:%02x:%02x:%02x:%02x, sip:%s, tip:%s\n",
			pvlan_acl_info->smac[0],pvlan_acl_info->smac[1],pvlan_acl_info->smac[2],
			pvlan_acl_info->smac[3],pvlan_acl_info->smac[4],pvlan_acl_info->smac[5],
			pvlan_acl_info->dmac[0],pvlan_acl_info->dmac[1],pvlan_acl_info->dmac[2],
			pvlan_acl_info->dmac[3],pvlan_acl_info->dmac[4],pvlan_acl_info->dmac[5],
			sender_ip,target_ip);
	} else if (pvlan_acl_info->ethertype == 0x86dd) {
		struct ipv6hdr *ip6 = (struct ipv6hdr*)(pktinfo->nh);
		unsigned char nexthdr = ip6 ->nexthdr; 
		struct ipv6_exthdr *e = (void *)ip6+sizeof(struct ipv6hdr);	// exthdr is valid only if not payload(tcp/udp/icmp/ospfv3) after ipv6 hdr 
		void *exthdr_end = (void *)e + 8 + (e->optlen <<3);
		
		//v6sip, v6dip
		pvlan_acl_info->v6sip = ip6->saddr;
		pvlan_acl_info->v6dip = ip6->daddr;

		while (nexthdr != NEXTHDR_NONE)
		{
			if (nexthdr==NEXTHDR_TCP || nexthdr==NEXTHDR_UDP)
			{
				struct tcphdr *tcp = (void *)e;
				struct udphdr *udp = (void *)e;
				
				if  (nexthdr==NEXTHDR_TCP) {
					pvlan_acl_info->sport=ntohs(tcp->source);
					pvlan_acl_info->dport=ntohs(tcp->dest);
				} else {
					pvlan_acl_info->sport=ntohs(udp->source);
					pvlan_acl_info->dport=ntohs(udp->dest);
				}

				pvlan_acl_info->protocol = nexthdr;
				
				break; // process end
			} else if (nexthdr == NEXTHDR_ICMP) {
				struct icmp6hdr *icmp6 = (struct icmp6hdr *)e;

				//get target addr
				if (icmp6->icmp6_type == NDISC_NEIGHBOUR_SOLICITATION || //135
					icmp6->icmp6_type == NDISC_NEIGHBOUR_ADVERTISEMENT || //136
					 icmp6->icmp6_type == NDISC_REDIRECT) // 137
				{
					struct nd_msg *nd = (struct nd_msg *)icmp6;
					pvlan_acl_info->v6nd_target_ip = nd->target;
					pvlan_acl_info->care_bit.v6nd_target_ip = 1; //use this bit to mark for comparing
				}
				
				pvlan_acl_info->protocol = nexthdr;

				break; // process end
			} else if (nexthdr == NEXTHDR_OSPFV3) {
				pvlan_acl_info->protocol = nexthdr;
				break;
			} else if (nexthdr == NEXTHDR_FRAGMENT) {
				//has extension header, but this header format is different with others, re-caculate extension end.
				exthdr_end = (void *)e + 8; //fragment head is 8 bytes
			}

			//check packet boundary
			if (exthdr_end > (void *)(ip6 + (pktinfo->frame_len-((void*)ip6-(void*)pktinfo->frame_ptr))))
			{
				break; //process end
			} else {
				//otherwise, move to next one
				nexthdr = e->nexthdr;
				e = exthdr_end;
				//check next extend header boundary
				if (((void *)e + 8) > ((void *)(ip6 + (pktinfo->frame_len-((void*)ip6-(void*)pktinfo->frame_ptr)))))
				{
					break; //process end
				} else {
					exthdr_end = (void *)e + 8 + (e->optlen <<3);	
				}
			}
		}
	}

	return 0;
}

//return 0: the same, -1: not the same
static int cpuport_pvlan_acl_cmp_v6ip(struct in6_addr v6ip1, struct in6_addr v6ip2, unsigned char prefix_len)
{
	unsigned char plen = (prefix_len >0 &&  prefix_len <= 128) ? prefix_len : 128; //0 -> 128
	int i;
	unsigned char pbytes = plen / 8;
	unsigned char pbits = plen % 8;
	unsigned char p;

	if (pbits == 0)
	{
		p = pbytes;
	} else {
		p = pbytes + 1;
	}

	for (i = 0; i < p; i++)
	{
		if (i == (p - 1) && pbits != 0)
		{
			if ((v6ip1.s6_addr[i] >> (8 - pbits)) != (v6ip2.s6_addr[i] >> (8 - pbits)))
			{
				return -1;
			}
		} else {
			if (v6ip1.s6_addr[i] != v6ip2.s6_addr[i])
			{
				return -1;
			}
		}
	}

	return 0; //the same
}

static int
cpuport_pvlan_acl_cmp(struct classf_pvlan_acl_t *pvlan_acl_info, struct classf_pvlan_acl_t *pvlan_acl)
{
	int i;

	if ( pvlan_acl->care_bit.ethertype ) {
		if (pvlan_acl_info->fake_arp_to_ip == 1)
		{
			//pvlan_acl_info must be arp
			if (pvlan_acl->ethertype != 0x0800 && pvlan_acl->ethertype != 0x0806)
			{
				return -1;
			}
		} else {
			if ( pvlan_acl_info->ethertype != pvlan_acl->ethertype)
				return -1;
		}
	}

	if ( pvlan_acl->care_bit.smac) {
		if ( pvlan_acl->care_bit.smac_mask ) {
			for ( i=0 ; i<6; i++ ) {
				if ( (pvlan_acl->smac_mask[i] & pvlan_acl_info->smac[i]) != (pvlan_acl->smac_mask[i] & pvlan_acl->smac[i]) )
					return -1;
			}
		}else{
			if (memcmp(pvlan_acl->smac,pvlan_acl_info->smac,CPUPORT_MAC_ADDR_LEN))
				return -1;
		}
	}

	if ( pvlan_acl->care_bit.dmac) {
		if ( pvlan_acl->care_bit.dmac_mask ) {
			for ( i=0 ; i<6; i++ ) {
				if ( (pvlan_acl->dmac_mask[i] & pvlan_acl_info->dmac[i]) != (pvlan_acl->dmac_mask[i] & pvlan_acl->dmac[i]) )
					return -1;
			}
		}else{
			if (memcmp(pvlan_acl->dmac,pvlan_acl_info->dmac,CPUPORT_MAC_ADDR_LEN))
				return 1;
		}
	}

	if ( pvlan_acl->care_bit.sip) {
		if ( pvlan_acl->care_bit.sip_mask) {
			if ((pvlan_acl->sip.s_addr & pvlan_acl->sip_mask.s_addr) != (pvlan_acl_info->sip.s_addr & pvlan_acl->sip_mask.s_addr ))
				return -1;
		}else{
			if (pvlan_acl->sip.s_addr != pvlan_acl_info->sip.s_addr)
				return -1;
		}
	}

	if ( pvlan_acl->care_bit.dip) {
		if ( pvlan_acl->care_bit.dip_mask) {
			if ((pvlan_acl->dip.s_addr & pvlan_acl->dip_mask.s_addr) != (pvlan_acl_info->dip.s_addr & pvlan_acl->dip_mask.s_addr ))
				return -1;
		}else{
			if (pvlan_acl->dip.s_addr != pvlan_acl_info->dip.s_addr)
				return -1;
		}
	}

	if ( pvlan_acl->care_bit.v6sip) {
		if (cpuport_pvlan_acl_cmp_v6ip(pvlan_acl_info->v6sip, pvlan_acl->v6sip, pvlan_acl->v6sip_prefix) < 0)
		{
			return -1;
		}
	}

	if ( pvlan_acl->care_bit.v6dip) {
		if (cpuport_pvlan_acl_cmp_v6ip(pvlan_acl_info->v6dip, pvlan_acl->v6dip, pvlan_acl->v6dip_prefix) < 0)
		{
			return -1;
		}
	}

	if ( pvlan_acl->care_bit.protocol) {
		if ( pvlan_acl->protocol != pvlan_acl_info->protocol)
			return -1;
	}

	if ( pvlan_acl->care_bit.v6nd_target_ip) {
		if (!pvlan_acl_info->care_bit.v6nd_target_ip || cpuport_pvlan_acl_cmp_v6ip(pvlan_acl_info->v6nd_target_ip, pvlan_acl->v6nd_target_ip, 128) < 0)
		{
			return -1;
		}
	}

	if ( pvlan_acl->care_bit.sport) {
		if (pvlan_acl->care_bit.sport_ubound) {
			if ((pvlan_acl_info->sport < pvlan_acl->sport) || (pvlan_acl_info->sport > pvlan_acl->sport_ubound) )
				return -1;
		}else {
			if ( pvlan_acl->sport != pvlan_acl_info->sport )
				return -1;
		}
	}

	if ( pvlan_acl->care_bit.dport) {
		if (pvlan_acl->care_bit.dport_ubound) {
			if ((pvlan_acl_info->dport < pvlan_acl->dport) || (pvlan_acl_info->dport > pvlan_acl->dport_ubound) )
				return -1;
		}else {
			if ( pvlan_acl->dport != pvlan_acl_info->dport )
				return -1;
		}
	}

	return 0;
}

static char *
cpuport_pvlan_acl_str(struct classf_pvlan_acl_t *pvlan_acl)
{
	static char line[LINE_MAXSIZE];
	char *s = line;
	char *sep ="";

	if ( pvlan_acl->care_bit.ethertype ) {
		s += sprintf(s, "%sethertype=%04X", sep, pvlan_acl->ethertype);
		sep=", ";
	}

	if ( pvlan_acl->care_bit.smac) {
		s += sprintf(s, "%ssmac=%02x:%02x:%02x:%02x:%02x:%02x", sep,
			pvlan_acl->smac[0], pvlan_acl->smac[1], pvlan_acl->smac[2],
			pvlan_acl->smac[3], pvlan_acl->smac[4], pvlan_acl->smac[5]);
		if ( pvlan_acl->care_bit.smac_mask ) {
			s += sprintf(s, "/%02x:%02x:%02x:%02x:%02x:%02x",
				pvlan_acl->smac_mask[0], pvlan_acl->smac_mask[1], pvlan_acl->smac_mask[2],
				pvlan_acl->smac_mask[3], pvlan_acl->smac_mask[4], pvlan_acl->smac_mask[5]);
		}
		sep=", ";
	}
	if ( pvlan_acl->care_bit.dmac) {
		s += sprintf(s, "%sdmac=%02x:%02x:%02x:%02x:%02x:%02x", sep,
			pvlan_acl->dmac[0], pvlan_acl->dmac[1], pvlan_acl->dmac[2],
			pvlan_acl->dmac[3], pvlan_acl->dmac[4], pvlan_acl->dmac[5]);
		if ( pvlan_acl->care_bit.dmac_mask ) {
			s += sprintf(s, "/%02x:%02x:%02x:%02x:%02x:%02x", 
				pvlan_acl->dmac_mask[0], pvlan_acl->dmac_mask[1], pvlan_acl->dmac_mask[2],
				pvlan_acl->dmac_mask[3], pvlan_acl->dmac_mask[4], pvlan_acl->dmac_mask[5]);
		}
		sep=", ";
	}

	if ( pvlan_acl->care_bit.sip) {
		char ip_str[32];
		inet_ntop(AF_INET,&pvlan_acl->sip, ip_str,32);
		s += sprintf(s, "%ssip=%s", sep, ip_str); 
		if ( pvlan_acl->care_bit.sip_mask) {
			inet_ntop(AF_INET,&pvlan_acl->sip_mask, ip_str,32);
			s += sprintf(s, "/%s", ip_str); 
		}
		sep=", ";
	}
	if ( pvlan_acl->care_bit.dip) {
		char ip_str[32];
		inet_ntop(AF_INET,&pvlan_acl->dip, ip_str,32);
		s += sprintf(s, "%sdip=%s", sep, ip_str); 
		if ( pvlan_acl->care_bit.dip_mask) {
			inet_ntop(AF_INET,&pvlan_acl->dip_mask, ip_str,32);
			s += sprintf(s, "/%s", ip_str); 
		}
		sep=", ";
	}
	if (pvlan_acl->care_bit.v6sip)
	{
		char v6ip_str[64];
		util_inet_ntop(AF_INET6, pvlan_acl->v6sip.s6_addr, v6ip_str, 64);
		s += sprintf(s, "%sv6sip=%s", sep, v6ip_str);
		if (pvlan_acl->care_bit.v6sip_prefix && pvlan_acl->v6sip_prefix > 0)
		{
			s += sprintf(s, "/%d", pvlan_acl->v6sip_prefix);
		}
		sep=", ";
	}
	if (pvlan_acl->care_bit.v6sip)
	{
		char v6ip_str[64];
		util_inet_ntop(AF_INET6, pvlan_acl->v6dip.s6_addr, v6ip_str, 64);
		s += sprintf(s, "%sv6dip=%s", sep, v6ip_str);
		if (pvlan_acl->care_bit.v6dip_prefix && pvlan_acl->v6dip_prefix > 0)
		{
			s += sprintf(s, "/%d", pvlan_acl->v6dip_prefix);
		}
		sep=", ";
	}
	if ( pvlan_acl->care_bit.protocol) {
		char *str = cpuport_util_ip_proto_str(pvlan_acl->protocol);
		if (strlen(str)) {
			s += sprintf(s, "%sip_proto=%02x(%s)", sep, pvlan_acl->protocol, str); 
		} else {
			s += sprintf(s, "%sip_proto=%02x", sep, pvlan_acl->protocol); 
		}
		sep=", ";
	}
	if (pvlan_acl->care_bit.v6nd_target_ip)
	{
		char v6ip_str[64];
		util_inet_ntop(AF_INET6, pvlan_acl->v6nd_target_ip.s6_addr, v6ip_str, 64);
		s += sprintf(s, "%sv6nd_target_ip=%s", sep, v6ip_str);
		sep=", ";
	}
	if ( pvlan_acl->care_bit.sport) {
		s += sprintf(s, "%ssport=%d(0x%x)", sep, pvlan_acl->sport, pvlan_acl->sport); 
		if (pvlan_acl->care_bit.sport_ubound) {
			s += sprintf(s, "..%d(0x%x)", pvlan_acl->sport_ubound, pvlan_acl->sport_ubound); 
		}
		sep=", ";
	}
	if ( pvlan_acl->care_bit.dport) {
		s += sprintf(s, "%sdport=%d(0x%x)", sep, pvlan_acl->dport, pvlan_acl->dport); 
		if (pvlan_acl->care_bit.dport_ubound) {
			s += sprintf(s, "..%d(0x%x)", pvlan_acl->dport_ubound, pvlan_acl->dport_ubound); 
		}
		sep=", ";
	}
	return line;
}

static void
dbprintf_cpuport_pvlan_rule(struct classf_pvlan_rule_t *pvlan_rule) 
{
	if (omci_env_g.debug_level_cpuport >= LOG_NOTICE) {
		util_dbprintf(omci_env_g.debug_level_cpuport, LOG_NOTICE, 0, "Pvlan ACL%d:%s\n", 
			pvlan_rule->pvlan_acl.rule_num, cpuport_pvlan_acl_str(&pvlan_rule->pvlan_acl));
		util_dbprintf(omci_env_g.debug_level_cpuport, LOG_NOTICE, 0, "Pvlan rule%d: %s\n", 
			pvlan_rule->rule_fields.entry_id, omciutil_vlan_str_desc(&pvlan_rule->rule_fields, "\n\t", "", 0));
	}
}

static struct classf_pvlan_rule_t *
cpuport_pvlan_rule_search (struct me_t *bridge_port, unsigned char is_upstream, struct cpuport_tci_t *src_tci, struct classf_pvlan_acl_t *pvlan_acl_info)
{
	struct classf_info_t *t = batchtab_table_data_get("classf");
	struct classf_bridge_info_t *br_info=NULL, *br_info_n=NULL, *bridge_info=NULL;
	struct classf_bridge_info_lan_port_t *lan_port=NULL, *lan_port_n=NULL, *lan_port_info=NULL;
	struct classf_pvlan_rule_t *pvlan_rule=NULL, *pvlan_rule_n=NULL;
	struct list_head *pvlan_rule_list;

	if ( !t) {
		dbprintf_cpuport(LOG_NOTICE, "classf_info not found\n");
		return NULL;
	}

	list_for_each_entry_safe ( br_info, br_info_n, &t->bridge_info_list, b_node ) {
		if ( br_info->me_bridge->meid == meinfo_util_me_attr_data(bridge_port, 1) ) {
			bridge_info = br_info;
		}
	}
	if ( !bridge_info ) {
		dbprintf_cpuport(LOG_NOTICE, "classf br_info not found\n");
		batchtab_table_data_put("classf");
		return NULL;
	}

	list_for_each_entry_safe ( lan_port , lan_port_n , &bridge_info->lan_port_list , lp_node ) {
		if ( lan_port->me_bridge_port->meid == bridge_port->meid ) {
			lan_port_info=lan_port;
			break;
		}
	}
	if (!lan_port_info) {
		dbprintf_cpuport(LOG_NOTICE, "lan_port_info not found\n");
		batchtab_table_data_put("classf");
		return NULL;
	}

	pvlan_rule_list = (is_upstream)? &lan_port_info->port_protocol_vlan.us_pvlan_rule_list:&lan_port_info->port_protocol_vlan.ds_pvlan_rule_list;
	list_for_each_entry_safe ( pvlan_rule, pvlan_rule_n , pvlan_rule_list , pr_node ) {
		if ( cpuport_pvlan_acl_cmp(pvlan_acl_info, &pvlan_rule->pvlan_acl) == 0 ) {
			if (cpuport_vtagging_match( src_tci, &pvlan_rule->rule_fields) == CPUPORT_VTAGGING_RULE_MATCH) {
				batchtab_table_data_put("classf");
				return pvlan_rule;
			}
		}
	}

	batchtab_table_data_put("classf");
	return NULL;
}


static int
cpuport_pvlan_rule_apply (struct classf_pvlan_rule_t *pvlan_rule, struct cpuport_tci_t *src_tci,struct cpuport_tci_t *dst_tci)
{
	int result;
	result = cpuport_vtagging_treatment( src_tci, &pvlan_rule->rule_fields , dst_tci);
	if (result == CPUPORT_VTAGGING_TREATMENT_FAIL) {
		dbprintf_cpuport(LOG_NOTICE, "protocol vlan tag treatment fail\n");
		return -1;
	}
	return 0;
}

// return 0: ok, -1: not applied
int
cpuport_pvlan_vtagging(struct me_t *bridge_port, unsigned char is_upstream, struct cpuport_tci_t *src_tci, struct cpuport_tci_t *dst_tci, struct cpuport_info_t *cpuport_info)
{
	struct classf_pvlan_rule_t *pvlan_rule;
	struct classf_pvlan_acl_t pvlan_acl_info;

	// default assume dst_tci is the same as src_tci
	*dst_tci = *src_tci;

	//get information needed by pvlan acl match from packet
	if (cpuport_pvlan_acl_info_from_pkt(cpuport_info, &pvlan_acl_info) < 0 ) {
		dbprintf_cpuport(LOG_NOTICE, "fail to get pvlan acl info from pkt\n");
		return -1;
	}	
	if ((pvlan_rule = cpuport_pvlan_rule_search(bridge_port, is_upstream, src_tci, &pvlan_acl_info)) == NULL) {
		dbprintf_cpuport(LOG_NOTICE, "pvlan rule not found for the pkt\n");
		return -1;
	}
	
	dbprintf_cpuport_pvlan_rule(pvlan_rule);	// print vtagging & acl rule 

	if (cpuport_pvlan_rule_apply (pvlan_rule, src_tci, dst_tci) < 0) {
		dbprintf_cpuport(LOG_NOTICE, "pvlan rule not applied\n");
		return -1;
	}
	
	dbprintf_cpuport(LOG_NOTICE, "pvlan rule applied successfully\n");
	return 0;
}
