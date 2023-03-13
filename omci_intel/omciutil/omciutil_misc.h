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
 * Module  : omciutil
 * File    : omciutil_misc.h
 *
 ******************************************************************/

#ifndef __OMCIUTIL_MISC_H__
#define __OMCIUTIL_MISC_H__

#include "metacfg_adapter.h"

/* omciutil_misc.c */
int omciutil_misc_determine_port_interface(struct me_t *port_me, char *ifname);
int omciutil_misc_get_ts_neighbor_list(struct me_t *ts_me, struct me_t *ts_neighbor_list[], int list_size, int include_myself);
int omciutil_misc_locate_ts_neighbor_by_pq_priority(int pq_priority, struct me_t *ts_me, struct me_t **ts_neighbor);
int omciutil_misc_get_pbit2gemport_map_by_bport_me(struct me_t *bridge_port_me, int pbit2gemport_map[8]);
int omciutil_misc_get_gem_me_list_by_bport_me(struct me_t *bridge_port_me, struct me_t *gemport_list[8]);
int omciutil_misc_get_bport_me_list_by_gemport(int gemport, struct me_t *bport_me_list[8]);
int omciutil_misc_traffic_management_option_is_rate_control(void);
int omciutil_get_pq_pointer_for_gem_ctp(struct me_t *gem_me, int is_ratectrl_mode);
int omciutil_misc_most_idle_pq_under_same_tcont(struct me_t *me, unsigned short tcont_meid);
int omciutil_misc_reassign_pq_for_gem_ctp_in_rate_control_mode(struct me_t *gem_me);
struct me_t *omciutil_misc_find_pptp_related_veip(struct me_t *pptp);
struct me_t *omciutil_misc_find_me_related_bport(struct me_t *me);
int omciutil_misc_me_igmp_us_tci_tag_control_collect(struct me_t *wanif_me, unsigned short us_tci[], unsigned char us_tag_control[], int size);
int omciutil_misc_me_rg_tag_ani_tag_collect(struct me_t *wanif_me, unsigned short rg_tag_list[], unsigned short ani_tag_list[], int size);
int omciutil_misc_tag_list_lookup(unsigned short tag, int tag_list_total, unsigned short tag_list[]);
int omciutil_misc_get_veip_rg_tag_by_ani_tag(struct me_t *veip, unsigned short ani_tag);
int omciutil_misc_get_devname_by_veip_ani_tag(struct me_t *veip, unsigned short ani_tag, char *devname, int size);
struct me_t *omciutil_misc_get_classid_me_by_devname_base(int classid, char *devname);
int omciutil_misc_get_rg_tag_list_index_by_devname_vlan(unsigned short rg_tag_list[], int list_size, char *devname);
#ifdef OMCI_METAFILE_ENABLE
int omciutil_misc_locate_wan_new_devname_index(struct metacfg_t *kv_config, char *prefix);
#endif
char *omciutil_misc_get_uniname_by_logical_portid(unsigned int logical_portid);
#endif
