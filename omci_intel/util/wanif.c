/******************************************************************
 *
 * Copyright (C) 2016 5V Technologies Ltd.
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
 * Module  : util
 * File    : wanif.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <linux/if.h>
#include <errno.h>

#include "util.h"
#include "util_inet.h"
#include "conv.h"
#include "regexp.h"
#include "iphost.h"
#include "metacfg_adapter.h"
#include "wanif.h"

char *
wanif_devname_base(char *devname)
{
	static char basename_wheel[4][32];	// multiple buff to cover re-entry
	static int index = 0;
	char *basename = basename_wheel[(index++)%4];
	char *dotptr;

	strncpy(basename, devname, 31); basename[31] = 0;
	if ((dotptr = strrchr(basename, '.')) != NULL)
		*dotptr = 0;
	return basename;
}

int
wanif_devname_vlan(char *devname)
{
	char *dotptr;
	if ((dotptr = strrchr(devname, '.')) != NULL)
		return atoi(dotptr+1);
	return -1;
}

#ifdef OMCI_METAFILE_ENABLE
// devname: the devname used for match
// kvname: buff to store the found kv_config value
// kvname_size: the size of buff
int
wanif_locate_index_by_devname(struct metacfg_t *kv_config, char *devname, char *kvname, int kvname_size)
{
	char key[32], *value;
	int i;
	for (i=0; i<WANIF_INDEX_TOTAL; i++) {
		snprintf(key, 31, "dev_wan%d_interface", i);
		value = metacfg_adapter_config_get_value(kv_config, key, 0);
		if (value && strcmp(value, devname) == 0) {
			if (kvname && kvname_size>0)
				strncpy(kvname, value, kvname_size);
			return i;
		}
	}
	return -1;
}

// devname: the devname used for match
// kvname: buff to store the found kv_config value
// kvname_size: the size of buff
int
wanif_locate_index_by_devname_base(struct metacfg_t *kv_config, char *devname, char *kvname, int kvname_size)
{
	char key[32], *value;
	char devname_base[32]={0}, value_base[32]={0};
	int i;
	strncpy(devname_base, wanif_devname_base(devname), 31); devname_base[31] = 0;
	for (i=0; i<WANIF_INDEX_TOTAL; i++) {
		snprintf(key, 31, "dev_wan%d_interface", i);
		value = metacfg_adapter_config_get_value(kv_config, key, 0);
		if (value) {
			if (strlen(value) == 0)
				continue;
			strncpy(value_base, wanif_devname_base(value), 31); value_base[31] = 0;
			if (strcmp(value_base, devname_base) == 0) {
				if (kvname && kvname_size>0)
					strncpy(kvname, value, kvname_size);
				return i;
			}
		}
	}
	return -1;
}

// devname: the devname used for match
// kvname: buff to store the found kv_config value
// kvname_size: the size of buff
int
wanif_locate_index_by_vlan(struct metacfg_t *kv_config, int vlan, char *kvname, int kvname_size)
{
	char key[32], *value;
	int i, wanif_vlan;
	for (i=0; i<WANIF_INDEX_TOTAL; i++) {
		snprintf(key, 31, "dev_wan%d_interface", i);
		value = metacfg_adapter_config_get_value(kv_config, key, 0);
		if (value) {
			if (strlen(value) == 0)
				continue;
			wanif_vlan = wanif_devname_vlan(value);
			if (wanif_vlan <0)	// untag
				wanif_vlan = 0;
			if (wanif_vlan == vlan) {
				if (kvname && kvname_size>0)
					strncpy(kvname, value, kvname_size);
				return i;
			}
		}
	}
	return -1;
}

unsigned int
wanif_get_indexmap_by_vlan(struct metacfg_t *kv_config, int vlan)
{
	char key[32], *value;
	int i, wanif_vlan;
	unsigned int indexmap = 0;

	for (i=0; i<WANIF_INDEX_TOTAL; i++) {
		snprintf(key, 31, "dev_wan%d_interface", i);
		value = metacfg_adapter_config_get_value(kv_config, key, 0);
		if (value) {
			if (strlen(value) == 0)
				continue;
			wanif_vlan = wanif_devname_vlan(value);
			if (wanif_vlan <0)	// untag
				wanif_vlan = 0;
			if (wanif_vlan == vlan)
				indexmap |= (1<<i);
		}
	}
	return indexmap;
}

int
wanif_locate_brwan_index_by_vlan(struct metacfg_t *kv_config, int vlan)
{
	char key[32], *value;
	int i;
	for (i=0; i<BRWAN_INDEX_TOTAL; i++) {
		snprintf(key, 31, "brwan%d_vid", i);
		value = metacfg_adapter_config_get_value(kv_config, key, 0);
		if (value) {
			if (strlen(value) == 0)
				continue;
			int brwan_vid = util_atoi(value);
			if (brwan_vid == vlan)
				return i;
		}
	}
	return -1;
}

int
wanif_locate_index_unused(struct metacfg_t *kv_config)
{
	char key[32], *value;
	int i;
	for (i=0; i<WANIF_INDEX_TOTAL; i++) {
		snprintf(key, 31, "dev_wan%d_interface", i);
		value = metacfg_adapter_config_get_value(kv_config, key, 0);
		if (value == NULL || strlen(value) == 0)
			return i;
	}
	return -1;
}

int
wanif_locate_index_default_route(struct metacfg_t *kv_config)
{
	char key[32], *value;
	int i;
	value = metacfg_adapter_config_get_value(kv_config, "system_default_route_interface", 1);
	for (i=0; i<WANIF_INDEX_TOTAL; i++) {
		snprintf(key, 31, "dev_wan%d_interface", i);
		if (strcmp(key, value) == 0)
			return i;
	}
	return -1;
}

int
wanif_delete_by_index(struct metacfg_t *kv_config, int wanif_index) {
	char key[64], value[64];

	key[63] = value[63] = 0;

	snprintf(key, 63,  "wan%d_enable", wanif_index);
	metacfg_adapter_config_entity_update(kv_config, key, "0");

	snprintf(key, 63,  "dev_wan%d_interface", wanif_index);
	metacfg_adapter_config_entity_update(kv_config, key, "");
	snprintf(key, 63,  "wan%d_if_name", wanif_index);
	snprintf(value, 63, "Wan%d", wanif_index);
	metacfg_adapter_config_entity_update(kv_config, key, value);

	snprintf(key, 63,  "wan%d_pbit", wanif_index);
	metacfg_adapter_config_entity_update(kv_config, key, "0");
	snprintf(key, 63,  "wan%d_dscp2pbit_enable", wanif_index);
	metacfg_adapter_config_entity_update(kv_config, key, "0");

	snprintf(key, 63,  "wan%d_auto_created", wanif_index);
	metacfg_adapter_config_entity_update(kv_config, key, "0");
	snprintf(key, 63,  "wan%d_iphost_only", wanif_index);
	metacfg_adapter_config_entity_update(kv_config, key, "0");
	snprintf(key, 63,  "wan%d_igmp_enable", wanif_index);
	if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX) == 0) {
		metacfg_adapter_config_entity_update(kv_config, key, "0");
	} else {
		metacfg_adapter_config_entity_update(kv_config, key, "1");
	}

	// set ip_enable, ipv6_enable default to 1
	snprintf(key, 63,  "wan%d_ip_enable", wanif_index);
	metacfg_adapter_config_entity_update(kv_config, key, "1");
	snprintf(key, 63,  "wan%d_ipv6_enable", wanif_index);
	metacfg_adapter_config_entity_update(kv_config, key, "1");

	return 1;
}

int
wanif_delete_brwan_by_index(struct metacfg_t *kv_config, int brwan_index) {
	char key[64], value[64];

	key[63] = value[63] = 0;

	snprintf(key, 63,  "brwan%d_enable", brwan_index);
	metacfg_adapter_config_entity_update(kv_config, key, "0");

	snprintf(key, 63,  "brwan%d_name", brwan_index);
	metacfg_adapter_config_entity_update(kv_config, key, "");

	snprintf(key, 63,  "brwan%d_vid", brwan_index);
	metacfg_adapter_config_entity_update(kv_config, key, "0");
	snprintf(key, 63,  "brwan%d_pbit", brwan_index);
	metacfg_adapter_config_entity_update(kv_config, key, "0");
	snprintf(key, 63,  "brwan%d_dscp2pbit_enable", brwan_index);
	metacfg_adapter_config_entity_update(kv_config, key, "0");
	snprintf(key, 63,  "brwan%d_portmask", brwan_index);
	metacfg_adapter_config_entity_update(kv_config, key, "0");

	snprintf(key, 63,  "brwan%d_igmp_enable", brwan_index);
	if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX) == 0) {
		metacfg_adapter_config_entity_update(kv_config, key, "0");
	} else {
		metacfg_adapter_config_entity_update(kv_config, key, "1");
	}
	return 1;
}

// 1. the wan[0-7]_auto_created will be set when ever a wanif is matched in batchtab cb wanif
// 2. this routine would del only wanif with wan[0-7]_auto_created==1, which means  the wanif has even been created/used as calixrg/tr069 wanif or rgvaln wanif
// 3. this routine wont del wanif with wan[0-7]_auto_created==0, which might be created by web or cwmp
int
wanif_delete_auto_by_index(struct metacfg_t *kv_config, int wanif_index)
{
	char key[64], value[64];
	int is_auto_created;

	key[63] = value[63] = 0;

	snprintf(key, 63, "wan%d_auto_created", wanif_index); 
	is_auto_created = util_atoi(metacfg_adapter_config_get_value(kv_config, key, 1));
	if (is_auto_created) {
		wanif_delete_by_index(kv_config, wanif_index);
		return 1;
	}
	return 0;
}

int
wanif_get_replicated_indexmap(struct metacfg_t *kv_config)
{
	char key[32], *value, *value2;
	int i, j;
	int indexmap = 0;

	for (i=0; i<WANIF_INDEX_TOTAL; i++) {
		if (indexmap & (1<<i))
			continue;
		snprintf(key, 31, "dev_wan%d_interface", i);
		value = metacfg_adapter_config_get_value(kv_config, key, 1);
		if (strlen(value) == 0)
			continue;

		for (j=i+1; j<WANIF_INDEX_TOTAL; j++) {
			if (indexmap & (1<<j))
				continue;
			snprintf(key, 31, "dev_wan%d_interface", j);
			value2 = metacfg_adapter_config_get_value(kv_config, key, 1);
			if (strlen(value2) == 0)
				continue;
			if (strcmp(value, value2) == 0)
				indexmap |= ((1<<i)|(1<<j));
		}
	}

	return indexmap;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////

static int
is_ipv6_zero(struct in6_addr *ip6_addr)
{
	if (ip6_addr->s6_addr32[0] ||
	    ip6_addr->s6_addr32[1] ||
	    ip6_addr->s6_addr32[2] ||
	    ip6_addr->s6_addr32[3])
	    	return 0;
	return 1;
}

static void
set_ipv6_zero(struct in6_addr *ip6_addr)
{
	ip6_addr->s6_addr32[0] = ip6_addr->s6_addr32[1] = ip6_addr->s6_addr32[2] = ip6_addr->s6_addr32[3] = 0;
}
#endif

// get pppname for specific device
static char *
_wanif_get_agent_pppname(char *ifname, int is_ipv6)
{
	static char pppname_wheel[4][32];
	static int index = 0;

	char *pppname = pppname_wheel[(index++)%4];
	char filename[128];
	struct regexp_context_t re;
	char *output=NULL;

	pppname[0] = 0;
	if (is_ipv6)
		snprintf(filename, 127, "/var/run/ppp6/pppoe.info.%s", ifname);
	else
		snprintf(filename, 127, "/var/run/ppp/pppoe.info.%s", ifname);
	if (util_readfile(filename, &output) > 0) {
		// new pppoe.info.pon0.4082: DEVICE=pon0.4082, IFNAME=ppp0
		regexp_prepare(&re, "IFNAME=(ppp[0-9]+)");
		if (regexp_match(&re, output) > 0)
			strncpy(pppname, regexp_match_str(&re, 1), 32);
		regexp_release(&re);
		if (strlen(pppname) == 0) {	// try old version 
			// old pppoe.info.pon0.4082: IFNAME=pon0.4082, PPPNAME=ppp0
			regexp_prepare(&re, "PPPNAME=(ppp[0-9]+)");
			if (regexp_match(&re, output) > 0)
				strncpy(pppname, regexp_match_str(&re, 1), 32);
			regexp_release(&re);
		}
		free_safe(output);
	}
	return pppname;
}
char *
wanif_get_agent_pppname(char *ifname)
{
	return _wanif_get_agent_pppname(ifname, 0);
}
char *
wanif_get_agent_pppname6(char *ifname)
{
	return _wanif_get_agent_pppname(ifname, 1);
}

// get network info from ppp/udhcpc/isc-dhcpc agent
// this function returns error when ifname doesn't exist or agent file is not available
int
wanif_get_agent_info(char *ifname, int *assignment, int *ip_addr, int *netmask_addr, int *gw_addr, int *dns1_addr, int *dns2_addr)
{
	char filename[128];
	struct regexp_context_t re;
	char *output=NULL;

	*assignment = *ip_addr = *netmask_addr = *gw_addr = *dns1_addr = *dns2_addr = 0;

	if (iphost_interface_is_exist(ifname) == 0)
		return -1;

	snprintf(filename, 127, "/var/run/ppp/pppoe.info.%s", ifname);
	if (util_readfile(filename, &output) > 0) {

		*assignment = 2;	// 0:static, 1:dhcp, 2:pppoe

		regexp_prepare(&re, "IPLOCAL=([0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+)");
		if (regexp_match(&re, output) > 0)
			*ip_addr = inet_addr(regexp_match_str(&re, 1));
		regexp_release(&re);
		*netmask_addr = inet_addr("255.255.255.255");
		regexp_prepare(&re, "IPREMOTE=([0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+)");
		if (regexp_match(&re, output) > 0)
			*gw_addr = inet_addr(regexp_match_str(&re, 1));
		regexp_release(&re);
		regexp_prepare(&re, "DNS1=([0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+)");
		if (regexp_match(&re, output) > 0)
			*dns1_addr = inet_addr(regexp_match_str(&re, 1));
		regexp_release(&re);
		regexp_prepare(&re, "DNS2=([0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+)");
		if (regexp_match(&re, output) > 0)
			*dns2_addr = inet_addr(regexp_match_str(&re, 1));
		regexp_release(&re);

		free_safe(output);
		return 0;
	}

	snprintf(filename, 127, "/var/run/dhcp/dhcpcv4.leases.%s", ifname);
	if (util_readfile(filename, &output) > 0) {
		char *start, *ptr;

		*assignment = 1;	// 0:static, 1:dhcp, 2:pppoe

		// try to locate the last 'lease {...}' block, the start pointer will point to the next char after 'lease {'
		for (start = output; (ptr = strstr(start, "lease {")) != NULL; start = ptr+7);

		regexp_prepare(&re, "fixed-address ([0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+)");
		if (regexp_match(&re, start) > 0)
			*ip_addr = inet_addr(regexp_match_str(&re, 1));
		regexp_release(&re);
		regexp_prepare(&re, "option subnet-mask ([0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+)");
		if (regexp_match(&re, start) > 0)
			*netmask_addr = inet_addr(regexp_match_str(&re, 1));
		regexp_release(&re);
		regexp_prepare(&re, "option routers ([0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+)");
		if (regexp_match(&re, start) > 0)
			*gw_addr = inet_addr(regexp_match_str(&re, 1));
		regexp_release(&re);

		regexp_prepare(&re, "option domain-name-servers ([0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+),([0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+)");
		if (regexp_match(&re, start) > 0) {
			*dns1_addr = inet_addr(regexp_match_str(&re, 1));
			*dns2_addr = inet_addr(regexp_match_str(&re, 2));
		}
		regexp_release(&re);
		if (*dns1_addr == 0) {
			regexp_prepare(&re, "option domain-name-servers ([0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+)");
			if (regexp_match(&re, start) > 0)
				*dns1_addr = inet_addr(regexp_match_str(&re, 1));
			regexp_release(&re);
		}

		free_safe(output);
		return 0;
	}

	snprintf(filename, 127, "/var/run/udhcpc/udhcpc.lease.%s", ifname);
	if (util_readfile(filename, &output) > 0) {

		*assignment = 1;	// 0:static, 1:dhcp, 2:pppoe

		regexp_prepare(&re, "udhcpc_ip=([0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+)");
		if (regexp_match(&re, output) > 0)
			*ip_addr = inet_addr(regexp_match_str(&re, 1));
		regexp_release(&re);
		regexp_prepare(&re, "udhcpc_subnet=([0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+)");
		if (regexp_match(&re, output) > 0)
			*netmask_addr = inet_addr(regexp_match_str(&re, 1));
		regexp_release(&re);
		regexp_prepare(&re, "udhcpc_router=([0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+)");
		if (regexp_match(&re, output) > 0)
			*gw_addr = inet_addr(regexp_match_str(&re, 1));
		regexp_release(&re);

		regexp_prepare(&re, "udhcpc_dns=([0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+) ([0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+)");
		if (regexp_match(&re, output) > 0) {
			*dns1_addr = inet_addr(regexp_match_str(&re, 1));
			*dns2_addr = inet_addr(regexp_match_str(&re, 2));
		}
		regexp_release(&re);
		if (*dns1_addr == 0) {
			regexp_prepare(&re, "udhcpc_dns=([0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+)");
			if (regexp_match(&re, output) > 0)
				*dns1_addr = inet_addr(regexp_match_str(&re, 1));
			regexp_release(&re);
		}

		free_safe(output);
		return 0;
	}

	return -2;
}

// get network info from odhcp6c agent
// this function returns error when ifname doesn't exist or agent file is not available
int
wanif_get_agent_info6(char *ifname, int *assignment, 
			struct in6_addr *ip6_addr, int *masklen, 
			struct in6_addr *ip6_prefix, int *prefix_masklen,
			struct in6_addr *ip6_gw, 
			struct in6_addr *ip6_dns1, struct in6_addr *ip6_dns2)
{
#ifdef OMCI_METAFILE_ENABLE
	char filename[128];
	struct metacfg_t kv_config;
	char *ifname2;
	int ret = -2;
	
	// init ret var with 0, so caller won't get uninitialized value when error happen
	*assignment = *masklen = *prefix_masklen = 0;
	set_ipv6_zero(ip6_addr); set_ipv6_zero(ip6_gw); set_ipv6_zero(ip6_prefix);
	set_ipv6_zero(ip6_dns1); set_ipv6_zero(ip6_dns2);

	if (iphost_interface_is_exist(ifname) == 0)
		return -1;
	ifname2 = wanif_get_agent_pppname6(ifname);
	if (strlen(ifname2) > 0) {
		*assignment = 2;	// 0:static, 1:dhcp, 2:pppoe
		snprintf(filename, 127, "/var/run/odhcp6c.%s", ifname2);	// odhcp6c.ppp0
	} else {
		*assignment = 1;	// 0:static, 1:dhcp, 2:pppoe
		snprintf(filename, 127, "/var/run/odhcp6c.%s", ifname);		// odhcp6c.pon1.4081
	}

	memset(&kv_config, 0x0, sizeof(struct metacfg_t));
	metacfg_adapter_config_init(&kv_config);
	if (util_file_is_available(filename) &&
	    metacfg_adapter_config_file_load_safe(&kv_config, filename) == 0) {
		char *value;
		value = metacfg_adapter_config_get_value(&kv_config, "ACTION", 1);
		
		// refer to odhcp6c info file only if the ACTION is not stopped/unbound
		if (strcmp(value, "stopped")!=0 && strcmp(value, "unbound")!=0) {
			char *start, *end;

			// server that delivers RA (the dhcpv6 server might be different than RA source)
			// value = kv_config_get_value(&kv_config, "SERVER", 1);
			//if (strlen(value) > 0)
			//	util_inet_pton(AF_INET6, value, (unsigned char *)ip6_gw);

			// routes for this ipv6 wan, eg:RA_ROUTES=::/0,fe80::cd59:ffba:cffb:b,1800,512 2001:b011:2010:135a:2532::/96,,86400,256
			value = metacfg_adapter_config_get_value(&kv_config, "RA_ROUTES", 1);
			if ((start = strstr(value, "::/0,")) != NULL) {
				start += strlen("::/0,");
			} else {
				start = strstr(value, "fe80:");
			}
			if (start) {
				end = strstr(start, " ");
				if (end) *end = 0;
				end = strstr(start, ",");
				if (end) *end = 0;
				if (strlen(start) > 0)
					util_inet_pton(AF_INET6, start, (unsigned char *)ip6_gw);
			}

			// only available if iapd is enabled
			value = metacfg_adapter_config_get_value(&kv_config, "PREFIXES", 1);
			if (strlen(value) > 0) {
				char *p=strchr(value, '/');
				if (p) {
					*p = 0;
					*prefix_masklen = util_atoi(p+1);
					util_inet_pton(AF_INET6, value, (unsigned char *)ip6_prefix);
				}
			}
			// ADDRESS is merged from ADDRESSES and RA_ADDRESSES
			value = metacfg_adapter_config_get_value(&kv_config, "ADDRESS", 1);
			if (strlen(value) > 0) {
				char *p=strchr(value, '/');
				if (p) {
					*p = 0;
					*masklen = util_atoi(p+1);
					util_inet_pton(AF_INET6, value, (unsigned char *)ip6_addr);
				}
			}
			// DNS is merged from RDNSS & RA_DNS by odhcp6c-update
			value = metacfg_adapter_config_get_value(&kv_config, "DNS", 1);
			if (strlen(value) > 0) {
				char *dns_list[8];
				int dns_total = conv_str2array(value, " ", dns_list, 8);
				if (dns_total>=1)
					util_inet_pton(AF_INET6, dns_list[0], (unsigned char *)ip6_dns1);
				if (dns_total>=2)
					util_inet_pton(AF_INET6, dns_list[1], (unsigned char *)ip6_dns2);
			}
		}
		ret = 0;
	}
	metacfg_adapter_config_release(&kv_config);
	return ret;
#else
	return 0;
#endif
}

// get interface network from files: including pppoe/dhcp agent files, system metafiles
// this function returns the extstance of the interface, no error would be returned
int
wanif_get_info(char *ifname, int *assignment, int *ip_addr, int *netmask_addr, int *gw_addr, int *dns1_addr, int *dns2_addr)
{
#ifdef OMCI_METAFILE_ENABLE
	struct metacfg_t kv_config;
	char wanif_devname[32];
	int wanif_index;
	char key[128] = {0};
	int is_exist;

	// init ret var with 0, so caller won't get uninitialized value when error happen
	*assignment = *ip_addr = *netmask_addr = *gw_addr = *dns1_addr = *dns2_addr = 0;

	is_exist = iphost_interface_is_exist(ifname);

	// load current metafile.dat, must do first
	memset(&kv_config, 0x0, sizeof(struct metacfg_t));
	metacfg_adapter_config_init(&kv_config);
	metacfg_adapter_config_file_load_part(&kv_config, "/etc/wwwctrl/metafile.dat", "dev_wan0_interface", "datetime_ntp_enable");

	wanif_index = wanif_locate_index_by_devname(&kv_config, ifname, wanif_devname, 32);
	if (wanif_index < 0) {
		// no metafile info for reference, try ppp/dhcp agent anyway
		metacfg_adapter_config_release(&kv_config);

		if (!is_exist) {
			dbprintf(LOG_NOTICE, "%s: wanif_index<0, is_exist=0\n", ifname);
			return 0;
		}
		wanif_get_agent_info(ifname, assignment, ip_addr, netmask_addr, gw_addr, dns1_addr, dns2_addr);
		if (*assignment == 0) {	// not ppp/dhcp, try iphost runtime
			int is_up;
		    	iphost_get_ipv4_ip_netmask(ifname, ip_addr, netmask_addr, &is_up);
		    	iphost_get_ipv4_defgw(gw_addr);
		    	iphost_get_ipv4_dns("/etc/resolv.conf", dns1_addr, dns2_addr);
		}
		dbprintf(LOG_NOTICE, "%s: wanif_index<0, is_exist=%d, assignment=%d, ip=0x%x, netmask=0x%x, gw=0x%x, dns1=0x%x, dns2=0x%x\n",
	   		ifname, is_exist, *assignment, *ip_addr, *netmask_addr, *gw_addr, *dns1_addr, *dns2_addr);
		return 1;
	}

	snprintf(key, 127, "wan%d_ip_assignment", wanif_index);
	*assignment = util_atoi(metacfg_adapter_config_get_value(&kv_config, key, 1));
	if (*assignment == 0) {	// static ip
		char *value;
		snprintf(key, 127, "wan%d_ip", wanif_index);
		value = metacfg_adapter_config_get_value(&kv_config, key, 1);
		if ((*ip_addr = inet_addr(value)) == -1) *ip_addr = 0;

		snprintf(key, 127, "wan%d_netmask", wanif_index);
		value = metacfg_adapter_config_get_value(&kv_config, key, 1);
		if ((*netmask_addr = inet_addr(value)) == -1 && strcmp(value, "255.255.255.255")!=0) *netmask_addr = 0;

		snprintf(key, 127, "wan%d_gateway", wanif_index);
		value = metacfg_adapter_config_get_value(&kv_config, key, 1);
		if ((*gw_addr = inet_addr(value)) == -1) *gw_addr = 0;

		snprintf(key, 127, "wan%d_dns1", wanif_index);
		value = metacfg_adapter_config_get_value(&kv_config, key, 1);
		if ((*dns1_addr = inet_addr(value)) == -1) *dns1_addr = 0;

		snprintf(key, 127, "wan%d_dns2", wanif_index);
		value = metacfg_adapter_config_get_value(&kv_config, key, 1);
		if ((*dns2_addr = inet_addr(value)) == -1) *dns2_addr = 0;

	} else if (*assignment == 1 || *assignment == 2) {	// 1:dhcp, 2:pppoe
		int dnsauto_enable, agent_assignment;
		if (is_exist) {
			// wanif_get_agent_info might return assignment=0 in case error happen, we use agent_assignment instead,
			// so *assignment wont be override (we need *assignment as it is derived from metafile)
			if (wanif_get_agent_info(ifname, &agent_assignment, ip_addr, netmask_addr, gw_addr, dns1_addr, dns2_addr) <0)
				dbprintf(LOG_NOTICE, "%s: wanif_get_agent_info err, agent not started?\n", ifname);
		}
		snprintf(key, 127, "wan%d_dnsauto_enable", wanif_index);
		dnsauto_enable = util_atoi(metacfg_adapter_config_get_value(&kv_config, key, 1));
		if (dnsauto_enable == 0) {
			snprintf(key, 127, "wan%d_dns1", wanif_index);
			*dns1_addr = inet_addr(metacfg_adapter_config_get_value(&kv_config, key, 1));
			snprintf(key, 127, "wan%d_dns2", wanif_index);
			*dns2_addr = inet_addr(metacfg_adapter_config_get_value(&kv_config, key, 1));
			if (*dns1_addr == -1) *dns1_addr = 0;
			if (*dns2_addr == -1) *dns2_addr = 0;
		}
	}
	dbprintf(LOG_NOTICE, "%s: is_exist=%d, wanif_index=%d, assignment=%d, ip=0x%x, netmask=0x%x, gw=0x%x, dns1=0x%x, dns2=0x%x\n",
   		ifname, is_exist, wanif_index, *assignment, *ip_addr, *netmask_addr, *gw_addr, *dns1_addr, *dns2_addr);

	metacfg_adapter_config_release(&kv_config);
	return is_exist;
#else
	return 0;
#endif
}

// get interface network from files: including pppoe/odhcp6c agent files, system metafiles
// this function returns the extstance of the interface, no error would be returned
int
wanif_get_info6(char *ifname, int *assignment, 
		struct in6_addr *ip6_addr, int *masklen, 
		struct in6_addr *ip6_prefix, int *prefix_masklen,
		struct in6_addr *ip6_gw, 
		struct in6_addr *ip6_dns1, struct in6_addr *ip6_dns2)
{
#ifdef OMCI_METAFILE_ENABLE
	struct metacfg_t kv_config;
	char wanif_devname[32];
	int wanif_index;
	char key[128] = {0};
	int is_exist;

	// init ret var with 0, so caller won't get uninitialized value when error happen
	*assignment = *masklen = *prefix_masklen = 0;
	set_ipv6_zero(ip6_addr); set_ipv6_zero(ip6_gw); set_ipv6_zero(ip6_prefix);
	set_ipv6_zero(ip6_dns1); set_ipv6_zero(ip6_dns2);

	is_exist = iphost_interface_is_exist(ifname);

	// load current metafile.dat, must do first
	memset(&kv_config, 0x0, sizeof(struct metacfg_t));
	metacfg_adapter_config_init(&kv_config);
	metacfg_adapter_config_file_load_part(&kv_config, "/etc/wwwctrl/metafile.dat", "dev_wan0_interface", "datetime_ntp_enable");

	wanif_index = wanif_locate_index_by_devname(&kv_config, ifname, wanif_devname, 32);
	if (wanif_index < 0) {
		// no metafile info for reference, try ppp/dhcp agent anyway
		metacfg_adapter_config_release(&kv_config);

		if (!is_exist) {
			dbprintf(LOG_NOTICE, "%s: wanif_index<0, is_exist=0\n", ifname);
			return 0;
		}
		wanif_get_agent_info6(ifname, assignment, ip6_addr, masklen, ip6_prefix, prefix_masklen, ip6_gw, ip6_dns1, ip6_dns2);
		if (*assignment == 0) {	// not ppp/dhcp, try iphost runtime
			struct iphost_ifaddrs_t iph;
			int n;
			//int is_up = 0;			
			iphost_get_ifaddrs(ifname, &iph);
			//is_up = (iph.if_flags & (IFF_UP|IFF_RUNNING))? 1: 0;
			for (n=0; n< iph.ipv6_total; n++) {
				if (iph.in6_addr[n].s6_addr[0] != 0xfe80) {
					*ip6_addr = iph.in6_addr[n];
					*masklen = iphost_get_masklen_ipv6(&iph.in6_netmask[n]);
					break;
				}
			}
		    	// iphost_get_ipv6_defgw(gw_addr);	// FIXME, to be done...
		    	iphost_get_ipv6_dns("/etc/resolv.conf", ip6_dns1, ip6_dns2);
		}
		{
			char str[80];			
			dbprintf(LOG_NOTICE, "%s: wanif_index<0, is_exist=%d, assignment=%d", ifname, is_exist, *assignment);
			util_dbprintf(omci_env_g.debug_level, LOG_NOTICE, 0, ", ip=%s, masklen=%d", util_inet_ntop(AF_INET6, ip6_addr, str, sizeof(str)), *masklen);
			util_dbprintf(omci_env_g.debug_level, LOG_NOTICE, 0, ", prefix=%s, prefix_masklen=%d", util_inet_ntop(AF_INET6, ip6_prefix, str, sizeof(str)), *prefix_masklen);
			util_dbprintf(omci_env_g.debug_level, LOG_NOTICE, 0, ", gw=%s", util_inet_ntop(AF_INET6, ip6_gw, str, sizeof(str)));
			util_dbprintf(omci_env_g.debug_level, LOG_NOTICE, 0, ", dns1=%s", util_inet_ntop(AF_INET6, ip6_dns1, str, sizeof(str)));
			util_dbprintf(omci_env_g.debug_level, LOG_NOTICE, 0, ", dns2=%s", util_inet_ntop(AF_INET6, ip6_dns2, str, sizeof(str)));
			util_dbprintf(omci_env_g.debug_level, LOG_NOTICE, 0, "\n");
		}
		return 1;
	}

	snprintf(key, 127, "wan%d_ipv6_assignment", wanif_index);
	*assignment = util_atoi(metacfg_adapter_config_get_value(&kv_config, key, 1));
	if (*assignment == 0) {	// static ip
		char *value;
		snprintf(key, 127, "wan%d_ipv6_ip", wanif_index);
		value = metacfg_adapter_config_get_value(&kv_config, key, 1);
		if (value && strlen(value) >0)
			util_inet_pton(AF_INET6, value, (unsigned char *)ip6_addr);

		snprintf(key, 127, "wan%d_ipv6_prefix_length", wanif_index);
		value = metacfg_adapter_config_get_value(&kv_config, key, 1);
		if (value && strlen(value) >0)
			*masklen = *prefix_masklen = util_atoi(value);
		
		snprintf(key, 127, "wan%d_ipv6_gateway", wanif_index);
		value = metacfg_adapter_config_get_value(&kv_config, key, 1);
		if (value && strlen(value) >0)
			util_inet_pton(AF_INET6, value, (unsigned char *)ip6_gw);

		snprintf(key, 127, "wan%d_ip6_dns1", wanif_index);
		value = metacfg_adapter_config_get_value(&kv_config, key, 1);
		if (value && strlen(value) >0)
			util_inet_pton(AF_INET6, value, (unsigned char *)ip6_dns1);

		snprintf(key, 127, "wan%d_ip6_dns2", wanif_index);
		value = metacfg_adapter_config_get_value(&kv_config, key, 1);
		if (value && strlen(value) >0)
			util_inet_pton(AF_INET6, value, (unsigned char *)ip6_dns2);

	} else if (*assignment == 1 || *assignment == 2) {	// 1:dhcp, 2:pppoe
		int dnsauto_enable, agent_assignment;
		if (is_exist) {
			// wanif_get_agent_info might return assignment=0 in case error happen, we use agent_assignment instead,
			// so *assignment wont be override (we need *assignment as it is derived from metafile)
			if (wanif_get_agent_info6(ifname, &agent_assignment, ip6_addr, masklen, ip6_prefix, prefix_masklen, ip6_gw, ip6_dns1, ip6_dns2) <0)
				dbprintf(LOG_NOTICE, "%s: wanif_get_agent_info6 err, agent not started?\n", ifname);
		}
		snprintf(key, 127, "wan%d_dnsauto_enable", wanif_index);
		dnsauto_enable = util_atoi(metacfg_adapter_config_get_value(&kv_config, key, 1));
		if (dnsauto_enable == 0) {
			char *value;
			snprintf(key, 127, "wan%d_ipv6_dns1", wanif_index);
			value = metacfg_adapter_config_get_value(&kv_config, key, 1);
			if (value && strlen(value) >0)
				util_inet_pton(AF_INET6, value, (unsigned char *)ip6_dns2);
			snprintf(key, 127, "wan%d_ipv6_dns2", wanif_index);
			value = metacfg_adapter_config_get_value(&kv_config, key, 1);
			if (value && strlen(value) >0)
				util_inet_pton(AF_INET6, value, (unsigned char *)ip6_dns2);
		}
	}
	{
		char str[80];			
		dbprintf(LOG_NOTICE, "%s: wanif_index=%d, is_exist=%d, assignment=%d", ifname, wanif_index, is_exist, *assignment);
		util_dbprintf(omci_env_g.debug_level, LOG_NOTICE, 0, ", ip=%s, masklen=%d", util_inet_ntop(AF_INET6, ip6_addr, str, sizeof(str)), *masklen);
		util_dbprintf(omci_env_g.debug_level, LOG_NOTICE, 0, ", prefix=%s, prefix_masklen=%d", util_inet_ntop(AF_INET6, ip6_prefix, str, sizeof(str)), *prefix_masklen);
		util_dbprintf(omci_env_g.debug_level, LOG_NOTICE, 0, ", gw=%s", util_inet_ntop(AF_INET6, ip6_gw, str, sizeof(str)));
		util_dbprintf(omci_env_g.debug_level, LOG_NOTICE, 0, ", dns1=%s", util_inet_ntop(AF_INET6, ip6_dns1, str, sizeof(str)));
		util_dbprintf(omci_env_g.debug_level, LOG_NOTICE, 0, ", dns2=%s", util_inet_ntop(AF_INET6, ip6_dns2, str, sizeof(str)));
		util_dbprintf(omci_env_g.debug_level, LOG_NOTICE, 0, "\n");
	}

	metacfg_adapter_config_release(&kv_config);
	return is_exist;
#else
	return 0;
#endif
}

int
wanif_get_hwintf_index_by_ifname(char *ifname)
{
#ifdef OMCI_METAFILE_ENABLE
	struct metacfg_t kv_config;
	int ret = -1;

	memset(&kv_config, 0x0, sizeof(struct metacfg_t));
	metacfg_adapter_config_init(&kv_config);
	if (util_file_is_available("/var/run/rg_intf.conf") &&
	    metacfg_adapter_config_file_load_safe(&kv_config, "/var/run/rg_intf.conf") == 0) {
		char key[128] = {0};
		char *value;
		int i;
		for (i=0; i<8; i++) {
			snprintf(key, 127, "wan%d_hw_intf_name", i);
			value = metacfg_adapter_config_get_value(&kv_config, key, 1);
			if (strcmp(value, ifname) == 0) {	// name matched, find intf index
				int hwintf_index;
				snprintf(key, 127, "wan%d_hw_intf_index", i);
				value = metacfg_adapter_config_get_value(&kv_config, key, 1);
				hwintf_index = util_atoi(value);
				if (strlen(value) >0 && hwintf_index >=0) {
					ret = hwintf_index;
				}
				break;
			}
		}
	}
	metacfg_adapter_config_release(&kv_config);
	return ret;
#else
	return -1;
#endif
}

// this function integrate the infro from metafile, ppp/dhcp agent and interface runtime
int
wanif_print_status(int fd)
{
#ifdef OMCI_METAFILE_ENABLE
	struct metacfg_t kv_config;
	char key[128], *value;
	int default_wanif_index = -1, i;
	int is_brwan_header_printed = 0;

	memset(&kv_config, 0x0, sizeof(struct metacfg_t));
	metacfg_adapter_config_init(&kv_config);
	metacfg_adapter_config_file_load_part(&kv_config, "/etc/wwwctrl/metafile.dat", "dev_wan0_interface", "nat_dmz_enable");

	// system_default_route_interface=dev_wanX_interface
	value= metacfg_adapter_config_get_value(&kv_config, "system_default_route_interface", 1);
	if (strncmp(value, "dev_wan", 7) == 0)
		default_wanif_index = util_atoi(value+7);

	// brwan status
	for (i=0; i<BRWAN_INDEX_TOTAL; i++) {
		int is_en, vid, pbit, is_dscp2pbit, portmask, wifimask;

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

		snprintf(key, 128, "brwan%d_dscp2pbit", i);
		value = metacfg_adapter_config_get_value(&kv_config, key, 1);
		is_dscp2pbit = util_atoi(value);

		snprintf(key, 128, "brwan%d_portmask", i);
		value = metacfg_adapter_config_get_value(&kv_config, key, 1);
		portmask = util_atoi(value);

		snprintf(key, 128, "brwan%d_wifimask", i);
		value = metacfg_adapter_config_get_value(&kv_config, key, 1);
		wifimask = util_atoi(value);

		if (!is_brwan_header_printed) {
			is_brwan_header_printed = 1;
			util_fdprintf(fd, "%1s %-9s %-4s %-2s %-9s %-8s %-8s\n",
				"#", "brwan-vid", "pbit", "en", "dscp2pbit", "portmask", "wifimask");
		}
		util_fdprintf(fd, "%d %9u %4u %2u %9u     0x%02x     0x%02x\n", 
			i, vid, pbit, is_en, is_dscp2pbit, portmask, wifimask);
	}

	// wanif status
#ifdef RG_ENABLE
	util_fdprintf(fd, "%1s %-10s %-6s %-4s %-2s %-3s %-3s %-3s %-3s %-4s %-5s %-2s %-3s  %-17s\n",
		"#", "wanif", "hwintf", "auto", "en", "ip4", "ip6", "iph", "nat", "igmp", "exist", "up", "def", "hwaddr");
#else
	util_fdprintf(fd, "%1s %-10s %-4s %-2s %-3s %-3s %-3s %-3s %-4s %-5s %-2s %-3s  %-17s\n",
		"#", "wanif", "auto", "en", "ip4", "ip6", "iph", "nat", "igmp", "exist", "up", "def", "hwaddr");
#endif
	for (i=0; i<WANIF_INDEX_TOTAL; i++) {
		int is_auto, is_en, ipv4_enable, ipv6_enable, is_iphost, is_nat, is_igmp, is_exist, is_up;
		int assignment;
		char *ifname, *ifname2;
		int MAYBE_UNUSED hwintf;

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

		snprintf(key, 128, "wan%d_ip_enable", i);
		value = metacfg_adapter_config_get_value(&kv_config, key, 1);
		ipv4_enable = util_atoi(value);

		snprintf(key, 128, "wan%d_ipv6_enable", i);
		value = metacfg_adapter_config_get_value(&kv_config, key, 1);
		ipv6_enable = util_atoi(value);
			
		snprintf(key, 128, "wan%d_iphost_only", i);
		value = metacfg_adapter_config_get_value(&kv_config, key, 1);
		is_iphost = util_atoi(value);

		snprintf(key, 128, "wan%d_assignment", i);
		value = metacfg_adapter_config_get_value(&kv_config, key, 1);
		assignment = util_atoi(value);

		snprintf(key, 128, "wan%d_igmp_enable", i);
		value = metacfg_adapter_config_get_value(&kv_config, key, 1);
		is_igmp = util_atoi(value);

		snprintf(key, 128, "nat%d_enable", i);
		value = metacfg_adapter_config_get_value(&kv_config, key, 1);
		is_nat = util_atoi(value);
		is_exist = iphost_interface_is_exist(ifname);	// even interface doesn't exist, there might be still some info on metafile

		ifname2 = ifname;
		if (assignment == 2) 
			ifname2 = wanif_get_agent_pppname(ifname);
		is_up = 0;
		iphost_get_interface_updown(ifname2, &is_up);
#ifdef RG_ENABLE
		hwintf = wanif_get_hwintf_index_by_ifname(ifname);
		util_fdprintf(fd, "%d %-10s %5s%c %4d %2d %3d %3d %3d %3d %4d %5d %2d %3d",
			i, ifname, "",(hwintf>=0)?hwintf+'0':'-', is_auto, is_en, ipv4_enable, ipv6_enable, is_iphost, is_nat, is_igmp, is_exist, is_up, (i == default_wanif_index)?1:0);
#else
		util_fdprintf(fd, "%d %-10s %4d %2d %3d %3d %3d %3d %4d %5d %2d %3d",
			i, ifname, is_auto, is_en, ipv4_enable, ipv6_enable, is_iphost, is_nat, is_igmp, is_exist, is_up, (i == default_wanif_index)?1:0);
#endif
		if (is_exist) {
			unsigned char hwaddr[6];
			iphost_get_hwaddr(ifname, hwaddr);
			util_fdprintf(fd, "  %02x:%02x:%02x:%02x:%02x:%02x\n", hwaddr[0],hwaddr[1],hwaddr[2],hwaddr[3],hwaddr[4],hwaddr[5]);
		} else {
			util_fdprintf(fd, "  -\n");
		}
	}

	// wanif ip status
	util_fdprintf(fd, "%1s %-10s %-6s %s\n",
		"#", "wanif", "type", "addr");
	for (i=0; i<WANIF_INDEX_TOTAL; i++) {
		struct iphost_ifaddrs_t iph;
		char *ifname, *ifname2_ipv4, *ifname2_ipv6;

		snprintf(key, 128, "dev_wan%d_interface", i);
		ifname = metacfg_adapter_config_get_value(&kv_config, key, 1);
		if (strlen(ifname) == 0)
			continue;

		{ // get wanif info from pppoe/dhcp files or metafile.dat
			char *wan_type = "?";
			int assignment, ip_addr, netmask_addr, gw_addr, dns1_addr, dns2_addr;
			int masklen;
	
			wanif_get_info(ifname, &assignment, &ip_addr, &netmask_addr, &gw_addr, &dns1_addr, &dns2_addr);
			{
				struct in_addr ipv4_mask;
				ipv4_mask.s_addr = netmask_addr;
				masklen = iphost_get_masklen_ipv4(&ipv4_mask);
			}			
	
			ifname2_ipv4 = ifname;
			if (assignment == 0) {
				wan_type = "static";
			} else if (assignment == 1) {
				wan_type = "dhcp";
			} else if (assignment == 2) {
				wan_type = "pppoe";
				ifname2_ipv4 = wanif_get_agent_pppname(ifname);
				if (ifname2_ipv4 && iphost_interface_is_exist(ifname2_ipv4)) {
					wan_type = ifname2_ipv4;
				} else {
					// ppp interface doesn't exist, clear info returned from agent file
					ip_addr = netmask_addr = gw_addr = dns1_addr = dns2_addr = 0;
				}
			} else {
				wan_type = "?";
				ifname2_ipv4 = ifname;
			}
			util_fdprintf(fd, "%d %-10s %-6s ", i, ifname, wan_type);
			util_fdprintf(fd, "ipv4=%s/%d gw=%s", 
				iphost_inet_string(ip_addr),
				masklen,
				iphost_inet_string(gw_addr));
			if (dns1_addr)
				 util_fdprintf(fd, " dns1=%s", iphost_inet_string(dns1_addr));
			if (dns2_addr)
				 util_fdprintf(fd, " dns2=%s", iphost_inet_string(dns2_addr));
			 util_fdprintf(fd, "\n");
		}

		{ // get wanif info from pppoe/odhcp6c files or metafile.dat
			char *wan_type = "?";
			int ip6_assignment, masklen, prefix_masklen;
			struct in6_addr ip6_addr, ip6_prefix, ip6_gw, ip6_dns1, ip6_dns2;		
			char str[80];

			wanif_get_info6(ifname, &ip6_assignment, &ip6_addr, &masklen, &ip6_prefix, &prefix_masklen, &ip6_gw, &ip6_dns1, &ip6_dns2);
			ifname2_ipv6 = ifname;
			if (ip6_assignment == 0) {
				wan_type = "static";
			} else if (ip6_assignment == 1) {
				wan_type = "dhcp";
			} else if (ip6_assignment == 2) {
				wan_type = "pppoe";
				ifname2_ipv6 = wanif_get_agent_pppname6(ifname);
				if (ifname2_ipv6 && iphost_interface_is_exist(ifname2_ipv6)) {
					wan_type = ifname2_ipv6;
				} else {
					// ppp interface doesn't exist, clear info returned from agent file
					masklen = prefix_masklen = 0;
					set_ipv6_zero(&ip6_addr); set_ipv6_zero(&ip6_gw); set_ipv6_zero(&ip6_prefix);
					set_ipv6_zero(&ip6_dns1); set_ipv6_zero(&ip6_dns2);
				}
			} else {
				wan_type = "?";
				ifname2_ipv6 = ifname;
			}

			if (!is_ipv6_zero(&ip6_addr) || !is_ipv6_zero(&ip6_gw)) {
				util_fdprintf(fd, "%-1s %-10s %-6s ", "", "", wan_type);
				if (!is_ipv6_zero(&ip6_addr))
					util_fdprintf(fd, "ipv6=%s/%d ", util_inet_ntop(AF_INET6, &ip6_addr, str, sizeof(str)), masklen);
				if (!is_ipv6_zero(&ip6_gw))
					util_fdprintf(fd, "gw=%s ", util_inet_ntop(AF_INET6, &ip6_gw, str, sizeof(str)));
				if (!is_ipv6_zero(&ip6_prefix))
					util_fdprintf(fd, "pf=%s/%d ", util_inet_ntop(AF_INET6, &ip6_prefix, str, sizeof(str)), prefix_masklen);
				util_fdprintf(fd, "\n");
				if (!is_ipv6_zero(&ip6_dns1) || !is_ipv6_zero(&ip6_dns2)) {
					util_fdprintf(fd, "%-1s %-10s %-6s ", "", "", "");
					if (!is_ipv6_zero(&ip6_dns1))
						util_fdprintf(fd, "dns1=%s ", util_inet_ntop(AF_INET6, &ip6_dns1, str, sizeof(str)));
					if (!is_ipv6_zero(&ip6_dns2))
						util_fdprintf(fd, "dns2=%s ", util_inet_ntop(AF_INET6, &ip6_dns2, str, sizeof(str)));
					util_fdprintf(fd, "\n");
				}
			}
		}
		
		// current ipv4 addr on interface
		if (strlen(ifname2_ipv4) >0) {
			int n;
			// fetch info for ipv4/ipv6
			memset(&iph, 0, sizeof(iph));
			iphost_get_ifaddrs(ifname2_ipv4, &iph);
			for (n=0; n< iph.ipv4_total; n++) {
				char addr[80];
				int masklen = iphost_get_masklen_ipv4(&iph.in_netmask[n]);
				inet_ntop(AF_INET, &iph.in_addr[n], addr, sizeof(addr));
				util_fdprintf(fd, "%1s %-10s %-6s %s/%d\n",
					"", "", "ifcfg", addr, masklen);
			}
		}		 
		
		// current ipv6 addr on interface
		if (strlen(ifname2_ipv6) >0) {
			int n;			
			if (strcmp(ifname2_ipv6, ifname2_ipv4) != 0) {
				memset(&iph, 0, sizeof(iph));
				iphost_get_ifaddrs(ifname2_ipv6, &iph);
			}
			for (n=0; n< iph.ipv6_total; n++) {
				char addr[80];
				int masklen = iphost_get_masklen_ipv6(&iph.in6_netmask[n]);
				inet_ntop(AF_INET6, &iph.in6_addr[n], addr, sizeof(addr));
				util_fdprintf(fd, "%1s %-10s %-6s %s/%d\n",
					"", "",  "ifcfg", addr, masklen);
			}
		}		 
	}

	metacfg_adapter_config_release(&kv_config);
#endif
	return 0;
}
