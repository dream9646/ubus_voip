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
 * Module  : util
 * File    : env.h
 *
 ******************************************************************/

#ifndef __ENV_H__
#define __ENV_H__

#include <pthread.h>

#define SYSTEM_VERSION_FILE		"/root/version"
#define CUSTOM_VERSION_FILE		"/etc/custom_version"
#define WAN_DAT_FILE			"/nvram/wan.dat"
#define GPON_DAT_FILE			"/nvram/gpon.dat"

#define ENV_SIZE_MIB_UPLOAD_ORDER	32
#define ENV_SIZE_IFNAME			16
#define ENV_SIZE_IPSTR			16
#define ENV_SIZE_PATH			256
#define ENV_SIZE_XML_SECTION		32
#define ENV_SIZE_DROP_VID 		16
#define ENV_SIZE_IPHOST_PORT		4
#define ENV_SIZE_STRING			256

enum env_task_no_t{
	ENV_TASK_NO_MAIN,
	ENV_TASK_NO_TIMER,
	ENV_TASK_NO_CLI,
	ENV_TASK_NO_CORE,
	ENV_TASK_NO_RECV,
	ENV_TASK_NO_ALARM,
	ENV_TASK_NO_PM,
	ENV_TASK_NO_AVC,
	ENV_TASK_NO_CPUPORT,
	ENV_TASK_NO_TEST,
	ENV_TASK_NO_ARP,
	ENV_TASK_NO_IGMP,
	ENV_TASK_NO_DHCP,
	ENV_TASK_NO_PPPOE,
	ENV_TASK_NO_RSTP,
	ENV_TASK_NO_CFM,
	ENV_TASK_NO_MACTAB_FIX,
	ENV_TASK_NO_VOIPCMD,
	ENV_TASK_NO_LOOPDETECT,
	ENV_TASK_NO_EXTOAM,
	ENV_TASK_NO_PKT,
	ENV_TASK_NO_LLDP,
	ENV_TASM_NO_TOTAL
};
#define ENV_SIZE_TASK				ENV_TASM_NO_TOTAL

enum env_crc_type_t {
	ENV_CRC_TYPE_DISABLE = 0, 	// disable crc check
	ENV_CRC_TYPE_BE, 		// Big Endian
	ENV_CRC_TYPE_LE, 		// Little Endian
	ENV_CRC_TYPE_AUTO = 9
};

enum env_mib_rw_mode_t {
	ENV_MIB_RW_MODE_SWSW = 0,
	ENV_MIB_RW_MODE_SWHW,
	ENV_MIB_RW_MODE_HWHW
};

enum env_mib_boundary_check_t {
	ENV_MIB_BOUNDARY_CHECK_DISABLE = 0,
	ENV_MIB_BOUNDARY_CHECK_WANRING,
	ENV_MIB_BOUNDARY_CHECK_ERROR
};

enum env_mib_pointer_check_t {
	ENV_MIB_POINTER_CHECK_DISABLE = 0,
	ENV_MIB_POINTER_CHECK_WANRING,
	ENV_MIB_POINTER_CHECK_ERROR
};

enum env_mib_readonly_check_t {
	ENV_MIB_READONLY_CHECK_DISABLE = 0,
	ENV_MIB_READONLY_CHECK_WANRING,
	ENV_MIB_READONLY_CHECK_ERROR
};

enum env_mib_nonexist_del_err_t {
	ENV_MIB_NONEXIST_DEL_ERR_DISABLE = 0,	// no error for all nonexist cases
	ENV_MIB_NONEXIST_DEL_ERR_ME,		// error if nonexist me
	ENV_MIB_NONEXIST_DEL_ERR_ME_TABENTRY	// error if nontexst me or nonexist table entry
};

enum env_mib_uni_adminlock_default_t {
	ENV_MIB_UNI_ADMINLOCK_DEFAULT_DISABLE = 0,
	ENV_MIB_UNI_ADMINLOCK_DEFAULT_ENABLE = 1,
	ENV_MIB_UNI_ADMINLOCK_DEFAULT_DONTOVERRIDE = 2
};

enum env_anig_type_t {
	ENV_ANIG_TYPE_NONE = 0,
	ENV_ANIG_TYPE_TRANCEIVER,
	ENV_ANIG_TYPE_BOSA_MINDSPEED,
	ENV_ANIG_TYPE_BOSA_MINDSPEED_LIB,
	ENV_ANIG_TYPE_BOSA_REPORT,
	ENV_ANIG_TYPE_SFF8472,
	ENV_ANIG_TYPE_BOSA_AUTO = 9
};

enum env_anig_support_t {
	ENV_ANIG_SUPPORT_DISABLE = 0,
	ENV_ANIG_SUPPORT_GET,
	ENV_ANIG_SUPPORT_GET_TEST,
	ENV_ANIG_SUPPORT_GET_TEST_ALARM
};

enum env_anig_mon_mode_t {
	ENV_ANIG_MON_MODE_CAL_INT = 0,
	ENV_ANIG_MON_MODE_CAL_EXT = 1
};

enum env_stp_conf_t {
	ENV_STP_CONF_NO_CHANGE = 0,
	ENV_STP_CONF_ALL_DISABLE,
	ENV_STP_CONF_CPU_OFF_ELSE_DISABLE,
	ENV_STP_CONF_CPU_UNI_OFF_ELSE_DISABLE,
	ENV_STP_CONF_ALL_OFF
};

enum env_bridge_port_type_t {
	ENV_BRIDGE_PORT_TYPE_CPU = 0,
	ENV_BRIDGE_PORT_TYPE_UNI = 1,
	ENV_BRIDGE_PORT_TYPE_WAN = 2,
	ENV_BRIDGE_PORT_TYPE_IPHOST = 3,
	ENV_BRIDGE_PORT_TYPE_DS_BCAST = 4,
	ENV_BRIDGE_PORT_TYPE_UNUSED = 5,
};

enum env_debug_log_type_t {
	ENV_DEBUG_LOG_TYPE_BITMASK_CONSOLE = 1,
	ENV_DEBUG_LOG_TYPE_BITMASK_FILE = 2,
	ENV_DEBUG_LOG_TYPE_BITMASK_SYSLOG = 4
};

enum env_olt_workaround_t {
	ENV_OLT_WORKAROUND_NONE = 0x0,
	ENV_OLT_WORKAROUND_ALU,
	ENV_OLT_WORKAROUND_DASAN,
	ENV_OLT_WORKAROUND_ERICSSON,
	ENV_OLT_WORKAROUND_HUAWEI,
	ENV_OLT_WORKAROUND_OCCAM,
	ENV_OLT_WORKAROUND_CALIX,
	ENV_OLT_WORKAROUND_TELLION,
	ENV_OLT_WORKAROUND_ZTE,
	ENV_OLT_WORKAROUND_ADTRAN,
	ENV_OLT_WORKAROUND_FIBERHOME
};

#define ENV_SIZE_PROPRIETARY_VENDOR	16
#define ENV_OLT_PROPRIETARY_NONE	"none"
#define ENV_OLT_PROPRIETARY_FIBERHOME	"fiberhome"
#define ENV_OLT_PROPRIETARY_ZTE		"zte"
#define ENV_OLT_PROPRIETARY_ALU		"alu"
#define ENV_OLT_PROPRIETARY_HUAWEI	"huawei"
#define ENV_OLT_PROPRIETARY_DASAN	"dasan"
#define ENV_OLT_PROPRIETARY_CALIX	"calix"
#define ENV_OLT_PROPRIETARY_TELLION	"tellion"
#define ENV_OLT_PROPRIETARY_ERICSSON	"ericsson"
#define ENV_OLT_PROPRIETARY_ADTRAN	"adtran"

enum env_pq_tcont_relation_t {
	ENV_PQ_TCONT_RELATION_BY_TS_POINTER = 0x0,
	ENV_PQ_TCONT_RELATION_BY_RELATED_PORT
};

#define ENV_SIZE_IGMP_PREASSIGN_LIST	8
struct omci_env_igmp_preassign_t {
	unsigned char mac[6];		// for L2 mcast preassign
	unsigned char mask[6];
	unsigned int ipaddr;		// for ipv4 mcast preassign
	unsigned int ipaddr_mask;
};

#define ENV_SIZE_TCONT_MAP	32
#define ENV_SIZE_IPHOST_PORT	4

enum env_voip_enable_t {
	ENV_VOIP_DISABLE = 0,
	ENV_VOIP_ENABLE,
	ENV_VOIP_AUTO = 9
};

#define ENV_CPUPORT_WIFI_BLOCK_BC 0x1 //bit 0
#define ENV_CPUPORT_WIFI_BLOCK_MC 0x2 //bit 1
#define ENV_CPUPORT_WIFI_BLOCK_UC 0x4 //bit 2

struct omci_env_t {
	char *taskname[ENV_SIZE_TASK];
	int taskid[ENV_SIZE_TASK];
	pthread_t pthreadid[ENV_SIZE_TASK];
	unsigned long long taskloopcount[ENV_SIZE_TASK];
	long taskts[ENV_SIZE_TASK]; //task time stamp
	char taskstate[ENV_SIZE_TASK]; //task state, 0:idle, 1:running

// global
	unsigned int cli_ip;
	unsigned short cli_port;
	unsigned char transport_type;
	char etc_omci_path[ENV_SIZE_PATH];
	char etc_omci_path2[ENV_SIZE_PATH];
	char start_cmd_file[ENV_SIZE_PATH];

// interface config
	char trap_if[ENV_SIZE_IFNAME];
	char omcc_if[ENV_SIZE_IFNAME];
	char wifi_if[ENV_SIZE_IFNAME];
	char router_wan_if[ENV_SIZE_IFNAME];
	char router_lan_if[ENV_SIZE_IFNAME];

// connecting olt simulator
	char sim_olt_ip[ENV_SIZE_IPSTR];
	unsigned short sim_olt_udp_port;
	unsigned short sim_pon_id;
	unsigned short sim_onu_id;
	unsigned char sim_tcont_enable;

// omci msg
	unsigned char omcc_rx_crc_type;		// 0: disable, 1: be, 2: le, 9:auto  (default 9)
	unsigned char omcc_tx_crc_type;		// 0: disable, 1: be, 2: le (default 1)

	unsigned char omci_tranx_cache_num;
	unsigned char omci_tranx_match_mode; //0: loose, 1: strict
	unsigned char omci_tranx_bypass; //0:passthrough:, 1:bypass
	unsigned char omci_tranx_busy_enable; //0:disable, 1:enable

	unsigned short omci_exec_time_limitation; //milli seconds
	unsigned char omci_tx_flag; //0: auto, 1: always send, 2: always not send
	unsigned char omci_tx_resend_flag; //0:not resned, 1: resend

	char omci_history_enable; 		// 0:disable, 1:enable
	unsigned int omci_history_size;		// max list entry number
	unsigned int omci_history_msgmask;	// msg to be put into history

	unsigned char omcidump_enable;
	unsigned int omcidump_msgmask;

	unsigned char omccrx_log_enable;
	unsigned int omccrx_log_msgmask;	// msg to be put omccrx.log
	char omccrx_log_file[ENV_SIZE_PATH];

	unsigned char restore_log_enable;
	char restore_log_file[ENV_SIZE_PATH];

// mib
	unsigned short xml_section[ENV_SIZE_XML_SECTION];
	unsigned char xml_load_namestr;

	unsigned short mib_upload_order[ENV_SIZE_MIB_UPLOAD_ORDER];
	enum env_mib_boundary_check_t mib_boundary_check; 	// 0: not chk, 1: chk for warning, 2: chk for err
	enum env_mib_pointer_check_t mib_pointer_check;		// 0: not chk, 1: chk for warning, 2: chk for err
	enum env_mib_readonly_check_t mib_readonly_check;	// 0: not chk, 1: chk for warning, 2: chk for err
	enum env_mib_rw_mode_t mib_rw_mode; 	// 0: swsw, 1: swhw, 2: hwhw
	unsigned char mib_aliasid_enable;
	unsigned short mib_rare_meid_start;
	unsigned short mib_rare2_meid_start;
	unsigned char mib_nonexist_del_err;	// 0: no err, 1: if me del, 2 if me or tab_entry del

	char autouni_enable;
	char autoveip_enable;
	unsigned char erdiag_mon_enable;
	unsigned char mibreset_dump_enable;
	unsigned char mib_uni_adminlock_default;	// 0: unlock, 1: lock, 2: dont override
	char adminlock_trigger_opstate;
	unsigned short untag_reserved_vid; //default: 4088

	char port_forward_operation_default;

	char pseudo_ts_pointer_enable;
	char invert_weight_enable;
	char invert_priority_enable;
	char pq_tcont_relation;
	char tcont_map_enable;
	char tcont_map_policy[2];
	char tcont_map_ts[ENV_SIZE_TCONT_MAP];
	char tcont_map_pq[ENV_SIZE_TCONT_MAP];

// sw download
	char sw_download_end_behavior;
	char sw_download_active_always_ok;
	char sw_download_commit_always_ok;
	unsigned char sw_download_window_size;
	char sw_download_script[ENV_SIZE_PATH];
	char sw_download_image_name[ENV_SIZE_PATH];

// thread tasks
	unsigned int task_pause_mask;
	unsigned int avc_refresh_interval; //secs
	unsigned int alarm_refresh_interval; //secs
	unsigned int pm_refresh_interval; //secs
	unsigned int pm_swap_interval; //secs

	char cpuport_history_enable;
	unsigned int cpuport_history_size;
	char cpuport_arp_enable;
	char cpuport_dhcp_enable;
	unsigned char cpuport_dhcp_cust_tag[ENV_SIZE_STRING]; // customized dhcp option string
	char cpuport_dhcp_server_onlan;
	char cpuport_pppoe_enable;
	unsigned char cpuport_pppoe_cust_tag[ENV_SIZE_STRING]; // customized pppoe tag string
	char cpuport_pkt_ssdp_fwd_wan;
	char cpuport_im_enable; //0: disable, 1: enable, multicast did not forward to wan
	char cpuport_im_lan2lan_enable; // 0: disable, 1: enable, multicast forward between lan
	char cpuport_wifi_block_mask; //bit 0: broadcast, bit 1: multicast, bit 2: unknown unicast, 0: forward, 1: block
	char cpuport_wifi_block_ra; //block ds ipv6 RA to wifi, 0: forward, 1: drop
	char cpuport_mcast_drop; //upstream multicast drop, 0: forward, 1: drop
	char stp_enable;

	char cpu_usage_threshold;
	char cpul_task_no[ENV_SIZE_TASK]; //cpu limit tasks no, init -1
	unsigned char cpul_total_usage; // %

// debug
	char debug_level;	// use char instead of unsigned char for debug_level* to eliminate compiler warning when compared with 0(LOG_EMERG)
	char debug_level_bat;
	char debug_level_cfm;
	char debug_level_cpuport;
	char debug_level_fwk;
	char debug_level_igmp;
	char debug_level_lldp;
	char debug_level_vacl;
	char debug_level_voip;
	char debug_level_xml;
	unsigned char debug_log_time;
	enum env_debug_log_type_t debug_log_type;
	char debug_log_file[ENV_SIZE_PATH];
	unsigned char memdbg_enable;
	char er_group_hw_debug_enable;
	char er_group_hw_time_enable;

// olt
	char olt_vendor_check_enable;
	char olt_vendor_config_change;
	char olt_workaround_enable;
	char olt_alu_mcast_support;
	char olt_huawei_ignore_vlan_drop_rule;
	char olt_proprietary_support[ENV_SIZE_PROPRIETARY_VENDOR];
	char ont_force_vendor_id[5];

// anig
	char anig_type;
	char anig_support;
	char anig_txrx_threshold[4];
	char anig_devport;			// 0: default anig device bus
	char anig_devaddr0;			// 0x0: default device address, 0x1~0xFE: specific dev address
	char anig_mon_mode;			// 0: Internal calibration, 1: Exrernally calibration

// logical port id/mask
	unsigned char wan_logical_portid;	// wan port logical port id
	unsigned int uni_logical_portmask;
	unsigned int cpu_logical_portmask;
	unsigned int wifi_logical_portmask;

// switch
	char ivlan_enable;	// 0: disable, 1: enable
	char stp_conf_before_omci;
	char switch_history_mode;	// 0: diff, 1: total
	unsigned short switch_history_interval;	// unit: minute

	unsigned int br_allowed_srcmask;
	unsigned int rt_allowed_srcmask;
	unsigned int trunk_group_portmask;	// default 0x0, trunk disabled

	char localbridging_default;	// 0: disable, 1: enable
	unsigned int localbridging_addon_portmask;	// ports always reachable, default: cpu ext port 1..5
	unsigned int ctc_internet_bridge_enabled_srcmask;

	char port2port_enable;	// 0: disable, 1: enable
	char maclearn_drop_enable;
	char veip_for_wifi_enable;
	char cpuport_ds_passthrough;
	char cpuport_pvlan_clone_ipv4_to_arp; // clone pvlan of ipv4 to arp, 0:disable, 1:enable
	unsigned long long rma_forward_mask;
	char bundle_rate_support;
	unsigned char linkdown_debounce_time;	// trigger linkdown event after physical down for more than N sec
	unsigned char linkup_debounce_time;	// trigger linkup event after physical up for more than N sec

// rldp
	char rldp_enable;	// 0: disable, 1: enable
	char rldp_def_portmask;	// 0: disable, 0x1f: enable all port
	char rldp_passive_only;	// 0: allow , 1: disallow generate detective packet

// tag
	char tag_treatment_correct;
	char tag_discard_reversing;
	char tag_preview_table;
	char tag_auto_reverse; //0:not reverse, 1:reverse
	char tag_mc_ds_from_us; //0: not create rules, 1: create one tag rule from us tci, 1: create two tags rules from us tci
	char tag_mc_sort_priority; //regular rules default priority is 5, greater value is higher priority
	char tag_pbit_workaround; //ds tag pbit workaround, 0:disable, 1: don't care, 2: remap, 3: add one more don't care rule, 4: add one more transparent rule
	unsigned char tag_pbit_portmask; //ds tag pbit workaround portmask
	char tag_pbit_remap_table[8]; //ds tag pbit workaround, remap table for each pbit value
	char tag_ds_tci_dont_reverse_workaround; //0:disable, 1:enable
	char tag_insert_65305_rules; //0: not insert, 1: insert
	char tag_ctc_translation_path[ENV_SIZE_PATH];
	char tag_mc_ds_wildcard; //0: disable, 1: enable
	char tag_l2_rules_reverse; //0: auto reverse, 1: not reverse
	char tag_l2_rules_ds_mac; //0: ds not include mac, 1: ds include mac
	unsigned short tag_iphost_vid_map[ENV_SIZE_IPHOST_PORT]; //original tag vid for iphost, 0-4095: original vid, 4096: disable

// classf
	char classf_tag_mode; //0: collect all tagging rules, 1: ignore 2 tags rules
	char classf_add_default_rules; //0: disable, 1: add default rules while empty, 2: force to add default rules
	char classf_force_stpid;
	unsigned short classf_stag_tpid;
	unsigned short classf_hw_stag_tpid; //update by classf hw
	char classf_et_combine; //0: disable, 1: enable
	char classf_ds_gem_dg; //0: disable, 1: enable
	char classf_dscp2pbit_mode; //0: per port configuration, 1: global table, 2: global table, force to assign veip 171 dscp-2-pbit mapping table
#if 0 // Remove VEIP related workaround
	unsigned short classf_veip_orig_tci_map[ENV_SIZE_VEIP_PORT]; //original tag tci from veip
	unsigned short classf_veip_real_tci_map[ENV_SIZE_VEIP_PORT]; //real tag tci from veip
	char  classf_veip_real_tag_rule_priority; // sort priority for the rules generated by this tci, default is 4
#endif
	char classf_ds_unmatch; //classf downstream unmatch action: 0: pass, 1: drop
	char classf_ds_mc_extend; //add one more rule for ds multicast gem, 0: disable, 1: enable
	char classf_ds_mc_force_strip; //force me 310 on veip ds mc rules to strip tags, 0: disable, 1: enable
	char classf_veip_rules_reassign; //reassign veip port rules, MARK and COPY untag(priority tag) rules to all uni ports for sw(bridge port)/hw(classf),
	//0: disable, 1: hw MARK and COPY, 2: hw only MARK, 3: all MARK and COPY, 4: all only MARK
	char classf_dropping_rules_add; //add dropping rules, 0: do NOT add, 1: add
	char classf_vmap_pbit_workaround; //0: disable, 1: pbit do NOT care
	char classf_hw_2tags_mode; //0: involve acl if necessary, 1: always involve acl
	char classf_hw_2tags_workaround; //0: filter pri or vid, 1: filter pri only, 2: filter vid only
	char classf_add_ipv4mc_strip;		//0: disable, 1: strip 1tag, 2: strip 2tags
	char classf_add_hwnat_1tag_rules; //0: disable, 1: enable
	char classf_add_hwnat_untag_rules; //0: disable, 1: enable
	unsigned short classf_add_hwnat_untag_vid; //vid value
	unsigned short classf_add_hwnat_untag_vid_mask; //vid value mask
	char classf_hw_rules_migration; //0: disable, 1:enable
	char classf_hw_colour_force; //0: green, 1:yellow, 2: auto
	char classf_keep_candi; //0: not keep candi rules, 1: keep candi rules
	char classf_statistic_enable; //0: disable, 1: enable
	char classf_tlan_pattern; //0: disable, 1: enable
	unsigned short classf_hw_intf_lan_vid; //system hw interface lan vid start value
	char mc_ds_untag_discard_enable; //for zte proprietary me 65282
	char classf_add_default_rules_by_me_84; //add default classf rule by me, 84, for GWS

// cfm
	char cfm_enable; 				// whether to start cfm task
	char cfm_y1731_enable;
	char cfm_portmask;				// logical ports running cfm
	char cfm_mac_ifname[ENV_SIZE_IFNAME];
	char cfm_send_timeout;
	char cfm_reverse_mep_direction;
	unsigned char cfm_rmep_ccm_before_hold;
	unsigned int cfm_rmep_holdtime_before_active;
	unsigned char cfm_rmep_ccm_lost_threshold;
	unsigned char cfm_defect_mask_in_rdi_gen;
	unsigned char cfm_defect_mask_in_ais_gen;
	unsigned char cfm_defect_mask_supressed_by_ais;
	unsigned char cfm_defect_mask_supressed_by_lck;

// lldp
	char lldp_enable; 				// whether to start lldp task
	char lldp_ifname[ENV_SIZE_IFNAME]; 		// use ifname's macaddr/ipaddr
	char lldp_dst_mac;				// dst mac to send lldp message
	char lldp_send_interval;			// interval to send lldp message
	char lldp_fast_tx;				// msgFastTx : Time interval in timer ticks between transmissions during fast transmission periods (sec)
	char lldp_credit_max;				// txCreditMax : The maximum value of txCredit.
	char lldp_tx_fast_init;                         // txFastInit : The number of LLDPDUs that are transmitted during a fast transmission period.
	char lldp_system_name[ENV_SIZE_STRING];		// system name to be set in lldp message
	char lldp_system_desc[ENV_SIZE_STRING];		// system desc to be set in lldp message
	char lldp_med_opt;
	short lldp_med_cap;
// tm
	char tm_pq_gem_qos_method;
	unsigned short tm_gemus_rate_factor; // percentage for gem us td rate limit (hw pq cir/pir), 0-200
	unsigned short tm_gemds_rate_factor; // percentage for gem ds td rate limit (hw meter), 0-200
	unsigned short tm_unius_rate_factor; // percentage for uni us td rate limit (hw pq cir/pir), 0-200
	unsigned short tm_unids_rate_factor; // percentage for uni ds td rate limit (hw meter), 0-200
	unsigned char tm_pir_zero_action; // 0:drop, 1:bypass
	unsigned char tm_protect_enable; // b1:uni, b2:pon

// igmp
	char igmp_enable;
	char igmp_version_overwrite;
	char igmp_join_default_grant;
	char igmp_mcast_me_support;
	char igmp_proxy_snooping;
	char igmp_query_pass_through;
	unsigned short igmp_upstream_tpid;
	unsigned short igmp_downstream_tpid;
	char igmp_v1_ignore;
	struct omci_env_igmp_preassign_t igmp_preassign_list[ENV_SIZE_IGMP_PREASSIGN_LIST];
	unsigned char igmp_mcast_mode_src; //how to decide mcast mode, 0:force pass mode, 1:force drop mode, 2: by mcast gem, 3: by private me assignment

// extoam
	char extoam_enable; 				// whether to start extoam task
	unsigned char extoam_port;
	char extoam_mac_ifname[ENV_SIZE_IFNAME]; 		// use ifname macaddr as extoam src mac
	char extoam_itas_image_name[256]; 		// use ifname macaddr as extoam src mac

// voip
	char voip_enable;	// 0:disable, 1:enable, 9:auto
	char voip_debug_set_flag;
	char voip_debug_get_flag;
	char voip_debug_release_flag;

// misc
	unsigned int batchtab_extevent_idletime;
	char batchtab_retry_timeout;
	unsigned int batchtab_wanif_feature;
	unsigned char batchtab_hardbridge_enable;
	char exclude_flowid_0;	// 0: disable, 1: enable
	char avoid_pir_drop;	// 0: disable, 1: enable
	unsigned int uni_policing_pbit_mask;
	unsigned int uni_policing_default_bs;
	unsigned char uni_policing_aggregate_acl;
	char uni_linkdown_lock;	// 0: disable, 1: enable
	unsigned short watchdog_omccmiss_timeout[2]; // check if omccmiss remains for more than N sec, then reset serdes, or reboot
	unsigned short watchdog_tcontmiss_timeout[2]; // check if tcontmiss remains for more than N sec, then reset serdes, or reboot
	unsigned short watchdog_o2stuck_timeout[2]; // check if o2stuck remains for more than N sec, then reset serdes, or reboot
	unsigned short watchdog_idlebw_guard[4]; // check if any tcont overalloc bw from olt
	unsigned char uni_config_lock;	// 0: disable, 1: enable
};

extern struct omci_env_t omci_env_g;

/* env.c */
int env_print_igmp_preassign_list(int fd);
int env_set_igmp_preassign_list(char *str);
int env_set_tcont_map_policy(char *str);
int env_set_sim_tcont_enable(int enable);
int env_set_to_default(void);
char *env_get_taskname_by_id(int taskid);

#endif
