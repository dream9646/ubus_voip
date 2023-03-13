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
 * File    : env.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "util.h"
#include "meinfo.h"
#include "gpon_sw.h"
#include "tm.h"
#include "conv.h"
#include "env.h"

struct omci_env_t omci_env_g;

static unsigned char all_zero[6]={0,0,0,0,0,0};
static unsigned char all_ff[6]={0xff,0xff,0xff,0xff,0xff,0xff};

// syntax:  192.168.1.0/0xffffffff, 01:00:5e:01:02:03/ff:ff:ff:ff:ff:ff
//	    192.168.1.0, 01:00:5e:01:02:03 (mask is assumed to be all 0xff if not specfied)
int
env_print_igmp_preassign_list(int fd)
{
	int i;
	for (i=0; i<ENV_SIZE_IGMP_PREASSIGN_LIST; i++) {
		if (omci_env_g.igmp_preassign_list[i].ipaddr!=0) {
			struct in_addr in;
			in.s_addr=htonl(omci_env_g.igmp_preassign_list[i].ipaddr);
			util_fdprintf(fd, "%s%s", (i!=0)?", ":"", inet_ntoa(in));
			if (omci_env_g.igmp_preassign_list[i].ipaddr_mask != 0xffffffff) {
				util_fdprintf(fd, "/0x%x", omci_env_g.igmp_preassign_list[i].ipaddr_mask);
			}
		} else if (memcmp(omci_env_g.igmp_preassign_list[i].mac, all_zero, 6) !=0) {
			util_fdprintf(fd, "%s%s", (i!=0)?", ":"", util_str_macaddr(omci_env_g.igmp_preassign_list[i].mac));
			if (memcmp(omci_env_g.igmp_preassign_list[i].mask, all_ff, 6) !=0)  {
				util_fdprintf(fd, "/%s", util_str_macaddr(omci_env_g.igmp_preassign_list[i].mask));
			}
		}
	}
	
	return 0;
}

int
env_set_igmp_preassign_list(char *str)
{
	char buff[LINE_MAXSIZE];
	char *a[ENV_SIZE_IGMP_PREASSIGN_LIST];
	int n, i, j;

	memset(omci_env_g.igmp_preassign_list, 0, sizeof(omci_env_g.igmp_preassign_list));

	strncpy(buff, str, LINE_MAXSIZE - 1); buff[LINE_MAXSIZE - 1] = 0;
	n = conv_str2array(buff, ",", a, ENV_SIZE_IGMP_PREASSIGN_LIST);
	for (i=0, j=0; i<n; i++) {
		char *p=index(a[i], '/');
		if (p)
			*p=0;
		if (strlen(a[i])>=17) {	// L2 mcast addr
			unsigned char mac[6], mask[6];
			if (conv_str_mac_to_mem(mac, a[i]) ==0 &&
			    memcmp(mac, all_zero, 6) != 0) {
				memcpy(omci_env_g.igmp_preassign_list[j].mac, mac, 6);
				if (p && conv_str_mac_to_mem(mask, p+1) ==0) {
				   	memcpy(omci_env_g.igmp_preassign_list[j].mask, mask, 6);
				} else {
					memcpy(omci_env_g.igmp_preassign_list[j].mask, all_ff, 6);
				}
				j++;

			} 
		} else {					// ipv4 mcast
			unsigned int ipaddr=ntohl(inet_addr(a[i]));			
			if (ipaddr && ipaddr!=0xffffffff) {
				// 2510 only supports mask 0xffffffff
				omci_env_g.igmp_preassign_list[j].ipaddr=ipaddr;
				if (p) {
					omci_env_g.igmp_preassign_list[j].ipaddr_mask=util_atoll(p+1);
				} else {
					omci_env_g.igmp_preassign_list[j].ipaddr_mask=0xffffffff;
				}
				j++;
			}
		}
	}

	return j;
}

int
env_set_tcont_map_policy(char *str)
{
	char buff[64], *a[2];
	int i, n;
	strncpy(buff, str, 64); buff[63]=0;
	n=conv_str2array(str, ",", a, 2);
	for (i=0; i<n; i++) {
		if (strcmp(a[i], "SP")==0) {
			omci_env_g.tcont_map_policy[i]=1;
		} else if (strcmp(a[i], "WRR")==0) {
			omci_env_g.tcont_map_policy[i]=2;
		}
	}
	return n;
}

int
env_set_sim_tcont_enable(int enable)
{
	static struct gpon_hw_t orig_gpon_hw_g = {0};

	if (orig_gpon_hw_g.init == NULL)
		orig_gpon_hw_g = gpon_hw_g;

	if (enable) {	
		struct meinfo_t *miptr=meinfo_util_miptr(262);
		struct me_t *me;
		struct attr_value_t attr;

		// assign allocid for all tcont me
		list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
			attr.data = 256 + me->meid % GPON_TCONT_ID_TOTAL;	// fake allocid
			meinfo_me_attr_set(me, 1, &attr, 0);
		}
		gpon_hw_g.tm_tcont_get_by_allocid = gpon_null_tm_tcont_get_by_allocid;
		gpon_hw_g.tm_pq_config_get = gpon_null_tm_pq_config_get;
		gpon_hw_g.tm_pq_config_set = gpon_null_tm_pq_config_set;
	} else {
		gpon_hw_g = orig_gpon_hw_g;
	}
	return 0;
}

#define ENV_DEFAULT_UNI_LOGICAL_PROTMASK		0x1f
#define ENV_DEFAULT_CPU_LOGICAL_PROTMASK		0x1c0
#define ENV_DEFAULT_WIFI_LOGICAL_PROTMASK		0x180
#define ENV_DEFAULT_BR_ALLOWED_SRCMASK			0x1f
#define ENV_DEFAULT_RT_ALLOWED_SRCMASK			0x1f
#define ENV_DEFAULT_LOCALBRIDGING_ADDON_PORTMASK	0xf80	// cpu ext0..5
#define ENV_DEFAULT_RLDP_ENABLE				0
#define ENV_DEFAULT_RLDP_DEF_PORTMASK			0x1f
#define ENV_DEFAULT_CFM_PORTMASK			0x2f


int
env_set_to_default(void)
{
	int i;
	
	memset(&omci_env_g, 0x00, sizeof(omci_env_g));

// global
	omci_env_g.cli_ip = 0; // INADDR_ANY
	omci_env_g.cli_port = 3097;
	omci_env_g.transport_type = 0;
	strncpy(omci_env_g.etc_omci_path, "/etc/omci", 256);
	strncpy(omci_env_g.etc_omci_path2, "/opt/omci", 256);
	strncpy(omci_env_g.start_cmd_file, "/var/run/omci.start_cmd", 256);

// interface config
	strncpy(omci_env_g.trap_if, "ifsw0", ENV_SIZE_IFNAME);
	strncpy(omci_env_g.omcc_if, "ifomci", ENV_SIZE_IFNAME);

// connecting olt simulator
	strncpy(omci_env_g.sim_olt_ip, "127.0.0.1", 16);
	omci_env_g.sim_olt_udp_port = 5555;
	omci_env_g.sim_pon_id = 1;
	omci_env_g.sim_onu_id = 1;
	omci_env_g.sim_tcont_enable = 0;

// omci msg
	omci_env_g.omcc_rx_crc_type = ENV_CRC_TYPE_AUTO;
	omci_env_g.omcc_tx_crc_type = ENV_CRC_TYPE_BE;

	omci_env_g.omci_tranx_cache_num = 10;
	omci_env_g.omci_tranx_match_mode = 1;
	omci_env_g.omci_tranx_bypass = 0;
	omci_env_g.omci_tranx_busy_enable = 1;

	omci_env_g.omci_exec_time_limitation = 3000;
	omci_env_g.omci_tx_flag = 0;
	omci_env_g.omci_tx_resend_flag = 1;

	omci_env_g.omci_history_enable = 1;
	omci_env_g.omci_history_size = 100;
	omci_env_g.omci_history_msgmask = 0xfbefafff;	// all except get_next, dl_section, mib_ul_next, get_all_alarm_next

	omci_env_g.omcidump_enable = 4;			// one line mode
	omci_env_g.omcidump_msgmask = 0xfbefafff;	// all except get_next, dl_section, mib_ul_next, get_all_alarm_next

	omci_env_g.omccrx_log_enable = 0;		// none
	omci_env_g.omccrx_log_msgmask = 0xfbefafff;	// all except get_next, dl_section, mib_ul_next, get_all_alarm_next
	strncpy(omci_env_g.omccrx_log_file, "/var/log/omccrx.log", 256);

	omci_env_g.restore_log_enable = 0;
	strncpy(omci_env_g.restore_log_file, "/etc/restore.log", 256);

// mib
	for (i=0; i<14; i++) {	// load section 1..14
		omci_env_g.xml_section[i]=i+1;
	}
	omci_env_g.xml_section[i]=0;	
	omci_env_g.xml_load_namestr = 1;

	omci_env_g.mib_boundary_check = ENV_MIB_BOUNDARY_CHECK_WANRING;
	omci_env_g.mib_pointer_check = ENV_MIB_POINTER_CHECK_WANRING;
	omci_env_g.mib_readonly_check = ENV_MIB_READONLY_CHECK_WANRING;
	omci_env_g.mib_rw_mode = ENV_MIB_RW_MODE_SWSW;
	omci_env_g.mib_aliasid_enable = 1;
	omci_env_g.mib_rare_meid_start = 0xffe0;
	omci_env_g.mib_rare2_meid_start = 0xfff0;
	omci_env_g.mib_nonexist_del_err = ENV_MIB_NONEXIST_DEL_ERR_ME;

	omci_env_g.autouni_enable = 1;
	omci_env_g.autoveip_enable = 0;
	omci_env_g.erdiag_mon_enable = 0;
	omci_env_g.mibreset_dump_enable = 0;
	omci_env_g.mib_uni_adminlock_default = ENV_MIB_UNI_ADMINLOCK_DEFAULT_DISABLE;
	omci_env_g.adminlock_trigger_opstate = 1;
	omci_env_g.untag_reserved_vid = 4095;	

	omci_env_g.port_forward_operation_default = 0;	// 9.3.11 attr2

	omci_env_g.pseudo_ts_pointer_enable = 0;
	omci_env_g.invert_weight_enable = 0;
	omci_env_g.invert_priority_enable = 0;
	omci_env_g.pq_tcont_relation = ENV_PQ_TCONT_RELATION_BY_TS_POINTER;
	omci_env_g.tcont_map_enable = 0;
	omci_env_g.tcont_map_policy[0]=1;	// sp
	omci_env_g.tcont_map_policy[1]=1;	// sp
	for (i=0; i<ENV_SIZE_TCONT_MAP; i++) {
		omci_env_g.tcont_map_ts[i]=4;
		omci_env_g.tcont_map_pq[i]=4;
	}

// sw_download
	omci_env_g.sw_download_end_behavior = 0;
	omci_env_g.sw_download_active_always_ok= 0;
	omci_env_g.sw_download_commit_always_ok= 0;
	omci_env_g.sw_download_window_size= 255;
	strncpy(omci_env_g.sw_download_script, "/usr/sbin/sw_download.sh", 256);
	strncpy(omci_env_g.sw_download_image_name, "/var/run/swimage/swimage.bin", 256);

// task threads
	omci_env_g.task_pause_mask = 0;
	omci_env_g.avc_refresh_interval = 5;
	omci_env_g.alarm_refresh_interval = 5;
	omci_env_g.pm_refresh_interval = 60;
	omci_env_g.pm_swap_interval = 900; //15 mins

	omci_env_g.cpuport_history_enable = 0;
	omci_env_g.cpuport_history_size = 1000;
	omci_env_g.cpuport_arp_enable = 0;		//filter ARP
	omci_env_g.cpuport_dhcp_enable = 0;		//filter DHCP OFFER/ACK/NAK
	memset(omci_env_g.cpuport_dhcp_cust_tag, 0, sizeof(omci_env_g.cpuport_dhcp_cust_tag));
	omci_env_g.cpuport_dhcp_server_onlan = 0;
	omci_env_g.cpuport_pppoe_enable = 0;		//filter PPPoE of PPPoE plus
	memset(omci_env_g.cpuport_pppoe_cust_tag, 0, sizeof(omci_env_g.cpuport_pppoe_cust_tag));
	omci_env_g.cpuport_pkt_ssdp_fwd_wan = 1;
	omci_env_g.cpuport_im_enable = 0;
	omci_env_g.cpuport_im_lan2lan_enable = 0;
	omci_env_g.cpuport_wifi_block_mask = 0x0;
	omci_env_g.cpuport_wifi_block_ra = 0x0;
	omci_env_g.cpuport_mcast_drop = 0x0;
	omci_env_g.stp_enable = 0;
	
	omci_env_g.cpu_usage_threshold = 0;
	//for cpu limit task no
	for (i=0; i<ENV_SIZE_TASK; i++) {
		omci_env_g.cpul_task_no[i]=-1;
	}
	omci_env_g.cpul_total_usage = 100;

// debug
	omci_env_g.debug_level = 3;
	omci_env_g.debug_level_bat = 3;
	omci_env_g.debug_level_cfm = 3;
	omci_env_g.debug_level_cpuport = 3;
	omci_env_g.debug_level_fwk = 3;
	omci_env_g.debug_level_igmp = 3;
	omci_env_g.debug_level_lldp = 3;
	omci_env_g.debug_level_vacl = 3;
	omci_env_g.debug_level_voip = 3;
	omci_env_g.debug_level_xml = 4;
	omci_env_g.debug_log_time = 1;
	omci_env_g.debug_log_type = ENV_DEBUG_LOG_TYPE_BITMASK_CONSOLE;
	strncpy(omci_env_g.debug_log_file, "/var/log/omci.log", 256);
	omci_env_g.memdbg_enable = 0;
	omci_env_g.er_group_hw_debug_enable = 0;
	omci_env_g.er_group_hw_time_enable = 0;

// olt
	omci_env_g.olt_vendor_check_enable = 0;
	omci_env_g.olt_vendor_config_change = 0;
	omci_env_g.olt_workaround_enable = ENV_OLT_WORKAROUND_NONE;
	omci_env_g.olt_alu_mcast_support = 0;
	omci_env_g.olt_huawei_ignore_vlan_drop_rule = 0;
	strncpy(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_NONE, ENV_SIZE_PROPRIETARY_VENDOR);
	memset(omci_env_g.ont_force_vendor_id, 0, 5);

// anig
	omci_env_g.anig_type = ENV_ANIG_TYPE_TRANCEIVER;
	omci_env_g.anig_support = ENV_ANIG_SUPPORT_GET_TEST;
	omci_env_g.anig_txrx_threshold[0] = 7;		// tx max in dbm
	omci_env_g.anig_txrx_threshold[1] = -6;		// tx min
	omci_env_g.anig_txrx_threshold[2] = -4;		// rx max
	omci_env_g.anig_txrx_threshold[3] = -30;	// rx min
	omci_env_g.anig_devport = 0;
	omci_env_g.anig_devaddr0 = 0x0;
	omci_env_g.anig_mon_mode = ENV_ANIG_MON_MODE_CAL_INT;

// logical port id/mask
	omci_env_g.wan_logical_portid = 255;
	omci_env_g.uni_logical_portmask = ENV_DEFAULT_UNI_LOGICAL_PROTMASK;
	omci_env_g.cpu_logical_portmask = ENV_DEFAULT_CPU_LOGICAL_PROTMASK;
	omci_env_g.wifi_logical_portmask = ENV_DEFAULT_WIFI_LOGICAL_PROTMASK;

// switch
	omci_env_g.ivlan_enable = 0;
	omci_env_g.stp_conf_before_omci = ENV_STP_CONF_ALL_DISABLE;
	omci_env_g.switch_history_mode= 0;			// 0:diff, 1:total
	omci_env_g.switch_history_interval= 60;			// unit: minute

	omci_env_g.br_allowed_srcmask= ENV_DEFAULT_BR_ALLOWED_SRCMASK;
	omci_env_g.rt_allowed_srcmask= ENV_DEFAULT_RT_ALLOWED_SRCMASK;
	omci_env_g.trunk_group_portmask = 0x0;

	omci_env_g.localbridging_default = 1;
	omci_env_g.localbridging_addon_portmask = ENV_DEFAULT_LOCALBRIDGING_ADDON_PORTMASK;	// cpu ext bit 1..5
	omci_env_g.ctc_internet_bridge_enabled_srcmask = 0;
	
	omci_env_g.port2port_enable = 0;
	omci_env_g.maclearn_drop_enable = 0;
	omci_env_g.veip_for_wifi_enable = 1;
	omci_env_g.cpuport_ds_passthrough = 0;
	omci_env_g.cpuport_pvlan_clone_ipv4_to_arp = 1;
	omci_env_g.rma_forward_mask = 0xfffffffffffffffdULL;	// 01:80:c2:00:00:00..3f forward mask
	omci_env_g.bundle_rate_support = 0;
	omci_env_g.linkdown_debounce_time = 12;
	omci_env_g.linkup_debounce_time = 2;

// rldp
	omci_env_g.rldp_enable = ENV_DEFAULT_RLDP_ENABLE;
	omci_env_g.rldp_def_portmask= ENV_DEFAULT_RLDP_DEF_PORTMASK;
	omci_env_g.rldp_passive_only= 0;

// tag
	omci_env_g.tag_treatment_correct = 1;
	omci_env_g.tag_discard_reversing = 0;
	omci_env_g.tag_preview_table = 0;
	omci_env_g.tag_auto_reverse = 1;
	omci_env_g.tag_mc_ds_from_us = 0;
	omci_env_g.tag_mc_sort_priority = 6;
	omci_env_g.tag_pbit_workaround = 0;
	omci_env_g.tag_pbit_portmask = 0x0;
	for (i=0; i<8; i++) {
		omci_env_g.tag_pbit_remap_table[i]=i;
	}
	omci_env_g.tag_ds_tci_dont_reverse_workaround = 0;
	omci_env_g.tag_insert_65305_rules = 0;
	strncpy(omci_env_g.tag_ctc_translation_path, "/var/run/uni_vlan_translation.table", 256);
	omci_env_g.tag_mc_ds_wildcard = 0;
	omci_env_g.tag_l2_rules_reverse = 0; //auto reverse
	omci_env_g.tag_l2_rules_ds_mac = 0;
	omci_env_g.tag_iphost_vid_map[0] = 4096;
	omci_env_g.tag_iphost_vid_map[1] = 4096;
	omci_env_g.tag_iphost_vid_map[2] = 4096;
	omci_env_g.tag_iphost_vid_map[3] = 4096;

// classf
	omci_env_g.classf_tag_mode = 1;
	omci_env_g.classf_add_default_rules = 1;
	omci_env_g.classf_force_stpid = 0;
	omci_env_g.classf_stag_tpid = 0x88a8;
	omci_env_g.classf_hw_stag_tpid = 0x88a8;
	omci_env_g.classf_et_combine = 1;
	omci_env_g.classf_ds_gem_dg = 1;
	omci_env_g.classf_dscp2pbit_mode = 1;
	//omci_env_g.classf_veip_real_tag_rule_priority = 4; // Remove VEIP related workaround
	omci_env_g.classf_ds_unmatch = 0; //pass
	omci_env_g.classf_ds_mc_extend = 0; //disable
	omci_env_g.classf_ds_mc_force_strip = 0; //disable
	omci_env_g.classf_veip_rules_reassign = 0; //disable
	omci_env_g.classf_dropping_rules_add = 0; //do NOT add
	omci_env_g.classf_vmap_pbit_workaround = 0; //disable
	omci_env_g.classf_hw_2tags_mode = 1; //disable
	omci_env_g.classf_hw_2tags_workaround = 0; 	//filter pri and vid
	omci_env_g.classf_add_ipv4mc_strip = 0; 	//disable
	omci_env_g.classf_add_hwnat_1tag_rules = 0; 	//disable
	omci_env_g.classf_add_hwnat_untag_rules = 0; 	//disable
	omci_env_g.classf_add_hwnat_untag_vid = 0x0;
	omci_env_g.classf_add_hwnat_untag_vid_mask = 0xfff;
	omci_env_g.classf_hw_rules_migration = 1; 	//enable
	omci_env_g.classf_hw_colour_force = 2; 	//auto
	omci_env_g.classf_keep_candi = 2; 	//auto
	omci_env_g.classf_statistic_enable = 0; 	//disable
	omci_env_g.classf_tlan_pattern = 0; 	//disable
	omci_env_g.classf_hw_intf_lan_vid = 4091; 	//disable
	omci_env_g.mc_ds_untag_discard_enable = 1;
	omci_env_g.classf_add_default_rules_by_me_84 = 0;

// cfm
	omci_env_g.cfm_enable = 0;	// lbm/ltm/ccm
	omci_env_g.cfm_y1731_enable = 0;	// filter Y.1731
	omci_env_g.cfm_portmask = ENV_DEFAULT_CFM_PORTMASK;		// ports running cfm, default: pon + uni 0..3
	strncpy(omci_env_g.cfm_mac_ifname, "pon0", ENV_SIZE_IFNAME);
	omci_env_g.cfm_send_timeout = 5;	//CFM send LBM/LTM timeout
	omci_env_g.cfm_reverse_mep_direction = 0;	//CFM use alu mep up/down direction
	omci_env_g.cfm_rmep_ccm_before_hold = 3;	// number of consecutive ccm required for rmep changed from DISCOVER to HOLD
	omci_env_g.cfm_rmep_holdtime_before_active = 10000;	// unit:ms, holdtime(no ccm lost/defect) for rmep changed from HOLD state & ACTIVE
	omci_env_g.cfm_rmep_ccm_lost_threshold = 3;	// number of lost ccm for rmep changed from ACTIVE to LOST
	omci_env_g.cfm_defect_mask_in_rdi_gen = 0x7e;	// defect that triggers the CCM rdi send, AIS send
	omci_env_g.cfm_defect_mask_in_ais_gen = 0x7f;
	omci_env_g.cfm_defect_mask_supressed_by_ais = 0x04;	// defects supressed by AIS/LCK recv in alarm/ais/rdi generation, by default: only REMOTE_CCM
	omci_env_g.cfm_defect_mask_supressed_by_lck = 0x04;

// lldp
	omci_env_g.lldp_enable = 0;	// lldp
	strncpy(omci_env_g.lldp_ifname, "br0", ENV_SIZE_IFNAME);	// use ifname's macaddr/ipaddr
	omci_env_g.lldp_dst_mac = 0xe;		// dst mac to send lldp message
	omci_env_g.lldp_send_interval = 30;	// interval to send lldp message
	omci_env_g.lldp_fast_tx = 1;
	omci_env_g.lldp_credit_max = 1;
	omci_env_g.lldp_tx_fast_init = 5;
	strncpy(omci_env_g.lldp_system_name, "", ENV_SIZE_STRING);	// system name to be set in lldp message
	strncpy(omci_env_g.lldp_system_desc, "", ENV_SIZE_STRING);	// system desc to be set in lldp message
	omci_env_g.lldp_med_opt = 2;
	omci_env_g.lldp_med_cap = 0x23;

// tm (traffic manager method)
	omci_env_g.tm_pq_gem_qos_method=0;	// 0:normal(green only), 1:green, yellow, 2:each gem port has it own pq(todo)
	omci_env_g.tm_gemus_rate_factor=100;	// default 100% for shaping (pon pq)
	omci_env_g.tm_gemds_rate_factor=115;	// default 115% for policing (meter)
	omci_env_g.tm_unius_rate_factor=115;	// default 115% for policing (uni ingress)
	omci_env_g.tm_unids_rate_factor=100;	// default 100% for shaping (uni egress q)
	omci_env_g.tm_pir_zero_action=1;	// default bypass
	omci_env_g.tm_protect_enable=3;		// default b1:uni & b2:pon

// igmp
	omci_env_g.igmp_enable = 0;
	omci_env_g.igmp_version_overwrite = 0;
	omci_env_g.igmp_join_default_grant = 1;
	omci_env_g.igmp_mcast_me_support= 1;
	omci_env_g.igmp_proxy_snooping= 0;
	omci_env_g.igmp_upstream_tpid = 0x8100;
	omci_env_g.igmp_downstream_tpid = 0x8100;
	omci_env_g.igmp_query_pass_through = 0;
	omci_env_g.igmp_v1_ignore = 0;
	//env_set_igmp_preassign_list("224.0.0.0/0xfffffe00");
	//env_set_igmp_preassign_list("224.0.0.1/0xffffffff,224.0.0.2/0xffffffff,224.0.0.5/0xffffffff,224.0.0.6/0xffffffff");
	omci_env_g.igmp_mcast_mode_src = 1; //force enable;

// extoam
	omci_env_g.extoam_enable = 0; 		// whether to start extoam task
	omci_env_g.extoam_port = 5;
	strncpy(omci_env_g.extoam_mac_ifname, "br0", ENV_SIZE_IFNAME);	// use ifname macaddr as extoam src mac
	strncpy(omci_env_g.extoam_itas_image_name, "itas_image.tgz", 256);

// voip
	omci_env_g.voip_enable = ENV_VOIP_AUTO;
	//flags for voip debug
	omci_env_g.voip_debug_set_flag = 0;
	omci_env_g.voip_debug_get_flag = 0;
	omci_env_g.voip_debug_release_flag = 0;

// misc
	omci_env_g.batchtab_extevent_idletime = 3;
	omci_env_g.batchtab_retry_timeout = 60;
	// b0/1:iphost add/del, b2/3:tr069 add/del, b4/5:rgvlan add/del, b6/7:brwan add/del, 
	// b8:auto igmp enable, b9:rgvlan_ignore_untag, b10:rgvlan_del_nonauto, b11:tr069_is_iphost
	omci_env_g.batchtab_wanif_feature = 0x203;
	omci_env_g.batchtab_hardbridge_enable = 1;
	omci_env_g.exclude_flowid_0 = 0;
	omci_env_g.avoid_pir_drop = 0;
	omci_env_g.uni_policing_pbit_mask = 0;
	omci_env_g.uni_policing_default_bs = METER_MAX_BURST_BYTES;
	omci_env_g.uni_policing_aggregate_acl = 0;
	omci_env_g.uni_linkdown_lock = 0;
	omci_env_g.watchdog_omccmiss_timeout[0] = 60;		// check if omccmiss remains for more than N sec, then reset serdes or reboot
	omci_env_g.watchdog_omccmiss_timeout[1] = 120;		// 
	omci_env_g.watchdog_tcontmiss_timeout[0] = 240;		// check if tcontmiss remains for more than N sec, then reset serdes or reboot
	omci_env_g.watchdog_tcontmiss_timeout[1] = 300;		//
	omci_env_g.watchdog_o2stuck_timeout[0] = 60;		// check if ont remains in O2 for more than N sec, then reset serdes or reboot
	omci_env_g.watchdog_o2stuck_timeout[1] = 90;		//
	omci_env_g.watchdog_idlebw_guard[0] = 2;		// traffic: us traffic threshold, watchdog is checked only if us traffic is under threshold
	omci_env_g.watchdog_idlebw_guard[1] = 100;		// bw: overalloc bw threshold, a tcont is treated as overalloc if its idlegem bw - all gemflow cir > threshold
	omci_env_g.watchdog_idlebw_guard[2] = 20;		// interval: the overalloc is checked i interval secs
	omci_env_g.watchdog_idlebw_guard[2] = 6;		// repeat: reset_dbru will be issued if overalloc continues repeat times

	omci_env_g.uni_config_lock = 0;

	return 0;
}

char *
env_get_taskname_by_id(int taskid)
{
	char *taskname=NULL;
	int i;

	for (i=0; i<ENV_SIZE_TASK; i++) {
		if ( omci_env_g.taskid[i] == taskid) {
			taskname = omci_env_g.taskname[i];
			break;
		}
	}
	if (taskname == NULL)
		taskname = "?"; 	
	return taskname;
}
