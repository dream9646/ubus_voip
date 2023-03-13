/*************************************
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
 * Purpose : 5VT TRAF MANAGER
 * Module  : switch_hw
 * File    : cpuport_hw_fvt2510.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include <sys/syslog.h>
#include <sys/ioctl.h>
#include <sys/types.h>

#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>

#include "cpuport_hw_fvt2510.h"

int debug_level_cpuport = LOG_ERR;
static int sock_extract = -1;
static struct sockaddr_ll iftrap_addr_ll;
static fd_set readfds;

// hw desc print ///////////////////////////////////////////////////////////////////////////////////////////////////////

// tx_info, rx_info: the hw descriptor of per packet, defined by chip
// tx_desc, rx_desc: the sw descriptor of per packet, defined by cpuport soft module

static int
rx_info_print(struct rx_info *rx_info)
{
	util_dbprintf(debug_level_cpuport, LOG_NOTICE, 
		"rx_info\n"
		"opts1\town:%d eor:%d fs:%d ls:%d crcerr:%d ipv4csf:%d l4csf:%d rcdf:%d ipfrag:%d pppoetag:%d\n"
       			"\trwt:%d pkttype:%d l3routing:%d origformat:%d pctrl:%d data_length:%d\n",
			rx_info->opts1.bit.own, rx_info->opts1.bit.eor, rx_info->opts1.bit.fs, rx_info->opts1.bit.ls,
			rx_info->opts1.bit.crcerr, rx_info->opts1.bit.ipv4csf, rx_info->opts1.bit.l4csf,
			rx_info->opts1.bit.rcdf, rx_info->opts1.bit.ipfrag, rx_info->opts1.bit.pppoetag,
			rx_info->opts1.bit.rwt, rx_info->opts1.bit.pkttype, rx_info->opts1.bit.l3routing,
			rx_info->opts1.bit.origformat, rx_info->opts1.bit.pctrl,rx_info->opts1.bit.data_length);

	util_dbprintf(debug_level_cpuport, LOG_NOTICE, 
		"opts2\tcputag:%d ptp_in_cpu_tag_exist:%d svlan_tag_exist:%d rsvd_2:%d pon_stream_id:%d rsvd_1:%d ctagva:%d cvlan_tag:0x%04x\n",
			rx_info->opts2.bit.cputag, rx_info->opts2.bit.ptp_in_cpu_tag_exist, 
			rx_info->opts2.bit.svlan_tag_exist, rx_info->opts2.bit.rsvd_2,
			rx_info->opts2.bit.pon_stream_id, rx_info->opts2.bit.rsvd_1, 
			rx_info->opts2.bit.ctagva, rx_info->opts2.bit.cvlan_tag);
	util_dbprintf(debug_level_cpuport, LOG_NOTICE, 
		"opts3\tsrc_port_num:%d dst_port_mask:%d reason:%d internal_priority:%d ext_port_ttl_1:%d rsvd:%d\n",
			rx_info->opts3.bit.src_port_num, rx_info->opts3.bit.dst_port_mask, rx_info->opts3.bit.reason, 
			rx_info->opts3.bit.internal_priority, rx_info->opts3.bit.ext_port_ttl_1, 
			rx_info->opts3.bit.rsvd);
	return 0;
}

static int
tx_info_print(struct tx_info *tx_info)
{
	util_dbprintf(debug_level_cpuport, LOG_NOTICE, 
		"tx_info\n"
		"opts1\town:%d eor:%d fs:%d ls:%d ipcs:%d l4cs:%d keep:%d blu:%d crc:%d vsel:%d dislrn:%d\n"
			"\tcputag_ipcs:%d cputag_l4cs:%d cputag_psel:%d data_length:%d\n", 
			tx_info->opts1.bit.own, tx_info->opts1.bit.eor, tx_info->opts1.bit.fs, tx_info->opts1.bit.ls, tx_info->opts1.bit.ipcs, 
			tx_info->opts1.bit.l4cs, tx_info->opts1.bit.keep, tx_info->opts1.bit.blu, tx_info->opts1.bit.crc, tx_info->opts1.bit.vsel, 
			tx_info->opts1.bit.dislrn, tx_info->opts1.bit.cputag_ipcs, tx_info->opts1.bit.cputag_l4cs, tx_info->opts1.bit.cputag_psel, 
			tx_info->opts1.bit.data_length);
	util_dbprintf(debug_level_cpuport, LOG_NOTICE, 
		"opts2\tcputag:%d aspri:%d cputag_pri:%d tx_vlan_act:%d tx_pppoe_act:%d tx_pppoe_idx:%d efid:%d enhance_fid:%d vidl:0x%x prio:%d cfi:%d vidh:0x%x\n", 
			tx_info->opts2.bit.cputag, tx_info->opts2.bit.aspri, tx_info->opts2.bit.cputag_pri, tx_info->opts2.bit.tx_vlan_action, 
			tx_info->opts2.bit.tx_pppoe_action, tx_info->opts2.bit.tx_pppoe_idx, tx_info->opts2.bit.efid, tx_info->opts2.bit.enhance_fid, 
			tx_info->opts2.bit.vidl, tx_info->opts2.bit.prio, tx_info->opts2.bit.cfi, tx_info->opts2.bit.vidh);
	util_dbprintf(debug_level_cpuport, LOG_NOTICE, 
		"opts3\textspa:%d tx_portmask:0x%x tx_dst_stream_id:%d l34_keep:%d ptp:%d\n", 
			tx_info->opts3.bit.extspa, tx_info->opts3.bit.tx_portmask, tx_info->opts3.bit.tx_dst_stream_id, tx_info->opts3.bit.l34_keep, 
			tx_info->opts3.bit.ptp);
	return 0;
}

// rx desc tci related /////////////////////////////////////////////////////////////////////////////////////////////////////////

static int
rx_desc_fill_tci(struct cpuport_info_t *cpuport_info, struct rx_info *rxinfo) 
{
	struct cpuport_pkthdr_t *e = (void*)cpuport_info->frame_ptr;
	unsigned short tpid0= ntohs(e->tag_u.two.tpid0);
	unsigned short tpid1= ntohs(e->tag_u.two.tpid1);

	// vtag not detected by hw? workaround it....
	if (rxinfo->opts2.bit.svlan_tag_exist == 0  && rxinfo->opts2.bit.ctagva == 0) {	
		if (tpid0==0x8100 || tpid0==0x88a8 || tpid0==0x9100) {
			if (tpid1==0x8100 || tpid1==0x88a8 || tpid1==0x9100) {
				rxinfo->opts2.bit.svlan_tag_exist = 1;
				rxinfo->opts2.bit.ctagva = 1;
			} else {
				if (tpid0==0x8100) {
					rxinfo->opts2.bit.ctagva = 1;
				} else {
					rxinfo->opts2.bit.svlan_tag_exist = 1;
				}
			}
		}
	}
	dbprintf_cpuport(LOG_INFO, "ctagva=%d, svlan_tag_exist=%d\n", rxinfo->opts2.bit.ctagva, rxinfo->opts2.bit.svlan_tag_exist);
	
	if (rxinfo->opts2.bit.svlan_tag_exist && rxinfo->opts2.bit.ctagva) {
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
		
	} else if (rxinfo->opts2.bit.svlan_tag_exist || rxinfo->opts2.bit.ctagva){
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

//rx desc related //////////////////////////////////////////////////////////////////////////////////////////////////////////////

//remove struct rx_info in front of packet
static int
rx_desc_fill_for_sock_extract(struct cpuport_info_t *cpuport_info)
{
	struct rx_info *rxinfo=NULL;
	int i = 0;

	rxinfo = (struct rx_info *)(cpuport_info->frame_ptr + cpuport_info->frame_len - sizeof(struct rx_info));
	rx_desc_fill_tci(cpuport_info, rxinfo);

	rx_info_print(rxinfo);
	// we do not call cpuport_sniffer_dbprint_pkt() here, it is already done in cpuport_recv_func()

	// start with abnormal default: ext5 & null bport me
	cpuport_info->rx_desc.logical_port_id = 11;

	if (rxinfo->opts3.bit.src_port_num < 4) { 
		cpuport_info->rx_desc.logical_port_id = rxinfo->opts3.bit.src_port_num;
	} else if (rxinfo->opts3.bit.src_port_num == 4) {
		cpuport_info->rx_desc.logical_port_id = 5;
	} else if (rxinfo->opts3.bit.src_port_num == 5) {
		cpuport_info->rx_desc.logical_port_id = 4;
	} else if (rxinfo->opts3.bit.src_port_num > 6) {
		cpuport_info->rx_desc.logical_port_id = rxinfo->opts3.bit.src_port_num; 
	} else if (rxinfo->opts3.bit.src_port_num == 6) {		// tries to utilize ext port num hidden in dst_portmask by rg acl									
		if (rxinfo->opts3.bit.dst_port_mask==0x8) { 		//from EXT0 (Using ACL modify EXT_SPA to DST_PMSK)
			cpuport_info->rx_desc.logical_port_id = 7;
		} else if (rxinfo->opts3.bit.dst_port_mask==0x10) { 	//from EXT1 (Using ACL modify EXT_SPA to DST_PMSK)
			cpuport_info->rx_desc.logical_port_id = 8;
		}
	}
	cpuport_info->frame_len -= sizeof(struct rx_info);
	
	for (i=0 ; i < 6; i++) {
		cpuport_info->dst_mac[i] = cpuport_info->frame_ptr[i];
		cpuport_info->src_mac[i] = cpuport_info->frame_ptr[i+6];
	}

	if (cpuport_info->tci.itci.tpid == 0) //untag
		cpuport_info->ip_hdr = (struct iphdr *)(cpuport_info->frame_ptr+CPUPORT_MAC_ADDR_LEN*2+CPUPORT_ETHERTYE_LEN); 
	else if (cpuport_info->tci.otci.tpid == 0) //one tag
		cpuport_info->ip_hdr = (struct iphdr *)(cpuport_info->frame_ptr+CPUPORT_MAC_ADDR_LEN*2+CPUPORT_VTAG_LEN+CPUPORT_ETHERTYE_LEN); 
	else //two tags
		cpuport_info->ip_hdr = (struct iphdr *)(cpuport_info->frame_ptr+CPUPORT_MAC_ADDR_LEN*2+CPUPORT_VTAG_LEN*2+CPUPORT_ETHERTYE_LEN); 

	return 0;
}

// tx info related ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

static int
tx_info_prepare(struct cpuport_info_t *cpuport_info, int type, struct tx_info *txinfo)
{
	txinfo->opts1.dw = 0;
	txinfo->opts2.dw = 0;
	txinfo->opts3.dw = 0;

	txinfo->opts1.bit.keep = 1;
	txinfo->opts1.bit.crc = 1;
	txinfo->opts1.bit.data_length = cpuport_info->frame_len;
	switch (type) {
		case CPUPORT_XMIT_RAW: //Packets trap from switch and re-send to switch. These packets are not learned by L2 table 
			txinfo->opts1.bit.dislrn = 1;
			txinfo->opts1.bit.cputag_ipcs = 1;
			txinfo->opts1.bit.cputag_l4cs = 0;	// do not use hw to do tcp/udp checkusm, which is buggy for ip-fragmented pkt
			txinfo->opts2.bit.cputag= 1;
			txinfo->opts2.bit.tx_pppoe_action = 0;
			txinfo->opts2.bit.tx_vlan_action = 0;
			txinfo->opts1.bit.cputag_psel = 1;
			txinfo->opts3.bit.ptp = 0;
			txinfo->opts3.bit.l34_keep = 1;

			if (cpuport_info->tx_desc.logical_port_id < 4) {
				txinfo->opts3.bit.tx_portmask = 1 << cpuport_info->tx_desc.logical_port_id;
				txinfo->opts1.bit.cputag_psel = 0;
				txinfo->opts3.bit.ptp = 0;
			}else if (cpuport_info->tx_desc.logical_port_id >= 6 && cpuport_info->tx_desc.logical_port_id <= 11) { //wifi port
				txinfo->opts3.bit.tx_portmask = 0;
				txinfo->opts3.bit.extspa = cpuport_info->tx_desc.logical_port_id-6;
				txinfo->opts1.bit.cputag_psel = 0;
				txinfo->opts3.bit.ptp = 0;
			}else if (cpuport_info->tx_desc.logical_port_id == 4) {//MII port
				txinfo->opts3.bit.tx_portmask = 1 << 5;
				txinfo->opts1.bit.cputag_psel = 0;
				txinfo->opts3.bit.ptp = 0;
			}else if (cpuport_info->tx_desc.logical_port_id == 5){ //PON port
				unsigned char llid =0;	// for streamid b6..4
				unsigned char qid = 0;	// for streamid b3..0
				txinfo->opts3.bit.tx_portmask = 1 << 4;
				// TO BE FIX, qid should be determine more dynamically, eg: pkt pbit
				txinfo->opts3.bit.tx_dst_stream_id = (qid & 0xf) | ((llid & 0x7) <<4);
				txinfo->opts1.bit.cputag_psel = 1;
				txinfo->opts3.bit.ptp = 0;
			}

			break;
		default:
			break;

	}
	return 0;
}

#if 0
// add re8686.h struct tx_info after packet
static int
tx_info_add(struct cpuport_info_t *cpuport_info, struct tx_info *opts)
{
	if (cpuport_info->frame_len + sizeof(struct tx_info) > cpuport_info->buf_len) {
		dbprintf_cpuport(LOG_ERR, "buffer overflow? frame_len(%d) + tx_info(%d) > buf_len(%d)\n", 
			cpuport_info->frame_len, sizeof(struct tx_info), cpuport_info->buf_len);
		return -1;
	}
	// add tx_info (hw descriptor) to end of pkt
	memcpy(cpuport_info->frame_ptr+cpuport_info->frame_len, opts, sizeof(struct tx_info));
	cpuport_info->frame_len += sizeof(struct tx_info);
	return 0;
}
#endif

// major call back function //////////////////////////////////////////////////////////////////////////////////

int
cpuport_hw_fvt2510_init(char *trap_ifname)
{
	int rbuf_size = 0;
	struct ifreq ifr;
	int ret;

	/* init raw socket */
	if ((sock_extract = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) < 0) {
		dbprintf_cpuport(LOG_ERR, "create sock_extract error\n");
		return -1;
	}
	
	rbuf_size = 1105920;
	setsockopt(sock_extract, SOL_SOCKET, SO_RCVBUFFORCE, (void *)&rbuf_size, sizeof(rbuf_size));

	strncpy(ifr.ifr_name, trap_ifname, IFNAMSIZ);

	if ((ret = ioctl(sock_extract, SIOCGIFINDEX, &ifr)) < 0) {
		dbprintf_cpuport(LOG_ERR, "ioctl error(%d)\n", ret);
		close(sock_extract);
		return -1;
	}

	iftrap_addr_ll.sll_family = PF_PACKET;
	iftrap_addr_ll.sll_protocol = htons(ETH_P_ALL);
	iftrap_addr_ll.sll_halen = ETH_ALEN;
	iftrap_addr_ll.sll_ifindex = ifr.ifr_ifindex;

	if (bind(sock_extract, (struct sockaddr *)(&iftrap_addr_ll), sizeof(struct sockaddr_ll)) < 0)
		dbprintf_cpuport(LOG_ERR, "sock_iftrap bind interface error\n");
	
	return 0;
}

int
cpuport_hw_fvt2510_shutdown(void)
{
	int ret = -1;

	if ((ret = close(sock_extract)) < 0) 
		dbprintf_cpuport(LOG_ERR, "fail to close ifextract socket\n");
	sock_extract = -1;

	if (ret < 0)
		return -1;
	return 0;
}

#include <linux/sockios.h>
#define IFSW_IOCTL_NOP		(SIOCDEVPRIVATE+0)
#define IFSW_IOCTL_DIRECT_SEND	(SIOCDEVPRIVATE+1)
struct ifsw_ioctl_direct_send_parm_t {
	struct tx_info txdesc;
	unsigned short frame_len;
	unsigned char *frame_ptr;
};

int
cpuport_hw_fvt2510_frame_send(struct cpuport_info_t *cpuport_info)
{
	int ret = 0;

	if (cpuport_info->frame_len == 0)
		return -1;

	if (cpuport_info->tx_desc.logical_port_id < 6) {
		struct tx_info txinfo;
		tx_info_prepare(cpuport_info, CPUPORT_XMIT_RAW, &txinfo);
		tx_info_print(&txinfo);
		dbprintf_cpuport(LOG_WARNING, "%p, send to logical port=%d, len=%d ----->\n", 
			cpuport_info->frame_ptr, cpuport_info->tx_desc.logical_port_id, cpuport_info->frame_len);
		// we do not call cpuport_sniffer_dbprint_pkt() here, it is already done in cpuport_frame_send()

#if 0
		// tx through rawsocket send, consumer of the rawsocket would see pkt sent by each other
		// it wont be easy to know whether pkt is received from extract interface or from other consumer of the same raw socket
		if (tx_info_add(cpuport_info, &txinfo) == 0) {
			if (sendto(sock_extract, cpuport_info->frame_ptr, cpuport_info->frame_len, 0, (struct sockaddr*)&iftrap_addr_ll, sizeof(struct sockaddr_ll)) <0) {
				dbprintf_cpuport(LOG_ERR, "socket_extract sendto err %d(%s), logical_port=%u\n", 
					errno, strerror(errno), cpuport_info->tx_desc.logical_port_id);
				ret = -1;
			}
			cpuport_info->frame_len -= sizeof(struct tx_info);	// no matter sent is okay or not, minus the length of txinfo immediately after sent
		} else {
			ret = -1;
		}
#else
		// tx through rawsocket ioctl, consumer of the rawsocket wont see pkt sent by each other
		{
			struct ifsw_ioctl_direct_send_parm_t parm;
			struct ifreq ifr;
			
			parm.txdesc = txinfo;
			parm.frame_len = cpuport_info->frame_len;
			parm.frame_ptr = cpuport_info->frame_ptr;
			strncpy(ifr.ifr_name, TRAP_IFNAME, 16);
			ifr.ifr_data =  (caddr_t)&parm;	
			if (ioctl(sock_extract, IFSW_IOCTL_DIRECT_SEND, &ifr) < 0) {
				dbprintf_cpuport(LOG_ERR, "IFSW_IOCTL_DIRECT_SEND err(%d, %s), tx logical_port=%u\n", 
					errno, strerror(errno), cpuport_info->tx_desc.logical_port_id);
				ret = -1;		
			}
		}
#endif		

	}else if (cpuport_info->tx_desc.logical_port_id > 6) {
		return -1;
	}

	// when LOG_NOTICE, a lot info would be dumped on screen, add a blank line to seperate different frame_send 
	util_dbprintf(debug_level_cpuport, LOG_NOTICE, "\n");

	return ret;
}

int
cpuport_hw_fvt2510_frame_recv_is_available(struct timeval *timeout)
{
	int sel_rtn = 0;
	int avail_socks = 0;
	int maxfd=-1;

	FD_ZERO(&readfds);
	
	if (sock_extract >= 0) {
		FD_SET(sock_extract, &readfds);
		if (sock_extract > maxfd)
			maxfd = sock_extract;
	}

	sel_rtn = select(maxfd + 1, &readfds, NULL, NULL, timeout);

	if (sel_rtn > 0) {
		if (FD_ISSET(sock_extract, &readfds)) {
			avail_socks++;
		}
		return avail_socks;
	} else if (sel_rtn == 0) {
		return 0;
	} else if (sel_rtn < 0) {
		return -1;
	} else if (sel_rtn != avail_socks) {
		return -1;
	}
	dbprintf_cpuport(LOG_ERR, "unconditional status avail_socks %d sel_rtn %d\n", avail_socks, sel_rtn);
	return -1;
}

int
cpuport_hw_fvt2510_frame_recv(struct cpuport_info_t *cpuport_info, unsigned short buf_len)
{
	if (FD_ISSET(sock_extract, &readfds)) {
		int len;

		FD_CLR(sock_extract, &readfds);		
		len = recv(sock_extract, cpuport_info->frame_ptr, buf_len, MSG_TRUNC);
		if (len<4) {
			if (errno == EINTR) {
				FD_SET(sock_extract, &readfds);	// fd set so next call to frame_recv() wont skip this socket
			} else {
				dbprintf_cpuport(LOG_ERR, "recv failed on trap interface socket (%s)\n", strerror(errno)); 
			}
			return -1;
		}
		util_dbprintf(debug_level_cpuport, LOG_WARNING, "\n");
		dbprintf_cpuport(LOG_WARNING, "recv pkt from trap interface socket <=====\n"); 

		cpuport_info->frame_len = len;
		rx_desc_fill_for_sock_extract(cpuport_info);
		return 1;
	}

	return 0;
}
