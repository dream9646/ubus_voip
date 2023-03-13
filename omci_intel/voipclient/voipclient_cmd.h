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
 * Module  : voipclient
 * File    : voipclient_cmd.h
 *
 ******************************************************************/

#ifndef VOIP_OMCI_H_
#define VOIP_OMCI_H_

#define CMD_ACT_GET	"get"
#define CMD_ACT_SET	"set"

/*jone20100812:get per-bit value for mask*/
#define GET_BIT(mask, bit)			\
({						\
	unsigned char data;			\
	int index;				\
	if ((bit) > 7){				\
		index = 15 - (bit);		\
		data = (mask)[1];			\
	} else {				\
		index = 7 - (bit);			\
		data = (mask)[0];			\
	}					\
	(data >> index) & 0x01;			\
})

#define SET_BIT(mask, bit)	\
{	\
	unsigned char *data;	\
	int index;	\
	if ((bit) > 7){	\
		index = 15 - (bit);	\
		data = &(mask)[1];	\
	} else {	\
		index = 7 - (bit);	\
		data = &(mask)[0];	\
	}	\
	*data |= (1<<index);	\
}

#define CLEAR_BIT(mask, bit)	\
{	\
	unsigned char *data;	\
	int index;	\
	if ((bit) > 7){	\
		index = 15 - (bit);	\
		data = &(mask)[1];	\
	} else {	\
		index = 7 - (bit);	\
		data = &(mask)[0];	\
	}	\
	*data &= ~(1<<index);	\
}

#define LABEL_BIT_OUT(b)	bit##b##_out


/* common codes for simple get/set CLI */
/* takes outside value: action, port */
/* changes the value of ret and var */
#define Simple_Get_Set_Int(fmt_get, fmt_set, var) \
	{ \
		ret = 0;	\
		if (action == VOIP_HW_ACTION_GET) {	\
			int temp_int;	\
			ret = voipclient_cmd_get_int(&temp_int, fmt_get, port);	\
			if (ret == 0)	\
				var = temp_int;	\
		}	\
	}

#define Simple_Get_Set_String(fmt_get, fmt_set, var, len) \
	{ \
		ret = 0;	\
		if (action == VOIP_HW_ACTION_GET) {	\
			ret = voipclient_cmd_get_string(var, len, fmt_get, port);	\
		}	\
	}

extern int voipclient_cmd_init();
extern void voipclient_cmd_destroy();


/*add by volanda 20100714*/
extern int voipclient_cmd_get_hookstate(char *value, int port);
extern int voipclient_cmd_get_serverstatus(char *value, int port);

/*GET COMMAND*/
extern int voipclient_cmd_get_txgaintenthdb(int port, int *value);
extern int voipclient_cmd_get_rxgaintenthdb(int port, int *value);
extern int voipclient_cmd_get_fxsringimpedance_0(int *value);
extern int voipclient_cmd_get_fxsringimpedance_1(int *value);
extern int voipclient_cmd_get_txpacketsize_call(int port, int call, short *value);
extern int voipclient_cmd_get_callwaitingcancelkeys(char *value);
extern int voipclient_cmd_get_holdcallkeys(char *value);
extern int voipclient_cmd_get_cidblockactivekeys(char *value);
extern int voipclient_cmd_get_cidblockdeactivekeys(char *value);
extern int voipclient_cmd_get_dndactivekeys(char *value);
extern int voipclient_cmd_get_dnddeactivekeys(char *value);

/*jone20100908 for omci: get name flag*/
extern int voipclient_cmd_get_anonymous_name(int port, int *flag);

/*volanda add 100909*/
extern int voipclient_cmd_get_dnd(int port, int *value);
extern int voipclient_cmd_get_callwaitingopt(int port, int *value);
extern int voipclient_cmd_get_portsession(int port, int *value);
extern int voipclient_cmd_get_configurationstate(int *value);

//cater add 201101 for omci
extern int voipclient_cmd_get_transferopt(int port, int *value);
extern int voipclient_cmd_get_threeway(int port, int *value);
extern int voipclient_cmd_get_flash_on_emergency_service_call(int port, int *value);
extern int voipclient_cmd_get_mwivi(int port, int *value);
extern int voipclient_cmd_get_dcfe(int port, int *value);
extern int voipclient_cmd_get_dial_tone_feature_delay_timer(int port, int *value);
extern int voipclient_cmd_get_directconnecturi(int port, char *value);
extern int voipclient_cmd_get_bridgedlineagenturi(int port, char *value);
extern int voipclient_cmd_get_conferencefactoryuri(int port, char *value);
extern int voipclient_cmd_get_dndpinchange(char *value);
extern int voipclient_cmd_get_emergencyservicenumber(char *value);
extern int voipclient_cmd_get_intercomservice(char *value);
extern int voipclient_cmd_get_callparkkeys(char *value);

/*cater20110423 for calldestaddr*/
extern int voipclient_cmd_get_destaddr_call(int port, int call, char *value);

/*cater 20110307 for omci:longtimeout and shorttimeout*/
extern int voipclient_cmd_get_rtperrors(int port, int *value);
extern int voipclient_cmd_get_packetloss(int port, int *value);
extern int voipclient_cmd_get_maximumjitter(int port, int *value);
extern int voipclient_cmd_get_maxtimertcppackets(int port, int *value);

/* cater 20110520 for FH */
extern int voipclient_cmd_get_digitmapmatchmode(int port, int *value);
extern int voipclient_cmd_get_registrarkeepaliveenable(int port, int *value);
extern int voipclient_cmd_get_registrarkeepaliveinterval(int port, int *value);
extern int voipclient_cmd_get_registrarkeepalivemaxtry(int port, int *value);
extern int voipclient_cmd_get_payloadtype(int port, char *codec_name, int *value);
extern int voipclient_cmd_get_sipregister(int port, int *value);
extern int voipclient_cmd_get_mlt(int port, const char *test, char *result, int result_len);

extern int voipclient_cmd_get_pos_number(int port, int *value);
extern int voipclient_cmd_get_ipt_number(int port, int *value);
extern int voipclient_cmd_get_pos(int port, char *value, int length, int *num);
extern int voipclient_cmd_get_ipt(int port, char *value, int length, int *num);


extern int voipclient_cmd_common_v(char *result, int result_len, const char *action, const char *fmt_cmd, va_list ap);
extern int voipclient_cmd_get_string(char *result, int result_len, const char *fmt_cmd, ...) __attribute__((format(printf,3,4)));
extern int voipclient_cmd_get_int(int *result, const char *fmt_cmd, ...) __attribute__((format(printf,2,3)));
extern int voipclient_cmd_set(const char *fmt_cmd, ...) __attribute__((format(printf,1,2)));

extern int voipclient_cmd_codecpriority(int action, int port, int priority, unsigned char *omci_val_p);
extern int voipclient_cmd_get_digitmap(int port, char *buffer, int length, int *num, int type);
extern char* voipclient_cmd_dump_buf(char *buf, char *string_g, int length);

extern int voipclient_cmd_get_cidtype(int port, int *flag);
extern int voipclient_cmd_get_cidblocking(int port, int *flag);
extern int voipclient_cmd_get_dial_plan_format(int port, int *value);

#endif /*VOIP_OMCI */
