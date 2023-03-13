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
 * Module  : lldp
 * File    : lldp_core.h
 *
 ******************************************************************/
 
#ifndef __LLDP_CORE_H__
#define __LLDP_CORE_H__

#include <net/ethernet.h>
#include <netinet/ip.h>

#include "fwk_thread.h"
#include "cpuport.h"

// IFHWADDRLEN is defined in net/if.h, 
// however, net/if.h would conflict with linux/if*.h, so we declare IFHWADDRLEN directly
#ifndef IFHWADDRLEN
#define IFHWADDRLEN 6
#endif

extern int lldp_task_loop_g;
extern int lldp_timer_qid_g;

#define LLDP_TIMER_SYSPERIOD	 0x200

#define VLAN_TAG_OFFSET		4
#define MIN_ETH_FRAME_SIZE	64
#define BUF_SIZE		2030
#define NAME_SIZE		32
#define DESC_SIZE		128

#define TRUE	1
#define FALSE	0

#define LLDP_ETHERTYPE		0x88CC

#define LLDP_TLV_TYPE_END			0
#define LLDP_TLV_TYPE_CHASSIS_ID			1
#define LLDP_TLV_TYPE_PORT_ID			2
#define LLDP_TLV_TYPE_TIME_TO_LIVE			3
#define LLDP_TLV_TYPE_PORT_DESC		4
#define LLDP_TLV_TYPE_SYSTEM_NAME		5
#define LLDP_TLV_TYPE_SYSTEM_DESC		6
#define LLDP_TLV_TYPE_SYSTEM_CAP		7
#define LLDP_TLV_TYPE_MANAGE_ADDR		8
#define LLDP_TLV_TYPE_CUSTOM		127

#define LLDP_TLV_CUSTOM_SUBTYPE_MEDIA_CAP	1
#define LLDP_TLV_CUSTOM_SUBTYPE_NETWORK_POLICY	2
#define LLDP_TLV_CUSTOM_SUBTYPE_LOCATION_ID	3
#define LLDP_TLV_CUSTOM_SUBTYPE_EXT_POWER	4
#define LLDP_TLV_CUSTOM_SUBTYPE_HW_REVISION	5
#define LLDP_TLV_CUSTOM_SUBTYPE_FW_REVISION	6
#define LLDP_TLV_CUSTOM_SUBTYPE_SW_REVISION	7
#define LLDP_TLV_CUSTOM_SUBTYPE_SERIAL_NO	8
#define LLDP_TLV_CUSTOM_SUBTYPE_MFG_NAME	9
#define LLDP_TLV_CUSTOM_SUBTYPE_MODEL_NAME	10

#define LLDP_MED_OPT_NONE 0
#define LLDP_MED_OPT_FORCE 1
#define LLDP_MED_OPT_W_CAP 2

/* LLDP MED capabilities */
#define LLDP_MED_CAP_CAP        0x01
#define LLDP_MED_CAP_POLICY     0x02
#define LLDP_MED_CAP_LOCATION   0x04
#define LLDP_MED_CAP_MDI_PSE    0x08
#define LLDP_MED_CAP_MDI_PD     0x10
#define LLDP_MED_CAP_IVENTORY   0x20

/* Default LLDP Network Policy */
#define LLDP_DEFAULT_VID 4096
#define LLDP_DEFAULT_PBIT 7
#define LLDP_DEFAULT_DSCP 46


typedef struct {
#if (__BYTE_ORDER == __BIG_ENDIAN)
	unsigned short	tlv_type:7, // 0
			tlv_length:9;
#else
	unsigned short	tlv_length:9,
			tlv_type:7;
#endif
} __attribute((packed)) lldp_tlv_end_t;

typedef struct {
#if (__BYTE_ORDER == __BIG_ENDIAN)
	unsigned short	tlv_type:7, // 1
			tlv_length:9;
#else
	unsigned short	tlv_length:9,
			tlv_type:7;
#endif
	unsigned char subtype;
	unsigned char chassis_id[IFHWADDRLEN];
} __attribute((packed)) lldp_tlv_chassis_id_t;

typedef struct {
#if (__BYTE_ORDER == __BIG_ENDIAN)
	unsigned short	tlv_type:7, // 2
			tlv_length:9;
#else
	unsigned short	tlv_length:9,
			tlv_type:7;
#endif
	unsigned char subtype;
	unsigned char port_id[IFHWADDRLEN];
} __attribute((packed)) lldp_tlv_port_id_t;

typedef struct {
#if (__BYTE_ORDER == __BIG_ENDIAN)
	unsigned short	tlv_type:7, // 3
			tlv_length:9;
#else
	unsigned short	tlv_length:9,
			tlv_type:7;
#endif
	unsigned short ttl;
} __attribute((packed)) lldp_tlv_ttl_t;

typedef struct {
#if (__BYTE_ORDER == __BIG_ENDIAN)
	unsigned short	tlv_type:7, // 4
			tlv_length:9;
#else
	unsigned short	tlv_length:9,
			tlv_type:7;
#endif
	unsigned char desc[32];
} __attribute((packed)) lldp_tlv_port_desc_t;

typedef struct {
#if (__BYTE_ORDER == __BIG_ENDIAN)
	unsigned short	tlv_type:7, // 5
			tlv_length:9;
#else
	unsigned short	tlv_length:9,
			tlv_type:7;
#endif
	unsigned char name[NAME_SIZE];
} __attribute((packed)) lldp_tlv_system_name_t;

typedef struct {
#if (__BYTE_ORDER == __BIG_ENDIAN)
	unsigned short	tlv_type:7, // 6
			tlv_length:9;
#else
	unsigned short	tlv_length:9,
			tlv_type:7;
#endif
	unsigned char desc[DESC_SIZE];
} __attribute((packed)) lldp_tlv_system_desc_t;

typedef struct {
#if (__BYTE_ORDER == __BIG_ENDIAN)
	unsigned short	tlv_type:7, // 7
			tlv_length:9;
#else
	unsigned short	tlv_length:9,
			tlv_type:7;
#endif
	unsigned short cap;
	unsigned short en_cap;
} __attribute((packed)) lldp_tlv_system_cap_t;

typedef struct {
#if (__BYTE_ORDER == __BIG_ENDIAN)
	unsigned short	tlv_type:7, // 8
			tlv_length:9;
#else
	unsigned short	tlv_length:9,
			tlv_type:7;
#endif
	unsigned char length;
	unsigned char subtype;
	unsigned int addr;
	unsigned char itf_subtype;
	unsigned int itf_number;
	unsigned char oid_str_length;
} __attribute((packed)) lldp_tlv_manage_addr_ipv4_t;

typedef struct {
#if (__BYTE_ORDER == __BIG_ENDIAN)
	unsigned short	tlv_type:7, // 8
			tlv_length:9;
#else
	unsigned short	tlv_length:9,
			tlv_type:7;
#endif
	unsigned char length;
	unsigned char subtype;
	unsigned int addr[4];
	unsigned char itf_subtype;
	unsigned int itf_number;
	unsigned char oid_str_length;
} __attribute((packed)) lldp_tlv_manage_addr_ipv6_t;

typedef struct {
#if (__BYTE_ORDER == __BIG_ENDIAN)
	unsigned short	tlv_type:7, // 127
			tlv_length:9;
#else
	unsigned short	tlv_length:9,
			tlv_type:7;
#endif
	unsigned char oui[3];
	unsigned char subtype; // 1
	unsigned short cap;
	unsigned char type;
} __attribute((packed)) lldp_tlv_media_cap_t;

typedef struct {
#if (__BYTE_ORDER == __BIG_ENDIAN)
	unsigned short	tlv_type:7, // 127
			tlv_length:9;
#else
	unsigned short	tlv_length:9,
			tlv_type:7;
#endif
	unsigned char oui[3];
	unsigned char subtype; // 1
	unsigned char auto_nego_support;
	unsigned short adv_cap;
	unsigned short oper;
} __attribute((packed)) lldp_tlv_mac_phy_cap_t;

typedef struct {
#if (__BYTE_ORDER == __BIG_ENDIAN)
	unsigned short	tlv_type:7, // 127
			tlv_length:9;
#else
	unsigned short	tlv_length:9,
			tlv_type:7;
#endif
	unsigned char oui[3];
	unsigned char subtype; // 2
	unsigned int	type:8,
			policy:1,
			tagged:1,
			vid:13,
			pbit:3,
			dscp:6;
} __attribute((packed)) lldp_tlv_network_policy_t;

typedef struct {
#if (__BYTE_ORDER == __BIG_ENDIAN)
	unsigned short	tlv_type:7, // 127
			tlv_length:9;
#else
	unsigned short	tlv_length:9,
			tlv_type:7;
#endif
	unsigned char oui[3];
	unsigned char subtype; // 3
	unsigned char lci_format;
	unsigned char lci[10];
} __attribute((packed)) lldp_tlv_location_id_t;

typedef struct {
#if (__BYTE_ORDER == __BIG_ENDIAN)
	unsigned short	tlv_type:7, // 127
			tlv_length:9;
#else
	unsigned short	tlv_length:9,
			tlv_type:7;
#endif
	unsigned char oui[3];
	unsigned char subtype; // 4
#if (__BYTE_ORDER == __BIG_ENDIAN)
	unsigned char	power_type:2, // 127
			power_source:2,
			power_priority:4;
#else
	unsigned char	power_priority:4,
			power_source:2,
			power_type:2;
#endif
	unsigned short power_value;
} __attribute((packed)) lldp_tlv_ext_power_t;

typedef struct {
#if (__BYTE_ORDER == __BIG_ENDIAN)
	unsigned short	tlv_type:7, // 127
			tlv_length:9;
#else
	unsigned short	tlv_length:9,
			tlv_type:7;
#endif
	unsigned char oui[3];
	unsigned char subtype; // 5
	unsigned char revision[10];
} __attribute((packed)) lldp_tlv_hw_rev_t;

typedef struct {
#if (__BYTE_ORDER == __BIG_ENDIAN)
	unsigned short	tlv_type:7, // 127
			tlv_length:9;
#else
	unsigned short	tlv_length:9,
			tlv_type:7;
#endif
	unsigned char oui[3];
	unsigned char subtype; // 6
	unsigned char revision[10];
} __attribute((packed)) lldp_tlv_fw_rev_t;

typedef struct {
#if (__BYTE_ORDER == __BIG_ENDIAN)
	unsigned short	tlv_type:7, // 127
			tlv_length:9;
#else
	unsigned short	tlv_length:9,
			tlv_type:7;
#endif
	unsigned char oui[3];
	unsigned char subtype; // 7
	unsigned char revision[10];
} __attribute((packed)) lldp_tlv_sw_rev_t;

typedef struct {
#if (__BYTE_ORDER == __BIG_ENDIAN)
	unsigned short	tlv_type:7, // 127
			tlv_length:9;
#else
	unsigned short	tlv_length:9,
			tlv_type:7;
#endif
	unsigned char oui[3];
	unsigned char subtype; // 8
	unsigned char sn[12];
} __attribute((packed)) lldp_tlv_serial_no_t;

typedef struct {
#if (__BYTE_ORDER == __BIG_ENDIAN)
	unsigned short	tlv_type:7, // 127
			tlv_length:9;
#else
	unsigned short	tlv_length:9,
			tlv_type:7;
#endif
	unsigned char oui[3];
	unsigned char subtype; // 9
	unsigned char name[5];
} __attribute((packed)) lldp_tlv_mfg_name_t;

typedef struct {
#if (__BYTE_ORDER == __BIG_ENDIAN)
	unsigned short	tlv_type:7, // 127
			tlv_length:9;
#else
	unsigned short	tlv_length:9,
			tlv_type:7;
#endif
	unsigned char oui[3];
	unsigned char subtype; // 10
	unsigned char name[8];
} __attribute((packed)) lldp_tlv_model_name_t;

typedef struct {
	unsigned char dst_mac[IFHWADDRLEN];
	unsigned char src_mac[IFHWADDRLEN];
	unsigned short ethertype;
	lldp_tlv_chassis_id_t chassis_id;
	lldp_tlv_port_id_t port_id;
	lldp_tlv_ttl_t ttl;
	lldp_tlv_port_desc_t port_desc;
	lldp_tlv_system_name_t system_name;
	lldp_tlv_system_desc_t system_desc;
	lldp_tlv_system_cap_t system_cap;
	lldp_tlv_manage_addr_ipv4_t manage_addr_ipv4;
	lldp_tlv_manage_addr_ipv6_t manage_addr_ipv6;
	lldp_tlv_media_cap_t media_cap;
	lldp_tlv_mac_phy_cap_t mac_phy_cap;
	lldp_tlv_network_policy_t network_policy;
	lldp_tlv_location_id_t location_id;
	lldp_tlv_ext_power_t ext_power;
	lldp_tlv_hw_rev_t hw_rev;
	lldp_tlv_fw_rev_t fw_rev;
	lldp_tlv_sw_rev_t sw_rev;
	lldp_tlv_serial_no_t serial_no;
	lldp_tlv_mfg_name_t mfg_name;
	lldp_tlv_model_name_t model_name;
	lldp_tlv_end_t end;
} __attribute((packed)) lldp_pkt_t;

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
	lldp_pkt_t *lldp_pkt;
} lldp_pktinfo_t;

enum lldp_adminStatus {
	LLDP_RXTX = 0,
	LLDP_DISABLE = 1,
	LLDP_TXONLY = 2,
	LLDP_RXONLY = 3,
};

typedef struct {
	unsigned int tx;
	unsigned int rx;
} lldp_counter_t;

typedef struct {
	unsigned char is_tx_timer_init_state;

	unsigned char is_shutdown;
	unsigned char txNow;
	unsigned char txCredit;
	unsigned char newNeighbor;
	unsigned int txTTR;
	unsigned char txFast;
	unsigned char localChange;

	unsigned int  neighbor_couter;
	unsigned char rcvFrame;
	unsigned char rxInfoAge;
} lldp_sm_t;

typedef struct {
	unsigned int	type:8,
			policy:1,
			tagged:1,
			vid:13,
			pbit:3,
			dscp:6;
} lldp_network_policy_t;

typedef struct {
	unsigned char adminStatus;
	unsigned char tx_dst_mac[IFHWADDRLEN];
	unsigned char tx_src_mac[IFHWADDRLEN];
	lldp_network_policy_t network_policy;

	unsigned char rx_src_mac[IFHWADDRLEN];
	unsigned int rxTTL;

	unsigned int power_priority;
	unsigned int power_value;
} lldp_mib_t;

typedef struct {
	lldp_mib_t mib;
	lldp_sm_t sm;
	lldp_counter_t pkt_c;
} lldp_parm_t;

/* lldp_core.c */
int lldp_init(void);
int lldp_start(void);
int lldp_stop(void);
int lldp_shutdown(void);
int lldp_get_admin_state(int port_id);

/* lldp_send.c */
int lldp_send_pkt_process(int port_id);
int lldp_get_med_cap(void);

/* lldp_recv.c */
int lldp_pkt_recv_is_lldp(struct cpuport_info_t *);
int lldp_pkt_parse_pktinfo(struct cpuport_info_t *, lldp_pktinfo_t *);
int lldp_recv_pkt_process(lldp_pktinfo_t *);

/* lldp_sm.c */
int lldp_tx_sm(int port_id);
int lldp_rx_sm(int port_id);
int lldp_tx_timer_sm(int port_id);
void lldp_tx_timer_sm_init(lldp_sm_t *sm);
#endif
