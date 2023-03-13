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
 * Module  : igmp
 * File    : mcast_snoop.h
 *
 ******************************************************************/

#ifndef __IGMP_V3_LW_H__
#define __IGMP_V3_LW_H__


//#define IGMPV3_LW_DEBUG

#ifdef IGMPV3_LW_DEBUG
#define LOG_IGMPV3_LW LOG_ERR
#else
#define LOG_IGMPV3_LW LOG_WARNING
#endif 
#ifndef ipaddr_t
#define ipaddr_t unsigned int /* ipv4 address type */
#endif

struct igmp_v3_lw_src_t{
        union ipaddr src_ip;
        struct list_head host_ssm_list;
        struct list_head entry_node;    
};

struct igmp_v3_lw_mbship_t{
        short vid;
        union ipaddr group_ip;
        struct list_head host_asm_list;
        struct list_head src_list;
        struct list_head entry_node;    
};

int igmp_v3_lw_mbship_db_deinit();
int igmp_mbship_lw_v3_db_timeout_update(long timer_interval);
int igmp_v3_lw_mbship_report_worker(struct igmp_clientinfo_t **igmp_clientinfo);
void igmp_v3_lw_mbship_report_sanity_check(struct igmp_clientinfo_t **igmp_clientinfo);
int igmp_v3_lw_omci_filter(struct igmp_clientinfo_t *igmp_clientinfo);
int igmp_v3_lw_omci_active_group_list_add(struct igmp_mbship_t *h);
int igmp_v3_lw_omci_active_group_list_remove(struct igmp_mbship_t *h);
int igmp_omci_load_acl_table(struct list_head *acl_list, struct attr_table_header_t *tab_header);
int igmp_v3_lw_main_normal_leave_update(void *data_ptr);
#endif
