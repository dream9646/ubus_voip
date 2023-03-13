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
 * Module  : proprietary_calix
 * File    : meinfo_hw_65317.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include <arpa/inet.h>
#include <linux/if.h>
#include "meinfo.h"
#include "meinfo_hw.h"
#include "me_related.h"
#include "util.h"
#include "switch.h"
#include "regexp.h"
#include "conv.h"
#include "iphost.h"
#include "metacfg_adapter.h"
#include "wanif.h"
#include "omciutil_misc.h"
#include "batchtab.h"
#include "batchtab_cb.h"

//  me65317 attr12:
//
//    31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10 09 08 07 06 05 04 03 02 01 00 
//   +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+ 
// 0 |                       VLAN                    | Format(0) |WAN Type|T | WAN Proto | WAN Status|
//   +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
// 1 |                                       Current IP Address                                      |
//   +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
// 2 |                                       Current IP Mask                                         |
//   +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
// 3 |                                       Current IP Gateway                                      |
//   +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
// 4 |                                            MAC Address                                        |
//   +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
// 5 |               MAC Address Continued           |                      RFU                      |
//   +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
// 6 |          RFU          |
//   +--+--+--+--+--+--+--+--+
// 
//    31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10 09 08 07 06 05 04 03 02 01 00 
//   +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+ 
// 0 |                       VLAN                    | Format(1) |   RFU     | WAN Proto | WAN Status|
//   +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
// 1 |                                       Current IPv6 Address                                    |
//   +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
// 2 |                                       Current IPv6 Address (continued)                        |
//   +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
// 3 |                                       Current IPv6 Address (continued)                        |
//   +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
// 4 |                                       Current IPv6 Address (continued)                        |
//   +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
// 5 |    Current IPv6 Address Prefix Length         |                      RFU                      |
//   +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
// 6 |          RFU          |
//   +--+--+--+--+--+--+--+--+
// 
//    31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10 09 08 07 06 05 04 03 02 01 00 
//   +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+ 
// 0 |                       VLAN                    | Format(2) |               RFU                 |
//   +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
// 1 |                                       Current IPv6 Site Prefix                                |
//   +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
// 2 |                                       Current IPv6 Site Prefix (continued)                    |
//   +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
// 3 |                                       Current IPv6 Site Prefix (continued)                    |
//   +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
// 4 |                                       Current IPv6 Site Prefix (continued)                    |
//   +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
// 5 |    Current IPv6 Site Prefix Length            |                      RFU                      |
//   +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
// 6 |          RFU          |
//   +--+--+--+--+--+--+--+--+
// 
//    31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10 09 08 07 06 05 04 03 02 01 00 
//   +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+ 
// 0 |                       VLAN                    | Format(3) |              RFU                  |
//   +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
// 1 |                                       Current IPv6 Gateway                                    |
//   +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
// 2 |                                       Current IPv6 Gateway (continued)                        |
//   +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
// 3 |                                       Current IPv6 Gateway (continued)                        |
//   +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
// 4 |                                       Current IPv6 Gateway (continued)                        |
//   +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
// 5 |                                              RFU                                              |
//   +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
// 6 |          RFU          |
//   +--+--+--+--+--+--+--+--+
// 
// Legend:
// 
// VLAN - identifies the RG WAN interface VLAN
// Format - specifies the format of the entry: 
// WAN Type - IPv4Only (0), IPv6Only (1), DualStack (2), DsLite (3), 6rd (4)
// T - Is TR-69 enabled on this RG WAN interface (0 or 1)?
// WAN Proto - DHCP (0), Static IP (1), PPPoE (2)
// WAN Status - UNCONFIGURED (0), DISCONNECTED (1), CONNECTING (2), CONNECTED (3)
// 
// Format 0 fields (always present):
// Current IP Address - applies to all WAN proto
// Current IP Mask - applies to DHCP and Static IP WAN proto
// Current Gateway - applies to all WAN proto. For PPPoE this is the PPPoE server IP
// MAC Address - MAC address of RG WAN interface
// 
// Format 1 fields:
// WAN Proto (IPv6) - DHCP (0), Static IP (1), PPPoE (2)
// WAN Status (IPv6) - UNCONFIGURED (0), DISCONNECTED (1), CONNECTING (2), CONNECTED (3)
// Current IPv6 Address - 
// 
// Format 2 fields:
// Current IPv6 Site Prefix - 
// 
// Format 3 fields:
// Current IPv6 Gateway - 

#ifdef OMCI_METAFILE_ENABLE
static void
prepare_table_enry_for_claix_rg_config_format0(char *tab_entry,
	unsigned short rg_tag, unsigned short ani_tag,
	char *mac_address,
	int assignment, int ip_addr, int netmask_addr, int gw_addr, 
	int is_up, int is_tr069, int ipv4_enable, int ipv6_enable)
{
	unsigned char wan_type = 0;	// ipv4 only
	unsigned char wan_proto = 0;	// dhcp
	unsigned char wan_status = 1;	// disconnected
	
	memset(tab_entry, 0, 25);
	// wan_type
	if (ipv4_enable) {
		if (ipv6_enable)
			wan_type = 2;	// ipv4 & ipv6 enabled
		else
			wan_type = 0;	// ipv4 only
	} else {
		if (ipv6_enable)
			wan_type = 1;	// ipv6 only
		else
			wan_type = 0;	// both disable, treat as ipv4 only
	}
	// wan_proto
	switch (assignment) {
		case 0:	wan_proto = 1;	break; // static ip
		case 1:	wan_proto = 0;	break; // dhcp
		case 2:	wan_proto = 2;	break; // pppoe
		case 3:	wan_proto = 3;	break; // bridge
	}
	// wan status
	if (is_up) {
		if (wan_proto == 3) {	// bridge mode
			wan_status = 3;	// connected			
		} else {
			unsigned char *ip = (unsigned char *)(&ip_addr);
			if (ip_addr == 0 || (ip[0] == 169 && ip[1] == 254)) {
				wan_status = 2;	// connecting
			} else {
				wan_status = 3;	// connected
			}
		}
	}

	{ // add table entry for ipv4
		int i;
#if 1
		util_bitmap_set_value(tab_entry, 25*8, 0, 16, ani_tag & 0xfff); // Vlan
		util_bitmap_set_value(tab_entry, 25*8, 16, 4, 0);		// Format 0
		util_bitmap_set_value(tab_entry, 25*8, 20, 3, wan_type);	// WAN Type
		util_bitmap_set_value(tab_entry, 25*8, 23, 1, is_tr069?1:0);	// TR069 Enabled
		util_bitmap_set_value(tab_entry, 25*8, 24, 4, wan_proto);
		util_bitmap_set_value(tab_entry, 25*8, 28, 4, wan_status);
		util_bitmap_set_value(tab_entry, 25*8, 32, 32, ip_addr);	// Current IP Address
		util_bitmap_set_value(tab_entry, 25*8, 64, 32, netmask_addr);	// Current IP Mask
		util_bitmap_set_value(tab_entry, 25*8, 96, 32, gw_addr);	// Current Gateway
#else
		util_bitmap_set_value(tab_entry, 25*8, 0, 16, 0x4081);	
		util_bitmap_set_value(tab_entry, 25*8, 16, 4, 1);
		util_bitmap_set_value(tab_entry, 25*8, 20, 3, 1);
		util_bitmap_set_value(tab_entry, 25*8, 23, 1, 1);
		util_bitmap_set_value(tab_entry, 25*8, 24, 4, 2);
		util_bitmap_set_value(tab_entry, 25*8, 28, 4, 2);
		util_bitmap_set_value(tab_entry, 25*8, 32, 32, 0x0a141414);
		util_bitmap_set_value(tab_entry, 25*8, 64, 32, 0xffff0000);
		util_bitmap_set_value(tab_entry, 25*8, 96, 32, 0x0a140001);
#endif
		for ( i = 0 ; i < 6 ; i++)					// MAC Address
			util_bitmap_set_value(tab_entry, 25*8, 128+i*8, 8, mac_address[i]);
	}
}

static void
prepare_table_enry_for_claix_rg_config_format1(char *tab_entry,
	unsigned short rg_tag, unsigned short ani_tag, int assignment, struct in6_addr *ip6_addr, int masklen, int is_up)
{
	unsigned char wan_proto = 0;	// dhcp
	unsigned char wan_status = 1;	// disconnected

	memset(tab_entry, 0, 25);
	// wan_proto
	switch (assignment) {
		case 0:	wan_proto = 1;	break; // static ip
		case 1:	wan_proto = 0;	break; // dhcp
		case 2:	wan_proto = 2;	break; // pppoe
		case 3:	wan_proto = 3;	break; // bridge
	}
	// wan status
	if (is_up) {
		if (wan_proto == 3) {	// bridge mode
			wan_status = 3;	// connected			
		} else {
			if (ip6_addr->s6_addr32[0] == 0 && ip6_addr->s6_addr32[1] == 0 &&
			    ip6_addr->s6_addr32[2] == 0 && ip6_addr->s6_addr32[3] == 0) {
				wan_status = 2;	// connecting
			} else {
				wan_status = 3;	// connected
			}
		}
	}
	{ // add table entry for ipv6
		util_bitmap_set_value(tab_entry, 25*8, 0, 16, ani_tag & 0xfff); // Vlan
		util_bitmap_set_value(tab_entry, 25*8, 16, 4, 1);		// Format 1
		util_bitmap_set_value(tab_entry, 25*8, 24, 4, wan_proto);
		util_bitmap_set_value(tab_entry, 25*8, 28, 4, wan_status);
		util_bitmap_set_value(tab_entry, 25*8, 32, 32, ip6_addr->s6_addr32[0]);	// Current IPv6 Address
		util_bitmap_set_value(tab_entry, 25*8, 64, 32, ip6_addr->s6_addr32[1]);
		util_bitmap_set_value(tab_entry, 25*8, 96, 32, ip6_addr->s6_addr32[2]);
		util_bitmap_set_value(tab_entry, 25*8, 128, 32, ip6_addr->s6_addr32[3]);
		util_bitmap_set_value(tab_entry, 25*8, 160, 16, masklen);	// Current IPv6 Address Prefix len
	}
}

static void
prepare_table_enry_for_claix_rg_config_format2(char *tab_entry,
	unsigned short rg_tag, unsigned short ani_tag, struct in6_addr *ip6_prefix, int prefix_masklen)
{
	memset(tab_entry, 0, 25);
	{ // add table entry for ipv6
		util_bitmap_set_value(tab_entry, 25*8, 0, 16, ani_tag & 0xfff); // Vlan
		util_bitmap_set_value(tab_entry, 25*8, 16, 4, 2);		// Format 2
		util_bitmap_set_value(tab_entry, 25*8, 32, 32, ip6_prefix->s6_addr32[0]);
		util_bitmap_set_value(tab_entry, 25*8, 64, 32, ip6_prefix->s6_addr32[1]);
		util_bitmap_set_value(tab_entry, 25*8, 96, 32, ip6_prefix->s6_addr32[2]);
		util_bitmap_set_value(tab_entry, 25*8, 128, 32, ip6_prefix->s6_addr32[3]);
		util_bitmap_set_value(tab_entry, 25*8, 160, 16, prefix_masklen);
	}
}

static void
prepare_table_enry_for_claix_rg_config_format3(char *tab_entry,
	unsigned short rg_tag, unsigned short ani_tag, struct in6_addr *ip6_gw)
{
	memset(tab_entry, 0, 25);
	{ // add table entry for ipv6
		util_bitmap_set_value(tab_entry, 25*8, 0, 16, ani_tag & 0xfff); // Vlan
		util_bitmap_set_value(tab_entry, 25*8, 16, 4, 3);		// Format 3
		util_bitmap_set_value(tab_entry, 25*8, 32, 32, ip6_gw->s6_addr32[0]);
		util_bitmap_set_value(tab_entry, 25*8, 64, 32, ip6_gw->s6_addr32[1]);
		util_bitmap_set_value(tab_entry, 25*8, 96, 32, ip6_gw->s6_addr32[2]);
		util_bitmap_set_value(tab_entry, 25*8, 128, 32, ip6_gw->s6_addr32[3]);
	}
}
#endif

// this function integrate the infro from metafile, ppp/dhcp agent and interface runtime
int
meinfo_hw_65317_get_attr12(struct me_t *me) 
{
#ifdef OMCI_METAFILE_ENABLE
	unsigned char mgmt_mode = (unsigned char)meinfo_util_me_attr_data(me, 1); // 0: native (omci), 1: external (tr069 or other)
	struct batchtab_cb_wanif_t *t;
	struct attr_table_header_t *tab_header;
	void *tab_entry;

	struct metacfg_t kv_config;
	char key[128], *value;
	int MAYBE_UNUSED default_wanif_index = -1;
	int cwmp_index = -1;
	int MAYBE_UNUSED i, n;

	
	{ // /var/log/wan_ip.log is updated by staticip.c, dhcp script && pppoe ip-up/ipv6-up script whenever a new ip is set on any wanif
	  // we do this check to eliminate unnecessary read metafile.dat
		FILE *fp;
		if ((fp = fopen("/var/log/wan_ip.log", "r")) != NULL) {
			static unsigned long wan_mtime = 0, meta_mtime = 0;
			struct stat wan_sb, meta_sb;
			fclose(fp);
			if (stat("/var/log/wan_ip.log", &wan_sb) == 0 && stat("/etc/www/metafile.dat", &meta_sb) == 0) {
				if (wan_mtime == wan_sb.st_mtime && meta_mtime == meta_sb.st_mtime )
				    	return 0;
				wan_mtime = wan_sb.st_mtime;
				meta_mtime = meta_sb.st_mtime;
			}
		} else {
			return -1;
		}
	}
	
	if ((t = batchtab_table_data_get("wanif")) == NULL)
		return -1;
					
	// load metafile
	memset(&kv_config, 0x0, sizeof(struct metacfg_t));
	metacfg_adapter_config_init(&kv_config);
	metacfg_adapter_config_file_load_part(&kv_config, "/etc/wwwctrl/metafile.dat", "dev_wan0_interface", "cwmp_interface");

	// system_default_route_interface=dev_wanX_interface
	value= metacfg_adapter_config_get_value(&kv_config, "system_default_route_interface", 1);
	if (strncmp(value, "dev_wan", 7) == 0)
		default_wanif_index = util_atoi(value+7);

	// system_default_route_interface=dev_wanX_interface
	value= metacfg_adapter_config_get_value(&kv_config, "cwmp_interface", 1);
	if (strncmp(value, "dev_wan", 7) == 0)
		cwmp_index = util_atoi(value+7);

	//clear all old entries from table (wan status)
	tab_header = (struct attr_table_header_t *)meinfo_util_me_attr_ptr(me, 12);
	meinfo_me_attr_release_table_all_entries(tab_header);
	tab_entry = malloc_safe(25);

	for (i=0; i<WANIF_INDEX_TOTAL; i++) {
		int MAYBE_UNUSED is_auto, is_en;
		int is_iphost, is_exist, is_up;
		int ipv4_enable = 0, ipv6_enable = 0;
		char *ifname, *ifname2_ipv4 = NULL, *ifname2_ipv6 = NULL;
		int tag_index, ani_tag, rg_tag, rg_pbit;	
		struct iphost_ifaddrs_t iph;
		char mac_address[6]= {0};

		snprintf(key, 128, "dev_wan%d_interface", i);
		ifname = metacfg_adapter_config_get_value(&kv_config, key, 1);
		if (strlen(ifname) == 0)
			continue;

		snprintf(key, 128, "wan%d_auto_created", i);
		value = metacfg_adapter_config_get_value(&kv_config, key, 1);
		is_auto = util_atoi(value);

		snprintf(key, 128, "wan%d_enable", i);
		value = metacfg_adapter_config_get_value(&kv_config, key, 1);
		is_en = util_atoi(value);

		snprintf(key, 128, "wan%d_iphost_only", i);
		value = metacfg_adapter_config_get_value(&kv_config, key, 1);
		is_iphost = util_atoi(value);
		if (mgmt_mode==0 && is_iphost)
			continue;	// don't show iphost interface for calix rg config in native mode

		// even interface doesn't exist, there might be still some info on metafile
		is_exist = iphost_interface_is_exist(ifname);	
		if (is_exist)
			iphost_get_hwaddr(ifname, mac_address);

		snprintf(key, 128, "wan%d_pbit", i);
		value = metacfg_adapter_config_get_value(&kv_config, key, 1);
		rg_pbit = util_atoi(value);
		rg_tag =  wanif_devname_vlan(ifname) | (rg_pbit<<13);
		tag_index = omciutil_misc_tag_list_lookup(rg_tag, t->veip_rg_tag_total, t->veip_rg_tag_list);
		if (tag_index < 0)
			ani_tag = rg_tag;
		else
			ani_tag = t->veip_ani_tag_list[tag_index];

		snprintf(key, 128, "wan%d_ip_enable", i);
		value = metacfg_adapter_config_get_value(&kv_config, key, 1);
		ipv4_enable = util_atoi(value);
		snprintf(key, 128, "wan%d_ipv6_enable", i);
		value = metacfg_adapter_config_get_value(&kv_config, key, 1);
		ipv6_enable = util_atoi(value);
			
		{ // fill ipv4 info into attr12 table
			int assignment, ip_addr=0, netmask_addr=0, gw_addr=0, dns1_addr, dns2_addr;

			// get wanif ipv4 info from pppoe/dhcp files or metafile.dat
			wanif_get_info(ifname, &assignment, &ip_addr, &netmask_addr, &gw_addr, &dns1_addr, &dns2_addr);
			ifname2_ipv4 = ifname;
			if (assignment == 2)
				ifname2_ipv4 = wanif_get_agent_pppname(ifname);
	
			// get current ipv4/v6 addr/mask on the ifname
			is_up = 0;
			memset(&iph, 0, sizeof(iph));		
			if (strlen(ifname2_ipv4) >0) {
				iphost_get_ifaddrs(ifname2_ipv4, &iph);
				is_up = (iph.if_flags & (IFF_UP|IFF_RUNNING))? 1: 0;
			}
			if (iph.ipv4_total) {
				for (n=0; n< iph.ipv4_total; n++) {	// curr_ip=iph.in_addr[n], curr_netmask=iph.in_netmask[n]
					prepare_table_enry_for_claix_rg_config_format0(tab_entry,
						rg_tag, ani_tag, mac_address,
						assignment, iph.in_addr[n].s_addr, iph.in_netmask[n].s_addr, gw_addr,
						is_up, (i==cwmp_index)?1:0, ipv4_enable, ipv6_enable);
					meinfo_me_attr_set_table_entry(me, 12, tab_entry, 0);
				}
				for (n=0; n< iph.ipv6_total; n++) {	// ip=iph.in6_addr[n], netmask=iph.in6_netmask[n]
					// do nothing for ipv6 addr
				}
			} else  {
				prepare_table_enry_for_claix_rg_config_format0(tab_entry,
					rg_tag, ani_tag, mac_address,
					assignment, ip_addr, netmask_addr, gw_addr, 
					is_up, (i==cwmp_index)?1:0, ipv4_enable, ipv6_enable);
				meinfo_me_attr_set_table_entry(me, 12, tab_entry, 0);
			}
		}

		{ // fill ipv6 info into attr12 table
			int ip6_assignment, masklen, prefix_masklen;
			struct in6_addr ip6_addr, ip6_prefix, ip6_gw, ip6_dns1, ip6_dns2;		
	
			// get wanif ipv6 info from pppoe/odhcp6c files or metafile.dat
			wanif_get_info6(ifname, &ip6_assignment, &ip6_addr, &masklen, &ip6_prefix, &prefix_masklen, &ip6_gw, &ip6_dns1, &ip6_dns2);
			ifname2_ipv6 = ifname;
			if (ip6_assignment == 2)
				ifname2_ipv6 = wanif_get_agent_pppname6(ifname);

			if (strlen(ifname2_ipv6) >0) {
				if (strcmp(ifname2_ipv6, ifname2_ipv4) != 0) {
					memset(&iph, 0, sizeof(iph));
					iphost_get_ifaddrs(ifname2_ipv6, &iph);
					is_up = (iph.if_flags & (IFF_UP|IFF_RUNNING))? 1: 0;
				}
			}
			// ipv6 addr (iana)
			if (iph.ipv6_total) {
				for (n=0; n< iph.ipv6_total; n++) {
					int masklen = iphost_get_masklen_ipv6(&iph.in6_netmask[n]);
					if(iph.in6_addr[n].s6_addr[0] == 0xFE && iph.in6_addr[n].s6_addr[1] == 0x80) continue; // skip link-local address
					prepare_table_enry_for_claix_rg_config_format1(tab_entry, rg_tag, ani_tag, ip6_assignment, &ip6_addr, masklen, is_up);
					meinfo_me_attr_set_table_entry(me, 12, tab_entry, 0);
				}
			}
			// ipv6 prefix addr (iapd)
			if (ip6_prefix.s6_addr32[0] || ip6_prefix.s6_addr32[1] || ip6_prefix.s6_addr32[2] || ip6_prefix.s6_addr32[3]) {
				prepare_table_enry_for_claix_rg_config_format2(tab_entry, rg_tag, ani_tag, &ip6_prefix, prefix_masklen);
				meinfo_me_attr_set_table_entry(me, 12, tab_entry, 0);
			}
			// ipv6 gw
			if (ip6_gw.s6_addr32[0] || ip6_gw.s6_addr32[1] || ip6_gw.s6_addr32[2] || ip6_gw.s6_addr32[3]) {
				prepare_table_enry_for_claix_rg_config_format3(tab_entry, rg_tag, ani_tag, &ip6_gw);
				meinfo_me_attr_set_table_entry(me, 12, tab_entry, 0);
			}
		}
	}

	for (i=0; i<BRWAN_INDEX_TOTAL; i++) {
		int is_en, vid, pbit;
		int tag_index, ani_tag, rg_tag;	
		char mac_address[6]= {0};

		snprintf(key, 128, "brwan%d_enable", i);
		value = metacfg_adapter_config_get_value(&kv_config, key, 1);
		is_en = util_atoi(value);
		if (!is_en)
			continue;

		snprintf(key, 128, "brwan%d_vid", i);
		value = metacfg_adapter_config_get_value(&kv_config, key, 1);
		vid = util_atoi(value);

		snprintf(key, 128, "brwan%d_pbit", i);
		value = metacfg_adapter_config_get_value(&kv_config, key, 1);
		pbit = util_atoi(value);

		rg_tag =  vid | (pbit<<13);
		tag_index = omciutil_misc_tag_list_lookup(rg_tag, t->veip_rg_tag_total, t->veip_rg_tag_list);
		if (tag_index < 0)
			ani_tag = rg_tag;
		else
			ani_tag = t->veip_ani_tag_list[tag_index];
			
		memset(tab_entry, 0, 25);
		prepare_table_enry_for_claix_rg_config_format0(tab_entry,
			rg_tag, ani_tag, mac_address, 
			3, 0, 0, 0, 
			1, 0, 1, 0);
		meinfo_me_attr_set_table_entry(me, 12, tab_entry, 0);
	}

	metacfg_adapter_config_release(&kv_config);
	free_safe(tab_entry);
	batchtab_table_data_put("wanif");
#endif
	return 0;
}
// 9.99.18 65317 CalixRgConfig
static int 
meinfo_hw_65317_get(struct me_t *me, unsigned char attr_mask[2])
{
	struct me_t *veip_me;
	char *veip_devname = NULL;
	
	if (me == NULL) {
		dbprintf(LOG_ERR, "me ptr NULL?");
		return 0;
	}

#ifdef OMCI_METAFILE_ENABLE
	// zero attr mask means a special call from create_cb
	if (attr_mask[0] == 0 && attr_mask[1] == 0) {	
		struct attr_value_t attr;	
		attr.data = batchtab_cb_wanif_load_mgmt_mode_from_storage();
		// init attr1 for create_cb
		meinfo_me_attr_set(me, 1, &attr, 0);
		dbprintf(LOG_ERR, "init attr1(mgmt_mode) default to %d\n", attr.data);
		return 0;
	}
#endif
	if ((veip_me=meinfo_me_get_by_meid(329, me->meid)) != NULL)
		veip_devname = meinfo_util_get_config_devname(veip_me);
	
	if (util_attr_mask_get_bit(attr_mask, 12) && veip_devname != NULL) {	// attr12: WanStatus
		meinfo_hw_65317_get_attr12(me);
	}
	
	if (util_attr_mask_get_bit(attr_mask, 13)) {
		char password[GPON_PASSWORD_LEN];
		struct attr_value_t attr;

		memset(password, 0, sizeof(password));
		if (gpon_hw_g.onu_password_get!=NULL)
			gpon_hw_g.onu_password_get(password);
		else
			memcpy(password, "0000000000", GPON_PASSWORD_LEN);	// shoudn't happen

		attr.ptr=malloc_safe(GPON_PASSWORD_LEN);
		memcpy(attr.ptr, password, GPON_PASSWORD_LEN);
		meinfo_me_attr_set(me, 13, &attr, 0);
		if (attr.ptr)
			free_safe(attr.ptr);
	}
	return 0;
}

struct meinfo_hardware_t meinfo_hw_calix_65317 = {
	.get_hw		= meinfo_hw_65317_get,
};
