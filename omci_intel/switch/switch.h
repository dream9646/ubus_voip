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
 * Module  : switch
 * File    : switch.h
 *
 ******************************************************************/

#ifndef __SWITCH_H__
#define __SWITCH_H__

#include <netinet/in.h>
#include <netinet/ip.h>
#include "list.h"
#include "meinfo.h"
#include "cpuport.h"
#include "classf.h"
#include "vacl.h"
#include "fwk_mutex.h"
#include "er_group_hw_util.h"

// we use 32bit unsigned int to represent the logical portmask, so logical port total must <=32
#define SWITCH_LOGICAL_PORT_TOTAL	16

// override the SWITCH_LOGICAL_PORT_TOTAL if any hardware hint is available
#if   defined(CONFIG_SDK_APOLLOMP)
#undef SWITCH_LOGICAL_PORT_TOTAL
// uni=0..4, pon=5, cpu=6, wifi0=7, wifi1=8
#define	SWITCH_LOGICAL_PORT_TOTAL	9

#endif

#define SWITCH_IPMC_MODE_ASM            0
#define SWITCH_IPMC_MODE_SSM_INCLUDE    1
#define SWITCH_IPMC_MODE_SSM_EXCLUDE    2

#define SWITCH_MODE_AUTO	0
#define SWITCH_MODE_FORCE	1

#define SWITCH_NWAY_ENABLED	0
#define SWITCH_NWAY_DISABLED	1

#define SWITCH_PORT_SPEED_10M	0
#define SWITCH_PORT_SPEED_100M	1
#define SWITCH_PORT_SPEED_1000M 2
#ifdef CONFIG_SUBTYPE_RTL9601C
#define SWITCH_PORT_SPEED_2G5   5
#endif

#define SWITCH_PORT_HALF_DUPLEX 0
#define SWITCH_PORT_FULL_DUPLEX 1

#define	SWITCH_RET_ERR			-1
#define	SWITCH_RET_OK			0
#define	SWITCH_RET_ENTRY_NOTFOUND	1

#define SWITCH_DISABLED		0
#define SWITCH_ENABLED		1

#define UNKNOWN_MULTICAST_METER		CHIP_MAX_NUM_OF_METERING-2
#define UNKNOWN_UNICAST_METER		CHIP_MAX_NUM_OF_METERING-1
#define BROADCAST_METER			CHIP_MAX_NUM_OF_METERING-1

typedef enum switch_enable_e
{
    DISABLED = 0,
    ENABLED,
    RTK_ENABLE_END
} switch_enable_t;



enum stp_state_e {
	STP_DISABLE = 0,
	STP_LISTENING,
	STP_LEARNING,
	STP_FORWARDING,
	STP_BLOCK,
	STP_LINKDOWN,
	STP_OFF,
	STP_END,
};

enum flow_dir_e {
	DIR_INGRESS = 0,
	DIR_EGRESS,
	DIR_END,
};

typedef enum rate_storm_group_e
{
    STORM_GROUP_UNKNOWN_UNICAST = 0,
    STORM_GROUP_UNKNOWN_MULTICAST,
    STORM_GROUP_MULTICAST,
    STORM_GROUP_BROADCAST,
    STORM_GROUP_END
} rate_storm_group_t;

enum storm_type_e {
	STORM_UNKNOWN_UNICAST = 0,
	STORM_UNKNOWN_MULTICAST,
	STORM_MULTICAST,
	STORM_BROADCAST,
	STORM_DHCP,
	STORM_ARP,
	STORM_IGMP_MLD,
	STORM_UNKNOWN, // unknown unicast & unknown multicast
	STORM_END,	
};

// protocol type define in omci pre-assign filter 
enum protocol_type_e {
	PREASSIGN_FILTER_IPV4_MCAST = 0,
	PREASSIGN_FILTER_IPV6_MCAST,
	PREASSIGN_FILTER_IPV4_BCAST,
	PREASSIGN_FILTER_RARP,
	PREASSIGN_FILTER_IPX,
	PREASSIGN_FILTER_NETBEUI,
	PREASSIGN_FILTER_APPLE_TALK,
	PREASSIGN_FILTER_BRG_MNG,
	PREASSIGN_FILTER_ARP,
	PREASSIGN_FILTER_PPPOE_BCAST,
	PREASSIGN_FILTER_UNICAST, // custom define, out of omci scope
	PREASSIGN_FILTER_INVALID_TYPE,
};

enum qos_prisel_type_e {
	QOS_PRISEL_PORTBASED = 0,
	QOS_PRISEL_DOT1Q,
	QOS_PRISEL_DSCP,
	QOS_PRISEL_ACL,
	QOS_PRISEL_LUTFWD,
	QOS_PRISEL_SABASED,
	QOS_PRISEL_VLANBASED,
	QOS_PRISEL_SVLANBASED,
	QOS_PRISEL_L4BASED,
	QOS_PRISEL_END
};

enum forward_action_e {
	FORWARD_ACTION_FORWARD = 0,
	FORWARD_ACTION_DROP,
	FORWARD_ACTION_TRAP2CPU,
	FORWARD_ACTION_COPY2CPU,
	FORWARD_ACTION_TO_GUESTVLAN,
	FORWARD_ACTION_FLOOD_IN_VLAN,
	FORWARD_ACTION_FLOOD_IN_ALL_PORT,
	FORWARD_ACTION_FLOOD_IN_ROUTER_PORTS,
	FORWARD_ACTION_FORWARD_EXCLUDE_CPU,
	FORWARD_ACTION_DROP_EXCLUDE_RMA,
	FORWARD_ACTION_END
};

struct switch_port_info_t {
	unsigned char port_type; 	// 0:cpu(veip), 1:uni, 2:wan, 3:iphost, 4:ds bcast
	unsigned char port_type_index; 	// index within the same port type
	unsigned char logical_port_id;
	struct me_t *bridge_port_me;
};

struct switch_port_mac_config_t {
	unsigned int link;
	unsigned int force_mode; // valid in port config, not in port status
	unsigned int speed;
	unsigned int duplex;
	unsigned int nway;
	unsigned int tx_pause;
	unsigned int rx_pause;
};

struct switch_rate_control_config_t {
	unsigned char dir;
	unsigned int rate;
	unsigned int has_ifg;
	unsigned int fc_en;
	unsigned char enable; // rate control enable
};

struct switch_traffic_descriptor_t {
	unsigned char ingress_colour_marking;
	unsigned char meter_type;
	unsigned char egress_colour_marking;
	unsigned int cir;
	unsigned int cbs;
	unsigned int pir;
	unsigned int pbs;
};

union ipaddr {
	struct in_addr 		ipv4;
	struct in6_addr		ipv6;
};

struct switch_ipmcast_entry_t {
	union ipaddr src_ip;
	union ipaddr dst_ip;
};

struct switch_ipv4mcast_entry_t {
	union ipaddr dst_ip;
        unsigned char filter_mode;
	unsigned int src_list[8];
	unsigned int src_portmask[8];
	unsigned short vid;
};

struct switch_mac_tab_entry_t {
	unsigned short vlan;
	unsigned char mac_addr[6];
	unsigned char mac_mask[6];
	unsigned int port_bitmap;
	unsigned short age;
	unsigned short vid;
	unsigned char is_dynamic;
	unsigned char is_ivl;	// new added
	unsigned int fid;		// new added
	unsigned char auth;		// new added
	unsigned char sa_block;	// new added
	struct switch_ipmcast_entry_t ipmcast;	
};

#define IS_IPV6_ADDR(ip) ((ip).ipv6.s6_addr32[1] | (ip).ipv6.s6_addr32[2] | (ip).ipv6.s6_addr32[3])?1:0


// 52 MAC_bridge_port_PM 9.3.9
struct switch_bridge_port_pm_t {
	unsigned int forwarded_frame;
	unsigned int delay_exceeded_discard;
	unsigned int mtu_exceeded_discard;
	unsigned int received_frame;
	unsigned int received_and_discarded;
};

// 24 ethernet_pm 9.5.2
struct switch_ethernet_pm_t {
	unsigned int fcs_error;				// rx
	unsigned int excessive_collision_counter;	// tx
	unsigned int late_collision_counter;		// tx
	unsigned int frames_too_long;			// rx
	unsigned int buffer_overflows_on_receive;	// rx
	unsigned int buffer_overflows_on_transmit;	// tx
	unsigned int single_collision_frame_counter;	// tx
	unsigned int multiple_collisions_frame_counter;	// tx
	unsigned int sqe_counter;			// ?
	unsigned int deferred_transmission_counter;	// tx
	unsigned int internal_mac_transmit_error_counter; // tx
	unsigned int carrier_sense_error_counter;	// tx
	unsigned int alignment_error_counter;		// rx
	unsigned int internal_mac_receive_error_counter; //rx
};

// 89 ethernet_pm_2 9.5.3
struct switch_ethernet_pm2_t {
	unsigned int pppoe_filtered_frame_counter;	// rx
};

// 296 ethernet_pm_3 9.5.4			// uni port rx
struct switch_ethernet_frame_pm_t { 
	unsigned int drop_events;
	unsigned int octets;
	unsigned int packets;
	unsigned int broadcast_packets;
	unsigned int multicast_packets;
	unsigned int crc_errored_packets;	//(not use in eth_pm_data3, but in pm up/down stream)
	unsigned int undersize_packets;		//"etherStatsUndersizePkts"
	unsigned int fragments;		//"etherStatsFragments"(not use in pm up/down stream, but in pm_data3)
	unsigned int jabbers;		//"etherStatsOversizePkts"(the same as oversize_packets in eth_pm_data3)
	unsigned int packets_64_octets;
	unsigned int packets_65_to_127_octets;
	unsigned int packets_128_to_255_octets;
	unsigned int packets_256_to_511_octets;
	unsigned int packets_512_to_1023_octets;
	unsigned int packets_1024_to_1518_octets;
};

//9.3.30/9.3.31
// 322 ethernet_frame_pm_upstream 9.3.30	// bridge port us
// 321 ethernet_frame_pm_downstream 9.3.31	// bridge port ds
struct switch_ethernet_frame_pm_ud_t { 
	unsigned int drop_events;
	unsigned long long octets;
	unsigned long long packets;
	unsigned int broadcast_packets;
	unsigned int multicast_packets;
	unsigned int crc_errored_packets;
	unsigned int undersize_packets;
	unsigned int oversize_packets;
	unsigned int packets_64_octets;
	unsigned int packets_65_to_127_octets;
	unsigned int packets_128_to_255_octets;
	unsigned int packets_256_to_511_octets;
	unsigned int packets_512_to_1023_octets;
	unsigned int packets_1024_to_1518_octets;
};

struct switch_ethernet_frame_pm_ud_long_t { 
	unsigned long long drop_events;
	unsigned long long octets;
	unsigned long long packets;
	unsigned long long broadcast_packets;
	unsigned long long multicast_packets;
	unsigned long long crc_errored_packets;
	unsigned long long undersize_packets;
	unsigned long long oversize_packets;
	unsigned long long packets_64_octets;
	unsigned long long packets_65_to_127_octets;
	unsigned long long packets_128_to_255_octets;
	unsigned long long packets_256_to_511_octets;
	unsigned long long packets_512_to_1023_octets;
        unsigned long long packets_1024_to_1518_octets;
};

struct switch_eth_pm_data2_private_t {
	unsigned long long outoctets;
	unsigned long long outbroadcastpackets;
	unsigned long long outmulticastpackets;
	unsigned long long outunicastpackets;
	unsigned long long inunicastpackets;
	unsigned long long unipackets1519octets;
	unsigned long long inoctets;		//for calculate upstream speed
};

struct stp_designate_data_t { 
	unsigned long designated_bridge;
	unsigned long designated_root;
	unsigned int designated_port;
	unsigned int designated_cost;
};

struct switch_counter_global_t {
	unsigned long long dot1dTpLearnedEntryDiscards;
};

struct switch_counter_port_t {
	unsigned long long ifInOctets;
	unsigned long long ifInUcastPkts;
	unsigned long long ifInMulticastPkts;
	unsigned long long ifInBroadcastPkts;
	unsigned long long ifInDiscards;
	unsigned long long ifOutOctets;
	unsigned long long ifOutDiscards;
	unsigned long long ifOutUcastPkts;
	unsigned long long ifOutMulticastPkts;
	unsigned long long ifOutBroadcastPkts;	// sdk uses ifOutBrocastPkts
	unsigned long long dot1dBasePortDelayExceededDiscards;
	unsigned long long dot1dTpPortInDiscards;
	unsigned long long dot1dTpHcPortInDiscards;
	unsigned long long dot3InPauseFrames;
	unsigned long long dot3OutPauseFrames;
	unsigned long long dot3OutPauseOnFrames;
	unsigned long long dot3StatsAligmentErrors;
	unsigned long long dot3StatsFCSErrors;
	unsigned long long dot3StatsSingleCollisionFrames;
	unsigned long long dot3StatsMultipleCollisionFrames;
	unsigned long long dot3StatsDeferredTransmissions;
	unsigned long long dot3StatsLateCollisions;
	unsigned long long dot3StatsExcessiveCollisions;
	unsigned long long dot3StatsFrameTooLongs;
	unsigned long long dot3StatsSymbolErrors;
	unsigned long long dot3ControlInUnknownOpcodes;
	unsigned long long etherStatsDropEvents;
	unsigned long long etherStatsOctets;
	unsigned long long etherStatsBcastPkts;
	unsigned long long etherStatsMcastPkts;
	unsigned long long etherStatsUndersizePkts;
	unsigned long long etherStatsOversizePkts;
	unsigned long long etherStatsFragments;
	unsigned long long etherStatsJabbers;
	unsigned long long etherStatsCollisions;
	unsigned long long etherStatsCRCAlignErrors;
	unsigned long long etherStatsPkts64Octets;
	unsigned long long etherStatsPkts65to127Octets;
	unsigned long long etherStatsPkts128to255Octets;
	unsigned long long etherStatsPkts256to511Octets;
	unsigned long long etherStatsPkts512to1023Octets;
	unsigned long long etherStatsPkts1024to1518Octets;
	unsigned long long etherStatsTxOctets;
	unsigned long long etherStatsTxUndersizePkts;
	unsigned long long etherStatsTxOversizePkts;
	unsigned long long etherStatsTxPkts64Octets;
	unsigned long long etherStatsTxPkts65to127Octets;
	unsigned long long etherStatsTxPkts128to255Octets;
	unsigned long long etherStatsTxPkts256to511Octets;
	unsigned long long etherStatsTxPkts512to1023Octets;
	unsigned long long etherStatsTxPkts1024to1518Octets;
	unsigned long long etherStatsTxPkts1519toMaxOctets;
	unsigned long long etherStatsTxBroadcastPkts;	// sdk uses etherStatsTxBcastPkts
	unsigned long long etherStatsTxMulticastPkts;	// sdk uses etherStatsTxMcastPkts
	unsigned long long etherStatsTxFragments;
	unsigned long long etherStatsTxJabbers;
	unsigned long long etherStatsTxCRCAlignErrors;
	unsigned long long etherStatsRxUndersizePkts;
	unsigned long long etherStatsRxUndersizeDropPkts;
	unsigned long long etherStatsRxOversizePkts;
	unsigned long long etherStatsRxPkts64Octets;
	unsigned long long etherStatsRxPkts65to127Octets;
	unsigned long long etherStatsRxPkts128to255Octets;
	unsigned long long etherStatsRxPkts256to511Octets;
	unsigned long long etherStatsRxPkts512to1023Octets;
	unsigned long long etherStatsRxPkts1024to1518Octets;
	unsigned long long etherStatsRxPkts1519toMaxOctets;
	unsigned long long inOampduPkts;
	unsigned long long outOampduPkts;
};

struct switch_mib_t {
	struct switch_counter_global_t global;
	struct switch_counter_port_t port[9]; // 0~6:switch, 7~8:wifi
};

struct switch_pptp_eth_uni_data {
	/** Expected type */
	unsigned char expected_type;
	/** Auto detection configuration */
	unsigned char auto_detect_cfg;
	/** Ethernet loopback configuration */
	unsigned char ethernet_loopback;
	/** Max frame size */
	unsigned short max_frame_size;
	/** DTE or DCE ind */
	unsigned char dte_dce_ind;
	/** Pause time */
	unsigned short pause_time;
	/** Bridged or IP ind */
	unsigned char bridge_or_router_cfg;
	/** PPPoE filter */
	unsigned char pppoe_filter;
	/** Power control */
	unsigned char power_control;
};

struct switch_mc_profile_ext_vlan_update_data {
	unsigned short meid;
	/** LAN port index (0 to 3) */
	unsigned short lan_idx;
	/** Upstream multicast VLAN tag control */
	unsigned char us_igmp_tag_ctrl;
	/** Upstream multicast TCI value */
	unsigned short us_igmp_tci;
	/** Downstream multicast VLAN tag control */
	unsigned char ds_igmp_mc_tag_ctrl;
	/** Downstream multicast TCI value */
	unsigned short ds_igmp_mc_tci;
};
struct switch_mac_bp_config_data_upd_data {
	/** Bridge ID pointer */
	unsigned short bridge_id_ptr;
	/** Bridge port number */
	unsigned char port_num;
	/** Termination Point type */
	unsigned char tp_type;
	/** Termination Point pointer */
	unsigned short tp_ptr;
	/** Outbound Traffic Descriptor pointer */
	unsigned short outbound_td_ptr;
	/** Inbound Traffic Descriptor pointer */
	unsigned short inbound_td_ptr;
	/** MAC learning depth */
	unsigned char mac_learning_depth;
	/** IPv4 Multicast Control Enable/Disable flag */
	unsigned char mcc_ipv4_enabled;
	/** IPv6 Multicast Control Enable/Disable flag */
	unsigned char mcc_ipv6_enabled;
};

// external switch
struct switch_hw_t {
	/* switch_hw_trex_acl.c */
	//int (*acl_port_enable_set)(unsigned int port_id, unsigned int en);
	int (*acl_entry_delete)(unsigned int index);
	int (*acl_entry_enable_set)(unsigned int idx, unsigned int en);
	int (*acl_entry_flush)(void);
	int (*acl_entry_get_free_info)(unsigned int *entry_idx, unsigned int ending_idx, unsigned int *count);
	int (*acl_user_template_set)(unsigned int *field_typeid);
	int (*acl_user_template_get)(unsigned int *field_typeid);
	int (*acl_user_field_set)(unsigned int index, unsigned int field_format, unsigned int offset);
	int (*acl_user_field_get)(unsigned int index, unsigned int *field_format, unsigned int *offset);

	int (*vacl_init)(void);
	int (*vacl_shutdown)(void);
	int (*vacl_meter_clear_all)(void);
	int (*vacl_meter_commit_entry)(unsigned int bucket_size, unsigned int rate, unsigned int ifg_include, unsigned int index);
	int (*vacl_meter_get_all)(void);
	int (*vacl_entry_print)(int fd, unsigned int start_idx);
	int (*vacl_commit)(int *count);
	int (*vacl_clear)(void);
	int (*vacl_port_enable_set)(unsigned int port_id, unsigned int en);
	int (*vacl_port_permit_set)(unsigned int port_id, unsigned int en);
	int (*vacl_port_enable_print)(int fd);
	int (*vacl_port_permit_print)(int fd);
	int (*vacl_dump)(int fd, int hw_order, int order);
	int (*vacl_meter_clear)(int index);
	int (*vacl_rule_evaluate)(struct vacl_user_node_t *rule_p);
	int (*vacl_rule_valid_get)(int entry_index, int *valid_state);

	/* switch_hw_trex_classf.c */
	int (*classf_init)(void);
	int (*classf_manage)(struct classf_info_t *classf_info);
	int (*classf_print)(int fd, unsigned char type, unsigned char flag);
	int (*classf_gem_print)(int fd, struct classf_info_t *classf_info, int is_reset);
	int (*classf_stat_print)(int fd, struct classf_info_t *classf_info, unsigned char flag, unsigned short vid, unsigned char type, unsigned char dir);

	/* switch_hw_trex_init.c */
	int (*init)(void);
	int (*shutdown)(void);

	/* switch_hw_trex_int.c */
	int (*int_ctrl_set)(unsigned int type_idx, unsigned int enable);
	int (*int_ctrl_get)(unsigned int type_idx, unsigned int *enable);
	int (*int_ctrl_print)(int fd);
	int (*int_stat_set)(unsigned int type_idx);
	int (*int_stat_get)(unsigned int type_idx, unsigned int *result);
	int (*int_stat_print)(int fd);

	/* switch_hw_trex_ipmc.c */
	int (*ipmcastaddr_add)(struct switch_mac_tab_entry_t *mac_tab_entry);
	int (*ipmcastaddr_del)(struct switch_mac_tab_entry_t *mac_tab_entry);
	int (*ipmcastaddr_flush)(void);
	int (*ipmcastaddr_get)(struct switch_mac_tab_entry_t *mac_tab_entry);
	int (*ipmcastaddr_get_next)(struct switch_mac_tab_entry_t *mac_tab_entry, int *num);
	int (*ipmcast_mode_set)(void);
	int (*ipmcast_mode_clear)(void);
	int (*ipmcast_me309_mode_set)(unsigned short meid,unsigned short igmp_ver,int is_create);
	int (*ipmcast_me310_extvlan_set)(struct switch_mc_profile_ext_vlan_update_data *up_data);
	/* switch_hw_trex_mac.c */
	int (*mac_learning_limit_get)(unsigned int port, unsigned int *count, unsigned int *is_drop);
	int (*mac_learning_limit_set)(unsigned int port, unsigned int count, unsigned int is_drop);
	int (*mac_entry_add)(struct switch_mac_tab_entry_t *mac_tab_entry, int is_static);
	int (*mac_entry_del)(struct switch_mac_tab_entry_t *mac_tab_entry);
	int (*mac_entry_update)(struct switch_mac_tab_entry_t *mac_tab_entry);
	int (*mac_entry_trap_ps)(struct switch_mac_tab_entry_t *mac_tab_entry);
	int (*mac_entry_get)(struct switch_mac_tab_entry_t *mac_tab_entry);
	int (*mac_l2uc_get_next)(int port, unsigned int *pEntryIdx, struct switch_mac_tab_entry_t *mac_tab_entry);
	int (*mac_l2mc_get_next)(unsigned int *pEntryIdx, struct switch_mac_tab_entry_t *mac_tab_entry);
	int (*mac_ipmc_get_next)(unsigned int *pEntryIdx, struct switch_mac_tab_entry_t *mac_tab_entry);
	int (*mac_table_flush)(unsigned int portmap, int is_static);
	int (*mac_table_print_valid)(int fd, int type);
	int (*mac_aging_set)(unsigned int time);
	int (*mac_aging_get)(unsigned int *time);
	int (*mac_aging_enable_set)(unsigned int port, unsigned int enable);
	int (*mac_aging_enable_get)(unsigned int port, unsigned int *enable);
	int (*mac_flood_enable_set)(unsigned int port, unsigned int enable, unsigned int learning_ind);
	int (*mac_flood_enable_get)(unsigned int port, unsigned int *enable);
#if defined CONFIG_SDK_PRX126 || defined CONFIG_SDK_PRX120 || defined CONFIG_SDK_PRX321
	/*bridge px126*/
	int (*bridge_inf_create)(unsigned int brd_meid);
	int (*bridge_inf_del)(unsigned int brd_meid);
	int (*bridge_port_config_update)(unsigned short meid,struct switch_mac_bp_config_data_upd_data *upd_data,int is_del);
	int (*bridge_port_connect) (unsigned short br_meid,unsigned short br_port_meid,unsigned char tp_type,unsigned short tp_ptr);
#endif	
	/* switch_hw_trex_misc.c */
	int (*preassign_filter_get)(unsigned int portid, unsigned int protocolid, unsigned int *enable);
	int (*preassign_filter_set)(unsigned int portid, unsigned int protocolid, unsigned int enable);
	int (*igmp_upstream_rate_set)(unsigned int port_id, int rate);
	int (*igmp_upstream_rate_get)(unsigned int port_id, int *rate);
	int (*igmp_extract_enable_set)(unsigned int port_id, unsigned int enable);
	int (*igmp_extract_enable_get)(unsigned int port_id, unsigned int *enable);
	int (*dhcp_extract_enable_set)(unsigned int enable);
	int (*dhcp_extract_enable_get)(unsigned int *enable);
	int (*pppoe_extract_enable_set)(unsigned int enable);
	int (*pppoe_extract_enable_get)(unsigned int *enable);
	int (*cfm_extract_enable_set)(unsigned int enable);
	int (*cfm_extract_enable_get)(unsigned int *enable);
	int (*extoam_extract_enable_set)(unsigned int enable);
	int (*extoam_extract_enable_get)(unsigned int *enable);
	int (*eth_pppoe_filter_enable_set)(unsigned int portid, unsigned int enable);
	int (*eth_pppoe_filter_enable_get)(unsigned int portid, unsigned int *enable);
	int (*eth_pppoe_filter_enable_print)(int fd);
	int (*protocol_rate_limit_set)(unsigned int protocolid, unsigned int rate, unsigned int ifg_en);
	int (*protocol_rate_limit_get)(unsigned int protocolid, unsigned int *rate, unsigned int *ifg_en);
	int (*protocol_filter_set)(unsigned int portid, unsigned int protocolid, int enable);	// n/a
	int (*protocol_filter_get)(unsigned int portid, unsigned int protocolid, int *enable);// n/a
	int (*hw_usage_print)(int fd);
#if defined CONFIG_SDK_PRX126 || defined CONFIG_SDK_PRX120 || defined CONFIG_SDK_PRX321
	int (*pptp_lct_uni_create)(unsigned short meid);
	int (*pptp_lct_uni_update)(unsigned short meid, unsigned char state_admin);
	int (*pptp_lct_uni_destory)(unsigned short meid);
#endif

	/* switch_hw_trex_pm.c */
	int (*counter_global_get)(struct switch_counter_global_t *counter_global);
	int (*counter_port_get)(int port_id, struct switch_counter_port_t *counter_port);
	int (*pm_reset_port)(unsigned int logical_portid);
	int (*pm_refresh)(int is_reset);
	int (*pm_global_get)(struct switch_counter_global_t *counter_global);
	int (*pm_port_get)(int port_id, struct switch_counter_port_t *counter_port);
	int (*pm_summary_print)(int fd, int type);
	int (*bridge_port_pm_data)(unsigned int logical_port_id, struct switch_bridge_port_pm_t *bridge_port_pm);
	int (*eth_pm_data)(unsigned int port_id, struct switch_ethernet_pm_t *ethernet_pm);
	int (*eth_pm_data2)(unsigned int port, struct switch_ethernet_pm2_t *ethernet_pm2);
	int (*eth_pm_data3)(unsigned int port_id, struct switch_ethernet_frame_pm_t *ethernet_frame_pm);
	int (*eth_pm_data2_private)(unsigned int port_id, struct switch_eth_pm_data2_private_t *pm_data2_private);
	int (*eth_pm_data_upstream)(unsigned int port_id, struct switch_ethernet_frame_pm_ud_t *ethernet_frame_pmu);
	int (*eth_pm_data_downstream)(unsigned int port_id, struct switch_ethernet_frame_pm_ud_t *ethernet_frame_pmd);
	int (*eth_pm_data_upstream_long)(unsigned int port_id, struct switch_ethernet_frame_pm_ud_long_t *ethernet_frame_pmu);
	int (*eth_pm_data_downstream_long)(unsigned int port_id, struct switch_ethernet_frame_pm_ud_long_t *ethernet_frame_pmd);
	int (*eth_pm_rate_status_get)(int port_id, unsigned long long *rate_us, unsigned long long *rate_ds);

	/* switch_hw_trex_history.c */
	int (*history_init)(void);
	int (*history_update)(void);
	int (*history_add)(void);
	int (*history_print)(int fd, int type);

	/* switch_hw_trex_port.c */
	int (*port_config_set)(unsigned int port_id, struct switch_port_mac_config_t port_mac_config);
	int (*port_config_get)(unsigned int port_id, struct switch_port_mac_config_t *port_mac_config);
	int (*port_config_print)(int fd);
	int (*port_status_get)(unsigned int port_id, struct switch_port_mac_config_t *port_mac_status);
	int (*port_status_print)(int fd);
	int (*port_phylb_set)(unsigned int port_id, unsigned int enable);
	int (*port_phylb_get)(unsigned int port_id, unsigned int *enable);
	int (*port_phylb_print)(int fd);
	int (*port_phyiso_set)(unsigned int port_id, unsigned int enable);
	int (*port_phyiso_get)(unsigned int port_id, unsigned int *enable);
	int (*port_phyiso_print)(int fd);
	int (*port_eth_mtu_set)(unsigned int port_id, unsigned int mtu);
	int (*port_eth_mtu_get)(unsigned int port_id, unsigned int *mtu);
	int (*port_enable_set)(unsigned int port_id, unsigned int enable);
	int (*port_enable_get)(unsigned int port_id, unsigned int *enable);
	int (*port_testmode_set)(unsigned int port_id, unsigned int enable);
	int (*port_testmode_get)(unsigned int port_id, unsigned int *enable);
	int (*port_enable_print)(int fd);
	int (*port_autoneg_set)(unsigned int port_id, unsigned int autoneg_mode);
	int (*port_autoneg_get)(unsigned int port_id, unsigned int *autoneg_mode);
	int (*port_autoneg_print)(int fd);
#if defined CONFIG_SDK_PRX126 || defined CONFIG_SDK_PRX120 || defined CONFIG_SDK_PRX321	
	int (*port_inf_set)(unsigned short meid, int is_create);
	int (*port_inf_update)(unsigned short meid, struct switch_pptp_eth_uni_data *updata);
#endif
	int (*eth_address_set)(unsigned int port_id, unsigned char *ethaddr);
	int (*eth_address_get)(unsigned int port_id, unsigned char *ethaddr);
	int (*eth_address_print)(int fd);
	int (*dot1x_mode_set)(unsigned int port_id, unsigned int enable);
	int (*dot1x_mode_get)(unsigned int port_id, unsigned int *enable);
	int (*dot1x_state_set)(unsigned int port_id, unsigned int state);
	int (*dot1x_state_get)(unsigned int port_id, unsigned int *state);
	int (*dot1x_dir_set)(unsigned int port_id, unsigned int dir);
	int (*dot1x_dir_get)(unsigned int port_id, unsigned int *dir);
	int (*rma_action_set)(unsigned char address, unsigned int action);
	int (*rma_action_get)(unsigned char address, unsigned int *action);
	int (*port_phy_reg_print)(int fd, unsigned int port_id);
	int (*port_phy_reg_interpret_print)(int fd, unsigned char reg_id, unsigned short value);

	/* switch_hw_trex_qos.c */
	int (*qos_dot1p2pri_set_by_pri)(unsigned int org_pri, unsigned int rmp_pri);
	int (*qos_dot1p2pri_get_by_pri)(unsigned int org_pri, unsigned int *rmp_pri);
	int (*qos_dot1p2pri_print)(int fd);
	int (*qos_dscp2pri_set_by_pri)(unsigned int dscp, unsigned int rmp_pri);
	int (*qos_dscp2pri_get_by_pri)(unsigned int dscp, unsigned int *rmp_pri);
	int (*qos_dscp2pri_print)(int fd);
	int (*qos_p2q_set_by_pri)(unsigned int group_id, unsigned int pri, unsigned int qid);
	int (*qos_p2q_set_by_group)(unsigned int group_id, unsigned int *qid);
	int (*qos_p2q_group_set_by_port)(unsigned int port_id, unsigned int group_id);
	int (*qos_p2q_print)(int fd);
	int (*qos_portcfg_print)(int fd);
	int (*qos_port_priority_set)(unsigned int port_id, unsigned int pri);
	int (*qos_port_scheduled_weight_set_by_qid)(unsigned int port_id, unsigned int qid, unsigned int weight);
	int (*qos_meter_print)(int fd);
	int (*qos_meter_set_by_qid)(unsigned int port_id, unsigned int q_id, unsigned int meter_id);
	int (*qos_prisel_weight_set)(unsigned int port_id, unsigned int sel_type, unsigned int weight);
	int (*qos_prisel_weight_print)(int fd);
	int (*qos_ds_pbit_pq_map_get)(unsigned int port_id, unsigned int pbit, unsigned int *pq_id);
	int (*qos_ds_pbit_pq_map_set)(unsigned int port_id, unsigned int pbit, unsigned int pq_id);

	/* switch_hw_trex_rate.c */
	int (*rate_storm_set)(unsigned int port_id, unsigned int storm_type, unsigned int rate, unsigned int ifg_include);
	int (*rate_storm_get)(unsigned int port_id, unsigned int storm_type, unsigned int *rate, unsigned int *ifg_include);
	int (*rate_storm_enable_set)(unsigned int storm_type, unsigned int enable);
	int (*rate_storm_enable_get)(unsigned int storm_type, unsigned int *enable);
	int (*rate_storm_port_enable_set)(unsigned int port_id, unsigned int storm_type, unsigned int enable);
	int (*rate_storm_port_enable_get)(unsigned int port_id, unsigned int storm_type, unsigned int *enable);
	int (*rate_storm_print)(int fd, unsigned int port_id);
	int (*rate_meter_init)(void);
	int (*rate_meter_add)(unsigned int rate, unsigned int ifg_en, unsigned int *index);
	int (*rate_meter_del)(unsigned int index);
	int (*rate_meter_set)(unsigned int index, unsigned int rate, unsigned int ifg_en);
	int (*rate_meter_get)(unsigned int index, unsigned int *rate, unsigned int *ifg_en);
	int (*rate_meter_print)(int fd);
	int (*rate_meter_bucket_init)(void);
	int (*rate_meter_bucket_add)(unsigned int bucket_size, unsigned int *index);
	int (*rate_meter_bucket_del)(unsigned int index);
	int (*rate_meter_bucket_set)(unsigned int index, unsigned int bucket_size);
	int (*rate_meter_bucket_get)(unsigned int index, unsigned int *bucket_size);
	int (*rate_uni_constrain)(unsigned int mbps, unsigned int linkup_portmask);
	int (*rate_control_set)(unsigned int port_id, struct switch_rate_control_config_t rate_ctrl_cfg);
	int (*rate_control_get)(unsigned int port_id, struct switch_rate_control_config_t *rate_cfg);
	int (*rate_control_print)(int fd);
	int (*rate_status_print)(int fd);

	/* switch_hw_trex_rldp.c */
	int (*rldp_init)(void);
	int (*rldp_enable_set)(unsigned int enable);
	int (*rldp_enable_get)(unsigned int *enable);
	int (*rldp_time_period_set)(unsigned int interval_check, unsigned int interval_loop);
	int (*rldp_time_period_get)(unsigned int *interval_check, unsigned int *interval_loop);
	int (*rldp_portmask_set)(unsigned int portmask);
	int (*rldp_portmask_get)(unsigned int *portmask);
	int (*rldp_port_status_get)(unsigned int portid, unsigned int *loop_status, unsigned int *loop_enter, unsigned int *loop_leave, unsigned int *looped_portid);
	int (*rldp_port_status_clear)(unsigned int portid);
	int (*rldp_loopedportpair_get)(unsigned int portid, unsigned int *is_looped, unsigned int *port_pair);
	int (*rldp_loopedportmask_get)(unsigned int *loopedPortmask);
	int (*rldp_loop_print)(int fd);

	/* switch_hw_trex_stp.c */
	int (*stp_state_init)(void);
	int (*stp_state_shutdown)(void);
	int (*stp_designate_data_get)(unsigned int port, struct stp_designate_data_t *designate_data);
	int (*stp_state_print)(int fd);
	int (*stp_state_port_get)(unsigned int port, unsigned int *stp_state);
	int (*stp_state_port_set)(unsigned int port, unsigned int stp_state);

	/* switch_hw_trex_svlan.c */
	int (*svlan_service_port_set)(unsigned int port_id, unsigned int en);
	int (*svlan_tpid_set)(unsigned int tpid);
	int (*svlan_pvid_set)(unsigned int port_id, unsigned int svid);
	int (*svlan_entry_member_get)(unsigned int svid, unsigned int *mbr_msk, unsigned int *utg_msk);
	int (*svlan_entry_member_set)(unsigned int svid, unsigned int mbr_msk, unsigned int utg_msk);
	int (*svlan_entry_delete)(unsigned int svid);
	int (*svlan_entry_config_get)(unsigned int svid, unsigned int *pri, unsigned int *fid_en, unsigned int *fid, unsigned int *efid_en, unsigned int *efid);
	int (*svlan_entry_config_set)(unsigned int svid, unsigned int pri, unsigned int fid_en, unsigned int fid, unsigned int efid_en, unsigned int efid);
	int (*svlan_entry_print)(int fd);

	/* switch_hw_trex_switch.c */
	int (*hw_version_get)(unsigned int *family, unsigned int *version, unsigned int *rev, unsigned int *subtype);
	int (*port_isolation_set)(unsigned int srcport, unsigned int dstport_bitmap);
	int (*port_isolation_get)(unsigned int srcport, unsigned int *dstport_bitmap);
	int (*port_isolation_print)(int fd, int mode);
	int (*port_allowed)(int uni_port, unsigned int uni_mask);
	int (*local_bridging)(unsigned int en);
	int (*test_mode_set)(unsigned int port_id, unsigned int mode);
	int (*test_mode_get)(unsigned int port_id, unsigned int *mode);
	int (*unknown_mcast_drop_set)(unsigned int drop);
	int (*unknown_packet_action_set)(int portid, unsigned int pkt_type, unsigned int action);
	int (*unknown_packet_action_get)(int portid, unsigned int pkt_type, unsigned int *action);
	int (*unknown_packet_flood_mask_set)(int portid, unsigned int pkt_type, unsigned int action);
	int (*unknown_packet_flood_mask_get)(int portid, unsigned int pkt_type, unsigned int *action);
	int (*unknown_sa_action_set)(int portid, unsigned int action);
	int (*unknown_sa_action_get)(int portid, unsigned int *action);
	int (*over_maclimit_action_set)(int portid, unsigned int action);
	int (*over_maclimit_action_get)(int portid, unsigned int *action);
	int (*pause_quanta_set)(unsigned short quanta);	// n/a
	int (*pause_quanta_get)(unsigned short *quanta);	// n/a

	/* switch_hw_trex_trunk.c */
	int (*trunk_port_set)(unsigned int logical_portmask);
	int (*trunk_port_get)(unsigned int *logical_portmask);
	int (*trunk_dumbmode_set)(int enable);
	int (*trunk_dumbmode_get)(int *enable);
	int (*trunk_algorithm_set)(unsigned int algo_bitmask);
	int (*trunk_algorithm_get)(unsigned int *algo_bitmask);
	int (*trunk_portmask_config)(unsigned int trunk_portmask);

	/* switch_hw_trex_vlan.c */
	int (*vlan_enable)(unsigned int enable);
	int (*vlan_reserved_action)(unsigned int is_tag);
	int (*vlan_entry_print_by_vid)(int fd, unsigned int vid);
	int (*vlan_entry_default_set)(unsigned int vid);
	int (*vlan_entry_set)(unsigned int vid, unsigned int mbr_msk, unsigned int utg_msk);
	int (*vlan_entry_delete)(unsigned int vid);
	int (*vlan_entry_untagged_op)(unsigned int port_id, unsigned int fwd_op);
	int (*vlan_egress_mode_set)(unsigned int port, unsigned int mode, unsigned int egkeep_msk);
	int (*vlan_egress_mode_print)(int fd);
	int (*vlan_entry_get)(unsigned int vid, unsigned int *mbr_msk, unsigned int *utg_msk);
	int (*vlan_entry_flush)(unsigned int keep_default);
	int (*ipv4mcastaddr_add)(struct switch_ipv4mcast_entry_t *mac_tab_entry);
	int (*l2_impc_group_del)(unsigned int gip);
	int (*nat_session_hw_print)(int fd);
	int (*nat_session_sw_print)(int fd);

	/* switch_hw_trex_conv.c */
	int (*portid_logical_to_private)(unsigned int logical_portid, unsigned int *phy_portid, unsigned int *ext_portid);
	int (*portid_private_to_logical)(unsigned int *logical_portid, unsigned int phy_portid, unsigned int ext_portid);
	int (*portmask_logical_to_private)(unsigned int logical_portmask, unsigned int *phy_portmask, unsigned int *ext_portmask);
	int (*portmask_private_to_logical)(unsigned int *logical_portmask, unsigned int phy_portmask, unsigned int ext_portmask);

	/* switch_hw_prx126_sw_image.c */
	int (*sw_image_version_get)(unsigned char id, unsigned char version_size, char *version);
	int (*sw_image_valid_get)(unsigned char id, unsigned char *valid);
	int (*sw_image_active_get)(unsigned char id, unsigned char *active);
	int (*sw_image_active_set)(unsigned char id, unsigned int timeout);
	int (*sw_image_commit_get)(unsigned char id, unsigned char *commit);
	int (*sw_image_commit_set)(unsigned char id);
	int (*sw_image_download_start)(unsigned char id, unsigned int size);
	int (*sw_image_download_stop)(unsigned char id);
	int (*sw_image_download_end)(unsigned char id, unsigned int size, unsigned int crc, unsigned char filepath_size, char *filepath);
	int (*sw_image_download_handle_window)(unsigned char id, unsigned short int window_id, unsigned char *data, unsigned int size);
	int (*sw_image_download_store)(unsigned char id, unsigned char filepath_size, char *filepath);

};

extern struct switch_hw_t switch_hw_g;

/* switch.c */
void switch_init_wan_logical_portid(void);
void switch_update_linkup_logical_portmask(void);
unsigned int switch_get_linkup_logical_portmask(void);
unsigned int switch_get_dmac_logical_portmask(char *macaddr);
void switch_loop_detection(void);
int switch_loop_detect_mask_get(unsigned int *portmask);
int switch_loop_detect_set(unsigned int enable, unsigned int portmask);
int switch_loop_detect_interval_set(unsigned int interval_check, unsigned int interval_loop);
int switch_loop_block_time_set(unsigned int loop_block_time);
unsigned int switch_get_wan_logical_portmask(void);
unsigned int switch_get_uni_logical_portmask(void);
unsigned int switch_get_cpu_logical_portmask(void);
unsigned int switch_get_wifi_logical_portmask(void);
unsigned int switch_get_all_logical_portmask(void);
unsigned int switch_get_all_logical_portmask_without_cpuext(void);
char switch_get_wan_logical_portid(void);
char switch_get_uni_logical_portid(unsigned char uni_index);
char switch_get_cpu_logical_portid(unsigned char cpu_index);
char switch_get_wifi_logical_portid(unsigned char wifi_index);
char *switch_get_logical_portname(unsigned int logical_portid);
int switch_get_port_info_by_me(struct me_t *me, struct switch_port_info_t *port_info);
int switch_get_port_info_by_logical_port_id(unsigned int logical_port_id, struct switch_port_info_t *port_info);
int switch_get_port_info_by_port_type_index(int port_type, int port_type_index, struct switch_port_info_t *port_info);
int switch_get_port_info_by_uni_num(unsigned char uni_num, struct switch_port_info_t *port_info);
int switch_hw_register(struct switch_hw_t *switch_hw);
int switch_hw_unregister(void);
int switch_loop_detect_mask_get(unsigned int *portmask);
void switch_loop_detection(void);
int switch_get_port_config(int logical_port_id, struct switch_port_mac_config_t* port_mac_config);
int switch_get_system_hw_intf_lan_vid(void);
#endif
