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
 * File    : wanifo.c
 *
 ******************************************************************/

#ifndef __WANIF_H__
#define __WANIF_H__

#include "metacfg_adapter.h"

#define WANIF_INDEX_TOTAL	8
#define BRWAN_INDEX_TOTAL	8

/* wanif.c */
char *wanif_devname_base(char *devname);
int wanif_devname_vlan(char *devname);
#ifdef OMCI_METAFILE_ENABLE
int wanif_locate_index_by_devname(struct metacfg_t *kv_config, char *devname, char *kvname, int kvname_size);
int wanif_locate_index_by_devname_base(struct metacfg_t *kv_config, char *devname, char *kvname, int kvname_size);
int wanif_locate_index_by_vlan(struct metacfg_t *kv_config, int vlan, char *kvname, int kvname_size);
unsigned int wanif_get_indexmap_by_vlan(struct metacfg_t *kv_config, int vlan);
int wanif_locate_brwan_index_by_vlan(struct metacfg_t *kv_config, int vlan);
int wanif_locate_index_unused(struct metacfg_t *kv_config);
int wanif_locate_index_default_route(struct metacfg_t *kv_config);
int wanif_delete_by_index(struct metacfg_t *kv_config, int wanif_index);
int wanif_delete_brwan_by_index(struct metacfg_t *kv_config, int brwan_index);
int wanif_delete_auto_by_index(struct metacfg_t *kv_config, int wanif_index);
int wanif_get_replicated_indexmap(struct metacfg_t *kv_config);
#endif
char *wanif_get_agent_pppname(char *ifname);
char *wanif_get_agent_pppname6(char *ifname);
int wanif_get_agent_info(char *ifname, int *assignment, int *ip_addr, int *netmask_addr, int *gw_addr, int *dns1_addr, int *dns2_addr);
int wanif_get_agent_info6(char *ifname, int *assignment, struct in6_addr *ip6_addr, int *masklen, struct in6_addr *ip6_prefix, int *prefix_masklen, struct in6_addr *ip6_gw, struct in6_addr *ip6_dns1, struct in6_addr *ip6_dns2);
int wanif_get_info(char *ifname, int *assignment, int *ip_addr, int *netmask_addr, int *gw_addr, int *dns1_addr, int *dns2_addr);
int wanif_get_info6(char *ifname, int *assignment, struct in6_addr *ip6_addr, int *masklen, struct in6_addr *ip6_prefix, int *prefix_masklen, struct in6_addr *ip6_gw, struct in6_addr *ip6_dns1, struct in6_addr *ip6_dns2);
int wanif_get_hwintf_index_by_ifname(char *ifname);
int wanif_print_status(int fd);

#endif
