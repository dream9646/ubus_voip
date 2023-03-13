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
 * Module  : util
 * File    : iphost.h
 *
 ******************************************************************/

#ifndef __IPHOST_H__
#define __IPHOST_H__

struct iphost_ifaddrs_t {
	unsigned int if_flags;			// IFF_UP,IFF_RUNNING,IFF_BROADCAST,IFF_MULTICAST,IFF_POINTOPOINT,IFF_PROMISC,IFF_NOARP
	unsigned char ipv4_total;
	unsigned char ipv6_total;
	struct in_addr in_addr[8];		// 4
	struct in_addr in_netmask[8];
	struct in6_addr in6_addr[8];		// 16
	struct in6_addr in6_netmask[8];
};

struct iphost_nslookup_t {
	unsigned char ipv4_total;
	unsigned char ipv6_total;
	struct in_addr in_addr[8];		// 4
	struct in6_addr in6_addr[8];		// 16
};

/* iphost.c */
char *iphost_inet_string(int ip);
int iphost_set_hostname(char *hostname);
int iphost_get_hostname(char *hostname, int len);
int iphost_get_interface_list(char *ifname_list[], int ifname_list_size);
int iphost_interface_is_exist(char *ifname);
int iphost_set_interface_updown(char *ifname, int is_up);
int iphost_get_interface_updown(char *ifname, int *is_up);
int iphost_set_hwaddr(char *ifname, unsigned char *hwaddr);
int iphost_get_hwaddr(char *ifname, unsigned char *hwaddr);
int iphost_set_ipv4_ip_netmask(char *ifname, int ip_addr, int netmask_addr);
int iphost_get_ipv4_ip_netmask(char *ifname, int *ip_addr, int *netmask_addr, int *is_up);
int iphost_add_ipv4_route(int dstnet_addr, int genmask_addr, int gateway_addr);
int iphost_del_ipv4_route(int dstnet_addr, int genmask_addr);
int iphost_get_ipv4_route(int dstnet_addr, int genmask_addr, int *gateway_addr);
int iphost_set_ipv4_defgw(int gateway_addr);
int iphost_get_ipv4_defgw(int *gateway_addr);
int iphost_set_ipv4_dns(char *resolv_conf, char *search_domain, int dns1_addr, int dns2_addr);
int iphost_get_ipv4_dns(char *resolv_conf, int *dns1_addr, int *dns2_addr);
int iphost_get_ifaddrs(char *ifname, struct iphost_ifaddrs_t *iph);
int iphost_get_masklen_ipv4(struct in_addr *ipv4_mask);
int iphost_get_masklen_ipv6(struct in6_addr *ipv6_mask);
int iphost_get_ipv6_dns(char *resolv_conf, struct in6_addr *dns1_addr, struct in6_addr *dns2_addr);

#endif
