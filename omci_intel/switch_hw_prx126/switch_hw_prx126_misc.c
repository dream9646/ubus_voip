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
 * Module  : switch_hw_prx126
 * File    : switch_hw_prx126_misc.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>

// @sdk/include


#include "util.h"
#include "regexp.h"
#include "list.h"
#include "switch_hw_prx126.h"
#include "omciutil_vlan.h"
#include "util_run.h"
#include "switch.h"
#include "vacl.h"
#include "env.h"
#include "mcast.h"

#include "intel_px126_util.h"

// preassign filter ///////////////////////////////////////////////////////////////////////////


int
switch_hw_prx126_preassign_filter_get(unsigned int portid, unsigned int protocolid, unsigned int *enable)
{
	return 0;
}

int
switch_hw_prx126_preassign_filter_set(unsigned int portid, unsigned int protocolid, unsigned int enable)
{
	return 0;
}

// cfm extract enable/disable ctrl ////////////////////////////////////////////////////////////

int
switch_hw_prx126_cfm_extract_vacl_rule(struct vacl_user_node_t *acl_rule)
{
	return 0;
}

int
switch_hw_prx126_cfm_extract_enable_set(unsigned int enable)
{
	return 0;
}

int
switch_hw_prx126_cfm_extract_enable_get(unsigned int *enable)
{
	return 0;
}

// extoam extract enable/disable ctrl ////////////////////////////////////////////////////////////


int
switch_hw_prx126_extoam_extract_enable_set(unsigned int enable)
{
	return 0;
}

int
switch_hw_prx126_extoam_extract_enable_get(unsigned int *enable)
{
	return 0;
}

int
switch_hw_prx126_igmp_extract_enable_set(unsigned int port_id, unsigned int enable)
{
	return 0;
}

// igmp upstream rate ctrl ////////////////////////////////////////////////////////////
int
switch_hw_prx126_igmp_upstream_rate_set(unsigned int port_id, int rate)
{
	return 0;	     
}

int
switch_hw_prx126_igmp_upstream_rate_get(unsigned int port_id, int *rate)
{
	return 0;	
}
			
int
switch_hw_prx126_igmp_extract_enable_get(unsigned int port_id, unsigned int *enable)
{
	return 0;
}

// dhcp extract enable/disable ctrl ////////////////////////////////////////////////////////////
int
switch_hw_prx126_dhcp_extract_enable_set(unsigned int enable)
{
	return 0;
}

int
switch_hw_prx126_dhcp_extract_enable_get(unsigned int *enable)
{
	return 0;
}

// pppoe extract enable/disable ctrl ////////////////////////////////////////////////////////////
int
switch_hw_prx126_pppoe_extract_enable_set(unsigned int enable)
{
	return 0;
}

int
switch_hw_prx126_pppoe_extract_enable_get(unsigned int *enable)
{
	return 0;
}




int
switch_hw_prx126_eth_pppoe_filter_enable_set(unsigned int portid, unsigned int enable)
{
	return 0;
}

int
switch_hw_prx126_eth_pppoe_filter_enable_get(unsigned int port_id, unsigned int *enable)
{
	return 0;
}


// print related acl entries for pppoe filter 
int
switch_hw_prx126_eth_pppoe_filter_enable_print(int fd)
{
	return 0;
}

// print hw acl/classf table usage


int
switch_hw_prx126_hw_usage_print(int fd)
{
	return 0;	
}

int
switch_hw_prx126_eth_address_set(unsigned int port_id, unsigned char *ethaddr)
{
	return 0;
}

int
switch_hw_prx126_eth_address_get(unsigned int port_id, unsigned char *ethaddr)
{
	return 0;
}

int
switch_hw_prx126_eth_address_print(int fd)
{
	return 0;
}

int
switch_hw_prx126_dot1x_mode_set(unsigned int port_id, unsigned int enable)
{
	return 0;
}

int
switch_hw_prx126_dot1x_mode_get(unsigned int port_id, unsigned int *enable)
{
	return 0;
}

int
switch_hw_prx126_dot1x_state_set(unsigned int port_id, unsigned int state)
{
	return 0;
}

int
switch_hw_prx126_dot1x_state_get(unsigned int port_id, unsigned int *state)
{
	return 0;
}

int
switch_hw_prx126_dot1x_dir_set(unsigned int port_id, unsigned int dir)
{
	return 0;
}

int
switch_hw_prx126_dot1x_dir_get(unsigned int port_id, unsigned int *dir)
{
	return 0;
}

int
switch_hw_prx126_rma_action_set(unsigned char address, unsigned int action)
{
	return 0;
}

int
switch_hw_prx126_rma_action_get(unsigned char address, unsigned int *action)
{
	return 0;
}

int switch_hw_prx126_pptp_lct_uni_create(unsigned short meid)
{
	return intel_omci_pptp_lct_uni_create(meid);
}

int switch_hw_prx126_lct_uni_update(unsigned short meid, unsigned char state_admin)
{
	return intel_omci_pptp_lct_uni_update(meid, state_admin);
}

int switch_hw_prx126_pptp_lct_uni_destory(unsigned short meid)
{
	return intel_omci_pptp_lct_uni_destory(meid);
}


