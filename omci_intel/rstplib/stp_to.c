/************************************************************************ 
 * RSTP library - Rapid Spanning Tree (802.1t, 802.1w) 
 * Copyright (C) 2001-2003 Optical Access 
 * Author: Alex Rozin 
 * 
 * This file is part of RSTP library. 
 * 
 * RSTP library is free software; you can redistribute it and/or modify it 
 * under the terms of the GNU Lesser General Public License as published by the 
 * Free Software Foundation; version 2.1 
 * 
 * RSTP library is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser 
 * General Public License for more details. 
 * 
 * You should have received a copy of the GNU Lesser General Public License 
 * along with RSTP library; see the file COPYING.  If not, write to the Free 
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 
 * 02111-1307, USA. 
 **********************************************************************/

/* This file contains system dependent API
   from the RStp to a operation system (see stp_to.h) */

/* stp_to API for Linux */

//#include <stddef.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>


#include "base.h"
#include "stpm.h"
#include "stp_in.h"
#include "stp_to.h"
#include "uid.h"

#include "list.h"
#include "util.h"
#include "env.h"
#include "cpuport.h"
#include "switch.h"
#include "bridge.h"
#include "rstp_omci.h"
//extern BITMAP_T        enabled_ports;

/*************
void
stp_trace (const char *format, ...)
{
  #define MAX_MSG_LEN  128
  char     msg[MAX_MSG_LEN];
  va_list  args;

  va_start(args, format);
  vsnprintf (msg, MAX_MSG_LEN-1, format, args);
  printf ("%s\n", msg);
  va_end(args);
  
}
***********/

#ifdef STRONGLY_SPEC_802_1W
int
STP_OUT_set_learning (int port_index, int vlan_id, int enable)
{
	struct cpuport_desc_t desc;
	desc = stp_port[port_index-1];

	if ( False == enable )
		switch_hw_g.stp_state_port_set(desc.logical_port_id, STP_LISTENING);
	else if ( True == enable )
		switch_hw_g.stp_state_port_set(desc.logical_port_id,STP_LEARNING);

	return STP_OK;
}

int
STP_OUT_set_forwarding (int port_index, int vlan_id, int enable)
{
	struct cpuport_desc_t desc;
	desc = stp_port[port_index-1];

	if ( False == enable )
		switch_hw_g.stp_state_port_set(desc.logical_port_id, STP_LISTENING);

	else if ( True == enable )
		switch_hw_g.stp_state_port_set(desc.logical_port_id, STP_LEARNING);

	return STP_OK;
}
#else
/* 
 * In many kinds of hardware the state of ports may
 * be changed with another method
 */
int
STP_OUT_set_port_state (IN int port_index, IN int vlan_id,
            IN RSTP_PORT_STATE state)
{
	struct cpuport_desc_t desc;
	desc = stp_port[port_index-1];

	switch ( state )
	{
		case UID_PORT_NON_STP:

			switch_hw_g.stp_state_port_set(desc.logical_port_id, STP_OFF);
			break;

		case UID_PORT_DISABLED:

			switch_hw_g.stp_state_port_set(desc.logical_port_id, STP_DISABLE);

			break;

		case UID_PORT_DISCARDING:

			switch_hw_g.stp_state_port_set(desc.logical_port_id, STP_LISTENING);

			break;

		case UID_PORT_LEARNING:

			switch_hw_g.stp_state_port_set(desc.logical_port_id, STP_LEARNING);

			break;

		case UID_PORT_FORWARDING:

			switch_hw_g.stp_state_port_set(desc.logical_port_id, STP_FORWARDING);

			break;

		default:
			return -1;
	}


	return STP_OK;
  //return AR_INT_STP_set_port_state (port_index, vlan_id, state);
}
#endif


void
STP_OUT_get_port_mac (int port_index, unsigned char *mac)
{
	int fd;
	struct ifreq ifr;
	int i = 0;

	fd = socket(AF_INET, SOCK_DGRAM, 0);

	ifr.ifr_addr.sa_family = AF_INET;
	strncpy(ifr.ifr_name, "ifcpuuni0", IFNAMSIZ-1);
	ioctl(fd, SIOCGIFHWADDR, &ifr);
	close(fd);

	for ( i = 0 ; i < 6 ; i++ )
	{
		*(mac+i) = ifr.ifr_hwaddr.sa_data[i];
	}

}

int             /* 1- Up, 0- Down */
STP_OUT_get_port_link_status (int port_index)
{
	struct switch_port_mac_config_t port_mac_config;
	struct cpuport_desc_t desc;
	desc = stp_port[port_index-1];

	if ( switch_get_wan_logical_portid() == desc.logical_port_id  )
		return 1;
	else {
		if (switch_hw_g.port_status_get (desc.logical_port_id, &port_mac_config) < 0 )
			return -1;

		if (port_mac_config.link) {
			return 1;
		}

		return 0;
	}
	return -1;
}

int
STP_OUT_flush_lt (IN int port_index, IN int vlan_id, LT_FLASH_TYPE_T type, char* reason)
{
//	switch_hw_g.mac_table_flush();
	return STP_OK;
}

int
STP_OUT_set_hardware_mode (int vlan_id, UID_STP_MODE_T mode)
{
	int i = 2;
	//when mode == STP_ENABLED, 5vt2310 and rtl8367 don't need to do anything to init hardware state.
	//when mode == STP_DISABLED, just try  to disable the BPDU extraction  to mglc
	if ( mode == STP_DISABLED)
	{
		for(i = 0; i < switch_get_wan_logical_portid(); i++)
			switch_hw_g.stp_state_port_set(i , STP_OFF);

	}
	return STP_OK;
  //return AR_INT_STP_set_mode (vlan_id, mode);
}


int
STP_OUT_tx_bpdu (int port_index, int vlan_id,
         unsigned char *bpdu, size_t bpdu_len)
{
extern int bridge_tx_bpdu (int port_index, unsigned char *bpdu, size_t bpdu_len);
  return bridge_tx_bpdu (port_index, bpdu, bpdu_len);
}

const char *
STP_OUT_get_port_name (IN int port_index)
{
  static char tmp[4];
  sprintf (tmp, "p%02d", (int) port_index);
  return tmp;
  //return port2str (port_index, &sys_config);
}

unsigned long
STP_OUT_get_deafult_port_path_cost (IN unsigned int portNo)
{
  return 20000;
}

unsigned long STP_OUT_get_port_oper_speed (unsigned int portNo)
{
	struct cpuport_desc_t desc;
	struct switch_port_mac_config_t port_mac_status;

	desc = stp_port[portNo-1];

//dbprintf(LOG_ERR, "portNo %d sw0_id %d sw0_port %d sw1_id %d sw1_port %d\n",portNo,desc.sw0_id,desc.logical_port_id,desc.sw1_id,desc.sw1_port);

	switch_hw_g.port_status_get(desc.logical_port_id, &port_mac_status);


//dbprintf(LOG_ERR, "port_mac_status.speed %d\n",port_mac_status.speed);
	//unit 1K bit/s
	if ( port_mac_status.speed == 0 )
		return 10000000L;
	if ( port_mac_status.speed == 1 )
		return 100000000L;
	if ( port_mac_status.speed == 2 )
		return 1000000000L;
	return 0;
}

int             /* 1- Full, 0- Half */
STP_OUT_get_duplex (IN int port_index)
{
	struct cpuport_desc_t desc;
	struct switch_port_mac_config_t port_mac_status;

	desc = stp_port[port_index-1];
	switch_hw_g.port_status_get(desc.logical_port_id, &port_mac_status);

	if (port_mac_status.duplex == 1)
		return 1;
	return 0;
}

int
STP_OUT_get_init_stpm_cfg (IN int vlan_id,
                           INOUT UID_STP_CFG_T* cfg)
{
	unsigned short bridge_priority; 
	unsigned short max_age; 
	unsigned short hello_time; 
	unsigned short forward_delay; 

	if(rstp_omci_stp_bridge_priority_get(&bridge_priority) < 0 )
		cfg->bridge_priority =	DEF_BR_PRIO;
	else
		cfg->bridge_priority =	bridge_priority;

	if ( rstp_omci_stp_bridge_max_age_get( &max_age) < 0 )
		cfg->max_age =		DEF_BR_MAXAGE;
	else
		cfg->max_age =		max_age;

	if ( rstp_omci_stp_bridge_hello_time_get( &hello_time) <0 )
		cfg->hello_time =	DEF_BR_HELLOT;
	else
		cfg->hello_time =	hello_time;

	if ( rstp_omci_stp_bridge_forward_delay_get( &forward_delay) <0 )
		cfg->forward_delay =	DEF_BR_FWDELAY;
	else
		cfg->forward_delay =	forward_delay;

	cfg->force_version =          NORMAL_RSTP;

  return STP_OK;
}
  

int
STP_OUT_get_init_port_cfg (IN int vlan_id,
                           IN int port_index,
                           INOUT UID_STP_PORT_CFG_T* cfg)
{
	unsigned short port_priority, path_cost;

	if ( rstp_omci_stp_port_priority_get( stp_port[port_index-1], &port_priority) < 0 ){ 
		cfg->port_priority = DEF_PORT_PRIO;
	}else{ 
		cfg->port_priority = port_priority;
	}

	if (rstp_omci_stp_port_path_cost_get( stp_port[port_index-1], &path_cost) < 0 ){
		cfg->admin_port_path_cost = ADMIN_PORT_PATH_COST_AUTO;
	}else{
		cfg->admin_port_path_cost = path_cost;
	}

	cfg->admin_non_stp = DEF_ADMIN_NON_STP;
	cfg->admin_edge = DEF_ADMIN_EDGE;
	cfg->admin_point2point = DEF_P2P;

	return STP_OK;
}
