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
 * Module  : cli
 * File    : cli_env.c
 *
 ******************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <dirent.h>

#include "util.h"
#include "cli.h"
#include "conv.h"
#include "env.h"
#include "omciutil_misc.h"
#include "switch.h"
#include "batchtab.h"

#include "coretask.h"	// for coretask_cmd()
#include "extoam.h"
#include "extoam_queue.h"
#include "extoam_link.h"

extern struct extoam_link_status_t cpe_link_status_g;                    
#include "cpuport_frame.h"
#include "cpuport_history.h"
#include "mcast_pkt.h"
#include "mcast.h"
#include "igmp_mbship.h"
#include "mcast_const.h"
#include "mcast_switch.h"
#include "mcast_router.h"
#include "mcast_proxy.h"

// routines for envget ///////////////////////////
// global
static void cli_env_cli_ip(int fd)
{ 
	struct in_addr addr;
	addr.s_addr = htonl(omci_env_g.cli_ip);
	util_fdprintf(fd, "%s", inet_ntoa(addr)); 
}
static void cli_env_cli_port(int fd)			{ util_fdprintf(fd, "%d", omci_env_g.cli_port); }
static void cli_env_transport_type(int fd)		{ util_fdprintf(fd, "%d", omci_env_g.transport_type); }
static void cli_env_etc_omci_path(int fd)		{ util_fdprintf(fd, "%s", omci_env_g.etc_omci_path); }
static void cli_env_etc_omci_path2(int fd)		{ util_fdprintf(fd, "%s", omci_env_g.etc_omci_path2); }
static void cli_env_start_cmd_file(int fd)		{ util_fdprintf(fd, "%s", omci_env_g.start_cmd_file); }

// interface config
static void cli_env_trap_if(int fd)			{ util_fdprintf(fd, "%s", omci_env_g.trap_if); }
static void cli_env_omcc_if(int fd)			{ util_fdprintf(fd, "%s", omci_env_g.omcc_if); }
static void cli_env_wifi_if(int fd)			{ util_fdprintf(fd, "%s", omci_env_g.wifi_if); }
static void cli_env_router_wan_if(int fd)		{ util_fdprintf(fd, "%s", omci_env_g.router_wan_if); }
static void cli_env_router_lan_if(int fd)		{ util_fdprintf(fd, "%s", omci_env_g.router_lan_if); }

// connection olt simulator
static void cli_env_sim_olt_ip(int fd)			{ util_fdprintf(fd, "%s", omci_env_g.sim_olt_ip); }
static void cli_env_sim_olt_udp_port(int fd)		{ util_fdprintf(fd, "%d", omci_env_g.sim_olt_udp_port); }
static void cli_env_sim_pon_id(int fd)			{ util_fdprintf(fd, "%d", omci_env_g.sim_pon_id); }
static void cli_env_sim_onu_id(int fd)			{ util_fdprintf(fd, "%d", omci_env_g.sim_onu_id); }
static void cli_env_sim_tcont_enable(int fd)			{ util_fdprintf(fd, "%d", omci_env_g.sim_tcont_enable); }

// omci msg
static void cli_env_omcc_rx_crc_type(int fd)		{ util_fdprintf(fd, "%d", omci_env_g.omcc_rx_crc_type); }
static void cli_env_omcc_tx_crc_type(int fd)		{ util_fdprintf(fd, "%d", omci_env_g.omcc_tx_crc_type); }
static void cli_env_omci_tranx_cache_num(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.omci_tranx_cache_num); }
static void cli_env_omci_tranx_match_mode(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.omci_tranx_match_mode); }
static void cli_env_omci_tranx_bypass(int fd)		{ util_fdprintf(fd, "%d", omci_env_g.omci_tranx_bypass); }
static void cli_env_omci_tranx_busy_enable(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.omci_tranx_busy_enable); }

static void cli_env_omci_exec_time_limitation(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.omci_exec_time_limitation); }
static void cli_env_omci_tx_flag(int fd)		{ util_fdprintf(fd, "%d", omci_env_g.omci_tx_flag); }
static void cli_env_omci_tx_resend_flag(int fd)		{ util_fdprintf(fd, "%d", omci_env_g.omci_tx_resend_flag); }

static void cli_env_omci_history_enable(int fd)		{ util_fdprintf(fd, "%d", omci_env_g.omci_history_enable); }
static void cli_env_omci_history_size(int fd)		{ util_fdprintf(fd, "%u", omci_env_g.omci_history_size); }
static void cli_env_omci_history_msgmask(int fd)		{ util_fdprintf(fd, "0x%08x", omci_env_g.omci_history_msgmask); }

static void cli_env_omcidump_enable(int fd)		{ util_fdprintf(fd, "%d", omci_env_g.omcidump_enable); }
static void cli_env_omcidump_msgmask(int fd)		{ util_fdprintf(fd, "0x%08x", omci_env_g.omcidump_msgmask); }

static void cli_env_omccrx_log_enable(int fd)		{ util_fdprintf(fd, "%d", omci_env_g.omccrx_log_enable); }
static void cli_env_omccrx_log_msgmask(int fd)		{ util_fdprintf(fd, "0x%08x", omci_env_g.omccrx_log_msgmask); }
static void cli_env_omccrx_log_file(int fd)		{ util_fdprintf(fd, "%s", omci_env_g.omccrx_log_file); }

static void cli_env_restore_log_enable(int fd)		{ util_fdprintf(fd, "%d", omci_env_g.restore_log_enable); }
static void cli_env_restore_log_file(int fd)		{ util_fdprintf(fd, "%s", omci_env_g.restore_log_file); }

// mib
static void cli_env_xml_section(int fd)
{
	int i;
	for (i=0; omci_env_g.xml_section[i]!=0 && i<ENV_SIZE_XML_SECTION; i++)
		util_fdprintf(fd, "%s%d", (i!=0)?",":"", omci_env_g.xml_section[i]);
}
static void cli_env_xml_load_namestr(int fd)		{ util_fdprintf(fd, "%d", omci_env_g.xml_load_namestr); }

static void cli_env_mib_upload_order(int fd)
{
	int i;
	for (i=0; omci_env_g.mib_upload_order[i]!=0 && i<ENV_SIZE_MIB_UPLOAD_ORDER; i++)
		util_fdprintf(fd, "%s%d", (i!=0)?",":"", omci_env_g.mib_upload_order[i]);
}
static void cli_env_mib_boundary_check(int fd)		{ util_fdprintf(fd, "%d", omci_env_g.mib_boundary_check); }
static void cli_env_mib_pointer_check(int fd)		{ util_fdprintf(fd, "%d", omci_env_g.mib_pointer_check); }
static void cli_env_mib_readonly_check(int fd)		{ util_fdprintf(fd, "%d", omci_env_g.mib_readonly_check); }
static void cli_env_mib_rw_mode(int fd)		{ util_fdprintf(fd, "%d", omci_env_g.mib_rw_mode); }
static void cli_env_mib_aliasid_enable(int fd)		{ util_fdprintf(fd, "%d", omci_env_g.mib_aliasid_enable); }
static void cli_env_mib_rare_meid_start(int fd)		{ util_fdprintf(fd, "0x%x", omci_env_g.mib_rare_meid_start); }
static void cli_env_mib_rare2_meid_start(int fd)		{ util_fdprintf(fd, "0x%x", omci_env_g.mib_rare2_meid_start); }
static void cli_env_mib_nonexist_del_err(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.mib_nonexist_del_err); }

static void cli_env_autouni_enable(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.autouni_enable); }
static void cli_env_autoveip_enable(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.autoveip_enable); }
static void cli_env_erdiag_mon_enable(int fd)		{ util_fdprintf(fd, "%d", omci_env_g.erdiag_mon_enable); }
static void cli_env_mibreset_dump_enable(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.mibreset_dump_enable); }
static void cli_env_mib_uni_adminlock_default(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.mib_uni_adminlock_default); }
static void cli_env_adminlock_trigger_opstate(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.adminlock_trigger_opstate); }
static void cli_env_untag_reserved_vid (int fd) 	{ util_fdprintf(fd, "0x%x(%d)", omci_env_g.untag_reserved_vid, omci_env_g.untag_reserved_vid ); }

static void cli_env_port_forward_operation_default(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.port_forward_operation_default); }

static void cli_env_pseudo_ts_pointer_enable(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.pseudo_ts_pointer_enable); }
static void cli_env_invert_weight_enable(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.invert_weight_enable); }
static void cli_env_invert_priority_enable(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.invert_priority_enable); }
static void cli_env_pq_tcont_relation(int fd)		{ util_fdprintf(fd, "%d", omci_env_g.pq_tcont_relation); }

static void cli_env_tcont_map_enable(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.tcont_map_enable); }
static void cli_env_tcont_map_policy(int fd)	
{ 
	util_fdprintf(fd, "%s,%s", 
		(omci_env_g.tcont_map_policy[0]==2)?"WRR":"SP", 
		(omci_env_g.tcont_map_policy[1]==2)?"WRR":"SP"); 
}
static void cli_env_tcont_map_ts(int fd)
{
	int i;
	for (i=0; i<ENV_SIZE_TCONT_MAP; i++) {
		if (omci_env_g.tcont_map_ts[i]!=0)
			util_fdprintf(fd, "%s%d", (i!=0)?",":"", omci_env_g.tcont_map_ts[i]);
	}
}
static void cli_env_tcont_map_pq(int fd)
{
	int i;
	for (i=0; i<ENV_SIZE_TCONT_MAP; i++) {
		if (omci_env_g.tcont_map_pq[i]!=0)
			util_fdprintf(fd, "%s%d", (i!=0)?",":"", omci_env_g.tcont_map_pq[i]);
	}
}

// sw download
static void cli_env_sw_download_end_behavior(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.sw_download_end_behavior); }
static void cli_env_sw_download_active_always_ok(int fd){ util_fdprintf(fd, "%d", omci_env_g.sw_download_active_always_ok); }
static void cli_env_sw_download_commit_always_ok(int fd){ util_fdprintf(fd, "%d", omci_env_g.sw_download_commit_always_ok); }
static void cli_env_sw_download_window_size(int fd){ util_fdprintf(fd, "%d", omci_env_g.sw_download_window_size); }
static void cli_env_sw_download_script(int fd)		{ util_fdprintf(fd, "%s", omci_env_g.sw_download_script); }
static void cli_env_sw_download_image_name(int fd)	{ util_fdprintf(fd, "%s", omci_env_g.sw_download_image_name); }

// thread tasks
static void cli_env_task_pause_mask(int fd)		{ util_fdprintf(fd, "0x%04x", omci_env_g.task_pause_mask); }
static void cli_env_avc_refresh_interval(int fd)	{ util_fdprintf(fd, "%u", omci_env_g.avc_refresh_interval); }
static void cli_env_alarm_refresh_interval(int fd)	{ util_fdprintf(fd, "%u", omci_env_g.alarm_refresh_interval); }
static void cli_env_pm_refresh_interval(int fd)		{ util_fdprintf(fd, "%u", omci_env_g.pm_refresh_interval); }
static void cli_env_pm_swap_interval(int fd)		{ util_fdprintf(fd, "%u", omci_env_g.pm_swap_interval); }

static void cli_env_cpuport_history_enable(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.cpuport_history_enable); }
static void cli_env_cpuport_history_size(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.cpuport_history_size); }
static void cli_env_cpuport_arp_enable(int fd)		{ util_fdprintf(fd, "%d", omci_env_g.cpuport_arp_enable); }
static void cli_env_cpuport_dhcp_enable(int fd)		{ util_fdprintf(fd, "%d", omci_env_g.cpuport_dhcp_enable); }
static void cli_env_cpuport_dhcp_cust_tag(int fd)
{
	if (omci_env_g.cpuport_dhcp_cust_tag[0]==0) {
		util_fdprintf(fd, "0x%02x", omci_env_g.cpuport_dhcp_cust_tag[0]);
	} else {
		int i;
		for (i=0; i<=omci_env_g.cpuport_dhcp_cust_tag[0]; i++)
			util_fdprintf(fd, "%s0x%02x", (i!=0)?",":"", omci_env_g.cpuport_dhcp_cust_tag[i]);
	}
}
static void cli_env_cpuport_dhcp_server_onlan(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.cpuport_dhcp_server_onlan); }
static void cli_env_cpuport_pppoe_enable(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.cpuport_pppoe_enable); }
static void cli_env_cpuport_pppoe_cust_tag(int fd)
{
	if (omci_env_g.cpuport_pppoe_cust_tag[0]==0) {
		util_fdprintf(fd, "0x%02x", omci_env_g.cpuport_pppoe_cust_tag[0]);
	} else {
		int i;
		for (i=0; i<=omci_env_g.cpuport_pppoe_cust_tag[0]; i++)
			util_fdprintf(fd, "%s0x%02x", (i!=0)?",":"", omci_env_g.cpuport_pppoe_cust_tag[i]);
	}
}
static void cli_env_cpuport_pkt_ssdp_fwd_wan(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.cpuport_pkt_ssdp_fwd_wan); }
static void cli_env_cpuport_im_enable(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.cpuport_im_enable); }
static void cli_env_cpuport_im_lan2lan_enable(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.cpuport_im_lan2lan_enable); }
static void cli_env_cpuport_wifi_block_mask(int fd)	{ util_fdprintf(fd, "0x%02x", omci_env_g.cpuport_wifi_block_mask); }
static void cli_env_cpuport_wifi_block_ra(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.cpuport_wifi_block_ra); }
static void cli_env_cpuport_mcast_drop(int fd)	{ util_fdprintf(fd, "0x%02x", omci_env_g.cpuport_mcast_drop); }


static void cli_env_stp_enable(int fd)			{ util_fdprintf(fd, "%d", omci_env_g.stp_enable); }
static void cli_env_cpu_usage_threshold(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.cpu_usage_threshold); }
//for cpulimit tasks no
static void cli_env_cpul_task_no(int fd)
{
	int i;
	for (i=0; i<ENV_SIZE_TASK; i++) {
		if (omci_env_g.cpul_task_no[i]!=-1)
			util_fdprintf(fd, "%s%d", (i!=0)?",":"", omci_env_g.cpul_task_no[i]);
	}
}
static void cli_env_cpul_total_usage(int fd)		{ util_fdprintf(fd, "%d", omci_env_g.cpul_total_usage); }

// debug
static void cli_env_debug_level(int fd)			{ util_fdprintf(fd, "%d", omci_env_g.debug_level); }
static void cli_env_debug_level_bat(int fd)		{ util_fdprintf(fd, "%d", omci_env_g.debug_level_bat); }
static void cli_env_debug_level_cfm(int fd)		{ util_fdprintf(fd, "%d", omci_env_g.debug_level_cfm); }
static void cli_env_debug_level_cpuport(int fd)		{ util_fdprintf(fd, "%d", omci_env_g.debug_level_cpuport); }
static void cli_env_debug_level_fwk(int fd)		{ util_fdprintf(fd, "%d", omci_env_g.debug_level_fwk); }
static void cli_env_debug_level_igmp(int fd)		{ util_fdprintf(fd, "%d", omci_env_g.debug_level_igmp); }
static void cli_env_debug_level_lldp(int fd)		{ util_fdprintf(fd, "%d", omci_env_g.debug_level_lldp); }
static void cli_env_debug_level_vacl(int fd)		{ util_fdprintf(fd, "%d", omci_env_g.debug_level_vacl); }
static void cli_env_debug_level_voip(int fd)		{ util_fdprintf(fd, "%d", omci_env_g.debug_level_voip); }
static void cli_env_debug_level_xml(int fd)		{ util_fdprintf(fd, "%d", omci_env_g.debug_level_xml); }
static void cli_env_debug_log_time(int fd)		{ util_fdprintf(fd, "%d", omci_env_g.debug_log_time); }
static void cli_env_debug_log_type(int fd)		{ util_fdprintf(fd, "%d", omci_env_g.debug_log_type); }
static void cli_env_debug_log_file(int fd)		{ util_fdprintf(fd, "%s", omci_env_g.debug_log_file); }

static void cli_env_memdbg_enable(int fd)		{ util_fdprintf(fd, "%d", omci_env_g.memdbg_enable); }
static void cli_env_er_group_hw_debug_enable(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.er_group_hw_debug_enable); }
static void cli_env_er_group_hw_time_enable (int fd)	{ util_fdprintf(fd, "%d", omci_env_g.er_group_hw_time_enable); }

// olt 
static void cli_env_olt_vendor_check_enable(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.olt_vendor_check_enable); }
static void cli_env_olt_vendor_config_change(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.olt_vendor_config_change); }
static void cli_env_olt_workaround_enable(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.olt_workaround_enable); }
static void cli_env_olt_alu_mcast_support(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.olt_alu_mcast_support); }
static void cli_env_olt_huawei_ignore_vlan_drop_rule(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.olt_huawei_ignore_vlan_drop_rule); }
static void cli_env_olt_proprietary_support(int fd)	{ util_fdprintf(fd, "%s", omci_env_g.olt_proprietary_support); }
static void cli_env_ont_force_vendor_id(int fd)	{ util_fdprintf(fd, "%s", omci_env_g.ont_force_vendor_id); }

// anig
static void cli_env_anig_type(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.anig_type); }
static void cli_env_anig_support(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.anig_support); }
static void cli_env_anig_txrx_threshold(int fd)
{
	util_fdprintf(fd, "%d,%d,%d,%d", 
		omci_env_g.anig_txrx_threshold[0], omci_env_g.anig_txrx_threshold[1],
		omci_env_g.anig_txrx_threshold[2], omci_env_g.anig_txrx_threshold[3]);
}
static void cli_env_anig_devport(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.anig_devport); }
static void cli_env_anig_devaddr0(int fd)	{ util_fdprintf(fd, "0x%x", omci_env_g.anig_devaddr0); }
static void cli_env_anig_mon_mode(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.anig_mon_mode); }

// logical port id/mask
static void cli_env_wan_logical_portid(int fd)		{ util_fdprintf(fd, "%d", omci_env_g.wan_logical_portid); }
static void cli_env_uni_logical_portmask(int fd)	{ util_fdprintf(fd, "0x%x", omci_env_g.uni_logical_portmask); }
static void cli_env_cpu_logical_portmask(int fd)	{ util_fdprintf(fd, "0x%x", omci_env_g.cpu_logical_portmask); }
static void cli_env_wifi_logical_portmask(int fd)	{ util_fdprintf(fd, "0x%x", omci_env_g.wifi_logical_portmask); }

// switch
static void cli_env_ivlan_enable(int fd) { util_fdprintf(fd, "%d", omci_env_g.ivlan_enable); }
static void cli_env_stp_conf_before_omci(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.stp_conf_before_omci); }
static void cli_env_switch_history_mode(int fd)		{ util_fdprintf(fd, "%d", omci_env_g.switch_history_mode); }
static void cli_env_switch_history_interval(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.switch_history_interval); }

static void cli_env_br_allowed_srcmask(int fd)		{ util_fdprintf(fd, "0x%x", omci_env_g.br_allowed_srcmask); }
static void cli_env_rt_allowed_srcmask(int fd)		{ util_fdprintf(fd, "0x%x", omci_env_g.rt_allowed_srcmask); }
static void cli_env_trunk_group_portmask(int fd){ util_fdprintf(fd, "0x%x", omci_env_g.trunk_group_portmask); }

static void cli_env_localbridging_default(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.localbridging_default); }
static void cli_env_localbridging_addon_portmask(int fd){ util_fdprintf(fd, "0x%x", omci_env_g.localbridging_addon_portmask); }
static void cli_env_ctc_internet_bridge_enabled_srcmask(int fd)	{ util_fdprintf(fd, "0x%x", omci_env_g.ctc_internet_bridge_enabled_srcmask); }

static void cli_env_port2port_enable(int fd)		{ util_fdprintf(fd, "%d", omci_env_g.port2port_enable); }
static void cli_env_maclearn_drop_enable(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.maclearn_drop_enable); }
static void cli_env_veip_for_wifi_enable(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.veip_for_wifi_enable); }
static void cli_env_cpuport_ds_passthrough (int fd)	{ util_fdprintf(fd, "%d", omci_env_g.cpuport_ds_passthrough ); }
static void cli_env_cpuport_pvlan_clone_ipv4_to_arp (int fd)	{ util_fdprintf(fd, "%d", omci_env_g.cpuport_pvlan_clone_ipv4_to_arp ); }
static void cli_env_rma_forward_mask (int fd)		{ util_fdprintf(fd, "0x%0llx", omci_env_g.rma_forward_mask ); }
static void cli_env_bundle_rate_support(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.bundle_rate_support); }
static void cli_env_linkdown_debounce_time(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.linkdown_debounce_time); }
static void cli_env_linkup_debounce_time(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.linkup_debounce_time); }

// rldp
static void cli_env_rldp_enable(int fd)			{ util_fdprintf(fd, "%d", omci_env_g.rldp_enable); }
static void cli_env_rldp_def_portmask(int fd)		{ util_fdprintf(fd, "0x%x", omci_env_g.rldp_def_portmask); }
static void cli_env_rldp_passive_only(int fd)		{ util_fdprintf(fd, "%d", omci_env_g.rldp_passive_only); }

// tag processing
static void cli_env_tag_treatment_correct(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.tag_treatment_correct); }
static void cli_env_tag_discard_reversing(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.tag_discard_reversing); }
static void cli_env_tag_preview_table(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.tag_preview_table); }
static void cli_env_tag_auto_reverse(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.tag_auto_reverse); }
static void cli_env_tag_mc_ds_from_us(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.tag_mc_ds_from_us); }
static void cli_env_tag_mc_sort_priority(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.tag_mc_sort_priority); }
static void cli_env_tag_pbit_workaround(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.tag_pbit_workaround); }
static void cli_env_tag_pbit_portmask(int fd)	{ util_fdprintf(fd, "0x%.2x", omci_env_g.tag_pbit_portmask); }
static void cli_env_tag_pbit_remap_table(int fd)
{
	int i;
	for (i=0; i<8; i++)
		util_fdprintf(fd, "%s%d", (i!=0)?",":"", omci_env_g.tag_pbit_remap_table[i]);
}
static void cli_env_tag_ds_tci_dont_reverse_workaround(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.tag_ds_tci_dont_reverse_workaround); }
static void cli_env_tag_insert_65305_rules(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.tag_insert_65305_rules); }
static void cli_env_tag_ctc_translation_path(int fd)		{ util_fdprintf(fd, "%s", omci_env_g.tag_ctc_translation_path); }
static void cli_env_tag_mc_ds_wildcard(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.tag_mc_ds_wildcard); }
static void cli_env_tag_l2_rules_reverse(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.tag_l2_rules_reverse); }
static void cli_env_tag_l2_rules_ds_mac(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.tag_l2_rules_ds_mac); }
static void cli_env_tag_iphost_vid_map(int fd)
{
	int i;
	for (i=0; i<ENV_SIZE_IPHOST_PORT; i++)
		util_fdprintf(fd, "%s%d", (i!=0)?",":"", omci_env_g.tag_iphost_vid_map[i]);
}

static void cli_env_classf_tag_mode(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.classf_tag_mode); }
static void cli_env_classf_add_default_rules(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.classf_add_default_rules); }
static void cli_env_classf_force_stpid(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.classf_force_stpid); }
static void cli_env_classf_stag_tpid(int fd)	{ util_fdprintf(fd, "0x%.4x", omci_env_g.classf_stag_tpid); }
static void cli_env_classf_hw_stag_tpid(int fd)	{ util_fdprintf(fd, "0x%.4x", omci_env_g.classf_hw_stag_tpid); }
static void cli_env_classf_et_combine(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.classf_et_combine); }
static void cli_env_classf_ds_gem_dg(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.classf_ds_gem_dg); }
static void cli_env_classf_dscp2pbit_mode(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.classf_dscp2pbit_mode); }
#if 0 // Remove VEIP related workaround
static void cli_env_classf_veip_orig_tci_map(int fd)
{
	int i;
	for (i=0; i<ENV_SIZE_VEIP_PORT; i++)
		util_fdprintf(fd, "%s%d", (i!=0)?",":"", omci_env_g.classf_veip_orig_tci_map[i]);
}
static void cli_env_classf_veip_real_tci_map(int fd)
{
	int i;
	for (i=0; i<ENV_SIZE_VEIP_PORT; i++)
		util_fdprintf(fd, "%s%d", (i!=0)?",":"", omci_env_g.classf_veip_real_tci_map[i]);
}
static void cli_env_classf_veip_real_tag_rule_priority(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.classf_veip_real_tag_rule_priority); }
#endif

// classf
static void cli_env_classf_ds_unmatch(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.classf_ds_unmatch); }
static void cli_env_classf_ds_mc_extend(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.classf_ds_mc_extend); }
static void cli_env_classf_ds_mc_force_strip(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.classf_ds_mc_force_strip); }
static void cli_env_classf_veip_rules_reassign(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.classf_veip_rules_reassign); }
static void cli_env_classf_dropping_rules_add(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.classf_dropping_rules_add); }
static void cli_env_classf_vmap_pbit_workaround(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.classf_vmap_pbit_workaround); }
static void cli_env_classf_hw_2tags_mode(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.classf_hw_2tags_mode); }
static void cli_env_classf_hw_2tags_workaround(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.classf_hw_2tags_workaround); }
static void cli_env_classf_add_ipv4mc_strip(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.classf_add_ipv4mc_strip); }
static void cli_env_classf_add_hwnat_1tag_rules(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.classf_add_hwnat_1tag_rules); }
static void cli_env_classf_add_hwnat_untag_rules(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.classf_add_hwnat_untag_rules); }
static void cli_env_classf_add_hwnat_untag_vid(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.classf_add_hwnat_untag_vid); }
static void cli_env_classf_add_hwnat_untag_vid_mask(int fd)	{ util_fdprintf(fd, "0x%.3x", omci_env_g.classf_add_hwnat_untag_vid_mask); }
static void cli_env_classf_hw_rules_migration(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.classf_hw_rules_migration); }
static void cli_env_classf_hw_colour_force(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.classf_hw_colour_force); }
static void cli_env_classf_keep_candi(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.classf_keep_candi); }
static void cli_env_classf_statistic_enable(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.classf_statistic_enable); }
static void cli_env_classf_tlan_pattern(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.classf_tlan_pattern); }
static void cli_env_classf_hw_intf_lan_vid(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.classf_hw_intf_lan_vid); }
static void cli_env_mc_ds_untag_discard_enable(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.mc_ds_untag_discard_enable); }
static void cli_env_classf_add_default_rules_by_me_84(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.classf_add_default_rules_by_me_84); }

// cfm
static void cli_env_cfm_enable(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.cfm_enable); }
static void cli_env_cfm_y1731_enable(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.cfm_y1731_enable); }
static void cli_env_cfm_portmask(int fd)	{ util_fdprintf(fd, "0x%x", omci_env_g.cfm_portmask); }
static void cli_env_cfm_mac_ifname(int fd)		{ util_fdprintf(fd, "%s", omci_env_g.cfm_mac_ifname); }
static void cli_env_cfm_send_timeout(int fd)		{ util_fdprintf(fd, "%d", omci_env_g.cfm_send_timeout); }
static void cli_env_cfm_reverse_mep_direction(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.cfm_reverse_mep_direction); }
static void cli_env_cfm_rmep_ccm_before_hold(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.cfm_rmep_ccm_before_hold); }
static void cli_env_cfm_rmep_holdtime_before_active(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.cfm_rmep_holdtime_before_active); }
static void cli_env_cfm_rmep_ccm_lost_threshold(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.cfm_rmep_ccm_lost_threshold); }
static void cli_env_cfm_defect_mask_in_rdi_gen(int fd)	{ util_fdprintf(fd, "0x%02x", omci_env_g.cfm_defect_mask_in_rdi_gen); }
static void cli_env_cfm_defect_mask_in_ais_gen(int fd)	{ util_fdprintf(fd, "0x%02x", omci_env_g.cfm_defect_mask_in_ais_gen); }
static void cli_env_cfm_defect_mask_supressed_by_ais(int fd)	{ util_fdprintf(fd, "0x%02x", omci_env_g.cfm_defect_mask_supressed_by_ais); }
static void cli_env_cfm_defect_mask_supressed_by_lck(int fd)	{ util_fdprintf(fd, "0x%02x", omci_env_g.cfm_defect_mask_supressed_by_lck); }

// lldp
static void cli_env_lldp_enable(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.lldp_enable); }
static void cli_env_lldp_ifname(int fd)	{ util_fdprintf(fd, "%s", omci_env_g.lldp_ifname); }
static void cli_env_lldp_dst_mac(int fd)	{ util_fdprintf(fd, "0x%x", omci_env_g.lldp_dst_mac); }
static void cli_env_lldp_send_interval(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.lldp_send_interval); }
static void cli_env_lldp_fast_tx(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.lldp_fast_tx); }
static void cli_env_lldp_credit_max(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.lldp_credit_max); }
static void cli_env_lldp_tx_fast_init(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.lldp_tx_fast_init); }
static void cli_env_lldp_system_name(int fd)	{ util_fdprintf(fd, "%s", omci_env_g.lldp_system_name); }
static void cli_env_lldp_system_desc(int fd)	{ util_fdprintf(fd, "%s", omci_env_g.lldp_system_desc); }
static void cli_env_lldp_med_opt(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.lldp_med_opt); }
static void cli_env_lldp_med_cap(int fd)	{ util_fdprintf(fd, "0x%04x", omci_env_g.lldp_med_cap); }

// tm
static void cli_env_tm_pq_gem_qos_method(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.tm_pq_gem_qos_method); }
static void cli_env_tm_gemus_rate_factor(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.tm_gemus_rate_factor); }
static void cli_env_tm_gemds_rate_factor(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.tm_gemds_rate_factor); }
static void cli_env_tm_unius_rate_factor(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.tm_unius_rate_factor); }
static void cli_env_tm_unids_rate_factor(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.tm_unids_rate_factor); }
static void cli_env_tm_pir_zero_action(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.tm_pir_zero_action); }
static void cli_env_tm_protect_enable(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.tm_protect_enable); }

// igmp
static void cli_env_igmp_enable(int fd)		{ util_fdprintf(fd, "%d", omci_env_g.igmp_enable); }
static void cli_env_igmp_v1_ignore(int fd)		{ util_fdprintf(fd, "%d", omci_env_g.igmp_v1_ignore); }
static void cli_env_igmp_version_overwrite(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.igmp_version_overwrite); }
static void cli_env_igmp_join_default_grant(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.igmp_join_default_grant); }
static void cli_env_igmp_mcast_me_support(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.igmp_mcast_me_support); }
static void cli_env_igmp_proxy_snooping(int fd)		{ util_fdprintf(fd, "%d", omci_env_g.igmp_proxy_snooping ); }
static void cli_env_igmp_upstream_tpid(int fd)		{ util_fdprintf(fd, "0x%x", omci_env_g.igmp_upstream_tpid); }
static void cli_env_igmp_downstream_tpid(int fd)	{ util_fdprintf(fd, "0x%x", omci_env_g.igmp_downstream_tpid); }
static void cli_env_igmp_query_pass_through(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.igmp_query_pass_through); }
static void cli_env_igmp_preassign_list(int fd)		{ env_print_igmp_preassign_list(fd); } 
static void cli_env_igmp_mcast_mode_src(int fd)		{ util_fdprintf(fd, "%d", omci_env_g.igmp_mcast_mode_src); }

// extoam
static void cli_env_extoam_enable(int fd)		{ util_fdprintf(fd, "%d", omci_env_g.extoam_enable); }
static void cli_env_extoam_port(int fd)			{ util_fdprintf(fd, "%d", omci_env_g.extoam_port); }
static void cli_env_extoam_mac_ifname(int fd)		{ util_fdprintf(fd, "%s", omci_env_g.extoam_mac_ifname); }
static void cli_env_extoam_itas_image_name(int fd)	{ util_fdprintf(fd, "%s", omci_env_g.extoam_itas_image_name); }

// voip
static void cli_env_voip_enable(int fd)			{ util_fdprintf(fd, "%d", omci_env_g.voip_enable); }
//flags for voip debug
static void cli_env_voip_debug_set_flag(int fd)		{ util_fdprintf(fd, "%d", omci_env_g.voip_debug_set_flag); }
static void cli_env_voip_debug_get_flag(int fd)		{ util_fdprintf(fd, "%d", omci_env_g.voip_debug_get_flag); }
static void cli_env_voip_debug_release_flag(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.voip_debug_release_flag); }

// misc
static void cli_env_batchtab_extevent_idletime(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.batchtab_extevent_idletime); }
static void cli_env_batchtab_retry_timeout(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.batchtab_retry_timeout); }
static void cli_env_batchtab_wanif_feature(int fd)	{ util_fdprintf(fd, "0x%x", omci_env_g.batchtab_wanif_feature); }
static void cli_env_batchtab_hardbridge_enable(int fd)	{ util_fdprintf(fd, "0x%x", omci_env_g.batchtab_hardbridge_enable); }
static void cli_env_exclude_flowid_0(int fd)		{ util_fdprintf(fd, "%d", omci_env_g.exclude_flowid_0); }
static void cli_env_avoid_pir_drop(int fd)		{ util_fdprintf(fd, "%d", omci_env_g.avoid_pir_drop); }
static void cli_env_uni_policing_pbit_mask(int fd)	{ util_fdprintf(fd, "0x%x", omci_env_g.uni_policing_pbit_mask); }
static void cli_env_uni_policing_default_bs(int fd)	{ util_fdprintf(fd, "0x%x", omci_env_g.uni_policing_default_bs); }
static void cli_env_uni_policing_aggregate_acl(int fd)	{ util_fdprintf(fd, "%d", omci_env_g.uni_policing_aggregate_acl); }
static void cli_env_uni_linkdown_lock(int fd)		{ util_fdprintf(fd, "%d", omci_env_g.uni_linkdown_lock); }

static void cli_env_watchdog_omccmiss_timeout(int fd)	{ util_fdprintf(fd, "%u,%u", omci_env_g.watchdog_omccmiss_timeout[0], omci_env_g.watchdog_omccmiss_timeout[1]); }
static void cli_env_watchdog_tcontmiss_timeout(int fd)	{ util_fdprintf(fd, "%u,%u", omci_env_g.watchdog_tcontmiss_timeout[0], omci_env_g.watchdog_tcontmiss_timeout[1]); }
static void cli_env_watchdog_o2stuck_timeout(int fd)	{ util_fdprintf(fd, "%u,%u", omci_env_g.watchdog_o2stuck_timeout[0], omci_env_g.watchdog_o2stuck_timeout[1]); }
static void cli_env_watchdog_idlebw_guard(int fd)	{ util_fdprintf(fd, "%u,%u,%u,%u", omci_env_g.watchdog_idlebw_guard[0], omci_env_g.watchdog_idlebw_guard[1], omci_env_g.watchdog_idlebw_guard[2], omci_env_g.watchdog_idlebw_guard[3]); }

static void cli_env_uni_config_lock(int fd)       { util_fdprintf(fd, "%d", omci_env_g.uni_config_lock); }

// routines for envset //////////////////////////////////////////
// global
static void cli_env_set_cli_ip(int fd, char *str)	{ omci_env_g.cli_ip=(strcmp(str, "0") == 0)?0:ntohl(inet_addr(str)); }
static void cli_env_set_cli_port(int fd, char *str)	{ omci_env_g.cli_port=util_atoi(str); }
static void cli_env_set_transport_type(int fd, char *str)	{ omci_env_g.transport_type=util_atoi(str); }
static void cli_env_set_etc_omci_path(int fd, char *str)	{ strncpy(omci_env_g.etc_omci_path, str, ENV_SIZE_PATH); }
static void cli_env_set_etc_omci_path2(int fd, char *str)	{ strncpy(omci_env_g.etc_omci_path2, str, ENV_SIZE_PATH); }
static void cli_env_set_start_cmd_file(int fd, char *str)	{ strncpy(omci_env_g.start_cmd_file, str, ENV_SIZE_PATH); }

// interface config
static void cli_env_set_trap_if(int fd, char *str)		{ strncpy(omci_env_g.trap_if, str, ENV_SIZE_IFNAME); }
static void cli_env_set_omcc_if(int fd, char *str)		{ strncpy(omci_env_g.omcc_if, str, ENV_SIZE_IFNAME); }
static void cli_env_set_wifi_if(int fd, char *str)		{ strncpy(omci_env_g.wifi_if, str, ENV_SIZE_IFNAME); }
static void cli_env_set_router_lan_if(int fd, char *str)
{ 
	strncpy(omci_env_g.router_lan_if, str, ENV_SIZE_IFNAME); 
}
static void cli_env_set_router_wan_if(int fd, char *str)
{ 
	strncpy(omci_env_g.router_wan_if, str, ENV_SIZE_IFNAME); 
}

// connecting olt simulator
static void cli_env_set_sim_olt_ip(int fd, char *str)		{ strncpy(omci_env_g.sim_olt_ip, str, ENV_SIZE_IPSTR); }
static void cli_env_set_sim_olt_udp_port(int fd, char *str)	{ omci_env_g.sim_olt_udp_port=util_atoi(str); }
static void cli_env_set_sim_pon_id(int fd, char *str)		{ omci_env_g.sim_pon_id=util_atoi(str); }
static void cli_env_set_sim_onu_id(int fd, char *str)		{ omci_env_g.sim_onu_id=util_atoi(str); }
static void cli_env_set_sim_tcont_enable(int fd, char *str)		{ omci_env_g.sim_tcont_enable=util_atoi(str); env_set_sim_tcont_enable(omci_env_g.sim_tcont_enable); }

// omci msg
static void cli_env_set_omcc_rx_crc_type(int fd, char *str)	{ omci_env_g.omcc_rx_crc_type=util_atoi(str); }
static void cli_env_set_omcc_tx_crc_type(int fd, char *str)	{ omci_env_g.omcc_tx_crc_type=util_atoi(str); }

static void cli_env_set_omci_tranx_cache_num(int fd, char *str)	{ omci_env_g.omci_tranx_cache_num=util_atoi(str); }
static void cli_env_set_omci_tranx_match_mode(int fd, char *str)	{ omci_env_g.omci_tranx_match_mode=util_atoi(str); }
static void cli_env_set_omci_tranx_bypass(int fd, char *str)		{ omci_env_g.omci_tranx_bypass=util_atoi(str); }
static void cli_env_set_omci_tranx_busy_enable(int fd, char *str)	{ omci_env_g.omci_tranx_busy_enable=util_atoi(str); }

static void cli_env_set_omci_exec_time_limitation(int fd, char *str)	{ omci_env_g.omci_exec_time_limitation=util_atoi(str); }
static void cli_env_set_omci_tx_flag(int fd, char *str)	{ omci_env_g.omci_tx_flag=util_atoi(str); }
static void cli_env_set_omci_tx_resend_flag(int fd, char *str)	{ omci_env_g.omci_tx_resend_flag=util_atoi(str); }

static void cli_env_set_omci_history_enable(int fd, char *str)	{ omci_env_g.omci_history_enable=util_atoi(str); }
static void cli_env_set_omci_history_size(int fd, char *str)	{ omci_env_g.omci_history_size=util_atoi(str); }
static void cli_env_set_omci_history_msgmask(int fd, char *str)	{ omci_env_g.omci_history_msgmask=util_atoll(str); }

static void cli_env_set_omcidump_enable(int fd, char *str)		{ omci_env_g.omcidump_enable=util_atoll(str); }
static void cli_env_set_omcidump_msgmask(int fd, char *str)		{ omci_env_g.omcidump_msgmask=util_atoll(str); }

static void cli_env_set_omccrx_log_enable(int fd, char *str)	{ omci_env_g.omccrx_log_enable=util_atoi(str); }
static void cli_env_set_omccrx_log_msgmask(int fd, char *str)	{ omci_env_g.omccrx_log_msgmask=util_atoll(str); }
static void cli_env_set_omccrx_log_file(int fd, char *str)	{ strncpy(omci_env_g.omccrx_log_file, str, ENV_SIZE_PATH); }

static void cli_env_set_restore_log_enable(int fd, char *str)	{ omci_env_g.restore_log_enable=util_atoi(str); }
static void cli_env_set_restore_log_file(int fd, char *str)	{ strncpy(omci_env_g.restore_log_file, str, ENV_SIZE_PATH); }

// mib
static void cli_env_set_xml_section(int fd, char *str)
{
	conv_numlist2array(str, omci_env_g.xml_section, sizeof(unsigned short), ENV_SIZE_XML_SECTION);
}
static void cli_env_set_xml_load_namestr(int fd, char *str)	{ omci_env_g.xml_load_namestr=util_atoi(str); }

static void cli_env_set_mib_upload_order(int fd, char *str)
{
	conv_numlist2array(str, omci_env_g.mib_upload_order, sizeof(unsigned short), ENV_SIZE_MIB_UPLOAD_ORDER);
}
static void cli_env_set_mib_boundary_check(int fd, char *str)	{ omci_env_g.mib_boundary_check=util_atoi(str); }
static void cli_env_set_mib_pointer_check(int fd, char *str)	{ omci_env_g.mib_pointer_check=util_atoi(str); }
static void cli_env_set_mib_readonly_check(int fd, char *str)	{ omci_env_g.mib_readonly_check=util_atoi(str); }
static void cli_env_set_mib_rw_mode(int fd, char *str)		{ omci_env_g.mib_rw_mode=util_atoi(str); }
static void cli_env_set_mib_aliasid_enable(int fd, char *str)		{ omci_env_g.mib_aliasid_enable=util_atoi(str); }
static void cli_env_set_mib_rare_meid_start(int fd, char *str)		{ omci_env_g.mib_rare_meid_start=util_atoi(str); }
static void cli_env_set_mib_rare2_meid_start(int fd, char *str)		{ omci_env_g.mib_rare2_meid_start=util_atoi(str); }
static void cli_env_set_mib_nonexist_del_err(int fd, char *str)	{ omci_env_g.mib_nonexist_del_err=util_atoi(str); }

static void cli_env_set_autouni_enable(int fd, char *str)	{ 
	omci_env_g.autouni_enable=util_atoi(str); 
	batchtab_omci_update("autouni");
}
static void cli_env_set_autoveip_enable(int fd, char *str)	{ 
	omci_env_g.autoveip_enable=util_atoi(str); 
	batchtab_omci_update("autoveip");
}
static void cli_env_set_erdiag_mon_enable(int fd, char *str)	{ omci_env_g.erdiag_mon_enable=util_atoi(str); }
static void cli_env_set_mibreset_dump_enable(int fd, char *str)	{ omci_env_g.mibreset_dump_enable=util_atoi(str); }
static void cli_env_set_mib_uni_adminlock_default(int fd, char *str)	{ omci_env_g.mib_uni_adminlock_default=util_atoi(str); }
static void cli_env_set_adminlock_trigger_opstate(int fd, char *str)	{ omci_env_g.adminlock_trigger_opstate=util_atoi(str); }
static void cli_env_set_untag_reserved_vid (int fd, char *str)	{ omci_env_g.untag_reserved_vid=util_atoi(str); }

static void cli_env_set_port_forward_operation_default(int fd, char *str)	{ omci_env_g.port_forward_operation_default=util_atoi(str); }

static void cli_env_set_pseudo_ts_pointer_enable(int fd, char *str)	{ omci_env_g.pseudo_ts_pointer_enable=util_atoi(str); }
static void cli_env_set_invert_weight_enable(int fd, char *str)		{ omci_env_g.invert_weight_enable=util_atoi(str); }
static void cli_env_set_invert_priority_enable(int fd, char *str)	{ omci_env_g.invert_priority_enable=util_atoi(str); }
static void cli_env_set_pq_tcont_relation(int fd, char *str)	{ omci_env_g.pq_tcont_relation=util_atoi(str); }

static void cli_env_set_tcont_map_enable(int fd, char *str)	{ omci_env_g.tcont_map_enable=util_atoi(str); }
static void cli_env_set_tcont_map_policy(int fd, char *str)	{ env_set_tcont_map_policy(str); }
static void cli_env_set_tcont_map_ts(int fd, char *str)
{
	conv_numlist2array(str, omci_env_g.tcont_map_ts, sizeof(char), ENV_SIZE_TCONT_MAP);
}
static void cli_env_set_tcont_map_pq(int fd, char *str)
{
	conv_numlist2array(str, omci_env_g.tcont_map_pq, sizeof(char), ENV_SIZE_TCONT_MAP);
}

// sw download
static void cli_env_set_sw_download_end_behavior(int fd, char *str)	{omci_env_g.sw_download_end_behavior=util_atoi(str); }
static void cli_env_set_sw_download_active_always_ok(int fd, char *str)	{omci_env_g.sw_download_active_always_ok=util_atoi(str); }
static void cli_env_set_sw_download_commit_always_ok(int fd, char *str)	{omci_env_g.sw_download_commit_always_ok=util_atoi(str); }
static void cli_env_set_sw_download_window_size(int fd, char *str)	{omci_env_g.sw_download_window_size=util_atoi(str); }
static void cli_env_set_sw_download_script(int fd, char *str)	{strncpy(omci_env_g.sw_download_script,str, ENV_SIZE_PATH); }
static void cli_env_set_sw_download_image_name(int fd, char *str)	{ strncpy(omci_env_g.sw_download_image_name,str, ENV_SIZE_PATH); }

// thread tasks
static void cli_env_set_task_pause_mask(int fd, char *str)	{ omci_env_g.task_pause_mask=util_atoi(str); }
static void cli_env_set_avc_refresh_interval(int fd, char *str)	{ omci_env_g.avc_refresh_interval=util_atoi(str); }
static void cli_env_set_alarm_refresh_interval(int fd, char *str)	{ omci_env_g.alarm_refresh_interval=util_atoi(str); }
static void cli_env_set_pm_refresh_interval(int fd, char *str)	{ omci_env_g.pm_refresh_interval=util_atoi(str); }
static void cli_env_set_pm_swap_interval(int fd, char *str)	{ omci_env_g.pm_swap_interval=util_atoi(str); }

static void cli_env_set_cpuport_history_enable(int fd, char *str)	{  omci_env_g.cpuport_history_enable=util_atoi(str); }
static void cli_env_set_cpuport_history_size(int fd, char *str)	
{
	omci_env_g.cpuport_history_size=util_atoi(str); 
	cpuport_history_clear(omci_env_g.cpuport_history_size);
}
static void cli_env_set_cpuport_arp_enable(int fd, char *str)	{ omci_env_g.cpuport_arp_enable=util_atoi(str); }
static void cli_env_set_cpuport_dhcp_enable(int fd, char *str)	{ omci_env_g.cpuport_dhcp_enable=util_atoi(str); }
static void cli_env_set_cpuport_dhcp_cust_tag(int fd, char *str)	{ memset(omci_env_g.cpuport_dhcp_cust_tag, 0, sizeof(omci_env_g.cpuport_dhcp_cust_tag)); conv_numlist2array(str, omci_env_g.cpuport_dhcp_cust_tag, sizeof(char), ENV_SIZE_STRING); }
static void cli_env_set_cpuport_dhcp_server_onlan(int fd, char *str)	{ omci_env_g.cpuport_dhcp_server_onlan=util_atoi(str); }
static void cli_env_set_cpuport_pppoe_enable(int fd, char *str)	{ omci_env_g.cpuport_pppoe_enable=util_atoi(str); }
static void cli_env_set_cpuport_pppoe_cust_tag(int fd, char *str)	{ memset(omci_env_g.cpuport_pppoe_cust_tag, 0, sizeof(omci_env_g.cpuport_pppoe_cust_tag)); conv_numlist2array(str, omci_env_g.cpuport_pppoe_cust_tag, sizeof(char), ENV_SIZE_STRING); }
static void cli_env_set_cpuport_pkt_ssdp_fwd_wan(int fd, char *str)	{ omci_env_g.cpuport_pkt_ssdp_fwd_wan=util_atoi(str); }
static void cli_env_set_cpuport_im_enable(int fd, char *str)	{ omci_env_g.cpuport_im_enable=util_atoi(str); }
static void cli_env_set_cpuport_im_lan2lan_enable(int fd, char *str)	{ omci_env_g.cpuport_im_lan2lan_enable=util_atoi(str); }
static void cli_env_set_cpuport_wifi_block_mask(int fd, char *str)	{ omci_env_g.cpuport_wifi_block_mask=util_atoi(str); }
static void cli_env_set_cpuport_wifi_block_ra(int fd, char *str)	{ omci_env_g.cpuport_wifi_block_ra=util_atoi(str); }
static void cli_env_set_cpuport_mcast_drop(int fd, char *str)	{ omci_env_g.cpuport_mcast_drop=util_atoi(str); }
static void cli_env_set_stp_enable(int fd, char *str)	{ omci_env_g.stp_enable=util_atoi(str); }

static void cli_env_set_cpu_usage_threshold(int fd, char *str)	{ omci_env_g.cpu_usage_threshold=util_atoi(str); }
static void cli_env_set_cpul_task_no(int fd, char *str)		{ conv_numlist2array(str, omci_env_g.cpul_task_no, sizeof(char), ENV_SIZE_TASK); }
static void cli_env_set_cpul_total_usage(int fd, char *str)	{ omci_env_g.cpul_total_usage=util_atoi(str); }

// debug
static void cli_env_set_debug_level(int fd, char *str)		{ omci_env_g.debug_level=util_atoi(str); }
static void cli_env_set_debug_level_bat(int fd, char *str)	{ omci_env_g.debug_level_bat=util_atoi(str); }
static void cli_env_set_debug_level_cfm(int fd, char *str)	{ omci_env_g.debug_level_cfm=util_atoi(str); }
static void cli_env_set_debug_level_cpuport(int fd, char *str)	{ omci_env_g.debug_level_cpuport=util_atoi(str); }
static void cli_env_set_debug_level_fwk(int fd, char *str)	{ omci_env_g.debug_level_fwk=util_atoi(str); }
static void cli_env_set_debug_level_igmp(int fd, char *str)	{ omci_env_g.debug_level_igmp=util_atoi(str); }
static void cli_env_set_debug_level_lldp(int fd, char *str)	{ omci_env_g.debug_level_lldp=util_atoi(str); }
static void cli_env_set_debug_level_vacl(int fd, char *str)	{ omci_env_g.debug_level_vacl=util_atoi(str); }
static void cli_env_set_debug_level_voip(int fd, char *str)	{ omci_env_g.debug_level_voip=util_atoi(str); }
static void cli_env_set_debug_level_xml(int fd, char *str)	{ omci_env_g.debug_level_xml=util_atoi(str); }
static void cli_env_set_debug_log_time(int fd, char *str)	{ omci_env_g.debug_log_time=util_atoi(str); }
static void cli_env_set_debug_log_type(int fd, char *str)	{ omci_env_g.debug_log_type=util_atoi(str); }
static void cli_env_set_debug_log_file(int fd, char *str)	{ strncpy(omci_env_g.debug_log_file, str, ENV_SIZE_PATH); }

static void cli_env_set_memdbg_enable(int fd, char *str)	{ omci_env_g.memdbg_enable=util_atoi(str); }
static void cli_env_set_er_group_hw_debug_enable(int fd, char *str)	{ omci_env_g.er_group_hw_debug_enable=util_atoi(str); }
static void cli_env_set_er_group_hw_time_enable(int fd, char *str)	{ omci_env_g.er_group_hw_time_enable=util_atoi(str); }

// olt
static void cli_env_set_olt_vendor_check_enable(int fd, char *str)	{ omci_env_g.olt_vendor_check_enable=util_atoi(str); }
static void cli_env_set_olt_vendor_config_change(int fd, char *str)	{ omci_env_g.olt_vendor_config_change=util_atoi(str); }
static void cli_env_set_olt_workaround_enable(int fd, char *str)	{ omci_env_g.olt_workaround_enable=util_atoi(str); }
static void cli_env_set_olt_alu_mcast_support(int fd, char *str)	{ omci_env_g.olt_alu_mcast_support=util_atoi(str); }
static void cli_env_set_olt_huawei_ignore_vlan_drop_rule(int fd, char *str)	{ omci_env_g.olt_huawei_ignore_vlan_drop_rule=util_atoi(str); }
static void cli_env_set_olt_proprietary_support(int fd, char *str)	{ strncpy(omci_env_g.olt_proprietary_support, str, ENV_SIZE_PROPRIETARY_VENDOR); }
static void cli_env_set_ont_force_vendor_id(int fd, char *str)	{ strncpy(omci_env_g.ont_force_vendor_id, str, 5); }

// anig
static void cli_env_set_anig_type(int fd, char *str)	{ omci_env_g.anig_type=util_atoi(str); }
static void cli_env_set_anig_support(int fd, char *str)	{ omci_env_g.anig_support=util_atoi(str); }
static void cli_env_set_anig_txrx_threshold(int fd, char *str)
{
	conv_numlist2array(str, omci_env_g.anig_txrx_threshold, sizeof(char), 4);
}
static void cli_env_set_anig_devport(int fd, char *str)		{ omci_env_g.anig_devport=util_atoi(str); }
static void cli_env_set_anig_devaddr0(int fd, char *str)	{ omci_env_g.anig_devaddr0=util_atoi(str); }
static void cli_env_set_anig_mon_mode(int fd, char *str)	{ omci_env_g.anig_mon_mode=util_atoi(str); }

// logical port id/mask
static void cli_env_set_wan_logical_portid(int fd, char *str)	{ omci_env_g.wan_logical_portid=util_atoi(str); }
static void cli_env_set_uni_logical_portmask(int fd, char *str)	{ omci_env_g.uni_logical_portmask=util_atoi(str); }
static void cli_env_set_cpu_logical_portmask(int fd, char *str)	{ omci_env_g.cpu_logical_portmask=util_atoi(str); }
static void cli_env_set_wifi_logical_portmask(int fd, char *str){ omci_env_g.wifi_logical_portmask=util_atoi(str); }

// switch
static void 
cli_env_set_ivlan_enable(int fd, char *str)	{ 
	omci_env_g.ivlan_enable=util_atoi(str); 
	if ( switch_hw_g.unknown_mcast_drop_set == NULL ){
		util_fdprintf(fd, "switch_hw_g.unknown_mcast_drop_set == NULL\n");
	}else{
		// FIXME, KEVIN
		// why ivlan related to unknown_mcast_drop?
		if (omci_env_g.ivlan_enable == 0 ){
			switch_hw_g.unknown_mcast_drop_set(SWITCH_DISABLED);
		}
		if (omci_env_g.ivlan_enable == 1 ){
			switch_hw_g.unknown_mcast_drop_set(SWITCH_ENABLED);
		}
	}
}
static void cli_env_set_stp_conf_before_omci(int fd, char *str)	{ omci_env_g.stp_conf_before_omci=util_atoi(str); }
static void cli_env_set_switch_history_mode(int fd, char *str) 		{ omci_env_g.switch_history_mode=util_atoi(str); }
static void cli_env_set_switch_history_interval(int fd, char *str) 
{ 
	omci_env_g.switch_history_interval=util_atoi(str); 
	if (omci_env_g.switch_history_interval<1) 
		omci_env_g.switch_history_interval = 1; 
	coretask_cmd(CORETASK_CMD_SWITCH_HISTORY_INTERVAL_CHANGE); 
}

static void cli_env_set_br_allowed_srcmask(int fd, char *str)	{ 
	omci_env_g.br_allowed_srcmask=util_atoi(str); 
	batchtab_omci_update("isolation_set");
}
static void cli_env_set_rt_allowed_srcmask(int fd, char *str)	{ 
	omci_env_g.rt_allowed_srcmask=util_atoi(str); 
	batchtab_omci_update("isolation_set");
}
static void cli_env_set_trunk_group_portmask(int fd, char *str)	{ 
	omci_env_g.trunk_group_portmask=util_atoi(str); 
	if (switch_hw_g.trunk_portmask_config)
		switch_hw_g.trunk_portmask_config(omci_env_g.trunk_group_portmask);
}

static void cli_env_set_localbridging_default(int fd, char *str)
{ 
	omci_env_g.localbridging_default=util_atoi(str); 
	if (switch_hw_g.local_bridging(omci_env_g.localbridging_default) != 0)
		dbprintf(LOG_ERR, "switch hw local_bridging err?\n");
}
static void cli_env_set_localbridging_addon_portmask(int fd, char *str)	{ omci_env_g.localbridging_addon_portmask=util_atoi(str); }
static void cli_env_set_ctc_internet_bridge_enabled_srcmask(int fd, char *str)	{ 
	omci_env_g.ctc_internet_bridge_enabled_srcmask=util_atoi(str); 
	batchtab_omci_update("isolation_set");
}

static void cli_env_set_port2port_enable(int fd, char *str) 		{ omci_env_g.port2port_enable=util_atoi(str); }
static void cli_env_set_maclearn_drop_enable(int fd, char *str)	{ omci_env_g.maclearn_drop_enable=util_atoi(str); }
static void cli_env_set_veip_for_wifi_enable(int fd, char *str)	{ omci_env_g.veip_for_wifi_enable=util_atoi(str); }
static void cli_env_set_cpuport_ds_passthrough (int fd, char *str)	{ omci_env_g.cpuport_ds_passthrough =util_atoi(str); }
static void cli_env_set_cpuport_pvlan_clone_ipv4_to_arp (int fd, char *str)	{ omci_env_g.cpuport_pvlan_clone_ipv4_to_arp =util_atoi(str); }
static void cli_env_set_rma_forward_mask (int fd, char *str)	{
	int i = 0;
	omci_env_g.rma_forward_mask =util_atoull(str);
	for(i=0; i<64; i++) {
		if(switch_hw_g.rma_action_set != NULL)
			switch_hw_g.rma_action_set(i, (1ULL<<i & omci_env_g.rma_forward_mask) ? 0 : 1); // 0: forward, 1: drop
	}
}
static void cli_env_set_bundle_rate_support(int fd, char *str)	{ omci_env_g.bundle_rate_support=util_atoi(str); }
static void cli_env_set_linkdown_debounce_time(int fd, char *str)	{ omci_env_g.linkdown_debounce_time=util_atoi(str); }
static void cli_env_set_linkup_debounce_time(int fd, char *str)	{ omci_env_g.linkup_debounce_time=util_atoi(str); }

// rldp
static void cli_env_set_rldp_enable(int fd, char *str)	{ omci_env_g.rldp_enable=util_atoi(str); }
static void cli_env_set_rldp_def_portmask(int fd, char *str)	{ omci_env_g.rldp_def_portmask=util_atoi(str); }
static void cli_env_set_rldp_passive_only(int fd, char *str)	{ omci_env_g.rldp_passive_only=util_atoi(str); }

// tag
static void cli_env_set_tag_treatment_correct(int fd, char *str)	{ omci_env_g.tag_treatment_correct=util_atoi(str); }
static void cli_env_set_tag_discard_reversing(int fd, char *str)	{ omci_env_g.tag_discard_reversing=util_atoi(str); }
static void cli_env_set_tag_preview_table(int fd, char *str)	{ omci_env_g.tag_preview_table=util_atoi(str); }
static void cli_env_set_tag_auto_reverse(int fd, char *str)	{ omci_env_g.tag_auto_reverse=util_atoi(str); }
static void cli_env_set_tag_mc_ds_from_us(int fd, char *str)	{ omci_env_g.tag_mc_ds_from_us=util_atoi(str); }
static void cli_env_set_tag_mc_sort_priority(int fd, char *str)	{ omci_env_g.tag_mc_sort_priority=util_atoi(str); }
static void cli_env_set_tag_pbit_workaround(int fd, char *str)	{ omci_env_g.tag_pbit_workaround=util_atoi(str); }
static void cli_env_set_tag_pbit_portmask(int fd, char *str)	{ omci_env_g.tag_pbit_portmask=util_atoi(str); }
static void cli_env_set_tag_pbit_remap_table(int fd, char *str)
{
	conv_numlist2array(str, omci_env_g.tag_pbit_remap_table, sizeof(char), 8);
}
static void cli_env_set_tag_ds_tci_dont_reverse_workaround(int fd, char *str)	{ omci_env_g.tag_ds_tci_dont_reverse_workaround=util_atoi(str); }
static void cli_env_set_tag_insert_65305_rules(int fd, char *str)	{ omci_env_g.tag_insert_65305_rules=util_atoi(str); }
static void cli_env_set_tag_ctc_translation_path(int fd, char *str)	{ strncpy(omci_env_g.tag_ctc_translation_path, str, ENV_SIZE_PATH); }
static void cli_env_set_tag_mc_ds_wildcard(int fd, char *str)	{ omci_env_g.tag_mc_ds_wildcard=util_atoi(str); }
static void cli_env_set_tag_l2_rules_reverse(int fd, char *str)	{ omci_env_g.tag_l2_rules_reverse=util_atoi(str); }
static void cli_env_set_tag_l2_rules_ds_mac(int fd, char *str)	{ omci_env_g.tag_l2_rules_ds_mac=util_atoi(str); }
static void cli_env_set_tag_iphost_vid_map(int fd, char *str)
{
	conv_numlist2array(str, omci_env_g.tag_iphost_vid_map, sizeof(unsigned short), ENV_SIZE_IPHOST_PORT);
}

// classf
static void cli_env_set_classf_tag_mode(int fd, char *str)	{ omci_env_g.classf_tag_mode=util_atoi(str); }
static void cli_env_set_classf_add_default_rules(int fd, char *str)	{ omci_env_g.classf_add_default_rules=util_atoi(str); }
static void cli_env_set_classf_force_stpid(int fd, char *str)	{ omci_env_g.classf_force_stpid=util_atoi(str); }
static void cli_env_set_classf_stag_tpid(int fd, char *str)	{ omci_env_g.classf_stag_tpid=util_atoi(str); }
static void cli_env_set_classf_hw_stag_tpid(int fd, char *str)	{ omci_env_g.classf_hw_stag_tpid=util_atoi(str); }
static void cli_env_set_classf_et_combine(int fd, char *str)	{ omci_env_g.classf_et_combine=util_atoi(str); }
static void cli_env_set_classf_ds_gem_dg(int fd, char *str)	{ omci_env_g.classf_ds_gem_dg=util_atoi(str); }
static void cli_env_set_classf_dscp2pbit_mode(int fd, char *str)	{ omci_env_g.classf_dscp2pbit_mode=util_atoi(str); }
#if 0 // Remove VEIP related workaround
static void cli_env_set_classf_veip_orig_tci_map(int fd, char *str)
{
	conv_numlist2array(str, omci_env_g.classf_veip_orig_tci_map, sizeof(unsigned short), ENV_SIZE_VEIP_PORT);
}
static void cli_env_set_classf_veip_real_tci_map(int fd, char *str)
{
	conv_numlist2array(str, omci_env_g.classf_veip_real_tci_map, sizeof(unsigned short), ENV_SIZE_VEIP_PORT);
}
static void cli_env_set_classf_veip_real_tag_rule_priority(int fd, char *str)	{ omci_env_g.classf_veip_real_tag_rule_priority=util_atoi(str); }
#endif
static void cli_env_set_classf_ds_unmatch(int fd, char *str)	{ omci_env_g.classf_ds_unmatch=util_atoi(str); }
static void cli_env_set_classf_ds_mc_extend(int fd, char *str)	{ omci_env_g.classf_ds_mc_extend=util_atoi(str); }
static void cli_env_set_classf_ds_mc_force_strip(int fd, char *str)	{ omci_env_g.classf_ds_mc_force_strip=util_atoi(str); }
static void cli_env_set_classf_veip_rules_reassign(int fd, char *str)	{ omci_env_g.classf_veip_rules_reassign=util_atoi(str); }
static void cli_env_set_classf_dropping_rules_add(int fd, char *str)	{ omci_env_g.classf_dropping_rules_add=util_atoi(str); }
static void cli_env_set_classf_vmap_pbit_workaround(int fd, char *str)	{ omci_env_g.classf_vmap_pbit_workaround=util_atoi(str); }
static void cli_env_set_classf_hw_2tags_mode(int fd, char *str)	{ omci_env_g.classf_hw_2tags_mode=util_atoi(str); }
static void cli_env_set_classf_hw_2tags_workaround(int fd, char *str)	{ omci_env_g.classf_hw_2tags_workaround=util_atoi(str); }
static void cli_env_set_classf_add_ipv4mc_strip(int fd, char *str)		{ omci_env_g.classf_add_ipv4mc_strip=util_atoi(str); }
static void cli_env_set_classf_add_hwnat_1tag_rules(int fd, char *str)	{ omci_env_g.classf_add_hwnat_1tag_rules=util_atoi(str); }
static void cli_env_set_classf_add_hwnat_untag_rules(int fd, char *str)	{ omci_env_g.classf_add_hwnat_untag_rules=util_atoi(str); }
static void cli_env_set_classf_add_hwnat_untag_vid(int fd, char *str)	{ omci_env_g.classf_add_hwnat_untag_vid=util_atoi(str); }
static void cli_env_set_classf_add_hwnat_untag_vid_mask(int fd, char *str)	{ omci_env_g.classf_add_hwnat_untag_vid_mask=util_atoi(str); }
static void cli_env_set_classf_hw_rules_migration(int fd, char *str)	{ omci_env_g.classf_hw_rules_migration=util_atoi(str); }
static void cli_env_set_classf_hw_colour_force(int fd, char *str)	{ omci_env_g.classf_hw_colour_force=util_atoi(str); }
static void cli_env_set_classf_keep_candi(int fd, char *str)	{ omci_env_g.classf_keep_candi=util_atoi(str); }
static void cli_env_set_classf_statistic_enable(int fd, char *str)	{ omci_env_g.classf_statistic_enable=util_atoi(str); }
static void cli_env_set_classf_tlan_pattern(int fd, char *str)	{ omci_env_g.classf_tlan_pattern=util_atoi(str); }
static void cli_env_set_classf_hw_intf_lan_vid(int fd, char *str)	{ omci_env_g.classf_hw_intf_lan_vid=util_atoi(str); }
static void cli_env_set_mc_ds_untag_discard_enable(int fd, char *str)	{ omci_env_g.mc_ds_untag_discard_enable=util_atoi(str); }
static void cli_env_set_classf_add_default_rules_by_me_84(int fd, char *str)	{ omci_env_g.classf_add_default_rules_by_me_84=util_atoi(str); }


// cfm
static void cli_env_set_cfm_enable(int fd, char *str)	{ omci_env_g.cfm_enable=util_atoi(str); }
static void cli_env_set_cfm_y1731_enable(int fd, char *str)	{ omci_env_g.cfm_y1731_enable=util_atoi(str); }
static void cli_env_set_cfm_portmask(int fd, char *str)	{ omci_env_g.cfm_portmask=util_atoi(str); }
static void cli_env_set_cfm_mac_ifname(int fd, char *str)		{ strncpy(omci_env_g.cfm_mac_ifname, str, ENV_SIZE_IFNAME); }
static void cli_env_set_cfm_send_timeout(int fd, char *str)	{ omci_env_g.cfm_send_timeout=util_atoi(str); }
static void cli_env_set_cfm_reverse_mep_direction(int fd, char *str)	{ omci_env_g.cfm_reverse_mep_direction=util_atoi(str); }
static void cli_env_set_cfm_rmep_ccm_before_hold(int fd, char *str)	{ omci_env_g.cfm_rmep_ccm_before_hold=util_atoi(str); }
static void cli_env_set_cfm_rmep_holdtime_before_active(int fd, char *str)	{ omci_env_g.cfm_rmep_holdtime_before_active=util_atoi(str); }
static void cli_env_set_cfm_rmep_ccm_lost_threshold(int fd, char *str)	{ omci_env_g.cfm_rmep_ccm_lost_threshold=util_atoi(str); }
static void cli_env_set_cfm_defect_mask_in_rdi_gen(int fd, char *str)	{ omci_env_g.cfm_defect_mask_in_rdi_gen=util_atoi(str); }
static void cli_env_set_cfm_defect_mask_in_ais_gen(int fd, char *str)	{ omci_env_g.cfm_defect_mask_in_ais_gen=util_atoi(str); }
static void cli_env_set_cfm_defect_mask_supressed_by_ais(int fd, char *str)	{ omci_env_g.cfm_defect_mask_supressed_by_ais=util_atoi(str); }
static void cli_env_set_cfm_defect_mask_supressed_by_lck(int fd, char *str)	{ omci_env_g.cfm_defect_mask_supressed_by_lck=util_atoi(str); }

// lldp
static void cli_env_set_lldp_enable(int fd, char *str)	{ omci_env_g.lldp_enable=util_atoi(str); }
static void cli_env_set_lldp_ifname(int fd, char *str)		{ strncpy(omci_env_g.lldp_ifname, str, ENV_SIZE_IFNAME); }
static void cli_env_set_lldp_dst_mac(int fd, char *str)	{ omci_env_g.lldp_dst_mac=util_atoi(str); }
static void cli_env_set_lldp_send_interval(int fd, char *str)	{ omci_env_g.lldp_send_interval=util_atoi(str); }
static void cli_env_set_lldp_fast_tx(int fd, char *str)	{ omci_env_g.lldp_fast_tx=util_atoi(str); }
static void cli_env_set_lldp_credit_max(int fd, char *str)	{ omci_env_g.lldp_credit_max=util_atoi(str); }
static void cli_env_set_lldp_tx_fast_init(int fd, char *str)	{ omci_env_g.lldp_tx_fast_init=util_atoi(str); }
static void cli_env_set_lldp_system_name(int fd, char *str)		{ strncpy(omci_env_g.lldp_system_name, str, ENV_SIZE_STRING); }
static void cli_env_set_lldp_system_desc(int fd, char *str)		{ strncpy(omci_env_g.lldp_system_desc, str, ENV_SIZE_STRING); }
static void cli_env_set_lldp_med_opt(int fd, char *str)	{ omci_env_g.lldp_med_opt=util_atoi(str); }
static void cli_env_set_lldp_med_cap(int fd, char *str)	{ omci_env_g.lldp_med_cap=util_atoi(str); }

// tm
static void cli_env_set_tm_pq_gem_qos_method(int fd, char *str)	{ omci_env_g.tm_pq_gem_qos_method=util_atoi(str); }
static void cli_env_set_tm_gemus_rate_factor(int fd, char *str)
{ 
	omci_env_g.tm_gemus_rate_factor=util_atoi(str); 
	batchtab_omci_update("td");
}
static void cli_env_set_tm_gemds_rate_factor(int fd, char *str)
{ 
	omci_env_g.tm_gemds_rate_factor=util_atoi(str); 
	batchtab_omci_update("td");
}
static void cli_env_set_tm_unius_rate_factor(int fd, char *str)
{ 
	omci_env_g.tm_unius_rate_factor=util_atoi(str); 
	batchtab_omci_update("td");
}
static void cli_env_set_tm_unids_rate_factor(int fd, char *str)
{ 
	omci_env_g.tm_unids_rate_factor=util_atoi(str); 
	batchtab_omci_update("td");
}
static void cli_env_set_tm_pir_zero_action(int fd, char *str)
{ 
	omci_env_g.tm_pir_zero_action=util_atoi(str); 
	batchtab_omci_update("td");
}
static void cli_env_set_tm_protect_enable(int fd, char *str)	{ omci_env_g.tm_protect_enable=util_atoi(str); }

// igmp
static void cli_env_set_igmp_enable(int fd, char *str)
{
	omci_env_g.igmp_enable=util_atoi(str);
}
static void cli_env_set_igmp_version_overwrite(int fd, char *str)
{
	omci_env_g.igmp_version_overwrite=util_atoi(str);
}
static void cli_env_set_igmp_join_default_grant(int fd, char *str)
{
	omci_env_g.igmp_join_default_grant=util_atoi(str);
}
static void cli_env_set_igmp_upstream_tpid(int fd, char *str) 
{
	omci_env_g.igmp_upstream_tpid=util_atoi(str);
}
static void cli_env_set_igmp_downstream_tpid(int fd, char *str) 
{
	omci_env_g.igmp_downstream_tpid=util_atoi(str);
}
static void cli_env_set_igmp_mcast_me_support(int fd, char *str)
{
	omci_env_g.igmp_mcast_me_support=util_atoi(str);
}
static void cli_env_set_igmp_query_pass_through(int fd, char *str)
{
	omci_env_g.igmp_query_pass_through=util_atoi(str);
}
static void cli_env_set_igmp_proxy_snooping (int fd, char *str)	
{ 
	int old = omci_env_g.igmp_proxy_snooping;

	omci_env_g.igmp_proxy_snooping =util_atoi(str); 
	if (omci_env_g.igmp_proxy_snooping)
	{
		igmp_router_mode_deinit();
		if (igmp_router_mode_init() < 0)
		{
			omci_env_g.igmp_proxy_snooping = old;
			return;
		}		
		
		if (omci_env_g.igmp_proxy_snooping == 1)
		{
			if (switch_hw_g.unknown_mcast_drop_set(SWITCH_DISABLED) < 0)
			{
				dbprintf(LOG_ERR,"Fail to set unknown mcast drop\n");
			}
			if (igmp_switch_igmp_extract_set(SWITCH_ENABLED) < 0)
			{
				dbprintf(LOG_ERR,"igmp extract error\n");
			}
		}
		else
		{
			if (switch_hw_g.unknown_mcast_drop_set(SWITCH_ENABLED) < 0)
			{
				dbprintf(LOG_ERR,"Fail to set unknown mcast drop\n");
			}
			if (igmp_switch_igmp_extract_set(SWITCH_ENABLED) < 0)
			{
				dbprintf(LOG_ERR,"igmp extract error\n");
			}
		}
		
                if (igmp_config_g.igmp_version & IGMP_V3_BIT)					
                        sendigmpv3generalquery();
                else
                        sendgeneralmembershipquery();
		cpuport_frame_recv_abort_select();			
	}
}
static void cli_env_set_igmp_v1_ignore(int fd, char *str)	{ omci_env_g.igmp_v1_ignore=util_atoi(str); }
static void cli_env_set_igmp_preassign_list(int fd, char *str)	{ env_set_igmp_preassign_list(str); }
static void cli_env_set_igmp_mcast_mode_src(int fd, char *str)	{ omci_env_g.igmp_mcast_mode_src=util_atoi(str); }


// extoam
static void cli_env_set_extoam_enable(int fd, char *str)		{ omci_env_g.extoam_enable=util_atoi(str); }
static void cli_env_set_extoam_port(int fd, char *str)		
{ 
	omci_env_g.extoam_port=util_atoi(str); 
	cpe_link_status_g.dpu_port = omci_env_g.extoam_port;
}
static void cli_env_set_extoam_mac_ifname(int fd, char *str)		{ strncpy(omci_env_g.extoam_mac_ifname, str, ENV_SIZE_IFNAME); }
static void cli_env_set_extoam_itas_image_name(int fd, char *str)		{ strncpy(omci_env_g.extoam_itas_image_name, str, 256); }

// voip
static void cli_env_set_voip_enable(int fd, char *str)	{ omci_env_g.voip_enable=util_atoi(str); }
static void cli_env_set_voip_debug_set_flag(int fd, char *str)	{ omci_env_g.voip_debug_set_flag=util_atoi(str); }
static void cli_env_set_voip_debug_get_flag(int fd, char *str)	{ omci_env_g.voip_debug_get_flag=util_atoi(str); }
static void cli_env_set_voip_debug_release_flag(int fd, char *str)	{ omci_env_g.voip_debug_release_flag=util_atoi(str); }

// misc
static void cli_env_set_batchtab_extevent_idletime(int fd, char *str)	{ omci_env_g.batchtab_extevent_idletime=util_atoi(str); }
static void cli_env_set_batchtab_retry_timeout(int fd, char *str)	{ omci_env_g.batchtab_retry_timeout=util_atoi(str); }
static void cli_env_set_batchtab_wanif_feature(int fd, char *str)	{ omci_env_g.batchtab_wanif_feature=util_atoi(str); }
static void cli_env_set_batchtab_hardbridge_enable(int fd, char *str)	{ omci_env_g.batchtab_hardbridge_enable=util_atoi(str); }
static void cli_env_set_exclude_flowid_0(int fd, char *str) 		{ omci_env_g.exclude_flowid_0=util_atoi(str); }
static void cli_env_set_avoid_pir_drop(int fd, char *str) 		{ omci_env_g.avoid_pir_drop=util_atoi(str); }
static void cli_env_set_uni_policing_pbit_mask(int fd, char *str)	{ omci_env_g.uni_policing_pbit_mask=util_atoi(str); }
static void cli_env_set_uni_policing_default_bs(int fd, char *str)	{ omci_env_g.uni_policing_default_bs=util_atoi(str); }
static void cli_env_set_uni_policing_aggregate_acl(int fd, char *str)	{ omci_env_g.uni_policing_aggregate_acl=util_atoi(str); }
static void cli_env_set_uni_linkdown_lock(int fd, char *str)	{ omci_env_g.uni_linkdown_lock=util_atoi(str); }
static void cli_env_set_watchdog_omccmiss_timeout(int fd, char *str)	{ 
	conv_numlist2array(str, omci_env_g.watchdog_omccmiss_timeout, sizeof(unsigned short), 2);
}
static void cli_env_set_watchdog_tcontmiss_timeout(int fd, char *str)	{
	conv_numlist2array(str, omci_env_g.watchdog_tcontmiss_timeout, sizeof(unsigned short), 2);
}
static void cli_env_set_watchdog_o2stuck_timeout(int fd, char *str)	{
	conv_numlist2array(str, omci_env_g.watchdog_o2stuck_timeout, sizeof(unsigned short), 2);
}
static void cli_env_set_watchdog_idlebw_guard(int fd, char *str)	{
	conv_numlist2array(str, omci_env_g.watchdog_idlebw_guard, sizeof(unsigned short), 4);
}

static void cli_env_set_uni_config_lock(int fd, char *str)    { omci_env_g.uni_config_lock=util_atoi(str); }

struct cli_env_map_entry_t {
	char *name;
	void (*get_funcptr)(int fd);
	void (*set_funcptr)(int fd, char *str);
	int writeable;
};

static struct cli_env_map_entry_t cli_env_map[]={
// global
	{"cli_ip",		cli_env_cli_ip, cli_env_set_cli_ip, 0},
	{"cli_port",		cli_env_cli_port, cli_env_set_cli_port, 0},
	{"transport",		cli_env_transport_type,	cli_env_set_transport_type,	0},
	{"etc_omci_path",	cli_env_etc_omci_path, cli_env_set_etc_omci_path, 0},
	{"etc_omci_path2",	cli_env_etc_omci_path2, cli_env_set_etc_omci_path2, 0},
	{"start_cmd_file",	cli_env_start_cmd_file, cli_env_set_start_cmd_file, 0},

// interface config
	{"trap_if",		cli_env_trap_if, cli_env_set_trap_if, 0},
	{"omcc_if",		cli_env_omcc_if, cli_env_set_omcc_if, 0},
	{"wifi_if",		cli_env_wifi_if, cli_env_set_wifi_if, 0},
	{"router_lan_if",cli_env_router_lan_if,cli_env_set_router_lan_if,1},
	{"router_wan_if",cli_env_router_wan_if,cli_env_set_router_wan_if,1},

// connecting olt simulator
	{"sim_olt_ip",		cli_env_sim_olt_ip, cli_env_set_sim_olt_ip, 0},
	{"sim_olt_udp_port",	cli_env_sim_olt_udp_port, cli_env_set_sim_olt_udp_port, 0},
	{"sim_pon_id",		cli_env_sim_pon_id, cli_env_set_sim_pon_id, 0},
	{"sim_onu_id",		cli_env_sim_onu_id,cli_env_set_sim_onu_id, 0},
	{"sim_tcont_enable",	cli_env_sim_tcont_enable,cli_env_set_sim_tcont_enable, 1},

// omci msg
	{"omcc_rx_crc_type",	cli_env_omcc_rx_crc_type, cli_env_set_omcc_rx_crc_type, 1},
	{"omcc_tx_crc_type",	cli_env_omcc_tx_crc_type, cli_env_set_omcc_tx_crc_type, 1},

	{"omci_tranx_cache_num",	cli_env_omci_tranx_cache_num, cli_env_set_omci_tranx_cache_num, 1},
	{"omci_tranx_match_mode",	cli_env_omci_tranx_match_mode, cli_env_set_omci_tranx_match_mode, 1},
	{"omci_tranx_bypass",		cli_env_omci_tranx_bypass, cli_env_set_omci_tranx_bypass, 1},
	{"omci_tranx_busy_enable",	cli_env_omci_tranx_busy_enable, cli_env_set_omci_tranx_busy_enable, 1},

	{"omci_exec_time_limitation",	cli_env_omci_exec_time_limitation, cli_env_set_omci_exec_time_limitation, 1},
	{"omci_tx_flag",	cli_env_omci_tx_flag, cli_env_set_omci_tx_flag, 1},
	{"omci_tx_resend_flag",	cli_env_omci_tx_resend_flag, cli_env_set_omci_tx_resend_flag, 0},

	{"omci_history_enable",	cli_env_omci_history_enable, cli_env_set_omci_history_enable, 1},
	{"omci_history_size",	cli_env_omci_history_size, cli_env_set_omci_history_size, 1},
	{"omci_history_msgmask",	cli_env_omci_history_msgmask, cli_env_set_omci_history_msgmask, 1},

	{"omcidump_enable",	cli_env_omcidump_enable, cli_env_set_omcidump_enable, 1},
	{"omcidump_msgmask",	cli_env_omcidump_msgmask, cli_env_set_omcidump_msgmask, 1},

	{"omccrx_log_enable",	cli_env_omccrx_log_enable, cli_env_set_omccrx_log_enable, 1},
	{"omccrx_log_msgmask",	cli_env_omccrx_log_msgmask, cli_env_set_omccrx_log_msgmask, 1},
	{"omccrx_log_file",	cli_env_omccrx_log_file, cli_env_set_omccrx_log_file, 1},

	{"restore_log_enable",	cli_env_restore_log_enable, cli_env_set_restore_log_enable, 1},
	{"restore_log_file",	cli_env_restore_log_file, cli_env_set_restore_log_file, 1},

// mib
	{"xml_section",		cli_env_xml_section, cli_env_set_xml_section, 0},
	{"xml_load_namestr",	cli_env_xml_load_namestr, cli_env_set_xml_load_namestr, 0},

	{"mib_upload_order",	cli_env_mib_upload_order, cli_env_set_mib_upload_order, 1},
	{"mib_boundary_check",	cli_env_mib_boundary_check, cli_env_set_mib_boundary_check, 1},
	{"mib_pointer_check",	cli_env_mib_pointer_check, cli_env_set_mib_pointer_check, 1},
	{"mib_readonly_check",	cli_env_mib_readonly_check, cli_env_set_mib_readonly_check, 1},
	{"mib_rw_mode",		cli_env_mib_rw_mode, cli_env_set_mib_rw_mode, 1},
	{"mib_aliasid_enable",	cli_env_mib_aliasid_enable, cli_env_set_mib_aliasid_enable, 1},
	{"mib_rare_meid_start",	cli_env_mib_rare_meid_start, cli_env_set_mib_rare_meid_start, 1},
	{"mib_rare2_meid_start",cli_env_mib_rare2_meid_start, cli_env_set_mib_rare2_meid_start, 1},
	{"mib_nonexist_del_err",cli_env_mib_nonexist_del_err, cli_env_set_mib_nonexist_del_err, 1},

	{"autouni_enable", cli_env_autouni_enable, cli_env_set_autouni_enable, 1},
	{"autoveip_enable", cli_env_autoveip_enable, cli_env_set_autoveip_enable, 1},
	{"erdiag_mon_enable",	cli_env_erdiag_mon_enable, cli_env_set_erdiag_mon_enable, 1},
	{"mibreset_dump_enable",cli_env_mibreset_dump_enable, cli_env_set_mibreset_dump_enable, 1},
	{"mib_uni_adminlock_default",cli_env_mib_uni_adminlock_default, cli_env_set_mib_uni_adminlock_default, 1},
	{"adminlock_trigger_opstate", cli_env_adminlock_trigger_opstate, cli_env_set_adminlock_trigger_opstate, 1},
	{"untag_reserved_vid",	cli_env_untag_reserved_vid , cli_env_set_untag_reserved_vid , 1},

	{"port_forward_operation_default", cli_env_port_forward_operation_default, cli_env_set_port_forward_operation_default, 1},

	{"pseudo_ts_pointer_enable", cli_env_pseudo_ts_pointer_enable, cli_env_set_pseudo_ts_pointer_enable, 1},
	{"invert_weight_enable", cli_env_invert_weight_enable, cli_env_set_invert_weight_enable, 1},
	{"invert_priority_enable", cli_env_invert_priority_enable, cli_env_set_invert_priority_enable, 1},
	{"pq_tcont_relation", cli_env_pq_tcont_relation, cli_env_set_pq_tcont_relation, 1},

	{"tcont_map_enable", cli_env_tcont_map_enable, cli_env_set_tcont_map_enable, 0},
	{"tcont_map_policy", cli_env_tcont_map_policy, cli_env_set_tcont_map_policy, 0},
	{"tcont_map_ts", cli_env_tcont_map_ts, cli_env_set_tcont_map_ts, 0},
	{"tcont_map_pq", cli_env_tcont_map_pq, cli_env_set_tcont_map_pq, 0},

// sw download
	{"sw_download_script",	cli_env_sw_download_script, cli_env_set_sw_download_script, 1},
	{"sw_download_image_name",	cli_env_sw_download_image_name, cli_env_set_sw_download_image_name, 1},
	{"sw_download_end_behavior",	cli_env_sw_download_end_behavior, cli_env_set_sw_download_end_behavior, 1},
	{"sw_download_active_always_ok",	cli_env_sw_download_active_always_ok, cli_env_set_sw_download_active_always_ok, 1},
	{"sw_download_commit_always_ok",	cli_env_sw_download_commit_always_ok, cli_env_set_sw_download_commit_always_ok, 1},
	{"sw_download_window_size",	cli_env_sw_download_window_size, cli_env_set_sw_download_window_size, 1},

// thread tasks
	{"task_pause_mask",cli_env_task_pause_mask, cli_env_set_task_pause_mask, 1},
	{"avc_refresh_interval",	cli_env_avc_refresh_interval, cli_env_set_avc_refresh_interval, 1},
	{"alarm_refresh_interval",	cli_env_alarm_refresh_interval, cli_env_set_alarm_refresh_interval, 1},
	{"pm_refresh_interval",	cli_env_pm_refresh_interval, cli_env_set_pm_refresh_interval, 1},
	{"pm_swap_interval",	cli_env_pm_swap_interval, cli_env_set_pm_swap_interval, 1},

	{"cpuport_history_enable", cli_env_cpuport_history_enable, cli_env_set_cpuport_history_enable, 1},
	{"cpuport_history_size", cli_env_cpuport_history_size, cli_env_set_cpuport_history_size, 1},
	{"cpuport_arp_enable", cli_env_cpuport_arp_enable, cli_env_set_cpuport_arp_enable, 0},
	{"cpuport_dhcp_enable", cli_env_cpuport_dhcp_enable, cli_env_set_cpuport_dhcp_enable, 0},
	{"cpuport_dhcp_cust_tag", cli_env_cpuport_dhcp_cust_tag, cli_env_set_cpuport_dhcp_cust_tag, 1},
	{"cpuport_dhcp_server_onlan", cli_env_cpuport_dhcp_server_onlan, cli_env_set_cpuport_dhcp_server_onlan, 0},
	{"cpuport_pppoe_enable", cli_env_cpuport_pppoe_enable, cli_env_set_cpuport_pppoe_enable, 0},
	{"cpuport_pppoe_cust_tag", cli_env_cpuport_pppoe_cust_tag, cli_env_set_cpuport_pppoe_cust_tag, 1},
	{"cpuport_pkt_ssdp_fwd_wan", cli_env_cpuport_pkt_ssdp_fwd_wan, cli_env_set_cpuport_pkt_ssdp_fwd_wan, 1},
	{"cpuport_im_enable", cli_env_cpuport_im_enable, cli_env_set_cpuport_im_enable, 1},
	{"cpuport_im_lan2lan_enable", cli_env_cpuport_im_lan2lan_enable, cli_env_set_cpuport_im_lan2lan_enable, 1},
	{"cpuport_wifi_block_mask", cli_env_cpuport_wifi_block_mask, cli_env_set_cpuport_wifi_block_mask, 1},
	{"cpuport_wifi_block_ra", cli_env_cpuport_wifi_block_ra, cli_env_set_cpuport_wifi_block_ra, 1},
	{"cpuport_mcast_drop", cli_env_cpuport_mcast_drop, cli_env_set_cpuport_mcast_drop, 1},
	{"stp_enable", cli_env_stp_enable, cli_env_set_stp_enable, 0},

	{"cpu_usage_threshold", cli_env_cpu_usage_threshold, cli_env_set_cpu_usage_threshold, 1},
	{"cpul_task_no",cli_env_cpul_task_no, cli_env_set_cpul_task_no, 1},
	{"cpul_total_usage",cli_env_cpul_total_usage, cli_env_set_cpul_total_usage, 1},

// debug
	{"debug_level",		cli_env_debug_level, cli_env_set_debug_level, 1},
	{"debug_level_bat",	cli_env_debug_level_bat, cli_env_set_debug_level_bat, 1},
	{"debug_level_cfm",	cli_env_debug_level_cfm, cli_env_set_debug_level_cfm, 1},
	{"debug_level_cpuport",	cli_env_debug_level_cpuport, cli_env_set_debug_level_cpuport, 1},
	{"debug_level_fwk",	cli_env_debug_level_fwk, cli_env_set_debug_level_fwk, 1},
	{"debug_level_igmp",	cli_env_debug_level_igmp, cli_env_set_debug_level_igmp, 1},
	{"debug_level_lldp",	cli_env_debug_level_lldp, cli_env_set_debug_level_lldp, 1},
	{"debug_level_vacl",	cli_env_debug_level_vacl, cli_env_set_debug_level_vacl, 1},
	{"debug_level_voip",	cli_env_debug_level_voip, cli_env_set_debug_level_voip, 1},
	{"debug_level_xml",	cli_env_debug_level_xml, cli_env_set_debug_level_xml, 1},
	{"debug_log_time",	cli_env_debug_log_time, cli_env_set_debug_log_time, 1},
	{"debug_log_type",	cli_env_debug_log_type, cli_env_set_debug_log_type, 1},
	{"debug_log_file",	cli_env_debug_log_file, cli_env_set_debug_log_file, 1},
	{"memdbg_enable",	cli_env_memdbg_enable, cli_env_set_memdbg_enable, 0},
	{"er_group_hw_debug_enable", cli_env_er_group_hw_debug_enable, cli_env_set_er_group_hw_debug_enable, 1},
	{"er_group_hw_time_enable", cli_env_er_group_hw_time_enable, cli_env_set_er_group_hw_time_enable, 1},

// olt
	{"olt_vendor_check_enable", cli_env_olt_vendor_check_enable, cli_env_set_olt_vendor_check_enable, 0},
	{"olt_vendor_config_change", cli_env_olt_vendor_config_change, cli_env_set_olt_vendor_config_change, 0},
	{"olt_workaround_enable", cli_env_olt_workaround_enable, cli_env_set_olt_workaround_enable, 0},
	{"olt_alu_mcast_support", cli_env_olt_alu_mcast_support, cli_env_set_olt_alu_mcast_support, 1},
	{"olt_huawei_ignore_vlan_drop_rule", cli_env_olt_huawei_ignore_vlan_drop_rule, cli_env_set_olt_huawei_ignore_vlan_drop_rule, 1},
	{"olt_proprietary_support", cli_env_olt_proprietary_support, cli_env_set_olt_proprietary_support, 0},
	{"ont_force_vendor_id", cli_env_ont_force_vendor_id, cli_env_set_ont_force_vendor_id, 1},

// anig
	{"anig_type", cli_env_anig_type, cli_env_set_anig_type, 1},
	{"anig_support", cli_env_anig_support, cli_env_set_anig_support, 1},
	{"anig_txrx_threshold", cli_env_anig_txrx_threshold, cli_env_set_anig_txrx_threshold, 1},
	{"anig_devport", cli_env_anig_devport, cli_env_set_anig_devport, 1},
	{"anig_devaddr0", cli_env_anig_devaddr0, cli_env_set_anig_devaddr0, 1},
	{"anig_mon_mode", cli_env_anig_mon_mode, cli_env_set_anig_mon_mode, 1},

// logical port id/mask
	{"wan_logical_portid", cli_env_wan_logical_portid, cli_env_set_wan_logical_portid, 1},
	{"uni_logical_portmask", cli_env_uni_logical_portmask, cli_env_set_uni_logical_portmask, 1},
	{"cpu_logical_portmask", cli_env_cpu_logical_portmask, cli_env_set_cpu_logical_portmask, 1},
	{"wifi_logical_portmask", cli_env_wifi_logical_portmask, cli_env_set_wifi_logical_portmask, 1},

// switch
	{"ivlan_enable",	cli_env_ivlan_enable, cli_env_set_ivlan_enable, 1},
	{"stp_conf_before_omci", cli_env_stp_conf_before_omci, cli_env_set_stp_conf_before_omci, 0},
	{"switch_history_mode",	cli_env_switch_history_mode, cli_env_set_switch_history_mode, 1},
	{"switch_history_interval",	cli_env_switch_history_interval, cli_env_set_switch_history_interval, 1},

	{"br_allowed_srcmask",	cli_env_br_allowed_srcmask, cli_env_set_br_allowed_srcmask, 1},
	{"rt_allowed_srcmask",	cli_env_rt_allowed_srcmask, cli_env_set_rt_allowed_srcmask, 1},
	{"trunk_group_portmask",	cli_env_trunk_group_portmask, cli_env_set_trunk_group_portmask, 1},

	{"localbridging_default",	cli_env_localbridging_default, cli_env_set_localbridging_default, 1},
	{"localbridging_addon_portmask",	cli_env_localbridging_addon_portmask, cli_env_set_localbridging_addon_portmask, 1},
	{"ctc_internet_bridge_enabled_srcmask",	cli_env_ctc_internet_bridge_enabled_srcmask, cli_env_set_ctc_internet_bridge_enabled_srcmask, 1},

	{"port2port_enable",	cli_env_port2port_enable, cli_env_set_port2port_enable, 1},
	{"maclearn_drop_enable",	cli_env_maclearn_drop_enable, cli_env_set_maclearn_drop_enable, 1},
	{"veip_for_wifi_enable",	cli_env_veip_for_wifi_enable, cli_env_set_veip_for_wifi_enable, 1},
	{"cpuport_ds_passthrough", cli_env_cpuport_ds_passthrough, cli_env_set_cpuport_ds_passthrough , 1},
	{"cpuport_pvlan_clone_ipv4_to_arp", cli_env_cpuport_pvlan_clone_ipv4_to_arp, cli_env_set_cpuport_pvlan_clone_ipv4_to_arp , 1},
	{"rma_forward_mask", cli_env_rma_forward_mask, cli_env_set_rma_forward_mask, 1},
	{"bundle_rate_support", cli_env_bundle_rate_support, cli_env_set_bundle_rate_support, 1},
	{"linkdown_debounce_time", cli_env_linkdown_debounce_time, cli_env_set_linkdown_debounce_time, 1},
	{"linkup_debounce_time", cli_env_linkup_debounce_time, cli_env_set_linkup_debounce_time, 1},

// rldp
	{"rldp_enable",	cli_env_rldp_enable, cli_env_set_rldp_enable, 1},
	{"rldp_def_portmask",	cli_env_rldp_def_portmask, cli_env_set_rldp_def_portmask, 1},
	{"rldp_passive_only",	cli_env_rldp_passive_only, cli_env_set_rldp_passive_only, 1},

// tag processing
	{"tag_treatment_correct", cli_env_tag_treatment_correct, cli_env_set_tag_treatment_correct, 1},
	{"tag_discard_reversing", cli_env_tag_discard_reversing, cli_env_set_tag_discard_reversing, 1},
	{"tag_preview_table", cli_env_tag_preview_table, cli_env_set_tag_preview_table, 1},
	{"tag_auto_reverse", cli_env_tag_auto_reverse, cli_env_set_tag_auto_reverse, 1},
	{"tag_mc_ds_from_us", cli_env_tag_mc_ds_from_us, cli_env_set_tag_mc_ds_from_us, 1},
	{"tag_mc_sort_priority", cli_env_tag_mc_sort_priority, cli_env_set_tag_mc_sort_priority, 1},
	{"tag_pbit_workaround", cli_env_tag_pbit_workaround, cli_env_set_tag_pbit_workaround, 1},
	{"tag_pbit_portmask", cli_env_tag_pbit_portmask, cli_env_set_tag_pbit_portmask, 1},
	{"tag_pbit_remap_table", cli_env_tag_pbit_remap_table, cli_env_set_tag_pbit_remap_table, 1},
	{"tag_ds_tci_dont_reverse_workaround", cli_env_tag_ds_tci_dont_reverse_workaround, cli_env_set_tag_ds_tci_dont_reverse_workaround, 1},
	{"tag_insert_65305_rules", cli_env_tag_insert_65305_rules, cli_env_set_tag_insert_65305_rules, 1},
	{"tag_ctc_translation_path",	cli_env_tag_ctc_translation_path, cli_env_set_tag_ctc_translation_path, 1},
	{"tag_mc_ds_wildcard", cli_env_tag_mc_ds_wildcard, cli_env_set_tag_mc_ds_wildcard, 1},
	{"tag_l2_rules_reverse", cli_env_tag_l2_rules_reverse, cli_env_set_tag_l2_rules_reverse, 1},
	{"tag_l2_rules_ds_mac", cli_env_tag_l2_rules_ds_mac, cli_env_set_tag_l2_rules_ds_mac, 1},
	{"tag_iphost_vid_map", cli_env_tag_iphost_vid_map, cli_env_set_tag_iphost_vid_map, 0},

// classf
	{"classf_tag_mode", cli_env_classf_tag_mode, cli_env_set_classf_tag_mode, 1},
	{"classf_add_default_rules", cli_env_classf_add_default_rules, cli_env_set_classf_add_default_rules, 1},
	{"classf_force_stpid", cli_env_classf_force_stpid, cli_env_set_classf_force_stpid, 1},
	{"classf_stag_tpid", cli_env_classf_stag_tpid, cli_env_set_classf_stag_tpid, 1},
	{"classf_hw_stag_tpid", cli_env_classf_hw_stag_tpid, cli_env_set_classf_hw_stag_tpid, 1},
	{"classf_et_combine", cli_env_classf_et_combine, cli_env_set_classf_et_combine, 1},
	{"classf_ds_gem_dg", cli_env_classf_ds_gem_dg, cli_env_set_classf_ds_gem_dg, 1},
	{"classf_dscp2pbit_mode", cli_env_classf_dscp2pbit_mode, cli_env_set_classf_dscp2pbit_mode, 1},
#if 0 // Remove VEIP related workaround
	{"classf_veip_orig_tci_map", cli_env_classf_veip_orig_tci_map, cli_env_set_classf_veip_orig_tci_map, 0},
	{"classf_veip_real_tci_map", cli_env_classf_veip_real_tci_map, cli_env_set_classf_veip_real_tci_map, 0},
	{"classf_veip_real_tag_rule_priority", cli_env_classf_veip_real_tag_rule_priority, cli_env_set_classf_veip_real_tag_rule_priority, 0},
#endif
	{"classf_ds_unmatch", cli_env_classf_ds_unmatch, cli_env_set_classf_ds_unmatch, 1},
	{"classf_ds_mc_extend", cli_env_classf_ds_mc_extend, cli_env_set_classf_ds_mc_extend, 1},
	{"classf_ds_mc_force_strip", cli_env_classf_ds_mc_force_strip, cli_env_set_classf_ds_mc_force_strip, 1},
	{"classf_veip_rules_reassign", cli_env_classf_veip_rules_reassign, cli_env_set_classf_veip_rules_reassign, 1},
	{"classf_dropping_rules_add", cli_env_classf_dropping_rules_add, cli_env_set_classf_dropping_rules_add, 1},
	{"classf_vmap_pbit_workaround", cli_env_classf_vmap_pbit_workaround, cli_env_set_classf_vmap_pbit_workaround, 1},
	{"classf_hw_2tags_mode", cli_env_classf_hw_2tags_mode, cli_env_set_classf_hw_2tags_mode, 1},
	{"classf_hw_2tags_workaround", cli_env_classf_hw_2tags_workaround, cli_env_set_classf_hw_2tags_workaround, 1},
	{"classf_add_ipv4mc_strip", cli_env_classf_add_ipv4mc_strip, cli_env_set_classf_add_ipv4mc_strip, 1},
	{"classf_add_hwnat_1tag_rules", cli_env_classf_add_hwnat_1tag_rules, cli_env_set_classf_add_hwnat_1tag_rules, 1},
	{"classf_add_hwnat_untag_rules", cli_env_classf_add_hwnat_untag_rules, cli_env_set_classf_add_hwnat_untag_rules, 1},
	{"classf_add_hwnat_untag_vid", cli_env_classf_add_hwnat_untag_vid, cli_env_set_classf_add_hwnat_untag_vid, 1},
	{"classf_add_hwnat_untag_vid_mask", cli_env_classf_add_hwnat_untag_vid_mask, cli_env_set_classf_add_hwnat_untag_vid_mask, 1},
	{"classf_hw_rules_migration", cli_env_classf_hw_rules_migration, cli_env_set_classf_hw_rules_migration, 1},
	{"classf_hw_colour_force", cli_env_classf_hw_colour_force, cli_env_set_classf_hw_colour_force, 1},
	{"classf_keep_candi", cli_env_classf_keep_candi, cli_env_set_classf_keep_candi, 1},
	{"classf_statistic_enable", cli_env_classf_statistic_enable, cli_env_set_classf_statistic_enable, 1},
	{"classf_tlan_pattern", cli_env_classf_tlan_pattern, cli_env_set_classf_tlan_pattern, 1},
	{"classf_hw_intf_lan_vid", cli_env_classf_hw_intf_lan_vid, cli_env_set_classf_hw_intf_lan_vid, 1},
	{"mc_ds_untag_discard_enable", cli_env_mc_ds_untag_discard_enable, cli_env_set_mc_ds_untag_discard_enable, 1},
	{"classf_add_default_rules_by_me_84", cli_env_classf_add_default_rules_by_me_84, cli_env_set_classf_add_default_rules_by_me_84, 1},

// cfm
	{"cfm_enable", cli_env_cfm_enable, cli_env_set_cfm_enable, 1},
	{"cfm_y1731_enable", cli_env_cfm_y1731_enable, cli_env_set_cfm_y1731_enable, 1},
	{"cfm_portmask", cli_env_cfm_portmask, cli_env_set_cfm_portmask, 1},
	{"cfm_mac_ifname",	cli_env_cfm_mac_ifname, cli_env_set_cfm_mac_ifname, 0},
	{"cfm_send_timeout", cli_env_cfm_send_timeout, cli_env_set_cfm_send_timeout, 1},
	{"cfm_reverse_mep_direction", cli_env_cfm_reverse_mep_direction, cli_env_set_cfm_reverse_mep_direction, 1},
	{"cfm_rmep_ccm_before_hold", cli_env_cfm_rmep_ccm_before_hold, cli_env_set_cfm_rmep_ccm_before_hold, 1},
	{"cfm_rmep_holdtime_before_active", cli_env_cfm_rmep_holdtime_before_active, cli_env_set_cfm_rmep_holdtime_before_active, 1},
	{"cfm_rmep_ccm_lost_threshold", cli_env_cfm_rmep_ccm_lost_threshold, cli_env_set_cfm_rmep_ccm_lost_threshold, 1},
	{"cfm_defect_mask_in_rdi_gen", cli_env_cfm_defect_mask_in_rdi_gen, cli_env_set_cfm_defect_mask_in_rdi_gen, 1},
	{"cfm_defect_mask_in_ais_gen", cli_env_cfm_defect_mask_in_ais_gen, cli_env_set_cfm_defect_mask_in_ais_gen, 1},
	{"cfm_defect_mask_supressed_by_ais", cli_env_cfm_defect_mask_supressed_by_ais, cli_env_set_cfm_defect_mask_supressed_by_ais, 1},
	{"cfm_defect_mask_supressed_by_lck", cli_env_cfm_defect_mask_supressed_by_lck, cli_env_set_cfm_defect_mask_supressed_by_lck, 1},

// lldp
	{"lldp_enable", cli_env_lldp_enable, cli_env_set_lldp_enable, 1},
	{"lldp_ifname", cli_env_lldp_ifname, cli_env_set_lldp_ifname, 1},
	{"lldp_dst_mac", cli_env_lldp_dst_mac, cli_env_set_lldp_dst_mac, 1},
	{"lldp_send_interval", cli_env_lldp_send_interval, cli_env_set_lldp_send_interval, 1},
	{"lldp_fast_tx", cli_env_lldp_fast_tx, cli_env_set_lldp_fast_tx, 1},
	{"lldp_credit_max", cli_env_lldp_credit_max, cli_env_set_lldp_credit_max, 1},
	{"lldp_tx_fast_init", cli_env_lldp_tx_fast_init, cli_env_set_lldp_tx_fast_init, 1},
	{"lldp_system_name", cli_env_lldp_system_name, cli_env_set_lldp_system_name, 1},
	{"lldp_system_desc", cli_env_lldp_system_desc, cli_env_set_lldp_system_desc, 1},
	{"lldp_med_opt", cli_env_lldp_med_opt, cli_env_set_lldp_med_opt, 1},
	{"lldp_med_cap", cli_env_lldp_med_cap, cli_env_set_lldp_med_cap, 1},

// tm
	{"tm_pq_gem_qos_method", cli_env_tm_pq_gem_qos_method, cli_env_set_tm_pq_gem_qos_method, 1},
	{"tm_gemus_rate_factor", cli_env_tm_gemus_rate_factor, cli_env_set_tm_gemus_rate_factor, 1},
	{"tm_gemds_rate_factor", cli_env_tm_gemds_rate_factor, cli_env_set_tm_gemds_rate_factor, 1},
	{"tm_unius_rate_factor", cli_env_tm_unius_rate_factor, cli_env_set_tm_unius_rate_factor, 1},
	{"tm_unids_rate_factor", cli_env_tm_unids_rate_factor, cli_env_set_tm_unids_rate_factor, 1},
	{"tm_pir_zero_action", cli_env_tm_pir_zero_action, cli_env_set_tm_pir_zero_action, 1},
	{"tm_protect_enable", cli_env_tm_protect_enable, cli_env_set_tm_protect_enable, 1},

// igmp
	{"igmp_enable", cli_env_igmp_enable, cli_env_set_igmp_enable, 0},
	{"igmp_v1_ignore", cli_env_igmp_v1_ignore, cli_env_set_igmp_v1_ignore, 1},
	{"igmp_version_overwrite", cli_env_igmp_version_overwrite, cli_env_set_igmp_version_overwrite, 0},
	{"igmp_join_default_grant", cli_env_igmp_join_default_grant, cli_env_set_igmp_join_default_grant, 1},
	{"igmp_mcast_me_support", cli_env_igmp_mcast_me_support, cli_env_set_igmp_mcast_me_support, 1},
	{"igmp_proxy_snooping", cli_env_igmp_proxy_snooping , cli_env_set_igmp_proxy_snooping , 1},
	{"igmp_upstream_tpid", cli_env_igmp_upstream_tpid, cli_env_set_igmp_upstream_tpid, 1},
	{"igmp_downstream_tpid", cli_env_igmp_downstream_tpid, cli_env_set_igmp_downstream_tpid, 1},
	{"igmp_preassign_list", cli_env_igmp_preassign_list, cli_env_set_igmp_preassign_list, 1},
	{"igmp_query_pass_through", cli_env_igmp_query_pass_through, cli_env_set_igmp_query_pass_through, 1},
	{"igmp_mcast_mode_src", cli_env_igmp_mcast_mode_src, cli_env_set_igmp_mcast_mode_src, 1},

// extoam
	{"extoam_enable", 		cli_env_extoam_enable, cli_env_set_extoam_enable, 1},
	{"extoam_port",			cli_env_extoam_port, cli_env_set_extoam_port, 1},
	{"extoam_mac_ifname", 		cli_env_extoam_mac_ifname, cli_env_set_extoam_mac_ifname, 1},
	{"extoam_itas_image_name", 	cli_env_extoam_itas_image_name, cli_env_set_extoam_itas_image_name, 1},

// voip
	{"voip_enable",	cli_env_voip_enable, cli_env_set_voip_enable, 0},
	{"voip_debug_set_flag",	cli_env_voip_debug_set_flag, cli_env_set_voip_debug_set_flag, 1},
	{"voip_debug_get_flag",cli_env_voip_debug_get_flag, cli_env_set_voip_debug_get_flag, 1},
	{"voip_debug_release_flag",cli_env_voip_debug_release_flag, cli_env_set_voip_debug_release_flag, 1},

// misc
	{"batchtab_extevent_idletime", cli_env_batchtab_extevent_idletime, cli_env_set_batchtab_extevent_idletime, 1},
	{"batchtab_retry_timeout", cli_env_batchtab_retry_timeout, cli_env_set_batchtab_retry_timeout, 1},
	{"batchtab_wanif_feature", cli_env_batchtab_wanif_feature, cli_env_set_batchtab_wanif_feature, 1},
	{"batchtab_hardbridge_enable", cli_env_batchtab_hardbridge_enable, cli_env_set_batchtab_hardbridge_enable, 1},
	{"exclude_flowid_0",	cli_env_exclude_flowid_0, cli_env_set_exclude_flowid_0, 1},
	{"avoid_pir_drop",	cli_env_avoid_pir_drop, cli_env_set_avoid_pir_drop, 1},
	{"uni_policing_pbit_mask",	cli_env_uni_policing_pbit_mask, cli_env_set_uni_policing_pbit_mask, 1},
	{"uni_policing_default_bs",	cli_env_uni_policing_default_bs, cli_env_set_uni_policing_default_bs, 1},
	{"uni_policing_aggregate_acl",	cli_env_uni_policing_aggregate_acl, cli_env_set_uni_policing_aggregate_acl, 1},
	{"uni_linkdown_lock",	cli_env_uni_linkdown_lock, cli_env_set_uni_linkdown_lock, 1},
	{"watchdog_omccmiss_timeout", cli_env_watchdog_omccmiss_timeout, cli_env_set_watchdog_omccmiss_timeout, 1},
	{"watchdog_tcontmiss_timeout",	cli_env_watchdog_tcontmiss_timeout, cli_env_set_watchdog_tcontmiss_timeout, 1},
	{"watchdog_o2stuck_timeout",	cli_env_watchdog_o2stuck_timeout, cli_env_set_watchdog_o2stuck_timeout, 1},
	{"watchdog_idlebw_guard", cli_env_watchdog_idlebw_guard, cli_env_set_watchdog_idlebw_guard, 1},
	{"uni_config_lock",   cli_env_uni_config_lock, cli_env_set_uni_config_lock, 1},
	{NULL,	NULL, NULL, 0},
};

static int
cli_envget(int fd, char *envname)
{
	int i, found=0;;

	// exact match
	for (i=0; cli_env_map[i].name!=NULL; i++) {
		if (strcmp(envname, cli_env_map[i].name)==0) {
			cli_env_map[i].get_funcptr(fd);
			util_fdprintf(fd, "\n");
			return CLI_OK;
		}
	}
	// prefix match	
	for (i=0; cli_env_map[i].name!=NULL; i++) {
		if (strstr(cli_env_map[i].name, envname)) {
			util_fdprintf(fd, "%s = ", cli_env_map[i].name);
			cli_env_map[i].get_funcptr(fd);
			util_fdprintf(fd, "\n");
			found++;
		}
	}
	if (found==0) {	
		util_fdprintf(fd, "unknown env?\n");
		return CLI_ERROR_SYNTAX;
	}
	return CLI_OK;	
}

static int
cli_envget_summary(int fd)
{
	int i;
	for (i=0; cli_env_map[i].name!=NULL; i++) {
		util_fdprintf(fd, "%s = ", cli_env_map[i].name);
		cli_env_map[i].get_funcptr(fd);
		if (cli_env_map[i].writeable) {
			util_fdprintf(fd, "\n");
		} else {
			util_fdprintf(fd, " (RO)\n");
		}
	}
	return CLI_OK;
}

static int
cli_envset(int fd, char *envname, char *str)
{
	int i;

	for (i=0; cli_env_map[i].name!=NULL; i++) {
		if (strcmp(envname, cli_env_map[i].name)==0) {
			if (cli_env_map[i].writeable) {
				cli_env_map[i].set_funcptr(fd, str);
				return CLI_OK;
			} else {
				util_fdprintf(fd, "%s is readonly\n", envname);
				return CLI_ERROR_READONLY;
			}
		}
	}
	util_fdprintf(fd, "unknown env?\n");
	return CLI_ERROR_SYNTAX;
}

// helper function used by cli.c /////////////////////////////////////////////////////////////////////////////////////////////

void
cli_env_help(int fd)
{
	util_fdprintf(fd, 
		"env [envname] [string]\n");
}

int
cli_env_cmdline(int fd, int argc, char *argv[])
{
	if (strcmp(argv[0], "env")==0) {
		if (argc==1) {
			return cli_envget_summary(fd);
		} else if (argc>=3) {
			char str[1024];
			int i;

			str[0]=0;	
			for (i=2; i<argc; i++) {
				if (i>2) strcat(str, " ");
				strcat(str, argv[i]);
			}
			return cli_envset(fd, argv[1], str);
		} if (argc==2) {
			return cli_envget(fd, argv[1]);
		}
	}
	return CLI_ERROR_OTHER_CATEGORY;
}
