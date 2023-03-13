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
 * Module  : cpuport_pkt
 * File    : cpuport_pkt.c
 *
 ******************************************************************/

#include <time.h>
#include <string.h>

#include <linux/if_ether.h>
#include <linux/ppp_defs.h>
#include <linux/tcp.h>
#include <linux/udp.h>

#include "list.h"
#include "fwk_msgq.h"
#include "fwk_thread.h"
#include "util.h"
#include "env.h"
#include "switch.h"
#include "cpuport.h"
#include "cpuport_util.h"
#include "cpuport_frame.h"
#include "cpuport_sniffer.h"
#include "cpuport_history.h"
#include "cpuport_util_send.h"
#include "cpuport_pkt.h"
#include "metacfg_adapter.h"
#include "mcast_pkt.h"
#include "cpuport_ipv6.h"

int pkt_recv_qid_g = -1;
int pkt_cmd_qid_g = -1;
int pkt_qsetid_g = -1;
int pkttask_loop_g = 0;
static struct fwk_thread_t pkttask_thread_g;
int rtn;

// multicast dmac=01:xx:xx:xx:xx:xx
static int
cpuport_pkt_is_multicast(struct cpuport_info_t *pktinfo)
{
	if ((pktinfo->dst_mac[0] & 0x81) == 0x1)
	{
		return 1;
	} else {
		return 0;
	}
}

// multicast dmac=01:xx:xx:xx:xx:xx, and is ipv4, 
// excludes reserve ipv4 multicast group 224.0.0.x and 224.0.1.x
static int
cpuport_pkt_is_multicast_ipv4(struct cpuport_info_t *pktinfo)
{
	if ((pktinfo->dst_mac[0] & 0x81) == 0x1 &&
		pktinfo->tci.ethertype == 0x0800) //ipv4
	{
		struct iphdr *ip=(struct iphdr*)(pktinfo->nh);
		unsigned char *d=(unsigned char*)&(ip->daddr);
		//NOT 224.0.0.x and 224.0.1.x
		if (!(d[0]==224 && d[1]==0 && d[2]==0) &&
			!(d[0]==224 && d[1]==0 && d[2]==1))
		{
			return 1;
		} else {
			return 0;
		}
	}
	return 0;
}

// SSDP (simple service discovery protocol) dip:dport is 239.255.255.250:1900
static int
cpuport_pkt_is_ssdp(struct cpuport_info_t *pktinfo)
{
	if ((pktinfo->tci.ethertype == ETH_P_IP || 
	    (pktinfo->tci.ethertype == ETH_P_PPP_SES && pktinfo->pppoe_proto == PPP_IP)) && pktinfo->nh) {
		struct iphdr *ip=(struct iphdr*)(pktinfo->nh);
		unsigned char *d=(unsigned char*)&(ip->daddr);
		if (d[0]==239 && d[1]==255 && d[2]==255 && d[3]==250 && ip->protocol==IPPROTO_UDP) {
			struct udphdr *udp=(struct udphdr *)(pktinfo->nh+ip->ihl*4);
			unsigned short dp=ntohs(udp->dest);
			if (dp == 1900)
				return 1;
		}
	}
	return 0;
}

//0: not tlan or block, 1: forward
static int
cpuport_pkt_is_tlan_forward(struct cpuport_info_t *pktinfo)
{
	if(strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX)==0) {
		// tlan (transparent lan) is ani side vlan, so it'll be only used on downstream direction
		extern unsigned char calix_tlan_g[4096];
		if((pktinfo->tci.itci.vid != 0) && (calix_tlan_g[pktinfo->tci.itci.vid] == 1))
			return 1;
		else if((pktinfo->tci.otci.vid != 0) && (calix_tlan_g[pktinfo->tci.otci.vid] == 1))
			return 1;
		else
			return 0;
	}
	
	return 0;
}

// stp: dmac 01:80:c2:00:00:00, 802.1d
// stp: dmac 01:80:C2:00:00:08, 802.1ad
// Cisco Shared Spanning Tree Protocol Address: 01:00:0c:cc:cc:cd
static int
cpuport_pkt_is_stp_bpdu(struct cpuport_info_t *pktinfo)
{
	if ((pktinfo->dst_mac[0] == 0x1 &&
		pktinfo->dst_mac[1] == 0x80 &&
		pktinfo->dst_mac[2] == 0xc2 &&
		pktinfo->dst_mac[3] == 0x0 &&
		pktinfo->dst_mac[4] == 0x0 &&
		pktinfo->dst_mac[5] == 0x0) ||
		(pktinfo->dst_mac[0] == 0x1 &&
		pktinfo->dst_mac[1] == 0x80 &&
		pktinfo->dst_mac[2] == 0xc2 &&
		pktinfo->dst_mac[3] == 0x0 &&
		pktinfo->dst_mac[4] == 0x0 &&
		pktinfo->dst_mac[5] == 0x8) ||
		(pktinfo->dst_mac[0] == 0x1 &&
		pktinfo->dst_mac[1] == 0x0 &&
		pktinfo->dst_mac[2] == 0xc &&
		pktinfo->dst_mac[3] == 0xcc &&
		pktinfo->dst_mac[4] == 0xcc &&
		pktinfo->dst_mac[5] == 0xcd))
	{
		return (cpuport_pkt_is_tlan_forward(pktinfo)) ? 0 : 1;
	}
	return 0;
}

//0: not, 1: yes
static int
cpuport_pkt_is_pause_frame(struct cpuport_info_t *pktinfo)
{
	unsigned char pause_frame[] = {0x1, 0x80, 0xc2, 0x0, 0x0, 0x1};
	unsigned short *sub_type = (unsigned short*)(pktinfo->nh);
	if ((memcmp(pktinfo->dst_mac, pause_frame, 6) == 0) &&
		(pktinfo->tci.ethertype == 0x8808) &&
		(ntohs(sub_type[0]) == 0x0001))
	{
		return 1;
	}

	return 0;
}

//0: not, 1: yes
static int
cpuport_pkt_is_rma(struct cpuport_info_t *pktinfo)
{
	unsigned char rma_prefix[] = {0x1, 0x80, 0xc2, 0x0, 0x0};
	
	if (memcmp(pktinfo->dst_mac, rma_prefix, 5) == 0 && pktinfo->dst_mac[5] <=0x3f)
	{
		return 1;
	}

	return 0;
}

//0: not rma or block, 1: forward
int
cpuport_pkt_is_rma_forward(struct cpuport_info_t *pktinfo)
{
	unsigned char rma_prefix[] = {0x1, 0x80, 0xc2, 0x0, 0x0};
	unsigned long long rma_forward_mask = omci_env_g.rma_forward_mask;

	// for upstream calix case, check per-uni port l2cp table
	if(strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX)==0) {
		// ingress is uni or wifi port
		if((1<<pktinfo->rx_desc.logical_port_id) & (switch_get_uni_logical_portmask()|switch_get_wifi_logical_portmask())) {
			extern unsigned long long calix_l2cp_g[SWITCH_LOGICAL_PORT_TOTAL];
			if(calix_l2cp_g[pktinfo->rx_desc.logical_port_id])
				rma_forward_mask = calix_l2cp_g[pktinfo->rx_desc.logical_port_id];
			else
				rma_forward_mask = 0;
		}
	}
	
	if (memcmp(pktinfo->dst_mac, rma_prefix, 5) == 0 &&
		pktinfo->dst_mac[5] <=0x3f &&
		((1ULL<<pktinfo->dst_mac[5]) & rma_forward_mask) != 0)
	{
		return (cpuport_pkt_is_pause_frame(pktinfo)) ? 0 : 1; // always drop pause frame
	}

	return 0;
}

//0: not, 1: yes
static int
cpuport_pkt_is_igmp_mld(struct cpuport_info_t *pktinfo)
{
	if (igmp_pkt_recv_is_igmp_join_leave(pktinfo) == 0 ||
		igmp_pkt_recv_is_igmp_query(pktinfo) == 0 ||
		igmp_pkt_recv_is_mld_join_leave(pktinfo) == 0 ||
		igmp_pkt_recv_is_mld_query(pktinfo) == 0)
	{
		return 1;
	}

	return 0;	
}

//0: not, 1: yes
static int
cpuport_pkt_is_icmpv6(struct cpuport_info_t *pktinfo)
{
	if (pktinfo->tci.ethertype == 0x86dd)
	{
		struct ipv6hdr *ip6 = (struct ipv6hdr*)(pktinfo->nh);
		unsigned char nexthdr = ip6 ->nexthdr; 
		struct ipv6_exthdr *e = (void *)ip6+sizeof(struct ipv6hdr);	// exthdr is valid only if not payload(tcp/udp/icmp/ospfv3) after ipv6 hdr 
		void *exthdr_end = (void *)e + 8 + (e->optlen <<3);
		
		while (nexthdr != NEXTHDR_NONE)
		{
			if (nexthdr==NEXTHDR_TCP || nexthdr==NEXTHDR_UDP || nexthdr == NEXTHDR_OSPFV3)
			{
				break; // process end
			} else if (nexthdr == NEXTHDR_ICMP) {
				return 1; // process end
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

//0: not, 1: yes
static int
cpuport_pkt_is_dhcpv6(struct cpuport_info_t *pktinfo)
{
	if (pktinfo->tci.ethertype == 0x86dd)
	{
		struct ipv6hdr *ip6 = (struct ipv6hdr*)(pktinfo->nh);
		unsigned char nexthdr = ip6 ->nexthdr; 
		struct ipv6_exthdr *e = (void *)ip6+sizeof(struct ipv6hdr);	// exthdr is valid only if not payload(tcp/udp/icmp/ospfv3) after ipv6 hdr 
		void *exthdr_end = (void *)e + 8 + (e->optlen <<3);
		
		while (nexthdr != NEXTHDR_NONE)
		{
			if (nexthdr==NEXTHDR_UDP)
			{
				struct udphdr *udp = (void *)e;
				unsigned short sp=0, dp=0;
				
				sp=ntohs(udp->source);
				dp=ntohs(udp->dest);

				if ((sp==546 && dp==547) || (sp==547 && dp==546)) 
				{
					return 1; //dhcpv6
				}
				
				break; // process end
			} else if (nexthdr==NEXTHDR_TCP || nexthdr == NEXTHDR_ICMP || nexthdr == NEXTHDR_OSPFV3) {
				break; // process end
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

//0: not, 1: yes
static int
cpuport_pkt_is_ospfv3(struct cpuport_info_t *pktinfo)
{
	if (pktinfo->tci.ethertype == 0x86dd)
	{
		struct ipv6hdr *ip6 = (struct ipv6hdr*)(pktinfo->nh);
		unsigned char nexthdr = ip6 ->nexthdr; 
		struct ipv6_exthdr *e = (void *)ip6+sizeof(struct ipv6hdr);	// exthdr is valid only if not payload(tcp/udp/icmp/ospfv3) after ipv6 hdr 
		void *exthdr_end = (void *)e + 8 + (e->optlen <<3);
		
		while (nexthdr != NEXTHDR_NONE)
		{
			if (nexthdr==NEXTHDR_OSPFV3)
			{
				return 1;
			} else if (nexthdr==NEXTHDR_TCP || nexthdr == NEXTHDR_UDP || nexthdr == NEXTHDR_ICMP) {
				break; // process end
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

static void *
cpuport_pkt_function(void *ptr)
{
	struct list_head *list_node = NULL;
	struct cpuport_info_t *pktinfo = NULL;
	int num;

	util_set_thread_name("omci-pkt");
	omci_env_g.taskname[ENV_TASK_NO_PKT] = "PKT";
	omci_env_g.taskid[ENV_TASK_NO_PKT] = fwk_thread_gettid();
	omci_env_g.pthreadid[ENV_TASK_NO_PKT] = pthread_self();
	omci_env_g.taskloopcount[ENV_TASK_NO_PKT] = 0;
	omci_env_g.taskts[ENV_TASK_NO_PKT] = 0;
	omci_env_g.taskstate[ENV_TASK_NO_PKT] = 0; //idle

	while ( pkttask_loop_g )
	{
		omci_env_g.taskloopcount[ENV_TASK_NO_PKT]++;
		omci_env_g.taskts[ENV_TASK_NO_PKT] = util_get_uptime_sec();
		omci_env_g.taskstate[ENV_TASK_NO_PKT] = 0; //idle

		if (omci_env_g.task_pause_mask & (1<<ENV_TASK_NO_PKT)) {
			msleep(100);
			continue;
		}
		if ((num = fwk_msgq_select(pkt_qsetid_g, FWK_MSGQ_WAIT_INFINITE)) > 0)
		{
			int i = 0;
			omci_env_g.taskstate[ENV_TASK_NO_PKT] = 1; //running
			for( i = 0 ; i < num ; i ++ )
			{
				if ( fwk_msgq_set_isset(pkt_qsetid_g, pkt_recv_qid_g) )
				{
					if (fwk_msgq_recv(pkt_recv_qid_g, &list_node) < 0) {
						dbprintf_cpuport(LOG_ERR, "pkt_cmd_qid_g recv err\n");
					} else {
						pktinfo = list_entry(list_node, struct cpuport_info_t, node);
						pktinfo->thread_name = omci_env_g.taskname[ENV_TASK_NO_PKT];
						util_get_uptime(&pktinfo->process_time);
						if (omci_env_g.cpuport_history_enable)
							cpuport_history_add(pktinfo);	
						
						//cpuport_sniffer_dbprint_pkt(LOG_DEBUG, "pkt_rx", pktinfo->frame_ptr, pktinfo->frame_len);
						if (!pktinfo->rx_desc.bridge_port_me ) {
							if (!cpuport_util_rx_port_type_is_wan(pktinfo) && omci_env_g.localbridging_default) {
								cpuport_util_send_multi_lan_from_lan(pktinfo, "generic-to-other-lan", NULL);
							}
						} else {
							if( cpuport_util_rx_port_type_is_wan(pktinfo)) {
								// packet coming from WAN
								dbprintf_cpuport(LOG_NOTICE, "ds multicast packet\n");
								if (cpuport_pkt_is_rma(pktinfo))
								{
									if(cpuport_pkt_is_rma_forward(pktinfo))
										cpuport_util_send_multi_lan(pktinfo, "generic-to-lan", NULL, NULL);
									else
										dbprintf_cpuport(LOG_NOTICE, "ds multicast packet drop\n");
								} else {
									if (!cpuport_pkt_is_stp_bpdu(pktinfo)) //not bpdu
									{
										cpuport_util_send_multi_lan(pktinfo, "generic-to-lan", NULL, NULL);
									} else
										dbprintf_cpuport(LOG_NOTICE, "ds multicast packet drop\n");
								}
							} else {
								// packet coming from LAN
								if (cpuport_pkt_is_multicast(pktinfo)) //all multicast
								{
									dbprintf_cpuport(LOG_NOTICE, "us multicast packet\n");
									// calix, drop all upstream multicast except igmp/mld
									// first, implement calix rg intralan multicast, block lan-to-wan ipv4 multicast
									if (cpuport_pkt_is_rma(pktinfo))
									{
										if(cpuport_pkt_is_rma_forward(pktinfo)) {
											if (omci_env_g.localbridging_default) {
												cpuport_util_send_multi_lan_from_lan(pktinfo, "generic-to-other-lan", NULL);
											}
											if (pktinfo->rx_desc.bridge_port_me) {
												cpuport_util_send_multi_wan(pktinfo, "generic-to-wan", NULL, NULL);
											}
										} else {
											dbprintf_cpuport(LOG_NOTICE, "us rma packet drop\n");
										}
									} else if (omci_env_g.cpuport_im_enable == 1 && 
										cpuport_pkt_is_multicast_ipv4(pktinfo))
									{
										if(strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX)==0) {
											extern struct me_t * get_pptp_uni_by_logical_portid(int);
											struct me_t *pptp_me = get_pptp_uni_by_logical_portid(pktinfo->rx_desc.logical_port_id);
											struct me_t *unig_me = meinfo_me_get_by_meid(264, pptp_me->meid);
											if(unig_me) {
												unsigned short veip_meid = (unsigned short)meinfo_util_me_attr_data(unig_me, 4);
												if (veip_meid==0x302 && omci_env_g.cpuport_im_lan2lan_enable == 1) {
													//for calix rg intralan multicast forward
													cpuport_util_send_multi_lan_from_lan(pktinfo, "generic-to-other-lan", NULL);
												} else if (omci_env_g.localbridging_default) {
													//for calix fb intralan multicast forward
													cpuport_util_send_multi_lan_from_lan(pktinfo, "generic-to-other-lan", NULL);
												} //otherwise, did not forward to lan
											}
										}
										//did not forward to wan
									} else if (omci_env_g.cpuport_mcast_drop == 0 || //second, drop all upstream multicast
										cpuport_pkt_is_igmp_mld(pktinfo) ||
										cpuport_pkt_is_icmpv6(pktinfo) ||
										cpuport_pkt_is_dhcpv6(pktinfo) ||
										cpuport_pkt_is_ospfv3(pktinfo)) {
										if (omci_env_g.localbridging_default) {
											cpuport_util_send_multi_lan_from_lan(pktinfo, "generic-to-other-lan", NULL);
										}
										if (pktinfo->rx_desc.bridge_port_me) {
											cpuport_util_send_multi_wan(pktinfo, "generic-to-wan", NULL, NULL);
										}
									} else
										dbprintf_cpuport(LOG_NOTICE, "us multicast packet drop\n");
									//otherwise, DROP
								} else {
									//others
									if (omci_env_g.localbridging_default) {
										cpuport_util_send_multi_lan_from_lan(pktinfo, "generic-to-other-lan", NULL);
									}
									if (pktinfo->rx_desc.bridge_port_me) {
										int fwd_to_wan = 1;
										if (omci_env_g.cpuport_pkt_ssdp_fwd_wan == 0 && cpuport_pkt_is_ssdp(pktinfo))
											fwd_to_wan = 0;
										if (fwd_to_wan)
											cpuport_util_send_multi_wan(pktinfo, "generic-to-wan", NULL, NULL);
										else
											dbprintf_cpuport(LOG_NOTICE, "us packet drop\n");
									}
									//otherwise, DROP
								}							
							}
						}
						
						util_get_uptime(&pktinfo->forward_time);
						if (util_timeval_diff_usec(&pktinfo->forward_time, &pktinfo->recv_time) > 5*1E6)
							dbprintf_cpuport(LOG_ERR, "packet is processed for more than 5 sec, system busy?\n");
						if (omci_env_g.cpuport_history_enable)
							cpuport_history_sync(pktinfo);	
						
						free_safe(pktinfo->buf_ptr);
						free_safe(pktinfo);
					}
				}

				if ( fwk_msgq_set_isset(pkt_qsetid_g, pkt_cmd_qid_g) )
				{
					if (fwk_msgq_recv(pkt_cmd_qid_g, &list_node) < 0)
					{
						dbprintf_cpuport(LOG_ERR, "pkt_cmd_qid_g recv err\n");
					}else{
						struct pkt_cmd_msg_t *pkt_cmd_msg;
						pkt_cmd_msg = list_entry(list_node, struct pkt_cmd_msg_t, q_entry);

						if (pkt_cmd_msg->cmd == PKT_CMD_TERMINATE)
						{
							pkttask_loop_g = 0;
						}

						free_safe(pkt_cmd_msg);
					}
				}
			}
		}
	}
	
	omci_env_g.taskstate[ENV_TASK_NO_PKT] = 0; //idle
	omci_env_g.taskid[ENV_TASK_NO_PKT] = 0;

	dbprintf_cpuport(LOG_ERR, "task terminate\n");
	return 0;
}
int
cpuport_pkt_init(void)
{
	int ret = 0;
#ifdef OMCI_METAFILE_ENABLE
	struct metacfg_t kv_config;
	char *value_ptr;
#endif
	
	if ((pkt_recv_qid_g = fwk_msgq_alloc(FWK_MSGQ_ALLOC_ID_UNASSIGN, FWK_MSGQ_MAX_QUEUE_SIZE, "Q_PKT_RECV" ) ) <0 )
	{
		dbprintf_cpuport(LOG_ERR, "pkt recv queue alloc error(%d)\n",pkt_recv_qid_g );
		return (-1);
	}
	if ((pkt_cmd_qid_g = fwk_msgq_alloc(FWK_MSGQ_ALLOC_ID_UNASSIGN, 5, "Q_PKT_CMD") ) <0 )
	{
		dbprintf_cpuport(LOG_ERR, "pkt cmd queue alloc error(%d)\n",pkt_cmd_qid_g );
		return (-1);
	}
	if ((pkt_qsetid_g = fwk_msgq_set_alloc())< 0)
	{
		fwk_msgq_release( pkt_recv_qid_g );
		fwk_msgq_release( pkt_cmd_qid_g );
		dbprintf_cpuport(LOG_ERR, "queue set alloc error(%d)\n", pkt_qsetid_g);
		return -1;
	}

	if ((fwk_msgq_set_zero( pkt_qsetid_g ))< 0)
	{
		fwk_msgq_release( pkt_recv_qid_g );
		fwk_msgq_release( pkt_cmd_qid_g );
		dbprintf_cpuport(LOG_ERR, "queue set zero error(%d)\n", pkt_qsetid_g );
		return -1;
	}

	if ((ret = fwk_msgq_set_set( pkt_qsetid_g ,pkt_recv_qid_g ))<0 )
	{
		fwk_msgq_set_release( pkt_qsetid_g );
		fwk_msgq_release( pkt_recv_qid_g );
		fwk_msgq_release( pkt_cmd_qid_g );
		dbprintf_cpuport(LOG_ERR, "queue set set error(%d)\n", ret);
		return -1;
	}
	if ((ret = fwk_msgq_set_set( pkt_qsetid_g ,pkt_cmd_qid_g))<0 )
	{
		fwk_msgq_set_release( pkt_qsetid_g );
		fwk_msgq_release( pkt_recv_qid_g );
		fwk_msgq_release( pkt_cmd_qid_g );
		dbprintf_cpuport(LOG_ERR, "queue set set error(%d)\n", ret);
		return -1;
	}

#ifdef OMCI_METAFILE_ENABLE
	//read metafile key igmpLan2LanMulticastEnable to omci_env_g.cpuport_im_lan2lan_enable
	memset(&kv_config, 0x0, sizeof(struct metacfg_t));
	metacfg_adapter_config_init(&kv_config);
	if(metacfg_adapter_config_file_load_safe(&kv_config, "/etc/wwwctrl/metafile.dat") == 0)
	{
		if(strlen(value_ptr = metacfg_adapter_config_get_value(&kv_config, "imLan2LanEnable", 1)) > 0)
			omci_env_g.cpuport_im_lan2lan_enable = util_atoi(value_ptr);
	}	
	metacfg_adapter_config_release(&kv_config);
#endif
	
	return 0;
}

int
cpuport_pkt_shutdown(void)
{
	fwk_msgq_set_release( pkt_qsetid_g );
	fwk_msgq_release( pkt_recv_qid_g );
	fwk_msgq_release( pkt_cmd_qid_g );
	return 0;
}

int
cpuport_pkt_start(void)
{
	int ret;

	pkttask_thread_g.policy = FWK_THREAD_POLICY_CPUPORT_HANDLER;
	pkttask_thread_g.priority = FWK_THREAD_PRIO_CPUPORT_LOW;
	pkttask_thread_g.stack_size = 128*1024;
	pkttask_thread_g.join = 1;
	sprintf(pkttask_thread_g.name, "omci-PKT");
	pkttask_loop_g = 1;	//start loop
	if ((ret = fwk_thread_create(&pkttask_thread_g, cpuport_pkt_function, NULL)) < 0) {
		dbprintf_cpuport(LOG_ERR, "thread create error(%d)\n", ret);
		return (-1);
	}
	return 0;
}

int
cpuport_pkt_stop(void)
{
	int ret;

	pkttask_loop_g = 0;	//start loop

	struct pkt_cmd_msg_t *pkt_cmd_msg;
	pkt_cmd_msg = malloc_safe(sizeof (struct pkt_cmd_msg_t));
	pkt_cmd_msg->cmd = PKT_CMD_TERMINATE;
	INIT_LIST_NODE(&pkt_cmd_msg->q_entry);
	if (fwk_msgq_send(pkt_cmd_qid_g, &pkt_cmd_msg->q_entry) < 0)
	{
		free_safe(pkt_cmd_msg);
	}
	if ((ret = fwk_thread_join(&pkttask_thread_g)) < 0) {
		dbprintf_cpuport(LOG_ERR, "join error(%d)\n", ret);
	}
	return 0;
}
