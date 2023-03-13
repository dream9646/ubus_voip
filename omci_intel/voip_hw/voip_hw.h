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
 * Module  : voip_hw
 * File    : voip_hw.h
 *
 ******************************************************************/

#ifndef __VOIP_HW_H__
#define __VOIP_HW_H__

#include "list.h"
#include "omcimsg.h"

/*
 53 Physical_path_termination_point_POTS_UNI [9.9.1]
153 SIP_user_data [9.9.2]
150 SIP_agent_config_data [9.9.3]
139 VoIP_voice_CTP [9.9.4]
142 VoIP_media_profile [9.9.5]
 58 Voice_service_profile [9.9.6]
143 RTP_profile_data [9.9.7]
146 VoIP_application_service_profile [9.9.8]
147 VoIP_feature_access_codes [9.9.9]
145 Network_dial_plan_table [9.9.10]
141 VoIP_line_status [9.9.11]
140 Call_control_performance_monitoring_history_data [9.9.12]
144 RTP_performance_monitoring_history_data [9.9.13]
151 SIP_agent_performance_monitoring_history_data [9.9.14]
152 SIP_call_initiation_performance_monitoring_history_data [9.9.15]
155 MGC_config_data [9.9.16]
156 MGC_performance_monitoring_history_data [9.9.17]
138 VoIP_config_data [9.9.18]
149 SIP_config_portal [9.9.19]
154 MGC_config_portal [9.9.20]
 80 Physical_path_termination_point_ISDN_UNI [9.9.21]
*/

#define VOIP_HW_MAX_PORT_NUMBER 2
#define VOIP_HW_GLOBAL_PORT_DEF 0xFFFF

#define PORT_STATUS__INITIAL		0
#define PORT_STATUS__REGISTERED		1
#define PORT_STATUS__IN_SESSION		2
#define PORT_STATUS__FAILED_ICMP	3
#define PORT_STATUS__FAILED_TCP		4
#define PORT_STATUS__FAILED_AUTH	5
#define PORT_STATUS__FAILED_TIMEOUT	6
#define PORT_STATUS__FAILED_SERVER	7
#define PORT_STATUS__FAILED_OTHER	99
#define PORT_STATUS__REGISTERING	100
#define PORT_STATUS__UNREGISTERING	101

#define SIP_STATUS__INITIAL		0
#define SIP_STATUS__REGISTERED		1
#define SIP_STATUS__IN_SESSION		2
#define SIP_STATUS__FAILED_ICMP		3
#define SIP_STATUS__FAILED_TCP		4
#define SIP_STATUS__FAILED_AUTH		5
#define SIP_STATUS__FAILED_TIMEOUT	6
#define SIP_STATUS__FAILED_SERVER	7
#define SIP_STATUS__FAILED_OTHER	7
#define SIP_STATUS__REGISTERING		0
#define SIP_STATUS__UNREGISTERING	0

//according to 988
#define CODEC_CLASS_ATTR_ID_PCMU 0
#define CODEC_CLASS_ATTR_ID_G723 4
#define CODEC_CLASS_ATTR_ID_PCMA 8
#define CODEC_CLASS_ATTR_ID_G729 18

#define CLASS_141_ATTR_3_IDLE 0
#define CLASS_141_ATTR_3_2WAY 1
#define CLASS_141_ATTR_3_3WAY 2
#define CLASS_141_ATTR_3_FAX  3
#define CLASS_141_ATTR_3_TELEMETRY 4
#define CLASS_141_ATTR_3_CONFERENCE 5

#define METAFILE_PATH	"/etc/wwwctrl/metafile.dat"

struct voip_hw_parm_bitmask_t {
	unsigned char voip_physical_path_termination_point_pots_uni[2];
	unsigned char sip_user_data[2];
	unsigned char sip_agent_config_data[2];
	unsigned char voip_voice_ctp[2];
	unsigned char voip_media_profile[2];
	unsigned char voip_voice_service_profile[2];
	unsigned char rtp_profile_data[2];
	unsigned char voip_application_service_profile[2];
	unsigned char voip_feature_access_codes[2];
	unsigned char network_dial_plan_table[2];
	unsigned char voip_line_status[2];
	unsigned char call_control_performance_monitoring_history_data[2];
	unsigned char rtp_performance_monitoring_history_data[2];
	unsigned char sip_agent_performance_monitoring_history_data[2];
	unsigned char sip_call_initiation_performance_monitoring_history_data[2];
	unsigned char voip_config_data[2];
	unsigned char sip_config_portal[2];
//FH, proprietary MEs
	unsigned char wri_rtp_resource_name[2];
	unsigned char wri_rfc2833_config_data[2];
	unsigned char wri_keepalive_config_data[2];
	unsigned char wri_pos_config_data[2];
	unsigned char wri_ipt_config_data[2];
	unsigned char wri_mg_port_register_control[2];
	unsigned char wri_digit_map_match_mode[2];
	unsigned char wri_fax_config_data[2];
	unsigned char wri_ngn_resource_info[2];
	unsigned char wri_ngn_port_statics[2];
	unsigned char wri_sip_config[2];
	unsigned char wri_softswitch_progile_config[2];
	unsigned char voip_call_statistics_extension4[2];
};

extern struct voip_hw_parm_bitmask_t voip_hw_parm_bitmask_g;

enum voip_hw_action_t {
	VOIP_HW_ACTION_INIT,
	VOIP_HW_ACTION_SET,
	VOIP_HW_ACTION_GET,
	VOIP_HW_ACTION_RELEASE,
	VOIP_HW_ACTION_INVALID
};

struct voip_hw_ip_host_config_data_t {
	unsigned char ip_options;
	unsigned char mac_address[6];
	unsigned char ont_identifier[26];
	unsigned int ip_address;
	unsigned int mask;
	unsigned int gateway;
	unsigned int primary_dns;
	unsigned int secondary_dns;
	unsigned int current_address;
	unsigned int current_mask;
	unsigned int current_gateway;
	unsigned int current_paimary_dns;
	unsigned int current_secondary_dns;
	unsigned char doman_name[26];
	unsigned char host_name[26];
	unsigned char devname[26];
};

struct voip_hw_tcp_udp_config_data_t {
	unsigned short port_id;
	unsigned char protocol;
	unsigned char tos_diffserv_field;
	struct voip_hw_ip_host_config_data_t *ip_host_pointer;
};

struct voip_hw_authentication_security_method_t {
	unsigned char validation_scheme;
	unsigned char username_1[26];
	unsigned char password[26];
	unsigned char realm[26];
	unsigned char username_2[26];
};

struct voip_hw_network_address_t {
	struct voip_hw_authentication_security_method_t *security_pointer;
	unsigned char *address_pointer; // 376 chars string
};

struct voip_hw_physical_path_termination_point_pots_uni_t {
	unsigned char administrative_state;
	unsigned char arc;
	unsigned char arc_interval;
	unsigned char impedance;
	unsigned char transmission_path;
	char rx_gain;	// -120..60
	char tx_gain;	// -120..60
	unsigned char operational_state;
	unsigned char hook_state;
	unsigned short pots_holdover_time;
};

struct voip_hw_tone_pattern_entry_t {
	unsigned char index;
	unsigned char tone_on;
	unsigned short frequency_1;
	unsigned char power_1;
	unsigned short frequency_2;
	unsigned char power_2;
	unsigned short frequency_3;
	unsigned char power_3;
	unsigned short frequency_4;
	unsigned char power_4;
	unsigned short modulation_frequency;
	unsigned char modulation_power;
	unsigned short duration;
	unsigned char next_entry;
	struct list_head tone_pattern_node;
};

struct voip_hw_tone_event_entry_t {
	unsigned char event;
	unsigned char tone_pattern;
	unsigned char *tone_file; // 376 chars string
	unsigned char tone_file_repetitions;
	unsigned short reserved;
	struct list_head tone_event_node;
};

struct voip_hw_ringing_pattern_entry_t {
	unsigned char index;
	unsigned char ringing_on;
	unsigned short duration;
	unsigned char next_entry;
	struct list_head ringing_pattern_node;
};

struct voip_hw_ringing_event_entry_t {
	unsigned char event;
	unsigned char ringing_pattern;
	unsigned char *ringing_file; // 376 chars string
	unsigned char ringing_file_repetitions;
	unsigned char *ringing_text; // 376 chars string
	struct list_head ringing_event_node;
};

struct voip_hw_voice_service_profile_t {
	unsigned char announcement_type;
	unsigned short jitter_target;
	unsigned short jitter_buffer_max;
	unsigned char echo_cancel_ind;
	unsigned short pstn_protocol_variant;
	unsigned short dtmf_digit_levels;
	unsigned short dtmf_digit_duration;
	unsigned short hook_flash_minimum_time;
	unsigned short hook_flash_maximum_time;
	struct list_head tone_pattern_list;
	struct list_head tone_event_list;
	struct list_head ringing_pattern_list;
	struct list_head ringing_event_list;
	struct voip_hw_network_address_t *network_specific_extensions;
};

struct voip_hw_signalling_protocols_t {
	unsigned char reserved:5;
	unsigned char mgcp:1;
	unsigned char h248:1;
	unsigned char sip:1;
};

struct voip_hw_configuration_methods_t {
	unsigned int omci:1;
	unsigned int configuration_file:1;
	unsigned int tr069:1;
	unsigned int sipping_config_framework:1;
	unsigned int reserved:20;
	unsigned int proprietary:8;
};

struct voip_hw_voip_config_data_t {
	struct voip_hw_signalling_protocols_t available_signalling_protocols;
	unsigned char signalling_protocols_used;
	struct voip_hw_configuration_methods_t available_voip_configuration_methods;
	unsigned char voip_configuration_method_used;
	struct voip_hw_network_address_t *voip_configuration_address_pointer;	// uri
	unsigned char voip_configuration_state;
	unsigned char retrieve_profile;
	unsigned char profile_version[26];	// 25byte,string,r
};

struct voip_hw_voip_voice_ctp_t {
	unsigned char signalling_code;
};

struct voip_hw_call_control_performance_monitoring_history_data_t {
	unsigned int call_setup_failures;
	unsigned int call_setup_timer;
	unsigned int call_terminate_failures;
	unsigned int analog_port_releases;
	unsigned int analog_port_off_hook_timer;
};

struct voip_hw_voip_line_status_t {
	unsigned short voip_codec_used;
	unsigned char voip_voice_server_status;
	unsigned char voip_port_session_type;
	unsigned short voip_call_1_packet_period;
	unsigned short voip_call_2_packet_period;
	unsigned char voip_call_1_dest_addr[26];	// 25byte,string,r
	unsigned char voip_call_2_dest_addr[26];	// 25byte,string,r
};

struct voip_hw_voip_media_profile_t {
	unsigned char fax_mode;
	unsigned char codec_selection_1st_order;
	unsigned char packet_period_selection_1st_order; // 10..30
	unsigned char silence_suppression_1st_order;
	unsigned char codec_selection_2nd_order;
	unsigned char packet_period_selection_2nd_order; // 10..30
	unsigned char silence_suppression_2nd_order;
	unsigned char codec_selection_3rd_order;
	unsigned char packet_period_selection_3rd_order; // 10..30
	unsigned char silence_suppression_3rd_order;
	unsigned char codec_selection_4th_order;
	unsigned char packet_period_selection_4th_order; // 10..30
	unsigned char silence_suppression_4th_order;
	unsigned char oob_dtmf;
};

struct voip_hw_rtp_profile_data_t {
	unsigned short local_port_min;
	unsigned short local_port_max;
	unsigned char dscp_mark;
	unsigned char piggyback_events;
	unsigned char tone_events;
	unsigned char dtmf_events;
	unsigned char cas_events;
};

struct voip_hw_rtp_performance_monitoring_history_data_t {
	unsigned int rtp_errors;
	unsigned int packet_loss;
	unsigned int maximum_jitter;
	unsigned int maximum_time_between_rtcp_packets;
	unsigned int buffer_underflows;
	unsigned int buffer_overflows;
};

struct voip_hw_dial_plan_entry_t {
	unsigned char id;
	unsigned char action;
	unsigned char token[29];	// 28 characters string
	struct list_head dial_plan_node;
};

struct voip_hw_network_dial_plan_table_t {
	unsigned short dial_plan_number;
	unsigned short dial_plan_table_max_size;
	unsigned short critical_dial_timeout;
	unsigned short partial_dial_timeout;
	unsigned char dial_plan_format;
	struct list_head dial_plan_list;
};

struct voip_hw_cid_features_t {
	unsigned char calling_number:1;
	unsigned char calling_name:1;
	unsigned char cid_blocking:1;
	unsigned char cid_number:1;
	unsigned char cid_name:1;
	unsigned char anonymous_cid_blocking:1;
	unsigned char reserved:2;
};

struct voip_hw_call_waiting_features_t {
	unsigned char call_waiting:1;
	unsigned char caller_id_announcement:1;
	unsigned char reserved:6;
};

struct voip_hw_call_progress_or_transfer_features_t {
	unsigned short threeway:1;
	unsigned short call_transfer:1;
	unsigned short call_hold:1;
	unsigned short call_park:1;
	unsigned short do_not_disturb:1;
	unsigned short flash_on_emergency_service_call:1;
	unsigned short emergency_service_originating_hold:1;
	unsigned short six_way:1;
	unsigned short reserved:8;
};

struct voip_hw_call_presentation_features_t {
	unsigned short message_waiting_indication_splash_ring:1;
	unsigned short message_waiting_indication_special_dial_tone:1;
	unsigned short message_waiting_indication_visual_indication:1;
	unsigned short call_forwarding_indication:1;
	unsigned short reserved:12;
};

struct voip_hw_direct_connect_feature_t {
	unsigned char direct_connect_feature_enabled:1;
	unsigned char dial_tone_feature_delay_option:1;
	unsigned char reserved:6;
};

struct voip_hw_voip_application_service_profile_t {
	struct voip_hw_cid_features_t cid_features;
	struct voip_hw_call_waiting_features_t call_waiting_features;
	struct voip_hw_call_progress_or_transfer_features_t call_progress_or_transfer_features;
	struct voip_hw_call_presentation_features_t call_presentation_features;
	struct voip_hw_direct_connect_feature_t direct_connect_feature;
	struct voip_hw_network_address_t *direct_connect_uri_pointer;		// uri
	struct voip_hw_network_address_t *bridged_line_agent_uri_pointer;	// uri
	struct voip_hw_network_address_t *conference_factory_uri_pointer;	// uri
	unsigned short voip_hw_dial_tone_feature_delay_timer;
};

struct voip_hw_voip_feature_access_codes_t {
	unsigned char cancel_call_waiting[6];	// 5byte,string,rw
	unsigned char call_hold[6];		// 5byte,string,rw
	unsigned char call_park[6];		// 5byte,string,rw
	unsigned char caller_id_activate[6];	// 5byte,string,rw
	unsigned char caller_id_deactivate[6];	// 5byte,string,rw
	unsigned char do_not_disturb_activation[6];	// 5byte,string,rw
	unsigned char do_not_disturb_deactivation[6];	// 5byte,string,rw
	unsigned char do_not_disturb_pin_change[6];	// 5byte,string,rw
	unsigned char emergency_service_number[6];	// 5byte,string,rw
	unsigned char intercom_service[6];		// 5byte,string,rw
	//
	unsigned char unattended_blind_call_transfer[6];	// 5byte,string,rw
	unsigned char attended_call_transfer[6];	// 5byte,string,rw
};

struct voip_hw_sip_config_portal_entry_t {
	unsigned char configuration_text[30];	// 29byte,table,r
	struct list_head sip_config_portal_node;
};
struct voip_hw_sip_config_portal_t {
	struct list_head sip_config_portal_list;
};

struct voip_hw_sip_agent_config_data_t {
	unsigned char *proxy_server_address_pointer; // 376 chars string
	unsigned char *outbound_proxy_address_pointer; // 376 chars string
	unsigned int primary_sip_dns;	// ipv4
	unsigned int secondary_sip_dns;	// ipv4
	struct voip_hw_tcp_udp_config_data_t *tcp_udp_pointer;
	unsigned int sip_reg_exp_time;
	unsigned int sip_rereg_head_start_time;
	unsigned char *host_part_uri; // 376 chars string
	unsigned char sip_status;
	struct voip_hw_network_address_t *sip_registrar;
	unsigned char softswitch[5];	// 4byte,string,rws
};

struct voip_hw_sip_agent_performance_monitoring_history_data_t {
	unsigned int transactions;
	unsigned int rx_invite_reqs;
	unsigned int rx_invite_retrans;
	unsigned int rx_noninvite_reqs;
	unsigned int rx_noninvite_retrans;
	unsigned int rx_response;
	unsigned int rx_response_retransmissions;
	unsigned int tx_invite_reqs;
	unsigned int tx_invite_retrans;
	unsigned int tx_noninvite_reqs;
	unsigned int tx_noninvite_retrans;
	unsigned int tx_response;
	unsigned int tx_response_retransmissions;
};

struct voip_hw_sip_call_initiation_performance_monitoring_history_data_t {
	unsigned int failed_to_connect_counter;
	unsigned int failed_to_validate_counter;
	unsigned int timeout_counter;
	unsigned int failure_received_counter;
	unsigned int failed_to_authenticate_counter;
};

struct voip_hw_sip_user_data_t {
	unsigned char *user_part_aor; // 376 chars string
	unsigned char sip_display_name[26];
	struct voip_hw_authentication_security_method_t *username_password;
	struct voip_hw_network_address_t *voicemail_server_sip_uri;
	unsigned int voicemail_subscription_expiration_time;
	unsigned char release_timer;
	unsigned char roh_timer;
};

//FH, proprietary MEs
struct voip_hw_wri_rtp_resource_name_t
{
	unsigned char rtp_name_fixed_part[13]; //12bytes string
	unsigned short rtp_name_var_begin;
	unsigned short rtp_name_var_end;
	unsigned short rtp_name_var_step;
	unsigned short rtp_name_fixed_length;
};

struct voip_hw_wri_rfc2833_config_data_t
{
	unsigned char rfc2833_nego_state;
	unsigned char def_rfc2833_pt;
	unsigned char def_rfc2198_pt;
};

struct voip_hw_wri_keepalive_config_data_t
{
	unsigned char keepalive_enable;
	unsigned short keepalive_interva;
	unsigned short keepalive_times;
	unsigned char keepalive_mode;
	unsigned char rereg_start_time_enable;
};

struct voip_hw_pos_data_entry_t {
	unsigned char id;
	unsigned char tel_num[21];	// 20 characters string
	unsigned short pos_type;
	struct list_head pos_data_node;
};

struct voip_hw_ipt_data_entry_t {
	unsigned char id;
	unsigned char tel_num[21];	// 20 characters string
	unsigned short pos_type;
	struct list_head ipt_data_node;
};

struct voip_hw_wri_pos_config_data_t
{
	unsigned char item_number;
	struct list_head pos_data_list; //linking list of pos_data_entry_t
};

struct voip_hw_wri_ipt_config_data_t
{
	unsigned char item_number;
	struct list_head ipt_data_list; //linking list of pos_data_entry_t
};

struct voip_hw_wri_mg_port_register_control_t
{
	unsigned char *mg_ip_pointer; // 376 chars string
	unsigned char mg_act;
	unsigned char port_act;
}; //by_port

struct voip_hw_wri_digit_map_match_mode_t
{
	unsigned char notify_match_mode;
};

struct voip_hw_wri_fax_config_data_t
{
	unsigned char fax_control_mode;
	unsigned char vbd_state;
	unsigned char vbd_tx_interval;
	unsigned char vbd_rx_interval;
	char vbd_voice_coder;
	unsigned char t38_event_detect_mode;
	unsigned char dtmf_mode;
}; //by_port

struct voip_hw_wri_ngn_port_statics_time_t
{
	unsigned short year;
	unsigned char month;
	unsigned char day;
	unsigned char hour;
	unsigned char minute;
	unsigned char second;
};

struct voip_hw_wri_ngn_resource_info_t
{
	unsigned int received_ngn_req_packets;
	unsigned int send_ngn_req_packets;
	unsigned int received_ngn_resp_packets;
	unsigned int send_ngn_resp_packets;
	unsigned int received_rtp_packets;
	unsigned int send_rtp_packets;
	unsigned int received_rtp_bytes;
	unsigned int send_rtp_bytes;
	unsigned int packet_loss;
	unsigned int network_avg_delay;
	unsigned int rtp_avg_jitter_buffer;
	unsigned int bandwidth_usage;
	unsigned int lost_sig_packets;
	unsigned int retransmit_packets;
	unsigned int wrong_sig_packets;
	unsigned int unknown_sig_packets;
};

struct voip_hw_wri_ngn_port_statics_t
{
	unsigned int received_rtp_packets;
	unsigned int send_rtp_packets;
	unsigned int received_rtp_bytes;
	unsigned int send_rtp_bytes;
	unsigned int rtp_jitter_buffer;
	struct voip_hw_wri_ngn_port_statics_time_t latest_call_begin_time;
	struct voip_hw_wri_ngn_port_statics_time_t latest_call_end_time;
	unsigned int last_call_duration;
	unsigned int total_call_times;
	unsigned int total_call_duration;
	unsigned int upstream_rates;
	unsigned int downstream_rates;
	unsigned int avg_network_delay;
	unsigned int packet_loss;
}; //by port

struct voip_hw_wri_sip_config_t
{
	unsigned char *secondary_sip_registrar_server_address_pointer; // 376 chars string
	unsigned char *secondary_proxy_server_address_pointer; // 376 chars string
	unsigned short first_registrar_port;
	unsigned short first_proxy_port;
	unsigned short secondary_registrar_port;
	unsigned short secondary_proxy_port;
	unsigned char *current_sip_registrar_pointer; // 376 chars string
};

struct voip_hw_wri_softswitch_progile_config_t
{
	unsigned char dail_tone_timeout;
	unsigned char noanswer_tone_timeout;
	unsigned char busy_tone_timeout;
	unsigned char roh_timeout;
	unsigned char retransmit_timeout;
};

//Calix, proprietary MEs
struct voip_hw_voip_call_statistics_extension_t{
	unsigned int dhcpattempts;
	unsigned int dhcpacks;
	unsigned int dhcpnacks;
	unsigned int regattempts;
	unsigned int regchallenges;
	unsigned int regrejects;
	unsigned int reggrants;
	unsigned int incallattempts;
	unsigned int incallcompletions;
	unsigned int incallbusy;
	unsigned int incallpeerdisconnects;
	unsigned int incallontdisconnects;
	unsigned int outcallattempts;
	unsigned int outcallcompletions;
	unsigned int outcallbusy;
};

struct voip_hw_voip_call_statistics_extension2_t{
	unsigned int outcallpeerdisconnects;
	unsigned int outcallontdisconnects;
	unsigned int e911callattempts;
	unsigned int e911callcompletions;
	unsigned int e911callbusy;
	unsigned int e911callpeerdisconnects;
	unsigned int e911callonhooks;
	unsigned int vmwinotifymsgswaiting;
	unsigned int vmwinotifynomsgswaiting;
	unsigned int rtppktssent;
	unsigned int rtppktsrecv;
	unsigned int rtpnullipsent;
	unsigned int rtpnulliprecv;
	unsigned int ext2activecallcntr;
};

struct voip_hw_voip_call_statistics_extension3_t{
	unsigned int recverrors;
	unsigned int xmiterrors;
	unsigned short encodetype;
	unsigned short packetsize;
	unsigned int missingrtp;
	unsigned int sequenceerr;
	unsigned int dropouts;
	unsigned int underruns;
	unsigned int listens;
	unsigned int recvbadsrcports;
	unsigned int recvcomfortnoisepkts;
};

struct voip_hw_voip_call_statistics_extension4_t{
	unsigned short localudpport;
	unsigned int remipadd;
	unsigned short remudpport;
	unsigned int secremipaddr;
	unsigned short secremudpport;
	unsigned int qos;
	unsigned short iplinestatus;
	unsigned char active911call;
	unsigned char clrerrorcntrs;
	unsigned char clrcallcntrs;
};

/* voip_hw.c */
int voip_hw_util_get_codec_id (char *codec);
void voip_hw_parm_bitmask_cast(unsigned char omci_bitmask[2], unsigned char voip_bitmask[2], unsigned char result_bitmask[2]);
void voip_hw_parm_bitmask_cast_reverse(unsigned char result_bitmask[2], unsigned char voip_bitmask[2], unsigned char omci_bitmask[2]);
int voip_hw_init(void);
int voip_hw_destory(void);
int voip_hw_physical_path_termination_point_pots_uni(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_physical_path_termination_point_pots_uni_t *parm);
int voip_hw_sip_user_data(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_sip_user_data_t *parm);
int voip_hw_sip_agent_config_data(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_sip_agent_config_data_t *parm);
int voip_hw_voip_voice_ctp(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_voip_voice_ctp_t *parm);
int voip_hw_voip_media_profile(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_voip_media_profile_t *parm);
int voip_hw_voice_service_profile(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_voice_service_profile_t *parm);
int voip_hw_rtp_profile_data(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_rtp_profile_data_t *parm);
int voip_hw_voip_application_service_profile(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_voip_application_service_profile_t *parm);
int voip_hw_voip_feature_access_codes(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_voip_feature_access_codes_t *parm);
int voip_hw_network_dial_plan_table(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_network_dial_plan_table_t *parm);
int voip_hw_voip_line_status(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_voip_line_status_t *parm);
int voip_hw_call_control_performance_monitoring_history_data(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_call_control_performance_monitoring_history_data_t *parm);
int voip_hw_rtp_performance_monitoring_history_data(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_rtp_performance_monitoring_history_data_t *parm);
int voip_hw_sip_agent_performance_monitoring_history_data(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_sip_agent_performance_monitoring_history_data_t *parm);
int voip_hw_sip_call_initiation_performance_monitoring_history_data(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_sip_call_initiation_performance_monitoring_history_data_t *parm);
int voip_hw_voip_config_data(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_voip_config_data_t *parm);
int voip_hw_sip_config_portal(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_sip_config_portal_t *parm);
int voip_hw_physical_path_termination_point_pots_uni_test(int port, struct omcimsg_test_pots_isdn_t *req, struct omcimsg_test_result_pots_isdn_t *result);

int voip_hw_wri_rtp_resource_name(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_wri_rtp_resource_name_t *parm);
int voip_hw_wri_rfc2833_config_data(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_wri_rfc2833_config_data_t *parm);
int voip_hw_wri_keepalive_config_data(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_wri_keepalive_config_data_t *parm);
int voip_hw_wri_pos_config_data(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_wri_pos_config_data_t *parm);
int voip_hw_wri_ipt_config_data(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_wri_ipt_config_data_t *parm);
int voip_hw_wri_mg_port_register_control(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_wri_mg_port_register_control_t *parm);
int voip_hw_wri_digit_map_match_mode(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_wri_digit_map_match_mode_t *parm);
int voip_hw_wri_fax_config_data(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_wri_fax_config_data_t *parm);
int voip_hw_wri_ngn_resource_info(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_wri_ngn_resource_info_t *parm);
int voip_hw_wri_ngn_port_statics(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_wri_ngn_port_statics_t *parm);
int voip_hw_wri_sip_config(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_wri_sip_config_t *parm);
int voip_hw_wri_softswitch_progile_config(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_wri_softswitch_progile_config_t *parm);
int voip_hw_voip_call_statistics_extension(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_voip_call_statistics_extension_t *parm);
int voip_hw_voip_call_statistics_extension2(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_voip_call_statistics_extension2_t *parm);
int voip_hw_voip_call_statistics_extension3(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_voip_call_statistics_extension3_t *parm);
int voip_hw_voip_call_statistics_extension4(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_voip_call_statistics_extension4_t *parm);
int voip_hw_potsuniext(enum voip_hw_action_t action, int port, unsigned char mask[2], unsigned char *parm);

/* voip_hw_util.c */
int voip_hw_voice_service_profile_release_table(struct voip_hw_voice_service_profile_t *vvsp);
int voip_hw_network_dial_plan_table_release_table(struct voip_hw_network_dial_plan_table_t *ndpt);
int voip_hw_wri_pos_config_data_release_table(struct voip_hw_wri_pos_config_data_t *wpcd);
int voip_hw_wri_ipt_config_data_release_table(struct voip_hw_wri_ipt_config_data_t *wicd);
int voip_hw_sip_config_portal_release_table(struct voip_hw_sip_config_portal_t *scp);
#endif

