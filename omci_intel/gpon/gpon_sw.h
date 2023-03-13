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
 * Module  : gpon
 * File    : gpon.h
 *
 ******************************************************************/

// this file is renamed from gpon.h to gpon_sw.h as there is a gpon.h in directory referred by chipdef.h
#ifndef __GPON_H__
#define __GPON_H__

#include "chipdef.h"

#define GPON_VENDOR_ID_LEN 		4
#define GPON_VENDOR_SPECIFIC_LEN	4
#define GPON_ONU_SN_LEN			(GPON_VENDOR_ID_LEN+GPON_VENDOR_SPECIFIC_LEN)

#define GPON_PASSWORD_LEN		10
#define GPON_ONU_REGISTRATION_ID_LEN	36

#define GPON_SSHD_FLOW_ID 		(CHIP_GPON_OMCI_FLOW_ID-1)
#define GPON_OMCI_FLOW_ID 		CHIP_GPON_OMCI_FLOW_ID
#define GPON_OMCI_PQ_ID 		CHIP_GPON_OMCI_PQ_ID
#define GPON_OMCI_TCONT_ID		CHIP_GPON_OMCI_TCONT_ID

#define GPON_TCONT_ID_TOTAL		CHIP_GPON_TCONT_MAX
#define GPON_PQ_ID_TOTAL		CHIP_MAX_NUM_OF_PON_QUEUE
#define GPON_FLOW_ID_TOTAL		CHIP_GPON_FLOW_MAX

#define GPON_DS_FLOW_ID_TOTAL		GPON_FLOW_ID_TOTAL
#define GPON_US_FLOW_ID_TOTAL		GPON_FLOW_ID_TOTAL
#define GPON_FLOW_8021P_MAP_TOTAL		(GPON_FLOW_ID_TOTAL/8)

//#define GPON_TCONT_ID_PER_GROUP	(GPON_TCONT_ID_TOTAL/CHIP_GPON_TCONT_GROUP)
//#define GPON_PQ_ID_PER_GROUP		(GPON_PQ_ID_TOTAL/CHIP_GPON_TCONT_GROUP)
// for 9607: each group has equal tcont/pq
// for 9601b/9602c: last group has only one tcont/pq specifically for omci
#define GPON_TCONT_ID_PER_GROUP		31
#define GPON_PQ_ID_PER_GROUP		CHIP_PONMAC_TCONT_QUEUE_MAX

enum gpon_onu_state_e {
	GPON_ONU_STATE_UNKNOWN = 0,      /* Unknown State: Get Status fail */
	GPON_ONU_STATE_O1,               /* Initial State */
	GPON_ONU_STATE_O2,               /* Standby State */
	GPON_ONU_STATE_O3,               /* Serial Number State */
	GPON_ONU_STATE_O4,               /* Ranging State */
	GPON_ONU_STATE_O5,               /* Operation State */
	GPON_ONU_STATE_O6,               /* Intermittent LODS State */
	GPON_ONU_STATE_O7,               /* Emergency Stop State */
	GPON_ONU_STATE_O8,               /* Downstream Tunning State */
	GPON_ONU_STATE_O9                /* Upstream Tunning State */
};

enum gpon_onu_sdsf_e {
	GPON_ONU_SDSF_UNDER_CLEAR = 0,
	GPON_ONU_SDSF_UNDER = 1,
	GPON_ONU_SDSF_OVER = 2,
};

enum gpon_tm_policy_e {
	GPON_TM_POLICY_SP = 0,
	GPON_TM_POLICY_WRR,
	GPON_TM_POLICY_INVALID,
};

enum gpon_tm_flow_type_e {
	GPON_TM_FLOW_TYPE_OMCI = 0,
	GPON_TM_FLOW_TYPE_ETH,
	GPON_TM_FLOW_TYPE_TDM,
	GPON_TM_FLOW_TYPE_INVALID,
};

enum gpon_pm_global_index 
{
//gpon_hw_fvt2510_onu_counter_global_get(struct gpon_counter_global_t *counter_global)
	RX_SN_REQ=0,
	RX_RANGING_REQ,
	//
	RX_BIP_ERR_BIT,
	RX_BIP_ERR_BLOCK,
	RX_FEC_CORRECT_BIT,
	RX_FEC_CORRECT_BYTE,
	RX_FEC_CORRECT_CW,
	RX_FEC_UNCOR_CW,
	RX_LOM,
	RX_PLEN_ERR,
	//
	RX_PLOAM_CNT,
	RX_PLOAM_ERR,
	RX_PLOAM_CORRECTTED,
	RX_PLOAM_PROC,
	RX_PLOAM_OVERFLOW,
	//
	RX_BWMAP_CNT,
	RX_BWMAP_CRC_ERR,
	RX_BWMAP_OVERFLOW,
	RX_BWMAP_INV0,
	RX_BWMAP_INV1,
	//
	RX_GEM_LOS,
	RX_GEM_IDLE,
	RX_GEM_NON_IDLE,
	RX_HEC_CORRECT,
	RX_OVER_INTERLEAVING,
	RX_GEM_LEN_MIS,
	RX_MATCH_MULTI_FLOW,
	//
	RX_ETH_UNICAST,
	RX_ETH_MULTICAST,
	RX_ETH_MULTICAST_FWD,
	RX_ETH_MULTICAST_LEAK,
	RX_ETH_FCS_ERR,
	//
	RX_OMCI,
	RX_OMCI_CRC_ERR,
	RX_OMCI_PROC,
	RX_OMCI_DROP,
	//
	TX_BOH_CNT,
	//
	TX_DBRU_CNT,
	//	
	TX_PLOAM_CNT,
	TX_PLOAM_PROC,
	TX_PLOAM_URG,
	TX_PLOAM_URG_PROC,
	TX_PLOAM_NOR,
	TX_PLOAM_NOR_PROC,
	TX_PLOAM_SN,
	TX_PLOAM_NOMSG,
	//
	TX_GEM_CNT,
	TX_GEM_BYTE,
	//
	TX_ETH_ABORT_EBB,
	//
	TX_OMCI_PROC,
	GPON_PM_GLOBAL_INDEX_MAX,
};

enum gpon_pm_tcont_index 
{
//gpon_hw_fvt2510_onu_counter_tcont_get(int tcont_id, struct gpon_counter_tcont_t *counter_tcont)
	TX_GEM_PKT=0,
	//	
	TX_ETH_PKT,
	//
	TX_IDLE_BYTE,
	//
	GPON_PM_TCONT_INDEX_MAX,
};

enum gpon_pm_dsflow_index 
{
//gpon_hw_fvt2510_onu_counter_dsflow_get(int dsflow_id, struct gpon_counter_dsflow_t *counter_dsflow)
	RX_GEM_BLOCK=0,
	RX_GEM_BYTE,
	//
	RX_ETH_PKT,
	RX_ETH_PKT_FWD,
	GPON_PM_DSFLOW_INDEX_MAX,
};

enum gpon_pm_usflow_index 
{
//gpon_hw_fvt2510_onu_counter_usflow_get(int usflow_id, struct gpon_counter_usflow_t *counter_usflow)
	TX_GEM_BLOCK=0,
	TX_GEM_BYTE_1,
	//
	TX_ETH_CNT,
	//
	GPON_PM_USFLOW_INDEX_MAX,
};

struct gpon_onu_alarm_event_t {
	unsigned int los:1;
	unsigned int lof:1;
	unsigned int lom:1;
#if defined(CONFIG_SDK_PRX126) || defined CONFIG_SDK_PRX120 || defined CONFIG_SDK_PRX321
	unsigned int lol:1;
#endif
};

struct gpon_tm_pq_config_t {
	unsigned int tcont_id;
	unsigned short ts_id;		// for 2 level tm
	unsigned short sp_priority:12;	// for 2 level tm
	unsigned short wrr_weight:8;
	unsigned int enable:1;
	unsigned int policy:1;		// 0:sp, 1:wrr (for 1 level tm)
	unsigned int egress_drop:1;	// 2510 specific
	unsigned int pir;		// Specify the PIR rate unit 8kbps, APOLLOMP_PONMAC_PIR_CIR_RATE_MAX (0x1FFFF)
	unsigned int cir;		// Specify the CIR rate unit 8kbps, APOLLOMP_PONMAC_PIR_CIR_RATE_MAX (0x1FFFF)
};

struct gpon_tm_ts_config_t {			// for 2 level tm
	unsigned short tcont_id;
	unsigned short ts_ptr;	
	unsigned char policy;		// 0:sp, 1:wrr
	unsigned char priority_weight;

};

struct gpon_tm_tcont_config_t {		// for 2 level tm
	unsigned short me_id;
	unsigned short policy;		// 0:sp, 1:wrr
	unsigned short alloc_id;	// 0:sp, 1:wrr
};


struct gpon_tm_dsflow_config_t {
	unsigned short gemport;
	unsigned short flow_type:1;	// 0:omci, 1:ether, 2:tdm
	unsigned short aes_enable:1;	
	unsigned short mcast_enable:1;
	unsigned short enable:1;
};

struct gpon_tm_usflow_config_t {
	unsigned short gemport;
	unsigned short flow_type:1;	// 0:omci, 1:ether, 2:tdm
	unsigned short enable:1;
};

struct gpon_counter_global_t {
	unsigned long long rx_sn_req;
	unsigned long long rx_ranging_req;
	unsigned long long rx_bip_err_bit;
	unsigned long long rx_bip_err_block;
	unsigned long long rx_fec_correct_bit;
	unsigned long long rx_fec_correct_byte;
	unsigned long long rx_fec_correct_cw;
	unsigned long long rx_fec_uncor_cw;
	unsigned long long rx_lom;		// Superframe LOS
	unsigned long long rx_plen_err;
	unsigned long long rx_ploam_cnt;
	unsigned long long rx_ploam_err;
	unsigned long long rx_ploam_correctted;
	unsigned long long rx_ploam_proc;
	unsigned long long rx_ploam_overflow;
	unsigned long long rx_ploam_unknown;
	unsigned long long rx_bwmap_cnt;
	unsigned long long rx_bwmap_crc_err;
	unsigned long long rx_bwmap_overflow;
	unsigned long long rx_bwmap_inv0;	// Invalid BwMap 0
	unsigned long long rx_bwmap_inv1;	// Invalid BwMap 1
	unsigned long long rx_gem_los;
	unsigned long long rx_gem_idle;
	unsigned long long rx_gem_non_idle;
	unsigned long long rx_hec_correct;
	unsigned long long rx_over_interleaving;
	unsigned long long rx_gem_len_mis;
	unsigned long long rx_match_multi_flow;
	unsigned long long rx_eth_unicast;
	unsigned long long rx_eth_multicast;
	unsigned long long rx_eth_multicast_fwd;
	unsigned long long rx_eth_multicast_leak;
	unsigned long long rx_eth_fcs_err;
	unsigned long long rx_omci;
	unsigned long long rx_omci_crc_err;
	unsigned long long rx_omci_proc;	// Processed
	unsigned long long rx_omci_drop;
	unsigned long long tx_boh_cnt;
	unsigned long long tx_dbru_cnt;
	unsigned long long tx_ploam_cnt;
	unsigned long long tx_ploam_proc;
	unsigned long long tx_ploam_urg;
	unsigned long long tx_ploam_urg_proc;
	unsigned long long tx_ploam_nor;
	unsigned long long tx_ploam_nor_proc;
	unsigned long long tx_ploam_sn;
	unsigned long long tx_ploam_nomsg;
	unsigned long long tx_gem_cnt;
	unsigned long long tx_gem_byte;
	unsigned long long tx_eth_abort_ebb;	// Aborted Packet
	unsigned long long tx_omci_proc;
};

struct gpon_counter_tcont_t {
	unsigned long long tx_gem_pkt;
	unsigned long long tx_eth_pkt;
	unsigned long long tx_idle_byte;
};

struct gpon_counter_dsflow_t {
	unsigned long long rx_gem_block;	// packet
	unsigned long long rx_gem_byte;
	unsigned long long rx_eth_pkt;
	unsigned long long rx_eth_pkt_fwd;
};

struct gpon_counter_usflow_t {
	unsigned long long tx_gem_block;	// packet
	unsigned long long tx_gem_byte;
	unsigned long long tx_eth_cnt;
};
/** IEEE 802.1p Mapper Service Profile update data structure */
struct gpon_dot1p_mapper_update_data {
	/** TP pointer */
	unsigned short tp_pointer;
	/**
	 * Array of Interworking TP pointers for 802.1p priority assignment
	 * - X: GEM port network CTP connectivity
	 * - pointer: GEM Port ID value
	 */
	unsigned short *gem_port_id;
	/** Unmarked frame option */
	unsigned char unmarked_frame_opt;
	/** DSCP to P-bit mapping */
	unsigned char *dscp_prio_map;
	/** Default P-bit marking */
	unsigned char default_priority_marking;
	/** TP type */
	unsigned char tp_pointer_type;
};

struct gem_port_net_ctp_update_data {
	/** GEM Port ID value */
	unsigned short gem_port_id;
	/** T-CONT pointer */
	unsigned short tcont_ptr;
	/** Direction */
	unsigned char direction;
	/** Upstream priority queue (Upstream Traffic management pointer) */
	unsigned short us_priority_queue_ptr;
	/** Value of "Drop precedence color marking" attribute of the Priority
	 *  Queue-G pointed by "Upstream traffic management pointer"
	 */
	unsigned char us_priority_queue_drop_precedence_color_marking;
	/** Value of "Egress color marking attribute" of the Traffic
	 *  Descriptor pointed by "Upstream traffic descriptor profile".
	 *  Set to 255 if "Upstream traffic descriptor profile" is NULL
	 */
	unsigned char us_traffic_descriptor_egress_color_marking;
	/** Traffic descriptor profile pointer for upstream */
	unsigned short us_traffic_descriptor_profile_ptr;
	/** Priority queue pointer for downstream */
	unsigned short ds_priority_queue_ptr;
	/** Traffic descriptor profile pointer for downstream */
	unsigned short ds_traffic_descriptor_profile_ptr;
	/** Enable/Disable Multicast Control */
	unsigned char mcc_enable;
	/** Encryption key ring */
	unsigned char encryption_key_ring;
};
struct gem_bridge_port_data {
	/** Managed Entity ID */
	unsigned short me_id;
	/** Termination point type identifier */
	unsigned char tp_type;
	/** Termination point pointer */
	unsigned short tp_ptr;
};

/** This structure contains bridge configuration information */
struct gem_bridge_data {
	/** Managed Entity ID */
	unsigned short me_id;
	/** List of bridge ports connected to the bridge */
	struct gem_bridge_port_data port[16];
	/** Number of bridge ports connected to the bridge */
	unsigned short count;
};

struct gem_pmapper_data {
	/** ME Identifier */
	unsigned short me_id;
	/** Bridge ME identifier */
	unsigned short  bridge_port_me_id[8];
	unsigned short  bridge_me_id[8];
	/** Termination point pointer */
	unsigned short count;
	struct gem_bridge_data gembridge[8];
};
/** GEM interworking TP update structure */
struct gpon_gem_interworking_tp_update_data {
	/** Non-GEM function that is being interworked */
	unsigned char interworking_option;
	/** Service profile pointer */
	unsigned short service_profile_pointer;
	/** Interworking termination point pointer */
	unsigned short interworking_tp_pointer;
	/** GAL loopback configuration */
	unsigned char gal_loopback_configuration;
	unsigned short gal_payload_len;
	unsigned short gem_port_id;
	struct gem_bridge_data bridge_data;
};


// tcont_id: 0..31
// pq_id: 0..127, 127 for omci
// flowid: 0..127

struct gpon_hw_t {
	int (*init)(void);
	int (*shutdown)(void);
	int (*i2c_open)(int i2c_port);
	int (*i2c_read)(int i2c_port, unsigned int devaddr, unsigned short regaddr, void *buff, int len);
	int (*i2c_write)(int i2c_port, unsigned int devaddr, unsigned short regaddr, void *buff, int len);
	int (*i2c_close)(int i2c_port);
	int (*eeprom_open)(int i2c_port, unsigned int devaddr);
	int (*eeprom_data_get)(int i2c_port, unsigned int devaddr, unsigned short regaddr, void *buff, int len);
	int (*eeprom_data_set)(int i2c_port, unsigned int devaddr, unsigned short regaddr, void *buff, int len);
	int (*onu_mgmt_mode_get)(int *mgmt_mode);
	int (*onu_mgmt_mode_set)(int mgmt_mode);
	int (*onu_serial_number_get)(char *vendor_id, unsigned int *serial_no);
	int (*onu_serial_number_set)(char *vendor_id, unsigned int *serial_no);
	int (*onu_password_get)(char *password);
	int (*onu_password_set)(char *password);
	int (*onu_activate)(unsigned int state_o1_o7);
	int (*onu_deactivate)(void);
	int (*onu_state_get)(unsigned int *state);
	int (*onu_id_get)(unsigned short *id);
	int (*onu_alarm_event_get)(struct gpon_onu_alarm_event_t *alarm_event);
	int (*onu_gem_blocksize_get)(int *blocksize);
	int (*onu_gem_blocksize_set)(int blocksize);
	int (*onu_sdsf_get)(int sd_threshold_ind, int sf_threshold_ind, int *sd_state, int *sf_state);
	int (*onu_superframe_seq_get)(unsigned long long *superframe_seq);
	int (*counter_global_get)(struct gpon_counter_global_t *counter_global);
	int (*counter_tcont_get)(int tcont_id, struct gpon_counter_tcont_t *counter_tcont);
	int (*counter_dsflow_get)(int dsflow_id, struct gpon_counter_dsflow_t *counter_dsflow);
	int (*counter_usflow_get)(int usflow_id, struct gpon_counter_usflow_t *counter_usflow);
	int (*pm_refresh)(int is_reset);
	int (*pm_global_get)(struct gpon_counter_global_t *counter_global);
	int (*pm_tcont_get)(int tcont_id, struct gpon_counter_tcont_t *counter_tcont);
	int (*pm_dsflow_get)(int dsflow_id, struct gpon_counter_dsflow_t *counter_dsflow);
	int (*pm_usflow_get)(int usflow_id, struct gpon_counter_usflow_t *counter_usflow);
	int (*tm_pq_config_get)(int pq_id, struct gpon_tm_pq_config_t *pq_config);
	int (*tm_pq_config_set)(int pq_id, struct gpon_tm_pq_config_t *pq_config,int create_only);
	int (*tm_pq_config_del)(int pq_id, struct gpon_tm_pq_config_t *pq_config);
	int (*tm_ts_config_get)(int ts_id, struct gpon_tm_ts_config_t *ts_config);
	int (*tm_ts_config_set)(int ts_id, struct gpon_tm_ts_config_t *ts_config,int create_only);
	int (*tm_ts_config_del)(int ts_id, struct gpon_tm_ts_config_t *ts_config);
	int (*tm_tcont_config_get)(struct gpon_tm_tcont_config_t *tcont_config);
	int (*tm_tcont_config_set)(struct gpon_tm_tcont_config_t *tcont_config,int create_only);
	int (*tm_tcont_config_del)(struct gpon_tm_tcont_config_t *tcont_config);
	int (*tm_tcont_add_by_allocid)(int allocid, int *tcont_id);
	int (*tm_tcont_get_by_allocid)(int allocid, int *tcont_id);
	int (*tm_tcont_del_by_allocid)(int allocid);
	int (*tm_dsflow_get)(int dsflow_id, struct gpon_tm_dsflow_config_t *dsflow_config);
	int (*tm_dsflow_set)(int dsflow_id, struct gem_port_net_ctp_update_data *dsflow_config);
	int (*tm_dsflow_del)(int dsflow_id, struct gem_port_net_ctp_update_data *dsflow_config);
	int (*tm_usflow_get)(int usflow_id, struct gpon_tm_usflow_config_t *usflow_config);
	int (*tm_usflow_set)(int usflow_id, struct gem_port_net_ctp_update_data *usflow_config);
	int (*tm_usflow_del)(int usflow_id, struct gem_port_net_ctp_update_data *usflow_config);
	int (*tm_usflow2pq_get)(int usflow_id, int *pq_id);
	int (*tm_usflow2pq_set)(int usflow_id, int pq_id);
	int (*tm_flow_8021p_set)(int meid, struct gpon_dot1p_mapper_update_data *upd_cfg,int is_set);
	int (*tm_flow_iwtp_set)(int meid, struct gpon_gem_interworking_tp_update_data *upd_cfg,int is_set);
	int (*tm_flow_mcastiwtp_set)(unsigned short meid, unsigned short ctp_ptr,int is_set);
	void (*util_protect_start)(unsigned int *state);
	void (*util_protect_end)(unsigned int state);
	int (*util_clearpq_dummy)(void);
	void (*util_clearpq)(unsigned int page_threshold);
	void (*util_reset_dbru)(void);
	void (*util_reset_serdes)(int msec);
	void (*util_recovery)(void);
	int (*util_omccmiss_detect)(void);
	int (*util_tcontmiss_detect)(void);
	int (*util_o2stuck_detect)(void);
	int (*util_idlebw_is_overalloc)(unsigned short idlebw_guard_parm[3]);
	int (*history_init)(void);
	int (*history_add)(void);
	int (*history_print)(int fd, int type);                  
};

extern struct gpon_hw_t gpon_hw_g;

/* gpon.c */
int gpon_null_tm_tcont_get_by_allocid(int allocid, int *tcont_id);
int gpon_null_tm_pq_config_get(int pq_id, struct gpon_tm_pq_config_t *pq_config);
int gpon_null_tm_pq_config_set(int pq_id, struct gpon_tm_pq_config_t *pq_config,int create_only);
int gpon_hw_register(struct gpon_hw_t *gpon_hw);
int gpon_hw_unregister(void);

#endif
