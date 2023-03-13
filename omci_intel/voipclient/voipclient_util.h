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
 * File    : voipclient_util.h
 *
 ******************************************************************/

#ifndef voipclient_util_
#define voipclient_util_
#include "list.h"

struct CfgList {
	char name[30];
	char value[80];
	struct list_head list;
};

int voipclient_util_parse_digitmap_id(char *dialplan);
int voipclient_util_parse_digitmap_extension(char *dialplan);
int voipclient_util_parse_pos_id(char *pos);
int voipclient_util_parse_pos_type(char *pos);
struct CfgList *voipclient_util_analyze_nv_string(const char *str, char *innerDelim, char *outerDelim);
int voipclient_util_analyze_nv_string_release(struct CfgList *dataList);
int voipclient_util_set_dns(int index, char *dns);
int voipclient_util_get_dns(int index, char *dns);

#endif
