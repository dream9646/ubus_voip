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
 * File    : voipclient_util.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"
#include "voipclient_util.h"

typedef enum DIALPLAN_ACTION_TUPE { 
	ACTION_SUBSTITUTE = 0x1, 
	ACTION_DIAL_TO_PRECONNECTION = 0x2, 
	ACTION_DIAL_REJECT = 0x4,
} dialplan_action_type_t;

int
voipclient_util_parse_digitmap_id(char *dialplan)
{
	char buf[255], *extension, *tmpp;
	int dial_map_id = 0;
	if ((dialplan == NULL) || (*dialplan == '\0')) {
		return -1;
	}
	strncpy(buf, dialplan, sizeof(buf) - 1);
	buf[strlen(dialplan)] = '\0';
	extension = buf;
	tmpp = NULL;
	while (1) {
		if ((tmpp = strchr(extension, '~')) != NULL) {
			*tmpp = '\0';
			tmpp++;
			dial_map_id = atoi(tmpp);
		}
		return dial_map_id;
	}
	return dial_map_id;
}

int
voipclient_util_parse_digitmap_extension(char *dialplan)
{
	char buf[255], *extension, *next;
	int action = 0;
	if ((dialplan == NULL) || (*dialplan == '\0')) {
		return -1;
	}
	strncpy(buf, dialplan, sizeof(buf) - 1);
	buf[strlen(dialplan)] = '\0';
	extension = buf;
	next = NULL;

	/* extension format: "@p0&sub{23:345}+dial{EP1}" */
	while (1) {
		if ((next = strchr(extension, '&')) != NULL) {
			*next = '\0';
			next += 1;
		} else if ((next = strchr(extension, '+')) != NULL) {
			*next = '\0';
			next += 1;
		}

		/* specific port number */
		if (memcmp(extension, "p", 1) == 0) {
			extension += 1;
		}

		/* substitute */
		else if (memcmp(extension, "sub", 3) == 0) {
			action |= ACTION_SUBSTITUTE;
		}

		/* dial to specific destination */
		else if (memcmp(extension, "dial", 4) == 0) {
			action |= ACTION_DIAL_TO_PRECONNECTION;
		}

		/* reject dial */
		else if (memcmp(extension, "rej", 3) == 0) {
			action |= ACTION_DIAL_REJECT;
		}

		/* switch to specific port */
		else if (memcmp(extension, "switch", 6) == 0) {
		}
		if (next) {
			extension = next;
		} else {
			return action;
		}
	}
	return action;
}

int
voipclient_util_parse_pos_id(char *pos)
{
	char buf[255], *extension, *tmpp;
	int pos_map_id = 0;
	if ((pos == NULL) || (*pos == '\0')) {
		return -1;
	}
	strncpy(buf, pos, sizeof(buf) - 1);
	buf[strlen(pos)] = '\0';
	extension = buf;
	tmpp = NULL;
	while (1) {
		if ((tmpp = strchr(extension, '~')) != NULL) {
			*tmpp = '\0';
			tmpp++;
			if(!strncmp(tmpp, "(null)", 6))
				pos_map_id = -1;
			else
				pos_map_id = atoi(tmpp);
		}
		return pos_map_id;
	}
	return pos_map_id;
}

int
voipclient_util_parse_pos_type(char *pos)
{
	char buf[255], *extension, *tmpp;
	int pos_map_id = 0;
	if ((pos == NULL) || (*pos == '\0')) {
		return -1;
	}
	strncpy(buf, pos, sizeof(buf) - 1);
	buf[strlen(pos)] = '\0';
	extension = buf;
	tmpp = NULL;
	while (1) {
		if ((tmpp = strchr(extension, ':')) != NULL) {
			*tmpp = '\0';
			tmpp++;
			pos_map_id = atoi(tmpp);
		}
		return pos_map_id;
	}
	return pos_map_id;
}

/*
for example:
str:  "a=1&b=2&c=3"
innerDelim: "="
outerDelim: "&"
*/
struct CfgList *
voipclient_util_analyze_nv_string(const char *str, char *innerDelim, char *outerDelim)
{
	char *tempStr, *p;
	struct CfgList *dataList, *temp;
	char *outer_ptr = NULL;
	char *inner_ptr = NULL;
	char *buf;
	int in = 0;

	/* check data */
	if (str == NULL || strlen(str) == 0) {
		return NULL;
	}

	/* init data */
	tempStr = (char *)malloc_safe(strlen(str) + 1);
	if (tempStr == NULL) {
		return NULL;
	}
	buf = tempStr;
	if (str[0] == '\"') {
		strcpy(tempStr, str + 1);
	} else {
		strcpy(tempStr, str);
	}
	if (tempStr[strlen(tempStr) - 1] == '\"') {
		tempStr[strlen(tempStr) - 1] = 0;
	}
	dataList = (struct CfgList *)malloc_safe(sizeof(struct CfgList));
	if (dataList == NULL) {
		free_safe(tempStr);
		return NULL;
	}
	INIT_LIST_HEAD(&dataList->list);

	/* analyze data */
	while ((p = strtok_r(buf, outerDelim, &outer_ptr)) != NULL) {
		buf = p;
		temp = (struct CfgList *)malloc_safe(sizeof(struct CfgList));
		while ((p = strtok_r(buf, innerDelim, &inner_ptr)) != NULL) {
			if (in == 0) {
				strncpy(temp->name, p, sizeof(temp->name));
			} else {
				strncpy(temp->value, p, sizeof(temp->value));
			}
			in++;
			buf = NULL;
		}
		if (in == 0) {
			strcpy(temp->value, "");
		}
		temp->list.next = NULL;
		list_add(&(temp->list), &(dataList->list));
		buf = NULL;
		in = 0;
	}
	free_safe(tempStr);
	return dataList;
}

int
voipclient_util_analyze_nv_string_release(struct CfgList *dataList)
{
	struct CfgList *tmp;
	struct list_head *pos, *q;
	if (dataList == NULL) {
		return 0;
	}
	list_for_each_safe(pos, q, &dataList->list) {
		tmp = list_entry(pos, struct CfgList, list);
		list_del(pos);
		free_safe(tmp);
	} free_safe(dataList);
	return 0;
}

void
voipclient_util_analyze_nv_string_show(struct CfgList *dataList)
{
	struct CfgList *tmp = NULL;
	list_for_each_entry(tmp, &dataList->list, list) {
		printf("freeing item name= %s value= %s\n", tmp->name, tmp->value);
}}

/*input dns is 2.5.168.192*/
static int
convert_dns(char *value, char *output)
{
	char cmd[5][64];
	memset(cmd, 0, sizeof(cmd));
	int i = 0;
	char *find;
	if ((value == NULL) || (output == NULL)) {
		return -1;
	}

	/*find port displayname */
	while ((find = strstr(value, ".")) != NULL) {
		strncpy(cmd[i], value, strlen(value) - strlen(find));
		strcpy(value, value + strlen(value) - strlen(find) + 1);
		if (i == 2) {
			strcpy(cmd[3], value);
		}
		i++;
	}
	strcat(cmd[4], cmd[3]);
	strcat(cmd[4], ".");
	strcat(cmd[4], cmd[2]);
	strcat(cmd[4], ".");
	strcat(cmd[4], cmd[1]);
	strcat(cmd[4], ".");
	strcat(cmd[4], cmd[0]);
	strcpy(output, cmd[4]);
	return 0;
}

int
voipclient_util_set_dns(int index, char *dns)
{
	FILE *fp, *fpBack;
	char buf[256];
	char dnsIp[64];
	char dnsConv[64];
	int ret = -1;
	int flag = 0;
	if (dns == NULL) {
		return ret;
	}
	memset(dnsIp, 0, sizeof(dnsIp));
	memset(dnsConv, 0, sizeof(dnsConv));
	fp = fopen("/etc/resolv.conf", "r");
	if (fp == NULL) {
		return -1;
	}
	fpBack = fopen("/etc/resolvBak.conf", "w");
	if (fpBack == NULL) {
		fclose(fp);
		return -1;
	}
	if (-1 == convert_dns(dns, dnsConv)) {
		return -1;
	}
	while (fgets(buf, sizeof(buf), fp) != NULL) {
		if (buf[0] == '#') {
			fprintf(fpBack, "%s", buf);
		} else {
			flag++;
			if (index == flag) {
				if (strstr(buf, dnsConv) == NULL) {
					fprintf(fpBack, "nameserver %s\n", dnsConv);
				}
			}
			fprintf(fpBack, "%s", buf);
		}
	}
	fclose(fp);
	fclose(fpBack);
	unlink("/etc/resolv.conf");
	rename("/etc/resolvBak.conf", "/etc/resolv.conf");
	return 0;
}

int
voipclient_util_get_dns(int index, char *dns)
{
	FILE *fp;
	char buf[256];
	char dnsIp[64];
	int flag = 0;
	fp = fopen("/etc/resolv.conf", "r");
	if (fp == NULL) {
		return -1;
	}
	while (fgets(buf, sizeof(buf), fp) != NULL) {
		if (buf[0] == '#') {
			continue;
		}
		if (sscanf(buf, "%*s %s", dnsIp) == 1) {
			flag++;
			if (flag == index) {
				if (-1 == convert_dns(dnsIp, dns)) {
					fclose(fp);
					return -1;
				}
				//strcpy(dns,dnsIp);
				break;
			}
		}
	}
	fclose(fp);
	return 0;
}

