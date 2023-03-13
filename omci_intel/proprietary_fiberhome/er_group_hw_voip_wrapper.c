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
 * Module  : proprietary_fiberhome
 * File    : er_group_hw_voip_wrapper.c
 *
 ******************************************************************/

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <sys/types.h>
#include <signal.h>              
              
#include "voip_hw.h"
#include "util.h"

static void
wri_rtp_resource_name_show(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_wri_rtp_resource_name_t *parm)
{
	dbprintf(LOG_ERR, "===action=%u, port=%u, mask=0x%.2X,0x%.2X\n", action, port, mask[0], mask[1]);
	dbprintf(LOG_ERR, "rnfp=%s, rnvb=%u, rnve=%u, rnvs=%u, rnfl=%u\n",
		parm->rtp_name_fixed_part,
		parm->rtp_name_var_begin,
		parm->rtp_name_var_end,
		parm->rtp_name_var_step,
		parm->rtp_name_fixed_length);
}

int
er_group_hw_voip_wrapper_wri_rtp_resource_name(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_wri_rtp_resource_name_t *parm)
{
	int ret;

	if (action == VOIP_HW_ACTION_SET && omci_env_g.voip_debug_set_flag)
	{
		wri_rtp_resource_name_show(action, port, mask, parm);
	}

	ret = voip_hw_wri_rtp_resource_name(action, port, mask, parm);

	if (action == VOIP_HW_ACTION_GET && omci_env_g.voip_debug_get_flag)
	{
		wri_rtp_resource_name_show(action, port, mask, parm);
	}

	if (action == VOIP_HW_ACTION_RELEASE && omci_env_g.voip_debug_release_flag)
	{
		dbprintf(LOG_ERR, "===action=%u, port=%u, mask=0x%.2X,0x%.2X\n", action, port, mask[0], mask[1]);
	}

	return ret;
}

static void
wri_rfc2833_config_data_show(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_wri_rfc2833_config_data_t *parm)
{
	dbprintf(LOG_ERR, "===action=%u, port=%u, mask=0x%.2X,0x%.2X\n", action, port, mask[0], mask[1]);
	dbprintf(LOG_ERR, "rns=%u, drp=%u, drp1=%u\n",
		parm->rfc2833_nego_state,
		parm->def_rfc2833_pt,
		parm->def_rfc2198_pt);
}

int
er_group_hw_voip_wrapper_wri_rfc2833_config_data(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_wri_rfc2833_config_data_t *parm)
{
	int ret;

	if (action == VOIP_HW_ACTION_SET && omci_env_g.voip_debug_set_flag)
	{
		wri_rfc2833_config_data_show(action, port, mask, parm);
	}

	ret = voip_hw_wri_rfc2833_config_data(action, port, mask, parm);

	if (action == VOIP_HW_ACTION_GET && omci_env_g.voip_debug_get_flag)
	{
		wri_rfc2833_config_data_show(action, port, mask, parm);
	}

	if (action == VOIP_HW_ACTION_RELEASE && omci_env_g.voip_debug_release_flag)
	{
		dbprintf(LOG_ERR, "===action=%u, port=%u, mask=0x%.2X,0x%.2X\n", action, port, mask[0], mask[1]);
	}

	return ret;
}

static void
wri_keepalive_config_data_show(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_wri_keepalive_config_data_t *parm)
{
	dbprintf(LOG_ERR, "===action=%u, port=%u, mask=0x%.2X,0x%.2X\n", action, port, mask[0], mask[1]);
	dbprintf(LOG_ERR, "ke=%u, ki=%u, kt=%u, km=%u, rste=%u\n",
		parm->keepalive_enable,
		parm->keepalive_interva,
		parm->keepalive_times,
		parm->keepalive_mode,
		parm->rereg_start_time_enable);
}

int
er_group_hw_voip_wrapper_wri_keepalive_config_data(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_wri_keepalive_config_data_t *parm)
{
	int ret;

	if (action == VOIP_HW_ACTION_SET && omci_env_g.voip_debug_set_flag)
	{
		wri_keepalive_config_data_show(action, port, mask, parm);
	}

	ret = voip_hw_wri_keepalive_config_data(action, port, mask, parm);

	if (action == VOIP_HW_ACTION_GET && omci_env_g.voip_debug_get_flag)
	{
		wri_keepalive_config_data_show(action, port, mask, parm);
	}

	if (action == VOIP_HW_ACTION_RELEASE && omci_env_g.voip_debug_release_flag)
	{
		dbprintf(LOG_ERR, "===action=%u, port=%u, mask=0x%.2X,0x%.2X\n", action, port, mask[0], mask[1]);
	}

	return ret;
}

static void
wri_pos_config_data_show(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_wri_pos_config_data_t *parm)
{
	int i;
	struct voip_hw_pos_data_entry_t *pos_data_entry;

	dbprintf(LOG_ERR, "===action=%u, port=%u, mask=0x%.2X,0x%.2X\n", action, port, mask[0], mask[1]);
	dbprintf(LOG_ERR, "in=%u\n",
		parm->item_number);

	dbprintf(LOG_ERR, "table: \n");
	i = 0;
	list_for_each_entry(pos_data_entry, &parm->pos_data_list, pos_data_node)
	{
		dbprintf(LOG_ERR, "     entry(%d):id(%u),tel_num(%s), pos_type(%u)\n", i++, pos_data_entry->id, pos_data_entry->tel_num, pos_data_entry->pos_type);
	}
}

int
er_group_hw_voip_wrapper_wri_pos_config_data(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_wri_pos_config_data_t *parm)
{
	int ret;

	if (action == VOIP_HW_ACTION_SET && omci_env_g.voip_debug_set_flag)
	{
		wri_pos_config_data_show(action, port, mask, parm);
	}

	ret = voip_hw_wri_pos_config_data(action, port, mask, parm);

	if (action == VOIP_HW_ACTION_GET && omci_env_g.voip_debug_get_flag)
	{
		wri_pos_config_data_show(action, port, mask, parm);
	}

	if (action == VOIP_HW_ACTION_RELEASE && omci_env_g.voip_debug_release_flag)
	{
		dbprintf(LOG_ERR, "===action=%u, port=%u, mask=0x%.2X,0x%.2X\n", action, port, mask[0], mask[1]);
	}

	return ret;
}

static void
wri_ipt_config_data_show(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_wri_ipt_config_data_t *parm)
{
	int i;
	struct voip_hw_pos_data_entry_t *pos_data_entry;

	dbprintf(LOG_ERR, "===action=%u, port=%u, mask=0x%.2X,0x%.2X\n", action, port, mask[0], mask[1]);
	dbprintf(LOG_ERR, "in=%u\n",
		parm->item_number);

	dbprintf(LOG_ERR, "table: \n");
	i = 0;
	list_for_each_entry(pos_data_entry, &parm->ipt_data_list, pos_data_node)
	{
		dbprintf(LOG_ERR, "     entry(%d):id(%u),tel_num(%s), pos_type(%u)\n", i++, pos_data_entry->id, pos_data_entry->tel_num, pos_data_entry->pos_type);
	}
}

int
er_group_hw_voip_wrapper_wri_ipt_config_data(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_wri_ipt_config_data_t *parm)
{
	int ret;

	if (action == VOIP_HW_ACTION_SET && omci_env_g.voip_debug_set_flag)
	{
		wri_ipt_config_data_show(action, port, mask, parm);
	}

	ret = voip_hw_wri_ipt_config_data(action, port, mask, parm);

	if (action == VOIP_HW_ACTION_GET && omci_env_g.voip_debug_get_flag)
	{
		wri_ipt_config_data_show(action, port, mask, parm);
	}

	if (action == VOIP_HW_ACTION_RELEASE && omci_env_g.voip_debug_release_flag)
	{
		dbprintf(LOG_ERR, "===action=%u, port=%u, mask=0x%.2X,0x%.2X\n", action, port, mask[0], mask[1]);
	}

	return ret;
}

static void
wri_mg_port_register_control_show(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_wri_mg_port_register_control_t *parm)
{
	dbprintf(LOG_ERR, "===action=%u, port=%u, mask=0x%.2X,0x%.2X\n", action, port, mask[0], mask[1]);
	dbprintf(LOG_ERR, "mip=%s, ma=%u, pa=%u\n",
		parm->mg_ip_pointer,
		parm->mg_act,
		parm->port_act);
}

int
er_group_hw_voip_wrapper_wri_mg_port_register_control(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_wri_mg_port_register_control_t *parm)
{
	int ret;

	if (action == VOIP_HW_ACTION_SET && omci_env_g.voip_debug_set_flag)
	{
		wri_mg_port_register_control_show(action, port, mask, parm);
	}

	ret = voip_hw_wri_mg_port_register_control(action, port, mask, parm);

	if (action == VOIP_HW_ACTION_GET && omci_env_g.voip_debug_get_flag)
	{
		wri_mg_port_register_control_show(action, port, mask, parm);
	}

	if (action == VOIP_HW_ACTION_RELEASE && omci_env_g.voip_debug_release_flag)
	{
		dbprintf(LOG_ERR, "===action=%u, port=%u, mask=0x%.2X,0x%.2X\n", action, port, mask[0], mask[1]);
	}

	return ret;
}

static void
wri_digit_map_match_mode_show(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_wri_digit_map_match_mode_t *parm)
{
	dbprintf(LOG_ERR, "===action=%u, port=%u, mask=0x%.2X,0x%.2X\n", action, port, mask[0], mask[1]);
	dbprintf(LOG_ERR, "nmm=%u\n",
		parm->notify_match_mode);
}

int
er_group_hw_voip_wrapper_wri_digit_map_match_mode(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_wri_digit_map_match_mode_t *parm)
{
	int ret;

	if (action == VOIP_HW_ACTION_SET && omci_env_g.voip_debug_set_flag)
	{
		wri_digit_map_match_mode_show(action, port, mask, parm);
	}

	ret = voip_hw_wri_digit_map_match_mode(action, port, mask, parm);

	if (action == VOIP_HW_ACTION_GET && omci_env_g.voip_debug_get_flag)
	{
		wri_digit_map_match_mode_show(action, port, mask, parm);
	}

	if (action == VOIP_HW_ACTION_RELEASE && omci_env_g.voip_debug_release_flag)
	{
		dbprintf(LOG_ERR, "===action=%u, port=%u, mask=0x%.2X,0x%.2X\n", action, port, mask[0], mask[1]);
	}

	return ret;
}

static void
wri_fax_config_data_show(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_wri_fax_config_data_t *parm)
{
	dbprintf(LOG_ERR, "===action=%u, port=%u, mask=0x%.2X,0x%.2X\n", action, port, mask[0], mask[1]);
	dbprintf(LOG_ERR, "fcm=%u, vs=%u, vti=%u, vri=%u, vvc=%d, tedm=%u, dm=%u\n",
		parm->fax_control_mode,
		parm->vbd_state,
		parm->vbd_tx_interval,
		parm->vbd_rx_interval,
		parm->vbd_voice_coder,
		parm->t38_event_detect_mode,
		parm->dtmf_mode);
}

int
er_group_hw_voip_wrapper_wri_fax_config_data(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_wri_fax_config_data_t *parm)
{
	int ret;

	if (action == VOIP_HW_ACTION_SET && omci_env_g.voip_debug_set_flag)
	{
		wri_fax_config_data_show(action, port, mask, parm);
	}

	ret = voip_hw_wri_fax_config_data(action, port, mask, parm);

	if (action == VOIP_HW_ACTION_GET && omci_env_g.voip_debug_get_flag)
	{
		wri_fax_config_data_show(action, port, mask, parm);
	}

	if (action == VOIP_HW_ACTION_RELEASE && omci_env_g.voip_debug_release_flag)
	{
		dbprintf(LOG_ERR, "===action=%u, port=%u, mask=0x%.2X,0x%.2X\n", action, port, mask[0], mask[1]);
	}

	return ret;
}

static void
wri_ngn_resource_info_show(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_wri_ngn_resource_info_t *parm)
{
	dbprintf(LOG_ERR, "===action=%u, port=%u, mask=0x%.2X,0x%.2X\n", action, port, mask[0], mask[1]);
	dbprintf(LOG_ERR, "rnrp=%u, snrp=%u, rnrp=%u, snrp=%u, rrp=%u, srp=%u, rrb=%u, srb=%u,\npl=%u, nad=%u, rajb=%u, bu=%u, lsp=%u, rp=%u, wsp=%u, usp=%u\n",
		parm->received_ngn_req_packets,
		parm->send_ngn_req_packets,
		parm->received_ngn_resp_packets,
		parm->send_ngn_resp_packets,
		parm->received_rtp_packets,
		parm->send_rtp_packets,
		parm->received_rtp_bytes,
		parm->send_rtp_bytes,
		parm->packet_loss,
		parm->network_avg_delay,
		parm->rtp_avg_jitter_buffer,
		parm->bandwidth_usage,
		parm->lost_sig_packets,
		parm->retransmit_packets,
		parm->wrong_sig_packets,
		parm->unknown_sig_packets
		);
}

int
er_group_hw_voip_wrapper_wri_ngn_resource_info(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_wri_ngn_resource_info_t *parm)
{
	int ret;

	if (action == VOIP_HW_ACTION_SET && omci_env_g.voip_debug_set_flag)
	{
		wri_ngn_resource_info_show(action, port, mask, parm);
	}

	ret = voip_hw_wri_ngn_resource_info(action, port, mask, parm);

	if (action == VOIP_HW_ACTION_GET && omci_env_g.voip_debug_get_flag)
	{
		wri_ngn_resource_info_show(action, port, mask, parm);
	}

	if (action == VOIP_HW_ACTION_RELEASE && omci_env_g.voip_debug_release_flag)
	{
		dbprintf(LOG_ERR, "===action=%u, port=%u, mask=0x%.2X,0x%.2X\n", action, port, mask[0], mask[1]);
	}

	return ret;
}

static void
wri_ngn_port_statics_show(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_wri_ngn_port_statics_t *parm)
{
	dbprintf(LOG_ERR, "===action=%u, port=%u, mask=0x%.2X,0x%.2X\n", action, port, mask[0], mask[1]);
	dbprintf(LOG_ERR, "rrp=%u, srp=%u, rrb=%u, srb=%u, rjb=%u,\n",
		parm->received_rtp_packets,
		parm->send_rtp_packets,
		parm->received_rtp_bytes,
		parm->send_rtp_bytes,
		parm->rtp_jitter_buffer);
	dbprintf(LOG_ERR, "lcbt:y=%u, m=%u, d=%u, m=%u, s=%u,\n",
		parm->latest_call_begin_time.year,
		parm->latest_call_begin_time.month,
		parm->latest_call_begin_time.day,
		parm->latest_call_begin_time.hour,
		parm->latest_call_begin_time.minute,
		parm->latest_call_begin_time.second);
	dbprintf(LOG_ERR, "lcet:y=%u, m=%u, d=%u, m=%u, s=%u,\n",
		parm->latest_call_end_time.year,
		parm->latest_call_end_time.month,
		parm->latest_call_end_time.day,
		parm->latest_call_end_time.hour,
		parm->latest_call_end_time.minute,
		parm->latest_call_end_time.second);
	dbprintf(LOG_ERR, "lcd=%u, tct=%u, tcd=%u, ur=%u, dr=%u, and=%u, pl=%u\n",
		parm->last_call_duration,
		parm->total_call_times,
		parm->total_call_duration,
		parm->upstream_rates,
		parm->downstream_rates,
		parm->avg_network_delay,
		parm->packet_loss);
}

int
er_group_hw_voip_wrapper_wri_ngn_port_statics(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_wri_ngn_port_statics_t *parm)
{
	int ret;

	if (action == VOIP_HW_ACTION_SET && omci_env_g.voip_debug_set_flag)
	{
		wri_ngn_port_statics_show(action, port, mask, parm);
	}

	ret = voip_hw_wri_ngn_port_statics(action, port, mask, parm);

	if (action == VOIP_HW_ACTION_GET && omci_env_g.voip_debug_get_flag)
	{
		wri_ngn_port_statics_show(action, port, mask, parm);
	}

	if (action == VOIP_HW_ACTION_RELEASE && omci_env_g.voip_debug_release_flag)
	{
		dbprintf(LOG_ERR, "===action=%u, port=%u, mask=0x%.2X,0x%.2X\n", action, port, mask[0], mask[1]);
	}

	return ret;
}

static void
wri_sip_config_show(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_wri_sip_config_t *parm)
{
	dbprintf(LOG_ERR, "===action=%u, port=%u, mask=0x%.2X,0x%.2X\n", action, port, mask[0], mask[1]);
	dbprintf(LOG_ERR, "ssrsap=%s, spsap=%s, frp=%u, fpp=%u, srp=%u, spp=%u, csrp=%s\n",
		parm->secondary_sip_registrar_server_address_pointer,
		parm->secondary_proxy_server_address_pointer,
		parm->first_registrar_port,
		parm->first_proxy_port,
		parm->secondary_registrar_port,
		parm->secondary_proxy_port,
		parm->current_sip_registrar_pointer);
}

int
er_group_hw_voip_wrapper_wri_sip_config(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_wri_sip_config_t *parm)
{
	int ret;

	if (action == VOIP_HW_ACTION_SET && omci_env_g.voip_debug_set_flag)
	{
		wri_sip_config_show(action, port, mask, parm);
	}

	ret = voip_hw_wri_sip_config(action, port, mask, parm);

	if (action == VOIP_HW_ACTION_GET && omci_env_g.voip_debug_get_flag)
	{
		wri_sip_config_show(action, port, mask, parm);
	}

	if (action == VOIP_HW_ACTION_RELEASE && omci_env_g.voip_debug_release_flag)
	{
		dbprintf(LOG_ERR, "===action=%u, port=%u, mask=0x%.2X,0x%.2X\n", action, port, mask[0], mask[1]);
	}

	return ret;
}

static void
wri_softswitch_progile_config_show(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_wri_softswitch_progile_config_t *parm)
{
	dbprintf(LOG_ERR, "===action=%u, port=%u, mask=0x%.2X,0x%.2X\n", action, port, mask[0], mask[1]);
	dbprintf(LOG_ERR, "dtt=%u, ntt=%u, btt=%u, rt=%u, rt=%u\n",
		parm->dail_tone_timeout,
		parm->noanswer_tone_timeout,
		parm->busy_tone_timeout,
		parm->roh_timeout,
		parm->retransmit_timeout);
}

int
er_group_hw_voip_wrapper_wri_softswitch_progile_config(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_wri_softswitch_progile_config_t *parm)
{
	int ret;

	if (action == VOIP_HW_ACTION_SET && omci_env_g.voip_debug_set_flag)
	{
		wri_softswitch_progile_config_show(action, port, mask, parm);
	}

	ret = voip_hw_wri_softswitch_progile_config(action, port, mask, parm);

	if (action == VOIP_HW_ACTION_GET && omci_env_g.voip_debug_get_flag)
	{
		wri_softswitch_progile_config_show(action, port, mask, parm);
	}

	if (action == VOIP_HW_ACTION_RELEASE && omci_env_g.voip_debug_release_flag)
	{
		dbprintf(LOG_ERR, "===action=%u, port=%u, mask=0x%.2X,0x%.2X\n", action, port, mask[0], mask[1]);
	}

	return ret;
}
