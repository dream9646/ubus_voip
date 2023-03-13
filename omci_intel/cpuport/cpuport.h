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
 * File    : cpuport.h
 *
 ******************************************************************/

#ifndef __CPUPORT_H__
#define __CPUPORT_H__
#include "list.h"

#define CPUPORT_BUF_SIZE 	2030

#define CPUPORT_VTAG_LEN	4
#define CPUPORT_TCI_LEN		2
#define CPUPORT_MAC_ADDR_LEN	6
#define CPUPORT_ETHERTYE_LEN	2

#define CPUPORT_UNI		0
#define CPUPORT_WAN		1
#define CPUPORT_CPU		2
#define CPUPORT_DS_BCAST	3
#define CPUPORT_UNKNOWN		4

#define CPUPORT_INBOUND		0
#define CPUPORT_OUTBOUND	1

#define CPUPORT_MATCH 		0
#define CPUPORT_UNMATCH 	-1
#define CPUPORT_DROP		-2

#define CPUPORT_MAX_IF		20
//transmit mode
#define CPUPORT_XMIT_RAW	0	//Packets trap from switch and resend to switch.
					//These packets are not learned by L2 table 


struct cpuport_desc_t {
	unsigned short p2p_cfg;
	unsigned char logical_port_id;
	unsigned char router_mode;	// 0:packet from bridge  1:packet from router;
	struct me_t *bridge_port_me;
	unsigned char rx_wan_total; 	// how may possible wan port me for a pkt received from pon port
	struct me_t *rx_wan_me[8];	// when multi wan me are mapped to same flowid, 
					// a ds pkt of that flowid needs to iterate all possible wan port me to simulate the ds behavior
//	char *devname;
};

struct tci_fileds_t {
	unsigned short tpid;
	unsigned char priority;
	unsigned char de;
	unsigned short vid;
};

struct cpuport_tci_t {
	struct tci_fileds_t itci;
	struct tci_fileds_t otci;
	struct tci_fileds_t ext1_tci;
	struct tci_fileds_t ext2_tci;
	unsigned short ethertype;
	unsigned char dscp;
};

struct cpuport_info_t {
	struct cpuport_desc_t rx_desc;
	struct cpuport_desc_t tx_desc;
	unsigned char *buf_ptr;		// the whole buffer
	unsigned short buf_len;
	unsigned char *frame_ptr;	// frame inside the buffer
	unsigned short frame_len;

	unsigned char dst_mac[6];
	unsigned char src_mac[6];
	struct cpuport_tci_t tci; 	// where tci.ethertype is, eg: ETH_P_IP, ETH_P_IPV6, ETH_P_ARP, ETH_P_PPP_DISC, ETH_P_PPP_SES...
	unsigned short pppoe_proto;	// valid only if ethertype is ETH_P_PPP_SES, eg: PPP_IP, PPP_IPV6, PPP_IPCP, PPP_IPV6CP...(ref to cpuport_sniffer.c)
	void *nh;			// pointer to ipv4/ipv6/arp/pppoe header after ethertype or
					// pointer to ipv4/ipv6 header after pppoe header if ethertype is ETH_P_PPP_SES

	char *thread_name;		// pointer to a static thread name, no need to free
	struct timeval recv_time;	// the time that pkt received by cpuport_recv task
	struct timeval dispatch_time;	// the time that pkt dispatched to pkt handler rx Q
	struct timeval process_time;	// the time that pkt handler start to process this packet
	struct timeval forward_time;	// the time that pkt is forwarded by pkt handler
	struct cpuport_info_t *history;	// pointer to the copy kept in history log
	struct list_head node;
};

struct cpuport_hw_t {
	int (*init)(void);
	int (*shutdown)(void);
	int (*frame_send)(struct cpuport_info_t *cpuport_info , unsigned char xmit_type);
	int (*frame_recv_is_available)(struct timeval *timeout);
	int (*frame_recv)(struct cpuport_info_t *cpuport_info, unsigned short buf_len);
	int (*frame_recv_abort_select)(void);
	int (*frame_recv_abort_loop)(void);
	int (*frame_recv_fill_after_load)(struct cpuport_info_t *cpuport_info);
};

extern struct cpuport_hw_t cpuport_hw_g;
extern struct cpuport_hw_t cpuport_null_g;

int cpuport_hw_register(struct cpuport_hw_t *cpuport_hw);
int cpuport_hw_unregister(void);

#endif
