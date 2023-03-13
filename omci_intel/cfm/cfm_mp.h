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
 * File    : cfm_mp.h
 *
 ******************************************************************/

#ifndef __CFM_MP_H__
#define __CFM_MP_H__

#define CFM_MAX_MP			64
#define CFM_MAX_CCM_DB			12
#define CFM_MAX_TTL			255
#define CFM_SERVER_MEP_MPID		4095

///////////////////////////////////////////////////////////////////////////////

typedef struct {
	unsigned int tx;
	unsigned int tx_uni;
	unsigned int tx_ani;
	unsigned int rx;
	unsigned int rx_uni;
	unsigned int rx_ani;
	unsigned int drop;
	unsigned int drop_uni;
	unsigned int drop_ani;
	unsigned int fwd;
	unsigned int fwd_uni;
	unsigned int fwd_ani;
	unsigned int tx_rdi;
	unsigned int rx_rdi;
} cfm_pm_counter_t;

#define CHASSIS_ID_SUBTYPE_CHASSIS	1
#define CHASSIS_ID_SUBTYPE_IFALIAS	2
#define CHASSIS_ID_SUBTYPE_PORT		3
#define CHASSIS_ID_SUBTYPE_MACADDR	4
#define CHASSIS_ID_SUBTYPE_NETADDR	5
#define CHASSIS_ID_SUBTYPE_IFNAME	6
#define CHASSIS_ID_SUBTYPE_LOCAL	7

// chassis_id_length + mgmtaddr_domain_length + mgmtaddr_length should <=54byte , see ITU-T G.988 9.3.26
typedef struct {
	// From 9.3.26 Dot1ag chassis-management info (ME 306)
	unsigned char chassis_id_length;
	unsigned char chassis_id_subtype;
	unsigned char chassis_id[50];
	unsigned char mgmtaddr_domain_length;
	unsigned char mgmtaddr_domain[50];
	unsigned char mgmtaddr_length;
	unsigned char mgmtaddr[50];
} cfm_chassis_t;

///////////////////////////////////////////////////////////////////////////////

#define LBR_PAYLOAD_SIZE_MAX	1492
#define LTR_PAYLOAD_SIZE_MAX	74

typedef struct
{
	struct list_head node;
	unsigned char	lbr_mac[IFHWADDRLEN];
	unsigned short	lbr_length;
	unsigned long long lbr_timestamp;		// unit:us
	union {
		struct {
			cfm_hdr_t lbr_hdr;	//4 bytes
			cfm_pdu_lbr_t lbr_pdu;	//4 bytes
			unsigned char lbr_tlv[LBR_PAYLOAD_SIZE_MAX-sizeof(cfm_pdu_lbr_t)-sizeof(cfm_hdr_t)];
		} lbr;
		unsigned char lbr_payload[LBR_PAYLOAD_SIZE_MAX];
	} u;
} cfm_pkt_lbr_entry_t;

typedef struct
{
	struct list_head node;
	unsigned char	ltm_mac[IFHWADDRLEN];
	unsigned int	xid;
} cfm_pkt_ltm_entry_t;

typedef struct
{
	struct list_head node;
	unsigned char	ltr_mac[IFHWADDRLEN];
	unsigned short	ltr_length;
	unsigned long long ltr_timestamp;		// unit:us
	union {
		struct {
			cfm_hdr_t ltr_hdr;	// 4 bytes
			cfm_pdu_ltr_t ltr_pdu;	// 6 bytes
			unsigned char ltr_tlv[LBR_PAYLOAD_SIZE_MAX-sizeof(cfm_pdu_ltr_t)-sizeof(cfm_hdr_t)];
		} ltr;
		unsigned char ltr_payload[LTR_PAYLOAD_SIZE_MAX];
	} u;
} cfm_pkt_ltr_entry_t;

// rmep state for CCM list entry
//
//                       (no ccm for 3.5 interval)
// EXPECT/remove <---------+---------------------------------+                            INACTIVE
//                         |                                 |                                ^
//       (recv 1st ccm)    |  (recv 2 more consecutive ccm)  | (no defect within HOLDtime)    v    (no ccm for 3.5 interval)
// EXPECT ------------> DISCOVER -------------------------> HOLD -------------------------> ACTIVE -------------------------> LOST
//                         ^                                 |                                ^                                |
//                         +---------------------------------+                                +--------------------------------+
//                          (recv defect/nonconsecutive ccm)                                              (any ccm received)
//
// ps: DISCOVER & HOLD state are AT&T specific
// ps: LOST RMEP will be aging out (removed) after alarm_clear_soak_time
#define CCM_RMEP_STATE_NONE		0	// should not happen
#define CCM_RMEP_STATE_EXPECT		1	// when entries created from peer_mep_id[]
#define CCM_RMEP_STATE_DISCOVER		2	// when first ccm pkt received from a remote mep
#define CCM_RMEP_STATE_HOLD		3	// when entry in CCM_RMEP_STATE_DISCOVER, recv 3 consecutive ccm (including the first received pkt)
#define CCM_RMEP_STATE_ACTIVE		4	// when entry in CCM_RMEP_STATE_HOLD have no defect than holdtime (def:10 sec)
#define CCM_RMEP_STATE_INACTIVE		5	// when user manually set entry to inactive
#define CCM_RMEP_STATE_LOST		6	// not recv ccm from specific rmep for more than 3.5 ccm interval
#define CCM_RMEP_STATE_REMOVE		7	// when user manually set entry to remove

typedef struct
{
	struct list_head node;
	unsigned char	rmep_state;
	unsigned char	rmep_port_state;
	unsigned char	rmep_iface_state;
	unsigned char	rmep_interval;
	unsigned short	rmep_id;
	unsigned long long rmep_timestamp;		// unit:us
	unsigned long long rmep_clear_timestamp;	// unit:us
	unsigned char	rmep_mac[IFHWADDRLEN];
	unsigned char	rmep_rdi;
	unsigned int	rmep_seq;
	unsigned int	rmep_ccm;
	unsigned int	rmep_ccm_consecutive;

	unsigned int	TxFCf_ccm;
	unsigned int	RxFCb_ccm;
	unsigned int	TxFCb_ccm;
	unsigned int	TxFCl_ccm;
	unsigned int	RxFCl_ccm;
	unsigned int	test_id_1sl;
	unsigned int	TxFCl_1sl;
	unsigned int	TxFCf_1sl_stop_check;
	unsigned int	RxFCl_1sl;
	unsigned int	TxFCf_1sl;

	unsigned int	test_id_slm;
	unsigned int	TxFCl_slm;
	unsigned int	RxFCl_slm;
	unsigned int	TxFCl_slr;
	unsigned int	RxFCl_slr;
	unsigned int	TxFCf_slr;
	unsigned int	TxFCb_slr;

	unsigned int	TxFCl_lmm;
	unsigned int	RxFCl_lmm;
	unsigned int	TxFCl_lmr;
	unsigned int	RxFCl_lmr;

	unsigned int	TxFCf_p;
	unsigned int	RxFCf_p;
	unsigned int	RxFCb_p;
	unsigned int 	TxFCb_p;
	unsigned int 	RxFCl_p;
	unsigned int	TxFCf_c;
	unsigned int	RxFCf_c;
	unsigned int	RxFCb_c;
	unsigned int 	TxFCb_c;
	unsigned int 	RxFCl_c;
// lm_single
	time_t		eth_lm_single_start_time;
	time_t		eth_lm_single_stop_time;
	unsigned char	eth_lm_single_going_on;
	unsigned char	eth_lm_single_force_stop;
	unsigned int	eth_lm_single_expect_time;
	unsigned int	eth_lm_single_interval;
	unsigned int	eth_lm_single_repeat;
	unsigned int	eth_lm_single_fratio;
	unsigned int	eth_lm_single_nratio;
	unsigned int	eth_lm_single_rx_floss;
	unsigned int	eth_lm_single_rx_nloss;
	unsigned int	eth_lm_single_tx_floss;
	unsigned int	eth_lm_single_tx_nloss;
	int		eth_lm_single_start_check_timer;
	int		eth_lm_single_stop_check_timer;
	int		eth_lm_single_stop_timer;
// lm_dual
	time_t		eth_lm_dual_start_time;
	time_t		eth_lm_dual_stop_time;
	unsigned char	eth_lm_dual_going_on;
	unsigned char	eth_lm_dual_force_stop;
	unsigned int	eth_lm_dual_expect_time;
	unsigned int	eth_lm_dual_interval;
	unsigned int	eth_lm_dual_repeat;
	unsigned int	eth_lm_dual_fratio;
	unsigned int	eth_lm_dual_nratio;
	unsigned int	eth_lm_dual_rx_floss;
	unsigned int	eth_lm_dual_rx_nloss;
	unsigned int	eth_lm_dual_tx_floss;
	unsigned int	eth_lm_dual_tx_nloss;
	int		eth_lm_dual_sender_loop_timer;
	int		eth_lm_dual_stop_timer;
// slm_single
	time_t		eth_slm_single_start_time;
	time_t		eth_slm_single_stop_time;
	unsigned char	eth_slm_single_going_on;
	unsigned char	eth_slm_single_force_stop;
	unsigned int	eth_slm_single_expect_time;
	unsigned int	eth_slm_single_interval;
	unsigned int	eth_slm_single_repeat;
	unsigned int	eth_slm_single_fratio;
	unsigned int	eth_slm_single_nratio;
	unsigned int	eth_slm_single_rx_floss;
	unsigned int	eth_slm_single_rx_nloss;
	unsigned int	eth_slm_single_tx_floss;
	unsigned int	eth_slm_single_tx_nloss;
	int		eth_slm_single_sender_loop_timer;
	int		eth_slm_single_sender_stop_timer;
// slm_dual
	time_t		eth_slm_dual_start_time;
	time_t		eth_slm_dual_stop_time;
	unsigned char	eth_slm_dual_going_on;
	unsigned char	eth_slm_dual_force_stop;
	unsigned int	eth_slm_dual_expect_time;
	unsigned int	eth_slm_dual_interval;
	unsigned int	eth_slm_dual_repeat;
	unsigned int	eth_slm_dual_fratio;
	unsigned int	eth_slm_dual_nratio;
	unsigned int	eth_slm_dual_rx_nloss;
	unsigned int	eth_slm_dual_tx_nloss;
	int		eth_slm_dual_sender_loop_timer;
	int		eth_slm_dual_stop_check_timer;
	int		eth_slm_dual_recv_timer;

	cfm_tlv_sender_id_t	rmep_sender_id_tlv;
} cfm_pkt_rmep_entry_t;

///////////////////////////////////////////////////////////////////////////////

#define MD_NAME_FORMAT_NONE		1
#define MD_NAME_FORMAT_DNS		2
#define MD_NAME_FORMAT_MACADDR_SHORTINT	3
#define MD_NAME_FORMAT_STRING		4
#define MD_NAME_FORMAT_ICC		32

#define MA_NAME_FORMAT_VID		1
#define MA_NAME_FORMAT_STRING		2
#define MA_NAME_FORMAT_SHORTINT		3
#define MA_NAME_FORMAT_VPNID		4
#define MA_NAME_FORMAT_ICC		32

#define MA_MHF_CREATION_NONE		1	// mep: dont forward
#define MA_MHF_CREATION_DEFAULT		2	// mip: forward anyway
#define MA_MHF_CREATION_EXPLICIT	3	// mip: forward only if lower level mep exist
#define MA_MHF_CREATION_DEFER		4	// 1,2,3 used for both md/ma, 4 is for ma only

#define MA_SENDER_ID_PERMISSION_NONE		1	// none
#define MA_SENDER_ID_PERMISSION_CHASSIS		2	// chassis
#define MA_SENDER_ID_PERMISSION_MANAGE		3	// manage
#define MA_SENDER_ID_PERMISSION_CHASSIS_MANAGE	4	// chassis+manage
#define MA_SENDER_ID_PERMISSION_DEFER		5	// 1..4 used for both md/ma, 5 is for ma only

// mep control bit defined by omci
#define MEP_CONTROL_RESERVED		(1<<0)	// 0x1
#define MEP_CONTROL_CCM_ENABLE		(1<<1)	// 0x2
#define MEP_CONTROL_SERVER_MEP_ENABLE	(1<<2)	// 0x4
#define MEP_CONTROL_AIS_ENABLE		(1<<3)	// 0x8
#define MEP_CONTROL_IS_UP_MEP		(1<<4)	// 0x10
// mep control bits extension defined by 5vt
#define MEP_CONTROL2_LBR_ENABLE		(1<<0)	// 0x1
#define MEP_CONTROL2_LTR_ENABLE		(1<<1)	// 0x2
#define MEP_CONTROL2_DMM_ENABLE	(1<<2)	// 0x4
#define MEP_CONTROL2_LMM_ENABLE	(1<<3)	// 0x8

#define ETH_AIS_CONTROL_AIS_INTERVAL	(1<<0)
#define ETH_AIS_CONTROL_AIS_PRIORITY	(7<<1)
#define ETH_AIS_CONTROL_AIS_CLIENT_MDL	(7<<4)

#define CCM_INTERVAL_DISABLED	0
#define CCM_INTERVAL_3_33_MS	1
#define CCM_INTERVAL_10_MS	2
#define CCM_INTERVAL_100_MS	3
#define CCM_INTERVAL_1_SECOND	4
#define CCM_INTERVAL_10_SECOND	5
#define CCM_INTERVAL_1_MINUTE	6
#define CCM_INTERVAL_10_MINUTE	7

// defect bit map due to recv pkt from other mep, defined in 802.1ag & omci 9.3.22
#define DEFECT_RDI_CCM			0	// pri 1, recv ccm with rdi set, within 3.5 interval
#define DEFECT_MAC_STATUS		1	// pri 2, recv ccm with mac port status err, within 3.5 interval
#define DEFECT_REMOTE_CCM		2	// pri 3, ccm not received for more than 3.5 interval
#define DEFECT_ERROR_CCM		3	// pri 4, recv invalid ccm, within 3.5 interval (eg: sender mepid == receiver mepid)
#define DEFECT_XCON_CCM			4	// pri 5, recv ccm with maid/md unmatched, within 3.5 interval
#define DEFECT_UNEXP_PERIOD		5	// pri 4, recv ccm with different interval than us, within 3.5 interval
#define DEFECT_AIS			6	// recv any AIS from other mep, within 3.5 ais interval
#define DEFECT_INVALID			7	// invalid max for loop iteration

#define PRIORITY_DEFECT_RDI_CCM		1
#define PRIORITY_DEFECT_MAC_STATUS	2
#define PRIORITY_DEFECT_REMOTE_CCM	3
#define PRIORITY_DEFECT_ERROR_CCM	4
#define PRIORITY_DEFECT_XCON_CCM	5
#define PRIORITY_DEFECT_UNEXP_PERIOD	4
#define PRIORITY_DEFECT_AIS		6

// alarm fault threshold for defect bitmap
#define ALARM_THRESHOLD_0		0x3e	// UNEXP_PERIOD +XCON_CCM +ERROR_CCM +REMOTE_CCM +MAC_STATUS
#define ALARM_THRESHOLD_1		0x7f	// AIS +UNEXP_PERIOD +XCON_CCM +ERROR_CCM +REMOTE_CCM +MAC_STATUS +RDI_CCM
#define ALARM_THRESHOLD_2		0x3e	// UNEXP_PERIOD +XCON_CCM +ERROR_CCM +REMOTE_CCM +MAC_STATUS, 802.1ag default
#define ALARM_THRESHOLD_3		0x3c	// UNEXP_PERIOD +XCON_CCM +ERROR_CCM +REMOTE_CCM
#define ALARM_THRESHOLD_4		0x38    // UNEXP_PERIOD +XCON_CCM +ERROR_CCM
#define ALARM_THRESHOLD_5		0x10	// XCON_CCM
#define ALARM_THRESHOLD_6		0x0	// none

#define OPTION_TLV_SKIP			0
#define OPTION_TLV_DATA			1
#define OPTION_TLV_TEST			2

typedef struct {
	// Global values per MEP
	struct list_head node;
	unsigned char index;
	unsigned char port_type;
	unsigned char port_type_index;
	unsigned char logical_port_id;
	unsigned char macaddr[6];
	unsigned char auto_discovery;
	int ccm_send_timer_id;
	unsigned int timer_qid;          // Each MEP maintains a timer to send CCM periodically
	unsigned int timer_event_qid;      // Each MEP maintains a timer to manage timer event
	unsigned int nextLBMtransID; // For LBM
	unsigned int nextLTMtransID; // For LTM
	unsigned int CCIsentCCMs;    // For CCM
	unsigned int tst_sequence_number;    // For TST
	unsigned short tlv_tst_length;
	unsigned char tlv_tst_pattern_type;
	unsigned short tlv_data_length; //for Data TLV format
	unsigned char payload_format;	//DATA_TLV=0, TEST_TLV=1
	unsigned int payload_pattern; //for Data TLV pattern
	unsigned char send_lb_senderid;
	unsigned char send_lt_senderid;
	unsigned char send_cc_senderid;
	unsigned char send_lb_port_status;
	unsigned char send_lt_port_status;
	unsigned char send_cc_port_status;
	unsigned char send_lb_interface_status;
	unsigned char send_lt_interface_status;
	unsigned char send_cc_interface_status;
	unsigned char send_lb_data_status;
	unsigned char send_lt_data_status;
	unsigned char send_cc_data_status;

	unsigned char cfm_send_lmm_mac[IFHWADDRLEN];
	unsigned char cfm_send_dmm_mac[IFHWADDRLEN];
	unsigned char cfm_recv_dmr_mac[IFHWADDRLEN];
	unsigned long long cfm_send_lbm_timestamp;
	unsigned long long cfm_send_ltm_timestamp;
	unsigned long long cfm_send_ais_timestamp;
	unsigned long long cfm_send_lck_timestamp;
	unsigned long long TxTimestampf; // For 1DM/DMM/DMR
	unsigned long long RxTimestampf; // For 1DM/DMM/DMR
	unsigned long long TxTimestampb; // For DMM/DMR
	unsigned long long RxTimestampb; // For DMM/DMR
	unsigned long long frame_delay; // For DMM/DMR

	unsigned long long frame_delay_min; // For DMM/DMR
	unsigned long long frame_delay_max; // For DMM/DMR
	unsigned long long frame_delay_avg; // For DMM/DMR
	unsigned long long round_trip_min; // For DMM/DMR
	unsigned long long round_trip_max; // For DMM/DMR
	unsigned long long round_trip_avg; // For DMM/DMR
	unsigned long long response_time_min; // For DMM/DMR
	unsigned long long response_time_max; // For DMM/DMR
	unsigned long long response_time_avg; // For DMM/DMR

	struct list_head cfm_recv_lbr_list;
	struct list_head cfm_recv_ltm_list;
	struct list_head cfm_recv_ltr_list;
	struct list_head cfm_recv_rmep_list;
	// while opcode is unsigned char, but no more than 55 is actually used...
	// so we use size 64 array for all opcodes
	cfm_pm_counter_t cfm_pkt_pm[CFM_PDU_MAX+1];

	// From 9.3.19 Dot1ag maintenance domain (ME 299, MD)
	unsigned short md_meid;
	unsigned char md_level;
	unsigned char md_format;
	unsigned char md_name_1[25];	// 25 byte may be fill with non-NULL char
	unsigned char md_name_2[25];
	unsigned char md_mhf_creation;
	unsigned char md_sender_id_permission;

	// From 9.3.20 Dot1ag maintenance association (ME 300, MA)
	unsigned short ma_meid;
	unsigned char ma_format;
	unsigned char ma_name_1[25];	// 25 byte may be fill with non-NULL char
	unsigned char ma_name_2[25];
	unsigned char ccm_interval;
	unsigned short assoc_vlans[12];
	unsigned char ma_mhf_creation;
	unsigned char ma_sender_id_permission;

	// From 9.3.22 Dot1ag MEP (ME 302, MEP)
	unsigned short mep_meid;
	unsigned short mep_id;
	unsigned char mep_control;	// bits defined by omci
	unsigned char mep_control2;	// extension bits defined by 5vt
	unsigned short pri_vlan;
	unsigned char pri_pbit;
	unsigned char pbit_filter;
	unsigned char admin_state;
	unsigned char priority;
	unsigned char egress_id[8];
	unsigned short peer_mep_ids[12];	// the list of expected rmepid, it might differ than real rmepid
	unsigned char eth_ais_control;	// b0: ais interval, 0 means 1sec, 1 means 1min, b3..1: pbit for AIS, b6..4:md level of client mep
	unsigned char fault_alarm_threshold;	//
	unsigned short alarm_declaration_soak_time;	// def: 2.5 * 1000 ms
	unsigned short alarm_clear_soak_time;		// def: 10 * 1000ms

	// From 9.3.23 Dot1ag MEP status
	unsigned char highest_priority_defect_observed;
	unsigned char last_recv_err_ccm_msg_len;
	unsigned char last_xcon_err_ccm_msg_len;
	unsigned char last_recv_err_ccm_msg[128];
	unsigned char last_xcon_err_ccm_msg[128];
	unsigned long invalid_senderid_ccm_count;
	unsigned long invalid_port_status_ccm_count;
	unsigned long invalid_interface_status_ccm_count;
	unsigned long out_of_seqence_ccm_count;
	unsigned long unexpected_ltr_count;

	// data stru for defect/alarm & rdi ///////////////////////////

	// defect timestamp is set when related ccm/ais pkt is received
	unsigned long long defect_rdi_ccm_timestamp;
	unsigned long long defect_mac_status_timestamp;
	unsigned long long defect_error_ccm_timestamp;
	unsigned long long defect_xcon_ccm_timestamp;
	unsigned long long defect_unexp_period_timestamp;
	unsigned long long defect_ais_timestamp;
	unsigned long long recv_lck_timestamp;		// recv lck similar to ais, but not a defect

	unsigned short lm_dual_test_rmep;
	unsigned char lmm_dual_test_enable;

	// the defect_status/defect_status_timestamp will be set after defect timeout check is done
	unsigned char defect_status;			// per bit for one defect
	unsigned long long defect_status_timestamp[7];	// time stamp when related defect status changed
	// From 9.3.22 alarm table (alarm number 0..7)
	unsigned char alarm_status;			// per bit for one alarm
	// we need a 1 sec alarm timer to do defect timeout check, change alarm status, show alarm, send AIS
	//    if bit change from 0 to 1 for more than 2.5s(alarm_declaration_soak_time), set corresponding bit in alarm_status
	//    if bit change from 1 to 0 for more than 10s(alarm_clear_soak_time), clear corresponding bit in alarm_status
	//    if alarm != prev alarm,  show alarm to console
	//    if alarm !=0, send AIS
} cfm_config_t;

///////////////////////////////////////////////////////////////////////////////

extern cfm_chassis_t cfm_chassis_g;
extern struct list_head cfm_config_list;
extern cfm_config_t server_mep_cfm_config_g;
extern unsigned int server_mep_logical_portmask_g;

/* cfm_mp.c */
int cfm_config_list_init(void);
int cfm_config_list_release(void);
cfm_config_t *cfm_mp_add(unsigned char md_level, unsigned short mep_id, unsigned char port_type, unsigned char port_type_index);
cfm_config_t *cfm_mp_find(unsigned char md_level, unsigned short mep_id, unsigned char port_type, unsigned char port_type_index);
int cfm_mp_del(unsigned char md_level, unsigned short mep_id, unsigned char port_type, unsigned char port_type_index);
int cfm_mp_del_all(void);
int cfm_mp_flush_lbr_list(cfm_config_t *cfm_config);
int cfm_mp_flush_ltm_list(cfm_config_t *cfm_config);
int cfm_mp_flush_ltr_list(cfm_config_t *cfm_config);
int cfm_mp_flush_rmep_list(cfm_config_t *cfm_config);
int cfm_mp_add_peer_mep_ids_to_rmep_list(cfm_config_t *cfm_config);
int cfm_mp_is_peer_mep_id(cfm_config_t *cfm_config, unsigned short rmepid);
int cfm_mp_update_defect_status(cfm_config_t *cfm_config);
int cfm_mp_update_alarm_status(cfm_config_t *cfm_config);
int cfm_mp_check_interval_and_send_ais_lck(cfm_config_t *cfm_config);

#endif
