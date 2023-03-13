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
 * File    : voipclient_cmd.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include "util.h"
#include "util_run.h"
#include "voip_hw.h"
#include "voipclient_util.h"
#include "voipclient_comm.h"
#include "voipclient_cmd.h"
#include "fwk_msgq.h"

extern struct fwk_mutex_t mutex_voip;

int
voipclient_cmd_init(void)
{
	int ret;
	ret = fwk_mutex_create(&mutex_voip);
	return ret;
}

void
voipclient_cmd_destroy(void)
{
	fwk_mutex_destroy(&mutex_voip);
	return;
}

/*
0 -onhook 
1 -offhook
*/
int
voipclient_cmd_get_hookstate(char *value, int port)
{
	char buf[255];
	char cmd[255];
	int ret = -1;
	memset(buf, 0, 255);
	if (value == NULL) {
		ret = -1;
	}
	snprintf(cmd, sizeof(cmd), "get hookstate %d\n\n", port);
	ret = voipclient_comm_cli(cmd, buf, sizeof(buf));
	if (ret == 0) {
		if (0 == strncmp(buf, "Onhook", 6)) {
			printf("get onhook state\n");
			*value = '0';
			ret = 0;
		} else if (0 == strncmp(buf, "Offhook", 7)) {
			printf("get offhook state \n");
			*value = '1';
			ret = 0;
		} else {
			ret = -1;
		}
	}
	return ret;
}

/*
server status:

0 None/initial
1 Registered
2 In session
3 Failed registration XC icmp error
4 Failed registration XC failed tcp
5 Failed registration XC failed authentication
6 Failed registration XC timeout
7 Failed registration XC server fail code
8 Failed invite XC icmp error
9 Failed invite XC failed tcp
10 Failed invite XC failed authentication
11 Failed invite XC timeout
12 Failed invite XC server fail code
13 Port not configured
14 Config done
*/
/*
fvcli value = get epregstate %d
	omci <-> fvcli value
	0    <->  0:   INITIAL
	1    <->  1:   REGISTERED
	1    <->  2:   IN_SESSION
	3    <->  3:   FAILED_ICMP
	4    <->  4:   FAILED_TCP
	5    <->  5:   FAILED_AUTH
	6    <->  6:   FAILED_TIMEOUT
	7    <->  7:   FAILED_SERVER
	7    <->  99:  FAILED_OTHER
	0    <->  100: REGISTERING
	0    <->  101: UNREGISTERING
*/
int
voipclient_cmd_get_serverstatus(char *value, int port)
{
	char cmd[255];
	char buf[255];
	int ret = 0;
	memset(buf, 0, 255);
	if (value == NULL) {
		ret = -1;
	}
	snprintf(cmd, sizeof(cmd), "get epregstate %d\n\n", port);
	ret = voipclient_comm_cli(cmd, buf, sizeof(buf));
	if (ret == 0) {
		switch(atoi(buf)) {
		case 0:
			*value=0;
			break;
		case 1:
			*value=1;
			break;
		case 2:
			*value=1;
			break;
		case 3:
			*value=3;
			break;
		case 4:
			*value=4;
			break;
		case 5:
			*value=5;
			break;
		case 6:
			*value=6;
			break;
		case 7:
			*value=7;
			break;
		case 99:
			*value=7;
			break;
		case 100:
			*value=0;
			break;
		case 101:
			*value=0;
			break;
		}
	}
	return ret;
}

/*voice level is 1-8*/
int
voipclient_cmd_get_txgaintenthdb(int port, int *value)
{
	char cmd[255];
	char buf[255];
	int ret = 0;
	memset(buf, 0, 255);
	if (value == NULL) {
		ret = -1;
	}
	snprintf(cmd, sizeof(cmd), "get txgaintenthdb %d\n\n", port);
	ret = voipclient_comm_cli(cmd, buf, sizeof(buf));
	if (ret == 0) {
		printf("txgaintenthdb 0:%s\n", buf);
		*value = atoi(buf);
	}
	return ret;
}

int
voipclient_cmd_get_rxgaintenthdb(int port, int *value)
{
	char cmd[255];
	char buf[255];
	int ret = 0;
	memset(buf, 0, 255);
	if (value == NULL) {
		ret = -1;
	}
	snprintf(cmd, sizeof(cmd), "get rxgaintenthdb %d\n\n", port);
	ret = voipclient_comm_cli(cmd, buf, sizeof(buf));
	if (ret == 0) {
		printf("rxgaintenthdb 0:%s\n", buf);
		*value = atoi(buf);
	}
	return ret;
}

int
voipclient_cmd_get_fxsringimpedance_0(int *value)
{
	char cmd[255];
	char buf[255];
	int ret = 0;
	memset(buf, 0, 255);
	if (value == NULL) {
		ret = -1;
	}
	snprintf(cmd, sizeof(cmd), "get tdiimpedanceindex 0 \n\n");
	ret = voipclient_comm_cli(cmd, buf, sizeof(buf));
	if (ret == 0) {
		printf("tdiimpedanceindex 0:%s\n", buf);
		*value = atoi(buf);
	}
	return ret;
}

int
voipclient_cmd_get_fxsringimpedance_1(int *value)
{
	char cmd[255];
	char buf[255];
	int ret = 0;
	memset(buf, 0, 255);
	if (value == NULL) {
		ret = -1;
	}
	snprintf(cmd, sizeof(cmd), "get tdiimpedanceindex 1 \n\n");
	ret = voipclient_comm_cli(cmd, buf, sizeof(buf));
	if (ret == 0) {
		printf("tdiimpedanceindex 1:%s\n", buf);
		*value = atoi(buf);
	}
	return ret;
}

/* give url, this function set domainname and port in the url
 * eg: url = "abc 123", then domainname->'a', port->'1'
 *     url becomes "abc"NULL"123"
 * if not found, domainname will point to the NULL before port
 * port will point to url's terminating NULL
 * if error, return -1
 */
int voipclient_util_parse_url(char *url, char **domainname, char **port)
{
	char *separater;

	if(url==NULL || domainname==NULL || port==NULL)
		return -1;

	separater = strchr(url, ':');
	if(separater != NULL) {
		*separater = '\0';
		*domainname = url;
		*port = separater+1;
	}
	else {
		*domainname = url;
		*port = url+strlen(url);	/* the terminating NULL */
	}

	return 0;
}

int
voipclient_cmd_get_txpacketsize_call(int port, int call, short *value)
{
	char cmd[255];
	char buf[255];
	int ret = 0;
	memset(buf, 0, 255);
	if (value == NULL) {
		ret = -1;
	}
	snprintf(cmd, sizeof(cmd), "get txpacketsize%d %d\n\n", call, port);
	ret = voipclient_comm_cli(cmd, buf, sizeof(buf));
	if (ret == 0) {
		*value = atoi(buf);
	}
	return ret;
}

int
voipclient_cmd_get_callwaitingcancelkeys(char *value)
{
	char cmd[255];
	char buf[255];
	int ret = 0;
	memset(buf, 0, 255);
	if (value == NULL) {
		return -1;
	}
	snprintf(cmd, sizeof(cmd), "get callwaitingcancelkeys \n\n");
	ret = voipclient_comm_cli(cmd, buf, 255);
	if (ret == 0) {
		strcpy(value, buf);
	}
	return ret;
}

int
voipclient_cmd_get_holdcallkeys(char *value)
{
	char cmd[255];
	char buf[255];
	int ret = 0;
	memset(buf, 0, 255);
	if (value == NULL) {
		return -1;
	}
	snprintf(cmd, sizeof(cmd), "get holdcallkeys \n\n");
	ret = voipclient_comm_cli(cmd, buf, 255);
	if (ret == 0) {
		strcpy(value, buf);
	}
	return ret;
}

int
voipclient_cmd_get_cidblockactivekeys(char *value)
{
	char cmd[255];
	char buf[255];
	int ret = 0;
	memset(buf, 0, 255);
	if (value == NULL) {
		return -1;
	}
	snprintf(cmd, sizeof(cmd), "get cidblockactivekeys \n\n");
	ret = voipclient_comm_cli(cmd, buf, 255);
	if (ret == 0) {
		strcpy(value, buf);
	}
	return ret;
}

int
voipclient_cmd_get_cidblockdeactivekeys(char *value)
{
	char cmd[255];
	char buf[255];
	int ret = 0;
	memset(buf, 0, 255);
	if (value == NULL) {
		return -1;
	}
	snprintf(cmd, sizeof(cmd), "get cidblockdeactivekeys \n\n");
	ret = voipclient_comm_cli(cmd, buf, 255);
	if (ret == 0) {
		strcpy(value, buf);
	}
	return ret;
}

int
voipclient_cmd_get_dndactivekeys(char *value)
{
	char cmd[255];
	char buf[255];
	int ret = 0;
	memset(buf, 0, 255);
	if (value == NULL) {
		return -1;
	}
	snprintf(cmd, sizeof(cmd), "get dndactivekeys \n\n");
	ret = voipclient_comm_cli(cmd, buf, 255);
	if (ret == 0) {
		strcpy(value, buf);
	}
	return ret;
}

int
voipclient_cmd_get_dnddeactivekeys(char *value)
{
	char cmd[255];
	char buf[255];
	int ret = 0;
	memset(buf, 0, 255);
	if (value == NULL) {
		return -1;
	}
	snprintf(cmd, sizeof(cmd), "get dnddeactivekeys \n\n");
	ret = voipclient_comm_cli(cmd, buf, 255);
	if (ret == 0) {
		strcpy(value, buf);
	}
	return ret;
}

/*jone20100908 for omci: get name flag*/
int
voipclient_cmd_get_anonymous_name(int port, int *flag)
{
	char cmd[255];
	char buf[255];
	int ret = 0;
	memset(buf, 0, 255);
	if (flag == NULL) {
		return -1;
	}
	snprintf(cmd, sizeof(cmd), "get ciddatamsgformat %d\n\n", port);
	ret = voipclient_comm_cli(cmd, buf, sizeof(buf));
	if (ret == 0) {
		*flag = atoi(buf);
	}
	return ret;
}

int
voipclient_cmd_get_dnd(int port, int *value)
{
	char cmd[255];
	char buf[255];
	int ret = 0;
	memset(buf, 0, 255);
	snprintf(cmd, sizeof(cmd), "get dndoption %d\n\n", port);
	ret = voipclient_comm_cli(cmd, buf, sizeof(buf));
	if (ret == 0) {
		*value = atoi(buf);
	}
	return ret;
}

int
voipclient_cmd_get_callwaitingopt(int port, int *value)
{
	char cmd[255];
	char buf[255];
	int ret = 0;
	memset(buf, 0, 255);
	snprintf(cmd, sizeof(cmd), "get callwaitingopt %d\n\n", port);
	ret = voipclient_comm_cli(cmd, buf, sizeof(buf));
	if (ret == 0) {
		*value = atoi(buf);
	}
	return ret;
}

int
voipclient_cmd_get_portsession(int port, int *value)
{
	char cmd[255];
	char buf[255];
	int ret = 0;
	memset(buf, 0, 255);
	snprintf(cmd, sizeof(cmd), "get voipsessiontype %d\n\n", port);
	ret = voipclient_comm_cli(cmd, buf, sizeof(buf));
	if (ret == 0) {
		*value = atoi(buf);
	}
	return ret;
}

int
voipclient_cmd_get_configurationstate(int *value)
{
	char cmd[255];
	char buf[255];
	int ret = 0;
	memset(buf, 0, 255);
	snprintf(cmd, sizeof(cmd), "get configurationstate\n\n");
	ret = voipclient_comm_cli(cmd, buf, sizeof(buf));
	if (ret == 0) {
		*value = atoi(buf);
	}
	return ret;
}

//cater add 201101
int
voipclient_cmd_get_transferopt(int port, int *value)
{
	char cmd[255];
	char buf[255];
	int ret = 0;
	memset(buf, 0, 255);
	snprintf(cmd, sizeof(cmd), "get transferoption %d\n\n", port);
	ret = voipclient_comm_cli(cmd, buf, sizeof(buf));
	if (ret == 0) {
		*value = atoi(buf);
	}
	return ret;
}

int
voipclient_cmd_get_threeway(int port, int *value)
{
	char cmd[255];
	char buf[255];
	int ret = 0;
	memset(buf, 0, 255);
	snprintf(cmd, sizeof(cmd), "get threeway %d\n\n", port);
	ret = voipclient_comm_cli(cmd, buf, sizeof(buf));
	if (ret == 0) {
		*value = atoi(buf);
	}
	return ret;
}

int
voipclient_cmd_get_flash_on_emergency_service_call(int port, int *value)
{
	int temp_int, ret;
	if((ret = voipclient_cmd_get_int(&temp_int, "flashhookoption %d", port)) == 0)
		*value= (temp_int & 0x04);	//0:disable, 1:enable
	return ret;
}

int
voipclient_cmd_get_mwivi(int port, int *value)
{
	char cmd[255];
	char buf[255];
	int ret = 0;
	memset(buf, 0, 255);
	snprintf(cmd, sizeof(cmd), "get mwitype %d\n\n", port);
	ret = voipclient_comm_cli(cmd, buf, sizeof(buf));
	if (ret == 0) {
		*value = atoi(buf);
	}
	return ret;
}

int
voipclient_cmd_get_dcfe(int port, int *value)
{
	char cmd[255];
	char buf[255];
	int ret = 0;
	memset(buf, 0, 255);
	snprintf(cmd, sizeof(cmd), "get hotlineoption %d\n\n", port);
	ret = voipclient_comm_cli(cmd, buf, sizeof(buf));
	if (ret == 0) {
		*value = atoi(buf);
	}
	return ret;
}

int
voipclient_cmd_get_dial_tone_feature_delay_timer(int port, int *value)
{
	char cmd[255];
	char buf[255];
	int ret = 0;
	memset(buf, 0, 255);
	snprintf(cmd, sizeof(cmd), "get hotlinedelaytime %d\n\n", port);
	ret = voipclient_comm_cli(cmd, buf, sizeof(buf));
	if (ret == 0) {
		*value = atoi(buf);
	}
	return ret;
}

int
voipclient_cmd_get_directconnecturi(int port, char *value)
{
	char cmd[255];
	char buf[255];
	int ret = 0;
	memset(buf, 0, 255);
	if (value == NULL) {
		ret = -1;
	}
	snprintf(cmd, sizeof(cmd), "get hotlineuri %d\n\n", port);
	ret = voipclient_comm_cli(cmd, buf, sizeof(buf));
	if (ret == 0) {
		strcpy(value, buf);
	}
	return ret;
}

int
voipclient_cmd_get_bridgedlineagenturi(int port, char *value)
{
	char cmd[255];
	char buf[255];
	int ret = 0;
	memset(buf, 0, 255);
	if (value == NULL) {
		ret = -1;
	}
	snprintf(cmd, sizeof(cmd), "get bridgedlineagenturi %d\n\n", port);
	ret = voipclient_comm_cli(cmd, buf, sizeof(buf));
	if (ret == 0) {
		strcpy(value, buf);
	}
	return ret;
}

int
voipclient_cmd_get_conferencefactoryuri(int port, char *value)
{
	char cmd[255];
	char buf[255];
	int ret = 0;
	memset(buf, 0, 255);
	if (value == NULL) {
		ret = -1;
	}
	snprintf(cmd, sizeof(cmd), "get conferencefactoryuri %d\n\n", port);
	ret = voipclient_comm_cli(cmd, buf, sizeof(buf));
	if (ret == 0) {
		strcpy(value, buf);
	}
	return ret;
}

int
voipclient_cmd_get_dndpinchange(char *value)
{
	char cmd[255];
	char buf[255];
	int ret = 0;
	memset(buf, 0, 255);
	if (value == NULL) {
		return -1;
	}
	snprintf(cmd, sizeof(cmd), "get dndpinchange \n\n");
	ret = voipclient_comm_cli(cmd, buf, 255);
	if (ret == 0) {
		strcpy(value, buf);
	}
	return ret;
}

int
voipclient_cmd_get_emergencyservicenumber(char *value)
{
	char cmd[255];
	char buf[255];
	int ret = 0;
	memset(buf, 0, 255);
	if (value == NULL) {
		return -1;
	}
	snprintf(cmd, sizeof(cmd), "get emergencyservicenumber \n\n");
	ret = voipclient_comm_cli(cmd, buf, 255);
	if (ret == 0) {
		strcpy(value, buf);
	}
	return ret;
}

int
voipclient_cmd_get_intercomservice(char *value)
{
	char cmd[255];
	char buf[255];
	int ret = 0;
	memset(buf, 0, 255);
	if (value == NULL) {
		return -1;
	}
	snprintf(cmd, sizeof(cmd), "get intercomservice \n\n");
	ret = voipclient_comm_cli(cmd, buf, 255);
	if (ret == 0) {
		strcpy(value, buf);
	}
	return ret;
}

int
voipclient_cmd_get_callparkkeys(char *value)
{
	char cmd[255];
	char buf[255];
	int ret = 0;
	memset(buf, 0, 255);
	if (value == NULL) {
		return -1;
	}
	snprintf(cmd, sizeof(cmd), "get callparkkeys \n\n");
	ret = voipclient_comm_cli(cmd, buf, 255);
	if (ret == 0) {
		strcpy(value, buf);
	}
	return ret;
}

int
voipclient_cmd_get_destaddr_call(int port, int call, char *value)
{
	char cmd[255];
	char buf[255];
	int ret = 0;
	memset(buf, 0, 255);
	if (value == NULL) {
		ret = -1;
	}
	snprintf(cmd, sizeof(cmd), "get calldestaddr%d %d\n\n", call, port);
	ret = voipclient_comm_cli(cmd, buf, 255);
	if (ret == 0) {
		strcpy(value, buf);
	}
	return ret;
}

/* robert20110314 for omci: get calllog */
int
voipclient_cmd_get_calllog(char *value, int length, int *num)
{
	char cmd[255];
	int ret = 0;
	if (value == NULL || num == NULL) {
		return -1;
	}
	snprintf(cmd, sizeof(cmd), "get calllog\n\n");
	ret = voipclient_comm_cli(cmd, value, length);
	if (ret == 0) {
		char *ptr1, *ptr2;
		ptr1 = value;
		if ((ptr2 = strchr(ptr1, '\n')) != NULL && ptr1) {
			if (ptr2 == NULL) {
				*num = 0;
				return 0;
			}
			*ptr2++ = '\0';
			*num = atoi(ptr1);
			if (*num > 0) {
				while (*ptr2 != '\0') {
					*value++ = *ptr2++;
				}
				*value = '\0';
			}
		} else {
			*num = 0;
			return 0;
		}
	}
	return ret;
}

/* robert20110314 for omci: end calllog */
int
voipclient_cmd_get_rtperrors(int port, int *value)
{
	char cmd[255];
	char buf[255];
	int ret = 0;
	memset(buf, 0, 255);
	snprintf(cmd, sizeof(cmd), "get  rtperror %d\n\n", port);
	ret = voipclient_comm_cli(cmd, buf, sizeof(buf));
	if (ret == 0) {
		*value = atoi(buf);
	}
	return ret;
}

int
voipclient_cmd_get_packetloss(int port, int *value)
{
	char cmd[255];
	char buf[255];
	int ret = 0;
	memset(buf, 0, 255);
	snprintf(cmd, sizeof(cmd), "get  rtplossrat %d\n\n", port);
	ret = voipclient_comm_cli(cmd, buf, sizeof(buf));
	if (ret == 0) {
		*value = atoi(buf);
	}
	if (*value == 1) {	//convert based on G984.4-9.9.13
		*value = 0xFFFF;
	}
	return ret;
}

int
voipclient_cmd_get_maximumjitter(int port, int *value)
{
	char cmd[255];
	char buf[255];
	int ret = 0;
	memset(buf, 0, 255);
	snprintf(cmd, sizeof(cmd), "get  rtpjitter %d\n\n", port);
	ret = voipclient_comm_cli(cmd, buf, sizeof(buf));
	if (ret == 0) {
		*value = atoi(buf);
	}
	return ret;
}

int
voipclient_cmd_get_maxtimertcppackets(int port, int *value)
{
	char cmd[255];
	char buf[255];
	int ret = 0;
	memset(buf, 0, 255);
	snprintf(cmd, sizeof(cmd), "get   rtpdelay %d\n\n", port);
	ret = voipclient_comm_cli(cmd, buf, sizeof(buf));
	if (ret == 0) {
		*value = atoi(buf);
	}
	return ret;
}

int
voipclient_cmd_get_digitmapmatchmode(int port, int *value)
{
	char cmd[255];
	char buf[255];
	int ret = 0;
	memset(buf, 0, 255);
	snprintf(cmd, sizeof(cmd), "get digitmapmatchmode %d\n\n", port);
	ret = voipclient_comm_cli(cmd, buf, sizeof(buf));
	if (ret == 0) {
		*value = atoi(buf);
	}
	return ret;
}

int
voipclient_cmd_get_registrarkeepaliveenable(int port, int *value)
{
	char cmd[255];
	char buf[255];
	int ret = 0;
	memset(buf, 0, 255);
	snprintf(cmd, sizeof(cmd), "get registrarkeepaliveenable %d\n\n", port);
	ret = voipclient_comm_cli(cmd, buf, sizeof(buf));
	if (ret == 0) {
		*value = atoi(buf);
	}
	return ret;
}

int
voipclient_cmd_get_registrarkeepaliveinterval(int port, int *value)
{
	char cmd[255];
	char buf[255];
	int ret = 0;
	memset(buf, 0, 255);
	snprintf(cmd, sizeof(cmd), "get registrarkeepaliveinterval %d\n\n", port);
	ret = voipclient_comm_cli(cmd, buf, sizeof(buf));
	if (ret == 0) {
		*value = atoi(buf);
	}
	return ret;
}

int
voipclient_cmd_get_registrarkeepalivemaxtry(int port, int *value)
{
	char cmd[255];
	char buf[255];
	int ret = 0;
	memset(buf, 0, 255);
	snprintf(cmd, sizeof(cmd), "get registrarkeepalivemaxtry %d\n\n", port);
	ret = voipclient_comm_cli(cmd, buf, sizeof(buf));
	if (ret == 0) {
		*value = atoi(buf);
	}
	return ret;
}

int
voipclient_cmd_get_payloadtype(int port, char *codec_name, int *value)
{
	char cmd[255];
	char buf[255];
	int ret = 0;
	memset(buf, 0, 255);
	snprintf(cmd, sizeof(cmd), "get payloadtype %d %s\n\n", port, codec_name);
	ret = voipclient_comm_cli(cmd, buf, sizeof(buf));
	printf("%s: buf=%s\n",  __FUNCTION__, buf);
	if (ret == 0) {
		*value = atoi(buf);
	}
	return ret;
}

int
voipclient_cmd_get_sipregister(int port, int *value)
{
	char cmd[255];
	char buf[255];
	int ret = 0;
	memset(buf, 0, 255);
	snprintf(cmd, sizeof(cmd), "get sipregister %d\n\n", port);
	ret = voipclient_comm_cli(cmd, buf, sizeof(buf));
	if (ret == 0) {
		*value = atoi(buf);
	}
	return ret;
}

int
voipclient_cmd_get_mlt(int port, const char *test, char *result, int result_len)
{
	int ret = 0;
	char command[64];
	FILE *ppipe;
	int message_len = 4095;
	char *message = NULL;
	int message_read_len;
	char *resp, *resp_end;
	char temp_char;

	message = malloc_safe(message_len+1);
	if(message == NULL) {
		dbprintf_voip(LOG_ERR, "malloc() error!\n");
		ret = -1;
		goto out;
	}
	message[message_len] = 0;
	message_read_len = 0;

	/* stop voip, but not kill */
	util_run_by_system("kill -SIGSTOP `ps | grep \"  /opt/voip/fvphone\" | grep -v grep | cut -c1-5`");

	sprintf(command, "mlt_test --no_voip --channel %d %s", port, test);
	if( (ppipe = popen(command, "r")) == NULL ) {
		dbprintf_voip(LOG_ERR, "popen() error! command is '%s'\n", command);
		ret = -1;
		goto out;
	}

	message_read_len = fread(message, 1, message_len, ppipe);
	message[message_read_len] = 0;

	resp = strstr(message, "\nCH");
	if(resp==NULL) {
		dbprintf_voip(LOG_ERR, "can't find a good response. len=%d\n", message_read_len);
		dbprintf_voip(LOG_ERR, "dump result:\n%s\n", message);
		ret = -1;
		pclose(ppipe);
		goto out;
	}
	resp+=3;

	resp_end = strstr(resp, "\n\n");
	if(resp_end==NULL) {
		dbprintf_voip(LOG_ERR, "can't find a good response end.\n");
		ret = -1;
		pclose(ppipe);
		goto out;
	}

	temp_char = resp_end[2];
	resp_end[2] = 0;
	strncpy(result, resp, result_len);
	dbprintf_voip(LOG_ERR, "%s\n", result);
	resp_end[2] = temp_char;

	pclose(ppipe);

	/* kill voip, and looprun will restart it */
	util_run_by_system("kill -SIGKILL `ps | grep \"  /opt/voip/fvphone\" | grep -v grep | cut -c1-5`");

out:
	if(message != NULL)
		free_safe(message);
	return ret;
}

int
voipclient_cmd_get_pos_number(int port, int *value)
{
	char cmd[255];
	char buf[255];
	int ret = 0;

	if (value == NULL) {
		return -1;
	}
	memset(buf, 0, 255);
	snprintf(cmd, sizeof(cmd), "get posnum 0\n\n");
	ret = voipclient_comm_cli(cmd, buf, sizeof(buf));
	if (ret == 0) {
		*value = atoi(buf);
	}
	return ret;
}

int
voipclient_cmd_get_ipt_number(int port, int *value)
{
	char cmd[255];
	char buf[255];
	int ret = 0;

	if (value == NULL) {
		return -1;
	}
	memset(buf, 0, 255);
	snprintf(cmd, sizeof(cmd), "get iptnum 0\n\n");
	ret = voipclient_comm_cli(cmd, buf, sizeof(buf));
	if (ret == 0) {
		*value = atoi(buf);
	}
	return ret;
}

static char value_tmp2[4096];

int
voipclient_cmd_get_pos(int port, char *value, int length, int *num)
{
	char cmd[255];
	char value_tmp0[256], value_tmp1[256];
	int ret = 0;

	memset(value_tmp0, 0, sizeof(value_tmp0));
	memset(value_tmp1, 0, sizeof(value_tmp1));
	memset(value_tmp2, 0, sizeof(value_tmp2));
	
	if (value == NULL || num == NULL) {
		return -1;
	}
	
	if (port == VOIP_HW_GLOBAL_PORT_DEF)
		port = 0;
	
	snprintf(cmd, sizeof(cmd), "get postable 0\n\n");
	ret = voipclient_comm_cli(cmd, value, length);
	//dbprintf_voip(LOG_ERR, "ret = %d\n", ret);
	//dbprintf_voip(LOG_ERR, "value = %s\n", value);
	if (ret == 0) {
		char *ptr1, *ptr2;
		int begin_parse = 0;

		ptr1 = value;
		ptr2 = strtok(ptr1, "\n");
		if (ptr2 != NULL) {
			do{
				if( begin_parse ){
					char *result_tmp;
					//dbprintf_voip(LOG_ERR, "ptr2 = %s\n", ptr2);
					result_tmp = strcat(value_tmp0, ptr2);
					result_tmp = strcat(result_tmp, " ");
					strcpy(value_tmp0, result_tmp);
				}
				if(!strncmp("pos table:", ptr2, 10))
					begin_parse = 1;
			}while((ptr2=strtok(NULL, "\n")) != NULL);
			//dbprintf_voip(LOG_ERR, "value_tmp0 = %s\n", value_tmp0);
			ptr1 = value_tmp0;
			ptr2 = strtok(ptr1, " ");
			if (ptr2 != NULL) {
				do{
					char *id, *telnum, *postype;
					id = ptr2;
					telnum = strtok(NULL, " ");
					postype = strtok(NULL, " ");
					sprintf(value_tmp1, "pos=%s@p%d~%s:%s\n", telnum, port, id, postype);
					strcat(value_tmp2, value_tmp1);
				}while((ptr2=strtok(NULL, " ")) != NULL);
				strncpy(value, value_tmp2, length);
			} else {
				*num = -1;
				value[0]='\0';
			}
		}	
	}
	
	//dbprintf_voip(LOG_ERR, "value = %s\n", value);
	
	return ret;
}

//char value_test[]="id telnum postype\n0 2373 0\n1 4689 0\n\n";;
int
voipclient_cmd_get_ipt(int port, char *value, int length, int *num)
{
	char cmd[255];
	char value_tmp0[256], value_tmp1[256];
	int ret = 0;
	
	memset(value_tmp0, 0, sizeof(value_tmp0));
	memset(value_tmp1, 0, sizeof(value_tmp1));
	memset(value_tmp2, 0, sizeof(value_tmp2));
	if (value == NULL || num == NULL) {
		return -1;
	}
	if (port == VOIP_HW_GLOBAL_PORT_DEF)
		port = 0;
		
	snprintf(cmd, sizeof(cmd), "get ipttable 0\n\n");
	ret = voipclient_comm_cli(cmd, value, length);
//	dbprintf_voip(LOG_ERR, "ret = %d\n", ret);
//	strcpy(value, value_test);
//	dbprintf_voip(LOG_ERR, "value = %s\n", value);
	if (ret == 0) {
		char *ptr1, *ptr2;
		int begin_parse = 0;
			
		ptr1 = value;
		ptr2 = strtok(ptr1, "\n");
		if (ptr2 != NULL) {
			do{
				if( begin_parse ){
					char *result_tmp;
					//dbprintf_voip(LOG_ERR, "ptr2 = %s\n", ptr2);
					result_tmp = strcat(value_tmp0, ptr2);
					result_tmp = strcat(result_tmp, " ");
					strcpy(value_tmp0, result_tmp);
				}
				if(!strncmp("ipt table:", ptr2, 10))
					begin_parse = 1;
			}while((ptr2=strtok(NULL, "\n")) != NULL);
			//dbprintf_voip(LOG_ERR, "value_tmp0 = %s\n", value_tmp0);
			ptr1 = value_tmp0;
			ptr2 = strtok(ptr1, " ");
			if (ptr2 != NULL) {
				do{
					char *id, *telnum, *postype;
					id = ptr2;
					telnum = strtok(NULL, " ");
					postype = strtok(NULL, " ");
					sprintf(value_tmp1, "ipt=%s@p%d~%s:%s\n", telnum, port, id, postype);
					strcat(value_tmp2, value_tmp1);
				}while((ptr2=strtok(NULL, " ")) != NULL);
				strncpy(value, value_tmp2, length);
			}else {
				*num = -1;
				value[0]='\0';
			}
		}
	}
	
	//dbprintf_voip(LOG_ERR, "value = %s\n", value);

	return ret;
}

/*  send command and get result string, called with a va_list
    fmt_cmd must contain command syntex
    ex: fmt_cmd="siplocalport %d", arg0=0
    	then will send command "get siplocalport 0"
    	and set the result to the response
    return value:
    	0: ok
    	-1: error
 */ 
int
voipclient_cmd_common_v(char *result, int result_len, const char *action, const char *fmt_cmd, va_list ap)
{
	char *cmd0 = NULL;
	int cmd0_len;
	char *cmd1 = NULL;
	int cmd1_len;
	int ret = 0;

	cmd0_len = 4095;
	cmd0 = malloc_safe(cmd0_len+1);
	if(cmd0 == NULL) {
		ret = -1;
		goto out_return;
	}
	cmd0[cmd0_len]='\0';

	cmd1_len = 4095;
	cmd1 = malloc_safe(cmd1_len+1);
	if(cmd1 == NULL) {
		ret = -1;
		goto out_free0;
	}
	cmd1[cmd1_len]='\0';

	/* add action(get/set) and new lines */
	ret = snprintf (cmd0, cmd0_len, "%s %s\n\n", (char*)action, (char*)fmt_cmd);
	if(ret < 0 || ret >= cmd0_len) {
		ret = -1;
		goto out_free;
	}

	/* put args into the command */
	/* if also uses cmd0 as output, the %s will crash */
	ret = vsnprintf (cmd1, cmd1_len, cmd0, ap);
	if(ret < 0 || ret >= cmd1_len) {
		ret = -1;
		goto out_free;
	}

	ret = voipclient_comm_cli(cmd1, result, result_len);
	if(ret!=0)
		if(result!=NULL && result_len>0)
			result[0] = '\0';

out_free:
	if(cmd1!=NULL)
		free_safe(cmd1);
out_free0:
	if(cmd0!=NULL)
		free_safe(cmd0);
out_return:
	return ret;
}

/* get the result in type string */
int
voipclient_cmd_get_string(char *result, int result_len, const char *fmt_cmd, ...)
{
	va_list args;
	int ret = 0;

	/* build and send the command */
	va_start (args, fmt_cmd);
	ret = voipclient_cmd_common_v(result, result_len, CMD_ACT_GET, fmt_cmd, args);
	va_end (args);
	if(ret != 0) {
		ret = -1;
		goto out_return;
	}

out_return:
	return ret;
}

/* get the result in type int */
/* if result==NULL, nothing will be done */
int
voipclient_cmd_get_int(int *result, const char *fmt_cmd, ...)
{
	char *resp = NULL;
	int resp_len;
	va_list args;
	int ret = 0;

	if(result == NULL) {
		ret = -1;
		goto out_free;
	}

	resp_len = 31;
	resp = malloc_safe(resp_len+1);
	if(resp == NULL)
		return -1;
	resp[resp_len] = '\0';

	/* build and send the command */
	va_start (args, fmt_cmd);
	ret = voipclient_cmd_common_v(resp, resp_len, CMD_ACT_GET, fmt_cmd, args);
	va_end (args);
	if(ret != 0) {
		ret = -1;
		goto out_free;
	}

	*result = atoi(resp);

out_free:
	if(resp != NULL)
		free_safe(resp);
	return ret;
}

/* send a set command */
int
voipclient_cmd_set(const char *fmt_cmd, ...)
{
	va_list args;
	int ret = 0;

	/* build and send the command */
	va_start (args, fmt_cmd);
	ret = voipclient_cmd_common_v(NULL, 0, CMD_ACT_SET, fmt_cmd, args);
	va_end (args);
	if(ret != 0) {
		ret = -1;
		goto out;
	}

out:
	return ret;
}

//according to 984.4
#define PCMU	0
#define PCMA	8
#define G729	18
#define G723	4

int
voipclient_cmd_codecpriority(int action, int port, int priority, unsigned char *omci_val_p)
{
	int ret;

	if (action == VOIP_HW_ACTION_GET) {
		char codec_str[32];
		memset(codec_str, 0, 32);
		ret = voipclient_cmd_get_string(codec_str, 32, "codecpriority %d %d", port, priority);
		if(ret != 0)
			goto error_out;
		if(strcasestr(codec_str, "PCMU")!=NULL) {
			*omci_val_p = PCMU;
		}else if(strcasestr(codec_str, "PCMA")!=NULL) {
			*omci_val_p = PCMA;
		}else if(strcasestr(codec_str, "G729")!=NULL) {
			*omci_val_p = G729;
		}else if(strcasestr(codec_str, "G723")!=NULL) {
			*omci_val_p = G723;
		}
	}
	return 0;

error_out:
	return -1;
}

int
voipclient_cmd_get_digitmap(int port, char *buffer, int length, int *num, int dial_plan_type)
{
	int ret = 0;
	char *result = NULL;
	int result_len;
	int len_count;

	if (buffer == NULL || num == NULL)
		return -1;

	result_len = 4095;
	result = malloc_safe(result_len+1);
	if(result==NULL)
		return -1;
	result[result_len] = '\0';
	ret = voipclient_cmd_get_string(result, result_len, "digitmap %d", port);
	if(ret != 0)
		goto out_free;

	char *ptr1, *ptr2;
	ptr1 = result + strlen("No. of digitmaps: ");
	ptr2 = strchr(ptr1, '\n');
	if (ptr2 != NULL) {
		*ptr2++ = '\0';
		*num = atoi(ptr1);
		if (*num == 0)
			goto out_free;

		len_count = 0;
		if( dial_plan_type == 2) {
			*buffer='('; 	//add '('
			buffer++;
			len_count++;
		}

		while (*ptr2 != '\0' && len_count < length) {
			if (*ptr2 == '\n') {
				*buffer++ = '|';	//change '\n' to '|'
				ptr2++;
			} else
				*buffer++ = *ptr2++;
			len_count++;
		}

		if( dial_plan_type == 2 && len_count < length) {
			*buffer = ')';	//add ')'
			buffer++;
			len_count++;
		}
		*buffer = '\0';
	} else {
		*num = 0;
		goto out_free;
	}
	
out_free:
	if(result != NULL)
		free_safe(result);
	return ret;
}

char *
voipclient_cmd_dump_buf(char *buf, char *string_g, int length)
{
	char *c = buf;
	char *s = string_g;

	if(buf==0 || string_g==0)
		return 0;

	if(length<=1) {
		string_g[0] = '\0';
		return string_g;
	}

	while(*c!='\0') {
		if(isprint(*c)) {
			*s = *c;
		} else if(*c==0xa) {
			*s = '|';
		} else {
			*s = '?';
		}
		c++;
		s++;
		if(s-string_g >= length-1)
			break;
	}

	*s = '\0';
	return string_g;
}

int
voipclient_cmd_get_cidtype(int port, int *flag)
{
	char cmd[255];
	char buf[255];
	int ret = 0;
	memset(buf, 0, 255);
	if (flag == NULL) {
		return -1;
	}
	snprintf(cmd, sizeof(cmd), "get cidtype %d\n\n", port);
	ret = voipclient_comm_cli(cmd, buf, sizeof(buf));
	if (ret == 0) {
		*flag = atoi(buf);
	}
	return ret;
}

int
voipclient_cmd_get_cidblocking(int port, int *flag)
{
	char cmd[255];
	char buf[255];
	int ret = 0;
	memset(buf, 0, 255);
	if (flag == NULL) {
		return -1;
	}
	snprintf(cmd, sizeof(cmd), "get cidblockingoption %d\n\n", port);
	ret = voipclient_comm_cli(cmd, buf, sizeof(buf));
	if (ret == 0) {
		*flag = atoi(buf);
	}
	return ret;
}

int
voipclient_cmd_get_dial_plan_format(int port, int *value)
{
	char cmd[255];
	char buf[255];
	int ret = 0;
	memset(buf, 0, 255);
	snprintf(cmd, sizeof(cmd), "get digitmapsyntax %d\n\n", port);
	ret = voipclient_comm_cli(cmd, buf, sizeof(buf));
	if (ret == 0) {
		*value = atoi(buf);
	}
	return ret;
}

