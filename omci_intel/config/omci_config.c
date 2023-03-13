/******************************************************************
 *
 * Copyright (C) 2021 5V Technologies Ltd.
 * All Rights Reserved.
 *
 * This program is the proprietary software of 5V Technologies Ltd
 * and/or its licensors, and only be used, duplicated,
 * modified or distributed under the authorized license from 5VT.
 *
 * ANY USE OF THE SOFTWARE OTHER THAN AS AUTHORIZED UNDER
 * THIS LICENSE OR COPYRIGHT LAW IS PROHIBITED.
 *
 * Purpose : 5VT OMCI uci config
 * Module  : config
 * File    : omci_config.c
 *
 ******************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <syslog.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <uci.h>

#include "util.h"
#include "env.h"
#include "conv.h"


#define OMCI_CONFIG_DEFAULT_CONFIG_PATH	NULL /* use the default set in libuci */


extern struct omci_env_t omci_env_g;
const char *g_omci_config_path = OMCI_CONFIG_DEFAULT_CONFIG_PATH;
static struct uci_context *g_omci_uci_context = NULL;
static struct uci_package *g_omci_uci_package = NULL;


static struct uci_package *
omci_config_init_package(const char *config)
{
	struct uci_context *ctx = g_omci_uci_context;
	struct uci_package *p = NULL;

	if (!ctx) {
		ctx = uci_alloc_context();
		g_omci_uci_context = ctx;

		ctx->flags &= ~UCI_FLAG_STRICT;
		if (g_omci_config_path)
			uci_set_confdir(ctx, g_omci_config_path);

	} else {
		p = uci_lookup_package(ctx, config);
		if (p)
			uci_unload(ctx, p);
	}

	if (uci_load(ctx, config, &p))
		return NULL;

	return p;
}

static void
omci_config_init_config(void)
{
#if 1
	struct uci_element *e;
	const char *pValue;


	uci_foreach_element(&g_omci_uci_package->sections, e) {
		struct uci_section *s = uci_to_section(e);

		// global
		if (strcmp(s->type, "global") == 0)
		{
	        if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "cli_ip")))
	        {
				omci_env_g.cli_ip=(strcmp(pValue, "0") == 0)?0:ntohl(inet_addr(pValue));
	        }

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "cli_port")))
	        {
				omci_env_g.cli_port = util_atoi((char *)pValue);
	        }

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "transport_type")))
			{
				omci_env_g.transport_type = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "etc_omci_path")))
			{
				strncpy(omci_env_g.etc_omci_path, pValue, ENV_SIZE_PATH);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "etc_omci_path2")))
			{
				strncpy(omci_env_g.etc_omci_path2, pValue, ENV_SIZE_PATH);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "start_cmd_file")))
			{
				strncpy(omci_env_g.start_cmd_file, pValue, ENV_SIZE_PATH);
			}
		}	// global

		// interface config
		if (strcmp(s->type, "interface_config") == 0)
		{
			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "trap_if")))
			{
				strncpy(omci_env_g.trap_if, pValue, ENV_SIZE_IFNAME);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "omcc_if")))
			{
				strncpy(omci_env_g.omcc_if, pValue, ENV_SIZE_IFNAME);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "wifi_if")))
			{
				strncpy(omci_env_g.wifi_if, pValue, ENV_SIZE_IFNAME);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "router_wan_if")))
			{
				strncpy(omci_env_g.router_wan_if, pValue, ENV_SIZE_IFNAME);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "router_lan_if")))
			{
				strncpy(omci_env_g.router_lan_if, pValue, ENV_SIZE_IFNAME);
			}
		}	// interface config

		// connnecting olt simulator
		if (strcmp(s->type, "connecting_olt_simulator") == 0)
		{
			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "sim_olt_ip")))
			{
				strncpy(omci_env_g.sim_olt_ip, pValue, ENV_SIZE_IPSTR);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "sim_olt_udp_port")))
			{
				omci_env_g.sim_olt_udp_port = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "sim_pon_id")))
			{
				omci_env_g.sim_pon_id= util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "sim_onu_id")))
			{
				omci_env_g.sim_onu_id = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "sim_tcont_enable")))
			{
				omci_env_g.sim_tcont_enable = util_atoi((char *)pValue);
			}
		}	// connnecting olt simulator

		// omci msg
		if (strcmp(s->type, "omci_msg") == 0)
		{
			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "omcc_rx_crc_type")))
			{
				omci_env_g.omcc_rx_crc_type = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "omcc_tx_crc_type")))
			{
				omci_env_g.omcc_tx_crc_type = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "omci_tranx_cache_num")))
			{
				omci_env_g.omci_tranx_cache_num = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "omci_tranx_match_mode")))
			{
				omci_env_g.omci_tranx_match_mode = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "omci_tranx_bypass")))
			{
				omci_env_g.omci_tranx_bypass = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "omci_tranx_busy_enable")))
			{
				omci_env_g.omci_tranx_busy_enable = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "omci_history_enable")))
			{
				omci_env_g.omci_history_enable = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "omci_history_size")))
			{
				omci_env_g.omci_history_size = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "omci_history_msgmask")))
			{
				omci_env_g.omci_history_msgmask = util_atoll((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "omci_exec_time_limitation")))
			{
				omci_env_g.omci_exec_time_limitation = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "omci_tx_flag")))
			{
				omci_env_g.omci_tx_flag = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "omci_tx_resend_flag")))
			{
				omci_env_g.omci_tx_resend_flag = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "omcidump_enable")))
			{
				omci_env_g.omcidump_enable = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "omcidump_msgmask")))
			{
				omci_env_g.omcidump_msgmask = util_atoll((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "omccrx_log_enable")))
			{
				omci_env_g.omccrx_log_enable = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "omccrx_log_msgmask")))
			{
				omci_env_g.omccrx_log_msgmask = util_atoll((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "omccrx_log_file")))
			{
				strncpy(omci_env_g.omccrx_log_file, pValue, ENV_SIZE_PATH);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "restore_log_enable")))
			{
				omci_env_g.restore_log_enable = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "restore_log_file")))
			{
				strncpy(omci_env_g.restore_log_file, pValue, ENV_SIZE_PATH);
			}
		}	// omci msg

		// mib
		if (strcmp(s->type, "mib") == 0)
		{
			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "xml_section")))
			{
				memset(omci_env_g.xml_section, 0, sizeof(omci_env_g.xml_section));
				conv_numlist2array((char *)pValue, omci_env_g.xml_section, sizeof(unsigned short), 32);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "xml_load_namestr")))
			{
				omci_env_g.xml_load_namestr = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "mib_upload_order")))
			{
				memset(omci_env_g.mib_upload_order, 0, sizeof(omci_env_g.mib_upload_order));
				conv_numlist2array((char *)pValue, omci_env_g.mib_upload_order, sizeof(unsigned short), ENV_SIZE_MIB_UPLOAD_ORDER);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "mib_boundary_check")))
			{
				omci_env_g.mib_boundary_check = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "mib_pointer_check")))
			{
				omci_env_g.mib_pointer_check = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "mib_readonly_check")))
			{
				omci_env_g.mib_readonly_check = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "mib_rw_mode")))
			{
				omci_env_g.mib_rw_mode = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "mib_aliasid_enable")))
			{
				omci_env_g.mib_aliasid_enable = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "mib_rare_meid_start")))
			{
				omci_env_g.mib_rare_meid_start = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "mib_rare2_meid_start")))
			{
				omci_env_g.mib_rare2_meid_start = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "mib_nonexist_del_err")))
			{
				omci_env_g.mib_nonexist_del_err = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "autouni_enable")))
			{
				omci_env_g.autouni_enable = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "autoveip_enable")))
			{
				omci_env_g.autoveip_enable = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "erdiag_mon_enable")))
			{
				omci_env_g.erdiag_mon_enable = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "mibreset_dump_enable")))
			{
				omci_env_g.mibreset_dump_enable = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "mib_uni_adminlock_default")))
			{
				omci_env_g.mib_uni_adminlock_default = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "adminlock_trigger_opstate")))
			{
				omci_env_g.adminlock_trigger_opstate = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "untag_reserved_vid")))
			{
				omci_env_g.untag_reserved_vid = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "port_forward_operation_default")))
			{
				omci_env_g.port_forward_operation_default = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "pseudo_ts_pointer_enable")))
			{
				omci_env_g.pseudo_ts_pointer_enable = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "invert_weight_enable")))
			{
				omci_env_g.invert_weight_enable = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "invert_priority_enable")))
			{
				omci_env_g.invert_priority_enable = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "pq_tcont_relation")))
			{
				omci_env_g.pq_tcont_relation = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "tcont_map_enable")))
			{
				omci_env_g.tcont_map_enable = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "tcont_map_policy")))
			{
				omci_env_g.tcont_map_enable = util_atoi((char *)pValue);
				env_set_tcont_map_policy((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "tcont_map_ts")))
			{
				memset(omci_env_g.tcont_map_ts, 0, sizeof(omci_env_g.tcont_map_ts));
				conv_numlist2array((char *)pValue, omci_env_g.tcont_map_ts, sizeof(char), ENV_SIZE_TCONT_MAP);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "tcont_map_pq")))
			{
				memset(omci_env_g.tcont_map_pq, 0, sizeof(omci_env_g.tcont_map_pq));
				conv_numlist2array((char *)pValue, omci_env_g.tcont_map_pq, sizeof(char), ENV_SIZE_TCONT_MAP);
			}
		}	// mib

		// sw_download
		if (strcmp(s->type, "sw_download") == 0)
		{
			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "sw_download_end_behavior")))
			{
				omci_env_g.sw_download_end_behavior = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "sw_download_active_always_ok")))
			{
				omci_env_g.sw_download_active_always_ok = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "sw_download_commit_always_ok")))
			{
				omci_env_g.sw_download_commit_always_ok = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "sw_download_window_size")))
			{
				omci_env_g.sw_download_window_size = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "sw_download_script")))
			{
				strncpy(omci_env_g.sw_download_script, pValue, ENV_SIZE_PATH);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "sw_download_image_name")))
			{
				strncpy(omci_env_g.sw_download_image_name, pValue, ENV_SIZE_PATH);
			}
		}	// sw_download

		// thread_tasks
		if (strcmp(s->type, "thread_tasks") == 0)
		{
			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "task_pause_mask")))
			{
				omci_env_g.task_pause_mask= util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "avc_refresh_interval")))
			{
				omci_env_g.avc_refresh_interval= util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "alarm_refresh_interval")))
			{
				omci_env_g.alarm_refresh_interval= util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "pm_refresh_interval")))
			{
				omci_env_g.pm_refresh_interval= util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "pm_swap_interval")))
			{
				omci_env_g.pm_swap_interval= util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "cpuport_history_enable")))
			{
				omci_env_g.cpuport_history_enable= util_atoi((char *)pValue);
			}
			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "cpuport_history_size")))
			{
				omci_env_g.cpuport_history_size= util_atoi((char *)pValue);
			}
			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "cpuport_arp_enable")))
			{
				omci_env_g.cpuport_arp_enable= util_atoi((char *)pValue);
			}
			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "cpuport_pppoe_enable")))
			{
				omci_env_g.cpuport_pppoe_enable= util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "cpuport_pppoe_cust_tag")))
			{
				memset(omci_env_g.cpuport_pppoe_cust_tag, 0x00, sizeof(omci_env_g.cpuport_pppoe_cust_tag));
				conv_numlist2array((char *)pValue, omci_env_g.cpuport_pppoe_cust_tag, sizeof(unsigned char), ENV_SIZE_STRING);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "cpuport_dhcp_enable")))
			{
				omci_env_g.cpuport_dhcp_enable= util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "cpuport_dhcp_cust_tag")))
			{
				memset(omci_env_g.cpuport_dhcp_cust_tag, 0x00, sizeof(omci_env_g.cpuport_dhcp_cust_tag));
				conv_numlist2array((char *)pValue, omci_env_g.cpuport_dhcp_cust_tag, sizeof(unsigned char), ENV_SIZE_STRING);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "cpuport_dhcp_server_onlan")))
			{
				omci_env_g.cpuport_dhcp_server_onlan= util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "cpuport_pkt_ssdp_fwd_wan")))
			{
				omci_env_g.cpuport_pkt_ssdp_fwd_wan= util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "cpuport_im_enable")))
			{
				omci_env_g.cpuport_im_enable= util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "cpuport_im_lan2lan_enable")))
			{
				omci_env_g.cpuport_im_lan2lan_enable= util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "cpuport_wifi_block_mask")))
			{
				omci_env_g.cpuport_wifi_block_mask= util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "cpuport_wifi_block_ra")))
			{
				omci_env_g.cpuport_wifi_block_ra= util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "cpuport_mcast_drop")))
			{
				omci_env_g.cpuport_mcast_drop= util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "stp_enable")))
			{
				omci_env_g.stp_enable= util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "cpu_usage_threshold")))
			{
				omci_env_g.cpu_usage_threshold= util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "cpul_task_no")))
			{
				memset(omci_env_g.cpul_task_no, 0xff, sizeof(omci_env_g.cpul_task_no));
				conv_numlist2array((char *)pValue, omci_env_g.cpul_task_no, sizeof(char), ENV_SIZE_TASK);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "cpul_total_usage")))
			{
				omci_env_g.cpul_total_usage= util_atoi((char *)pValue);
			}
		}	// thread_tasks

		// debug
		if (strcmp(s->type, "debug") == 0)
		{
			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "debug_level")))
			{
				omci_env_g.debug_level = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "debug_level_bat")))
			{
				omci_env_g.debug_level_bat = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "debug_level_cfm")))
			{
				omci_env_g.debug_level_cfm = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "debug_level_cpuport")))
			{
				omci_env_g.debug_level_cpuport = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "debug_level_fwk")))
			{
				omci_env_g.debug_level_fwk = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "debug_level_igmp")))
			{
				omci_env_g.debug_level_igmp = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "debug_level_lldp")))
			{
				omci_env_g.debug_level_lldp = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "debug_level_vacl")))
			{
				omci_env_g.debug_level_vacl = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "debug_level_voip")))
			{
				omci_env_g.debug_level_voip = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "debug_level_xml")))
			{
				omci_env_g.debug_level_xml = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "debug_log_time")))
			{
				omci_env_g.debug_log_time = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "debug_log_type")))
			{
				omci_env_g.debug_log_type = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "debug_log_file")))
			{
				strncpy(omci_env_g.debug_log_file, pValue, ENV_SIZE_PATH);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "memdbg_enable")))
			{
				omci_env_g.memdbg_enable = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "er_group_hw_debug_enable")))
			{
				omci_env_g.er_group_hw_debug_enable = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "er_group_hw_time_enable")))
			{
				omci_env_g.er_group_hw_time_enable = util_atoi((char *)pValue);
			}
		}	// debug


		// olt
		if (strcmp(s->type, "olt") == 0)
		{
			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "olt_vendor_check_enable")))
			{
				omci_env_g.olt_vendor_check_enable = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "olt_vendor_config_change")))
			{
				omci_env_g.olt_vendor_config_change = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "olt_workaround_enable")))
			{
				omci_env_g.olt_workaround_enable = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "olt_alu_mcast_support")))
			{
				omci_env_g.olt_alu_mcast_support = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "olt_huawei_ignore_vlan_drop_rule")))
			{
				omci_env_g.olt_huawei_ignore_vlan_drop_rule = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "olt_proprietary_support")))
			{
				strncpy(omci_env_g.olt_proprietary_support, (char *)pValue, ENV_SIZE_PROPRIETARY_VENDOR);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "ont_force_vendor_id")))
			{
				strncpy(omci_env_g.ont_force_vendor_id, (char *)pValue, 5);
			}
		}	// olt


		// anig
		if (strcmp(s->type, "anig") == 0)
		{
			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "anig_type")))
			{
				omci_env_g.anig_type = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "anig_support")))
			{
				omci_env_g.anig_support = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "anig_txrx_threshold")))
			{
				memset(omci_env_g.anig_txrx_threshold, 0, sizeof(omci_env_g.anig_txrx_threshold));
				conv_numlist2array((char *)pValue, omci_env_g.anig_txrx_threshold, sizeof(char), 4);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "anig_devport")))
			{
				omci_env_g.anig_devport = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "anig_devaddr0")))
			{
				omci_env_g.anig_devaddr0 = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "anig_mon_mode")))
			{
				omci_env_g.anig_mon_mode = util_atoi((char *)pValue);
			}
		}	// anig

		// logical_port_id_mask
		if (strcmp(s->type, "logical_port_id_mask") == 0)
		{
			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "wan_logical_portid")))
			{
				omci_env_g.wan_logical_portid = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "uni_logical_portmask")))
			{
				omci_env_g.uni_logical_portmask = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "cpu_logical_portmask")))
			{
				omci_env_g.cpu_logical_portmask = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "wifi_logical_portmask")))
			{
				omci_env_g.wifi_logical_portmask = util_atoi((char *)pValue);
			}
		}	// logical_port_id_mask

		// switch
		if (strcmp(s->type, "switch") == 0)
		{
			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "ivlan_enable")))
			{
				omci_env_g.ivlan_enable = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "stp_conf_before_omci")))
			{
				omci_env_g.stp_conf_before_omci = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "switch_history_mode")))
			{
				omci_env_g.switch_history_mode = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "switch_history_interval")))
			{
				omci_env_g.switch_history_interval = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "br_allowed_srcmask")))
			{
				omci_env_g.br_allowed_srcmask = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "rt_allowed_srcmask")))
			{
				omci_env_g.rt_allowed_srcmask = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "trunk_group_portmask")))
			{
				omci_env_g.trunk_group_portmask = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "localbridging_default")))
			{
				omci_env_g.localbridging_default = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "localbridging_addon_portmask")))
			{
				omci_env_g.localbridging_addon_portmask = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "ctc_internet_bridge_enabled_srcmask")))
			{
				omci_env_g.ctc_internet_bridge_enabled_srcmask = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "port2port_enable")))
			{
				omci_env_g.port2port_enable = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "maclearn_drop_enable")))
			{
				omci_env_g.maclearn_drop_enable = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "veip_for_wifi_enable")))
			{
				omci_env_g.veip_for_wifi_enable = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "cpuport_ds_passthrough")))
			{
				omci_env_g.cpuport_ds_passthrough = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "rma_forward_mask")))
			{
				omci_env_g.rma_forward_mask = util_atoull((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "bundle_rate_support")))
			{
				omci_env_g.bundle_rate_support = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "linkdown_debounce_time")))
			{
				omci_env_g.linkdown_debounce_time = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "linkup_debounce_time")))
			{
				omci_env_g.linkup_debounce_time = util_atoi((char *)pValue);
			}
		}	// switch


		// rldp
		if (strcmp(s->type, "rldp") == 0)
		{
			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "rldp_enable")))
			{
				omci_env_g.rldp_enable = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "rldp_def_portmask")))
			{
				omci_env_g.rldp_def_portmask = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "rldp_passive_only")))
			{
				omci_env_g.rldp_passive_only = util_atoi((char *)pValue);
			}
		}	// rldp


		// tag_processing
		if (strcmp(s->type, "tag_processing") == 0)
		{
			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "tag_treatment_correct")))
			{
				omci_env_g.tag_treatment_correct = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "tag_discard_reversing")))
			{
				omci_env_g.tag_discard_reversing = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "tag_preview_table")))
			{
				omci_env_g.tag_preview_table = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "tag_auto_reverse")))
			{
				omci_env_g.tag_auto_reverse = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "tag_mc_ds_from_us")))
			{
				omci_env_g.tag_mc_ds_from_us = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "tag_mc_sort_priority")))
			{
				omci_env_g.tag_mc_sort_priority = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "tag_pbit_workaround")))
			{
				omci_env_g.tag_pbit_workaround = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "tag_pbit_portmask")))
			{
				omci_env_g.tag_pbit_portmask = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "tag_pbit_remap_table")))
			{
				memset(omci_env_g.tag_pbit_remap_table, 0, sizeof(omci_env_g.tag_pbit_remap_table));
				conv_numlist2array((char *)pValue, omci_env_g.tag_pbit_remap_table, sizeof(char), 8);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "tag_ds_tci_dont_reverse_workaround")))
			{
				omci_env_g.tag_ds_tci_dont_reverse_workaround = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "tag_insert_65305_rules")))
			{
				omci_env_g.tag_insert_65305_rules = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "tag_ctc_translation_path")))
			{
				strncpy(omci_env_g.tag_ctc_translation_path, (char *)pValue, ENV_SIZE_PATH);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "tag_mc_ds_wildcard")))
			{
				omci_env_g.tag_mc_ds_wildcard = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "tag_l2_rules_reverse")))
			{
				omci_env_g.tag_l2_rules_reverse = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "tag_l2_rules_ds_mac")))
			{
				omci_env_g.tag_l2_rules_ds_mac = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "tag_iphost_vid_map")))
			{
				memset(omci_env_g.tag_iphost_vid_map, 0, sizeof(omci_env_g.tag_iphost_vid_map));
				conv_numlist2array((char *)pValue, omci_env_g.tag_iphost_vid_map, sizeof(unsigned short), ENV_SIZE_IPHOST_PORT);
			}
		}	// tag_processing

		// classf
		if (strcmp(s->type, "classf") == 0)
		{
			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "classf_tag_mode")))
			{
				omci_env_g.classf_tag_mode = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "classf_add_default_rules")))
			{
				omci_env_g.classf_add_default_rules = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "classf_force_stpid")))
			{
				omci_env_g.classf_force_stpid = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "classf_stag_tpid")))
			{
				omci_env_g.classf_stag_tpid = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "classf_hw_stag_tpid")))
			{
				omci_env_g.classf_hw_stag_tpid = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "classf_et_combine")))
			{
				omci_env_g.classf_et_combine = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "classf_ds_gem_dg")))
			{
				omci_env_g.classf_ds_gem_dg = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "classf_dscp2pbit_mode")))
			{
				omci_env_g.classf_dscp2pbit_mode = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "classf_ds_unmatch")))
			{
				omci_env_g.classf_ds_unmatch = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "classf_ds_mc_extend")))
			{
				omci_env_g.classf_ds_mc_extend = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "classf_ds_mc_force_strip")))
			{
				omci_env_g.classf_ds_mc_force_strip = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "classf_veip_rules_reassign")))
			{
				omci_env_g.classf_veip_rules_reassign = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "classf_dropping_rules_add")))
			{
				omci_env_g.classf_dropping_rules_add = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "classf_vmap_pbit_workaround")))
			{
				omci_env_g.classf_vmap_pbit_workaround = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "classf_hw_2tags_mode")))
			{
				omci_env_g.classf_hw_2tags_mode = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "classf_add_ipv4mc_strip")))
			{
				omci_env_g.classf_add_ipv4mc_strip = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "classf_hw_2tags_workaround")))
			{
				omci_env_g.classf_hw_2tags_workaround = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "classf_add_ipv4mc_strip")))
			{
				omci_env_g.classf_add_ipv4mc_strip = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "classf_add_hwnat_1tag_rules")))
			{
				omci_env_g.classf_add_hwnat_1tag_rules = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "classf_add_hwnat_untag_rules")))
			{
				omci_env_g.classf_add_hwnat_untag_rules = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "classf_add_hwnat_untag_vid")))
			{
				omci_env_g.classf_add_hwnat_untag_vid = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "classf_add_hwnat_untag_vid_mask")))
			{
				omci_env_g.classf_add_hwnat_untag_vid_mask = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "classf_hw_rules_migration")))
			{
				omci_env_g.classf_hw_rules_migration = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "classf_hw_colour_force")))
			{
				omci_env_g.classf_hw_colour_force = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "classf_keep_candi")))
			{
				omci_env_g.classf_keep_candi = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "classf_statistic_enable")))
			{
				omci_env_g.classf_statistic_enable = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "classf_tlan_pattern")))
			{
				omci_env_g.classf_tlan_pattern = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "mc_ds_untag_discard_enable")))
			{
				omci_env_g.mc_ds_untag_discard_enable = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "classf_add_default_rules_by_me_84")))
			{
				omci_env_g.classf_add_default_rules_by_me_84 = util_atoi((char *)pValue);
			}
		}	// classf

		// cfm
		if (strcmp(s->type, "cfm") == 0)
		{
			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "cfm_enable")))
			{
				omci_env_g.cfm_enable = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "cfm_y1731_enable")))
			{
				omci_env_g.cfm_y1731_enable = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "cfm_portmask")))
			{
				omci_env_g.cfm_portmask = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "cfm_mac_ifname")))
			{
				strncpy(omci_env_g.cfm_mac_ifname, (char *)pValue, ENV_SIZE_IFNAME);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "cfm_send_timeout")))
			{
				omci_env_g.cfm_send_timeout = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "cfm_reverse_mep_direction")))
			{
				omci_env_g.cfm_reverse_mep_direction = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "cfm_rmep_ccm_before_hold")))
			{
				omci_env_g.cfm_rmep_ccm_before_hold = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "cfm_rmep_holdtime_before_active")))
			{
				omci_env_g.cfm_rmep_holdtime_before_active = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "cfm_rmep_ccm_lost_threshold")))
			{
				omci_env_g.cfm_rmep_ccm_lost_threshold = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "cfm_defect_mask_in_rdi_gen")))
			{
				omci_env_g.cfm_defect_mask_in_rdi_gen = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "cfm_defect_mask_in_ais_gen")))
			{
				omci_env_g.cfm_defect_mask_in_ais_gen = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "cfm_defect_mask_supressed_by_ais")))
			{
				omci_env_g.cfm_defect_mask_supressed_by_ais = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "cfm_defect_mask_supressed_by_lck")))
			{
				omci_env_g.cfm_defect_mask_supressed_by_lck = util_atoi((char *)pValue);
			}
		}	// cfm

		// lldp
		if (strcmp(s->type, "lldp") == 0)
		{
			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "lldp_enable")))
			{
				omci_env_g.lldp_enable = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "lldp_ifname")))
			{
				strncpy(omci_env_g.lldp_ifname, (char *)pValue, ENV_SIZE_IFNAME);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "lldp_dst_mac")))
			{
				omci_env_g.lldp_dst_mac = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "lldp_send_interval")))
			{
				omci_env_g.lldp_send_interval = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "lldp_fast_tx")))
			{
				omci_env_g.lldp_fast_tx = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "lldp_credit_max")))
			{
				omci_env_g.lldp_credit_max = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "lldp_tx_fast_init")))
			{
				omci_env_g.lldp_tx_fast_init = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "lldp_system_name")))
			{
				strncpy(omci_env_g.lldp_system_name, (char *)pValue, ENV_SIZE_STRING);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "lldp_system_desc")))
			{
				strncpy(omci_env_g.lldp_system_desc, (char *)pValue, ENV_SIZE_STRING);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "lldp_med_opt")))
			{
				omci_env_g.lldp_med_opt = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "lldp_med_cap")))
			{
				omci_env_g.lldp_med_cap = util_atoi((char *)pValue);
			}
		}	// lldp

		// extoam
		if (strcmp(s->type, "extoam") == 0)
		{
			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "extoam_enable")))
			{
				omci_env_g.extoam_enable = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "extoam_port")))
			{
				omci_env_g.extoam_port = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "extoam_mac_ifname")))
			{
				strncpy(omci_env_g.extoam_mac_ifname, (char *)pValue, ENV_SIZE_IFNAME);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "extoam_itas_image_name")))
			{
				strncpy(omci_env_g.extoam_itas_image_name, (char *)pValue, 256);
			}
		}	// extoam

		// tm
		if (strcmp(s->type, "tm") == 0)
		{
			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "tm_pq_gem_qos_method")))
			{
				omci_env_g.tm_pq_gem_qos_method = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "tm_gemus_rate_factor")))
			{
				omci_env_g.tm_gemus_rate_factor = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "tm_gemds_rate_factor")))
			{
				omci_env_g.tm_gemds_rate_factor = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "tm_unius_rate_factor")))
			{
				omci_env_g.tm_unius_rate_factor = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "tm_unids_rate_factor")))
			{
				omci_env_g.tm_unids_rate_factor = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "tm_pir_zero_action")))
			{
				omci_env_g.tm_pir_zero_action = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "tm_protect_enable")))
			{
				omci_env_g.tm_protect_enable = util_atoi((char *)pValue);
			}
		}	// tm

		// igmp
		if (strcmp(s->type, "igmp") == 0)
		{
			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "igmp_enable")))
			{
				omci_env_g.igmp_enable = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "igmp_version_overwrite")))
			{
				omci_env_g.igmp_version_overwrite = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "igmp_mcast_me_support")))
			{
				omci_env_g.igmp_mcast_me_support = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "igmp_proxy_snooping")))
			{
				omci_env_g.igmp_proxy_snooping = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "igmp_join_default_grant")))
			{
				omci_env_g.igmp_join_default_grant = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "igmp_query_pass_through")))
			{
				omci_env_g.igmp_query_pass_through = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "igmp_preassign_list")))
			{
				env_set_igmp_preassign_list((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "igmp_v1_ignore")))
			{
				omci_env_g.igmp_v1_ignore = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "igmp_mcast_mode_src")))
			{
				omci_env_g.igmp_mcast_mode_src = util_atoi((char *)pValue);
			}
		}	// igmp

		// voip
		if (strcmp(s->type, "voip") == 0)
		{
			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "voip_enable")))
			{
				omci_env_g.voip_enable = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "voip_debug_set_flag")))
			{
				omci_env_g.voip_debug_set_flag = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "voip_debug_get_flag")))
			{
				omci_env_g.voip_debug_get_flag = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "voip_debug_release_flag")))
			{
				omci_env_g.voip_debug_release_flag = util_atoi((char *)pValue);
			}
		}	// voip

		// misc
		if (strcmp(s->type, "misc") == 0)
		{
			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "batchtab_extevent_idletime")))
			{
				omci_env_g.batchtab_extevent_idletime = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "batchtab_retry_timeout")))
			{
				omci_env_g.batchtab_retry_timeout = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "batchtab_wanif_feature")))
			{
				omci_env_g.batchtab_wanif_feature = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "batchtab_hardbridge_enable")))
			{
				omci_env_g.batchtab_hardbridge_enable = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "exclude_flowid_0")))
			{
				omci_env_g.exclude_flowid_0 = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "avoid_pir_drop")))
			{
				omci_env_g.avoid_pir_drop = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "uni_policing_pbit_mask")))
			{
				omci_env_g.uni_policing_pbit_mask = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "uni_policing_default_bs")))
			{
				omci_env_g.uni_policing_default_bs = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "uni_policing_aggregate_acl")))
			{
				omci_env_g.uni_policing_aggregate_acl = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "uni_linkdown_lock")))
			{
				omci_env_g.uni_linkdown_lock = util_atoi((char *)pValue);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "watchdog_omccmiss_timeout")))
			{
				conv_numlist2array((char *)pValue, omci_env_g.watchdog_omccmiss_timeout, sizeof(unsigned short), 2);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "watchdog_tcontmiss_timeout")))
			{
				conv_numlist2array((char *)pValue, omci_env_g.watchdog_tcontmiss_timeout, sizeof(unsigned short), 2);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "watchdog_o2stuck_timeout")))
			{
				conv_numlist2array((char *)pValue, omci_env_g.watchdog_o2stuck_timeout, sizeof(unsigned short), 2);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "watchdog_idlebw_guard")))
			{
				conv_numlist2array((char *)pValue, omci_env_g.watchdog_idlebw_guard, sizeof(unsigned short), 4);
			}

			if (NULL != (pValue = uci_lookup_option_string(g_omci_uci_context, s, "uni_config_lock")))
			{
				omci_env_g.uni_config_lock = util_atoi((char *)pValue);
			}
		}	// misc
	}
#endif

}

int
omci_config_load_config(void)
{
	int ret = 0;
	char *err;


	g_omci_uci_package = omci_config_init_package("omci");
	if (!g_omci_uci_package) {
		uci_get_errorstr(g_omci_uci_context, &err, NULL);
		dbprintf_bat(LOG_ERR, "Failed to load OMCI config (%s)\n", err);

		free(err);
		return -1;
	}

	omci_config_init_config();

	return ret;
}
