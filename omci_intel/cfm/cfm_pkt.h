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
 * Module  : cfm
 * File    : cfm_pkt.h
 *
 ******************************************************************/
 
#ifndef __CFM_PKT_H__
#define __CFM_PKT_H__

#include <net/ethernet.h>
#include <netinet/ip.h>

// IFHWADDRLEN is defined in net/if.h, 
// however, net/if.h would conflict with linux/if*.h, so we declare IFHWADDRLEN directly
#ifndef IFHWADDRLEN
#define IFHWADDRLEN 6
#endif

#include "cpuport.h"
#include "cpuport_frame.h"

#define VLAN_TAG_OFFSET		4
#define MIN_ETH_FRAME_SIZE	64
#define BUF_SIZE		2030

#define TRUE	1
#define FALSE	0

#define CFM_ETHERTYPE		0x8902

#define CFM_PDU_INVALID	0
#define CFM_PDU_CCM	1
#define CFM_PDU_LBR	2
#define CFM_PDU_LBM	3
#define CFM_PDU_LTR	4
#define CFM_PDU_LTM	5
#define CFM_PDU_AIS	33
#define CFM_PDU_LCK	35
#define CFM_PDU_TST	37
#define CFM_PDU_LAPS	39
#define CFM_PDU_RAPS	40
#define CFM_PDU_MCC	41
#define CFM_PDU_LMR	42
#define CFM_PDU_LMM	43
#define CFM_PDU_1DM	45
#define CFM_PDU_DMR	46
#define CFM_PDU_DMM	47
#define CFM_PDU_EXR	48
#define CFM_PDU_EXM	49
#define CFM_PDU_VSR	50
#define CFM_PDU_VSM	51
#define CFM_PDU_CSF	52
#define CFM_PDU_1SL	53
#define CFM_PDU_SLR	54
#define CFM_PDU_SLM	55
#define CFM_PDU_MAX	63	// as we define cfm pm counter size to 64, we use 63 as max!

typedef struct {
#if (__BYTE_ORDER == __BIG_ENDIAN)
	unsigned char	md_level:3,
			version:5;
#else
	unsigned char	version:5,
			md_level:3;
#endif
	unsigned char opcode;
	unsigned char flags;
	unsigned char first_tlv_offset;
} __attribute((packed)) cfm_hdr_t;

typedef struct {
	unsigned int xid;
} __attribute((packed)) cfm_pdu_lb_t;

typedef struct {
	unsigned int xid;
	unsigned char ttl;
	unsigned char origin_addr[IFHWADDRLEN];
	unsigned char target_addr[IFHWADDRLEN];
} __attribute((packed)) cfm_pdu_ltm_t;

typedef struct {
	unsigned int xid;
} __attribute((packed)) cfm_pdu_lbr_t;

typedef struct {
	unsigned int xid;
	unsigned char ttl;
	unsigned char relay_action;
} __attribute((packed)) cfm_pdu_ltr_t;

typedef struct {
	unsigned int seq;
	unsigned short mep_id;
	unsigned char ma_id[48];
	unsigned int y_1731[4];
} __attribute((packed)) cfm_pdu_ccm_t;

typedef struct {
} __attribute((packed)) cfm_pdu_aislck_t;	//  ais/lck has TLV offset always 0, which means there is nothing between tlv_offset & tlv_end

typedef struct {
	unsigned int TxFCf;
	unsigned int RxFCf;
	unsigned int TxFCb;
} __attribute((packed)) cfm_pdu_lm_t;

struct ieee1588_time {
	//for 64 bit, not 80 bit
	unsigned int sec; /*Seconds */
	unsigned int nanosec; /*Nano seconds*/
} __attribute__ ((packed));

typedef struct {
	struct ieee1588_time TxTimestampf;
	struct ieee1588_time RxTimestampf;
} __attribute((packed)) cfm_pdu_1dm_t;

typedef struct {
	unsigned char type; //32 
	unsigned short length;	//max 1480
} __attribute((packed)) cfm_tlv_data_t;

typedef struct {
	unsigned char type; //32 
	unsigned short length;	//max 1480, include pattern_type, test_pattern, crc_32
	unsigned char pattern_type; //0,1,2,3,4-255
	//unsigned char *test_pattern;
	//unsigned int crc_32;
} __attribute((packed)) cfm_tlv_test_t;

typedef struct {
	unsigned int sequence_number;
} __attribute((packed)) cfm_pdu_tst_t;

typedef struct {
	struct ieee1588_time TxTimestampf;
	struct ieee1588_time RxTimestampf;
	struct ieee1588_time TxTimestampb;
	struct ieee1588_time RxTimestampb;
} __attribute((packed)) cfm_pdu_dm_t;

typedef struct {
	unsigned short source_mep_id;
	unsigned short responder_mep_id;
	unsigned int test_id;
	unsigned int TxFCf;
	unsigned int TxFCb;
} __attribute((packed)) cfm_pdu_sl_t;

typedef struct {
	unsigned char type; // 0
} __attribute((packed)) cfm_tlv_end_t;


#define CFM_TLV_TYPE_END			0
#define CFM_TLV_TYPE_SENDER_ID			1
#define CFM_TLV_TYPE_PORT_STATUS		2
#define CFM_TLV_TYPE_DATA			3
#define CFM_TLV_TYPE_INTERFACE_STATUS		4
#define CFM_TLV_TYPE_REPLY_INGRESS		5
#define CFM_TLV_TYPE_REPLY_EGRESS		6
#define CFM_TLV_TYPE_LTM_EGRESS_ID		7
#define CFM_TLV_TYPE_LTR_EGRESS_ID		8
#define CFM_TLV_TYPE_TEST			32
#define CFM_TLV_TYPE_TESTID			36

typedef struct {
	unsigned char type; // 1
	unsigned short len;
	unsigned char sender_id[160];
} __attribute((packed)) cfm_tlv_sender_id_t;

typedef struct {
	unsigned char type; // 2
	unsigned short len; // 1
	unsigned char port_status;
} __attribute((packed)) cfm_tlv_port_status_t;

typedef struct {
	unsigned char type; // 4
	unsigned short len; // 1
	unsigned char interface_status;
} __attribute((packed)) cfm_tlv_interface_status_t;

typedef struct {
	unsigned char type; // 5
	unsigned short len; // 7 
	unsigned char ingress_action;
	unsigned char ingress_mac[IFHWADDRLEN];
} __attribute((packed)) cfm_tlv_reply_ingress_t;

typedef struct {
	unsigned char type; // 6
	unsigned short len; // 7
	unsigned char egress_action;
	unsigned char egress_mac[IFHWADDRLEN];
} __attribute((packed)) cfm_tlv_reply_egress_t;

typedef struct {
	unsigned char type; // 7
	unsigned short len; // 8
	unsigned short egress_id;
	unsigned char egress_mac[IFHWADDRLEN];
} __attribute((packed)) cfm_tlv_ltm_egress_id_t;

typedef struct {
	unsigned char type; // 8
	unsigned short len; // 16
	unsigned short last_egress_id;
	unsigned char last_egress_mac[IFHWADDRLEN];
	unsigned short next_egress_id;
	unsigned char next_egress_mac[IFHWADDRLEN];
} __attribute((packed)) cfm_tlv_ltr_egress_id_t;

typedef struct {
	struct cpuport_desc_t rx_desc;
	struct cpuport_desc_t tx_desc;
	unsigned char *frame_ptr;
	unsigned short frame_len;
	unsigned char *dst_mac_ptr;
	unsigned char *src_mac_ptr;
	unsigned char *in_tci;
	unsigned char *out_tci;
	unsigned char vlan_num;
	cfm_hdr_t *cfm_hdr;
} cfm_pktinfo_t;

#include "cfm_mp.h"

/* cfm_pkt.c */
int cfm_pkt_recv_is_cfm(struct cpuport_info_t *pktinfo);
int cfm_pkt_parse_pktinfo(struct cpuport_info_t *pktinfo, cfm_pktinfo_t *cfm_pktinfo);
int cfm_pkt_type(cfm_pktinfo_t *cfm_pktinfo);
int cfm_pkt_check_mac(cfm_config_t *cfm_config, cfm_pktinfo_t *cfm_pktinfo);
int cfm_pkt_check_vlan(cfm_config_t *cfm_config, cfm_pktinfo_t *cfm_pktinfo);

#endif
