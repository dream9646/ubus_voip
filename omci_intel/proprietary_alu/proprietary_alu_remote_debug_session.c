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
 * Module  : proprietary_alu
 * File    : proprietary_alu_remote_debug_session.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <linux/if_ether.h>
#include "meinfo.h"
#include "util.h"
#include "util_run.h"
#include "notify_chain.h"
#include "gpon_sw.h"
#include "proprietary_alu.h"

#define REMOTE_SESSION_IFACE_PHY	"pon0"
#define REMOTE_SESSION_IFACE_VID	4090
#define REMOTE_SESSION_INTERFACE	"pon0.4090"
#define OLT_IP	"192.168.2.1"
#define ONT_IP	"192.168.2.2"
#define IP_NETMASK	"255.255.255.252"


#ifndef X86

#include "switch.h"
static int remote_debug_session_init = 0;

int
proprietary_alu_set_ip(char *iface, char *ip)
{
	char vendor_id[8];
	unsigned int sn = 0;
	struct ifreq ifr;
	struct sockaddr_in *sin;
	struct in_addr ina;
	int sockfd = 0;
	unsigned char mac[IFHWADDRLEN] = {0x02, 0x4c, 0x00, 0x00, 0x00, 0x00};
	
	if (gpon_hw_g.onu_serial_number_get) {
		int ret = 0;
		if ((ret = gpon_hw_g.onu_serial_number_get(vendor_id, &sn)) < 0) {
			dbprintf(LOG_ERR, "set mac error, ret=%d\n", ret);
			return -1;
		}
	} else {
		dbprintf(LOG_ERR, "cannot set mac\n");
		return -1;
	}
	
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		return -1;
	
	strcpy(ifr.ifr_name, iface);
	ifr.ifr_addr.sa_family = AF_INET;	
	if (ioctl(sockfd, SIOCGIFFLAGS, &ifr) < 0) {
		close(sockfd);
		return -1;
	}
	
	ifr.ifr_flags &= ~IFF_UP;
	if (ioctl(sockfd, SIOCSIFFLAGS, &ifr)< 0) {
		close(sockfd);
		return -1;
	}
	
	ifr.ifr_addr.sa_family = ARPHRD_ETHER;
	sn = htonl(sn);
	memcpy(mac+2, &sn, IFHWADDRLEN-2);
	memcpy(ifr.ifr_hwaddr.sa_data, mac, IFHWADDRLEN);
	if (ioctl(sockfd, SIOCSIFHWADDR, &ifr) < 0) {
		close(sockfd);
		return -1;
	}
	
	ifr.ifr_addr.sa_family = AF_INET;
	sin = (struct sockaddr_in *)&ifr.ifr_addr;
	ina.s_addr = inet_addr(ip);
	memcpy(&sin->sin_addr, (char *) &ina, sizeof(struct in_addr));
	if (ioctl(sockfd, SIOCSIFADDR, &ifr) <0) {
		close(sockfd);
		return -1;
	}
	
	inet_pton(AF_INET, IP_NETMASK, ifr.ifr_addr.sa_data + 2);
	if (ioctl(sockfd, SIOCSIFNETMASK, &ifr) <0) {
		close(sockfd);
		return -1;
	}
	
	ifr.ifr_flags |= IFF_UP;
	if (ioctl(sockfd, SIOCSIFFLAGS, &ifr) <0) {
		close(sockfd);
		return -1;
	}
	close(sockfd);
	
	if (switch_hw_g.mac_entry_add) {
		struct switch_mac_tab_entry_t mac_table_entry;
		int i;
		for (i=0; i<6; i++) mac_table_entry.mac_addr[i]=mac[i];
		mac_table_entry.port_bitmap = 0x40;
		mac_table_entry.is_ivl = omci_env_g.ivlan_enable;
		mac_table_entry.fid = 0;
		if (switch_hw_g.mac_entry_add(&mac_table_entry, 1) < 0)
			return -1;
	}	

	return 0;
}

int
proprietary_alu_set_static_arp(char *iface, char *ip, unsigned char *mac)
{
	struct arpreq arpreq;
	struct sockaddr_in *sin;
	struct in_addr ina;
	int flags, rc, sockfd = 0;
	
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		return -1;
	
	memset(&arpreq, 0, sizeof(struct arpreq));
	sin = (struct sockaddr_in *) &arpreq.arp_pa;
	memset(sin, 0, sizeof(struct sockaddr_in));
	arpreq.arp_pa.sa_family = AF_INET;
	ina.s_addr = inet_addr(ip);
	memcpy(&sin->sin_addr, (char *) &ina, sizeof(struct in_addr));
	memcpy(arpreq.arp_ha.sa_data, mac, IFHWADDRLEN);
	strcpy(arpreq.arp_dev, iface);
	flags = ATF_PERM | ATF_COM;
	arpreq.arp_flags = flags;
	
	rc = ioctl(sockfd, SIOCSARP, &arpreq);
	if (rc < 0)
	{
		close(sockfd);
		return -1;
	}
	close(sockfd);
	return 0;
}

int proprietary_alu_set_gem_port(int enable);

static int
o5_sequence_handler(unsigned char event, unsigned short classid, unsigned short instance_num, unsigned short meid, void *data_ptr)
{

	if (event == NOTIFY_CHAIN_EVENT_O5_SEQUENCE) {
		dbprintf(LOG_WARNING, "O5 sequence event, reconfigure alu ssh gemport\n");
		proprietary_alu_set_gem_port(1);
	}
	return 0;
}

int
proprietary_alu_set_gem_port(int enable)
{
	char cmd[128];
	struct gpon_tm_dsflow_config_t gpon_tm_dsflow_config;
	struct gpon_tm_usflow_config_t gpon_tm_usflow_config;
		
	if(!remote_debug_session_init && enable) {
		unsigned char mac[IFHWADDRLEN] = {0x06, 0x00, 0x00, 0x00, 0x00, 0x80};
		
		sprintf(cmd, "ifconfig %s up", REMOTE_SESSION_IFACE_PHY);
		util_run_by_system(cmd);
		sprintf(cmd, "vconfig add %s %d", REMOTE_SESSION_IFACE_PHY, REMOTE_SESSION_IFACE_VID);
		util_run_by_system(cmd);
		sprintf(cmd, "ifconfig %s up", REMOTE_SESSION_INTERFACE);
		util_run_by_system(cmd);
		proprietary_alu_set_ip(REMOTE_SESSION_INTERFACE, ONT_IP);
		proprietary_alu_set_static_arp(REMOTE_SESSION_INTERFACE, OLT_IP, mac);
		
		// register o5 sequence handler, so flow can get reconfigured from non-o5 to o5
		notify_chain_register(NOTIFY_CHAIN_EVENT_O5_SEQUENCE, o5_sequence_handler, NULL);
		
		remote_debug_session_init = 1;
	}
	
	// Unset downstream by ds-flow=126
	memset(&gpon_tm_dsflow_config, 0, sizeof(gpon_tm_dsflow_config));
	if( gpon_hw_g.tm_dsflow_get(GPON_SSHD_FLOW_ID, &gpon_tm_dsflow_config)==0) {
		gpon_tm_dsflow_config.enable = 0;
#if	!defined(CONFIG_SDK_PRX126) && !defined(CONFIG_SDK_PRX120) && !defined(CONFIG_SDK_PRX321)
		gpon_hw_g.tm_dsflow_set(GPON_SSHD_FLOW_ID, &gpon_tm_dsflow_config);
#endif
	}
	// Unset uptream by us-flow=126
	memset(&gpon_tm_usflow_config, 0, sizeof(gpon_tm_usflow_config));
	if( gpon_hw_g.tm_usflow_get(GPON_SSHD_FLOW_ID, &gpon_tm_usflow_config)==0) {
		gpon_tm_usflow_config.enable = 0;
#if	!defined(CONFIG_SDK_PRX126) && !defined(CONFIG_SDK_PRX120) && !defined(CONFIG_SDK_PRX321)
		gpon_hw_g.tm_usflow_set(GPON_SSHD_FLOW_ID, &gpon_tm_usflow_config);
#endif
	}

	if (enable) {
		unsigned short omcc_gem_port = 0, external_gem_port = 0;
		
		// find the gemport for alu ssh	channel
		if( gpon_hw_g.tm_dsflow_get(GPON_OMCI_FLOW_ID, &gpon_tm_dsflow_config) !=0) {
			dbprintf(LOG_ERR, "tm_dsflow_get error\n");
			return -1;
		}
		omcc_gem_port = gpon_tm_dsflow_config.gemport;
		if((proprietary_alu_get_olt_version() == 0) && (proprietary_alu_get_olt_release() < 0x04090000)) 
			external_gem_port = 4026 + omcc_gem_port; // FTTU-based
		else 
			external_gem_port = 2044 - omcc_gem_port; // ISAM-based
		
		// Set downstream by ds-flow=126
		memset(&gpon_tm_dsflow_config, 0, sizeof(gpon_tm_dsflow_config));
		gpon_tm_dsflow_config.gemport = external_gem_port;
		gpon_tm_dsflow_config.flow_type = GPON_TM_FLOW_TYPE_ETH;
		gpon_tm_dsflow_config.aes_enable = 0;
		gpon_tm_dsflow_config.mcast_enable = 0;		
		gpon_tm_dsflow_config.enable = 1;
#if	!defined(CONFIG_SDK_PRX126) && !defined(CONFIG_SDK_PRX120) && !defined(CONFIG_SDK_PRX321)
		if( gpon_hw_g.tm_dsflow_set(GPON_SSHD_FLOW_ID, &gpon_tm_dsflow_config) !=0) {
			dbprintf(LOG_ERR, "tm_dsflow_set error\n");
			return -1;
		}	
#endif
		// Set uptream by us-flow=126
		memset(&gpon_tm_usflow_config, 0, sizeof(gpon_tm_usflow_config));
		gpon_tm_usflow_config.gemport = external_gem_port;
		gpon_tm_usflow_config.flow_type = GPON_TM_FLOW_TYPE_ETH;
		gpon_tm_usflow_config.enable = 1;
#if	!defined(CONFIG_SDK_PRX126) && !defined(CONFIG_SDK_PRX120) && !defined(CONFIG_SDK_PRX321)
		if( gpon_hw_g.tm_usflow_set(GPON_SSHD_FLOW_ID, &gpon_tm_usflow_config) !=0) {
			dbprintf(LOG_ERR, "tm_usflow_set error\n");
			return -1;
		}
#endif		
		// Set us-flow=126 to OMCC queue
		gpon_hw_g.tm_usflow2pq_set(GPON_SSHD_FLOW_ID, GPON_OMCI_PQ_ID);
	}
	
	return 0;
}
#endif
