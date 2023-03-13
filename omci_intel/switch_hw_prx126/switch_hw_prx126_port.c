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
 * File    : switch_hw_prx126_port.c
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
#include <arpa/inet.h>

// @sdk/include


#include "util.h"
#include "util_run.h"
#include "list.h"
#include "switch_hw_prx126.h"
#include "omciutil_vlan.h"
#include "omciutil_misc.h"
#include "switch.h"
#include "env.h"

#include "intel_px126_port.h"
// ----------------------------------------------------------------------------
// Set port configuration by UNI port
// arguments: port on/off, auto/force, speed, duplex, nway, tx_pause, rx_pause
// The argument value won't be set if value is out of range
// ----------------------------------------------------------------------------
static unsigned int g_uni_enable = 0;
int
switch_hw_prx126_port_config_set(unsigned int port_id, struct switch_port_mac_config_t port_mac_config)
{
	return 0;
}

// ----------------------------------------------------------------------------
// Get port configuration by port
// ----------------------------------------------------------------------------
int
switch_hw_prx126_port_config_get(unsigned int port_id, struct switch_port_mac_config_t * port_mac_config)
{
	return 0;
}

// ----------------------------------------------------------------------------
// Print port configuration for all UNI ports
// ----------------------------------------------------------------------------
int
switch_hw_prx126_port_config_print(int fd)
{
	return 0;
}

// ----------------------------------------------------------------------------
// Get mac status by port
// ----------------------------------------------------------------------------
int
switch_hw_prx126_port_status_get(unsigned int port_id, struct switch_port_mac_config_t * port_mac_status)
{
	return 0;
}

// ----------------------------------------------------------------------------
// Print port status for all UNI ports
// ----------------------------------------------------------------------------
int
switch_hw_prx126_port_status_print(int fd)
{
	return 0;
}

// ----------------------------------------------------------------------------
// Set phy loopback by port
// ----------------------------------------------------------------------------
int
switch_hw_prx126_port_phylb_set(unsigned int port_id, unsigned int enable)
{
	return 0;
}

// ----------------------------------------------------------------------------
// Get phy loopback by port
// ----------------------------------------------------------------------------
int
switch_hw_prx126_port_phylb_get(unsigned int port_id, unsigned int *enable)
{
	return 0;
}

// ----------------------------------------------------------------------------
// Print phy loopback info for all ports
// ----------------------------------------------------------------------------
int
switch_hw_prx126_port_phylb_print(int fd)
{
	return 0;
}

// ----------------------------------------------------------------------------
// Set phy isolate by port
// ----------------------------------------------------------------------------
int
switch_hw_prx126_port_phyiso_set(unsigned int port_id, unsigned int enable)
{
	return 0;
}

// ----------------------------------------------------------------------------
// Get phy isolate by port
// ----------------------------------------------------------------------------
int
switch_hw_prx126_port_phyiso_get(unsigned int port_id, unsigned int *enable)
{
	return 0;
}

// ----------------------------------------------------------------------------
// Print phy isolate info for all ports
// ----------------------------------------------------------------------------
int
switch_hw_prx126_port_phyiso_print(int fd)
{
	return 0;
}


int
switch_hw_prx126_port_eth_mtu_set(unsigned int port_id, unsigned int mtu)
{
	return 0;
}

// ----------------------------------------------------------------------------
// Get mtu by port
// ----------------------------------------------------------------------------
int
switch_hw_prx126_port_eth_mtu_get(unsigned int port_id, unsigned int *mtu)
{
	return 0;
}

// ----------------------------------------------------------------------------
// Enable/Disable port
// ----------------------------------------------------------------------------
int
switch_hw_prx126_port_enable_set(unsigned int port_id, unsigned int enable)
{
	unsigned short meid; 
	int ret;
	if (port_id ==0)
		meid = 0x100+1;
	else{
		dbprintf(LOG_ERR, "unknow portnum=%d\n", port_id);
	return 0;
	}
	
	if(enable)
		ret =  intel_px126_port_unlock(meid);
	else
		ret =  intel_px126_port_lock(meid);

	g_uni_enable = enable;
	return ret;
}

// ----------------------------------------------------------------------------
// Get port enable option by phy control reg powerdown bit (reg 0.11)
// ----------------------------------------------------------------------------
int
switch_hw_prx126_port_enable_get(unsigned int port_id, unsigned int *enable)
{
	if (port_id ==0)
		*enable = g_uni_enable;
	else{
		dbprintf(LOG_ERR, "unknow portnum=%d\n",  port_id);
		return 0;
	}
	return 0;
}

// ----------------------------------------------------------------------------
// Print port enable setting
// ----------------------------------------------------------------------------
int
switch_hw_prx126_port_enable_print(int fd)
{
	return 0;
}

int
switch_hw_prx126_port_inf_set(unsigned short meid,int is_create)
{
	if(is_create)
		return intel_px126_port_inf_set(meid);
	else
		return intel_px126_port_inf_del(meid);
	
	return 0;
}

int 
switch_hw_prx126_port_inf_update(unsigned short meid,struct switch_pptp_eth_uni_data *updata)
{
	return intel_px126_port_inf_update(meid,updata);
}

int
switch_hw_prx126_port_inf_unlock(unsigned short meid)
{
	return intel_px126_port_unlock(meid);
}

// ----------------------------------------------------------------------------
// Enable/Disable testmode
// ----------------------------------------------------------------------------
int
switch_hw_prx126_port_testmode_set(unsigned int port_id, unsigned int enable)
{
	return 0;
}

int
switch_hw_prx126_port_testmode_get(unsigned int port_id, unsigned int *enable)
{
	return 0;
}

int
switch_hw_prx126_port_testmode_print(int fd)
{
	return 0;
}


// FIXME! STEVE - port_autoneg_[get|set] calls hw functions by mode type
// auto  : port_phyAutoNegoAbility_[get|set]   
// force : port_phyForceModeAbility_[get|set]
int
switch_hw_prx126_port_autoneg_set(unsigned int port_id, unsigned int autoneg_mode)
{
	return 0;
}

int
switch_hw_prx126_port_autoneg_get(unsigned int port_id, unsigned int *autoneg_mode)
{
	return 0;
}

int
switch_hw_prx126_port_autoneg_print(int fd)
{
	return 0;
}



int
switch_hw_prx126_port_phy_reg_interpret_print(int fd, unsigned char reg_id, unsigned short value)
{
	return 0;
}

int
switch_hw_prx126_port_phy_reg_print(int fd, unsigned int port_id)
{
	return 0;
}

