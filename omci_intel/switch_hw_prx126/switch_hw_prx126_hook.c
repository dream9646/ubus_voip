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
 * Module  : switch_hw_prx126
 * File    : switch_hw_prx126_hook.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "list.h"
#include "switch_hw_prx126.h"
#include "omciutil_vlan.h"
#include "switch.h"
#include "env.h"

//----------------------------------------------------------------------------
// Register switch API functions for RTL8305
//----------------------------------------------------------------------------
struct switch_hw_t switch_hw_fvt_g = {
	/* switch_hw_prx126_acl.c */
	.acl_entry_delete = switch_hw_prx126_acl_entry_delete,
	.acl_entry_enable_set = switch_hw_prx126_acl_entry_enable_set,
	.acl_entry_flush = switch_hw_prx126_acl_entry_flush,
	.acl_entry_get_free_info = switch_hw_prx126_acl_entry_get_free_info,
	.acl_user_template_set = switch_hw_prx126_acl_user_template_set,
	.acl_user_template_get = switch_hw_prx126_acl_user_template_get,
	.acl_user_field_set = switch_hw_prx126_acl_user_field_set,
	.acl_user_field_get = switch_hw_prx126_acl_user_field_get,

	/* switch_hw_prx126_vacl.c */
	.vacl_entry_print = switch_hw_prx126_vacl_entry_print,
	.vacl_commit = switch_hw_prx126_vacl_commit,
	.vacl_init = switch_hw_prx126_vacl_init,
	.vacl_shutdown = switch_hw_prx126_vacl_shutdown,
	.vacl_dump = switch_hw_prx126_vacl_dump,
	.vacl_meter_clear = switch_hw_prx126_vacl_meter_clear,
	.vacl_rule_evaluate = switch_hw_prx126_vacl_rule_evaluate,
	.vacl_rule_valid_get = switch_hw_prx126_vacl_rule_valid_get,
	.vacl_clear = switch_hw_prx126_vacl_clear,
	.vacl_port_enable_set = switch_hw_prx126_vacl_port_enable_set,
	.vacl_port_permit_set = switch_hw_prx126_vacl_port_permit_set,
	.vacl_port_enable_print = switch_hw_prx126_vacl_port_enable_print,
	.vacl_port_permit_print = switch_hw_prx126_vacl_port_permit_print,

	/* switch_hw_prx126_classf.c */
	.classf_init = switch_hw_prx126_classf_init,
	.classf_manage = switch_hw_prx126_classf_manage,
	.classf_print = switch_hw_prx126_classf_print,

	/* switch_hw_prx126_init.c */
	.init = switch_hw_prx126_init,
	.shutdown = switch_hw_prx126_shutdown,

	/* switch_hw_prx126_int.c */
	.int_ctrl_set = switch_hw_prx126_int_ctrl_set,
	.int_ctrl_get = switch_hw_prx126_int_ctrl_get,
	.int_ctrl_print = switch_hw_prx126_int_ctrl_print,
	.int_stat_set = switch_hw_prx126_int_stat_set,
	.int_stat_get = switch_hw_prx126_int_stat_get,
	.int_stat_print = switch_hw_prx126_int_stat_print,

	/* switch_hw_prx126_ipmc.c */
	.ipmcastaddr_add = switch_hw_prx126_ipmcastaddr_add,
	.ipmcastaddr_del = switch_hw_prx126_ipmcastaddr_del,
	.ipmcastaddr_flush = switch_hw_prx126_ipmcastaddr_flush,
	.ipmcastaddr_get = switch_hw_prx126_ipmcastaddr_get,
	.ipmcastaddr_get_next = switch_hw_prx126_ipmcastaddr_get_next,
	.ipmcast_mode_set = switch_hw_prx126_ipmcast_mode_set,
	.ipmcast_mode_clear = switch_hw_prx126_ipmcast_mode_clear,
	.ipmcast_me309_mode_set = switch_hw_prx126_ipmcast_me309_mode_set,
	.ipmcast_me310_extvlan_set = switch_hw_prx126_ipmcast_me310_extvlan_set,
	/* switch_hw_prx126_mac.c */
	.mac_flood_enable_get = switch_hw_prx126_mac_flood_enable_get,
	.mac_flood_enable_set = switch_hw_prx126_mac_flood_enable_set,
	.mac_learning_limit_get = switch_hw_prx126_mac_learning_limit_get,
	.mac_learning_limit_set = switch_hw_prx126_mac_learning_limit_set,
	.mac_entry_add = switch_hw_prx126_mac_entry_add,
	.mac_entry_del = switch_hw_prx126_mac_entry_del,
	.mac_entry_update = switch_hw_prx126_mac_entry_update,
	.mac_entry_get = switch_hw_prx126_mac_entry_get,
	.mac_l2uc_get_next = switch_hw_prx126_mac_l2uc_get_next,
	.mac_l2mc_get_next = switch_hw_prx126_mac_l2mc_get_next,
	.mac_ipmc_get_next = switch_hw_prx126_mac_ipmc_get_next,
	.mac_table_flush = switch_hw_prx126_mac_table_flush,
	.mac_table_print_valid = switch_hw_prx126_mac_table_print_valid,
	.mac_aging_set = switch_hw_prx126_mac_aging_set,
	.mac_aging_get = switch_hw_prx126_mac_aging_get,
	.mac_aging_enable_set = switch_hw_prx126_mac_aging_enable_set,
	.mac_aging_enable_get = switch_hw_prx126_mac_aging_enable_get,
#if defined CONFIG_SDK_PRX126 || defined CONFIG_SDK_PRX120 || defined CONFIG_SDK_PRX321
	/*px126 bridge*/
	.bridge_inf_create = switch_hw_prx126_mac_bridge_inf_create,
	.bridge_inf_del = switch_hw_prx126_mac_bridge_inf_del,
	.bridge_port_config_update = switch_hw_prx126_mac_bridge_port_cgf_update,
	.bridge_port_connect = switch_hw_prx126_mac_bridge_port_connect,
#endif	
	/* switch_hw_prx126_misc.c */
	.preassign_filter_get = switch_hw_prx126_preassign_filter_get,
	.preassign_filter_set = switch_hw_prx126_preassign_filter_set,
	.igmp_upstream_rate_set = switch_hw_prx126_igmp_upstream_rate_set,
	.igmp_upstream_rate_get = switch_hw_prx126_igmp_upstream_rate_get,
	.igmp_extract_enable_set = switch_hw_prx126_igmp_extract_enable_set,
	.igmp_extract_enable_get = switch_hw_prx126_igmp_extract_enable_get,
	.dhcp_extract_enable_set = switch_hw_prx126_dhcp_extract_enable_set,
	.dhcp_extract_enable_get = switch_hw_prx126_dhcp_extract_enable_get,
	.pppoe_extract_enable_set = switch_hw_prx126_pppoe_extract_enable_set,
	.pppoe_extract_enable_get = switch_hw_prx126_pppoe_extract_enable_get,
	.cfm_extract_enable_set = switch_hw_prx126_cfm_extract_enable_set,
	.cfm_extract_enable_get = switch_hw_prx126_cfm_extract_enable_get,
	.extoam_extract_enable_set = switch_hw_prx126_extoam_extract_enable_set,
	.extoam_extract_enable_get = switch_hw_prx126_extoam_extract_enable_get,
	.eth_pppoe_filter_enable_set = switch_hw_prx126_eth_pppoe_filter_enable_set,
	.eth_pppoe_filter_enable_get = switch_hw_prx126_eth_pppoe_filter_enable_get,
	.eth_pppoe_filter_enable_print = switch_hw_prx126_eth_pppoe_filter_enable_print,
	.eth_address_set = switch_hw_prx126_eth_address_set,
	.eth_address_get = switch_hw_prx126_eth_address_get,
	.eth_address_print = switch_hw_prx126_eth_address_print,
	.dot1x_mode_set = switch_hw_prx126_dot1x_mode_set,
	.dot1x_mode_get = switch_hw_prx126_dot1x_mode_get,
	.dot1x_state_set = switch_hw_prx126_dot1x_state_set,
	.dot1x_state_get = switch_hw_prx126_dot1x_state_get,
	.dot1x_dir_set = switch_hw_prx126_dot1x_dir_set,
	.dot1x_dir_get = switch_hw_prx126_dot1x_dir_get,
	.rma_action_set = switch_hw_prx126_rma_action_set,
	.rma_action_get = switch_hw_prx126_rma_action_get,
	.hw_usage_print = switch_hw_prx126_hw_usage_print,
#if defined CONFIG_SDK_PRX126 || defined CONFIG_SDK_PRX120 || defined CONFIG_SDK_PRX321
	.pptp_lct_uni_create = switch_hw_prx126_pptp_lct_uni_create,
	.pptp_lct_uni_update = switch_hw_prx126_lct_uni_update,
	.pptp_lct_uni_destory = switch_hw_prx126_pptp_lct_uni_destory,
#endif

	/* switch_hw_prx126_pm.c */
	.counter_global_get = switch_hw_prx126_counter_global_get,
	.counter_port_get = switch_hw_prx126_counter_port_get,
	.pm_reset_port = switch_hw_prx126_pm_reset_port,
	.pm_refresh = switch_hw_prx126_pm_refresh,
	.pm_global_get = switch_hw_prx126_pm_global_get,
	.pm_port_get = switch_hw_prx126_pm_port_get,
	.pm_summary_print = switch_hw_prx126_pm_summary_print,
	.bridge_port_pm_data = switch_hw_prx126_bridge_port_pm_data,
	.eth_pm_data = switch_hw_prx126_eth_pm_data,
	.eth_pm_data2 = switch_hw_prx126_eth_pm_data2,
	.eth_pm_data3 = switch_hw_prx126_eth_pm_data3,
	.eth_pm_data2_private = switch_hw_prx126_eth_pm_data2_private,
	.eth_pm_data_upstream = switch_hw_prx126_eth_pm_data_upstream,
	.eth_pm_data_downstream = switch_hw_prx126_eth_pm_data_downstream,
	.eth_pm_data_upstream_long = switch_hw_prx126_eth_pm_data_upstream_long,
	.eth_pm_data_downstream_long = switch_hw_prx126_eth_pm_data_downstream_long,
	.eth_pm_rate_status_get = switch_hw_prx126_eth_pm_rate_status_get,

	/* switch_hw_prx126_port.c */
	.port_config_set = switch_hw_prx126_port_config_set,
	.port_config_get = switch_hw_prx126_port_config_get,
	.port_config_print = switch_hw_prx126_port_config_print,
	.port_status_get = switch_hw_prx126_port_status_get,
	.port_status_print = switch_hw_prx126_port_status_print,
	.port_phylb_set = switch_hw_prx126_port_phylb_set,
	.port_phylb_get = switch_hw_prx126_port_phylb_get,
	.port_phylb_print = switch_hw_prx126_port_phylb_print,
	.port_phyiso_set = switch_hw_prx126_port_phyiso_set,
	.port_phyiso_get = switch_hw_prx126_port_phyiso_get,
	.port_phyiso_print = switch_hw_prx126_port_phyiso_print,
	.port_eth_mtu_set = switch_hw_prx126_port_eth_mtu_set,
	.port_eth_mtu_get = switch_hw_prx126_port_eth_mtu_get,
	.port_enable_set = switch_hw_prx126_port_enable_set,
	.port_enable_get = switch_hw_prx126_port_enable_get,
	.port_testmode_set = switch_hw_prx126_port_testmode_set,
	.port_testmode_get = switch_hw_prx126_port_testmode_get,
	.port_enable_print = switch_hw_prx126_port_enable_print,
	.port_inf_set      = switch_hw_prx126_port_inf_set,
	.port_inf_update      = switch_hw_prx126_port_inf_update,
	.port_autoneg_set = switch_hw_prx126_port_autoneg_set,
	.port_autoneg_get = switch_hw_prx126_port_autoneg_get,
	.port_autoneg_print = switch_hw_prx126_port_autoneg_print,
	.port_phy_reg_print = switch_hw_prx126_port_phy_reg_print,
	.port_phy_reg_interpret_print = switch_hw_prx126_port_phy_reg_interpret_print,

	/* switch_hw_prx126_history.c */
	.history_init = switch_hw_prx126_history_init,
	.history_update = switch_hw_prx126_history_update,
	.history_add = switch_hw_prx126_history_add,
	.history_print = switch_hw_prx126_history_print,

	/* switch_hw_prx126_qos.c */
	.qos_dot1p2pri_set_by_pri = switch_hw_prx126_qos_dot1p2pri_set_by_pri,
	.qos_dot1p2pri_get_by_pri = switch_hw_prx126_qos_dot1p2pri_get_by_pri,
	.qos_dot1p2pri_print = switch_hw_prx126_qos_dot1p2pri_print,
	.qos_dscp2pri_set_by_pri = switch_hw_prx126_qos_dscp2pri_set_by_pri,
	.qos_dscp2pri_get_by_pri = switch_hw_prx126_qos_dscp2pri_get_by_pri,
	.qos_dscp2pri_print = switch_hw_prx126_qos_dscp2pri_print,
	.qos_p2q_set_by_pri = switch_hw_prx126_qos_p2q_set_by_pri,
	.qos_p2q_set_by_group = switch_hw_prx126_qos_p2q_set_by_group,
	.qos_p2q_group_set_by_port = switch_hw_prx126_qos_p2q_group_set_by_port,
	.qos_p2q_print = switch_hw_prx126_qos_p2q_print,
	.qos_portcfg_print = switch_hw_prx126_qos_portcfg_print,
	.qos_port_priority_set = switch_hw_prx126_qos_port_priority_set,
	.qos_port_scheduled_weight_set_by_qid = switch_hw_prx126_qos_port_scheduled_weight_set_by_qid,
	.qos_meter_print = switch_hw_prx126_qos_meter_print,
	.qos_meter_set_by_qid = switch_hw_prx126_qos_meter_set_by_qid,
	.qos_prisel_weight_set = switch_hw_prx126_qos_prisel_weight_set,
	.qos_prisel_weight_print = switch_hw_prx126_qos_prisel_weight_print,
	.qos_ds_pbit_pq_map_get = switch_hw_prx126_qos_ds_pbit_pq_map_get,
	.qos_ds_pbit_pq_map_set = switch_hw_prx126_qos_ds_pbit_pq_map_set,

	/* switch_hw_prx126_rate.c */
	.rate_storm_set = switch_hw_prx126_rate_storm_set,
	.rate_storm_get = switch_hw_prx126_rate_storm_get,
	.rate_storm_enable_set = switch_hw_prx126_rate_storm_enable_set,
	.rate_storm_enable_get = switch_hw_prx126_rate_storm_enable_get,
	.rate_storm_port_enable_set = switch_hw_prx126_rate_storm_port_enable_set,
	.rate_storm_port_enable_get = switch_hw_prx126_rate_storm_port_enable_get,
	.rate_storm_print = switch_hw_prx126_rate_storm_print,
	.rate_meter_init = switch_hw_prx126_rate_meter_init,
	.rate_meter_add = switch_hw_prx126_rate_meter_add,
	.rate_meter_del = switch_hw_prx126_rate_meter_del,
	.rate_meter_set = switch_hw_prx126_rate_meter_set,
	.rate_meter_get = switch_hw_prx126_rate_meter_get,
	.rate_meter_print = switch_hw_prx126_rate_meter_print,
	.rate_meter_bucket_init = switch_hw_prx126_rate_meter_bucket_init,
	.rate_meter_bucket_add = switch_hw_prx126_rate_meter_bucket_add,
	.rate_meter_bucket_del = switch_hw_prx126_rate_meter_bucket_del,
	.rate_meter_bucket_set = switch_hw_prx126_rate_meter_bucket_set,
	.rate_meter_bucket_get = switch_hw_prx126_rate_meter_bucket_get,
	.rate_control_set = switch_hw_prx126_rate_control_set,
	.rate_control_get = switch_hw_prx126_rate_control_get,
	.rate_control_print = switch_hw_prx126_rate_control_print,
	.rate_status_print = switch_hw_prx126_rate_status_print,

	/* switch_hw_prx126_rldp.c */
	.rldp_init = switch_hw_prx126_rldp_init,
	.rldp_enable_set = switch_hw_prx126_rldp_enable_set,
	.rldp_enable_get = switch_hw_prx126_rldp_enable_get,
	.rldp_time_period_set = switch_hw_prx126_rldp_time_period_set,
	.rldp_time_period_get = switch_hw_prx126_rldp_time_period_get,
	.rldp_portmask_set = switch_hw_prx126_rldp_portmask_set,
	.rldp_portmask_get = switch_hw_prx126_rldp_portmask_get,
	.rldp_port_status_get = switch_hw_prx126_rldp_port_status_get,
	.rldp_port_status_clear = switch_hw_prx126_rldp_port_status_clear,
	.rldp_loopedportpair_get = switch_hw_prx126_rldp_loopedportpair_get,
	.rldp_loopedportmask_get = switch_hw_prx126_rldp_loopedportmask_get,
	.rldp_loop_print = switch_hw_prx126_rldp_loop_print,

	/* switch_hw_prx126_stp.c */
	.stp_state_init = switch_hw_prx126_stp_state_init,
	.stp_state_shutdown = switch_hw_prx126_stp_state_shutdown,
	.stp_designate_data_get = switch_hw_prx126_stp_designate_data_get,
	.stp_state_print = switch_hw_prx126_stp_state_print,
	.stp_state_port_get = switch_hw_prx126_stp_state_port_get,
	.stp_state_port_set = switch_hw_prx126_stp_state_port_set,

	/* switch_hw_prx126_svlan.c */
	.svlan_service_port_set = switch_hw_prx126_svlan_service_port_set,
	.svlan_tpid_set = switch_hw_prx126_svlan_tpid_set,
	.svlan_pvid_set = switch_hw_prx126_svlan_pvid_set,
	.svlan_entry_member_get = switch_hw_prx126_svlan_entry_member_get,
	.svlan_entry_member_set = switch_hw_prx126_svlan_entry_member_set,
	.svlan_entry_delete = switch_hw_prx126_svlan_entry_delete,
	.svlan_entry_config_get = switch_hw_prx126_svlan_entry_config_get,
	.svlan_entry_config_set = switch_hw_prx126_svlan_entry_config_set,
	.svlan_entry_print = switch_hw_prx126_svlan_entry_print,

	/* switch_hw_prx126_switch.c */
	.hw_version_get = switch_hw_prx126_hw_version_get,
	.port_isolation_set = switch_hw_prx126_port_isolation_set,
	.port_isolation_get = switch_hw_prx126_port_isolation_get,
	.port_isolation_print = switch_hw_prx126_port_isolation_print,
	.port_allowed = switch_hw_prx126_port_allowed,
	.local_bridging = switch_hw_prx126_local_bridging,
	.test_mode_set = switch_hw_prx126_test_mode_set,
	.test_mode_get = switch_hw_prx126_test_mode_get,
	.unknown_mcast_drop_set = switch_hw_prx126_unknown_mcast_drop_set,
	.unknown_packet_action_set = switch_hw_prx126_unknown_packet_action_set,
	.unknown_packet_action_get = switch_hw_prx126_unknown_packet_action_get,
	.unknown_packet_flood_mask_set = switch_hw_prx126_unknown_packet_flood_mask_set,
	.unknown_packet_flood_mask_get = switch_hw_prx126_unknown_packet_flood_mask_get,
	.unknown_sa_action_set = switch_hw_prx126_unknown_sa_action_set,
	.unknown_sa_action_get = switch_hw_prx126_unknown_sa_action_get,
	.over_maclimit_action_set = switch_hw_prx126_over_maclimit_action_set,
	.over_maclimit_action_get = switch_hw_prx126_over_maclimit_action_get,

	/* switch_hw_prx126_trunk.c */
	.trunk_port_set = switch_hw_prx126_trunk_port_set,
	.trunk_port_get = switch_hw_prx126_trunk_port_get,
	.trunk_dumbmode_set = switch_hw_prx126_trunk_dumbmode_set,
	.trunk_dumbmode_get = switch_hw_prx126_trunk_dumbmode_get,
	.trunk_algorithm_set = switch_hw_prx126_trunk_algorithm_set,
	.trunk_algorithm_get = switch_hw_prx126_trunk_algorithm_get,
	.trunk_portmask_config = switch_hw_prx126_trunk_portmask_config,

	/* switch_hw_prx126_vlan.c */
	.vlan_enable = switch_hw_prx126_vlan_enable,
	.vlan_reserved_action = switch_hw_prx126_vlan_reserved_action,
	.vlan_entry_print_by_vid = switch_hw_prx126_vlan_entry_print_by_vid,
	.vlan_entry_default_set = switch_hw_prx126_vlan_entry_default_set,
	.vlan_entry_set = switch_hw_prx126_vlan_entry_set,
	.vlan_entry_delete = switch_hw_prx126_vlan_entry_delete,
	.vlan_entry_untagged_op = switch_hw_prx126_vlan_entry_untagged_op,
	.vlan_egress_mode_set = switch_hw_prx126_vlan_egress_mode_set,
	.vlan_egress_mode_print = switch_hw_prx126_vlan_egress_mode_print,
	.vlan_entry_get = switch_hw_prx126_vlan_entry_get,
	.vlan_entry_flush = switch_hw_prx126_vlan_entry_flush,

	/* switch_hw_prx126_ipmc.c */
	.ipv4mcastaddr_add = switch_hw_prx126_ipv4mcastaddr_add,
	.l2_impc_group_del = switch_hw_prx126_l2_impc_group_del,

	/* switch_hw_prx126_rg_nat.c */
	.nat_session_hw_print = NULL,	//switch_hw_prx126_rg_nat_session_hw_print
	.nat_session_sw_print = NULL,	//switch_hw_prx126_rg_nat_session_sw_print

	/* switch_hw_prx126_conv.c */
	.portid_logical_to_private = switch_hw_prx126_conv_portid_logical_to_private,
	.portid_private_to_logical = switch_hw_prx126_conv_portid_private_to_logical,
	.portmask_logical_to_private = switch_hw_prx126_conv_portmask_logical_to_private,
	.portmask_private_to_logical = switch_hw_prx126_conv_portmask_private_to_logical,

	/* switch_hw_prx126_sw_image.c */
	.sw_image_version_get = switch_hw_prx126_sw_image_version_get,
	.sw_image_valid_get = switch_hw_prx126_sw_image_valid_get,
	.sw_image_active_get = switch_hw_prx126_sw_image_active_get,
	.sw_image_active_set = switch_hw_prx126_sw_image_active_set,
	.sw_image_commit_get = switch_hw_prx126_sw_image_commit_get,
	.sw_image_commit_set = switch_hw_prx126_sw_image_commit_set,
	.sw_image_download_start = switch_hw_prx126_sw_image_download_start,
	.sw_image_download_stop = switch_hw_prx126_sw_image_download_stop,
	.sw_image_download_end = switch_hw_prx126_sw_image_download_end,
	.sw_image_download_handle_window = switch_hw_prx126_sw_image_download_handle_window,
	.sw_image_download_store = switch_hw_prx126_sw_image_download_store,

};

