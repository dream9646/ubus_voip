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
 * File    : voip_hw.c
 *
 ******************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdarg.h>
#include "voip_hw.h"
#include "util.h"
#include "util_run.h"
#include "metacfg_adapter.h"
#include "voipclient_util.h"
#include "voipclient_comm.h"
#include "voipclient_cmd.h"
#include "omcimain.h"

/* command results */
#define CMD_OK		0
#define CMD_ERROR	(-1)

struct voip_hw_sip_agent_config_data_t AgentBackup;

//voip define
typedef enum hs_voip_codec {
	/* Standard payload types are defined for the following codecs */
	HS_VOIP_CODEC_PCMU,	/* 0: PCMU (G711 - uLaw) */
	HS_VOIP_CODEC_G723,
	HS_VOIP_CODEC_PCMA,	/* 2: PCMA (G711 - aLaw) */
	HS_VOIP_CODEC_CN,	/* 3: Comfort Noise */
	HS_VOIP_CODEC_G729A,	/* 4: For G729, G729B, G729AB also */

	/* Standard payload types are not defined for these codecs */
	HS_VOIP_CODEC_G726_16,
	HS_VOIP_CODEC_G726_24,
	HS_VOIP_CODEC_G726_32,
	HS_VOIP_CODEC_G726_40,
	HS_VOIP_CODEC_ILBC,
	HS_VOIP_CODEC_G722,
	HS_VOIP_CODEC_G7221_24,
	HS_VOIP_CODEC_G7221_32,
	HS_VOIP_CODEC_AMR,

	HS_VOIP_CODEC_DTMF,	/* DTMF packet */
	HS_VOIP_CODEC_T38,
	HS_VOIP_CODEC_WB_CN,	/* Wideband Comfort Noise */

	HS_VOIP_CODEC_INVALID = 0xFF	/* Invalid codec / No change in codec */
} hs_voip_codec_t;

typedef enum fv_dsp_codec
{
   FV_DSP_VPM_CODEC_UNKNOWN = 0,
   FV_DSP_VPM_CODEC_LINEAR16,
   FV_DSP_VPM_CODEC_G711_ULAW,
   FV_DSP_VPM_CODEC_G711_ALAW,
   FV_DSP_VPM_CODEC_G726_40K,
   FV_DSP_VPM_CODEC_G726_32K,
   FV_DSP_VPM_CODEC_G726_24K,
   FV_DSP_VPM_CODEC_G726_16K,
   FV_DSP_VPM_CODEC_G729AB,
   FV_DSP_VPM_CODEC_G723_1A,
   FV_DSP_VPM_CODEC_AMR_NB,
   FV_DSP_VPM_CODEC_AMR_WB = 12,
   FV_DSP_VPM_CODEC_G722,
   FV_DSP_VPM_CODEC_LAST, // dummy
   FV_DSP_VPM_CODEC_G711_CNG,
   FV_DSP_VPM_CODEC_G711_CNG_WB,
   FV_DSP_VPM_CODEC_TELE_EVENT,
   FV_DSP_VPM_CODEC_REDUNDANCY,
   FV_DSP_VPM_CODEC_INVALID
} fv_dsp_codec_t;

struct voip_hw_parm_bitmask_t voip_hw_parm_bitmask_g = {
	.voip_physical_path_termination_point_pots_uni = {0xBF, 0xE0},
	.sip_user_data = {0x7C, 0x30},
	.sip_agent_config_data = {0xFF, 0xE0},
	.voip_voice_ctp = {0x10, 0x00},
	.voip_media_profile = {0xBF, 0xFE},
	.voip_voice_service_profile = {0xFF, 0xFC},
	.rtp_profile_data = {0xFE, 0x00},
	.voip_application_service_profile = {0xFF, 0x80},
	.voip_feature_access_codes = {0xFF, 0xC0},
	.network_dial_plan_table = {0xFC, 0x00},
	.voip_line_status = {0xFE, 0x00},
	.call_control_performance_monitoring_history_data = {0x3E, 0x00},
	.rtp_performance_monitoring_history_data = {0x3F, 0x00},
	.sip_agent_performance_monitoring_history_data = {0x3F, 0xFE},
	.sip_call_initiation_performance_monitoring_history_data = {0x3E, 0x00},
	//.mgc_config_data = {  },//cater
	//.mgc_performance_monitoring_history_data = { },//cater
	.voip_config_data = {0xFF, 0x00},
	.sip_config_portal = {0x80, 0x00},
	//.mgc_config_portal = {  },//cater
	//.voip_physical_path_termination_point_isdn_uni = { }//cater
//FH, proprietary MEs
	.wri_rtp_resource_name = {0xF8, 0x00},
	.wri_rfc2833_config_data = {0xE0, 0x00},
	.wri_keepalive_config_data = {0xF8, 0x00},
	.wri_pos_config_data = {0xC0, 0x00},
	.wri_ipt_config_data = {0xC0, 0x00},
	.wri_mg_port_register_control = {0xD0, 0x00},
	.wri_digit_map_match_mode = {0x80, 0x00},
	.wri_fax_config_data = {0xFD, 0x00},
	.wri_ngn_resource_info = {0xFF, 0xFF},
	.wri_ngn_port_statics = {0x7F, 0xFE},
	.wri_sip_config = {0xFE, 0x00},
	.wri_softswitch_progile_config = {0xF8, 0x00},
//Calix, proprietary MEs
	//todo fixme attr number is 15 or 16 ?//10
	.voip_call_statistics_extension4= {0xFF, 0xC0},
};

// obsolete:stay to witness history
// copy the bitmask that is support by 5vt voip_bitmask & accessed by omci_bitmask to result
// so this function is to turn omci access_mask to voip access mask
// after calling this function, the result_bitmask will be passed to voip functions to indicate which attr need to be accessed
// and if the access is okay, the related bit will be changed from 1 to 0 by voip functions
void
voip_hw_parm_bitmask_cast(unsigned char omci_bitmask[2], unsigned char voip_bitmask[2], unsigned char result_bitmask[2])
{
	int i, j;
	unsigned long long omci_bit, voip_bit, result_bit;

	if (omci_bitmask == NULL || voip_bitmask == NULL || result_bitmask == NULL)
		return;

	memset(result_bitmask, 0x00, 2);

	for (i = 0, j = 0; i < 16; i++) {
		omci_bit = util_bitmap_get_value(omci_bitmask, 8 * 2, i, 1);
		voip_bit = util_bitmap_get_value(voip_bitmask, 8 * 2, i, 1);
		if (voip_bit) {
			result_bit = omci_bit & voip_bit;
			util_bitmap_set_value(result_bitmask, 8 * 2, j, 1, result_bit);
			j++;
		}
	}

	return;
}

// obsolete:stay to witness history
// before calling this function, result_bitmask was changed from 1 to 0 by voip functions which means an attr is accessed ok.
// copy the bitmask that is support by 5vt voip_bitmask & access failed in result_bitmask to omci_bitmask
// so this function is to turn voip result_mask to omci result_mask
// ps: attr_mask in omci request msg means an attr will be accessed
//     option_mask in omci reply msg means an attr is not supported
//     result_mask in omci reply msg means an attr access is failed
void
voip_hw_parm_bitmask_cast_reverse(unsigned char result_bitmask[2], unsigned char voip_bitmask[2], unsigned char omci_bitmask[2])
{
	int i, j, k;
	unsigned long long voip_bit, result_bit;

	if (omci_bitmask == NULL || voip_bitmask == NULL || result_bitmask == NULL)
		return;

	memset(omci_bitmask, 0x00, 2);

	for (i = 0; i < 16; i++)
	{
		if ((result_bit = util_bitmap_get_value(result_bitmask, 8 * 2, i, 1)))
		{
			for (j = 0, k = 0; j < 16; j++)
			{
				if ((voip_bit = util_bitmap_get_value(voip_bitmask, 8 * 2, j, 1)))
				{
					if (k == i)
					{
						util_bitmap_set_value(omci_bitmask, 8 * 2, j, 1, voip_bit);
						break;
					}
					k++;
				}
			}
		}
	}

	return;
}

//return: 0, success, <0, fail
int
voip_hw_init()
{
	memset(&AgentBackup, 0, sizeof(struct voip_hw_sip_agent_config_data_t));
	return voipclient_cmd_init();
}

//return: 0, success, <0, fail
int
voip_hw_destory()
{
	voipclient_cmd_destroy();
	return 0;
}

static char* voip_hw_action_str[] = { "init", "SET", "get", "release" };

//ME 53
//53@1,53@3,53@4,53@5,53@6,53@7,53@8,53@9,53@10,53@11
int
voip_hw_physical_path_termination_point_pots_uni(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_physical_path_termination_point_pots_uni_t *parm)
{
	int ret = 0;
	char mask_r[2] = {0, 0};
	int value;
	char impedance[2];
	int voipImp = 0;
	memset(impedance, 0, sizeof(impedance));

	dbprintf_voip(LOG_WARNING, "action=%s, port=%d, mask=0x%02x%02x\n", voip_hw_action_str[action%4], port, mask[0], mask[1]);

	//1: Administrative_state (1byte,enumeration,RW)
	if (GET_BIT(mask, 0) > 0) {
		/* administrative_state 0  <-> accountenable 1 */
		/* administrative_state 1  <-> accountenable 0 */
		unsigned char *omci_val_p = &(parm->administrative_state);
		unsigned char cli_val;
		Simple_Get_Set_Int("accountenable %d", "accountenable %d %hhu", cli_val);
		if (action == VOIP_HW_ACTION_GET) {
			if(cli_val == 0)
				*omci_val_p = 1;
			else
				*omci_val_p = 0;
		}

		if(ret != 0)
			goto bit0_out;
		SET_BIT(mask_r, 0);
	}
bit0_out:

	// 5: Impedance (1byte,enumeration,RW)
	if (GET_BIT(mask, 4) > 0) {
		/*
		   SI32176       G.984
		   0/1/2         0  C1=0 nF, R1=0 Ohm, R2=600 Ohm.
		   x                      1  C1=0 nF, R1=0 Ohm, R2=900 Ohm.
		   3             2  C1=150 nF, R1=750 Ohm, R2=270 Ohm.
		   8                      3  C1=115 nF, R1=820 Ohm, R2=220 Ohm.
		   x                      4  C1=230 nF, R1=1050 Ohm, R2=320 Ohm.
		 */
		if (action == VOIP_HW_ACTION_GET) {

			if (port == 0) {
				ret = voipclient_cmd_get_fxsringimpedance_0(&value);
			} else if (port == 1) {
				ret = voipclient_cmd_get_fxsringimpedance_1(&value);
			}
			if (value == 0) {
				voipImp = 0;
			} else if (value == 9) {
				voipImp = 1;
			} else if (value == 1) {
				voipImp = 2;
			} else if (value == 3) {
				voipImp = 3;
			} else if (value == 10) {
				voipImp = 4;
			}
			parm->impedance = (unsigned char)voipImp;
		} else {
			ret = 0;
		}

		if(ret != 0)
			goto bit4_out;
		SET_BIT(mask_r, 4);
	}
bit4_out:

	//7: Rx_gain (1byte,signed integer,RW) [-120 ~ 60]
	if (GET_BIT(mask, 6) > 0) {
		int omci_gain;	/* -120 to 60 */
		if (action == VOIP_HW_ACTION_GET) {
			ret = voipclient_cmd_get_rxgaintenthdb(port, &value);
			omci_gain = value;
			if(omci_gain < -120)
				omci_gain = -120;
			else if(omci_gain > 60)
				omci_gain = 60;
			parm->rx_gain = (char)omci_gain;
			dbprintf_voip(LOG_INFO, "parm->rx_gain = %3d\n", parm->rx_gain);
		} else {
			ret = 0;
		}

		if(ret != 0)
			goto bit6_out;
		SET_BIT(mask_r, 6);
	}
bit6_out:

	//8: Tx_gain (1byte,signed integer,RW) [-120 ~ 60]
	if (GET_BIT(mask, 7) > 0) {
		int omci_gain;	/* -120 to 60 */
		if (action == VOIP_HW_ACTION_GET) {
			ret = voipclient_cmd_get_txgaintenthdb(port, &value);
			omci_gain = value;
			if(omci_gain < -120)
				omci_gain = -120;
			else if(omci_gain > 60)
				omci_gain = 60;
			parm->tx_gain = (char)omci_gain;
			dbprintf_voip(LOG_INFO, "parm->tx_gain = %3d\n", parm->tx_gain);
		} else {
			ret = 0;
		}

		if(ret != 0)
			goto bit7_out;
		SET_BIT(mask_r, 7);
	}
bit7_out:

	/*if(GET_BIT(mask,7)>0){
	   if(action == VOIP_HW_ACTION_GET){
	   ret = voipclient_cmd_get_operational_state(&parm->operational_state,port);
	   if( parm->operational_state == '0'){

	   parm->operational_state = 0;
	   }else{
	   parm->operational_state = 1;
	   }
	   }else{
	   ret = 0;
	   }
	   } */
	//11: POTS_holdover_time (2byte,unsigned integer,RW)
	if (GET_BIT(mask, 10) > 0) {
		if (action == VOIP_HW_ACTION_GET) {
			ret = voipclient_cmd_get_hookstate(&parm->hook_state, port);
			if (parm->hook_state == '1') {
				parm->hook_state = 1;
			} else {
				parm->hook_state = 0;
			}
			dbprintf_voip(LOG_WARNING, "get hook state:%d \n", parm->hook_state);
		} else {
			ret = 0;
		}

		if(ret != 0)
			goto bit10_out;
		SET_BIT(mask_r, 10);
	}
bit10_out:

	mask[0] = mask_r[0];
	mask[1] = mask_r[1];

	return 0;
}

/* ME 153 */
//153@2,153@3,153@4,153@5,153@6,153@11,153@12
int
voip_hw_sip_user_data(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_sip_user_data_t *parm)
{
	int ret = 0;
	char mask_r[2] = { 0, 0};
	dbprintf_voip(LOG_WARNING, "action=%s, port=%d, mask=0x%02x%02x\n", voip_hw_action_str[action%4], port, mask[0], mask[1]);

	/* recursive when called with all port */
	if (port == VOIP_HW_GLOBAL_PORT_DEF) {
		int i;
		for (i = 0; i < VOIP_HW_MAX_PORT_NUMBER; i++) {
			mask_r[0] = mask[0];
			mask_r[1] = mask[1];
			ret = voip_hw_sip_user_data(action, i, mask, parm);
			if(ret != 0)
				return ret;
		}
		return 0;
	}
	// 2: User_part_AOR (2byte,pointer,RWS) [classid 157]
	if (GET_BIT(mask, 1) > 0) {
		unsigned char *omci_val_p = parm->user_part_aor;
		int val_len = 376-1;
		if(omci_val_p == NULL)
			goto bit1_out;
		omci_val_p[val_len] = '\0';
		Simple_Get_Set_String("usermap %d", "usermap %d %s", omci_val_p, val_len);
		if(ret != 0)
			goto bit1_out;
		SET_BIT(mask_r, 1);
	}
bit1_out:

	//3: SIP_display_name (25byte,string,RW)
	if (GET_BIT(mask, 2) > 0) {
		unsigned char *omci_val_p = parm->sip_display_name;
		int val_len = sizeof(parm->sip_display_name)-1;	/* 26-1 */
		omci_val_p[val_len] = '\0';
		Simple_Get_Set_String("displayname %d", "displayname %d %s", omci_val_p, val_len);
		if(ret != 0)
			goto bit2_out;
		SET_BIT(mask_r, 2);
	}
bit2_out:

	//4: Username/password (2byte,pointer,RWS) [classid 148]
	if (GET_BIT(mask, 3) > 0) {
		if (parm->username_password == NULL)
			goto bit3_out;

		unsigned char *omci_username1 = parm->username_password->username_1;
		unsigned char *omci_username2 = parm->username_password->username_2;
		const int val_len = 26-1;
		omci_username1[val_len] = '\0';
		omci_username2[val_len] = '\0';

		unsigned char username[51];	/* 25+25+1 */
		const int username_len = sizeof(username)-1;
		username[username_len] = '\0';

		/* username */
		Simple_Get_Set_String("username %d", "username %d %s", username, username_len);
		if(ret != 0)
			goto bit3_out;
		if (action == VOIP_HW_ACTION_GET) {
			strncpy(omci_username1, username, val_len);
			if (strlen(username) >= val_len)
				strncpy(omci_username2, username + val_len, val_len);
		}

		/* password */
		unsigned char *omci_password = parm->username_password->password;
		omci_password[val_len] = '\0';
		Simple_Get_Set_String("userpassword %d", "userpassword %d %s", omci_password, val_len);
		if(ret != 0)
			goto bit3_out;

		SET_BIT(mask_r, 3);
	}
bit3_out:

	//5: Voicemail_server_SIP_URI (2byte,pointer,RWS) [classid 137]
	if (GET_BIT(mask, 4) > 0) {
		if (parm->voicemail_server_sip_uri == NULL)
			goto bit4_out;
		if (parm->voicemail_server_sip_uri->address_pointer == NULL)
			goto bit4_out;

		unsigned char *omci_val = parm->voicemail_server_sip_uri->address_pointer;
		int omci_val_len = 376-1;
		omci_val[omci_val_len] = '\0';
		Simple_Get_Set_String("mwiaccounturi %d", "mwiaccounturi %d %s", omci_val, omci_val_len);
		if(ret != 0)
			goto bit4_out;
		SET_BIT(mask_r, 4);
	}
bit4_out:

	//6: Voicemail_subscription_expiration_time (4byte,unsigned integer,RWS)
	if (GET_BIT(mask, 5) > 0) {
		Simple_Get_Set_Int("mwisubscribetimeout %d", "mwisubscribetimeout %d %d", parm->voicemail_subscription_expiration_time);
		if(ret != 0)
			goto bit5_out;
		SET_BIT(mask_r, 5);
	}
bit5_out:

	//11: Release_timer (1byte,unsigned integer,RW)
	if (GET_BIT(mask, 10) > 0) {
		Simple_Get_Set_Int("releasetimeout %d", "releasetimeout %d %hhu", parm->release_timer);
		if(ret != 0)
			goto bit10_out;
		SET_BIT(mask_r, 10);
	}
bit10_out:

	//12: ROH_timer (1byte,unsigned integer,RW)
	if (GET_BIT(mask, 11) > 0) {
		Simple_Get_Set_Int("rohtimeout %d", "rohtimeout %d %hhu", parm->roh_timer);
		if(ret != 0)
			goto bit11_out;
		SET_BIT(mask_r, 11);
	}
bit11_out:

	
	if (action == VOIP_HW_ACTION_RELEASE) {
	}

	mask[0] = mask_r[0];
	mask[1] = mask_r[1];
	
	return 0;
}

/* ME 150 */
//150@1,150@2,150@3,150@4,150@5,150@6,150@7,150@8,150@9,150@10,150@11
int
voip_hw_sip_agent_config_data(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_sip_agent_config_data_t *parm)
{
	int ret = 0;		/*jone20110419:for FiberHome OMCI error */
	unsigned char mask_r[2] = {0, 0};
	static unsigned char is_proxy_server_address_has_port=0, is_outbound_proxy_server_address_has_port=0, is_sip_registrar_has_port=0;

	dbprintf_voip(LOG_WARNING, "action=%s, port=%d, mask=0x%02x%02x\n", voip_hw_action_str[action%4], port, mask[0], mask[1]);

	/* recursive when called with all port */
	if (port == VOIP_HW_GLOBAL_PORT_DEF) {
		int i;
		for (i = 0; i < VOIP_HW_MAX_PORT_NUMBER; i++) {
			mask_r[0] = mask[0];
			mask_r[1] = mask[1];
			ret = voip_hw_sip_agent_config_data(action, i, mask_r, parm);
			if(ret != 0)
				return ret;
		}
		return 0;
	}

	//1: Proxy_server_address_pointer (2byte,pointer,RWS) [classid 157]
	if (GET_BIT(mask, 0) > 0) {
		unsigned char *omci_val_p = parm->proxy_server_address_pointer;
		int val_len = 376-1;

		if(omci_val_p == NULL)
			goto bit0_out;

		omci_val_p[val_len] = '\0';
		/* we only want the IP part */
		if (action == VOIP_HW_ACTION_GET) {
			unsigned char *pos;
			ret = voipclient_cmd_get_string(omci_val_p, val_len, "sipproxy %d", port);
			if(ret != 0)
				goto bit0_out;

			if(is_proxy_server_address_has_port==0) {
				pos = strrchr(omci_val_p, ':');
				if(pos != NULL)
					*pos = '\0';
			}
		}
		SET_BIT(mask_r, 0);
	}
bit0_out:

	//2: Outbound_proxy_address_pointer (2byte,pointer,RWS) [classid 157]
	if (GET_BIT(mask, 1) > 0) {
		unsigned char *omci_val_p = parm->outbound_proxy_address_pointer;
		int val_len = 376-1;

		if(omci_val_p == NULL) {
			goto bit1_out;
		}
		omci_val_p[val_len] = '\0';
		ret = 0;

		if (action == VOIP_HW_ACTION_GET) {
			unsigned char *pos;
			ret = voipclient_cmd_get_string(omci_val_p, val_len, "outboundproxy %d", port);
			if(ret != 0)
				goto bit1_out;
			
			if( is_outbound_proxy_server_address_has_port==0) {
				/* we only want the IP part */
				pos = strrchr(omci_val_p, ':');
				if(pos != NULL)
					*pos = '\0';
			}
		}
		SET_BIT(mask_r, 1);
	}
bit1_out:

	//3: Primary_SIP_DNS (4byte,unsigned integer-ipv4,RWS)
	if (GET_BIT(mask, 2) > 0) {
		unsigned int *omci_val_p = &(parm->primary_sip_dns);
		unsigned char ip_string[32];
		int ip_string_len = sizeof(ip_string)-1;
		ip_string[ip_string_len] = '\0';

		struct in_addr MAYBE_UNUSED in;
		in.s_addr = htonl(*omci_val_p);

		ret = 0;
		if (action == VOIP_HW_ACTION_GET) {
			//ret = voipclient_cmd_get_string(ip_string, ip_string_len, "dnsservers %d 0", port);
			ret = voipclient_cmd_get_string(ip_string, ip_string_len, "dnsservers 0");
		}
		if(ret != 0)
			goto bit2_out;

		/* we only want the IP part */
		if (action == VOIP_HW_ACTION_GET) {
			unsigned char *pos;
			pos = strchr(ip_string, ':');
			if(pos != NULL)
				*pos = '\0';
			*omci_val_p = ntohl(inet_addr(ip_string));
		}
		SET_BIT(mask_r, 2);
	}
bit2_out:

	//4: Secondary_SIP_DNS (4byte,unsigned integer-ipv4,RWS)
	if (GET_BIT(mask, 3) > 0) {
		unsigned int *omci_val_p = &(parm->secondary_sip_dns);
		unsigned char ip_string[32];
		int ip_string_len = sizeof(ip_string)-1;
		ip_string[ip_string_len] = '\0';

		struct in_addr MAYBE_UNUSED in;
		in.s_addr = htonl(*omci_val_p);

		ret = 0;
		if (action == VOIP_HW_ACTION_GET) {
			//ret = voipclient_cmd_get_string(ip_string, ip_string_len, "dnsservers %d 1", port);
			ret = voipclient_cmd_get_string(ip_string, ip_string_len, "dnsservers 1");
		}
		if(ret != 0)
			goto bit3_out;

		/* we only want the IP part */
		if (action == VOIP_HW_ACTION_GET) {
			unsigned char *pos;
			pos = strchr(ip_string, ':');
			if(pos != NULL)
				*pos = '\0';
			*omci_val_p = ntohl(inet_addr(ip_string));
		}
		SET_BIT(mask_r, 3);
	}
bit3_out:

	//5: TCP/UDP_pointer (2byte,pointer,RW) [classid 136]
	if (GET_BIT(mask, 4) > 0) {
		if (parm->tcp_udp_pointer == NULL)
			goto bit4_out;

		/* protocol */
		Simple_Get_Set_Int("transportprotocol %d", "transportprotocol %d %hhu", parm->tcp_udp_pointer->protocol);
		if(ret != 0)
			goto bit4_out;

		/* ip */
		if (parm->tcp_udp_pointer->ip_host_pointer == NULL)
			goto bit4_out;

		unsigned int *omci_ip_p = &(parm->tcp_udp_pointer->ip_host_pointer->ip_address);
		unsigned char ip_string[32];
		int ip_string_len = sizeof(ip_string)-1;
		ip_string[ip_string_len] = '\0';

		struct in_addr MAYBE_UNUSED in;
		in.s_addr = htonl(*omci_ip_p);

		Simple_Get_Set_String("ipaddress %d", "ipaddress %d %s", ip_string, ip_string_len);
		if(ret != 0)
			goto bit4_out;
		if (action == VOIP_HW_ACTION_GET)
			*omci_ip_p = ntohl(inet_addr(ip_string));

		SET_BIT(mask_r, 4);
	}
bit4_out:

	//6: SIP_reg_exp_time (4byte,unsigned integer,RW)
	if (GET_BIT(mask, 5) > 0) {
		Simple_Get_Set_Int("registrationtimeout %d", "registrationtimeout %d %d", parm->sip_reg_exp_time);
		if(ret != 0)
			goto bit5_out;
		SET_BIT(mask_r, 5);
	}
bit5_out:

	//7: SIP_rereg_head_start_time (4byte,unsigned integer,RW)
	if (GET_BIT(mask, 6) > 0) {
		Simple_Get_Set_Int("sipreregistrationheadstarttime %d", "sipreregistrationheadstarttime %d %d", parm->sip_rereg_head_start_time);
		if(ret != 0)
			goto bit6_out;
		SET_BIT(mask_r, 6);
	}
bit6_out:

	//8: Host_part_URI (2byte,pointer,RWS) [classid 157]
	if (GET_BIT(mask, 7) > 0) {
		unsigned char *omci_val_p = parm->host_part_uri;
		int val_len = 376-1;
		if(omci_val_p == NULL)
			goto bit7_out;
		omci_val_p[val_len] = '\0';
		Simple_Get_Set_String("sipdomainname %d", "sipdomainname %d %s", omci_val_p, val_len);
		if(ret != 0)
			goto bit7_out;
		/* we only want the IP part */
		if (action == VOIP_HW_ACTION_GET) {
			unsigned char *pos;
			pos = strchr(omci_val_p, ':');
			if(pos != NULL)
				*pos = '\0';
		}
		SET_BIT(mask_r, 7);
	}
bit7_out:

	//9: SIP_status (1byte,enumeration,R)
	if (GET_BIT(mask, 8) > 0) {
		/* get only */
		unsigned char *omci_val_p = &(parm->sip_status);
		ret = 0;
		if (action == VOIP_HW_ACTION_GET) {
			int temp_int;
			ret = voipclient_cmd_get_int(&temp_int, "epregstate %d", port);
			if (ret == 0)
				*omci_val_p = temp_int;
		}
		if(ret != 0)
			goto bit8_out;
		if (action == VOIP_HW_ACTION_GET) {
			/* map value for 9.9.11 to 9.9.3 */
			switch (*omci_val_p) {
			case PORT_STATUS__INITIAL:	*omci_val_p = SIP_STATUS__INITIAL;
				break;
			case PORT_STATUS__REGISTERED:	*omci_val_p = SIP_STATUS__REGISTERED;
				break;
			case PORT_STATUS__IN_SESSION:	*omci_val_p = SIP_STATUS__REGISTERED;
				break;
			case PORT_STATUS__FAILED_ICMP:	*omci_val_p = SIP_STATUS__FAILED_ICMP;
				break;
			case PORT_STATUS__FAILED_TCP:	*omci_val_p = SIP_STATUS__FAILED_TCP;
				break;
			case PORT_STATUS__FAILED_AUTH:	*omci_val_p = SIP_STATUS__FAILED_AUTH;
				break;
			case PORT_STATUS__FAILED_TIMEOUT: *omci_val_p = SIP_STATUS__FAILED_TIMEOUT;
				break;
			case PORT_STATUS__FAILED_SERVER: *omci_val_p = SIP_STATUS__FAILED_SERVER;
				break;
			case PORT_STATUS__FAILED_OTHER: *omci_val_p = SIP_STATUS__FAILED_OTHER;
				break;
			case PORT_STATUS__REGISTERING: *omci_val_p = SIP_STATUS__REGISTERING;
				break;
			case PORT_STATUS__UNREGISTERING: *omci_val_p = SIP_STATUS__UNREGISTERING;
				break;
			default:
				*omci_val_p = SIP_STATUS__FAILED_AUTH;
			}
		}
		SET_BIT(mask_r, 8);
	}
bit8_out:

	//10: SIP_registrar (2byte,pointer,RWS) [classid 137]
	if (GET_BIT(mask, 9) > 0) {
		/* for now, we ignore set the username/password */
		if (parm->sip_registrar == NULL)
			goto bit9_out;
		if (parm->sip_registrar->security_pointer == NULL)
			goto bit9_registar;

		unsigned char *omci_username1 = parm->sip_registrar->security_pointer->username_1;
		unsigned char *omci_username2 = parm->sip_registrar->security_pointer->username_2;
		int val_len = 26-1;
		omci_username1[val_len] = '\0';
		omci_username2[val_len] = '\0';

		unsigned char username[51];	/* 25+25+1 */
		const int username_len = sizeof(username)-1;
		username[username_len] = '\0';

		ret = 0;
		/* username */
		if (action == VOIP_HW_ACTION_GET)
			ret = voipclient_cmd_get_string(username, username_len, "username %d", port);
		if(ret != 0)
			goto bit9_out;
		if (action == VOIP_HW_ACTION_GET) {
			strncpy(omci_username1, username, val_len);
			if (strlen(username) >= val_len)
				strncpy(omci_username2, username + val_len, val_len);
		}

		/* password */
		unsigned char *omci_password = parm->sip_registrar->security_pointer->password;
		omci_password[val_len] = '\0';
		if (action == VOIP_HW_ACTION_GET)
			ret = voipclient_cmd_get_string(omci_password, val_len, "userpassword %d", port);
		if(ret != 0)
			goto bit9_out;

bit9_registar:	; //This is an empty statement.
		/* sipregistrar */
		unsigned char *omci_val_p = parm->sip_registrar->address_pointer;
		val_len = 376-1;
		if(omci_val_p == NULL)
			goto bit9_out;

		/* we only want the IP part */
		if (action == VOIP_HW_ACTION_GET) {
			unsigned char *pos;
			ret = voipclient_cmd_get_string(omci_val_p, val_len, "sipregistrar %d", port);
			if(ret != 0)
				goto bit9_out;
			if(is_sip_registrar_has_port==0) {
				pos = strrchr(omci_val_p, ':');
				if(pos != NULL)
					*pos = '\0';
			}
		}

		SET_BIT(mask_r, 9);
	}
bit9_out:

	//11: Softswitch (4byte,string,RWS)
	if (GET_BIT(mask, 10) > 0) {
		/* not support this CLI yet */
		ret=-1;
		goto bit10_out;
	}

bit10_out:

	mask[0] = mask_r[0];
	mask[1] = mask_r[1];

	return ret;
}

//ME 139
//139@4
int
voip_hw_voip_voice_ctp(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_voip_voice_ctp_t *parm)
{
	char type[2];
	unsigned char mask_r[2] = {0, 0};

	dbprintf_voip(LOG_WARNING, "action=%s, port=%d, mask=0x%02x%02x\n", voip_hw_action_str[action%4], port, mask[0], mask[1]);

	memset(type, 0, sizeof(type));


	if (GET_BIT(mask, 0) > 0) {
		/*user_protocol_pointer */
	}

	if (GET_BIT(mask, 1) > 0) {
		/*voip_media_profile_pointer */
	}

	if (GET_BIT(mask, 2) > 0) {
		/*pptp_pointer */
	}

	// 4: Signalling_code (1byte,enumeration,RWS)
	if (GET_BIT(mask, 3) > 0) {
		if (action == VOIP_HW_ACTION_GET) {
			parm->signalling_code = 1;
		}
		SET_BIT(mask_r, 3);
	}

	mask[0] = mask_r[0];
	mask[1] = mask_r[1];
	
	return 0;
}

/* ME 142 */
//,142@1,142@3,142@4,142@5,142@6,142@7,142@8,142@9,142@10,142@11,142@12,142@13,142@14,142@15
int
voip_hw_voip_media_profile(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_voip_media_profile_t *parm)
{
	int ret = 0;
	char mask_r[2] = { 0, 0};
	int i=0, j=0, bit, valid_group=0;

	dbprintf_voip(LOG_WARNING, "action=%s, port=%d, mask=0x%02x%02x\n", voip_hw_action_str[action%4], port, mask[0], mask[1]);

	/* recursive when called with all port */
	if (port == VOIP_HW_GLOBAL_PORT_DEF) {
		for (i = 0; i < VOIP_HW_MAX_PORT_NUMBER; i++) {
			mask_r[0] = mask[0];
			mask_r[1] = mask[1];
			ret = voip_hw_voip_media_profile(action, i, mask_r, parm);
			if(ret != 0)
				return ret;
		}
		return 0;
	}

	//1: Fax_mode (1byte,enumeration,RWS)
	if (GET_BIT(mask, 0) > 0) {
		/* pass_through: OMCI=0 VOIP=3 */
		/* T38:          OMCI=1 VOIP=2 */
		unsigned char *omci_val_p = &(parm->fax_mode);
		unsigned char cli_val = 3;	//recommend default
		Simple_Get_Set_Int("t38faxopt %d", "t38faxopt %d %hhu", cli_val);
		if(ret != 0)
			goto bit0_out;
		if (action == VOIP_HW_ACTION_GET) {
			if(cli_val == 2)
				*omci_val_p = 1;
			else
				*omci_val_p = 0;
		}
		SET_BIT(mask_r, 0);
	}
bit0_out:	; //This is an empty statement.

	/* (codec + packet_period + silence_suppression ) x 4 */
	unsigned char* codec_selection[4] = {
		&parm->codec_selection_1st_order, 
		&parm->codec_selection_2nd_order,
		&parm->codec_selection_3rd_order,
		&parm->codec_selection_4th_order};

	unsigned char* packet_period_selection[4] = {
		&parm->packet_period_selection_1st_order,
		&parm->packet_period_selection_2nd_order,
		&parm->packet_period_selection_3rd_order,
		&parm->packet_period_selection_4th_order};
	unsigned char* silence_suppression[4] = {
		&parm->silence_suppression_1st_order,
		&parm->silence_suppression_2nd_order,
		&parm->silence_suppression_3rd_order,
		&parm->silence_suppression_4th_order};

	/* our voip dose not support per codec period and vad */
	//after discuss, we choice 1st_order for our system


	// 1. if any codec need modify, in our system, we should reset all 
	// 2. due to the order represent priority, in our system, we need to remove duplicate codec
	//ex:0,8,18,0 => 0,8,18; 0,8,8,18 => 0,8,18

	unsigned char tmp_codec_selection[4]={255,255,255,255};
	for(i=0; i < 4; i++) {
		int found;
		found=0;
		for(j=0; j < 4; j++ ) {
			if(*codec_selection[i] == tmp_codec_selection[j]) {
				found=1;
				break;
			}
		}
		if( found == 0 ) {
			tmp_codec_selection[valid_group]=*codec_selection[i];
			valid_group++;
		}
	}

	//valid_group=1,2,3,4(0: no valid)
	/*
	3: Codec_selection_1st_order (1byte,enumeration,RWS)
	4: Packet_period_selection_1st_order (1byte,unsigned integer,RWS) [10 ~ 30]
	5: Silence_suppression_1st_order (1byte,enumeration,RWS)
	*/

	for(i=0; i < valid_group; i++) {
		bit = i*3 + 2;	/* attr order:3, 6, 8, 12, bit 2, 5, 8, 11 */

		{	//should redo add all codec
			unsigned char *omci_val_p = codec_selection[i];
			int ret = 0;

			//tmp_codec_selection already remove duplicate codec
			ret = voipclient_cmd_codecpriority(action, port, i, omci_val_p);
			if(ret != 0)
				goto loop_out1;
			SET_BIT(mask_r, bit);
		}
loop_out1:
		if (GET_BIT(mask, (bit+1)) > 0) {
			ret = 0;
			if (action == VOIP_HW_ACTION_GET) {
				int temp_int;
				ret = voipclient_cmd_get_int(&temp_int, "txpacketsize %d", port);
				if (ret == 0)
					*packet_period_selection[i] = temp_int;
			}
			if(ret != 0)
				goto loop_out2;
			SET_BIT(mask_r, (bit+1));
		}
loop_out2:

		if (GET_BIT(mask, (bit+2)) > 0) {
			/* OFF: OMCI=0 VOIP=1 FV_VAD_TYPE_OFF */
			/*	ON: OMCI=1 VOIP=3 FV_VAD_TYPE_ALL */
			unsigned char *omci_val_p = silence_suppression[i];
			unsigned char cli_val;
			ret = 0;
			if (action == VOIP_HW_ACTION_GET) {
				int temp_int;
				ret = voipclient_cmd_get_int(&temp_int, "vadtype %d", port);
				if(ret != 0)
					goto loop_out3;
				cli_val = temp_int;
				switch(cli_val) {
					case 1: *omci_val_p = 0;
						break;
					case 3:	*omci_val_p = 1;
						break;
					default:
						goto loop_out3;
				}
			}
			SET_BIT(mask_r, (bit+2));
		}
loop_out3:	; //This is an empty statement.

	}

	//15: OOB_DTMF (1byte,enumeration,RWS)
	if (GET_BIT(mask, 14) > 0) {
		unsigned char *omci_val_p = &(parm->oob_dtmf);
		ret = 0;
		if (action == VOIP_HW_ACTION_GET) {
			int temp_int;
			ret = voipclient_cmd_get_int(&temp_int, "dtmftype %d", port);
			if (ret != 0)
				goto bit14_out;
			
			if ((omci_get_olt_vendor() == ENV_OLT_WORKAROUND_CALIX)) {
				switch(temp_int) {
					case 0: 	
						*omci_val_p = 0;
					break;
					case 3:	
						*omci_val_p = 1;
					break;
					case 4:	
						*omci_val_p = 2;
					break;
					goto bit14_out;
				}

			} else {
				switch(temp_int) {
					case 0: 	
						*omci_val_p = 0;
					break;
					case 3:	
						*omci_val_p = 1;
					break;
					default:
						goto bit14_out;
				}
			}	
		}
		SET_BIT(mask_r, 14);
	}
bit14_out:

	mask[0] = mask_r[0];
	mask[1] = mask_r[1];

	return 0;
}

/* ME 58 */
//,58@1,58@2,58@3,58@4,58@5,58@6,58@7,58@8,58@9,58@10,58@11,58@12,58@13,58@14
int
voip_hw_voice_service_profile(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_voice_service_profile_t *parm)
{
	int ret = 0;
	char mask_r[2] = { 0, 0};

	dbprintf_voip(LOG_WARNING, "action=%s, port=%d, mask=0x%02x%02x\n", voip_hw_action_str[action%4], port, mask[0], mask[1]);

	/* recursive when called with all port */
	if (port == VOIP_HW_GLOBAL_PORT_DEF) {
		int i;
		char mask_r[2];
		for (i = 0; i < VOIP_HW_MAX_PORT_NUMBER; i++) {
			mask_r[0] = mask[0];
			mask_r[1] = mask[1];
			ret = voip_hw_voice_service_profile(action, i, mask_r, parm);
			if(ret != 0)
				return ret;
		}
		return 0;
	}

	//1: Announcement_type (1byte,enumeration,RWS)
	if (GET_BIT(mask, 0) > 0) {
		/*always 3-fastbusy*/
		unsigned char *omci_var_p = &(parm->announcement_type);
		if (action == VOIP_HW_ACTION_GET) {
			*omci_var_p = 3;
		}
		SET_BIT(mask_r, 0);
	}

	//2: Jitter_target (2byte,unsigned integer,RWS)
	if (GET_BIT(mask, 1) > 0) {
		/* adaptive: OMCI=0            VOIP TYPE=1 */
		/* fixed:    OMCI=preload_size VOIP TYPE=0 DELAY=preload_size */
		unsigned short *omci_var_p = &(parm->jitter_target);
		int cli_var_type, cli_var_delay;
		if (action == VOIP_HW_ACTION_GET) {
			ret = voipclient_cmd_get_int(&cli_var_type, "jbtype %d", port);
			if(ret!=0)
				goto bit1_out;
			if(cli_var_type==0) {
				ret = voipclient_cmd_get_int(&cli_var_delay, "jbnormaldelay %d", port);
				if(ret!=0)
					goto bit1_out;
				*omci_var_p = cli_var_delay;
			}
			else {
				*omci_var_p = 0;
			}
		}
		SET_BIT(mask_r, 1);
	}
bit1_out:

	//3: Jitter_buffer_max (2byte,unsigned integer,RWS)
	if (GET_BIT(mask, 2) > 0) {
		unsigned short *omci_val_p = &(parm->jitter_buffer_max);
		if (action == VOIP_HW_ACTION_GET) {
			int temp_int=300;
			//fvphone set fjbusemode to reference ajb max size
			ret = voipclient_cmd_get_int(&temp_int, "jbcinfiglevel %d", port);
			if(ret != 0)
				goto bit2_out;
			*omci_val_p = temp_int;
		}
		SET_BIT(mask_r, 2);
	}
bit2_out:

	//4: Echo_cancel_ind (1byte,enumeration,RWS)
	if (GET_BIT(mask, 3) > 0) {
		Simple_Get_Set_Int("aecenable %d", "aecenable %d %hhu", parm->echo_cancel_ind);
		if(ret != 0)
			goto bit3_out;
		SET_BIT(mask_r, 3);
	}
bit3_out:

	//5: PSTN_protocol_variant (2byte,unsigned integer,RWS)
	if (GET_BIT(mask, 4) > 0) {
		unsigned short *omci_val_p = &(parm->pstn_protocol_variant);
		if (action == VOIP_HW_ACTION_GET) {
			int temp_int;
			ret = voipclient_cmd_get_int(&temp_int, "e164countrycode");
			if(ret != 0)
				goto bit4_out;
			*omci_val_p = temp_int;
		}
		SET_BIT(mask_r, 4);
	}
bit4_out:

	//6: DTMF_digit_levels (2byte,unsigned integer,RWS)
	if (GET_BIT(mask, 5) > 0) {
		//unsigned short *omci_val_p = &(parm->dtmf_digit_levels);
	}

	//7: DTMF_digit_duration (2byte,unsigned integer,RW
	if (GET_BIT(mask, 6) > 0) {
		//unsigned short *omci_val_p = &(parm->dtmf_digit_duration);
	}

	//8: Hook_flash_minimum_time (2byte,unsigned integer,RWS)
	if (GET_BIT(mask, 7) > 0) {
		unsigned short *omci_val_p = &(parm->hook_flash_minimum_time);
		if (action == VOIP_HW_ACTION_GET) {
			int temp_int;
			ret = voipclient_cmd_get_int(&temp_int, "flashhooktimeoutlb %d", port);
			if(ret != 0)
				goto bit7_out;
			*omci_val_p = temp_int;
		}
		SET_BIT(mask_r, 7);
	}
bit7_out:

	//9: Hook_flash_maximum_time (2byte,unsigned integer,RWS)
	if (GET_BIT(mask, 8) > 0) {
		unsigned short *omci_val_p = &(parm->hook_flash_maximum_time);
		if (action == VOIP_HW_ACTION_GET) {
			int temp_int;
			ret = voipclient_cmd_get_int(&temp_int, "flashhooktimeoutub %d", port);
			if(ret != 0)
				goto bit8_out;
			*omci_val_p = temp_int;
		}
		SET_BIT(mask_r, 8);
	}
bit8_out:

	//10: Tone_pattern_table (20byte,table,RW)
	if (GET_BIT(mask, 9) > 0) {
		//struct list_head *omci_val_p = &(parm->tone_pattern_list);
	}

	//11: Tone_event_table (7byte,table,RW)
	if (GET_BIT(mask, 10) > 0) {
	}

	//12: Ringing_pattern_table (5byte,table,RW)
	if (GET_BIT(mask, 11) > 0) {
		//struct list_head *omci_val_p = &(parm->ringing_pattern_list);
	}

	//13: Ringing_event_table (7byte,table,RW)
	if (GET_BIT(mask, 12) > 0) {
	}

	//14: Network_specific_extensions_pointer (2byte,pointer,RWS) [classid 137]
	if (GET_BIT(mask, 13) > 0) {
		//struct list_head *omci_val_p = &(parm->network_specific_extensions);
	}
	
	mask[0] = mask_r[0];
	mask[1] = mask_r[1];

	return 0;
}

/* ME 143 */
//143@1,143@2,143@3,143@4,143@5,143@6,143@7
int
voip_hw_rtp_profile_data(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_rtp_profile_data_t *parm)
{
	int ret = 0;
	unsigned char mask_r[2] = {0, 0};

	dbprintf_voip(LOG_WARNING, "action=%s, port=%d, mask=0x%02x%02x\n", voip_hw_action_str[action%4], port, mask[0], mask[1]);

	/* recursive when called with all port */
	if (port == VOIP_HW_GLOBAL_PORT_DEF) {
		int i;
		for (i = 0; i < VOIP_HW_MAX_PORT_NUMBER; i++) {
			mask_r[0] = mask[0];
			mask_r[1] = mask[1];
			ret = voip_hw_rtp_profile_data(action, i, mask_r, parm);
			if(ret != 0)
				return ret;
		}
		return 0;
	}

	//1: Local_port_min (2byte,unsigned integer,RWS)
	if (GET_BIT(mask, 0) > 0) {
		unsigned short *omci_val_p = &(parm->local_port_min);
		ret = 0;
		if (action == VOIP_HW_ACTION_GET) {
			int temp_int;
			ret = voipclient_cmd_get_int(&temp_int, "rtpportbase");
			if (ret == 0)
				*omci_val_p = temp_int;
		}
		if(ret != 0)
			goto bit0_out;
		SET_BIT(mask_r, 0);
	}
bit0_out:

	//2: Local_port_max (2byte,unsigned integer,RWS)
	if (GET_BIT(mask, 1) > 0) {
		unsigned short *omci_val_p = &(parm->local_port_max);
		ret = 0;
		if (action == VOIP_HW_ACTION_GET) {
			int temp_int;
			ret = voipclient_cmd_get_int(&temp_int, "rtpportlimit");
			if (ret == 0)
				*omci_val_p = temp_int;
		}
		if(ret != 0)
			goto bit1_out;
		SET_BIT(mask_r, 1);
	}
bit1_out:

	//3: DSCP_mark (1byte,unsigned integer,RWS)
	if (GET_BIT(mask, 2) > 0) {
		unsigned char *omci_val_p = &(parm->dscp_mark);
		ret = 0;
		if (action == VOIP_HW_ACTION_GET) {
			int temp_int;
			ret = voipclient_cmd_get_int(&temp_int, "mediatos");
			if (ret == 0)
				*omci_val_p = (temp_int >> 2);
		}
		if(ret != 0)
			goto bit2_out;
		SET_BIT(mask_r, 2);
	}
bit2_out:

	mask[0] = mask_r[0];
	mask[1] = mask_r[1];

	return 0;
}

//ME 146
//146@1,146@2,146@3,146@4,146@5,146@6,146@7,146@8,146@9
int
voip_hw_voip_application_service_profile(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_voip_application_service_profile_t *parm)
{
	unsigned char mask_r[2] = {0, 0};
	char buf[256];

	dbprintf_voip(LOG_WARNING, "action=%s, port=%d, mask=0x%02x%02x\n", voip_hw_action_str[action%4], port, mask[0], mask[1]);

	memset(buf, 0, 256);

	//1: CID_features (1byte,bit field,RWS)
	if (GET_BIT(mask, 0) > 0) {
		if (action == VOIP_HW_ACTION_GET) {
			//int numFlag, nameFlag, anonymousFlag, anonymousRejectFlag;
			int cidtype, nameflag, cidblocking;
			//calling number
			voipclient_cmd_get_cidtype(port, &cidtype);
			parm->cid_features.calling_number = (cidtype & 0x1);

			//Calling name
			voipclient_cmd_get_anonymous_name(port, &nameflag);
			parm->cid_features.calling_name = nameflag;

			//CID block
			voipclient_cmd_get_cidblocking(port, &cidblocking);
			parm->cid_features.cid_blocking = (cidblocking & 0x01);

			//anonymous CID block
			voipclient_cmd_get_cidblocking(port, &cidblocking);
			parm->cid_features.anonymous_cid_blocking = ((cidblocking & 0x02)>>1);
		}
		SET_BIT(mask_r, 0);
	}

	//2: Call_waiting_features (1byte,bit field,RWS)
	if (GET_BIT(mask, 1) > 0) {
		if (action == VOIP_HW_ACTION_GET) {
			int cidtype = 0;
			int waitingFlag, announcementFlag;
			//Call waiting	
			voipclient_cmd_get_callwaitingopt(port, &waitingFlag);
			parm->call_waiting_features.call_waiting = waitingFlag;

			//Caller ID announcement
			voipclient_cmd_get_cidtype(port, &cidtype);
			announcementFlag= (cidtype | 0x2) >> 1;
			parm->call_waiting_features.caller_id_announcement= announcementFlag;
		}
		SET_BIT(mask_r, 1);
	}

	//3: Call_progress_or_transfer_features (2byte,bit field,RWS)
	if (GET_BIT(mask, 2) > 0) {
		dbprintf_voip(LOG_WARNING, "Attribute '%s' is skipped to preserve local setting.\n",
				 "Call_progress_or_transfer_features");
		char dndOpt[2];
		//char callholdOpt[2];
		char calltransOPt[2];
		char threewayOpt[2];
		char flash_on_emergency_service_callOpt[2];

		memset(dndOpt, 0, sizeof(dndOpt));
		//memset(callholdOpt, 0, sizeof(callholdOpt));
		memset(calltransOPt, 0, sizeof(calltransOPt));
		memset(threewayOpt, 0, sizeof(threewayOpt));
		memset(flash_on_emergency_service_callOpt, 0, sizeof(flash_on_emergency_service_callOpt));

		sprintf(dndOpt, "%d", parm->call_progress_or_transfer_features.do_not_disturb);
		//sprintf(callholdOpt, "%d", parm->call_progress_or_transfer_features.call_hold);
		if (0 == parm->call_progress_or_transfer_features.call_transfer) {
			sprintf(calltransOPt, "0");
		} else {
			sprintf(calltransOPt, "7");
		}
		sprintf(threewayOpt, "%d", parm->call_progress_or_transfer_features.threeway);
		sprintf(flash_on_emergency_service_callOpt, "%d", parm->call_progress_or_transfer_features.flash_on_emergency_service_call);

		if (action == VOIP_HW_ACTION_GET) {
			int dndFlag;
			int calltransFlag;
			//int callholdFlag;
			int threewayFlag;
			int flash_on_emergency_service_callFlag;

			voipclient_cmd_get_dnd(port, &dndFlag);
			voipclient_cmd_get_transferopt(port, &calltransFlag);
			//voipclient_cmd_get_callhold(port, &callholdFlag);
			voipclient_cmd_get_threeway(port, &threewayFlag);
			voipclient_cmd_get_flash_on_emergency_service_call(port, &flash_on_emergency_service_callFlag);

			parm->call_progress_or_transfer_features.do_not_disturb = dndFlag;
			if (0 == calltransFlag) {
				parm->call_progress_or_transfer_features.call_transfer = 0;
			} else {
				parm->call_progress_or_transfer_features.call_transfer = 1;
			}
			//parm->call_progress_or_transfer_features.call_hold = callholdFlag;
			parm->call_progress_or_transfer_features.threeway = threewayFlag;
			parm->call_progress_or_transfer_features.flash_on_emergency_service_call= flash_on_emergency_service_callFlag;
		}
		SET_BIT(mask_r, 2);
	}

	//4: Call_presentation_features (2byte,bit field,RWS)
	if (GET_BIT(mask, 3) > 0) {
		dbprintf_voip(LOG_WARNING, "Attribute '%s' is skipped to preserve local setting.\n",
				 "Call_presentation_features");
		/*call_presentation_features */
		//char mwisropt[2];
		//char mwisdtopt[2];
		char mwiviopt_str[3];
		//char cfiopt[2];
		//memset(mwisropt, 0, 2);
		//memset(mwisdtopt,0,2);

		memset(mwiviopt_str,0,3);
		//memset(cfiopt, 0, 2);
		//sprintf(mwisropt,"%d",parm->call_presentation_features.message_waiting_indication_splash_ring);
		/* OMCI <-> fvcli
		MWI special dial tone 0x0002 <-> mwitype [EP] 0x01
		MWI visual indication 0x0004 <-> mwitype [EP] 0x04 */

		if (action == VOIP_HW_ACTION_GET) {
			//int mwisrFlg,mwisdtFlg,cfiFlg;
			int mwiviFlg=0;

			//voipclient_cmd_get_mwisr(port,&mwisrFlg);
			//voipclient_cmd_get_mwisdt(port,&mwisdtFlg);
			voipclient_cmd_get_mwivi(port,&mwiviFlg);
			//voipclient_cmd_get_callforwardaction(port, &cfiFlg);
			//parm->call_presentation_features.message_waiting_indication_splash_ring = mwisrFlg;
			//parm->call_presentation_features.message_waiting_indication_special_dial_tone = mwisdtFlg;

			if( mwiviFlg & 0x01)
				parm->call_presentation_features.message_waiting_indication_special_dial_tone = 1;

			if ( mwiviFlg & 0x04)
				parm->call_presentation_features.message_waiting_indication_visual_indication = 1;

			//parm->call_presentation_features.call_forwarding_indication = cfiFlg;

		}
		SET_BIT(mask_r, 3);
	}

	//5: Direct_connect_feature (1byte,bit field,RWS)
	if (GET_BIT(mask, 4) > 0) {
		/*direct_connect_feature */
		if (action == VOIP_HW_ACTION_GET) {
			int dcfeFlg=0;
			voipclient_cmd_get_dcfe(port, &dcfeFlg);

			if(dcfeFlg == 0) {
				parm->direct_connect_feature.direct_connect_feature_enabled = 0;
				parm->direct_connect_feature.dial_tone_feature_delay_option = 0;
			} else if(dcfeFlg == 1) {
				parm->direct_connect_feature.direct_connect_feature_enabled = 1;
				parm->direct_connect_feature.dial_tone_feature_delay_option = 0;
			} else if(dcfeFlg == 2) {
				//can't derivation all bits
				goto bit4_out;
				//parm->direct_connect_feature.direct_connect_feature_enabled = 1;
				parm->direct_connect_feature.dial_tone_feature_delay_option = 1;
			}
		}
		SET_BIT(mask_r, 4);
	}
bit4_out:

	//6: Direct_connect_URI_pointer (2byte,pointer,RWS) [classid 137]
	if (GET_BIT(mask, 5) > 0) {
		/*direct_connect_uri_pointer */

		if (parm->direct_connect_uri_pointer == NULL) {
			goto bit5_out;
		}
		if (parm->direct_connect_uri_pointer->address_pointer == NULL) {
			goto bit5_out;
		}

		if (action == VOIP_HW_ACTION_GET) {
			voipclient_cmd_get_directconnecturi(port, parm->direct_connect_uri_pointer->address_pointer);
		}
		SET_BIT(mask_r, 5);
	}
bit5_out:

	//7: Bridged_line_agent_URI_pointer (2byte,pointer,RWS) [classid 137]
	if (GET_BIT(mask, 6) > 0) {
		/*bridged_line_agent_uri_pointer */
		if (parm->bridged_line_agent_uri_pointer == NULL) {
			goto bit6_out;
		}
		if (parm->bridged_line_agent_uri_pointer->address_pointer == NULL) {
			goto bit6_out;
		}
		if (action == VOIP_HW_ACTION_GET) {
			voipclient_cmd_get_bridgedlineagenturi(port, parm->bridged_line_agent_uri_pointer->address_pointer);
		}
		SET_BIT(mask_r, 6);
	}
bit6_out:

	//8: Conference_factory_URI_pointer (2byte,pointer,RWS) [classid 137]
	if (GET_BIT(mask, 7) > 0) {
		/*conference_factory_uri_pointer */
		if (parm->conference_factory_uri_pointer == NULL) {
			goto bit7_out;
		}
		if (parm->direct_connect_uri_pointer == NULL) {
			goto bit7_out;
		}
		if (parm->direct_connect_uri_pointer->address_pointer == NULL) {
			goto bit7_out;
		}
		if (action == VOIP_HW_ACTION_GET) {
			voipclient_cmd_get_conferencefactoryuri(port, parm->conference_factory_uri_pointer->address_pointer);
		}
		SET_BIT(mask_r, 7);
	}
bit7_out:

	//9: Dial_tone_feature_delay_Warmline_timer (2byte,unsigned integer,RW)
	if (GET_BIT(mask, 8) > 0) {
		if (action == VOIP_HW_ACTION_GET) {
			int delay_timer;
			voipclient_cmd_get_dial_tone_feature_delay_timer(port, &delay_timer);
			parm->voip_hw_dial_tone_feature_delay_timer = delay_timer;
		}
		SET_BIT(mask_r, 8);
	}

	mask[0] = mask_r[0];
	mask[1] = mask_r[1];

	return 0;
}

//ME 147
//147@1,147@2,147@3,147@4,147@5,147@6,147@7,147@8,147@9,147@10
int
voip_hw_voip_feature_access_codes(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_voip_feature_access_codes_t *parm)
{
	int ret = 0;
	char tmp[255];
	unsigned char mask_r[2] = {0, 0};

	dbprintf_voip(LOG_WARNING, "action=%s, port=%d, mask=0x%02x%02x\n", voip_hw_action_str[action%4], port, mask[0], mask[1]);

	//1: Cancel_call_waiting (5byte,string,RW)
	if (GET_BIT(mask, 0) > 0) {
		if (action == VOIP_HW_ACTION_GET) {
			memset(tmp, 0, sizeof(tmp));
			ret = voipclient_cmd_get_callwaitingcancelkeys(tmp);
			if (ret == 0) {
				strncpy(parm->cancel_call_waiting, tmp, sizeof(parm->cancel_call_waiting));
			}
		} else {
			ret = 0;
		}
		SET_BIT(mask_r, 0);
	}

	//2: Call_hold (5byte,string,RW)
	if (GET_BIT(mask, 1) > 0) {
		if (action == VOIP_HW_ACTION_GET) {
			ret = voipclient_cmd_get_holdcallkeys(tmp);
			if (ret == 0) {
				strncpy(parm->call_hold, tmp, sizeof(parm->call_hold));
			}
		} else {
			ret = 0;
		}
		SET_BIT(mask_r, 1);
	}

	//3: Call_park (5byte,string,RW)
	if (GET_BIT(mask, 2) > 0) {
		/*call_park */
		if (action == VOIP_HW_ACTION_GET) {
			memset(tmp, 0, sizeof(tmp));
			ret = voipclient_cmd_get_callparkkeys(tmp);
			if (ret == 0) {
				strncpy(parm->call_park, tmp, sizeof(parm->call_park));
			}
		} else {
			ret = 0;
		}
		SET_BIT(mask_r, 2);
	}

	//4: Caller_ID_activate (5byte,string,RW)
	if (GET_BIT(mask, 3) > 0) {
		if (action == VOIP_HW_ACTION_GET) {
			ret = voipclient_cmd_get_cidblockactivekeys(tmp);
			if (ret == 0) {
				strncpy(parm->caller_id_activate, tmp, sizeof(parm->caller_id_activate));
			}
		} else {
			ret = 0;
		}
		SET_BIT(mask_r, 3);
	}

	//5: Caller_ID_deactivate (5byte,string,RW)
	if (GET_BIT(mask, 4) > 0) {
		if (action == VOIP_HW_ACTION_GET) {
			ret = voipclient_cmd_get_cidblockdeactivekeys(tmp);
			if (ret == 0) {
				strncpy(parm->caller_id_deactivate, tmp, sizeof(parm->caller_id_deactivate));
			}
		} else {
			ret = 0;
		}
		SET_BIT(mask_r, 4);
	}

	//6: Do_not_disturb_activation (5byte,string,RW)
	if (GET_BIT(mask, 5) > 0) {
		if (action == VOIP_HW_ACTION_GET) {
			ret = voipclient_cmd_get_dndactivekeys(tmp);
			if (ret == 0) {
				strncpy(parm->do_not_disturb_activation, tmp, sizeof(parm->do_not_disturb_activation));
			}
		} else {
			ret = 0;
		}
		SET_BIT(mask_r, 5);
	}

	//7: Do_not_disturb_deactivation (5byte,string,RW)
	if (GET_BIT(mask, 6) > 0) {
		if (action == VOIP_HW_ACTION_GET) {
			ret = voipclient_cmd_get_dnddeactivekeys(tmp);
			if (ret == 0) {
				strncpy(parm->do_not_disturb_deactivation, tmp, sizeof(parm->do_not_disturb_deactivation));
			}
		} else {
			ret = 0;
		}
		SET_BIT(mask_r, 6);
	}

	//8: Do_not_disturb_PIN_change (5byte,string,RW)
	if (GET_BIT(mask, 7) > 0) {
		/*do_not_disturb_pin_change */
		if (action == VOIP_HW_ACTION_GET) {
			memset(tmp, 0, sizeof(tmp));
			ret = voipclient_cmd_get_dndpinchange(tmp);
			if (ret == 0) {
				strncpy(parm->do_not_disturb_pin_change, tmp, sizeof(parm->do_not_disturb_pin_change));
			}
		} else {
			ret = 0;
		}
		SET_BIT(mask_r, 7);
	}

	//9: Emergency_service_number (5byte,string,RW)
	if (GET_BIT(mask, 8) > 0) {
		/*emergency_service_number */
		if (action == VOIP_HW_ACTION_GET) {
			memset(tmp, 0, sizeof(tmp));
			ret = voipclient_cmd_get_emergencyservicenumber(tmp);
			if (ret == 0) {
				strncpy(parm->emergency_service_number, tmp, sizeof(parm->emergency_service_number));
			}
		} else {
			ret = 0;
		}
		SET_BIT(mask_r, 8);
	}

	//10: Intercom_service (5byte,string,RW)
	if (GET_BIT(mask, 9) > 0) {
		/*intercom_service */
		if (action == VOIP_HW_ACTION_GET) {
			memset(tmp, 0, sizeof(tmp));
			ret = voipclient_cmd_get_intercomservice(tmp);
			if (ret == 0) {
				strncpy(parm->intercom_service, tmp, sizeof(parm->intercom_service));
			}
		} else {
			ret = 0;
		}
		SET_BIT(mask_r, 9);
	}

	mask[0] = mask_r[0];
	mask[1] = mask_r[1];

	return 0;
}

/* ME 145 */
//145@1,145@2,145@3,145@4,145@5,145@6

int
voip_hw_network_dial_plan_table(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_network_dial_plan_table_t *parm)
{
	int ret = 0;
	unsigned char mask_r[2] = {0, 0};

	dbprintf_voip(LOG_WARNING, "action=%s, port=%d, mask=0x%02x%02x\n", voip_hw_action_str[action%4], port, mask[0], mask[1]);

	/* recursive when called with all port */
	if (port == VOIP_HW_GLOBAL_PORT_DEF) {
		int i;
		for (i = 0; i < VOIP_HW_MAX_PORT_NUMBER; i++) {
			mask_r[0] = mask[0];
			mask_r[1] = mask[1];
			ret = voip_hw_network_dial_plan_table(action, i, mask_r, parm);
			if(ret != 0)
				return ret;
		}
		return 0;
	}

	//1: Dial_plan_number (2byte,unsigned integer,R)
	if (GET_BIT(mask, 0) > 0) {
		/* get only */
		unsigned short *omci_val_p = &(parm->dial_plan_number);
		if (action == VOIP_HW_ACTION_GET) {
			int temp_int;
			ret = voipclient_cmd_get_int(&temp_int, "digitmapnum %d", port);
			if (ret != 0)
				goto bit0_out;
			*omci_val_p = temp_int;
		}
		SET_BIT(mask_r, 0);
	}
bit0_out:

	//2: Dial_plan_table_max_size (2byte,unsigned integer,RWS)
	if (GET_BIT(mask, 1) > 0) {
		unsigned short *omci_val_p = &(parm->dial_plan_table_max_size);
		Simple_Get_Set_Int("digitmapmaxnum %d", "digitmapmaxnum %d %hu", *omci_val_p);
		if(ret != 0)
			goto bit1_out;
		SET_BIT(mask_r, 1);
	}
bit1_out:

	//3: Critical_dial_timeout (2byte,unsigned integer,RWS)
	if (GET_BIT(mask, 2) > 0) {
		unsigned short *omci_val_p = &(parm->critical_dial_timeout);	
		Simple_Get_Set_Int("digitstarttimeout %d", "digitstarttimeout %d %hu", *omci_val_p);
		if(ret != 0)
			goto bit2_out;
		SET_BIT(mask_r, 2);
	}
bit2_out:

	//4: Partial_dial_timeout (2byte,unsigned integer,RWS)
	if (GET_BIT(mask, 3) > 0) {
		unsigned short *omci_val_p = &(parm->partial_dial_timeout);
		Simple_Get_Set_Int("dialtonetimeout %d", "dialtonetimeout %d %hu", *omci_val_p);
		if(ret != 0)
			goto bit3_out;
		SET_BIT(mask_r, 3);
	}
bit3_out:

	//5: Dial_plan_format (1byte,enumeration,RWS)
	if (GET_BIT(mask, 4) > 0) {

		if (action == VOIP_HW_ACTION_GET) {
			int fvcli_digitmapsyntax;
			voipclient_cmd_get_dial_plan_format(port, &fvcli_digitmapsyntax);
			if( fvcli_digitmapsyntax == 2) {
				if(omci_env_g.olt_workaround_enable == ENV_OLT_WORKAROUND_CALIX) {
					parm->dial_plan_format = 1;
				} else {
					parm->dial_plan_format = 2;
				}
			} else {
				parm->dial_plan_format = fvcli_digitmapsyntax;
			}

		}
		SET_BIT(mask_r, 4);
	}

	//6: Dial_plan_table (30byte,table,RW)
	if (GET_BIT(mask, 5) > 0) {
		char *buf = NULL, *str = NULL;
		int buf_len;
		struct voip_hw_dial_plan_entry_t *entry = NULL;
		int token_len=0, rule_count=0, dial_plan_type=0;
		char *start_pos=NULL, *latest_or_pos=NULL;

		dbprintf_voip(LOG_INFO, " the dial plan list \n");
		if (action == VOIP_HW_ACTION_GET) {
			int num = 0;
			struct list_head *pos, *q;

			buf_len = 4095;
			buf = malloc_safe(buf_len+1);
			if(buf==NULL)
				goto bit5_out;
			buf[buf_len] = '\0';
			ret = voipclient_cmd_get_digitmap(port, buf, buf_len, &num, dial_plan_type);

			if (ret < 0)
				goto bit5_out;

			//number in cli, not omci
			parm->dial_plan_number = num;
			dbprintf_voip(LOG_INFO, "parm->dial_plan_number=%d\n", parm->dial_plan_number);
			if (num <= 0) {
				/* nothing to do */
				SET_BIT(mask_r, 5);
				goto bit5_out;
			}

			/* step1: make sure clean the dialplan list
			   step2: get the dialing plan, exampe: "1xx\n2s\n0x.t\n1x.t"
			   step3: arrange to customer's omci formate(each token <= 28 bytes)
			   All are verifyed for memory leak!
			 */

			//collect token to fit omci msg format(<= 28 bytes)

			dbprintf_voip(LOG_INFO, "clear the dial plan list first\n");
			list_for_each_safe(pos, q, &parm->dial_plan_list) {
				entry = list_entry(pos, struct voip_hw_dial_plan_entry_t, dial_plan_node);
				if (entry) {
					list_del(pos);
					free_safe(entry);
				}
			}

			INIT_LIST_HEAD(&parm->dial_plan_list);
			str=buf;
			start_pos=buf;
			while (*str != '\0') {
				token_len++;

				if (*str == '|' && token_len <= 28)
					latest_or_pos=str;
				// customer's dial plan table ex:
				//type:2 (0xxT|00T|[1-7]xxx|8xxxxxxx|#xxxxxxx|*xx|91123xxxxxxxxxx|9011xxxxx.T)
				//type:3 ^911n|^411|^[2-9][0-9]{6}|^1[2-9][0-9]{9}|^011[0-9]*T|^S[0-9]{2}

				if(token_len == 28) {
					if( latest_or_pos!=NULL) {
						entry = (struct voip_hw_dial_plan_entry_t *) malloc_safe(sizeof(struct voip_hw_dial_plan_entry_t));
						entry->id = rule_count;
						entry->action = 1; //0: remove, 1: add

						if(*str == ')') {       //end of all token
							memcpy(entry->token, start_pos, token_len);
						} else {
							memcpy(entry->token, start_pos, latest_or_pos-start_pos+1);
							str=latest_or_pos;
						}
						list_add(&(entry->dial_plan_node), &(parm->dial_plan_list));
						rule_count++;
					}
					token_len=0;
					start_pos=latest_or_pos+1;
					latest_or_pos=NULL;
				}
				str++;
			}

			if(token_len > 0) {
				entry = (struct voip_hw_dial_plan_entry_t *) malloc_safe(sizeof(struct voip_hw_dial_plan_entry_t));
				entry->id = rule_count;
				entry->action = 1; //0: remove, 1: add
				memcpy(entry->token, start_pos, token_len);
				list_add(&(entry->dial_plan_node), &(parm->dial_plan_list));
				rule_count++;
			}
		}

		SET_BIT(mask_r, 5);
bit5_out:
		if(buf!=NULL)
			free_safe(buf);
	}

	mask[0] = mask_r[0];
	mask[1] = mask_r[1];

	return 0;
}

//ME 141 VoIP_line_status
//53@254,141@1,141@2,141@3,141@4,141@5,141@6,141@7
int
voip_hw_voip_line_status(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_voip_line_status_t *parm)
{
	unsigned char mask_r[2] = {0, 0};
	int ret=0;

	dbprintf_voip(LOG_WARNING, "action=%s, port=%d, mask=0x%02x%02x\n", voip_hw_action_str[action%4], port, mask[0], mask[1]);

	/* recursive when called with all port */
	if (port == VOIP_HW_GLOBAL_PORT_DEF) {
		int i;
		for (i = 0; i < VOIP_HW_MAX_PORT_NUMBER; i++) {
			mask_r[0] = mask[0];
			mask_r[1] = mask[1];
			ret = voip_hw_voip_line_status(action, i, mask_r, parm);
			if(ret != 0)
				return ret;
		}
		return 0;
	}

	//1: Voip_codec_used (2byte,enumeration,R)
	if (GET_BIT(mask, 0) > 0) {
		unsigned short *omci_val_p = &(parm->voip_codec_used);
		if (action == VOIP_HW_ACTION_GET) {
			unsigned char codec_string[16];
			int codec_string_len = sizeof(codec_string)-1, ret;
			memset(codec_string, 0, sizeof(codec_string));
			ret=voipclient_cmd_get_string(codec_string, codec_string_len, "currentcodec %d", port);
			if(ret != 0) 
				goto bit0_out;
			if(strcasestr(codec_string, "PCMU")!=NULL) {
				*omci_val_p = CODEC_CLASS_ATTR_ID_PCMU;
			}else if(strcasestr(codec_string, "PCMA")!=NULL) {
				*omci_val_p = CODEC_CLASS_ATTR_ID_PCMA;
			}else if(strcasestr(codec_string, "G729")!=NULL) {
				*omci_val_p = CODEC_CLASS_ATTR_ID_G729;
			}else if(strcasestr(codec_string, "G723")!=NULL) {
				*omci_val_p = CODEC_CLASS_ATTR_ID_G723;
			}
		}
		SET_BIT(mask_r, 0);
	}
bit0_out:

	//2: Voip_voice_server_status (1byte,enumeration,R)
	if (GET_BIT(mask, 1) > 0) {
		if (action == VOIP_HW_ACTION_GET) {
			char status;
			voipclient_cmd_get_serverstatus(&status, port);
			parm->voip_voice_server_status = status;
		}
		SET_BIT(mask_r, 1);
	}

	//3: Voip_port_session_type (1byte,enumeration,R)
	if (GET_BIT(mask, 2) > 0) {
		if (action == VOIP_HW_ACTION_GET) {
			int session;
			voipclient_cmd_get_portsession(port, &session);
			parm->voip_port_session_type = session;
		}
		SET_BIT(mask_r, 2);
	}

	//4: Voip_call_1_packet_period (2byte,unsigned integer,R)
	if (GET_BIT(mask, 3) > 0) {
		if (action == VOIP_HW_ACTION_GET) {
			short packet;
			voipclient_cmd_get_txpacketsize_call(port, 1, &packet);
			parm->voip_call_1_packet_period = packet;
		}
		SET_BIT(mask_r, 3);
	}

	//5: Voip_call_2_packet_period (2byte,unsigned integer,R)
	if (GET_BIT(mask, 4) > 0) {
		if (action == VOIP_HW_ACTION_GET) {
			short packet;
			voipclient_cmd_get_txpacketsize_call(port, 2, &packet);
			parm->voip_call_2_packet_period = packet;
		}
		SET_BIT(mask_r, 4);
	}

	//6: Voip_call_1_dest_addr (25byte,string,R)
	if (GET_BIT(mask, 5) > 0) {
		/*voip_call_1_dest_addr */
		if (action == VOIP_HW_ACTION_GET) {
			voipclient_cmd_get_destaddr_call(port, 1, parm->voip_call_1_dest_addr);
		}
		SET_BIT(mask_r, 5);
	}

	//7: Voip_call_2_dest_addr (25byte,string,R)
	if (GET_BIT(mask, 6) > 0) {
		/*voip_call_2_dest_addr */
		if (action == VOIP_HW_ACTION_GET) {
			voipclient_cmd_get_destaddr_call(port, 2, parm->voip_call_2_dest_addr);
		}
		SET_BIT(mask_r, 6);
	}

	mask[0] = mask_r[0];
	mask[1] = mask_r[1];

	return 0;
}

//ME 140
//140@3,140@4,140@5,140@6,140@7
int
voip_hw_call_control_performance_monitoring_history_data(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_call_control_performance_monitoring_history_data_t *parm)
{
	dbprintf_voip(LOG_WARNING, "action=%s, port=%d, mask=0x%02x%02x\n", voip_hw_action_str[action%4], port, mask[0], mask[1]);
	dbprintf_voip(LOG_WARNING, "%s: VoIP CLI wrapper is not implemented.\n",  __FUNCTION__);
	return 0;
}

//ME 144
//144@3,144@4,144@5,144@6,144@7,144@8
int
voip_hw_rtp_performance_monitoring_history_data(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_rtp_performance_monitoring_history_data_t *parm)
{
	unsigned char mask_r[2] = {0, 0};

	dbprintf_voip(LOG_WARNING, "action=%s, port=%d, mask=0x%02x%02x\n", voip_hw_action_str[action%4], port, mask[0], mask[1]);

	//3: RTP_errors (4byte,unsigned integer,R)
	if (GET_BIT(mask, 2) > 0) {

		if (action == VOIP_HW_ACTION_GET) {
			voipclient_cmd_get_rtperrors(port, &parm->rtp_errors);
		}
		SET_BIT(mask_r, 2);
	}
	//4: Packet_loss (4byte,unsigned integer,R)
	if (GET_BIT(mask, 3) > 0) {

		if (action == VOIP_HW_ACTION_GET) {
			voipclient_cmd_get_packetloss(port, &parm->packet_loss);
		}
		SET_BIT(mask_r, 3);
	}
	//5: Maximum_jitter (4byte,unsigned integer,R)
	if (GET_BIT(mask, 4) > 0) {
		if (action == VOIP_HW_ACTION_GET) {

			voipclient_cmd_get_maximumjitter(port, &parm->maximum_jitter);
		}
		SET_BIT(mask_r, 4);
	}
	//6: Maximum_time_between_RTCP_packets (4byte,unsigned integer,R)
	if (GET_BIT(mask, 5) > 0) {
		if (action == VOIP_HW_ACTION_GET) {
			voipclient_cmd_get_maxtimertcppackets(port, &parm->maximum_time_between_rtcp_packets);
		}
		SET_BIT(mask_r, 5);
	}

	mask[0] = mask_r[0];
	mask[1] = mask_r[1];

	return 0;
}

//ME 151
//151@3,151@4,151@5,151@6,151@7,151@8,151@9,151@10,151@11,151@12,151@13,151@14,151@15
int
voip_hw_sip_agent_performance_monitoring_history_data(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_sip_agent_performance_monitoring_history_data_t *parm)
{
	dbprintf_voip(LOG_WARNING, "action=%s, port=%d, mask=0x%02x%02x\n", voip_hw_action_str[action%4], port, mask[0], mask[1]);
	dbprintf_voip(LOG_WARNING, "%s: VoIP CLI wrapper is not implemented.\n",  __FUNCTION__);
	return 0;
}

//ME 152
//152@3,152@4,152@5,152@6,152@7
int
voip_hw_sip_call_initiation_performance_monitoring_history_data(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_sip_call_initiation_performance_monitoring_history_data_t *parm)
{
	dbprintf_voip(LOG_WARNING, "action=%s, port=%d, mask=0x%02x%02x\n", voip_hw_action_str[action%4], port, mask[0], mask[1]);

	return 0;
}

//ME 138
//138@1,138@2,138@3,138@4,138@5,138@6,138@7,138@8
int
voip_hw_voip_config_data(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_voip_config_data_t *parm)
{
	unsigned char mask_r[2] = {0, 0};

	dbprintf_voip(LOG_WARNING, "action=%s, port=%d, mask=0x%02x%02x\n", voip_hw_action_str[action%4], port, mask[0], mask[1]);

	//1: Available_signalling_protocols (1byte,bit field,R)
	if (GET_BIT(mask, 0) > 0) {
		if (action == VOIP_HW_ACTION_GET) {
			parm->available_signalling_protocols.sip = 1;
			parm->available_signalling_protocols.h248 = 0;
			parm->available_signalling_protocols.mgcp = 0;
		}
		SET_BIT(mask_r, 0);
	}

	//3: Available_VoIP_configuration_methods (4byte,bit field,R)
	if (GET_BIT(mask, 2) > 0) {
		if (action == VOIP_HW_ACTION_GET) {
			parm->available_voip_configuration_methods.omci = 1;
			parm->available_voip_configuration_methods.configuration_file = 0;
			parm->available_voip_configuration_methods.sipping_config_framework = 0;
			parm->available_voip_configuration_methods.tr069 = 0;
		}
		SET_BIT(mask_r, 2);
	}

	//6: VoIP_configuration_state (1byte,enumeration,R)
	if (GET_BIT(mask, 5) > 0) {
		if (action == VOIP_HW_ACTION_GET) {
			int state;
			voipclient_cmd_get_configurationstate(&state);
			parm->voip_configuration_state = state;
		}
		SET_BIT(mask_r, 5);
	}

	mask[0] = mask_r[0];
	mask[1] = mask_r[1];

	return 0;
}

int
voip_hw_sip_config_portal(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_sip_config_portal_t *parm)
{
	dbprintf_voip(LOG_WARNING, "action=%s, port=%d, mask=0x%02x%02x\n", voip_hw_action_str[action%4], port, mask[0], mask[1]);
	dbprintf_voip(LOG_WARNING, "%s: VoIP CLI wrapper is not implemented.\n",  __FUNCTION__);
	return 0;
}

/* return the pointer to value position */
/* ex. [key1]=[value1] [key2]=[value2] */
static char*
get_key_value(const char *string, const char *key)
{
	char *key_pos;
	char *value_pos;
	int key_len;
	int string_len;

	/* problem: "ABCD" and "ABC" will both match "AB" */
	/* no such case now */
	key_pos = strcasestr(string, key);
	if(key_pos == NULL)
		return NULL;

	key_len = strlen(key);
	string_len = strlen(string);

	/* assume always [key]=[value] */
	value_pos = key_pos + key_len + 1;	/* include the '=' */
	if(value_pos >= (string+string_len))
		return NULL;

	return value_pos;
}

#define MLT_TEST_ALL		0x0	/* all MLT tests */
#define MLT_TEST_HAZARDOUS	0x1 /* hazardous potential */
#define MLT_TEST_EMF		0x2	/* foreign EMF */
#define MLT_TEST_RESISTIVE	0x3	/* resistive faults */
#define MLT_TEST_OFFHOOK	0x4	/* receiver off-hook */
#define MLT_TEST_RINGER		0x5	/* ringer */
#define MLT_TEST_NT1		0x6	/* NT1 dc signature test */
#define MLT_TEST_SELF		0x7	/* self test */
#define MLT_TEST_DIALTONE	0x8	/* dial tone make-break test */
#define MLT_RESULT_NOT_RUN			0x0	/* 000 test not run */
#define MLT_RESULT_FAIL_NO_DATA		0x2	/* 010 fail, measurement not reported */
#define MLT_RESULT_FAIL_DATA		0x3	/* 011 fail, measurement reported */
#define MLT_RESULT_PASS_NO_DATA		0x6	/* 110 pass, measurement not reported */
#define MLT_RESULT_PASS_DATA		0x7	/* 111 pass, measurement reported */
//test
int
voip_hw_physical_path_termination_point_pots_uni_test(int port, struct omcimsg_test_pots_isdn_t *req, struct omcimsg_test_result_pots_isdn_t *result)
{
	//int force_mode;
	int select_test;
	char mlt_all_result[256];
	int ret = 0;
	char *value_pos;
	int value;
	float ren;
	int pots_test_class = (req->select_test>>4) & 0x3;

	dbprintf_voip(LOG_WARNING, "port=%d, pots_test_class=%d, select_test=%d\n", port, pots_test_class, req->select_test & 0xf);

	if (pots_test_class == 1) {
		//char *number_to_dial = req->u.class1.number_to_dial;
		// sip call test should be done here
		result->select_test_and_mlt_drop_test_result = 0;
		result->select_test_and_mlt_drop_test_result |= 1<<4;	/* this bit is 1 for class 1 result */
		result->select_test_and_mlt_drop_test_result |= 1;	// test passed
		//result->select_test_and_mlt_drop_test_result |= 2;	// not completed, line off hook
		//result->select_test_and_mlt_drop_test_result |= 3;	// not completed, other reason
		dbprintf_voip(LOG_ERR, "sip/h.248 call test is not supported\n");
		return 0;
	} else if (pots_test_class == 2 || pots_test_class == 3) {
		// reserved
		return 0;
	}
	
	// below case is pots_test_class ==1 ///////////////////////////////////////////////////////


	//force_mode = req->select_test >> 7;
	select_test = req->select_test & 0xf;

	/* init the results */
	memset((void*)result, 0, sizeof(result));

	ret = voipclient_cmd_get_mlt(port, "-v -r -n -o", mlt_all_result, sizeof(mlt_all_result));
	if(ret != 0) {
		dbprintf_voip(LOG_ERR, "get result failed!\n");
		return 0;
	}
	/* hazardous potential */
	if(select_test == MLT_TEST_ALL || select_test == MLT_TEST_HAZARDOUS) {
		value_pos = get_key_value(mlt_all_result, "DCTG_MV");
		if(value_pos!=NULL) {
			value = atoi(value_pos);
			value = value / 1000;	/* mV to V */
			result->tip_ground_dc_voltage = (unsigned short)value;
			result->loop_test_dc_voltage_flags |= MLT_RESULT_PASS_DATA<<3;
			dbprintf_voip(LOG_ERR, "result->tip_ground_dc_voltage = %hd\n", result->tip_ground_dc_voltage);
		}

		value_pos = get_key_value(mlt_all_result, "DCRG_MV");
		if(value_pos!=NULL) {
			value = atoi(value_pos);
			value = value / 1000;	/* mV to V */
			result->ring_ground_dc_voltage = (unsigned short)value;
			result->loop_test_dc_voltage_flags |= MLT_RESULT_PASS_DATA;
			dbprintf_voip(LOG_ERR, "result->ring_ground_dc_voltage = %hd\n", result->ring_ground_dc_voltage);
		}

		value_pos = get_key_value(mlt_all_result, "DCTR_MV");
		if(value_pos!=NULL) {
			value = atoi(value_pos);
			value = value / 1000;	/* mV to V */
			result->tip_ring_dc_voltage = (unsigned short)value;
			result->loop_tip_ring_test_ac_dc_voltage_flags |= MLT_RESULT_PASS_DATA;
			dbprintf_voip(LOG_ERR, "result->tip_ring_dc_voltage = %hd\n", result->tip_ring_dc_voltage);
		}

		value_pos = get_key_value(mlt_all_result, "ACTG_VRMS2");
		if(value_pos!=NULL) {
			value = atoi(value_pos);
			value = value / 100;	/* 1/100*Vrms^2 to Vrms */
			result->tip_ground_ac_voltage = (unsigned char)value;
			result->loop_test_ac_voltage_flags |= MLT_RESULT_PASS_DATA<<3;
			dbprintf_voip(LOG_ERR, "result->tip_ground_ac_voltage = %hhd\n", result->tip_ground_ac_voltage);
		}

		value_pos = get_key_value(mlt_all_result, "ACRG_VRMS2");
		if(value_pos!=NULL) {
			value = atoi(value_pos);
			value = value / 100;	/* 1/100*Vrms^2 to Vrms */
			result->ring_ground_ac_voltage = (unsigned char)value;
			result->loop_test_ac_voltage_flags |= MLT_RESULT_PASS_DATA;
			dbprintf_voip(LOG_ERR, "result->ring_ground_ac_voltage = %hhd\n", result->ring_ground_ac_voltage);
		}

		value_pos = get_key_value(mlt_all_result, "ACTR_VRMS2");
		if(value_pos!=NULL) {
			value = atoi(value_pos);
			value = value / 100;	/* 1/100*Vrms^2 to Vrms */
			result->tip_ring_ac_voltage = (unsigned short)value;
			result->loop_tip_ring_test_ac_dc_voltage_flags |= MLT_RESULT_PASS_DATA<<3;
			dbprintf_voip(LOG_ERR, "result->tip_ring_ac_voltage = %hd\n", result->tip_ring_ac_voltage);
		}

		result->select_test_and_mlt_drop_test_result |= 1<<5;	/* pass test, or test not run */
	}

	/* foreign EMF */
	if(select_test == MLT_TEST_ALL || select_test == MLT_TEST_EMF) {
		result->select_test_and_mlt_drop_test_result |= 1<<4;	/* pass test, or test not run */
	}

	/* resistive faults */
	if(select_test == MLT_TEST_ALL || select_test == MLT_TEST_RESISTIVE) {
		value_pos = get_key_value(mlt_all_result, "RESTG_OHMS");
		if(value_pos!=NULL) {
			value = atoi(value_pos);
			if(value == 1000000000 || value > 5000000)
				result->tip_ground_dc_resistance = 0xffff;
			else {
				value = value / 10000;	/* (1/10 ohms) to kohms */
				result->tip_ground_dc_resistance = (unsigned short)value;
			}
			result->loop_test_resistance_flags_1 |= MLT_RESULT_PASS_DATA<<3;
			dbprintf_voip(LOG_ERR, "result->tip_ground_dc_resistance = %hu\n", result->tip_ground_dc_resistance);
		}

		value_pos = get_key_value(mlt_all_result, "RESRG_OHMS");
		if(value_pos!=NULL) {
			value = atoi(value_pos);
			if(value == 1000000000 || value > 5000000)
				result->ring_ground_dc_resistance = 0xffff;
			else {
				value = value / 10000;	/* (1/10 ohms) to kohms */
				result->ring_ground_dc_resistance = (unsigned short)value;
			}
			result->loop_test_resistance_flags_1 |= MLT_RESULT_PASS_DATA;
			dbprintf_voip(LOG_ERR, "result->ring_ground_dc_resistance = %hu\n", result->ring_ground_dc_resistance);
		}

		value_pos = get_key_value(mlt_all_result, "RESTR_OHMS");
		if(value_pos!=NULL) {
			value = atoi(value_pos);
			if(value == 1000000000 || value > 5000000)
				result->tip_ring_dc_resistance = 0xffff;
			else {
				value = value / 10000;	/* (1/10 ohms) to kohms */
				result->tip_ring_dc_resistance = (unsigned short)value;
			}
			result->loop_test_resistance_flags_2 |= MLT_RESULT_PASS_DATA<<3;
			dbprintf_voip(LOG_ERR, "result->tip_ring_dc_resistance = %hu\n", result->tip_ring_dc_resistance);
		}

		result->select_test_and_mlt_drop_test_result |= 1<<3;	/* pass test, or test not run */
	}

	/* receiver off-hook */
	/* FH says this is for FXO */
	result->select_test_and_mlt_drop_test_result |= 1<<2;	/* pass test, or test not run */

	/* ringer */
	if(select_test == MLT_TEST_ALL || select_test == MLT_TEST_RINGER) {
		value_pos = get_key_value(mlt_all_result, "REN");
		if(value_pos!=NULL) {
			ren = atof(value_pos);
			value = ren * 10;	/* REN/1000 to 0.1 REN */
			//value = value / 30;	/* REN/1000 to 0.1 REN */
			result->ringer_equivalence= (unsigned char)value;
			result->loop_test_resistance_flags_2 |= MLT_RESULT_PASS_DATA;
			dbprintf_voip(LOG_ERR, "result->ringer_equivalence = %hhu\n", result->ringer_equivalence);
		}

		result->select_test_and_mlt_drop_test_result |= 1<<1;	/* pass test, or test not run */
	}

	/* NT1 dc signature test */
	if(select_test == MLT_TEST_ALL || select_test == MLT_TEST_NT1) {
		result->select_test_and_mlt_drop_test_result |= 1<<0;	/* pass test, or test not run */
	}

	/* self test */
	if(select_test == MLT_TEST_ALL || select_test == MLT_TEST_SELF) {
		result->self_or_vendor_test_result = 1;	/* FH: always pass */
	}

	/* dial tone make-break test */
	if(select_test == MLT_TEST_ALL || select_test == MLT_TEST_DIALTONE) {

	}

	return 0;
}

//FH, proprietary MEs
int
voip_hw_wri_rtp_resource_name(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_wri_rtp_resource_name_t *parm)
{
	dbprintf_voip(LOG_WARNING, "action=%s, port=%d, mask=0x%02x%02x\n", voip_hw_action_str[action%4], port, mask[0], mask[1]);

	if (GET_BIT(mask, 0) > 0) {
		/* for H248, no need to support */
	}

	if (GET_BIT(mask, 1) > 0) {
		/* for H248, no need to support */
	}

	if (GET_BIT(mask, 2) > 0) {
		/* for H248, no need to support */
	}

	if (GET_BIT(mask, 3) > 0) {
		/* for H248, no need to support */
	}

	if (GET_BIT(mask, 4) > 0) {
		/* for H248, no need to support */
	}

	return 0;
}

//ME 65412 proprietary_fiberhome
int
voip_hw_wri_rfc2833_config_data(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_wri_rfc2833_config_data_t *parm)
{
	int i, ret = 0;
	int for_begin, for_end;
	unsigned char mask_r[2] = {0, 0};

	dbprintf_voip(LOG_WARNING, "action=%s, port=%d, mask=0x%02x%02x\n", voip_hw_action_str[action%4], port, mask[0], mask[1]);

	if (port == VOIP_HW_GLOBAL_PORT_DEF){
		for_begin = 0;
		for_end = VOIP_HW_MAX_PORT_NUMBER;
	}
	else {
		for_begin = port;
		for_end = port;
	}

	// 1: RFC2833_nego_state (1byte,enumeration,RWS)
	if (GET_BIT(mask, 0) > 0) {
		/* for H248, no need to support */
	}

	for (i = for_begin; i < for_end; i++){ 
		//2: Def_RFC2833_PT (1byte,unsigned integer,RWS) [96 ~ 127]
		if (GET_BIT(mask, 1) > 0) {
			if (action == VOIP_HW_ACTION_GET) {
				int state;
				ret = voipclient_cmd_get_payloadtype(i, "telephone-event", &state);
				if (ret == 0) {
					parm->def_rfc2833_pt = state;
				}
			}
			SET_BIT(mask_r, 1);
		}

		//3: Def_RFC2198_PT (1byte,unsigned integer,RWS) [96 ~ 127]
		if (GET_BIT(mask, 2) > 0) {
			if (action == VOIP_HW_ACTION_GET) {
				int state;
				ret = voipclient_cmd_get_payloadtype(i, "red", &state);
				if (ret == 0) {
					parm->def_rfc2198_pt = state;
				}
			}
			SET_BIT(mask_r, 2);
		}
	}

	mask[0] = mask_r[0];
	mask[1] = mask_r[1];

	return 0;
}

//ME 65413 proprietary_fiberhome
static int
voip_hw_wri_keepalive_config_data_by_port(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_wri_keepalive_config_data_t *parm)
{
	int ret = 0;
	unsigned char mask_r[2] = {0, 0};
	int map_port = port;

	dbprintf_voip(LOG_WARNING, "action=%s, port=%d, mask=0x%02x%02x\n", voip_hw_action_str[action%4], port, mask[0], mask[1]);

	if(map_port==VOIP_HW_GLOBAL_PORT_DEF)
		map_port = 0;

	//1: Keepalive_enable (1byte,enumeration,RWS)
	if (GET_BIT(mask, 0) > 0) {
		if (action == VOIP_HW_ACTION_GET) {
			int state;
			ret = voipclient_cmd_get_registrarkeepaliveenable(map_port, &state);
			if (ret == 0) {
				parm->keepalive_enable = state;
			}
		}
	}

	//2: Keepalive_interval (2byte,unsigned integer,RWS) [1 ~ 65535]
	if (GET_BIT(mask, 1) > 0) {
		if (action == VOIP_HW_ACTION_GET) {
			int state;
			ret = voipclient_cmd_get_registrarkeepaliveinterval(map_port, &state);
			if (ret == 0) {
				parm->keepalive_interva = state;
			}
		}
	}

	//3: Keepalive_times (2byte,unsigned integer,RWS) [1 ~ 65535]
	if (GET_BIT(mask, 2) > 0) {
		if (action == VOIP_HW_ACTION_GET) {
			int state;
			ret = voipclient_cmd_get_registrarkeepalivemaxtry(map_port, &state);
			if (ret == 0) {
				parm->keepalive_times = state;
			}
		}
	}

	//4: Keepalive_mode (1byte,enumeration,RWS)
	if (GET_BIT(mask, 3) > 0) {
		/* for H248, no need to support */
	}

	//5: Rereg_start_time_enable (1byte,enumeration,RWS)
	if (GET_BIT(mask, 4) > 0) {
		int var = 0;
		/* 1: VoIP uses head start time  : reregistrationpercentage=0
		   0: VoIP uses 0.9 reg time out : reregistrationpercentage=90 */ 
		Simple_Get_Set_Int("reregistrationpercentage %d", "reregistrationpercentage %d %d", var);
		if(ret != 0)
			goto bit4_out;
		if (action == VOIP_HW_ACTION_GET) {
			if(var == 0)
				parm->rereg_start_time_enable = 1;
			else
				parm->rereg_start_time_enable = 0;
		}
		SET_BIT(mask_r, 4);
	}
bit4_out:

	mask[0] = mask_r[0];
	mask[1] = mask_r[1];
	return 0;
}

int
voip_hw_wri_keepalive_config_data(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_wri_keepalive_config_data_t *parm)
{
	int i;
	dbprintf_voip(LOG_WARNING, "action=%s, port=%d, mask=0x%02x%02x\n", voip_hw_action_str[action%4], port, mask[0], mask[1]);

	for (i = 0; i < VOIP_HW_MAX_PORT_NUMBER; i++)
		voip_hw_wri_keepalive_config_data_by_port(action, i, mask, parm);
		
	return 0;
}	


//ME 65414 proprietary_fiberhome
int
voip_hw_wri_pos_config_data(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_wri_pos_config_data_t *parm)
{
	unsigned char mask_r[2] = {0, 0};
	char buf[2048];
	struct CfgList *dataList = NULL, *temp = NULL;
	struct voip_hw_pos_data_entry_t *entry = NULL;
	struct list_head *pos, *q;
	char *extention = NULL;
	char *next = NULL;
	int ret = 0;
	int num = 0;

	dbprintf_voip(LOG_WARNING, "action=%s, port=%d, mask=0x%02x%02x\n", voip_hw_action_str[action%4], port, mask[0], mask[1]);

	//1: Item_number (1byte,unsigned integer,RWS) [. ~ 10]
	if (GET_BIT(mask, 0) > 0) {
		if (action == VOIP_HW_ACTION_GET) {
			ret = voipclient_cmd_get_pos_number(port, &num);
			if (ret < 0) {
				goto pos_bit0_out;
			} else {
				parm->item_number = num;
			}
		}
		SET_BIT(mask_r, 0);
	}
pos_bit0_out:

	//2: POS_table (23byte,table,RW)
	if (GET_BIT(mask, 1) > 0) {
		if (action == VOIP_HW_ACTION_GET) {
			memset(buf, 0, sizeof(buf));
			ret = voipclient_cmd_get_pos(port, buf, sizeof(buf), &num);
			if (ret < 0) {
				goto pos_bit1_out;
			} else {
				if (num < 0) {
					goto pos_bit1_out;
				}
				dataList = voipclient_util_analyze_nv_string(buf, "=", "\n");
				if (dataList) {
					list_for_each_safe(pos, q, &parm->pos_data_list) {
						entry = list_entry(pos, struct
								   voip_hw_pos_data_entry_t, pos_data_node);
						if (entry) {
							list_del(pos);
							free_safe(entry);
						}
					}
					INIT_LIST_HEAD(&parm->pos_data_list);
					list_for_each_entry(temp, &dataList->list, list) {
						entry = (struct voip_hw_pos_data_entry_t *)
							malloc_safe(sizeof(struct voip_hw_pos_data_entry_t));
						entry->id = voipclient_util_parse_pos_id(temp->value);
						entry->pos_type = voipclient_util_parse_pos_type(temp->value);
						strncpy(buf, temp->value, sizeof(buf));
						if ((next = strchr(buf, '@')) != NULL) {
							*next = '\0';
							extention = buf;
							strncpy(entry->tel_num, extention, sizeof(entry->tel_num));
						}	
						list_add(&(entry->pos_data_node), &(parm->pos_data_list));
					}
					voipclient_util_analyze_nv_string_release(dataList);
				}
			}
		}
		SET_BIT(mask_r, 1);
	}
pos_bit1_out:

	mask[0] = mask_r[0];
	mask[1] = mask_r[1];
	return 0;
}

//ME 65415 proprietary_fiberhome
int
voip_hw_wri_ipt_config_data(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_wri_ipt_config_data_t *parm)
{
	unsigned char mask_r[2] = {0, 0};
	char buf[2048];
	struct CfgList *dataList = NULL, *temp = NULL;
	struct voip_hw_ipt_data_entry_t *entry = NULL;
	struct list_head *pos, *q;
	char *extention = NULL;
	char *next = NULL;
	int ret = 0;
	int num = 0;

	dbprintf_voip(LOG_WARNING, "action=%s, port=%d, mask=0x%02x%02x\n", voip_hw_action_str[action%4], port, mask[0], mask[1]);

	//1: Item_number (1byte,unsigned integer,RWS) [. ~ 10]
	if (GET_BIT(mask, 0) > 0) {
		if (action == VOIP_HW_ACTION_GET) {
			ret = voipclient_cmd_get_ipt_number(port, &num);
			if (ret < 0) {
				goto bit0_out;
			} else {
				parm->item_number = num;
			}
		}
		SET_BIT(mask_r, 0);
	}
bit0_out:

	//2: IPT_table (23byte,table,RW)
	if (GET_BIT(mask, 1) > 0) {
		if (action == VOIP_HW_ACTION_GET) {
			memset(buf, 0, sizeof(buf));
			ret = voipclient_cmd_get_ipt(port, buf, sizeof(buf), &num);
			if (ret < 0) {
				goto bit1_out;
			} else {
				if (num < 0) {
					goto bit1_out;
				}
				dataList = voipclient_util_analyze_nv_string(buf, "=", "\n");
				if (dataList) {
					list_for_each_safe(pos, q, &parm->ipt_data_list) {
						entry = list_entry(pos, struct
								   voip_hw_ipt_data_entry_t, ipt_data_node);
						if (entry) {
							list_del(pos);
							free_safe(entry);
						}
					}
					INIT_LIST_HEAD(&parm->ipt_data_list);
					list_for_each_entry(temp, &dataList->list, list) {
						entry = (struct voip_hw_ipt_data_entry_t *)
							malloc_safe(sizeof(struct voip_hw_ipt_data_entry_t));
						entry->id = voipclient_util_parse_pos_id(temp->value);
						entry->pos_type = voipclient_util_parse_pos_type(temp->value);
						strncpy(buf, temp->value, sizeof(buf));
						if ((next = strchr(buf, '@')) != NULL) {
							*next = '\0';
							extention = buf;
							strncpy(entry->tel_num, extention, sizeof(entry->tel_num));
						}
						list_add(&(entry->ipt_data_node), &(parm->ipt_data_list));
					}	
					voipclient_util_analyze_nv_string_release(dataList);
				}
			}
		}
		SET_BIT(mask_r, 1);
	}

bit1_out:

	mask[0] = mask_r[0];
	mask[1] = mask_r[1];
	return 0;
}

//ME 65416
//proprietary_fiberhome
//65416@1,65416@2,65416@4
int
voip_hw_wri_mg_port_register_control(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_wri_mg_port_register_control_t *parm)
{
	int ret = 0;
	unsigned char mask_r[2] = {0, 0};

	dbprintf_voip(LOG_WARNING, "action=%s, port=%d, mask=0x%02x%02x\n", voip_hw_action_str[action%4], port, mask[0], mask[1]);

	if (GET_BIT(mask, 0) > 0) {
		/* not used for now */
	}

	if (GET_BIT(mask, 1) > 0) {
		/* for H248, no need to support */
	}

	//4: Port_act (1byte,enumeration,RW)
	if (GET_BIT(mask, 3) > 0) {
		if (action == VOIP_HW_ACTION_GET) {
			int state;
			ret = voipclient_cmd_get_sipregister(port, &state);
			if (ret == 0) {
				parm->port_act = state;
			}
		}
		SET_BIT(mask_r, 3);
	}

	mask[0] = mask_r[0];
	mask[1] = mask_r[1];
	return 0;
}

//ME 65417 proprietary_fiberhome
int
voip_hw_wri_digit_map_match_mode(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_wri_digit_map_match_mode_t *parm)
{
	int i, ret = 0;
	unsigned char mask_r[2] = {0, 0};

	dbprintf_voip(LOG_WARNING, "action=%s, port=%d, mask=0x%02x%02x\n", voip_hw_action_str[action%4], port, mask[0], mask[1]);

	if (port == VOIP_HW_GLOBAL_PORT_DEF){
		for (i = 0; i < VOIP_HW_MAX_PORT_NUMBER; i++){ 
			if (GET_BIT(mask, 0) > 0) {
				if (action == VOIP_HW_ACTION_GET) {
					int state;
					ret = voipclient_cmd_get_digitmapmatchmode(i, &state);
					if (ret == 0) {
						parm->notify_match_mode = state;
					}
				}
			}
		}
	} else {
		//1: Notify_match_mode (1byte,enumeration,RWS)
		if (GET_BIT(mask, 0) > 0) {
			if (action == VOIP_HW_ACTION_GET) {
				int state;
				ret = voipclient_cmd_get_digitmapmatchmode(port, &state);
				if (ret == 0) {
					parm->notify_match_mode = state;
				}
			}
			SET_BIT(mask_r, 0);
		}
	}

	mask[0] = mask_r[0];
	mask[1] = mask_r[1];
	return 0;
}

/*  ME 65418 proprietary_fiberhome
 */ 
//65418@1,65418@2,65418@3,65418@4,65418@5,65418@6,65418@8
int
voip_hw_wri_fax_config_data(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_wri_fax_config_data_t *parm)
{
	int ret = 0;
	char mask_r[2] = { 0, 0};

	dbprintf_voip(LOG_WARNING, "action=%s, port=%d, mask=0x%02x%02x\n", voip_hw_action_str[action%4], port, mask[0], mask[1]);

	/* recursive when called with all port */
	if (port == VOIP_HW_GLOBAL_PORT_DEF) {
		int i;
		char mask_r[2];
		for (i = 0; i < VOIP_HW_MAX_PORT_NUMBER; i++) {
			mask_r[0] = mask[0];
			mask_r[1] = mask[1];
			ret = voip_hw_wri_fax_config_data(action, i, mask_r, parm);
			if(ret != 0)
				return ret;
		}
		return 0;
	}

	//1: Fax_control_mode (1byte,enumeration,RWS)
	if (GET_BIT(mask, 0) > 0) {
		/* for H248, no need to support */
	}

	//2: VBD_state (1byte,enumeration,RWS)
	if (GET_BIT(mask, 1) > 0) {
		Simple_Get_Set_Int("vbdstate %d", "vbdstate %d %hhu", parm->vbd_state);
		if(ret != 0)
			goto bit1_out;
		SET_BIT(mask_r, 1);
	}
bit1_out:

	//3: VBD_tx_interval (1byte,unsigned integer,RWS) [1 ~ 255]
	if (GET_BIT(mask, 2) > 0) {
		Simple_Get_Set_Int("vbdtxinterval %d", "vbdtxinterval %d %hhu", parm->vbd_tx_interval);
		if(ret != 0)
			goto bit2_out;
		SET_BIT(mask_r, 2);
	}
bit2_out:

	//4: VBD_rx_interval (1byte,unsigned integer,RWS) [1 ~ 255]
	if (GET_BIT(mask, 3) > 0) {
		Simple_Get_Set_Int("vbdrxinterval %d", "vbdrxinterval %d %hhu", parm->vbd_rx_interval);
		if(ret != 0)
			goto bit3_out;
		SET_BIT(mask_r, 3);
	}
bit3_out:

	//5: VBD_voice_coder (1byte,signed integer,RWS)
	if (GET_BIT(mask, 4) > 0) {
		Simple_Get_Set_Int("vbdvoicecoder %d", "vbdvoicecoder %d %hhd", parm->vbd_voice_coder);
		if(ret != 0)
			goto bit4_out;
		SET_BIT(mask_r, 4);
	}
bit4_out:

	//6: T38_event_detect_mode (1byte,enumeration,RWS)
	if (GET_BIT(mask, 5) > 0) {
		/* for H248, no need to support */
	}

	//8: DTMF_mode (1byte,enumeration,RWS)
	if (GET_BIT(mask, 7) > 0) {
		unsigned char *omci_val_p = &(parm->dtmf_mode);
		unsigned char cli_val;
		Simple_Get_Set_Int("dtmftype %d", "dtmftype %d %hhu", cli_val);
		if(ret != 0)
			goto bit7_out;
		if (action == VOIP_HW_ACTION_GET) {
			if(cli_val == 0)
				*omci_val_p = 0;
			else if(cli_val == 3)
				*omci_val_p = 1;
			else {
				goto bit7_out;
			}
		}
		SET_BIT(mask_r, 7);
	}
bit7_out:

	mask[0] = mask_r[0];
	mask[1] = mask_r[1];

	return 0;
}


//ME 65425 proprietary_fiberhome
int
voip_hw_wri_ngn_resource_info(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_wri_ngn_resource_info_t *parm)
{
	int ret = 0;
	unsigned char mask_r[2] = {0, 0};
	char *result = NULL;
	int result_len;
	struct voip_hw_wri_ngn_resource_info_t parm_all;

	dbprintf_voip(LOG_WARNING, "action=%s, port=%d, mask=0x%02x%02x\n", voip_hw_action_str[action%4], port, mask[0], mask[1]);

	/* the sipcli command dose not need port */

	/* get all data from the fvcli first */
	result_len = 4095;
	result = malloc_safe(result_len+1);
	if(result==NULL)
		return -1;
	result[result_len] = '\0';
	ret = voipclient_cmd_get_string(result, result_len,
									"resourceinfo");
	if(ret != 0) {
		free_safe(result);
		return ret;
	}

	/* assume the response format is divided by space */
	/*
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
	*/
	ret = sscanf(result, "%u %u %u %u %u %u %u %u "\
		   "%u %u %u %u %u %u %u %u",
		   &parm_all.received_ngn_req_packets,
		   &parm_all.send_ngn_req_packets,
		   &parm_all.received_ngn_resp_packets,
		   &parm_all.send_ngn_resp_packets,
		   &parm_all.received_rtp_packets,
		   &parm_all.send_rtp_packets,
		   &parm_all.received_rtp_bytes,
		   &parm_all.send_rtp_bytes,
		   &parm_all.packet_loss,
		   &parm_all.network_avg_delay,
		   &parm_all.rtp_avg_jitter_buffer,
		   &parm_all.bandwidth_usage,
		   &parm_all.lost_sig_packets,
		   &parm_all.retransmit_packets,
		   &parm_all.wrong_sig_packets,
		   &parm_all.unknown_sig_packets);
	if(ret != 16) {
		free_safe(result);
		return -1;
	}

	free_safe(result);
/*
 1: Received_NGN_req_packets (4byte,unsigned integer,R)
 2: Send_NGN_req_packets (4byte,unsigned integer,R)
 3: Received_NGN_resp_packets (4byte,unsigned integer,R)
 4: Send_NGN_resp_packets (4byte,unsigned integer,R)
 5: Received_rtp_packets (4byte,unsigned integer,R)
 6: Send_rtp_packets (4byte,unsigned integer,R)
 7: Received_rtp_bytes (4byte,unsigned integer,R)
 8: Send_rtp_bytes (4byte,unsigned integer,R)
 9: Packet_loss (4byte,unsigned integer,R)
10: Network_avg_delay (4byte,unsigned integer,R)
11: RTP_avg_jitter_buffer (4byte,unsigned integer,R)
12: Bandwidth_usage (4byte,unsigned integer,R)
13: Lost_sig_packets (4byte,unsigned integer,R)
14: Retransmit_packets (4byte,unsigned integer,R)
15: Wrong_sig_packets (4byte,unsigned integer,R)
16: Unknown_sig_packets (4byte,unsigned integer,R)
*/
	if (GET_BIT(mask, 0) > 0) {
		if (action == VOIP_HW_ACTION_GET) {
			parm->received_ngn_req_packets = parm_all.received_ngn_req_packets;
		}
		SET_BIT(mask_r, 0);
	}
//bit0_out:

	if (GET_BIT(mask, 1) > 0) {
		if (action == VOIP_HW_ACTION_GET) {
			parm->send_ngn_req_packets = parm_all.send_ngn_req_packets;
		}
		SET_BIT(mask_r, 1);
	}
//bit1_out:

	if (GET_BIT(mask, 2) > 0) {
		if (action == VOIP_HW_ACTION_GET) {
			parm->received_ngn_resp_packets = parm_all.received_ngn_resp_packets;
		}
		SET_BIT(mask_r, 2);
	}
//bit2_out:

	if (GET_BIT(mask, 3) > 0) {
		if (action == VOIP_HW_ACTION_GET) {
			parm->send_ngn_resp_packets = parm_all.send_ngn_resp_packets;
		}
		SET_BIT(mask_r, 3);
	}
//bit3_out:

	if (GET_BIT(mask, 4) > 0) {
		if (action == VOIP_HW_ACTION_GET) {
			parm->received_rtp_packets = parm_all.received_rtp_packets;
		}
		SET_BIT(mask_r, 4);
	}
//bit4_out:

	if (GET_BIT(mask, 5) > 0) {
		if (action == VOIP_HW_ACTION_GET) {
			parm->send_rtp_packets = parm_all.send_rtp_packets;
		}
		SET_BIT(mask_r, 5);
	}
//bit5_out:

	if (GET_BIT(mask, 6) > 0) {
		if (action == VOIP_HW_ACTION_GET) {
			parm->received_rtp_bytes = parm_all.received_rtp_bytes;
		}
		SET_BIT(mask_r, 6);
	}
//bit6_out:

	if (GET_BIT(mask, 7) > 0) {
		if (action == VOIP_HW_ACTION_GET) {
			parm->send_rtp_bytes = parm_all.send_rtp_bytes;
		}
		SET_BIT(mask_r, 7);
	}
//bit7_out:

	if (GET_BIT(mask, 8) > 0) {
		if (action == VOIP_HW_ACTION_GET) {
			parm->packet_loss = parm_all.packet_loss;
		}
		SET_BIT(mask_r, 8);
	}
//bit8_out:

	if (GET_BIT(mask, 9) > 0) {
		if (action == VOIP_HW_ACTION_GET) {
			parm->network_avg_delay = parm_all.network_avg_delay;
		}
		SET_BIT(mask_r, 9);
	}
//bit9_out:

	if (GET_BIT(mask, 10) > 0) {
		if (action == VOIP_HW_ACTION_GET) {
			parm->rtp_avg_jitter_buffer = parm_all.rtp_avg_jitter_buffer;
		}
		SET_BIT(mask_r, 10);
	}
//bit10_out:

	if (GET_BIT(mask, 11) > 0) {
		if (action == VOIP_HW_ACTION_GET) {
			parm->bandwidth_usage = parm_all.bandwidth_usage;
		}
		SET_BIT(mask_r, 11);
	}
//bit11_out:

	if (GET_BIT(mask, 12) > 0) {
		if (action == VOIP_HW_ACTION_GET) {
			parm->lost_sig_packets = parm_all.lost_sig_packets;
		}
		SET_BIT(mask_r, 12);
	}
//bit12_out:

	if (GET_BIT(mask, 13) > 0) {
		if (action == VOIP_HW_ACTION_GET) {
			parm->retransmit_packets = parm_all.retransmit_packets;
		}
		SET_BIT(mask_r, 13);
	}
//bit13_out:

	if (GET_BIT(mask, 14) > 0) {
		if (action == VOIP_HW_ACTION_GET) {
			parm->wrong_sig_packets = parm_all.wrong_sig_packets;
		}
		SET_BIT(mask_r, 14);
	}
//bit14_out:

	if (GET_BIT(mask, 15) > 0) {
		if (action == VOIP_HW_ACTION_GET) {
			parm->unknown_sig_packets = parm_all.unknown_sig_packets;
		}
		SET_BIT(mask_r, 15);
	}
//bit15_out:


	mask[0] = mask_r[0];
	mask[1] = mask_r[1];

	return 0;
}

//ME 65426 proprietary_fiberhome
//65426@2,65426@3,65426@4,65426@5,65426@6,65426@7,65426@8,65426@9,65426@10,65426@11,65426@12,65426@13,65426@14,65426@15
int
voip_hw_wri_ngn_port_statics(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_wri_ngn_port_statics_t *parm)
{
	int ret = 0;
	unsigned char mask_r[2] = {0, 0};
	char *result = NULL;
	int result_len;
	struct voip_hw_wri_ngn_port_statics_t parm_all;

	dbprintf_voip(LOG_WARNING, "action=%s, port=%d, mask=0x%02x%02x\n", voip_hw_action_str[action%4], port, mask[0], mask[1]);

	/* recursive when called with all port */
	if (port == VOIP_HW_GLOBAL_PORT_DEF) {
		int i;
		for (i = 0; i < VOIP_HW_MAX_PORT_NUMBER; i++) {
			mask_r[0] = mask[0];
			mask_r[1] = mask[1];
			ret = voip_hw_wri_ngn_port_statics(action, i, mask_r, parm);
			if(ret != 0)
				return ret;
		}
		return 0;
	}

	/* get all data from the fvcli first */
	result_len = 4095;
	result = malloc_safe(result_len+1);
	if(result == NULL)
		return -1;
	result[result_len] = '\0';
	ret = voipclient_cmd_get_string(result, result_len, "portstatics %d", port);
	if(ret != 0) {
		free_safe(result);
		return ret;
	}
	
	/* assume the response format is divided by space */
	/*
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
	*/
	ret = sscanf(result, "%u %u %u %u %u "\
		   "%hu/%hhu/%hhu %hhu:%hhu:%hhu "\
		   "%hu/%hhu/%hhu %hhu:%hhu:%hhu "\
		   "%u %u %u %u %u %u %u",
		   &parm_all.received_rtp_packets,
		   &parm_all.send_rtp_packets,
		   &parm_all.received_rtp_bytes,
		   &parm_all.send_rtp_bytes,
		   &parm_all.rtp_jitter_buffer,
		   &parm_all.latest_call_begin_time.year,
		   &parm_all.latest_call_begin_time.month,
		   &parm_all.latest_call_begin_time.day,
		   &parm_all.latest_call_begin_time.hour,
		   &parm_all.latest_call_begin_time.minute,
		   &parm_all.latest_call_begin_time.second,
		   &parm_all.latest_call_end_time.year,
		   &parm_all.latest_call_end_time.month,
		   &parm_all.latest_call_end_time.day,
		   &parm_all.latest_call_end_time.hour,
		   &parm_all.latest_call_end_time.minute,
		   &parm_all.latest_call_end_time.second,
		   &parm_all.last_call_duration,
		   &parm_all.total_call_times,
		   &parm_all.total_call_duration,
		   &parm_all.upstream_rates,
		   &parm_all.downstream_rates,
		   &parm_all.avg_network_delay,
		   &parm_all.packet_loss);
	if(ret != 24) {
		free_safe(result);
		return -1;
	}
	dbprintf_voip(LOG_INFO, "\nGET PortStatics: %s\n", result);
	
	//free_safe(result);

	//2: Received_rtp_packets (4byte,unsigned integer,R)
	if (GET_BIT(mask, 1) > 0) {
		if (action == VOIP_HW_ACTION_GET) {
			parm->received_rtp_packets = parm_all.received_rtp_packets;
			dbprintf_voip(LOG_INFO, "Received_rtp_packets: %u\n", parm->received_rtp_packets);
		}
		SET_BIT(mask_r, 1);
	}

	//3: Send_rtp_packets (4byte,unsigned integer,R)
	if (GET_BIT(mask, 2) > 0) {
		if (action == VOIP_HW_ACTION_GET) {
			parm->send_rtp_packets = parm_all.send_rtp_packets;
		}
		SET_BIT(mask_r, 2);
	}
//bit2_out:

	//4: Received_rtp_bytes (4byte,unsigned integer,R)
	if (GET_BIT(mask, 3) > 0) {
		if (action == VOIP_HW_ACTION_GET) {
			parm->received_rtp_bytes = parm_all.received_rtp_bytes;
			dbprintf_voip(LOG_INFO, "Received_rtp_byte: %u\n", parm->received_rtp_bytes);
		}
		SET_BIT(mask_r, 3);
	}
//bit3_out:

	//5: Send_rtp_bytes (4byte,unsigned integer,R)
	if (GET_BIT(mask, 4) > 0) {
		if (action == VOIP_HW_ACTION_GET) {
			parm->send_rtp_bytes = parm_all.send_rtp_bytes;
			dbprintf_voip(LOG_INFO, "Send_rtp_byte: %u\n", parm->send_rtp_bytes);
		}
		SET_BIT(mask_r, 4);
	}
//bit4_out:

	//6: RTP_jitter_buffer (4byte,unsigned integer,R)
	if (GET_BIT(mask, 5) > 0) {
		if (action == VOIP_HW_ACTION_GET) {
			parm->rtp_jitter_buffer = parm_all.rtp_jitter_buffer;
			dbprintf_voip(LOG_INFO, "RTP_jitter_buffer: %u\n", parm->rtp_jitter_buffer);
		}
		SET_BIT(mask_r, 5);
	}
//bit5_out:

	//7: Latest_call_begin_time (7byte,bit field,R)
	if (GET_BIT(mask, 6) > 0) {
		if (action == VOIP_HW_ACTION_GET) {
			memcpy(&parm->latest_call_begin_time,
				   &parm_all.latest_call_begin_time,
				   sizeof(parm_all.latest_call_begin_time));
		}
		SET_BIT(mask_r, 6);
	}
//bit6_out:

	//8: Latest_call_end_time (7byte,bit field,R)
	if (GET_BIT(mask, 7) > 0) {
		if (action == VOIP_HW_ACTION_GET) {
			memcpy(&parm->latest_call_end_time,
				   &parm_all.latest_call_end_time,
				   sizeof(parm_all.latest_call_end_time));
		}
		SET_BIT(mask_r, 7);
	}
//bit7_out:

	//9: Last_call_duration (4byte,unsigned integer,R)
	if (GET_BIT(mask, 8) > 0) {
		if (action == VOIP_HW_ACTION_GET) {
			parm->last_call_duration = parm_all.last_call_duration;
		}
		SET_BIT(mask_r, 8);
	}
//bit8_out:

	//10: Total_call_times (4byte,unsigned integer,R)
	if (GET_BIT(mask, 9) > 0) {
		if (action == VOIP_HW_ACTION_GET) {
			parm->total_call_times = parm_all.total_call_times;
		}
		SET_BIT(mask_r, 9);
	}
//bit9_out:

	//11: Total_call_duration (4byte,unsigned integer,R)
	if (GET_BIT(mask, 10) > 0) {
		if (action == VOIP_HW_ACTION_GET) {
			parm->total_call_duration = parm_all.total_call_duration;
		}
		SET_BIT(mask_r, 10);
	}
//bit10_out:

	//12: Upstream_rates (4byte,unsigned integer,R)
	if (GET_BIT(mask, 11) > 0) {
		if (action == VOIP_HW_ACTION_GET) {
			parm->upstream_rates = parm_all.upstream_rates;
		}
		SET_BIT(mask_r, 11);
	}
//bit11_out:

	//13: Downstream_rates (4byte,unsigned integer,R)
	if (GET_BIT(mask, 12) > 0) {
		if (action == VOIP_HW_ACTION_GET) {
			parm->downstream_rates = parm_all.downstream_rates;
		}
		SET_BIT(mask_r, 12);
	}
//bit12_out:

	//14: Avg_network_delay (4byte,unsigned integer,R)
	if (GET_BIT(mask, 13) > 0) {
		if (action == VOIP_HW_ACTION_GET) {
			parm->avg_network_delay = parm_all.avg_network_delay;
		}
		SET_BIT(mask_r, 13);
	}
//bit13_out:

	//15: Packet_loss (4byte,unsigned integer,R)
	if (GET_BIT(mask, 14) > 0) {
		if (action == VOIP_HW_ACTION_GET) {
			parm->packet_loss = parm_all.packet_loss;
		}
		SET_BIT(mask_r, 14);
	}
//bit14_out:

	free_safe(result);

	mask[0] = mask_r[0];
	mask[1] = mask_r[1];

	return 0;
}

//ME 65419 proprietary_fiberhome
int voip_hw_wri_sip_config(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_wri_sip_config_t *parm)
{
	int ret = 0;
	unsigned char mask_r[2] = {0, 0};
	int val_len = 376-1;
	unsigned char *pos = NULL;

	dbprintf_voip(LOG_WARNING, "action=%s, port=%d, mask=0x%02x%02x\n", voip_hw_action_str[action%4], port, mask[0], mask[1]);

	/* recursive when called with all port */
	if (port == VOIP_HW_GLOBAL_PORT_DEF) {
		int i;
		for (i = 0; i < VOIP_HW_MAX_PORT_NUMBER; i++) {
			mask_r[0] = mask[0];
			mask_r[1] = mask[1];
			ret = voip_hw_wri_sip_config(action, i, mask_r, parm);
			if(ret != 0)
				return ret;
		}
		return 0;
	}

	//1: Secondary_SIP_registrar_server_address_pointer (2byte,pointer,RWS) [classid 157]
	if (GET_BIT(mask, 0) > 0) {
		unsigned char *omci_val_p = parm->secondary_sip_registrar_server_address_pointer;
		if(omci_val_p == NULL)
			goto bit0_out;
		Simple_Get_Set_String("sipregistraraddrs %d 1", "sipregistraraddrs %d 1 %s", omci_val_p, val_len);
		if(ret != 0)
			goto bit0_out;
		SET_BIT(mask_r, 0);
	}
bit0_out:

	//2: Secondary_proxy_server_address_pointer (2byte,pointer,RWS) [classid 157]
	if (GET_BIT(mask, 1) > 0) {
		unsigned char *omci_val_p = parm->secondary_proxy_server_address_pointer;
		if(omci_val_p == NULL)
			goto bit1_out;
		Simple_Get_Set_String("sipproxyaddrs %d 1", "sipproxyaddrs %d 1 %s", omci_val_p, val_len);
		if(ret != 0)
			goto bit1_out;
		SET_BIT(mask_r, 1);
	}
bit1_out:

	//3: First_registrar_port (2byte,unsigned integer,RWS)
	if (GET_BIT(mask, 2) > 0) {
		Simple_Get_Set_Int("sipregistrarports %d 0", "sipregistrarports %d 0 %hu", parm->first_registrar_port);
		if(ret != 0)
			goto bit2_out;
		SET_BIT(mask_r, 2);
	}
bit2_out:

	//4: First_proxy_port (2byte,unsigned integer,RWS)
	if (GET_BIT(mask, 3) > 0) {
		Simple_Get_Set_Int("sipproxyports %d 0", "sipproxyports %d 0 %hu", parm->first_proxy_port);
		if(ret != 0)
			goto bit3_out;
		SET_BIT(mask_r, 3);
	}
bit3_out:

	//5: Secondary_registrar_port (2byte,unsigned integer,RWS)
	if (GET_BIT(mask, 4) > 0) {
		Simple_Get_Set_Int("sipregistrarports %d 1", "sipregistrarports %d 1 %hu", parm->secondary_registrar_port);
		if(ret != 0)
			goto bit4_out;
		SET_BIT(mask_r, 4);
	}
bit4_out:

	//6: Secondary_proxy_port (2byte,unsigned integer,RWS)
	if (GET_BIT(mask, 5) > 0) {
		Simple_Get_Set_Int("sipproxyports %d 1", "sipproxyports %d 1 %hu", parm->secondary_proxy_port);
		if(ret != 0)
			goto bit5_out;
		SET_BIT(mask_r, 5);
	}
bit5_out:

	//7: Current_SIP_registrar_pointer (2byte,pointer,R) [classid 157]
	if (GET_BIT(mask, 6) > 0) {
		unsigned char *omci_val_p = parm->current_sip_registrar_pointer;
		if(omci_val_p == NULL)
			goto release_out;
		if (action == VOIP_HW_ACTION_GET) {
			ret = voipclient_cmd_get_string(omci_val_p, val_len, "currentregistrar %d", port);
			if(ret != 0)
				goto release_out;
			pos = strchr(omci_val_p, ':');
			if(pos != NULL)
				*pos = '\0';
		}
		SET_BIT(mask_r, 6);
	}

release_out:

	mask[0] = mask_r[0];
	mask[1] = mask_r[1];

	return 0;
}

//ME 65420 proprietary_fiberhome
int
voip_hw_wri_softswitch_progile_config(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_wri_softswitch_progile_config_t *parm)
{
	int ret = 0;
	unsigned char mask_r[2] = {0, 0};

	dbprintf_voip(LOG_WARNING, "action=%s, port=%d, mask=0x%02x%02x\n", voip_hw_action_str[action%4], port, mask[0], mask[1]);

	/* recursive when called with all port */
	if (port == VOIP_HW_GLOBAL_PORT_DEF) {
		int i;
		for (i = 0; i < VOIP_HW_MAX_PORT_NUMBER; i++) {
			mask_r[0] = mask[0];
			mask_r[1] = mask[1];
			ret = voip_hw_wri_softswitch_progile_config(action, i, mask_r, parm);
			if(ret != 0)
				return ret;
		}
		return 0;
	}

	//1: Dail_tone_timeout (1byte,unsigned integer,RWS) [1 ~ 254]
	if (GET_BIT(mask, 0) > 0) {
		Simple_Get_Set_Int("dialtonetimeout %d", "dialtonetimeout %d %d", parm->dail_tone_timeout);
		if(ret != 0)
			goto bit0_out;
		SET_BIT(mask_r, 0);
	}
bit0_out:

	//2: Noanswer_tone_timeout (1byte,unsigned integer,RWS) [1 ~ 254]
	if (GET_BIT(mask, 1) > 0) {
		Simple_Get_Set_Int("answertimeout %d", "answertimeout %d %d", parm->noanswer_tone_timeout);
		if(ret != 0)
			goto bit1_out;
		SET_BIT(mask_r, 1);
	}
bit1_out:

	//3: Busy_tone_timeout (1byte,unsigned integer,RWS) [1 ~ 254]
	if (GET_BIT(mask, 2) > 0) {
		Simple_Get_Set_Int("busytonetimeout %d", "busytonetimeout %d %d", parm->busy_tone_timeout);
		if(ret != 0)
			goto bit2_out;
		SET_BIT(mask_r, 2);
	}
bit2_out:

	//4: ROH_timeout (1byte,unsigned integer,RWS) [1 ~ 254]
	if (GET_BIT(mask, 3) > 0) {
		Simple_Get_Set_Int("howlingtonetimeout %d", "howlingtonetimeout %d %d", parm->roh_timeout);
		if(ret != 0)
			goto release_out;
		SET_BIT(mask_r, 3);
	}

	//5: Retransmit_timeout (1byte,unsigned integer,RWS) [1 ~ 254]
	// Not support ?
release_out:
	
	mask[0] = mask_r[0];
	mask[1] = mask_r[1];

	return 0;
}

extern char voip_interface_name_g[26];
#define COUNTER_NAME_LEN	64
int get_count_for_dhcpc_action(char *action_name, int *value)
{
	FILE	*fptr;
	char *ch, *key_start, *value_start;
	char strbuf[COUNTER_NAME_LEN], file_name[128];

	*value=0;
	snprintf(file_name, 128, "/var/run/udhcpc.count.%s", voip_interface_name_g);
	if((fptr=fopen(file_name, "r")) == NULL) {       /*file to parser*/
		return 0;
	}
	
	while( fgets(strbuf, COUNTER_NAME_LEN,fptr) != NULL) {
		key_start = strbuf;
		if (key_start[0] == 0)
			continue;

		//printf("%s",strbuf);
		if ((ch = strchr(key_start, '=')) == NULL) {
			fclose(fptr);
			dbprintf_voip(LOG_ERR, "%s formate error, should be never happen\n", file_name);
			return -1;
		} else {
			value_start = ch + 1;
			*ch = 0x0;
		}

		if( strcmp(action_name, key_start)==0 ) {
			*value=atoi(value_start);
			fclose(fptr);
			return 0;
		}
	}
	fclose(fptr);
	return 0;
}

//ME 65280 proprietary_calix
int
voip_hw_voip_call_statistics_extension(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_voip_call_statistics_extension_t *parm)
{
	int ret = -1;
	unsigned char mask_r[2] = {0, 0};

	dbprintf_voip(LOG_WARNING, "action=%s, port=%d, mask=0x%02x%02x\n", voip_hw_action_str[action%4], port, mask[0], mask[1]);

	/* recursive when called with all port */
	if (port == VOIP_HW_GLOBAL_PORT_DEF) {
		int i;
		for (i = 0; i < VOIP_HW_MAX_PORT_NUMBER; i++) {
			mask_r[0] = mask[0];
			mask_r[1] = mask[1];
			ret = voip_hw_voip_call_statistics_extension(action, i, mask_r, parm);
			if(ret != 0)
				return ret;
		}
		return 0;
	}

	// 1: DhcpAttempts (4byte,unsigned integer,R)
	if (GET_BIT(mask, 0) > 0) {
		/* get only */
		unsigned int *omci_val_p = &(parm->dhcpattempts);
		if (action == VOIP_HW_ACTION_GET) {
			int temp_int;
			*omci_val_p = 0;
			ret=get_count_for_dhcpc_action("dhcpc_attempts_count", &temp_int);
			if (ret != 0)
				goto bit0_out;
			*omci_val_p = temp_int;
		}
		SET_BIT(mask_r, 0);
	}
bit0_out:

	//2: DhcpAcks (4byte,unsigned integer,R)
	if (GET_BIT(mask, 1) > 0) {
		/* get only */
		unsigned int *omci_val_p = &(parm->dhcpacks);
		if (action == VOIP_HW_ACTION_GET) {
			int temp_int;
			*omci_val_p = 0;
			ret=get_count_for_dhcpc_action("dhcpc_acks_received_count", &temp_int);
			if (ret != 0)
				goto bit1_out;
			*omci_val_p = temp_int;
		}
		SET_BIT(mask_r, 1);
	}
bit1_out:

	//3: DhcpNacks (4byte,unsigned integer,R)
	if (GET_BIT(mask, 2) > 0) {
		/* get only */
		unsigned int *omci_val_p = &(parm->dhcpnacks);
		if (action == VOIP_HW_ACTION_GET) {
			int temp_int;
			ret=get_count_for_dhcpc_action("dhcpc_nacks_count", &temp_int);
			*omci_val_p = 0;
			if (ret != 0)
				goto bit2_out;
			*omci_val_p = temp_int;
		}
		SET_BIT(mask_r, 2);
	}
bit2_out:

	//4: RegAttempts (4byte,unsigned integer,R)
	if (GET_BIT(mask, 3) > 0) {
		/* get only */
		unsigned int *omci_val_p = &(parm->regattempts);
		if (action == VOIP_HW_ACTION_GET) {
			int temp_int;
			ret = voipclient_cmd_get_int(&temp_int, "registerattemptstats %d", port);
			if (ret != 0)
				goto bit3_out;
			*omci_val_p = temp_int;
		}
		SET_BIT(mask_r, 3);
	}
bit3_out:

	//5: RegChallenges (4byte,unsigned integer,R)
	if (GET_BIT(mask, 4) > 0) {
		/* get only */
		unsigned int *omci_val_p = &(parm->regchallenges);
		if (action == VOIP_HW_ACTION_GET) {
			int temp_int;
			ret = voipclient_cmd_get_int(&temp_int, "registerauthstats %d", port);
			if (ret != 0)
				goto bit4_out;
			*omci_val_p = temp_int;
		}
		SET_BIT(mask_r, 4);
	}
bit4_out:

	//6: RegRejects (4byte,unsigned integer,R)
	if (GET_BIT(mask, 5) > 0) {
		/* get only */
		unsigned int *omci_val_p = &(parm->regrejects);
		if (action == VOIP_HW_ACTION_GET) {
			int temp_int;
			ret = voipclient_cmd_get_int(&temp_int, "registerrejectstats %d", port);
			if (ret != 0)
				goto bit5_out;
			*omci_val_p = temp_int;
		}
		SET_BIT(mask_r, 5);
	}
bit5_out:

	//7: RegGrants (4byte,unsigned integer,R)
	if (GET_BIT(mask, 6) > 0) {
		/* get only */
		unsigned int *omci_val_p = &(parm->reggrants);
		if (action == VOIP_HW_ACTION_GET) {
			int temp_int;
			ret = voipclient_cmd_get_int(&temp_int, "registersuccessstats %d", port);
			if (ret != 0)
				goto bit6_out;
			*omci_val_p = temp_int;
		}
		SET_BIT(mask_r, 6);
	}
bit6_out:

	//8: InCallAttempts (4byte,unsigned integer,R)
	if (GET_BIT(mask, 7) > 0) {
		/* get only */
		unsigned int *omci_val_p = &(parm->incallattempts);
		if (action == VOIP_HW_ACTION_GET) {
			int temp_int;
			ret = voipclient_cmd_get_int(&temp_int, "incallattemptstats %d", port);
			if (ret != 0)
				goto bit7_out;
			*omci_val_p = temp_int;
		}
		SET_BIT(mask_r, 7);
	}
bit7_out:

	//9: InCallCompletions (4byte,unsigned integer,R)
	if (GET_BIT(mask, 8) > 0) {
		/* get only */
		unsigned int *omci_val_p = &(parm->incallcompletions);
		if (action == VOIP_HW_ACTION_GET) {
			int temp_int;
			ret = voipclient_cmd_get_int(&temp_int, "incallcompletestats %d", port);
			if (ret != 0)
				goto bit8_out;
			*omci_val_p = temp_int;
		}
		SET_BIT(mask_r, 8);
	}
bit8_out:

	//10: InCallBusy (4byte,unsigned integer,R)
	if (GET_BIT(mask, 9) > 0) {
		/* get only */
		unsigned int *omci_val_p = &(parm->incallbusy);
		if (action == VOIP_HW_ACTION_GET) {
			int temp_int;
			ret = voipclient_cmd_get_int(&temp_int, "incallrejectstats %d", port);
			if (ret != 0)
				goto bit9_out;
			*omci_val_p = temp_int;
		}
		SET_BIT(mask_r, 9);
	}
bit9_out:

	//11: InCallPeerDisconnects (4byte,unsigned integer,R)
	if (GET_BIT(mask, 10) > 0) {
		/* get only */
		unsigned int *omci_val_p = &(parm->incallpeerdisconnects);
		if (action == VOIP_HW_ACTION_GET) {
			int temp_int;
			ret = voipclient_cmd_get_int(&temp_int, "incallremotedisconnectstats %d", port);
			if (ret != 0)
				goto bit10_out;
			*omci_val_p = temp_int;
		}
		SET_BIT(mask_r, 10);
	}
bit10_out:

	//12: InCallOntDisconnects (4byte,unsigned integer,R)
	if (GET_BIT(mask, 11) > 0) {
		/* get only */
		unsigned int *omci_val_p = &(parm->incallontdisconnects);
		if (action == VOIP_HW_ACTION_GET) {
			int temp_int;
			ret = voipclient_cmd_get_int(&temp_int, "incalllocaldisconnectstats %d", port);
			if (ret != 0)
				goto bit11_out;
			*omci_val_p = temp_int;
		}
		SET_BIT(mask_r, 11);
	}
bit11_out:

	//13: OutCallAttempts (4byte,unsigned integer,R)
	if (GET_BIT(mask, 12) > 0) {
		/* get only */
		unsigned int *omci_val_p = &(parm->outcallattempts);
		if (action == VOIP_HW_ACTION_GET) {
			int temp_int;
			ret = voipclient_cmd_get_int(&temp_int, "outcallattemptstats %d", port);
			if (ret != 0)
				goto bit12_out;
			*omci_val_p = temp_int;
		}
		SET_BIT(mask_r, 12);
	}
bit12_out:

	//14: OutCallCompletions (4byte,unsigned integer,R)
	if (GET_BIT(mask, 13) > 0) {
		/* get only */
		unsigned int *omci_val_p = &(parm->outcallcompletions);
		if (action == VOIP_HW_ACTION_GET) {
			int temp_int;
			ret = voipclient_cmd_get_int(&temp_int, "outcallcompletestats %d", port);
			if (ret != 0)
				goto bit13_out;
			*omci_val_p = temp_int;
		}
		SET_BIT(mask_r, 13);
	}
bit13_out:

	//15: OutCallBusy (4byte,unsigned integer,R)
	if (GET_BIT(mask, 14) > 0) {
		/* get only */
		unsigned int *omci_val_p = &(parm->outcallbusy);
		if (action == VOIP_HW_ACTION_GET) {
			int temp_int;
			ret = voipclient_cmd_get_int(&temp_int, "outcallrejectstats %d", port);
			if (ret != 0)
				goto release_out;
			*omci_val_p = temp_int;
		}
		SET_BIT(mask_r, 14);
	}

release_out:
	
	mask[0] = mask_r[0];
	mask[1] = mask_r[1];

	return 0;
}

//ME 65281 proprietary_calix
int
voip_hw_voip_call_statistics_extension2(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_voip_call_statistics_extension2_t *parm)
{
	int ret = -1;
	unsigned char mask_r[2] = {0, 0};
	char *result = NULL, *token, *ch, *key_start, *value_start;
	int result_len, pos=0;
	struct voip_hw_voip_call_statistics_extension2_t current_ext2;

	dbprintf_voip(LOG_WARNING, "action=%s, port=%d, mask=0x%02x%02x\n", voip_hw_action_str[action%4], port, mask[0], mask[1]);

	/* recursive when called with all port */
	if (port == VOIP_HW_GLOBAL_PORT_DEF) {
		int i;
		for (i = 0; i < VOIP_HW_MAX_PORT_NUMBER; i++) {
			mask_r[0] = mask[0];
			mask_r[1] = mask[1];
			ret = voip_hw_voip_call_statistics_extension2(action, i, mask_r, parm);
			if(ret != 0)
				return ret;
		}
		return 0;
	}

	memset(&current_ext2, 0, sizeof(current_ext2));

	if (action == VOIP_HW_ACTION_GET) {

		result_len = 4095;
		result = malloc_safe(result_len+1);
		if(result==NULL)
			return -1;
		result[result_len] = '\0';
		ret = voipclient_cmd_get_string(result, result_len, "rtpstatsall %d", port);
		if(ret != 0)
			goto out_free;

		token=strtok(result, "\n");
		while(token)
		{
			switch(pos)
			{
			case 0:
				//skip title
				dbprintf_voip(LOG_INFO, "[%s]\n", token);
			break;

			default:
				//printf("[%s]\n", token);
				key_start = token;
				if (key_start[0] == 0)
					return -1;

				if ((ch = strchr(key_start, ':')) != NULL) {
					*ch= '\0';
					value_start = ch + 1;

					dbprintf_voip(LOG_INFO, "\t\t[%s], %s\n",key_start, value_start);

					if(strcmp(key_start, "Sent Pkts")==0 ) {
						sscanf(value_start, "%u", &current_ext2.rtppktssent);
						dbprintf_voip(LOG_INFO, "Sent Pkts=%u\n", current_ext2.rtppktssent);
					}
					if(strcmp(key_start, "Recv Pkts")==0 ) {
						sscanf(value_start, "%u", &current_ext2.rtppktsrecv);
						dbprintf_voip(LOG_INFO, "Recv Pkts=%u\n", current_ext2.rtppktsrecv);
					}
					if(strcmp(key_start, "Sent Null Pkts")==0 ) {
						sscanf(value_start, "%u", &current_ext2.rtpnullipsent);
						dbprintf_voip(LOG_INFO, "Sent Null Pkts=%u\n", current_ext2.rtpnullipsent);
					}
					if(strcmp(key_start, "Recv Null Pkts")==0 ) {
						sscanf(value_start, "%u", &current_ext2.rtpnulliprecv);
						dbprintf_voip(LOG_INFO, "Recv Null Pkts=%u\n", current_ext2.rtpnulliprecv);
					}
				}
			break;
			}
			pos++;
			token=strtok( NULL , "\n");
		}
	}

	//1: OutCallPeerDisconnects (4byte,unsigned integer,R)
	if (GET_BIT(mask, 0) > 0) {
		/* get only */
		unsigned int *omci_val_p = &(parm->outcallpeerdisconnects);
		if (action == VOIP_HW_ACTION_GET) {
			int temp_int;
			ret = voipclient_cmd_get_int(&temp_int, "outcallremotedisconnectstats %d", port);
			if (ret != 0)
				goto bit0_out;
			*omci_val_p = temp_int;
		}
		SET_BIT(mask_r, 0);
	}
bit0_out:

	//2: OutCallOntDisconnects (4byte,unsigned integer,R)
	if (GET_BIT(mask, 1) > 0) {
		/* get only */
		unsigned int *omci_val_p = &(parm->outcallontdisconnects);
		if (action == VOIP_HW_ACTION_GET) {
			int temp_int;
			ret = voipclient_cmd_get_int(&temp_int, "outcalllocaldisconnectstats %d", port);
			if (ret != 0)
				goto bit1_out;
			*omci_val_p = temp_int;
		}
		SET_BIT(mask_r, 1);
	}
bit1_out:

	//3: E911CallAttempts (4byte,unsigned integer,R)
	if (GET_BIT(mask, 2) > 0) {
		/* get only */
		unsigned int *omci_val_p = &(parm->e911callattempts);
		if (action == VOIP_HW_ACTION_GET) {
			int temp_int;
			ret = voipclient_cmd_get_int(&temp_int, "emergencycallattemptstats %d", port);
			if (ret != 0)
				goto bit2_out;
			*omci_val_p = temp_int;
		}
		SET_BIT(mask_r, 2);
	}
bit2_out:

	//4: E911CallCompletions (4byte,unsigned integer,R)
	if (GET_BIT(mask, 3) > 0) {
		/* get only */
		unsigned int *omci_val_p = &(parm->e911callcompletions);
		if (action == VOIP_HW_ACTION_GET) {
			int temp_int;
			ret = voipclient_cmd_get_int(&temp_int, "emergencycallcompletestats %d", port);
			if (ret != 0)
				goto bit3_out;
			*omci_val_p = temp_int;
		}
		SET_BIT(mask_r, 3);
	}
bit3_out:

	//5: E911CallBusy (4byte,unsigned integer,R)
	if (GET_BIT(mask, 4) > 0) {
		/* get only */
		unsigned int *omci_val_p = &(parm->e911callbusy);
		if (action == VOIP_HW_ACTION_GET) {
			int temp_int;
			ret = voipclient_cmd_get_int(&temp_int, "emergencycallrejectstats %d", port);
			if (ret != 0)
				goto bit4_out;
			*omci_val_p = temp_int;
		}
		SET_BIT(mask_r, 4);
	}
bit4_out:

	//6: E911CallPeerDisconnects (4byte,unsigned integer,R)
	if (GET_BIT(mask, 5) > 0) {
		/* get only */
		unsigned int *omci_val_p = &(parm->e911callpeerdisconnects);
		if (action == VOIP_HW_ACTION_GET) {
			int temp_int;
			ret = voipclient_cmd_get_int(&temp_int, "emergencycallremotedisconnectstats %d", port);
			if (ret != 0)
				goto bit5_out;
			*omci_val_p = temp_int;
		}
		SET_BIT(mask_r, 5);
	}
bit5_out:

	//7: E911CallOnHooks (4byte,unsigned integer,R)
	if (GET_BIT(mask, 6) > 0) {
		/* get only */
		unsigned int *omci_val_p = &(parm->e911callonhooks);
		if (action == VOIP_HW_ACTION_GET) {
			int temp_int;
			ret = voipclient_cmd_get_int(&temp_int, "emergencycalllonhookstats %d", port);
			if (ret != 0)
				goto bit6_out;
			*omci_val_p = temp_int;
		}
		SET_BIT(mask_r, 6);
	}
bit6_out:

	//8: VmwiNotifyMsgsWaiting (4byte,unsigned integer,R)
	if (GET_BIT(mask, 7) > 0) {
		/* get only */
		unsigned int *omci_val_p = &(parm->vmwinotifymsgswaiting);
		if (action == VOIP_HW_ACTION_GET) {
			int temp_int;
			ret = voipclient_cmd_get_int(&temp_int, "messagewaitingyesstats %d", port);
			if (ret != 0)
				goto bit7_out;
			*omci_val_p = temp_int;
		}
		SET_BIT(mask_r, 7);
	}
bit7_out:

	//9: VmwiNotifyNoMsgsWaiting (4byte,unsigned integer,R)
	if (GET_BIT(mask, 8) > 0) {
		/* get only */
		unsigned int *omci_val_p = &(parm->vmwinotifynomsgswaiting);
		if (action == VOIP_HW_ACTION_GET) {
			int temp_int;
			ret = voipclient_cmd_get_int(&temp_int, "messagewaitingnostats %d", port);
			if (ret != 0)
				goto bit8_out;
			*omci_val_p = temp_int;
		}
		SET_BIT(mask_r, 8);
	}
bit8_out:

	//10: RtpPktsSent (4byte,unsigned integer,R)
	if (GET_BIT(mask, 9) > 0) {
		/* get only */
		unsigned int *omci_val_p = &(parm->rtppktssent);
		if (action == VOIP_HW_ACTION_GET) {
			*omci_val_p = current_ext2.rtppktssent;
		}
		SET_BIT(mask_r, 9);
	}

	//11: RtpPktsRecv (4byte,unsigned integer,R)
	if (GET_BIT(mask, 10) > 0) {
		/* get only */
		unsigned int *omci_val_p = &(parm->rtppktsrecv);
		if (action == VOIP_HW_ACTION_GET) {
			*omci_val_p = current_ext2.rtppktsrecv;
		}
		SET_BIT(mask_r, 10);
	}

	//12: RtpNullIPSent (4byte,unsigned integer,R)
	if (GET_BIT(mask, 11) > 0) {
		/* get only */
		unsigned int *omci_val_p = &(parm->rtpnullipsent);
		if (action == VOIP_HW_ACTION_GET) {
			*omci_val_p = current_ext2.rtpnullipsent;
		}
		SET_BIT(mask_r, 11);
	}

	//13: RtpNullIPRecv (4byte,unsigned integer,R)
	if (GET_BIT(mask, 12) > 0) {
		/* get only */
		unsigned int *omci_val_p = &(parm->rtpnulliprecv);
		if (action == VOIP_HW_ACTION_GET) {
			*omci_val_p = current_ext2.rtpnulliprecv;
		}
		SET_BIT(mask_r, 12);
	}

	//14: Ext2ActiveCallCntr (4byte,unsigned integer,R)
	if (GET_BIT(mask, 13) > 0) {
		/* get only */
		unsigned int *omci_val_p = &(parm->ext2activecallcntr);
		if (action == VOIP_HW_ACTION_GET) {
			int temp_int;
			ret = voipclient_cmd_get_int(&temp_int, "totalcallduration %d", port);
			if (ret != 0)
				goto release_out;

			if(temp_int != 0)
				*omci_val_p = temp_int/100; 	//ActiveCallCntr = duration / 100
			else
				*omci_val_p = 0;
		}
		SET_BIT(mask_r, 13);
	}

release_out:
out_free:
	if(result != NULL)
		free_safe(result);
	
	mask[0] = mask_r[0];
	mask[1] = mask_r[1];

	return 0;
}

//ME 65282 proprietary_calix
int
voip_hw_voip_call_statistics_extension3(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_voip_call_statistics_extension3_t *parm)
{
	int ret = -1;
	unsigned char mask_r[2] = {0, 0};
	char *result = NULL, *token, *ch, *key_start, *value_start;
	int result_len, pos=0;
	struct voip_hw_voip_call_statistics_extension3_t current_ext3;

	dbprintf_voip(LOG_WARNING, "action=%s, port=%d, mask=0x%02x%02x\n", voip_hw_action_str[action%4], port, mask[0], mask[1]);

	/* recursive when called with all port */
	if (port == VOIP_HW_GLOBAL_PORT_DEF) {
		int i;
		for (i = 0; i < VOIP_HW_MAX_PORT_NUMBER; i++) {
			mask_r[0] = mask[0];
			mask_r[1] = mask[1];
			ret = voip_hw_voip_call_statistics_extension3(action, i, mask_r, parm);
			if(ret != 0)
				return ret;
		}
		return 0;
	}

	memset(&current_ext3, 0, sizeof(current_ext3));

	if (action == VOIP_HW_ACTION_GET) {

		result_len = 4095;
		result = malloc_safe(result_len+1);
		if(result==NULL)
			return -1;
		result[result_len] = '\0';
		ret = voipclient_cmd_get_string(result, result_len, "rtpstatsall %d", port);
		if(ret != 0)
			goto out_free;

		token=strtok(result, "\n");
		while(token)
		{
			switch(pos)
			{
			case 0:
				//skip title
				dbprintf_voip(LOG_INFO, "[%s]\n", token);
			break;

			default:
				//printf("[%s]\n", token);
				key_start = token;
				if (key_start[0] == 0)
					return -1;

				if ((ch = strchr(key_start, ':')) != NULL) {
					*ch= '\0';
					value_start = ch + 1;

					dbprintf_voip(LOG_INFO, "\t\t[%s], %s\n",key_start, value_start);

					if(strcmp(key_start, "Recv Error Pkts")==0 ) {
						sscanf(value_start, "%u", &current_ext3.recverrors);
						dbprintf_voip(LOG_INFO, "Recv Error Pkts=%u\n", current_ext3.recverrors);
					}
					if(strcmp(key_start, "Sent Error Pkts")==0 ) {
						sscanf(value_start, "%u", &current_ext3.xmiterrors);
						dbprintf_voip(LOG_INFO, "Sent Error Pkts=%u\n", current_ext3.xmiterrors);
					}
					if(strcmp(key_start, "Last Sent Payload Type")==0 ) {
						sscanf(value_start, "%hu", &current_ext3.encodetype);
						dbprintf_voip(LOG_INFO, "Last Sent Payload Type=%u\n", current_ext3.encodetype);
					}
					if(strcmp(key_start, "Last Recv Samples")==0 ) {
						sscanf(value_start, "%hu", &current_ext3.packetsize);
						dbprintf_voip(LOG_INFO, "Last Recv Samples=%u\n", current_ext3.packetsize);
					}
					if(strcmp(key_start, "Recv Listen Count")==0 ) {
						sscanf(value_start, "%u", &current_ext3.listens);
						dbprintf_voip(LOG_INFO, "Recv Listen Count=%u\n", current_ext3.listens);
					}
					if(strcmp(key_start, "Sequence Error Pkts")==0 ) {
						sscanf(value_start, "%u", &current_ext3.sequenceerr);
						dbprintf_voip(LOG_INFO, "Sequence Error Pkts=%u\n", current_ext3.sequenceerr);
					}
					if(strcmp(key_start, "Lost Pkts")==0 ) {
						sscanf(value_start, "%u", &current_ext3.dropouts);
						dbprintf_voip(LOG_INFO, "Lost Pkts=%u\n", current_ext3.dropouts);
					}
					if(strcmp(key_start, "Under Runs Pkts")==0 ) {
						sscanf(value_start, "%u", &current_ext3.underruns);
						dbprintf_voip(LOG_INFO, "Under Runs Pkts=%u\n", current_ext3.underruns);
					}
					if(strcmp(key_start, "SID Pkts")==0 ) {
						sscanf(value_start, "%u", &current_ext3.recvcomfortnoisepkts);
						dbprintf_voip(LOG_INFO, "SID Pkts=%u\n", current_ext3.recvcomfortnoisepkts);
					}
				}
			break;
			}
			pos++;
			token=strtok( NULL , "\n");
		}
		current_ext3.missingrtp=current_ext3.dropouts+current_ext3.sequenceerr+current_ext3.underruns;
	}

	//1: RecvErrors (4byte,unsigned integer,R)
	if (GET_BIT(mask, 0) > 0) {
		/* get only */
		unsigned int *omci_val_p = &(parm->recverrors);
		if (action == VOIP_HW_ACTION_GET) {
			*omci_val_p = current_ext3.recverrors;
		}
		SET_BIT(mask_r, 0);
	}

	//2: XmitErrors (4byte,unsigned integer,R)
	if (GET_BIT(mask, 1) > 0) {
		/* get only */
		unsigned int *omci_val_p = &(parm->xmiterrors);
		if (action == VOIP_HW_ACTION_GET) {
			*omci_val_p = current_ext3.xmiterrors;
		}
		SET_BIT(mask_r, 1);
	}

	//3: EncodeType (2byte,unsigned integer,R)
	if (GET_BIT(mask, 2) > 0) {
		/* get only */
		unsigned short *omci_val_p = &(parm->encodetype);
		if (action == VOIP_HW_ACTION_GET) {
			*omci_val_p = current_ext3.encodetype;
		}
		SET_BIT(mask_r, 2);
	}

	//4: PacketSize (2byte,unsigned integer,R)
	if (GET_BIT(mask, 3) > 0) {
		/* get only */
		unsigned short *omci_val_p = &(parm->packetsize);
		if (action == VOIP_HW_ACTION_GET) {
			*omci_val_p = current_ext3.packetsize;
		}
		SET_BIT(mask_r, 3);
	}

	//5: MissingRtp (4byte,unsigned integer,R)
	if (GET_BIT(mask, 4) > 0) {
		/* get only */
		unsigned int *omci_val_p = &(parm->missingrtp);
		if (action == VOIP_HW_ACTION_GET) {
			*omci_val_p = current_ext3.missingrtp;
		}
		SET_BIT(mask_r, 4);
	}

	//6: SequenceErr (4byte,unsigned integer,R)
	if (GET_BIT(mask, 5) > 0) {
		/* get only */
		unsigned int *omci_val_p = &(parm->sequenceerr);
		if (action == VOIP_HW_ACTION_GET) {
			*omci_val_p = current_ext3.sequenceerr;
		}
		SET_BIT(mask_r, 5);
	}

	//7: DropOuts (4byte,unsigned integer,R)
	if (GET_BIT(mask, 6) > 0) {
		/* get only */
		unsigned int *omci_val_p = &(parm->dropouts);
		if (action == VOIP_HW_ACTION_GET) {
			*omci_val_p = current_ext3.dropouts;
		}
		SET_BIT(mask_r, 6);
	}

	//8: UnderRuns (4byte,unsigned integer,R)
	if (GET_BIT(mask, 7) > 0) {
		/* get only */
		unsigned int *omci_val_p = &(parm->underruns);
		if (action == VOIP_HW_ACTION_GET) {
			*omci_val_p = current_ext3.underruns;
		}
		SET_BIT(mask_r, 7);
	}

	//9: Listens (4byte,unsigned integer,R)
	if (GET_BIT(mask, 8) > 0) {
		/* get only */
		unsigned int *omci_val_p = &(parm->listens);
		if (action == VOIP_HW_ACTION_GET) {
			*omci_val_p = current_ext3.listens;
		}
		SET_BIT(mask_r, 8);
	}

	//10: RecvBadSrcPorts (4byte,unsigned integer,R)
	if (GET_BIT(mask, 9) > 0) {
		/* get only */
		unsigned int *omci_val_p = &(parm->recvbadsrcports);
		if (action == VOIP_HW_ACTION_GET) {
			*omci_val_p = 0;	//RecvBadSrcPorts always 0
		}
		SET_BIT(mask_r, 9);
	}

	//11: RecvComfortNoisePkts (4byte,unsigned integer,R)
	if (GET_BIT(mask, 10) > 0) {
		/* get only */
		unsigned int *omci_val_p = &(parm->recvcomfortnoisepkts);
		if (action == VOIP_HW_ACTION_GET) {
			*omci_val_p = current_ext3.recvcomfortnoisepkts;
		}
		SET_BIT(mask_r, 10);
	}

out_free:
	if(result != NULL)
		free_safe(result);
	
	mask[0] = mask_r[0];
	mask[1] = mask_r[1];

	return 0;
}

//ME 65283 proprietary_calix
int
voip_hw_voip_call_statistics_extension4(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_voip_call_statistics_extension4_t *parm)
{
	int ret = -1;
	unsigned char mask_r[2] = {0, 0};

	dbprintf_voip(LOG_WARNING, "action=%s, port=%d, mask=0x%02x%02x\n", voip_hw_action_str[action%4], port, mask[0], mask[1]);

	/* recursive when called with all port */
	if (port == VOIP_HW_GLOBAL_PORT_DEF) {
		int i;
		for (i = 0; i < VOIP_HW_MAX_PORT_NUMBER; i++) {
			mask_r[0] = mask[0];
			mask_r[1] = mask[1];
			ret = voip_hw_voip_call_statistics_extension4(action, i, mask_r, parm);
			if(ret != 0)
				return ret;
		}
		return 0;
	}

	dbprintf_voip(LOG_INFO, "action=%u, port=%u, mask=0x%.2X,0x%.2X\n", action, port, mask[0], mask[1]);

	//1: LocalUdpPort (2byte,unsigned integer,R)
	if (GET_BIT(mask, 0) > 0) {
		/* get only */
		unsigned short *omci_val_p = &(parm->localudpport);
		if (action == VOIP_HW_ACTION_GET) {
			int temp_int;
			ret = voipclient_cmd_get_int(&temp_int, "localmediaport1 %d", port);
			if (ret != 0)
				goto bit0_out;
			*omci_val_p = htons(temp_int);
		}
		SET_BIT(mask_r, 0);
	}
bit0_out:

	//2: RemIpAdd (4byte,unsigned integer-ipv4,R)
	if (GET_BIT(mask, 1) > 0) {
		/* get only */
		unsigned int *omci_val_p = &(parm->remipadd);
		if (action == VOIP_HW_ACTION_GET) {
			unsigned char ip_string[16];
			int ip_string_len=16;
			ret = voipclient_cmd_get_string(ip_string, ip_string_len, "remotemediaaddr1 %d", port);
			if (ret != 0)
				goto bit1_out;
			*omci_val_p = inet_addr(ip_string); //ntohl will use in meinfo_hw_65283.c
			if( *omci_val_p == INADDR_NONE ) 
				*omci_val_p=0;
		}
		SET_BIT(mask_r, 1);
	}
bit1_out:

	//3: RemUdpPort (2byte,unsigned integer,R)
	if (GET_BIT(mask, 2) > 0) {
		/* get only */
		unsigned short *omci_val_p = &(parm->remudpport);
		if (action == VOIP_HW_ACTION_GET) {
			int temp_int;
			ret = voipclient_cmd_get_int(&temp_int, "remotemediaport1 %d", port);
			if (ret != 0)
				goto bit2_out;
			*omci_val_p = htons(temp_int);
		}
		SET_BIT(mask_r, 2);
	}
bit2_out:

	//4: SecRemIpAddr (4byte,unsigned integer-ipv4,R)
	if (GET_BIT(mask, 3) > 0) {
		/* get only */
		unsigned int *omci_val_p = &(parm->secremipaddr);
		if (action == VOIP_HW_ACTION_GET) {
			unsigned char ip_string[16];
			int ip_string_len=16;
			ret = voipclient_cmd_get_string(ip_string, ip_string_len, "remotemediaaddr2 %d", port);
			if (ret != 0)
				goto bit3_out;
			*omci_val_p = inet_addr(ip_string); //ntohl will use in meinfo_hw_65283.c
			if( *omci_val_p == INADDR_NONE ) 
				*omci_val_p=0;
		}
		SET_BIT(mask_r, 3);
	}
bit3_out:

	//5: SecRemUdpPort (2byte,unsigned integer,R)
	if (GET_BIT(mask, 4) > 0) {
		/* get only */
		unsigned short *omci_val_p = &(parm->secremudpport);
		if (action == VOIP_HW_ACTION_GET) {
			int temp_int;
			ret = voipclient_cmd_get_int(&temp_int, "remotemediaport2 %d", port);
			if (ret != 0)
				goto bit4_out;
			*omci_val_p = htons(temp_int);
		}
		SET_BIT(mask_r, 4);
	}
bit4_out:

	//6: Qos (4byte,unsigned integer,R)
	if (GET_BIT(mask, 5) > 0) {
		/* get only */
		unsigned int *omci_val_p = &(parm->qos);
		if (action == VOIP_HW_ACTION_GET) {
			int temp_int;
			ret = voipclient_cmd_get_int(&temp_int, "mediatos");	//this parameter is for whole system
			if (ret != 0)
				goto bit5_out;
			*omci_val_p = (temp_int >> 2);
		}
		SET_BIT(mask_r, 5);
	}
bit5_out:

	//7: IpLineStatus (2byte,unsigned integer,R)
	if (GET_BIT(mask, 6) > 0) {
		/* get only */
		unsigned short *omci_val_p = &(parm->iplinestatus);
		if (action == VOIP_HW_ACTION_GET) {
			int epenable, eplinestate, epmediadir, epsessiontype;
			unsigned short line_status=0;

			ret = voipclient_cmd_get_int(&epenable, "epenable %d", port);

			if (ret == 0 && epenable !=0) {
				if(epenable==1)
					line_status &= 0x01;
				ret = voipclient_cmd_get_int(&epmediadir, "rtpmediadir %d", port);
				if ( ret==0 && epmediadir&0x02 )
					line_status &= 0x02;
				if ( ret==0 && epmediadir&0x01 )
					line_status &= 0x04;

				ret = voipclient_cmd_get_int(&eplinestate, "eplinestate %d", port);
				if ( ret==0 && eplinestate==11 )
					line_status &= 0x08;

				ret = voipclient_cmd_get_int(&epsessiontype, "voipsessiontype %d", port);
				if ( ret==0 && epsessiontype==2 )
					line_status &= 0x16;
				if ( ret==0 && epsessiontype==4 )
					line_status &= 0x32;

				*omci_val_p = line_status;
			} else {
				*omci_val_p = 0;
			}
		}
		SET_BIT(mask_r, 6);
	}

	//8: Active911Call (1byte,enumeration,R)
	if (GET_BIT(mask, 7) > 0) {
		/* get only */
		unsigned char *omci_val_p = &(parm->active911call);
		if (action == VOIP_HW_ACTION_GET) {
			int temp_int;
			ret = voipclient_cmd_get_int(&temp_int, "emergencycallstate %d", port);
			if (ret != 0)
				goto bit7_out;
			if(temp_int==1 || temp_int==2 ||temp_int==3 ||temp_int==5)
				*omci_val_p = 1;
			else
				*omci_val_p = 0;	//0 or 4, or others
		}
		SET_BIT(mask_r, 7);
	}
bit7_out:

	//9: ClrErrorCntrs (1byte,unsigned integer,W)
	//write only
	if (GET_BIT(mask, 8) > 0) {
		unsigned char *omci_val_p = &(parm->clrerrorcntrs);
		if (action == VOIP_HW_ACTION_GET) {
			//write only
			*omci_val_p = 0;
		} else {
			ret = -1;
		}

		if(ret != 0)
			goto bit8_out;
		SET_BIT(mask_r, 8);
	}
bit8_out:

	//10: ClrCallCntrs (1byte,unsigned integer,W)
	//write only
	if (GET_BIT(mask, 9) > 0) {
		unsigned char *omci_val_p = &(parm->clrcallcntrs);
		if (action == VOIP_HW_ACTION_GET) {
			//write only
			*omci_val_p = 0;
		} else {
			ret = -1;
		}

		if(ret != 0)
			goto release_out;
		SET_BIT(mask_r, 9);
	}

release_out:
	
	mask[0] = mask_r[0];
	mask[1] = mask_r[1];

	return 0;
}

//ME 244 proprietary_calix
int
voip_hw_potsuniext(enum voip_hw_action_t action, int port, unsigned char mask[2], unsigned char *parm)
{
	int ret = -1;
	unsigned char mask_r[2] = {0, 0};

	dbprintf_voip(LOG_WARNING, "action=%s, port=%d, mask=0x%02x%02x\n", voip_hw_action_str[action%4], port, mask[0], mask[1]);

	/* recursive when called with all port */
	if (port == VOIP_HW_GLOBAL_PORT_DEF) {
		int i;
		for (i = 0; i < VOIP_HW_MAX_PORT_NUMBER; i++) {
			mask_r[0] = mask[0];
			mask_r[1] = mask[1];
			ret = voip_hw_potsuniext(action, i, mask_r, parm);
			if(ret != 0)
				return ret;
		}
		return 0;
	}

	dbprintf_voip(LOG_INFO, "action=%u, port=%u, mask=0x%.2X,0x%.2X\n", action, port, mask[0], mask[1]);

	//1: CallStatus (1byte,unsigned integer,R)
	if (GET_BIT(mask, 0) > 0) {
		/* get only */
		unsigned char *omci_val_p = parm;
		if (action == VOIP_HW_ACTION_GET) {
			int temp_int;
			ret = voipclient_cmd_get_int(&temp_int, "eplinestate %d", port);
			if (ret != 0) {
				*omci_val_p = 0;
				goto release_out;
			}
			
/* need translation from fvcli to omci
0	Disabled
1	Idle
2	Dialing
3	Invite
4	Ringback
5	Busy
6	Ringing
7	Active
8	Hold
9	Disconnecting
10	Disconnected
*/
			
			switch (temp_int) {
			case 0:
			case 10:
			case 12:
				*omci_val_p = 1;	//Idle
			break;

			case 1:
			case 2:
				*omci_val_p = 2;	//Dialing
			break;

			case 13:
				*omci_val_p = 3;	//Invite
			break;
			case 4:
				*omci_val_p = 4;	//Ringback
			break;
			case 8:
			case 9:
				*omci_val_p = 5;	//Busy
			break;
			case 3:
			case 5:
				*omci_val_p = 6;	//Ringing
			break;
			case 6:
				*omci_val_p = 7;	//Active
			break;

			case 11:
				*omci_val_p = 8;	//Hold
			break;
			case 7:
				*omci_val_p = 9;	//Disconnecting
			break;
			case 14:
				*omci_val_p = 10;	//Disconnected
			break;
			}
		}
		SET_BIT(mask_r, 0);
	}
release_out:
	
	mask[0] = mask_r[0];
	mask[1] = mask_r[1];

	return 0;
}

