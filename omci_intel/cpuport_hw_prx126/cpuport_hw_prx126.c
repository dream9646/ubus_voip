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
 * Module  : switch_hw
 * File    : cpuport_hw_prx126.c
 *
 ******************************************************************/

#include <sys/ioctl.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <net/if.h>
#include <errno.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <linux/ppp_defs.h>
#include <linux/if_pppox.h>
#include <sys/socket.h>
#include <string.h>


#include "list.h"
#include "util.h"
#include "util_inet.h"
#include "iphost.h"
#include "wanif.h"

#include "batchtab_cb.h"
#include "batchtab.h"
#include "cpuport.h"
#include "cpuport_util.h"
#include "cpuport_sniffer.h"
#include "cpuport_hw_prx126.h"
#include "switch_hw_prx126.h"
#include "switch.h"
#include "tm.h"
#include "batchtab_cb.h"
#include "batchtab.h"
//#include "mcast_pkt.h"
#include "mcast_router.h"
#include "intel_px126_adapter.h"
 // misc routines //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


// major call back function //////////////////////////////////////////////////////////////////////////////////

static int
cpuport_hw_prx126_init()
{
	/*intel_mcc_init by intel_omci_init*/
	return 0;
}

static int
cpuport_hw_prx126_shutdown(void)
{
	/*intel_mcc_exit by intel_omci_exit*/
	return 0;
}


static int
cpuport_hw_prx126_frame_send(struct cpuport_info_t *cpuport_info, unsigned char xmit_type)
{
	int ret = 0;
	ret = intel_adapter_cpuport_send(cpuport_info);
	return ret;
}

static int
cpuport_hw_prx126_frame_recv_is_available(struct timeval *timeout)
{
	/*always return 1 and wait on intel mcc layer*/
	return 1;
}

static int
cpuport_hw_prx126_rx_fill_pktinfo_tci(struct cpuport_info_t *cpuport_info)
{
	struct cpuport_pkthdr_t *e = (void*)cpuport_info->frame_ptr;
	unsigned short tpid0= ntohs(e->tag_u.two.tpid0);
	unsigned short tpid1= ntohs(e->tag_u.two.tpid1);
	unsigned char is_svlan = 0 , is_cvlan=0;
	// init tci with zero (so dscp_remark & internal_priority_remark default to 0)
	memset(&cpuport_info->tci, 0, sizeof(cpuport_info->tci));

	//0. svlan_tag_exist and ctagva are unreliable from hw
	//1. according hardware stag tpid value, to determine tag number
	//2. implement TLS (any tags + one tag)
	//3. hardware dependent issue, should be considered for porting
	
	if (tpid0==0x8100 || tpid0==0x88a8 || tpid0==0x9100)
	{
		if (tpid0 == omci_env_g.classf_hw_stag_tpid)
		{
			is_svlan = 1;
			if (tpid1 == 0x8100)
			{
				is_cvlan = 1;
			}
		} else if (tpid0 == 0x8100) {
			is_cvlan = 1;
		}
	}

	if (is_svlan && is_cvlan) {
			unsigned short tci0 = ntohs(e->tag_u.two.tci0);
			unsigned short tci1 = ntohs(e->tag_u.two.tci1);
			cpuport_info->tci.otci.tpid= tpid0;
			cpuport_info->tci.otci.priority= tci0 >> 13;
			cpuport_info->tci.otci.de= (tci0 >>12) & 1;
			cpuport_info->tci.otci.vid= tci0 &0xfff;
			cpuport_info->tci.itci.tpid= tpid1;
			cpuport_info->tci.itci.priority= tci1 >> 13;
			cpuport_info->tci.itci.de= (tci1 >>12) & 1;
			cpuport_info->tci.itci.vid= tci1 &0xfff;
			cpuport_info->tci.ethertype = ntohs(e->tag_u.two.ethertype);
			if (cpuport_info->tci.ethertype == 0x800)
				cpuport_info->tci.dscp = *(cpuport_info->frame_ptr + IFHWADDRLEN*2 + sizeof(struct cpuport_tag2_t) +1) >> 2;
			else
				cpuport_info->tci.dscp = 0;
	
	} else if (is_cvlan){
		unsigned short tci0 = ntohs(e->tag_u.one.tci0);
		cpuport_info->tci.itci.tpid = tpid0;
		cpuport_info->tci.itci.priority= tci0 >> 13;
		cpuport_info->tci.itci.de= (tci0 >>12) & 1;
		cpuport_info->tci.itci.vid= tci0 &0xfff;
		cpuport_info->tci.ethertype = tpid1;
		if (cpuport_info->tci.ethertype == 0x800)
			cpuport_info->tci.dscp = *(cpuport_info->frame_ptr + IFHWADDRLEN*2 + sizeof(struct cpuport_tag1_t) +1) >> 2;
		else
			cpuport_info->tci.dscp = 0;
	} else {
		cpuport_info->tci.otci.tpid= 0;
		cpuport_info->tci.otci.priority= 0;
		cpuport_info->tci.otci.de= 0;
		cpuport_info->tci.otci.vid= 0;
		cpuport_info->tci.itci.tpid= 0;
		cpuport_info->tci.itci.priority= 0;
		cpuport_info->tci.itci.de= 0;
		cpuport_info->tci.itci.vid= 0;
		cpuport_info->tci.ethertype = tpid0;
		if (cpuport_info->tci.ethertype == 0x800)
			cpuport_info->tci.dscp = *(cpuport_info->frame_ptr + IFHWADDRLEN*2 + sizeof(struct cpuport_tag0_t) +1) >> 2;
		else
			cpuport_info->tci.dscp = 0;
	}

	return 0;
}
static int
cpuport_hw_prx126_mcast_rx_fill_desc_bridge_port_me_by_lport_id(struct cpuport_info_t *cpuport_info)
{
	struct switch_port_info_t port_info;
	int i;

	// start with abnormal default: null bport me
	cpuport_info->rx_desc.bridge_port_me = NULL;
	cpuport_info->rx_desc.rx_wan_total = 0;

	// find bridge_port_me by logical port id -> port_info -> bport me
	if ((1<<cpuport_info->rx_desc.logical_port_id) & switch_get_uni_logical_portmask()) {	// uni
		// to be fixed,
		// port2port mapping should be done outside cpuport_hw_fvt2510, but this need heavy change on igmp_omci.c
		if (omci_env_g.port2port_enable)
			switch_get_port_info_by_logical_port_id(0, &port_info);
		else
			switch_get_port_info_by_logical_port_id(cpuport_info->rx_desc.logical_port_id, &port_info);
		cpuport_info->rx_desc.bridge_port_me = port_info.bridge_port_me;

	} else if ((1<<cpuport_info->rx_desc.logical_port_id) & switch_get_cpu_logical_portmask()) {   // cpu
		switch_get_port_info_by_logical_port_id(cpuport_info->rx_desc.logical_port_id, &port_info);
		cpuport_info->rx_desc.bridge_port_me = port_info.bridge_port_me;
	} else if (cpuport_info->rx_desc.logical_port_id == switch_get_wan_logical_portid()) {		    

		struct batchtab_cb_wan_t *t = batchtab_table_data_get("wan");
		if (t) {
           //  dbprintf_cpuport(LOG_ERR, "%s %d \n",__func__,__LINE__);
			unsigned int p = cpuport_util_get_pbit_from_tci(&cpuport_info->tci)%8;
			struct batchtab_cb_wan_node_t *w;
			// match both pbit, stream id
			list_for_each_entry (w, &t->wan_list[p], node) {
				struct me_t *bport_me = meinfo_me_get_by_meid(47, w->bport_meid);
               //  dbprintf_cpuport(LOG_ERR, "%s %d \n",__func__,__LINE__);
				if (bport_me != NULL) {
                //    dbprintf_cpuport(LOG_ERR, "%s %d \n",__func__,__LINE__);
					cpuport_info->rx_desc.bridge_port_me = bport_me;
					cpuport_info->rx_desc.rx_wan_me[cpuport_info->rx_desc.rx_wan_total] = bport_me;
					cpuport_info->rx_desc.rx_wan_total++;
					if (cpuport_info->rx_desc.rx_wan_total >=8)
						break;
				}
			}
			// try match only stream id if not found in above
			if (cpuport_info->rx_desc.bridge_port_me == NULL) {
				for (i=0; i < 8 ; i++) {
					list_for_each_entry (w, &t->wan_list[i], node) {
						struct me_t *bport_me = meinfo_me_get_by_meid(47, w->bport_meid);
                       //  dbprintf_cpuport(LOG_ERR, "%s %d \n",__func__,__LINE__);
						if ( bport_me != NULL) {
                        //     dbprintf_cpuport(LOG_ERR, "%s %d \n",__func__,__LINE__);
							cpuport_info->rx_desc.bridge_port_me = bport_me;
							cpuport_info->rx_desc.rx_wan_me[cpuport_info->rx_desc.rx_wan_total] = bport_me;
							cpuport_info->rx_desc.rx_wan_total++;
							//break;
						}
					}
					// traverse all possible wan bridge port me
					//if (cpuport_info->rx_desc.bridge_port_me)
					//	break;
				}
			}
			batchtab_table_data_put("wan");
			
		}
		
	} else {
		dbprintf_cpuport(LOG_ERR, "logical_port=%d is not used in uni/cpu/wan portmask\n",
			cpuport_info->rx_desc.logical_port_id);
		return -1;
	}
	return 0;
}
static int
cpuport_hw_prx126_rx_fill_pktinfo_mac_ethproto_nh(struct cpuport_info_t *cpuport_info)
{
	unsigned char *eth_proto_ptr;
	int i;

	for (i=0 ; i < 6; i++) {
		cpuport_info->dst_mac[i] = cpuport_info->frame_ptr[i];
		cpuport_info->src_mac[i] = cpuport_info->frame_ptr[i+6];
	}

	if (cpuport_info->tci.itci.tpid == 0) //untag
		eth_proto_ptr = cpuport_info->frame_ptr+CPUPORT_MAC_ADDR_LEN*2;
	else if (cpuport_info->tci.otci.tpid == 0) //one tag
		eth_proto_ptr = cpuport_info->frame_ptr+CPUPORT_MAC_ADDR_LEN*2+CPUPORT_VTAG_LEN;
	else //two tags
		eth_proto_ptr = cpuport_info->frame_ptr+CPUPORT_MAC_ADDR_LEN*2+CPUPORT_VTAG_LEN*2;

	//cpuport_info->eth_proto = ntohs(*(unsigned short*)eth_proto_ptr);
	cpuport_info->pppoe_proto = 0;
	cpuport_info->nh =  eth_proto_ptr + CPUPORT_ETHERTYE_LEN;
	if (cpuport_info->tci.ethertype == ETH_P_PPP_SES) {
		struct pppoe_hdr *pppoe = (struct pppoe_hdr *)cpuport_info->nh;
		cpuport_info->pppoe_proto = ntohs(pppoe->tag[0].tag_type);
		if (cpuport_info->pppoe_proto == PPP_IP || cpuport_info->pppoe_proto == PPP_IPV6)
			cpuport_info->nh = eth_proto_ptr + CPUPORT_ETHERTYE_LEN + 8;
	}

	return 0;
}

static int
cpuport_hw_prx126_frame_recv(struct cpuport_info_t *cpuport_info, unsigned short buf_len)
{
	int ret ;
	ret = intel_adapter_cpuport_recv(cpuport_info);
	if(ret == 1){
		cpuport_hw_prx126_rx_fill_pktinfo_tci(cpuport_info);
		if(cpuport_hw_prx126_mcast_rx_fill_desc_bridge_port_me_by_lport_id(cpuport_info) < 0 ){
			dbprintf_cpuport(LOG_ERR, "rx_fill_desc_bridge_port_me_by_lport_id fail(%d)\n",
			cpuport_info->rx_desc.logical_port_id);			
		}
		cpuport_hw_prx126_rx_fill_pktinfo_mac_ethproto_nh(cpuport_info);
	}
	return ret;
}

static int
cpuport_hw_prx126_frame_recv_abort_select(void)
{

 	return 0;
}

static int
cpuport_hw_prx126_frame_recv_abort_loop(void)
{

 	return 0;
}

static int
cpuport_hw_prx126_frame_recv_fill_after_load(struct cpuport_info_t *cpuport_info)
{
	

	return 0;
}

struct cpuport_hw_t cpuport_hw_fvt_g = {
	.init = cpuport_hw_prx126_init,
	.shutdown = cpuport_hw_prx126_shutdown,
	.frame_send = cpuport_hw_prx126_frame_send,
	.frame_recv_is_available = cpuport_hw_prx126_frame_recv_is_available,
	.frame_recv = cpuport_hw_prx126_frame_recv,
	.frame_recv_abort_select = cpuport_hw_prx126_frame_recv_abort_select,
	.frame_recv_abort_loop = cpuport_hw_prx126_frame_recv_abort_loop,
	.frame_recv_fill_after_load = cpuport_hw_prx126_frame_recv_fill_after_load,
};
