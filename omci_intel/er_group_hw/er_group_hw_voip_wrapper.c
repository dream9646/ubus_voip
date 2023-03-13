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
 * Module  : er_group_hw
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
#include "util_run.h"

static void
physical_path_termination_point_pots_uni_show(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_physical_path_termination_point_pots_uni_t *parm)
{
	dbprintf(LOG_ERR, "===action=%u, port=%u, mask=0x%.2X,0x%.2X\n", action, port, mask[0], mask[1]);
	dbprintf(LOG_ERR, "as=%u, arc=%u, arc_i=%u, im=%u, trans=%u, rxg=%d, txg=%d, os=%u, hook=%u, pots=%u\n",
		parm->administrative_state,
		parm->arc,
		parm->arc_interval,
		parm->impedance,
		parm->transmission_path,
		parm->rx_gain,  // -120..60
		parm->tx_gain,  // -120..60
		parm->operational_state,
		parm->hook_state,
		parm->pots_holdover_time);
}

int
er_group_hw_voip_wrapper_physical_path_termination_point_pots_uni(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_physical_path_termination_point_pots_uni_t *parm)
{
	int ret;

	if (action == VOIP_HW_ACTION_SET && omci_env_g.voip_debug_set_flag)
	{
		physical_path_termination_point_pots_uni_show(action, port, mask, parm);
	}

	ret = voip_hw_physical_path_termination_point_pots_uni(action, port, mask, parm);

	if (action == VOIP_HW_ACTION_GET && omci_env_g.voip_debug_get_flag)
	{
		physical_path_termination_point_pots_uni_show(action, port, mask, parm);
	}

	if (action == VOIP_HW_ACTION_RELEASE && omci_env_g.voip_debug_release_flag)
	{
		dbprintf(LOG_ERR, "===action=%u, port=%u, mask=0x%.2X,0x%.2X\n", action, port, mask[0], mask[1]);
	}

	return ret;
}

static void
sip_user_data_show(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_sip_user_data_t *parm)
{
	dbprintf(LOG_ERR, "===action=%u, port=%u, mask=0x%.2X,0x%.2X\n", action, port, mask[0], mask[1]);
	dbprintf(LOG_ERR, "upa=%s, dn=%s\n",
		parm->user_part_aor != NULL? parm->user_part_aor: NULL,
		parm->sip_display_name);

	if (parm->username_password != NULL)
	{
		dbprintf(LOG_ERR, "up.vs=%u, up.un1=%s, up.pw=%s, up.re=%s, up.un2=%s\n",
			parm->username_password->validation_scheme,
			parm->username_password->username_1,
			parm->username_password->password,
			parm->username_password->realm,
			parm->username_password->username_2);
	} else {
		dbprintf(LOG_ERR, "up=null\n");
	}

	if (parm->voicemail_server_sip_uri != NULL && parm->voicemail_server_sip_uri->security_pointer != NULL)
	{
		dbprintf(LOG_ERR, "vs.se.vs=%u, vs.se.un1=%s, vs.se.pw=%s, vs.se.re=%s, vs.se.un2=%s\n",
		parm->voicemail_server_sip_uri->security_pointer->validation_scheme,
		parm->voicemail_server_sip_uri->security_pointer->username_1,
		parm->voicemail_server_sip_uri->security_pointer->password,
		parm->voicemail_server_sip_uri->security_pointer->realm,
		parm->voicemail_server_sip_uri->security_pointer->username_2);
	} else {
		dbprintf(LOG_ERR, "vs.se=null\n");
	}

	if (parm->voicemail_server_sip_uri != NULL && parm->voicemail_server_sip_uri->address_pointer != NULL)
	{
		dbprintf(LOG_ERR, "vs.ad=%s\n", parm->voicemail_server_sip_uri->address_pointer);
	} else {
		dbprintf(LOG_ERR, "vs.ad=null\n");
	}

	dbprintf(LOG_ERR, "vset=%u, rt=%u, roh=%u\n",
		parm->voicemail_subscription_expiration_time,
		parm->release_timer,
		parm->roh_timer);
}

int
er_group_hw_voip_wrapper_sip_user_data(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_sip_user_data_t *parm)
{
	int ret;

	if (action == VOIP_HW_ACTION_SET && omci_env_g.voip_debug_set_flag)
	{
		sip_user_data_show(action, port, mask, parm);
	}

	ret = voip_hw_sip_user_data(action, port, mask, parm);

	if (action == VOIP_HW_ACTION_GET && omci_env_g.voip_debug_get_flag)
	{
		sip_user_data_show(action, port, mask, parm);
	}

	if (action == VOIP_HW_ACTION_RELEASE && omci_env_g.voip_debug_release_flag)
	{
		dbprintf(LOG_ERR, "===action=%u, port=%u, mask=0x%.2X,0x%.2X\n", action, port, mask[0], mask[1]);
	}

	return ret;
}

static void sip_agent_config_data_show(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_sip_agent_config_data_t *parm)
{
	struct in_addr addr1, addr2, addr3, addr4, addr5, addr6, addr7, addr8, addr9, addr10;

	addr1.s_addr = htonl(parm->primary_sip_dns);
	addr2.s_addr = htonl(parm->secondary_sip_dns);

	dbprintf(LOG_ERR, "===action=%u, port=%u, mask=0x%.2X,0x%.2X\n", action, port, mask[0], mask[1]);
	dbprintf(LOG_ERR, "ps=%s, op=%s, psd=%s\n",
	parm->proxy_server_address_pointer != NULL? parm->proxy_server_address_pointer: NULL,
		parm->outbound_proxy_address_pointer != NULL? parm->outbound_proxy_address_pointer: NULL,
		inet_ntoa(addr1));

	if (parm->tcp_udp_pointer != NULL)
	{
		dbprintf(LOG_ERR, "tu.pi=%u, tu.p=%u, tu.td=%u\n",
			parm->tcp_udp_pointer->port_id,
			parm->tcp_udp_pointer->protocol,
			parm->tcp_udp_pointer->tos_diffserv_field);
	} else {
		dbprintf(LOG_ERR, "tu=null\n");
	}

	if (parm->tcp_udp_pointer != NULL && parm->tcp_udp_pointer->ip_host_pointer != NULL)
	{
		addr1.s_addr = htonl(parm->tcp_udp_pointer->ip_host_pointer->ip_address);
		addr2.s_addr = htonl(parm->tcp_udp_pointer->ip_host_pointer->mask);
		addr3.s_addr = htonl(parm->tcp_udp_pointer->ip_host_pointer->gateway);
		addr4.s_addr = htonl(parm->tcp_udp_pointer->ip_host_pointer->primary_dns);
		addr5.s_addr = htonl(parm->tcp_udp_pointer->ip_host_pointer->secondary_dns);
		addr6.s_addr = htonl(parm->tcp_udp_pointer->ip_host_pointer->current_address);
		addr7.s_addr = htonl(parm->tcp_udp_pointer->ip_host_pointer->current_mask);
		addr8.s_addr = htonl(parm->tcp_udp_pointer->ip_host_pointer->current_gateway);
		addr9.s_addr = htonl(parm->tcp_udp_pointer->ip_host_pointer->current_paimary_dns);
		addr10.s_addr = htonl(parm->tcp_udp_pointer->ip_host_pointer->current_secondary_dns);

		dbprintf(LOG_ERR, "tu.ih.io=%u, tu.ih.mac=%.2x:%.2x:%.2x:%.2x:%.2x:%.2x, tu.ih.oi=%s\n",
			parm->tcp_udp_pointer->ip_host_pointer->ip_options,
			parm->tcp_udp_pointer->ip_host_pointer->mac_address[0],
			parm->tcp_udp_pointer->ip_host_pointer->mac_address[1],
			parm->tcp_udp_pointer->ip_host_pointer->mac_address[2],
			parm->tcp_udp_pointer->ip_host_pointer->mac_address[3],
			parm->tcp_udp_pointer->ip_host_pointer->mac_address[4],
			parm->tcp_udp_pointer->ip_host_pointer->mac_address[5],
			parm->tcp_udp_pointer->ip_host_pointer->ont_identifier);
		dbprintf(LOG_ERR, ", tu.ih.ia=%s\n",inet_ntoa(addr1));
		dbprintf(LOG_ERR, ", tu.ih.mask=%s\n",inet_ntoa(addr2));
		dbprintf(LOG_ERR, ", tu.ih.gate=%s\n",inet_ntoa(addr3));
		dbprintf(LOG_ERR, ", tu.ih.pd=%s\n",inet_ntoa(addr4));
		dbprintf(LOG_ERR, ", tu.ih.sd=%s\n",inet_ntoa(addr5));
		dbprintf(LOG_ERR, ", tu.ih.ca=%s\n",inet_ntoa(addr6));
		dbprintf(LOG_ERR, ", tu.ih.cm=%s\n",inet_ntoa(addr7));
		dbprintf(LOG_ERR, ", tu.ih.cg=%s\n",inet_ntoa(addr8));
		dbprintf(LOG_ERR, ", tu.ih.cpd=%s\n",inet_ntoa(addr9));
		dbprintf(LOG_ERR, ", tu.ih.csd=%s\n",inet_ntoa(addr10));
		dbprintf(LOG_ERR, ", tu.ih.dn=%s\n",parm->tcp_udp_pointer->ip_host_pointer->doman_name);
		dbprintf(LOG_ERR, ", tu.ih.hn=%s\n",parm->tcp_udp_pointer->ip_host_pointer->host_name);	
	} else {
		dbprintf(LOG_ERR, "tu.ih=null\n");
	}

	dbprintf(LOG_ERR, "sre=%u, srh=%u, hp=%s, ss=%u, s=%s\n",
		parm->sip_reg_exp_time,
		parm->sip_rereg_head_start_time,
		parm->host_part_uri != NULL? parm->host_part_uri: NULL,
		parm->sip_status,
		parm->softswitch);

	if (parm->sip_registrar != NULL && parm->sip_registrar->security_pointer != NULL)
	{
		dbprintf(LOG_ERR, "sr.se.vs=%u, sr.se.un1=%s, sr.se.pw=%s, sr.se.re=%s, sr.se.un2=%s\n",
			parm->sip_registrar->security_pointer->validation_scheme,
			parm->sip_registrar->security_pointer->username_1,
			parm->sip_registrar->security_pointer->password,
			parm->sip_registrar->security_pointer->realm,
			parm->sip_registrar->security_pointer->username_2);
	} else {
		dbprintf(LOG_ERR, "sr.se=null\n");
	}

	if (parm->sip_registrar != NULL && parm->sip_registrar->address_pointer != NULL)
	{
		dbprintf(LOG_ERR, "sr.ad=%s\n", parm->sip_registrar->address_pointer);
	} else {
		dbprintf(LOG_ERR, "sr.ad=null\n");
	}
}

int
er_group_hw_voip_wrapper_sip_agent_config_data(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_sip_agent_config_data_t *parm)
{
	int ret;

	if (action == VOIP_HW_ACTION_SET && omci_env_g.voip_debug_set_flag)
	{
		sip_agent_config_data_show(action, port, mask, parm);
	}

	ret = voip_hw_sip_agent_config_data(action, port, mask, parm);

	if (action == VOIP_HW_ACTION_GET && omci_env_g.voip_debug_get_flag)
	{
		sip_agent_config_data_show(action, port, mask, parm);
	}

	if (action == VOIP_HW_ACTION_RELEASE && omci_env_g.voip_debug_release_flag)
	{
		dbprintf(LOG_ERR, "===action=%u, port=%u, mask=0x%.2X,0x%.2X\n", action, port, mask[0], mask[1]);
	}

	return ret;
}

static void
voip_voice_ctp_show(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_voip_voice_ctp_t *parm)
{
	dbprintf(LOG_ERR, "===action=%u, port=%u, mask=0x%.2X,0x%.2X\n", action, port, mask[0], mask[1]);
	dbprintf(LOG_ERR, "sc=%u\n", parm->signalling_code);
}

int
er_group_hw_voip_wrapper_voip_voice_ctp(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_voip_voice_ctp_t *parm)
{
	int ret;

	if (action == VOIP_HW_ACTION_SET && omci_env_g.voip_debug_set_flag)
	{
		voip_voice_ctp_show(action, port, mask, parm);
	}

	ret = voip_hw_voip_voice_ctp(action, port, mask, parm);

	if (action == VOIP_HW_ACTION_GET && omci_env_g.voip_debug_get_flag)
	{
		voip_voice_ctp_show(action, port, mask, parm);
	}

	if (action == VOIP_HW_ACTION_RELEASE && omci_env_g.voip_debug_release_flag)
	{
		dbprintf(LOG_ERR, "===action=%u, port=%u, mask=0x%.2X,0x%.2X\n", action, port, mask[0], mask[1]);
	}

	return ret;
}

static void
voip_media_profile_show(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_voip_media_profile_t *parm)
{
	dbprintf(LOG_ERR, "===action=%u, port=%u, mask=0x%.2X,0x%.2X\n", action, port, mask[0], mask[1]);
	dbprintf(LOG_ERR, "fm=%u, cs1=%u, pp1=%u, ss1=%u, cs2=%u, pp2=%u, ss2=%u, cs3=%u, pp3=%u, ss3=%u, cs4=%u, pp4=%u, ss4=%u, oob=%u\n",
		parm->fax_mode,
		parm->codec_selection_1st_order,
		parm->packet_period_selection_1st_order,
		parm->silence_suppression_1st_order,
		parm->codec_selection_2nd_order,
		parm->packet_period_selection_2nd_order,
		parm->silence_suppression_2nd_order,
		parm->codec_selection_3rd_order,
		parm->packet_period_selection_3rd_order,
		parm->silence_suppression_3rd_order,
		parm->codec_selection_4th_order,
		parm->packet_period_selection_4th_order,
		parm->silence_suppression_4th_order,
		parm->oob_dtmf);
}

int
er_group_hw_voip_wrapper_voip_media_profile(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_voip_media_profile_t *parm)
{
	int ret;

	if (action == VOIP_HW_ACTION_SET && omci_env_g.voip_debug_set_flag)
	{
		voip_media_profile_show(action, port, mask, parm);
	}

	ret = voip_hw_voip_media_profile(action, port, mask, parm);

	if (action == VOIP_HW_ACTION_GET && omci_env_g.voip_debug_get_flag)
	{
		voip_media_profile_show(action, port, mask, parm);
	}

	if (action == VOIP_HW_ACTION_RELEASE && omci_env_g.voip_debug_release_flag)
	{
		dbprintf(LOG_ERR, "===action=%u, port=%u, mask=0x%.2X,0x%.2X\n", action, port, mask[0], mask[1]);
	}

	return ret;
}

static void
voice_service_profile_show(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_voice_service_profile_t *parm)
{
	int i;
	struct voip_hw_tone_pattern_entry_t *tp_entry;
	struct voip_hw_tone_event_entry_t *te_entry;
	struct voip_hw_ringing_pattern_entry_t *rp_entry;
	struct voip_hw_ringing_event_entry_t *re_entry;

	dbprintf(LOG_ERR, "===action=%u, port=%u, mask=0x%.2X,0x%.2X\n", action, port, mask[0], mask[1]);
	dbprintf(LOG_ERR, "at=%u, jt=%u, jbm=%u, eci=%u, ppv=%u, ddl=%u, ddd=%u, hfmin=%u, hfmax=%u\n",
		parm->announcement_type,
		parm->jitter_target,
		parm->jitter_buffer_max,
		parm->echo_cancel_ind,
		parm->pstn_protocol_variant,
		parm->dtmf_digit_levels,
		parm->dtmf_digit_duration,
		parm->hook_flash_minimum_time,
		parm->hook_flash_maximum_time);
	
	dbprintf(LOG_ERR, "tone pattern table: \n");
	i = 0;
	list_for_each_entry(tp_entry, &parm->tone_pattern_list, tone_pattern_node)
	{
		dbprintf(LOG_ERR, "     entry(%d):i=%u,ton=%u,f1=%u,p1=%u,f2=%u,p2=%u,f3=%u,p3=%u,f4=%u,p4=%u,mf=%u,mp=%u,du=%u,ne=%u\n",
			i++, 
			tp_entry->index,
			tp_entry->tone_on,
			tp_entry->frequency_1,
			tp_entry->power_1,
			tp_entry->frequency_2,
			tp_entry->power_2,
			tp_entry->frequency_3,
			tp_entry->power_3,
			tp_entry->frequency_4,
			tp_entry->power_4,
			tp_entry->modulation_frequency,
			tp_entry->modulation_power,
			tp_entry->duration,
			tp_entry->next_entry
		);
	}

	dbprintf(LOG_ERR, "tone event table: \n");
	i = 0;
	list_for_each_entry(te_entry, &parm->tone_event_list, tone_event_node)
	{
		dbprintf(LOG_ERR, "     entry(%d):e=%u,tp=%u,tf=%s,tfr=%u,re=%u\n",
			i++, 
			te_entry->event,
			te_entry->tone_pattern,
			(te_entry->tone_file != NULL) ? te_entry->tone_file : NULL ,
			te_entry->tone_file_repetitions,
			te_entry->reserved
		);
	}

	dbprintf(LOG_ERR, "ringing pattern table: \n");
	i = 0;
	list_for_each_entry(rp_entry, &parm->ringing_pattern_list, ringing_pattern_node)
	{
		dbprintf(LOG_ERR, "     entry(%d):i=%u,ton=%u,du=%u,ne=%u\n",
			i++, 
			rp_entry->index,
			rp_entry->ringing_on,
			rp_entry->duration,
			rp_entry->next_entry
		);
	}

	dbprintf(LOG_ERR, "ringing event table: \n");
	i = 0;
	list_for_each_entry(re_entry, &parm->ringing_event_list, ringing_event_node)
	{
		dbprintf(LOG_ERR, "     entry(%d):e=%u,tp=%u,tf=%s,tfr=%u,rt=%s\n",
			i++,
			re_entry->event,
			re_entry->ringing_pattern,
			(re_entry->ringing_file != NULL) ? re_entry->ringing_file : NULL ,
			re_entry->ringing_file_repetitions,
			(re_entry->ringing_text != NULL) ? re_entry->ringing_text : NULL
		);
	}

	if (parm->network_specific_extensions != NULL && parm->network_specific_extensions->security_pointer != NULL)
	{
		dbprintf(LOG_ERR, "nse.se.vs=%u, nse.se.un1=%s, nse.se.pw=%s, nse.se.re=%s, nse.se.un2=%s\n",
			parm->network_specific_extensions->security_pointer->validation_scheme,
			parm->network_specific_extensions->security_pointer->username_1,
			parm->network_specific_extensions->security_pointer->password,
			parm->network_specific_extensions->security_pointer->realm,
			parm->network_specific_extensions->security_pointer->username_2);
	} else {
		dbprintf(LOG_ERR, "nse.se=null\n");
	}

	if (parm->network_specific_extensions != NULL && parm->network_specific_extensions->address_pointer != NULL)
	{
		dbprintf(LOG_ERR, "nse.ad=%s\n", parm->network_specific_extensions->address_pointer);
	} else {
		dbprintf(LOG_ERR, "nse.ad=null\n");
	}
}

int
er_group_hw_voip_wrapper_voice_service_profile(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_voice_service_profile_t *parm)
{
	int ret;

	if (action == VOIP_HW_ACTION_SET && omci_env_g.voip_debug_set_flag)
	{
		voice_service_profile_show(action, port, mask, parm);
	}

	ret = voip_hw_voice_service_profile(action, port, mask, parm);

	if (action == VOIP_HW_ACTION_GET && omci_env_g.voip_debug_get_flag)
	{
		voice_service_profile_show(action, port, mask, parm);
	}

	if (action == VOIP_HW_ACTION_RELEASE && omci_env_g.voip_debug_release_flag)
	{
		dbprintf(LOG_ERR, "===action=%u, port=%u, mask=0x%.2X,0x%.2X\n", action, port, mask[0], mask[1]);
	}

	return ret;
}

static void
rtp_profile_data_show(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_rtp_profile_data_t *parm)
{
	dbprintf(LOG_ERR, "===action=%u, port=%u, mask=0x%.2X,0x%.2X\n", action, port, mask[0], mask[1]);
	dbprintf(LOG_ERR, "lpmin=%u, lpmax=%u, dscp=%u, pe=%u, te=%u, de=%u, ce=%u\n",
		parm->local_port_min,
		parm->local_port_max,
		parm->dscp_mark,
		parm->piggyback_events,
		parm->tone_events,
		parm->dtmf_events,
		parm->cas_events);
}

int
er_group_hw_voip_wrapper_rtp_profile_data(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_rtp_profile_data_t *parm)
{
	int ret;

	if (action == VOIP_HW_ACTION_SET && omci_env_g.voip_debug_set_flag)
	{
		rtp_profile_data_show(action, port, mask, parm);
	}

	ret = voip_hw_rtp_profile_data(action, port, mask, parm);

	if (action == VOIP_HW_ACTION_GET && omci_env_g.voip_debug_get_flag)
	{
		rtp_profile_data_show(action, port, mask, parm);
	}

	if (action == VOIP_HW_ACTION_RELEASE && omci_env_g.voip_debug_release_flag)
	{
		dbprintf(LOG_ERR, "===action=%u, port=%u, mask=0x%.2X,0x%.2X\n", action, port, mask[0], mask[1]);
	}

	return ret;
}

static void
voip_application_service_profile_show(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_voip_application_service_profile_t *parm)
{
	dbprintf(LOG_ERR, "===action=%u, port=%u, mask=0x%.2X,0x%.2X\n", action, port, mask[0], mask[1]);
	dbprintf(LOG_ERR, "cf=%u.%u.%u.%u.%u.%u.%u\n",
		parm->cid_features.calling_number,
		parm->cid_features.calling_name,
		parm->cid_features.cid_blocking,
		parm->cid_features.cid_number,
		parm->cid_features.cid_name,
		parm->cid_features.anonymous_cid_blocking,
		parm->cid_features.reserved);

	dbprintf(LOG_ERR, "cwf=%u.%u.%u\n",
		parm->call_waiting_features.call_waiting,
		parm->call_waiting_features.caller_id_announcement,
		parm->call_waiting_features.reserved);

	dbprintf(LOG_ERR, "cptf=%u.%u.%u.%u.%u.%u.%u.%u.%u\n",
		parm->call_progress_or_transfer_features.threeway,
		parm->call_progress_or_transfer_features.call_transfer,
		parm->call_progress_or_transfer_features.call_hold,
		parm->call_progress_or_transfer_features.call_park,
		parm->call_progress_or_transfer_features.do_not_disturb,
		parm->call_progress_or_transfer_features.flash_on_emergency_service_call,
		parm->call_progress_or_transfer_features.emergency_service_originating_hold,
		parm->call_progress_or_transfer_features.six_way,
		parm->call_progress_or_transfer_features.reserved);

	dbprintf(LOG_ERR, "cpf=%u.%u.%u.%u.%u\n",
		parm->call_presentation_features.message_waiting_indication_splash_ring,
		parm->call_presentation_features.message_waiting_indication_special_dial_tone,
		parm->call_presentation_features.message_waiting_indication_visual_indication,
		parm->call_presentation_features.call_forwarding_indication,
		parm->call_presentation_features.reserved);

	dbprintf(LOG_ERR, "dcf=%u.%u.%u\n",
		parm->direct_connect_feature.direct_connect_feature_enabled,
		parm->direct_connect_feature.dial_tone_feature_delay_option,
		parm->direct_connect_feature.reserved);

	if (parm->direct_connect_uri_pointer != NULL && parm->direct_connect_uri_pointer->security_pointer != NULL)
	{
		dbprintf(LOG_ERR, "dcu.se.vs=%u, dcu.se.un1=%s, dcu.se.pw=%s, dcu.se.re=%s, dcu.se.un2=%s\n",
			parm->direct_connect_uri_pointer->security_pointer->validation_scheme,
			parm->direct_connect_uri_pointer->security_pointer->username_1,
			parm->direct_connect_uri_pointer->security_pointer->password,
			parm->direct_connect_uri_pointer->security_pointer->realm,
			parm->direct_connect_uri_pointer->security_pointer->username_2);
	} else {
		dbprintf(LOG_ERR, "dcu.se=null\n");
	}

	if (parm->direct_connect_uri_pointer != NULL && parm->direct_connect_uri_pointer->address_pointer != NULL)
	{
		dbprintf(LOG_ERR, "dcu.ad=%s\n", parm->direct_connect_uri_pointer->address_pointer);
	} else {
		dbprintf(LOG_ERR, "dcu.ad=null\n");
	}

	if (parm->bridged_line_agent_uri_pointer != NULL && parm->bridged_line_agent_uri_pointer->security_pointer != NULL)
	{
		dbprintf(LOG_ERR, "bla.se.vs=%u, bla.se.un1=%s, bla.se.pw=%s, bla.se.re=%s, bla.se.un2=%s\n",
		parm->bridged_line_agent_uri_pointer->security_pointer->validation_scheme,
		parm->bridged_line_agent_uri_pointer->security_pointer->username_1,
		parm->bridged_line_agent_uri_pointer->security_pointer->password,
		parm->bridged_line_agent_uri_pointer->security_pointer->realm,
		parm->bridged_line_agent_uri_pointer->security_pointer->username_2);
	} else {
		dbprintf(LOG_ERR, "bla.se=null\n");
	}

	if (parm->bridged_line_agent_uri_pointer != NULL && parm->bridged_line_agent_uri_pointer->address_pointer != NULL)
	{
		dbprintf(LOG_ERR, "bla.ad(%s)\n", parm->bridged_line_agent_uri_pointer->address_pointer);
	} else {
		dbprintf(LOG_ERR, "bla.ad=null\n");
	}

	if (parm->conference_factory_uri_pointer != NULL && parm->conference_factory_uri_pointer->security_pointer != NULL)
	{
		dbprintf(LOG_ERR, "cf.se.vs=%u, cf.se.un1=%s, cf.se.pw=%s, cf.se.re=%s, cf.se.un2=%s\n",
		parm->conference_factory_uri_pointer->security_pointer->validation_scheme,
		parm->conference_factory_uri_pointer->security_pointer->username_1,
		parm->conference_factory_uri_pointer->security_pointer->password,
		parm->conference_factory_uri_pointer->security_pointer->realm,
		parm->conference_factory_uri_pointer->security_pointer->username_2);
	} else {
		dbprintf(LOG_ERR, "cf.se=null\n");
	}

	if (parm->conference_factory_uri_pointer != NULL && parm->conference_factory_uri_pointer->address_pointer != NULL)
	{
		dbprintf(LOG_ERR, "cf.ad=%s\n", parm->conference_factory_uri_pointer->address_pointer);
	} else {
		dbprintf(LOG_ERR, "cf.ad=null\n");
	}

	dbprintf(LOG_ERR, "Dial tone feature delay/Warmline timer=%u(seconds).\n", parm->voip_hw_dial_tone_feature_delay_timer);
}

int
er_group_hw_voip_wrapper_voip_application_service_profile(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_voip_application_service_profile_t *parm)
{
	int ret;

	if (action == VOIP_HW_ACTION_SET && omci_env_g.voip_debug_set_flag)
	{
		voip_application_service_profile_show(action, port, mask, parm);
	}

	ret = voip_hw_voip_application_service_profile(action, port, mask, parm);

	if (action == VOIP_HW_ACTION_GET && omci_env_g.voip_debug_get_flag)
	{
		voip_application_service_profile_show(action, port, mask, parm);
	}

	if (action == VOIP_HW_ACTION_RELEASE && omci_env_g.voip_debug_release_flag)
	{
		dbprintf(LOG_ERR, "===action=%u, port=%u, mask=0x%.2X,0x%.2X\n", action, port, mask[0], mask[1]);
	}

	return ret;
}

static void
voip_feature_access_codes_show(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_voip_feature_access_codes_t *parm)
{
	dbprintf(LOG_ERR, "===action=%u, port=%u, mask=0x%.2X,0x%.2X\n", action, port, mask[0], mask[1]);
	dbprintf(LOG_ERR, "ccw=%s, ch=%s, cp=%s, cia=%s, cid=%s, dnda=%s, dndd=%s, dndpc=%s, esn(%s), is(%s)\n",
		parm->cancel_call_waiting,
		parm->call_hold,
		parm->call_park,
		parm->caller_id_activate,
		parm->caller_id_deactivate,
		parm->do_not_disturb_activation,
		parm->do_not_disturb_deactivation,
		parm->do_not_disturb_pin_change,
		parm->emergency_service_number,
		parm->intercom_service);
}

int
er_group_hw_voip_wrapper_voip_feature_access_codes(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_voip_feature_access_codes_t *parm)
{
	int ret;

	if (action == VOIP_HW_ACTION_SET && omci_env_g.voip_debug_set_flag)
	{
		voip_feature_access_codes_show(action, port, mask, parm);
	}

	ret = voip_hw_voip_feature_access_codes(action, port, mask, parm);

	if (action == VOIP_HW_ACTION_GET && omci_env_g.voip_debug_get_flag)
	{
		voip_feature_access_codes_show(action, port, mask, parm);
	}

	if (action == VOIP_HW_ACTION_RELEASE && omci_env_g.voip_debug_release_flag)
	{
		dbprintf(LOG_ERR, "===action=%u, port=%u, mask=0x%.2X,0x%.2X\n", action, port, mask[0], mask[1]);
	}

	return ret;
}

static void
network_dial_plan_table_show(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_network_dial_plan_table_t *parm)
{
	int i;
	struct voip_hw_dial_plan_entry_t *dial_plan_entry;

	dbprintf(LOG_ERR, "===action=%u, port=%u, mask=0x%.2X,0x%.2X\n", action, port, mask[0], mask[1]);
	dbprintf(LOG_ERR, "dpn=%u, dpt=%u, cdt=%u, pdt=%u, dpf=%u\n",
	parm->dial_plan_number,
	parm->dial_plan_table_max_size,
	parm->critical_dial_timeout,
	parm->partial_dial_timeout,
	parm->dial_plan_format);

	dbprintf(LOG_ERR, "table: \n");
	i = 0;
	list_for_each_entry(dial_plan_entry, &parm->dial_plan_list, dial_plan_node)
	{
		dbprintf(LOG_ERR, "     entry(%d):id(%u),a(%u), token(%s)\n", i++, dial_plan_entry->id, dial_plan_entry->action, dial_plan_entry->token);
	}
}

int
er_group_hw_voip_wrapper_network_dial_plan_table(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_network_dial_plan_table_t *parm)
{
	int ret;

	if (action == VOIP_HW_ACTION_SET && omci_env_g.voip_debug_set_flag)
	{
		network_dial_plan_table_show(action, port, mask, parm);
	}

	ret = voip_hw_network_dial_plan_table(action, port, mask, parm);

	if (action == VOIP_HW_ACTION_GET && omci_env_g.voip_debug_get_flag)
	{
		network_dial_plan_table_show(action, port, mask, parm);
	}

	if (action == VOIP_HW_ACTION_RELEASE && omci_env_g.voip_debug_release_flag)
	{
		dbprintf(LOG_ERR, "===action=%u, port=%u, mask=0x%.2X,0x%.2X\n", action, port, mask[0], mask[1]);
	}

	return ret;
}

static void
voip_line_status_show(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_voip_line_status_t *parm)
{
	dbprintf(LOG_ERR, "===action=%u, port=%u, mask=0x%.2X,0x%.2X\n", action, port, mask[0], mask[1]);
	dbprintf(LOG_ERR, "vcu=%u, vvss=%u, vpst=%u, vc1pp=%u, vc2pp=%u, vc1da=%s, vc2da=%s\n",
		parm->voip_codec_used,
		parm->voip_voice_server_status,
		parm->voip_port_session_type,
		parm->voip_call_1_packet_period,
		parm->voip_call_2_packet_period,
		parm->voip_call_1_dest_addr,
		parm->voip_call_2_dest_addr);
}

int
er_group_hw_voip_wrapper_voip_line_status(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_voip_line_status_t *parm)
{
	int ret;

	if (action == VOIP_HW_ACTION_SET && omci_env_g.voip_debug_set_flag)
	{
		voip_line_status_show(action, port, mask, parm);
	}

	ret = voip_hw_voip_line_status(action, port, mask, parm);

	if (action == VOIP_HW_ACTION_GET && omci_env_g.voip_debug_get_flag)
	{
		voip_line_status_show(action, port, mask, parm);
	}

	if (action == VOIP_HW_ACTION_RELEASE && omci_env_g.voip_debug_release_flag)
	{
		dbprintf(LOG_ERR, "===action=%u, port=%u, mask=0x%.2X,0x%.2X\n", action, port, mask[0], mask[1]);
	}

	return ret;
}

static void
call_control_performance_monitoring_history_data_show(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_call_control_performance_monitoring_history_data_t *parm)
{
	dbprintf(LOG_ERR, "===action=%u, port=%u, mask=0x%.2X,0x%.2X\n", action, port, mask[0], mask[1]);
	dbprintf(LOG_ERR, "csf=%u, cst=%u, ctf=%u, apr=%u, apoht=%u\n",
		parm->call_setup_failures,
		parm->call_setup_timer,
		parm->call_terminate_failures,
		parm->analog_port_releases,
		parm->analog_port_off_hook_timer);
}

int
er_group_hw_voip_wrapper_call_control_performance_monitoring_history_data(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_call_control_performance_monitoring_history_data_t *parm)
{
	int ret;

	if (action == VOIP_HW_ACTION_SET && omci_env_g.voip_debug_set_flag)
	{
		call_control_performance_monitoring_history_data_show(action, port, mask, parm);
	}

	ret = voip_hw_call_control_performance_monitoring_history_data(action, port, mask, parm);

	if (action == VOIP_HW_ACTION_GET && omci_env_g.voip_debug_get_flag)
	{
		call_control_performance_monitoring_history_data_show(action, port, mask, parm);
	}

	if (action == VOIP_HW_ACTION_RELEASE && omci_env_g.voip_debug_release_flag)
	{
		dbprintf(LOG_ERR, "===action=%u, port=%u, mask=0x%.2X,0x%.2X\n", action, port, mask[0], mask[1]);
	}

	return ret;
}

static void
rtp_performance_monitoring_history_data_show(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_rtp_performance_monitoring_history_data_t *parm)
{
	dbprintf(LOG_ERR, "===action=%u, port=%u, mask=0x%.2X,0x%.2X\n", action, port, mask[0], mask[1]);
	dbprintf(LOG_ERR, "re=%u, pl=%u, mj=%u, mtbrp=%u, bu=%u, bo=%u\n",
		parm->rtp_errors,
		parm->packet_loss,
		parm->maximum_jitter,
		parm->maximum_time_between_rtcp_packets,
		parm->buffer_underflows,
		parm->buffer_overflows);
}

int
er_group_hw_voip_wrapper_rtp_performance_monitoring_history_data(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_rtp_performance_monitoring_history_data_t *parm)
{
	int ret;

	if (action == VOIP_HW_ACTION_SET && omci_env_g.voip_debug_set_flag)
	{
		rtp_performance_monitoring_history_data_show(action, port, mask, parm);
	}

	ret = voip_hw_rtp_performance_monitoring_history_data(action, port, mask, parm);

	if (action == VOIP_HW_ACTION_GET && omci_env_g.voip_debug_get_flag)
	{
		rtp_performance_monitoring_history_data_show(action, port, mask, parm);
	}

	if (action == VOIP_HW_ACTION_RELEASE && omci_env_g.voip_debug_release_flag)
	{
		dbprintf(LOG_ERR, "===action=%u, port=%u, mask=0x%.2X,0x%.2X\n", action, port, mask[0], mask[1]);
	}

	return ret;
}

static void
sip_agent_performance_monitoring_history_data_show(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_sip_agent_performance_monitoring_history_data_t *parm)
{
	dbprintf(LOG_ERR, "===action=%u, port=%u, mask=0x%.2X,0x%.2X\n", action, port, mask[0], mask[1]);
	dbprintf(LOG_ERR, "t=%u, rir=%u, rir=%u, rnr=%u, rnr=%u, rr=%u, rrr=%u, tir=%u, tir=%u, tnr=%u, tnr=%u, tr=%u, tnr=%u\n",
		parm->transactions,
		parm->rx_invite_reqs,
		parm->rx_invite_retrans,
		parm->rx_noninvite_reqs,
		parm->rx_noninvite_retrans,
		parm->rx_response,
		parm->rx_response_retransmissions,
		parm->tx_invite_reqs,
		parm->tx_invite_retrans,
		parm->tx_noninvite_reqs,
		parm->tx_noninvite_retrans,
		parm->tx_response,
		parm->tx_response_retransmissions);
}

int
er_group_hw_voip_wrapper_sip_agent_performance_monitoring_history_data(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_sip_agent_performance_monitoring_history_data_t *parm)
{
	int ret;

	if (action == VOIP_HW_ACTION_SET && omci_env_g.voip_debug_set_flag)
	{
		sip_agent_performance_monitoring_history_data_show(action, port, mask, parm);
	}

	ret = voip_hw_sip_agent_performance_monitoring_history_data(action, port, mask, parm);

	if (action == VOIP_HW_ACTION_GET && omci_env_g.voip_debug_get_flag)
	{
		sip_agent_performance_monitoring_history_data_show(action, port, mask, parm);
	}

	if (action == VOIP_HW_ACTION_RELEASE && omci_env_g.voip_debug_release_flag)
	{
		dbprintf(LOG_ERR, "===action=%u, port=%u, mask=0x%.2X,0x%.2X\n", action, port, mask[0], mask[1]);
	}

	return ret;
}

static void
sip_call_initiation_performance_monitoring_history_data_show(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_sip_call_initiation_performance_monitoring_history_data_t *parm)
{
	dbprintf(LOG_ERR, "===action=%u, port=%u, mask=0x%.2X,0x%.2X\n", action, port, mask[0], mask[1]);
	dbprintf(LOG_ERR, "ftcc=%u, ftvc=%u, tc=%u, frc=%u, ftac=%u\n",
		parm->failed_to_connect_counter,
		parm->failed_to_validate_counter,
		parm->timeout_counter,
		parm->failure_received_counter,
		parm->failed_to_authenticate_counter);
}

int
er_group_hw_voip_wrapper_sip_call_initiation_performance_monitoring_history_data(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_sip_call_initiation_performance_monitoring_history_data_t *parm)
{
	int ret;

	if (action == VOIP_HW_ACTION_SET && omci_env_g.voip_debug_set_flag)
	{
		sip_call_initiation_performance_monitoring_history_data_show(action, port, mask, parm);
	}

	ret = voip_hw_sip_call_initiation_performance_monitoring_history_data(action, port, mask, parm);

	if (action == VOIP_HW_ACTION_GET && omci_env_g.voip_debug_get_flag)
	{
		sip_call_initiation_performance_monitoring_history_data_show(action, port, mask, parm);
	}

	if (action == VOIP_HW_ACTION_RELEASE && omci_env_g.voip_debug_release_flag)
	{
		dbprintf(LOG_ERR, "===action=%u, port=%u, mask=0x%.2X,0x%.2X\n", action, port, mask[0], mask[1]);
	}

	return ret;
}

static void
voip_config_data_show(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_voip_config_data_t *parm)
{
	dbprintf(LOG_ERR, "===action=%u, port=%u, mask=0x%.2X,0x%.2X\n", action, port, mask[0], mask[1]);
	dbprintf(LOG_ERR, "asp=%u:%u:%u:%u, spu=%u, avcm=%u:%u:%u:%u:%u:%u, vcmu=%u, vcs=%u, rp=%u, pv=%s\n",
		parm->available_signalling_protocols.sip,
		parm->available_signalling_protocols.h248,
		parm->available_signalling_protocols.mgcp,
		parm->available_signalling_protocols.reserved,
		parm->signalling_protocols_used,
		parm->available_voip_configuration_methods.omci,
		parm->available_voip_configuration_methods.configuration_file,
		parm->available_voip_configuration_methods.tr069,
		parm->available_voip_configuration_methods.sipping_config_framework,
		parm->available_voip_configuration_methods.reserved,
		parm->available_voip_configuration_methods.proprietary,
		parm->voip_configuration_method_used,
		parm->voip_configuration_state,
		parm->retrieve_profile,
		parm->profile_version);

	if (parm->voip_configuration_address_pointer != NULL && parm->voip_configuration_address_pointer->security_pointer)
	{
		dbprintf(LOG_ERR, "vcap.se.vs=%u, vcap.se.un1=%s, vcap.se.pw=%s, vcap.se.re=%s, vcap.se.un2=%s\n",
			parm->voip_configuration_address_pointer->security_pointer->validation_scheme,
			parm->voip_configuration_address_pointer->security_pointer->username_1,
			parm->voip_configuration_address_pointer->security_pointer->password,
			parm->voip_configuration_address_pointer->security_pointer->realm,
			parm->voip_configuration_address_pointer->security_pointer->username_2);
	} else {
		dbprintf(LOG_ERR, "vcap.se=null\n");
	}

	if (parm->voip_configuration_address_pointer != NULL && parm->voip_configuration_address_pointer->address_pointer)
	{
		dbprintf(LOG_ERR, "vcap.ad=%s\n", parm->voip_configuration_address_pointer->address_pointer);
	} else {
		dbprintf(LOG_ERR, "vcap.ad=null\n");
	}
}

int
er_group_hw_voip_wrapper_voip_config_data(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_voip_config_data_t *parm)
{
	int ret;

	if (action == VOIP_HW_ACTION_SET && omci_env_g.voip_debug_set_flag)
	{
		voip_config_data_show(action, port, mask, parm);
	}

	ret = voip_hw_voip_config_data(action, port, mask, parm);

	if (action == VOIP_HW_ACTION_GET && omci_env_g.voip_debug_get_flag)
	{
		voip_config_data_show(action, port, mask, parm);
	}

	if (action == VOIP_HW_ACTION_RELEASE && omci_env_g.voip_debug_release_flag)
	{
		dbprintf(LOG_ERR, "===action=%u, port=%u, mask=0x%.2X,0x%.2X\n", action, port, mask[0], mask[1]);
	}

	return ret;
}

static void
sip_config_portal_show(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_sip_config_portal_t *parm)
{
	struct voip_hw_sip_config_portal_entry_t *sip_config_portal_entry;

	dbprintf(LOG_ERR, "===action=%u, port=%u, mask=0x%.2X,0x%.2X\n", action, port, mask[0], mask[1]);

	list_for_each_entry(sip_config_portal_entry, &parm->sip_config_portal_list, sip_config_portal_node)
	{
		dbprintf(LOG_ERR, "text:%s\n", sip_config_portal_entry->configuration_text);
	}
}

int
er_group_hw_voip_wrapper_sip_config_portal(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_sip_config_portal_t *parm)
{
	int ret;

	if (action == VOIP_HW_ACTION_SET && omci_env_g.voip_debug_set_flag)
	{
		sip_config_portal_show(action, port, mask, parm);
	}

	ret = voip_hw_sip_config_portal(action, port, mask, parm);

	if (action == VOIP_HW_ACTION_GET && omci_env_g.voip_debug_get_flag)
	{
		sip_config_portal_show(action, port, mask, parm);
	}

	if (action == VOIP_HW_ACTION_RELEASE && omci_env_g.voip_debug_release_flag)
	{
		dbprintf(LOG_ERR, "===action=%u, port=%u, mask=0x%.2X,0x%.2X\n", action, port, mask[0], mask[1]);
	}

	return ret;
}

static void
voip_call_statistics_extension4_show(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_voip_call_statistics_extension4_t *parm)
{
	dbprintf(LOG_ERR, "===action=%u, port=%u, mask=0x%.2X,0x%.2X\n", action, port, mask[0], mask[1]);
	dbprintf(LOG_ERR, "clrerrorcntrs=%d, clrcallcntrs=%d\n", parm->clrerrorcntrs, parm->clrcallcntrs);
}

int
er_group_hw_voip_wrapper_voip_call_statistics_extension4(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_voip_call_statistics_extension4_t *parm)
{
	int ret;

	//Only need 0->1 event, don't need really to set attribute value to 1
	if (action == VOIP_HW_ACTION_SET && omci_env_g.voip_debug_set_flag)
	{
		voip_call_statistics_extension4_show(action, port, mask, parm);
	}

	ret = voip_hw_voip_call_statistics_extension4(action, port, mask, parm);

	if (action == VOIP_HW_ACTION_GET && omci_env_g.voip_debug_get_flag)
	{
		voip_call_statistics_extension4_show(action, port, mask, parm);
	}

	if (action == VOIP_HW_ACTION_RELEASE && omci_env_g.voip_debug_release_flag)
	{
		dbprintf(LOG_ERR, "===action=%u, port=%u, mask=0x%.2X,0x%.2X\n", action, port, mask[0], mask[1]);
	}

	return ret;
}

