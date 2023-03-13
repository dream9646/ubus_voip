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
 * Module  : cpuport_pppoe
 * File    : cpuport_pppoe.c
 *
 ******************************************************************/

#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <linux/if_ether.h>
//#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <linux/ppp_defs.h>
//#include <linux/if_pppox.h>

#include "list.h"
#include "fwk_msgq.h"
#include "fwk_thread.h"
#include "util.h"
#include "env.h"
#include "cpuport.h"
#include "cpuport_extract_filter.h"
#include "cpuport_util.h"
#include "cpuport_util_send.h"
#include "cpuport_frame.h"
#include "cpuport_sniffer.h"
#include "cpuport_history.h"
#include "hwresource.h"

#include "switch.h"
#include "batchtab.h"
#include "batchtab_cb.h"
#include "cpuport_dhcp.h"
#include "cpuport_pppoe.h"

struct pppoe_tag_t {
        unsigned short tag_type;
        unsigned short tag_len;
        char tag_data[0];
};

struct pppoe_hdr_t {
        unsigned char ver : 4;
        unsigned char type : 4;
        unsigned char code;
        unsigned short sid;
        unsigned short length;
        struct pppoe_tag_t tag[0];
};


int pppoe_pkt_recv_qid_g = -1;
int pppoe_cmd_qid_g = -1;
int pppoe_qsetid_g = -1;
int pppoetask_loop_g = 0;

static struct fwk_thread_t pppoetask_thread_g;
static unsigned int pppoe_accept_total;
static unsigned int pppoe_drop_total;

int
cpuport_pppoe_count_reset(int fd)
{
	pppoe_accept_total=0 ;
	pppoe_drop_total=0 ;
	util_fdprintf(fd, "ACCEPT:[%d], DROP:[%d]\n", pppoe_accept_total, pppoe_drop_total);
	return 0;
}

int
cpuport_pppoe_count_get(int fd)
{
	util_fdprintf(fd, "ACCEPT:[%d], DROP:[%d]\n", pppoe_accept_total, pppoe_drop_total);
	return 0;
}

//Drop if PPPOE packet not PPPoE discovery
int 
cpuport_pppoe_pkt_filter( struct cpuport_info_t *pktinfo )
{
	unsigned short eth_proto=0;
	unsigned char *eth_proto_ptr;

	if (pktinfo->rx_desc.router_mode)
		return CPUPORT_UNMATCH;

	//dbprintf(LOG_ERR, "Packet was extract found\n\n");
	eth_proto_ptr=pktinfo->frame_ptr+12;
	eth_proto=ntohs(*(unsigned short *)eth_proto_ptr);

	if ((eth_proto == ETH_P_8021Q || eth_proto == 0x88a8 || eth_proto ==0x9100) ||	// outer tag
	    ((eth_proto&0xff00)==ETH_P_8021Q && eth_proto!=ETH_P_IPX) ) {		// 81xx but not 8137, for icplus 175d with source port in 81xx
	    	eth_proto_ptr+=4;
	    	eth_proto=ntohs(*(unsigned short *)eth_proto_ptr);
		if (eth_proto == ETH_P_8021Q || eth_proto == 0x88a8 || eth_proto ==0x9100) {	// inner tag
			eth_proto_ptr+=4;
		    	eth_proto=ntohs(*(unsigned short *)eth_proto_ptr);
		}
	}

        if (eth_proto==ETH_P_PPP_DISC) { // PPPoE discovery
		dbprintf_cpuport(LOG_INFO, "Packet is PPPoE discovery\n\n");
		return CPUPORT_MATCH;
        } else if (eth_proto==ETH_P_PPP_SES) {  // PPPoE session
		dbprintf_cpuport(LOG_INFO, "Packet is PPPoE session\n\n");
		return CPUPORT_UNMATCH;
	} else {
		return CPUPORT_UNMATCH;
	}
}

static int is_fiberhome_65330_cpuport_pppoe_enable()
{
	unsigned short classid, meid, data_classid;
	struct meinfo_t *miptr;
	struct me_t *me_ptr;
	struct attr_value_t cpuport_pppoe_enable;

	classid=65330;	//option 82,pppoe plus
	meid=0;

	if ((miptr=meinfo_util_miptr(classid))==NULL) {
		dbprintf_cpuport(LOG_WARNING, "classid=%u, not supported\n", classid);
		return 0;
	}
	if ((me_ptr = meinfo_me_get_by_meid(classid, meid)) == NULL) {
		dbprintf_cpuport(LOG_WARNING, "err=%d, classid=%u, meid=0x%x(%u)\n",
		MEINFO_ERROR_ME_NOT_FOUND, classid, meid, meid);
		return 0;
	}
	//double check if 65349 exist
	data_classid=65349;	//omci_me_large_info
	//data_classid=157;	//omci_me_large_info
	if ((miptr=meinfo_util_miptr(data_classid))==NULL) {
		dbprintf_cpuport(LOG_WARNING, "Can't found classid=%u\n", data_classid);
		return 0;
	}
	//format process
	meinfo_me_attr_get(me_ptr, 4, &cpuport_pppoe_enable);
	util_dbprintf(omci_env_g.debug_level, LOG_WARNING, 0, "format_length=%llu\n", cpuport_pppoe_enable.data);
	return cpuport_pppoe_enable.data;
}

static int is_ericsson_264_cpuport_pppoe_enable(unsigned char logical_port_id)
{
	struct meinfo_t *miptr;
	struct me_t *me_ptr;

	if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_ERICSSON)!=0)
		return 0;

	if ((miptr=meinfo_util_miptr(264))==NULL) {
		return 0;
	}
	list_for_each_entry(me_ptr, &miptr->me_instance_list, instance_node) {
		struct me_t *pptp_me = meinfo_me_get_by_meid(11, me_ptr->meid);
		struct me_t *ipptp_me = hwresource_public2private(pptp_me);
		unsigned char port_id = meinfo_util_me_attr_data(ipptp_me, 4);
		if(logical_port_id == port_id) {
			unsigned short meid = (unsigned short) meinfo_util_me_attr_data(me_ptr, 5);
			return (meinfo_me_get_by_meid(157, meid)) ? 1 : 0;
		}
	}
	
	return 0;
}

/* usually we don't need to expand pktinfo->ptr because: CPUPORT_BUF_SIZE is large (2030) */

static int
cpuport_pppoe_packet_add(struct cpuport_info_t *pktinfo) {
	unsigned short eth_proto=0;
	unsigned char *eth_proto_ptr, *trace_ptr, *cpuport_pppoe_str, vendor_id[]={0x00,0x00,0x0d,0xe9};
	unsigned char pppoe_str[312];
	struct pppoe_hdr_t *pppoe_hdr;
	struct pppoe_tag_t *pppoe_tag;
	int len;
	unsigned char logical_port_id = pktinfo->rx_desc.logical_port_id;

	cvlan_for_65330=0;
	eth_proto_ptr=pktinfo->frame_ptr+12;
	eth_proto=ntohs(*(unsigned short *)eth_proto_ptr);

	if ((eth_proto == ETH_P_8021Q || eth_proto == 0x88a8 || eth_proto ==0x9100) ||	// outer tag
	    ((eth_proto&0xff00)==ETH_P_8021Q && eth_proto!=ETH_P_IPX) ) {		// 81xx but not 8137, for icplus 175d with source port in 81xx
		unsigned short tci=ntohs(*(unsigned short *)(eth_proto_ptr+2));
	    	eth_proto_ptr+=4;
	    	eth_proto=ntohs(*(unsigned short *)eth_proto_ptr);
		if (eth_proto == ETH_P_8021Q || eth_proto == 0x88a8 || eth_proto ==0x9100) {	// inner tag
			unsigned short tci2=ntohs(*(unsigned short *)(eth_proto_ptr+2));
			eth_proto_ptr+=4;
		    	eth_proto=ntohs(*(unsigned short *)eth_proto_ptr);
			dbprintf_cpuport(LOG_INFO, "ovlan(s) 0x%x(%d) p%d, ivlan(c) 0x%x(%d) p%d\n", 
						tci & 0xfff, tci & 0xfff, (tci&0xe000)>>13, 
						tci2& 0xfff, tci2& 0xfff, (tci2&0xe000)>>13);
			cvlan_for_65330=(tci2 & 0xfff);
		} else {
			dbprintf_cpuport(LOG_INFO, "cvlan 0x%x(%d) p%d\n", tci & 0xfff, tci & 0xfff, (tci&0xe000)>>13);
			cvlan_for_65330=(tci & 0xfff);
		}
	}

	pppoe_hdr=(struct pppoe_hdr_t *)(eth_proto_ptr+2);
	if (eth_proto==ETH_P_PPP_DISC) { // PPPoE discovery
		
		util_dbprintf(omci_env_g.debug_level, LOG_DEBUG, 0, "Packet is PPPoE discovery=0x%X, frame_len=%d\n\n", eth_proto, pktinfo->frame_len);
		util_dbprintf(omci_env_g.debug_level, LOG_DEBUG, 0, "ver=%d, type=%d, code=0x%X, sessionid=0x%X, length=%d\n"
				,pppoe_hdr->ver, pppoe_hdr->type, pppoe_hdr->code, ntohs(pppoe_hdr->sid), ntohs(pppoe_hdr->length));

#if 0	//Dump all TLV for debug
		int offset=0;
		trace_ptr=(eth_proto_ptr+2);
		trace_ptr+=sizeof(struct pppoe_hdr_t);
		pppoe_tag=(struct pppoe_tag_t *)pppoe_hdr->tag;

		while(offset < ntohs(pppoe_hdr->length)) {
			util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0, "type=0x%X, len=0x%X\n",
						ntohs(pppoe_tag->tag_type), ntohs(pppoe_tag->tag_len));
			offset+=4;//type 2byte, len 2byte
			offset+=ntohs(pppoe_tag->tag_len);
			util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0, "offset=%d(0x%X)\n", offset, offset);
			trace_ptr+=sizeof(struct pppoe_tag_t);
			trace_ptr+=ntohs(pppoe_tag->tag_len);
			pppoe_tag=(struct pppoe_tag_t *)trace_ptr;
		}
#else
		//move to the end of original TLV
		trace_ptr=(eth_proto_ptr+2);
		trace_ptr+=sizeof(struct pppoe_hdr_t);
		trace_ptr+=ntohs(pppoe_hdr->length);
		//prepare to modify payload
		pppoe_tag=(struct pppoe_tag_t *)trace_ptr;
#endif
		//start modify payload
		pppoe_tag->tag_type=htons(0x105);	//Vendor-Specific
		if(is_fiberhome_65330_cpuport_pppoe_enable()) {
			//get data from me, data is the same as dhcp option 82
			if( cpuport_dhcp_generate_data_from_fiberhome_me(&cpuport_pppoe_str, pktinfo, &len) !=0 ) {
				dbprintf_cpuport(LOG_ERR, "Can't generate pppoe plus data\n");
				return -1;
			}
		} else if(is_ericsson_264_cpuport_pppoe_enable(logical_port_id)) {
			//get data from me, data is the same as dhcp option 82
			if( cpuport_dhcp_generate_data_from_ericsson_me(&cpuport_pppoe_str, pktinfo, &len) !=0 ) {
				dbprintf_cpuport(LOG_ERR, "Can't generate pppoe plus data\n");
				return -1;
			}
		} else if (omci_env_g.cpuport_pppoe_cust_tag[0]){
			int i;
			memset(pppoe_str, 0, sizeof(pppoe_str));
			cpuport_pppoe_str = pppoe_str;
			len = omci_env_g.cpuport_pppoe_cust_tag[0];
			for(i=0; i<len; i++)
				pppoe_str[i] = omci_env_g.cpuport_pppoe_cust_tag[i+1];
		}
		
		pppoe_tag->tag_len=htons(len+8);	//vendorid(4byte)+subopt1(2byte)+circuitid(data len)+subopt2(2byte)
		trace_ptr+=sizeof(struct pppoe_tag_t);

		//vendorid(4 byte)
		memcpy(trace_ptr, vendor_id, 4); 
		trace_ptr+=4;

		//subopt1(2 byte)
		trace_ptr[0]=0x1;
		trace_ptr[1]=len;
		trace_ptr+=2;

		memcpy(trace_ptr, cpuport_pppoe_str, len); 
		trace_ptr+=len;

		//subopt2(2 byte)
		trace_ptr[0]=0x2;
		trace_ptr[1]=0x0;

		pppoe_hdr->length+= htons(sizeof(struct pppoe_tag_t));
		pppoe_hdr->length+= htons(len+8);

		pktinfo->frame_len += sizeof(struct pppoe_tag_t);
		pktinfo->frame_len += (len+8);
	} else {
		return -1;
	}
	return 0;
}

static int
cpuport_pppoe_packet_del(struct cpuport_info_t *pktinfo) {
	unsigned short eth_proto=0;
	unsigned char *eth_proto_ptr, *trace_ptr;
	struct pppoe_hdr_t *pppoe_hdr;
	struct pppoe_tag_t *pppoe_tag;
	unsigned short pppoe_payload_len, remove_len=0, remain_len, offset=0;

	eth_proto_ptr=pktinfo->frame_ptr+12;
	eth_proto=ntohs(*(unsigned short *)eth_proto_ptr);

	if ((eth_proto == ETH_P_8021Q || eth_proto == 0x88a8 || eth_proto ==0x9100) ||	// outer tag
	    ((eth_proto&0xff00)==ETH_P_8021Q && eth_proto!=ETH_P_IPX) ) {		// 81xx but not 8137, for icplus 175d with source port in 81xx
	    	eth_proto_ptr+=4;
	    	eth_proto=ntohs(*(unsigned short *)eth_proto_ptr);
		if (eth_proto == ETH_P_8021Q || eth_proto == 0x88a8 || eth_proto ==0x9100) {	// inner tag
			eth_proto_ptr+=4;
		    	eth_proto=ntohs(*(unsigned short *)eth_proto_ptr);
		}
	}

	pppoe_hdr=(struct pppoe_hdr_t *)(eth_proto_ptr+2);
	if (eth_proto==ETH_P_PPP_DISC) { // PPPoE discovery
		
		util_dbprintf(omci_env_g.debug_level, LOG_DEBUG, 0, "Packet is PPPoE discovery=0x%X, frame_len=%d\n\n", eth_proto, pktinfo->frame_len);
		util_dbprintf(omci_env_g.debug_level, LOG_DEBUG, 0, "ver=%d, type=%d, code=0x%X, sessionid=0x%X, length=%d\n"
				,pppoe_hdr->ver, pppoe_hdr->type, pppoe_hdr->code, ntohs(pppoe_hdr->sid), ntohs(pppoe_hdr->length));

		//start modify payload

		trace_ptr=(eth_proto_ptr+2);
		trace_ptr+=sizeof(struct pppoe_hdr_t);
		pppoe_tag=(struct pppoe_tag_t *)pppoe_hdr->tag;

		while(offset < ntohs(pppoe_hdr->length)) {
			//util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0, "type=0x%X, len=0x%X\n",
			//			ntohs(pppoe_tag->tag_type), ntohs(pppoe_tag->tag_len));

			if(ntohs(pppoe_tag->tag_type) == 0x105) {
				pppoe_payload_len=ntohs(pppoe_hdr->length);
				remove_len=4+ntohs(pppoe_tag->tag_len);
				remain_len=pppoe_payload_len-offset-remove_len;
				memset(trace_ptr, 0, remove_len);
				memmove(trace_ptr, trace_ptr+remove_len, remain_len);

				pppoe_hdr->length-=htons(remove_len);
				pktinfo->frame_len-=remove_len;
				break;
			}

			offset+=4;//type 2byte, len 2byte
			offset+=ntohs(pppoe_tag->tag_len);
			//util_dbprintf(omci_env_g.debug_level, LOG_DEBUG, 0, "offset=%d(0x%X)\n", offset, offset);

			trace_ptr+=sizeof(struct pppoe_tag_t);
			trace_ptr+=ntohs(pppoe_tag->tag_len);
			pppoe_tag=(struct pppoe_tag_t *)trace_ptr;
		}

	} else {
		return -1;
	}
	return 0;
}

/* packet from lan side send to wan */
static int
pppoe_send_wan( struct cpuport_info_t *pktinfo)
{
	unsigned char logical_port_id = pktinfo->rx_desc.logical_port_id;
	if (is_fiberhome_65330_cpuport_pppoe_enable() || is_ericsson_264_cpuport_pppoe_enable(logical_port_id) || omci_env_g.cpuport_pppoe_cust_tag[0])
		cpuport_pppoe_packet_add(pktinfo);
	cpuport_util_send_multi_wan(pktinfo, "pppoe-to-wan", NULL, NULL);
	return 0;
}

/* packet from wan side send to all uni port and cpu */
static int
pppoe_send_lan( struct cpuport_info_t *pktinfo)
{
	unsigned char logical_port_id = pktinfo->rx_desc.logical_port_id;
	if (is_fiberhome_65330_cpuport_pppoe_enable() || is_ericsson_264_cpuport_pppoe_enable(logical_port_id) || omci_env_g.cpuport_pppoe_cust_tag[0])
		cpuport_pppoe_packet_del(pktinfo);
	cpuport_util_send_multi_lan(pktinfo, "pppoe-to-lan", NULL, NULL);
	return 0;
}

/* packet from lan side send to other uni ports and cpu */
static int
pppoe_send_other_lan( struct cpuport_info_t *pktinfo)
{
	return cpuport_util_send_multi_lan_from_lan(pktinfo, "pppoe-to-other-lan", NULL);
}

static void *
cpuport_pppoe_function(void *ptr)
{
	struct list_head *list_node = NULL;
	struct cpuport_info_t *pktinfo = NULL;
	int num;

	util_set_thread_name("omci-pppoe");
	omci_env_g.taskname[ENV_TASK_NO_PPPOE] = "PPPOE";
	omci_env_g.taskid[ENV_TASK_NO_PPPOE] = fwk_thread_gettid();
	omci_env_g.pthreadid[ENV_TASK_NO_PPPOE] = pthread_self();
	omci_env_g.taskloopcount[ENV_TASK_NO_PPPOE] = 0;
	omci_env_g.taskts[ENV_TASK_NO_PPPOE] = 0;
	omci_env_g.taskstate[ENV_TASK_NO_PPPOE] = 0; //idle

	while ( pppoetask_loop_g )
	{
		omci_env_g.taskloopcount[ENV_TASK_NO_PPPOE]++;
		omci_env_g.taskts[ENV_TASK_NO_PPPOE] = util_get_uptime_sec();
		omci_env_g.taskstate[ENV_TASK_NO_PPPOE] = 0; //idle

		if (omci_env_g.task_pause_mask & (1<<ENV_TASK_NO_PPPOE)) {
			msleep(100);
			continue;
		}
		if ((num = fwk_msgq_select(pppoe_qsetid_g, FWK_MSGQ_WAIT_INFINITE)) > 0) {
			int i = 0;
			omci_env_g.taskstate[ENV_TASK_NO_PPPOE] = 1; //running
			for( i = 0 ; i < num ; i ++ ) {
				if ( fwk_msgq_set_isset(pppoe_qsetid_g, pppoe_pkt_recv_qid_g) ) {
					if (fwk_msgq_recv(pppoe_pkt_recv_qid_g, &list_node) < 0) {
						dbprintf_cpuport(LOG_ERR, "pppoe_pkt_recv_qid_g recv err\n");
					} else {
						pktinfo = list_entry(list_node, struct cpuport_info_t, node);
						pktinfo->thread_name = omci_env_g.taskname[ENV_TASK_NO_PPPOE];
						util_get_uptime(&pktinfo->process_time);
						if (omci_env_g.cpuport_history_enable)
							cpuport_history_add(pktinfo);	

						//cpuport_sniffer_dbprint_pkt(LOG_DEBUG, "pppoe_rx", pktinfo->frame_ptr, pktinfo->frame_len);
						if (!pktinfo->rx_desc.bridge_port_me ) {
							if (!cpuport_util_rx_port_type_is_wan(pktinfo) && omci_env_g.localbridging_default) {
								dbprintf_cpuport(LOG_DEBUG, "packet pppoe come from lan side port=%d and send to other lan\n", pktinfo->rx_desc.logical_port_id);
								pppoe_send_other_lan(pktinfo);
							}
						} else {
							if( cpuport_util_rx_port_type_is_wan(pktinfo)) {
								dbprintf_cpuport(LOG_DEBUG, "packet pppoe come from wan side and send to lan\n");
								pppoe_send_lan(pktinfo);
							} else {
								dbprintf_cpuport(LOG_DEBUG, "packet pppoe come from lan side port=%d and send to wan\n", pktinfo->rx_desc.logical_port_id);
								if (omci_env_g.localbridging_default) {
									dbprintf_cpuport(LOG_DEBUG, "packet pppoe come from lan side port=%d and send to other lan\n", pktinfo->rx_desc.logical_port_id);
									pppoe_send_other_lan(pktinfo);
								}
								pppoe_send_wan(pktinfo);
							}
							pppoe_accept_total++ ;
							//dbprintf(LOG_WARNING, "count:[%d]\n", pppoe_accept_total);
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

				if ( fwk_msgq_set_isset(pppoe_qsetid_g, pppoe_cmd_qid_g) ) {

					if (fwk_msgq_recv(pppoe_cmd_qid_g, &list_node) < 0) {
						dbprintf_cpuport(LOG_ERR, "pppoe_cmd_qid_g recv err\n");

					} else {
						struct pppoe_cmd_msg_t *pppoe_cmd_msg;
						pppoe_cmd_msg = list_entry(list_node, struct pppoe_cmd_msg_t, q_entry);

						if (pppoe_cmd_msg->cmd == PPPOE_CMD_LIMIT_CHANGE) {
						}

						if (pppoe_cmd_msg->cmd == PPPOE_CMD_TERMINATE) {
							pppoetask_loop_g = 0;
						}
						free_safe(pppoe_cmd_msg);
					}
				}
			}
		}
	}
	omci_env_g.taskstate[ENV_TASK_NO_PPPOE] = 0; //idle
	omci_env_g.taskid[ENV_TASK_NO_PPPOE] = 0;

	dbprintf_cpuport(LOG_INFO, "task terminate\n");
	return 0;
}

int
cpuport_pppoe_init(void)
{
	int ret = 0;
	if ((pppoe_pkt_recv_qid_g = fwk_msgq_alloc(FWK_MSGQ_ALLOC_ID_UNASSIGN, FWK_MSGQ_MAX_QUEUE_SIZE, "Q_PPPOE_RECV" ) ) <0 )
	{
		dbprintf_cpuport(LOG_ERR, "pppoe recv queue alloc error(%d)\n",pppoe_pkt_recv_qid_g );
		return (-1);
	}
	if ((pppoe_cmd_qid_g = fwk_msgq_alloc(FWK_MSGQ_ALLOC_ID_UNASSIGN, 5, "Q_PPPOE_CMD") ) <0 )
	{
		dbprintf_cpuport(LOG_ERR, "pppoe cmd queue alloc error(%d)\n",pppoe_cmd_qid_g );
		return (-1);
	}
	if ((pppoe_qsetid_g = fwk_msgq_set_alloc())< 0)
	{
		fwk_msgq_release( pppoe_pkt_recv_qid_g );
		fwk_msgq_release( pppoe_cmd_qid_g );
		dbprintf_cpuport(LOG_ERR, "queue set alloc error(%d)\n", pppoe_qsetid_g);
		return -1;
	}

	if ((fwk_msgq_set_zero( pppoe_qsetid_g ))< 0)
	{
		fwk_msgq_release( pppoe_pkt_recv_qid_g );
		fwk_msgq_release( pppoe_cmd_qid_g );
		dbprintf_cpuport(LOG_ERR, "queue set zero error(%d)\n", pppoe_qsetid_g );
		return -1;
	}

	if ((ret = fwk_msgq_set_set( pppoe_qsetid_g ,pppoe_pkt_recv_qid_g ))<0 )
	{
		fwk_msgq_set_release( pppoe_qsetid_g );
		fwk_msgq_release( pppoe_pkt_recv_qid_g );
		fwk_msgq_release( pppoe_cmd_qid_g );
		dbprintf_cpuport(LOG_ERR, "queue set set error(%d)\n", ret);
		return -1;
	}
	if ((ret = fwk_msgq_set_set( pppoe_qsetid_g ,pppoe_cmd_qid_g))<0 )
	{
		fwk_msgq_set_release( pppoe_qsetid_g );
		fwk_msgq_release( pppoe_pkt_recv_qid_g );
		fwk_msgq_release( pppoe_cmd_qid_g );
		dbprintf_cpuport(LOG_ERR, "queue set set error(%d)\n", ret);
		return -1;
	}
	return 0;
}

int
cpuport_pppoe_shutdown(void)
{
	fwk_msgq_set_release( pppoe_qsetid_g );
	fwk_msgq_release( pppoe_pkt_recv_qid_g );
	fwk_msgq_release( pppoe_cmd_qid_g );
	return 0;
}


int
cpuport_pppoe_start(void)
{
	int ret;
#ifndef X86	//for build x86 version
	//only implement for core swith now
	switch_hw_g.pppoe_extract_enable_set(ENABLED);
#endif

	if ( cpuport_extract_filter_register(CPUPORT_EXTRACT_PPPOE, "pppoe", cpuport_pppoe_pkt_filter, pppoe_pkt_recv_qid_g) ) {
		dbprintf_cpuport(LOG_ERR, "cpuport_extract_filter_register error(%d)\n");
		return -1;
	}

	pppoetask_thread_g.policy = FWK_THREAD_POLICY_CPUPORT_HANDLER;
	pppoetask_thread_g.priority = FWK_THREAD_PRIO_CPUPORT_MED;
	pppoetask_thread_g.stack_size = 128*1024;
	pppoetask_thread_g.join = 1;
	sprintf(pppoetask_thread_g.name, "omci-PPPOE");
	pppoetask_loop_g = 1;	//start loop
	if ((ret = fwk_thread_create(&pppoetask_thread_g, cpuport_pppoe_function, NULL)) < 0) {
		dbprintf_cpuport(LOG_ERR, "thread create error(%d)\n", ret);
		return (-1);
	}

	return 0;
}

int
cpuport_pppoe_stop(void)
{
	int ret;

	if (cpuport_extract_filter_unregister(CPUPORT_EXTRACT_PPPOE))
		return -1;

	pppoetask_loop_g = 0;	//start loop
	struct pppoe_cmd_msg_t *pppoe_cmd_msg;
	pppoe_cmd_msg = malloc_safe(sizeof (struct pppoe_cmd_msg_t));
	pppoe_cmd_msg->cmd = PPPOE_CMD_TERMINATE;
	INIT_LIST_NODE(&pppoe_cmd_msg->q_entry);
	if (fwk_msgq_send(pppoe_cmd_qid_g, &pppoe_cmd_msg->q_entry) < 0)
	{
		free_safe(pppoe_cmd_msg);
	}
	if ((ret = fwk_thread_join(&pppoetask_thread_g)) < 0) {
		dbprintf_cpuport(LOG_ERR, "join error(%d)\n", ret);
	}
	return 0;
}
