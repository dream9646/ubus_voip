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
 * Purpose : 5VT TRAF MANAGER
 * Module  : cpuport
 * File    : cpuport.h
 *
 ******************************************************************/

#ifndef __CPUPORT_H__
#define __CPUPORT_H__

#define TRAP_IFNAME		"ifsw0"

#define CPUPORT_BUF_SIZE 	2030

#define CPUPORT_VTAG_LEN	4
#define CPUPORT_TCI_LEN		2
#define CPUPORT_MAC_ADDR_LEN	6
#define CPUPORT_ETHERTYE_LEN	2

//transmit mode
#define CPUPORT_XMIT_RAW	0	//Packets trap from switch and resend to switch.
					//These packets are not learned by L2 table 

#define util_dbprintf(threshold, level, fmt, args...)	\
	do { if (threshold>=(level)) 	fprintf(stderr, fmt, ##args); } while(0)
#define dbprintf_cpuport(level, fmt, args...)	\
	do { if (debug_level_cpuport>=(level)) util_dbprintf(debug_level_cpuport, level, "%s:%d: %s(): " fmt, __FILE__, __LINE__, __FUNCTION__, ##args); } while(0)

// tx_info, rx_info: the hw descriptor of per packet //////////////////////////

struct rx_info{
	union{
		struct{
			unsigned int own:1;//31
			unsigned int eor:1;//30
			unsigned int fs:1;//29
			unsigned int ls:1;//28
			unsigned int crcerr:1;//27
			unsigned int ipv4csf:1;//26
			unsigned int l4csf:1;//25
			unsigned int rcdf:1;//24
			unsigned int ipfrag:1;//23
			unsigned int pppoetag:1;//22
			unsigned int rwt:1;//21
			unsigned int pkttype:4;//17~20
			unsigned int l3routing:1;//16
			unsigned int origformat:1;//15
			unsigned int pctrl:1;//14
			unsigned int data_length:14;//0~13
		}bit;
		unsigned int dw;//double word
	}opts1;
	unsigned int addr;
	union{
		struct{
			unsigned int cputag:1;//31
			unsigned int ptp_in_cpu_tag_exist:1;//30
			unsigned int svlan_tag_exist:1;//29
			unsigned int rsvd_2:2;//27~28
			unsigned int pon_stream_id:7;//20~26
			unsigned int rsvd_1:3;//17~19
			unsigned int ctagva:1;//16
			unsigned int cvlan_tag:16;//0~15
		}bit;
		unsigned int dw;//double word
	}opts2;
	union{
		struct{
			unsigned int src_port_num:5;//27~31
			unsigned int dst_port_mask:6;//21~26
			unsigned int reason:8;//13~20
			unsigned int internal_priority:3;//10~12
			unsigned int ext_port_ttl_1:5;//5~9
			unsigned int rsvd:5;//0~4
		}bit;
		unsigned int dw;//double word
	}opts3;
};

struct tx_info{
	union{
		struct{
			unsigned int own:1;//31
			unsigned int eor:1;//30
			unsigned int fs:1;//29
			unsigned int ls:1;//28
			unsigned int ipcs:1;//27
			unsigned int l4cs:1;//26
			unsigned int keep:1;//25
			unsigned int blu:1;//24
			unsigned int crc:1;//23
			unsigned int vsel:1;//22
			unsigned int dislrn:1;//21
			unsigned int cputag_ipcs:1;//20
			unsigned int cputag_l4cs:1;//19
			unsigned int cputag_psel:1;//18
			unsigned int rsvd:1;//17
			unsigned int data_length:17;//0~16
		}bit;
		unsigned int dw;//double word
	}opts1;
	unsigned int addr;
	union{
		struct{
			unsigned int cputag:1;//31
			unsigned int aspri:1;//30
			unsigned int cputag_pri:3;//27~29
			unsigned int tx_vlan_action:2;//25~26
			unsigned int tx_pppoe_action:2;//23~24
			unsigned int tx_pppoe_idx:3;//20~22
			unsigned int efid:1;//19
			unsigned int enhance_fid:3;//16~18
			unsigned int vidl:8;//8~15
			unsigned int prio:3;//5~7
			unsigned int cfi:1;// 4
			unsigned int vidh:4;//0~3
		}bit;
		unsigned int dw;//double word
	}opts2;
	union{
		struct{
			unsigned int extspa:3;//29~31
			unsigned int tx_portmask:6;//23~28
			unsigned int tx_dst_stream_id:7;//16~22
			unsigned int rsvd:12;// 4~15
			unsigned int rsv1:1;// 3
			unsigned int rsv0:1;// 2
			unsigned int l34_keep:1;// 1
			unsigned int ptp:1;//0
		}bit;
		unsigned int dw;//double word
	}opts3;
	union{
		unsigned int dw;
	}opts4;
};

// tx_desc, rx_desc: the sw descriptor of per packet ///////////////////////////

struct cpuport_desc_t {
	unsigned char logical_port_id;
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
	struct cpuport_tci_t tci; 
	struct iphdr *ip_hdr; 
};

// packet header casting //////////////////////////////////////////////////////

struct cpuport_tag4_t {
	unsigned short tpid0;
	unsigned short tci0;
	unsigned short tpid1;
	unsigned short tci1;
	unsigned short tpid2;
	unsigned short tci2;
	unsigned short tpid3;
	unsigned short tci3;
	unsigned short ethertype;
};

struct cpuport_tag3_t {
	unsigned short tpid0;
	unsigned short tci0;
	unsigned short tpid1;
	unsigned short tci1;
	unsigned short tpid2;
	unsigned short tci2;
	unsigned short ethertype;
};

struct cpuport_tag2_t {
	unsigned short tpid0;
	unsigned short tci0;
	unsigned short tpid1;
	unsigned short tci1;
	unsigned short ethertype;
};

struct cpuport_tag1_t {
	unsigned short tpid0;
	unsigned short tci0;
	unsigned short ethertype;
};

struct cpuport_tag0_t {
	unsigned short ethertype;
};

struct cpuport_pkthdr_t {
	unsigned char dmac[6];
	unsigned char smac[6];
	union {
		struct cpuport_tag0_t zero;
		struct cpuport_tag1_t one;
		struct cpuport_tag2_t two;
		struct cpuport_tag3_t three;
		struct cpuport_tag4_t four;
	} tag_u;
};

extern int debug_level_cpuport;

/* cpuport_hw_fvt2510.c */
int cpuport_hw_fvt2510_init(char *trap_ifname);
int cpuport_hw_fvt2510_shutdown(void);
int cpuport_hw_fvt2510_frame_send(struct cpuport_info_t *cpuport_info);
int cpuport_hw_fvt2510_frame_recv_is_available(struct timeval *timeout);
int cpuport_hw_fvt2510_frame_recv(struct cpuport_info_t *cpuport_info, unsigned short buf_len);

#endif
