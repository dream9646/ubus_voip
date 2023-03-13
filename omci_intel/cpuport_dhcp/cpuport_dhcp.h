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
 * Module  : cpuport_dhcp
 * File    : cpuport_dhcp.h
 *
 ******************************************************************/

#ifndef __CPUPORT_DHCP_H__
#define __CPUPORT_DHCP_H__

enum dhcp_cmd_t {
	DHCP_CMD_LIMIT_CHANGE,
	DHCP_CMD_TERMINATE
};

struct dhcp_cmd_msg_t {
	enum dhcp_cmd_t cmd;
	unsigned int dhcp_limit;
	struct list_head q_entry;
};

extern unsigned short cvlan_for_65330;

/* cpuport_dhcp.c */
int cpuport_dhcp_count_reset(int fd);
int cpuport_dhcp_count_get(int fd);
int cpuport_dhcp_generate_data_from_fiberhome_me(unsigned char **option_str, struct cpuport_info_t *pktinfo, int *len);
int cpuport_dhcp_generate_data_from_ericsson_me(unsigned char **option_str, struct cpuport_info_t *pktinfo, int *len);
int cpuport_dhcp_init(void);
int cpuport_dhcp_shutdown(void);
int cpuport_dhcp_start(void);
int cpuport_dhcp_stop(void);

#endif
