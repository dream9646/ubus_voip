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
 * File    : igmp_v3_lw.c
 *
 ******************************************************************/
 
#include <string.h>
#include "list.h"
#include "util.h"
#include "switch.h"
#include "mcast_const.h"
#include "mcast_pkt.h"
#include "igmp_mbship.h"
#include "mcast_switch.h"
#include "mcast_snoop.h"
#include "mcast.h"
#include "mcast_timer.h"
#include "igmp_omci.h"
#include "fwk_timer.h"
#include "mcast_proxy.h"
#include "mcast_router.h"

extern int timer_id_non_fast_leave_g;

static inline void igmp_v3_lw_free_host(struct igmp_mbship_t *host)
{
	host->lw_igmpv3_signature = 0;
        if (!list_node_is_poison_or_null(&host->entry_node_v3)) list_del(&host->entry_node_v3);
	if (!list_node_is_poison_or_null(&host->entry_node)) list_del(&host->entry_node);
        free_safe(host);
}

static struct igmp_v3_lw_mbship_t *igmp_v3_lw_locate_group(struct igmp_group_t *group)
{
        struct igmp_v3_lw_mbship_t *pos, *n;
        
        list_for_each_entry_safe(pos, n, &igmp_mbship_db_g.mcast_snoop_list, entry_node) {
                if (memcmp(&group->group_ip, &pos->group_ip, sizeof(union ipaddr)) == 0)
                        return pos;                               
	}
        
        return NULL;        
}

static struct igmp_v3_lw_mbship_t *igmp_v3_lw_create_group(struct igmp_group_t *group)
{
        struct igmp_v3_lw_mbship_t *g;
        
        g = malloc_safe(sizeof(struct igmp_v3_lw_mbship_t));
        if (g == NULL) return NULL;
        INIT_LIST_NODE(&g->entry_node);
        memcpy(&g->group_ip, &group->group_ip, sizeof(union ipaddr));
        INIT_LIST_HEAD(&g->src_list);
        INIT_LIST_HEAD(&g->host_asm_list);
        list_add(&g->entry_node, &igmp_mbship_db_g.mcast_snoop_list);
        
        return g;        
}

static struct igmp_v3_lw_src_t *igmp_v3_lw_locate_group_source(struct igmp_v3_lw_mbship_t *g, struct igmp_group_t *group)
{
        struct igmp_v3_lw_src_t *pos, *n;
        
        list_for_each_entry_safe(pos, n, &g->src_list, entry_node) {
                if (memcmp(&group->src_ip, &pos->src_ip, sizeof(union ipaddr)) == 0)
                        return pos;                               
	}
        
        return NULL;        
}

static struct igmp_v3_lw_src_t *igmp_v3_lw_create_group_source(struct igmp_v3_lw_mbship_t *g, struct igmp_group_t *group)
{
        struct igmp_v3_lw_src_t *s;
        
        if (group->src_ip.ipv4.s_addr == 0) return NULL;
        s = malloc_safe(sizeof(struct igmp_v3_lw_src_t));
        if (s == NULL) return NULL;
        memcpy(&s->src_ip, &group->src_ip, sizeof(union ipaddr));
        INIT_LIST_HEAD(&s->host_ssm_list);
        INIT_LIST_NODE(&s->entry_node);
        list_add(&s->entry_node, &g->src_list);
        
        return s;        
}

static struct igmp_mbship_t *igmp_v3_lw_create_host(struct igmp_clientinfo_t *igmp_clientinfo, struct igmp_mbship_t *h, struct igmp_group_t *group)
{
        memset(h, 0, sizeof(struct igmp_mbship_t));
         
	h->src_port_meid = igmp_clientinfo->rx_desc.bridge_port_me->meid;
	h->logical_port_id = igmp_clientinfo->rx_desc.logical_port_id;
	h->p2p_cfg = igmp_clientinfo->rx_desc.p2p_cfg;
	h->uni_vid = igmp_clientinfo->vid ;
	
	igmp_pkt_ip_cpy(&h->client_ip, &igmp_clientinfo->client_ip);
	igmp_pkt_ip_cpy(&h->group_ip, &group->group_ip);
        igmp_pkt_ip_cpy(&h->src_ip, &group->src_ip);
        //h->allowed_type = igmp_group->allowed_type;
	h->access_time = igmp_clientinfo->access_time;

	if (omci_env_g.igmp_proxy_snooping == 0) {
                memcpy(h->client_mac, igmp_clientinfo->client_mac, 6);
	}
        h->lw_igmpv3_signature = IGMP_V3_LW_SIGNATURE;        
        
        return h;        
}

/*
        This function translates IGMPv3 record types to types supported by lightweight IGMPv3 (RFC5790)
        Remove EXCLUDE filter-mode record types i.e. (*, G) join
        For more details, refer to sec 4.4 of RFC 5790 spec
        
*/ 
static int igmp_v3_lw_record_parser(struct igmp_clientinfo_t *igmp_clientinfo)
{
	struct igmp_group_t *pos, *n;
	
        list_for_each_entry_safe(pos, n, &igmp_clientinfo->group_list, entry_node) {
                // SSM doesnt allow (*,G) join/leave in SSM range 232.0.0.0/8
                if (pos->group_ip.ipv4.s_addr >= 0xE8000000 &&
                        pos->group_ip.ipv4.s_addr < 0xE9000000)
                {
                        if (pos->src_ip.ipv4.s_addr == 0)
                        {                       
                                if (pos->record_type == MODE_IS_EXCLUDE
                                || pos->record_type == CHANGE_TO_EXCLUDE_MODE
                                || pos->record_type == CHANGE_TO_INCLUDE_MODE)
                                {
                                        list_del(&pos->entry_node);
                                        free_safe(pos);
                                        igmp_clientinfo->group_num--;
                                        continue;        
                                }
                        }
                }
                
                // IS_EX(x) and TO_EX(x) arent supported
                if (pos->record_type == CHANGE_TO_EXCLUDE_MODE
                || pos->record_type == MODE_IS_EXCLUDE)
                {
                        if (pos->src_ip.ipv4.s_addr != 0)
                        {
                                list_del(&pos->entry_node);
                                free_safe(pos);
                                igmp_clientinfo->group_num--;
                                continue;
                        }
                }
                
                // treate IS_EX({}) as TO_EX({}) 
                if (pos->record_type == MODE_IS_EXCLUDE)
	               pos->record_type = CHANGE_TO_EXCLUDE_MODE;
	}
        
        return 0;
}
static int igmp_v3_lw_mbship_is_equal(struct igmp_mbship_t *a, struct igmp_mbship_t *b)
{

	if (igmp_pkt_ip_cmp(&a->client_ip, &b->client_ip) == 0
	       && igmp_pkt_ip_cmp(&a->group_ip, &b->group_ip) == 0 
	       //&& igmp_pkt_ip_cmp(&a->src_ip, &b->src_ip) == 0
	       && a->uni_vid == b->uni_vid
	       && a->logical_port_id == b->logical_port_id)
	       return 1;
	
        return 0;
}

static void igmp_v3_lw_source_host_list_del(struct igmp_v3_lw_src_t *s, struct igmp_mbship_t *h)
{
        struct igmp_mbship_t *pos, *n;
        
        list_for_each_entry_safe(pos, n, &s->host_ssm_list, entry_node_v3) {
                if (igmp_v3_lw_mbship_is_equal(pos, h))
                {
                        igmp_v3_lw_omci_active_group_list_remove(pos);
                        igmp_v3_lw_free_host(pos);
                        return;
                } 
        }                   
}

/*
        This function processes source list and take necessary action(querier or proxy)
        For more details, refer to sec 5.3 & 5.4 of RFC 5790 spec
        
*/

static void igmp_v3_lw_group_host_list_add(struct igmp_v3_lw_mbship_t *g, struct igmp_v3_lw_src_t *s, struct igmp_mbship_t *h)
{
        struct igmp_mbship_t *pos, *n;
       
        list_for_each_entry_safe(pos, n, &g->host_asm_list, entry_node_v3) {
                if (igmp_v3_lw_mbship_is_equal(pos, h))
                {
                        pos->access_time = h->access_time;  // existing entry
                        return;
                } 
        }
        
        struct igmp_mbship_t *new_host;
        new_host = malloc_safe(sizeof(struct igmp_mbship_t));
        memcpy(new_host, h, sizeof(struct igmp_mbship_t));
        INIT_LIST_NODE(&new_host->entry_node);
        INIT_LIST_NODE(&new_host->entry_node_v3);
        list_add(&new_host->entry_node_v3, &g->host_asm_list);      // new entry
        list_add(&new_host->entry_node, &igmp_mbship_db_g.mbship_list);
        igmp_v3_lw_omci_active_group_list_add(new_host);
        if(!h->ani_vid && new_host->ani_vid) h->ani_vid = new_host->ani_vid;
        // sanity check, remove host from all source list
        struct igmp_v3_lw_src_t *pos1, *n1;
        
        
        list_for_each_entry_safe(pos1, n1, &g->src_list, entry_node) {
                igmp_v3_lw_source_host_list_del(pos1, h);                              
	}                    
}

static void igmp_v3_lw_group_host_list_del(struct igmp_v3_lw_mbship_t *g, struct igmp_mbship_t *h)
{
        struct igmp_mbship_t *pos, *n;
        
        list_for_each_entry_safe(pos, n, &g->host_asm_list, entry_node_v3) {
                if (igmp_v3_lw_mbship_is_equal(pos, h))
                {
                        igmp_v3_lw_omci_active_group_list_remove(pos);
                        igmp_v3_lw_free_host(pos);
                        return;
                }    
        }                 
}

static void igmp_v3_lw_source_host_list_add(struct igmp_v3_lw_mbship_t *g, struct igmp_v3_lw_src_t *s, struct igmp_mbship_t *h)
{
        struct igmp_mbship_t *pos, *n;
       
        list_for_each_entry_safe(pos, n, &s->host_ssm_list, entry_node_v3) {
                if (igmp_v3_lw_mbship_is_equal(pos, h))
                {
                        pos->access_time = h->access_time;  // existing entry
                        return;
                } 
        }
        
        struct igmp_mbship_t *new_host;
        new_host = malloc_safe(sizeof(struct igmp_mbship_t));
        memcpy(new_host, h, sizeof(struct igmp_mbship_t));
        INIT_LIST_NODE(&new_host->entry_node);
        INIT_LIST_NODE(&new_host->entry_node_v3);
        list_add(&new_host->entry_node_v3, &s->host_ssm_list);      // new entry
        list_add(&new_host->entry_node, &igmp_mbship_db_g.mbship_list);
        igmp_v3_lw_omci_active_group_list_add(new_host);
        if(!h->ani_vid && new_host->ani_vid) h->ani_vid = new_host->ani_vid;
        // sanity check, remove host from group list
        igmp_v3_lw_group_host_list_del(g, h);                     
}

static void igmp_v3_lw_mbship_t_tidy(struct igmp_group_t *r)
{
        struct igmp_v3_lw_mbship_t *pos, *n;
        struct igmp_v3_lw_src_t *pos1, *n1;
        
        struct igmp_v3_lw_mbship_t *g = igmp_v3_lw_locate_group(r);
        if (g == NULL) return;
        
        list_for_each_entry_safe(pos, n, &igmp_mbship_db_g.mcast_snoop_list, entry_node) {
                if (memcmp(&g->group_ip, &pos->group_ip, sizeof(union ipaddr)) == 0)
                {
                        list_for_each_entry_safe(pos1, n1, &pos->src_list, entry_node) {

                                if (list_empty(&pos1->host_ssm_list))
                                {
                                        list_del(&pos1->entry_node);
                                        free_safe(pos1);        // remove source                                                
                                }
                        }
                                 
                        if (list_empty(&pos->host_asm_list) && list_empty(&pos->src_list))
                        {
                                list_del(&pos->entry_node);
                                free_safe(pos);         // remove group
                        }
                }                                
	}
        
}
 
static int igmp_v3_lw_record_processor(int record_type, struct igmp_v3_lw_mbship_t *g, struct igmp_v3_lw_src_t *s, struct igmp_mbship_t *h)
{
        switch(record_type)
        {
        case MODE_IS_INCLUDE: // source A -> A + B, (B) = GMI
                if (s == NULL)
                {
                        dbprintf_igmp(LOG_IGMPV3_LW,"query response to (*, G) leave\n");        // should never happen
                }
                else
                {
                        dbprintf_igmp(LOG_IGMPV3_LW,"query response to (x, G) join\n");
                        igmp_v3_lw_source_host_list_add(g, s, h);
                }
                break;
        case MODE_IS_EXCLUDE:   // source A -> A, G_Timer = GMI
                if (s == NULL)
                {
                        dbprintf_igmp(LOG_IGMPV3_LW,"query response to (*, G) join\n");
                        igmp_v3_lw_group_host_list_add(g, s, h);
                }
                else
                        dbprintf_igmp(LOG_IGMPV3_LW,"query response to EXCLUDE (x, G) join is not supported\n");
                break;
        case CHANGE_TO_INCLUDE_MODE: // source A -> A + B, Send Q(G, A-B), If G_Timer > 0, Send Q(G)
                if (s == NULL)
                {
                        dbprintf_igmp(LOG_IGMPV3_LW,"(*, G) leave\n");
                        if (igmp_config_g.immediate_leave)
                                igmp_v3_lw_group_host_list_del(g, h);
                        else
                        {
                                struct igmp_mbship_t *copy = malloc_safe(sizeof(struct igmp_mbship_t));
                                memcpy(copy, h, sizeof(struct igmp_mbship_t));
                                timer_id_non_fast_leave_g = fwk_timer_create(igmp_timer_qid_g, TIMER_EVENT_NONE_FAST_LEAVE, igmp_config_g.last_member_query_interval*100, (void *)copy);
                        }
                }
                else
                {
                        dbprintf_igmp(LOG_IGMPV3_LW,"change to INCLUDE (x, G) join\n");
                        igmp_v3_lw_source_host_list_add(g, s, h);
                }
                break;
        case CHANGE_TO_EXCLUDE_MODE:    // source A -> A, G_Timer = GMI
                if (s == NULL) // (*,G) join
                {
                        dbprintf_igmp(LOG_IGMPV3_LW,"(*, G) join\n");
                        igmp_v3_lw_group_host_list_add(g, s, h);
                }
                else
                        dbprintf_igmp(LOG_IGMPV3_LW,"change to EXCLUDE (x, G) join is not supported\n"); 
                break;
        case ALLOW_NEW_SOURCES: // soruce A -> A + B, (B) = GMI
                dbprintf_igmp(LOG_IGMPV3_LW,"INCLUDE(x,G) join\n");
                if (s == NULL) dbprintf_igmp(LOG_IGMPV3_LW,"INCLUDE(x,G) join contains no source\n"); 
                igmp_v3_lw_source_host_list_add(g, s, h);
                break;
        case BLOCK_OLD_SOURCES: // source A -> A, Send Q(G, A-B) 
                dbprintf_igmp(LOG_IGMPV3_LW,"INCLUDE(x,G) leave\n");
                if (s == NULL) dbprintf_igmp(LOG_IGMPV3_LW,"INCLUDE(x,G) leave contains no source\n");
                if (igmp_config_g.immediate_leave)
                        igmp_v3_lw_source_host_list_del(s,h);
                else
                {
                        struct igmp_mbship_t *copy = malloc_safe(sizeof(struct igmp_mbship_t));
                        memcpy(copy, h, sizeof(struct igmp_mbship_t));
                        timer_id_non_fast_leave_g = fwk_timer_create(igmp_timer_qid_g, TIMER_EVENT_NONE_FAST_LEAVE, igmp_config_g.last_member_query_interval*100, (void *)copy);
                }
                break;
        default:
                break;
        }
        
        return 0;             
}

/*
        This function forwards i.e. masks hw according to status of group/source timer
        For more details, refer to sec 5.1 & 5.2 of RFC 5790 spec
        
*/
static unsigned int igmp_v3_lw_get_host_portmask(struct list_head *list)
{
        struct igmp_mbship_t *pos, *n;
        unsigned int portmask = 0;
        
        list_for_each_entry_safe(pos, n, list, entry_node_v3) {
                portmask |= 1 << pos->logical_port_id;
        }
        
        return  portmask;     
}

static void igmp_v3_lw_mbship_t_change_mode(struct igmp_v3_lw_mbship_t *group, int mode)
{
        // ASM - mode > 0, SSM - mode < 0
        int i = 1;
        struct switch_mac_tab_entry_t mac_tab_entry;
        int num=0;
        
        do {
		memset(&mac_tab_entry, 0, sizeof(struct switch_mac_tab_entry_t) );
		if (switch_hw_g.mac_ipmc_get_next(&num, &mac_tab_entry ) != 1)
			break;

                if (memcmp(&mac_tab_entry.ipmcast.dst_ip, &group->group_ip, sizeof(union ipaddr)) == 0)
                {
                        if (mode)       // ASM remove all sip != 0.0.0.0
                        {
                                if (mac_tab_entry.ipmcast.src_ip.ipv4.s_addr != 0)
                                {
                                        switch_hw_g.ipmcastaddr_del(&mac_tab_entry);
                                        dbprintf_igmp(LOG_IGMPV3_LW,"remove sip!=0, %d\n", num);
                                }
                        }
                        else            // SSM remove sip == 0.0.0.0
                        {
                                if (mac_tab_entry.ipmcast.src_ip.ipv4.s_addr == 0)
                                {
                                        switch_hw_g.ipmcastaddr_del(&mac_tab_entry);
                                        dbprintf_igmp(LOG_IGMPV3_LW,"remove sip==0, %d\n", num);
                                }
                        }
                }
		num++;
		i++;

	} while (i<8192);	// mactab entry range 0..2047, use 8192 for safety, better than while (1)

        if (mode)       switch_hw_g.l2_impc_group_del(*(ipaddr_t *)&group->group_ip);
}

static void igmp_v3_lw_mbship_t_forward(struct igmp_v3_lw_mbship_t *group)
{
        struct igmp_v3_lw_src_t *pos, *n;
        struct switch_ipv4mcast_entry_t mac_entry;
        short bDontcaresrc = 0;
        
	if (igmp_switch_preassign_list_exist(&group->group_ip)) return;	// preassigned
	
	memset(&mac_entry, 0, sizeof(struct switch_mac_tab_entry_t));
	mac_entry.vid = group->vid;
	igmp_pkt_ip_cpy(&mac_entry.dst_ip, &group->group_ip);	
	
	mac_entry.src_portmask[0] = igmp_v3_lw_get_host_portmask(&group->host_asm_list);
	
	bDontcaresrc = (mac_entry.src_portmask[0])?1:0;
                	
        if (bDontcaresrc)
	{
                mac_entry.filter_mode = SWITCH_IPMC_MODE_ASM;
                list_for_each_entry_safe(pos, n, &group->src_list, entry_node) {
                        mac_entry.src_portmask[0] |= igmp_v3_lw_get_host_portmask(&pos->host_ssm_list);
                }
        }
        else
        {
               mac_entry.filter_mode = SWITCH_IPMC_MODE_SSM_INCLUDE;
	       int i = 0;
               list_for_each_entry_safe(pos, n, &group->src_list, entry_node) {
                        if (i > 8)      break;
                        if (list_empty(&pos->host_ssm_list)) break;       // ignore source list with no hosts
                        mac_entry.src_list[i] = pos->src_ip.ipv4.s_addr;
                        mac_entry.src_portmask[i] = igmp_v3_lw_get_host_portmask(&pos->host_ssm_list);
                        i++;     
               }              
        }

        if (switch_hw_g.ipv4mcastaddr_add != NULL)
                switch_hw_g.ipv4mcastaddr_add(&mac_entry);
        else
                dbprintf_igmp(LOG_IGMPV3_LW,"igmp_v3_lw_mbship_t_forward error");
                        
        igmp_v3_lw_mbship_t_change_mode(group, bDontcaresrc);
            
}

/*
        This function translates IGMPv2/IGMPv1 report/leave messages to IGMPv3 comptaible ones
        For more details, refer to sec 6.2.2 of RFC 5790 spec
        
*/
static void igmp_v3_lw_mbship_legacy_report_modify(struct list_head *list, int msg_type)
{
        struct igmp_group_t *pos, *n;
        int i = 0;
        
        list_for_each_entry_safe(pos, n, list, entry_node) {
                pos->record_type = msg_type;
                pos->src_ip.ipv4.s_addr = 0;    // null source
                i++;
        }
        dbprintf_igmp(LOG_IGMPV3_LW,"%d IGMP v1/v2 legacy report modified\n", i);
}

static void mcast_mbship_db_remove_member(struct igmp_mbship_t *m)
{
        struct igmp_group_t r;

        memset(&r, 0, sizeof(struct igmp_group_t));
        memcpy(&r.group_ip, &m->group_ip, sizeof(union ipaddr));   

        igmp_v3_lw_free_host(m);        // remove host member
        
        struct igmp_v3_lw_mbship_t *g = igmp_v3_lw_locate_group(&r);
                        
        igmp_v3_lw_mbship_t_forward(g); // update forwarding
        igmp_v3_lw_mbship_t_tidy(&r);                    
}

void igmp_v3_lw_mbship_report_sanity_check(struct igmp_clientinfo_t **igmp_clientinfo)
{
        // kicks in when IGMP_V3_BIT is set via OMCI
        if (igmp_config_g.igmp_version & IGMP_V3_BIT)
        {
                switch ((*igmp_clientinfo)->igmp_msg_type)
                {
                case IGMP_V1_MEMBERSHIP_REPORT: // TO_EX({})
                        (*igmp_clientinfo)->igmp_msg_type = IGMP_V3_MEMBERSHIP_REPORT;
                        igmp_v3_lw_mbship_legacy_report_modify(&(*igmp_clientinfo)->group_list, CHANGE_TO_EXCLUDE_MODE); 
                        break;
                case IGMP_V2_MEMBERSHIP_REPORT: // TO_EX({})
                        (*igmp_clientinfo)->igmp_msg_type = IGMP_V3_MEMBERSHIP_REPORT;
                        igmp_v3_lw_mbship_legacy_report_modify(&(*igmp_clientinfo)->group_list, CHANGE_TO_EXCLUDE_MODE);
                        break;
                case IGMP_V2_LEAVE_GROUP:       // TO_IN({})
                        (*igmp_clientinfo)->igmp_msg_type = IGMP_V3_MEMBERSHIP_REPORT;
                        igmp_v3_lw_mbship_legacy_report_modify(&(*igmp_clientinfo)->group_list, CHANGE_TO_INCLUDE_MODE);
                        break;
                default:
                        break;
                }
        }
}

static int igmp_v3_lw_2511b_filter(struct igmp_clientinfo_t *igmp_clientinfo)
{
	struct igmp_group_t *pos, *n;
	
        list_for_each_entry_safe(pos, n, &igmp_clientinfo->group_list, entry_node) {
         
                if (pos->record_type == ALLOW_NEW_SOURCES || pos->record_type == CHANGE_TO_EXCLUDE_MODE || pos->record_type == MODE_IS_EXCLUDE)
                        pos->record_type = CHANGE_TO_EXCLUDE_MODE;
                else
                        pos->record_type = CHANGE_TO_INCLUDE_MODE;
                        
                pos->src_ip.ipv4.s_addr = 0;    // null source
	}
        
        return 0;
}

static int igmp_v3_lw_record_type_join(int record_type, union ipaddr *src_ip)
{
        int ret = 0;
        
        switch(record_type)
        {
        case MODE_IS_INCLUDE: // source A -> A + B, (B) = GMI
                if (src_ip->ipv4.s_addr != 0) ret = 1;
                break;
        case MODE_IS_EXCLUDE:   // source A -> A, G_Timer = GMI
                if (src_ip->ipv4.s_addr == 0) ret = 1;
                break;
        case CHANGE_TO_INCLUDE_MODE: // source A -> A + B, Send Q(G, A-B), If G_Timer > 0, Send Q(G)
                if (src_ip->ipv4.s_addr != 0) ret = 1;
                break;
        case CHANGE_TO_EXCLUDE_MODE:    // source A -> A, G_Timer = GMI
                if (src_ip->ipv4.s_addr == 0) ret = 1;
                break;
        case ALLOW_NEW_SOURCES: // soruce A -> A + B, (B) = GMI
                if (src_ip->ipv4.s_addr != 0) ret = 1;
                break;
        case BLOCK_OLD_SOURCES: // source A -> A, Send Q(G, A-B) 
                break;
        default:
                break;
        }
        
	return ret;
}

static int igmp_v3_lw_max_group_filter(int record_type, struct igmp_mbship_t *h)
{ 
	if (!igmp_v3_lw_record_type_join(record_type, &h->src_ip)) return 0; // let through
	// iterates through host list

        struct igmp_v3_lw_mbship_t *pos, *n;
        int grp_count = 0;
        
        list_for_each_entry_safe(pos, n, &igmp_mbship_db_g.mcast_snoop_list, entry_node) {
                if (memcmp(&pos->group_ip, &h->group_ip, sizeof(union ipaddr)) == 0) return 0;	// exist already
		grp_count++;                               
	}
        
        if (igmp_config_g.group_limit && (grp_count + 1) > igmp_config_g.group_limit) return 1;	// exceeed group limit
	return 0;
}

static int igmp_v3_lw_max_group_ssm_filter(int record_type, struct igmp_mbship_t *h)
{
	if (!igmp_v3_lw_record_type_join(record_type, &h->src_ip)) return 0; // let through
        
        int src_count = 0;
        struct igmp_v3_lw_mbship_t *grp_pos, *grp_n;
        
        list_for_each_entry_safe(grp_pos, grp_n, &igmp_mbship_db_g.mcast_snoop_list, entry_node) {
                if (memcmp(&grp_pos->group_ip, &h->group_ip, sizeof(union ipaddr)) == 0)
                {
                	struct igmp_v3_lw_src_t *src_pos, *src_n;
                	list_for_each_entry_safe(src_pos, src_n, &grp_pos->src_list, entry_node) {
				if (memcmp(&src_pos->src_ip, &h->src_ip, sizeof(union ipaddr)) == 0) return 0;	// exisit already
				src_count++;       
                	}
                	break;	// found matching group
		}                   
	}
        
        if ((src_count + 1) > igmp_config_g.max_ssm_source) return 1;	// exceeed source limit
		
	return 0;
}

static int igmp_v3_lw_max_group_member_filter(int record_type, struct igmp_mbship_t *h)
{
	if (!igmp_v3_lw_record_type_join(record_type, &h->src_ip)) return 0; // let through

        int mem_count = 0;
        struct igmp_v3_lw_mbship_t *grp_pos, *grp_n;  
        
        list_for_each_entry_safe(grp_pos, grp_n, &igmp_mbship_db_g.mcast_snoop_list, entry_node) {
                if (memcmp(&grp_pos->group_ip, &h->group_ip, sizeof(union ipaddr)) == 0)
                {
                	struct igmp_v3_lw_src_t *src_pos, *src_n;
                	struct igmp_mbship_t *pos, *n;
                	
        	        list_for_each_entry_safe(pos, pos, &grp_pos->host_asm_list, entry_node_v3) {
                                if (memcmp(&pos->client_ip, &h->client_ip, sizeof(union ipaddr)) == 0) return 0;                                		
				mem_count++;
	                }	// ASM 
	                
                	list_for_each_entry_safe(src_pos, src_n, &grp_pos->src_list, entry_node) {
				
                        	list_for_each_entry_safe(pos, n, &src_pos->host_ssm_list, entry_node_v3) {
                                	if (memcmp(&pos->client_ip, &h->client_ip, sizeof(union ipaddr)) == 0) return 0;                                		
					mem_count++;			
                        	}
                	}	// SSM
		}                   
	}
	
        if ((mem_count + 1) > igmp_config_g.max_group_member) return 1;	// exceeed source limit
		
	return 0;
}

struct grec_t {
	struct list_head node;
	struct igmpv3_grec grec;
};

int igmp_v3_lw_pkt_repack(struct igmp_clientinfo_t *igmp_clientinfo)
{
	if (IS_IPV6_ADDR(igmp_clientinfo->client_ip)) return 0;	// don't do this to IPv6
	
	if (igmp_clientinfo->group_num == 0)	return -1;
	
	struct igmpv3_report *rpt = (struct igmpv3_report *)igmp_clientinfo->igmphdr;
	if (rpt->type != IGMP_V3_MEMBERSHIP_REPORT) return 0;	// only modify igmpv3 pkt, otherwise pass as it is
	
	struct list_head list;
	INIT_LIST_HEAD(&list); 
	
	struct igmp_group_t *pos, *n;
	struct grec_t *pos2, *n2;
	int ngrec = 0, old_ngrec = 0, grec_nsrcs = 0, i = 0;
			
	list_for_each_entry_safe(pos, n, &igmp_clientinfo->group_list, entry_node) {
		
		int new_grec = 1;
		list_for_each_entry_safe(pos2, n2, &list, node) {
			if (pos->group_ip.ipv4.s_addr == pos2->grec.grec_mca && pos->record_type == pos2->grec.grec_type)
			{
				new_grec = 0;
				if (pos->src_ip.ipv4.s_addr != 0)
				{
					pos2->grec.grec_src[pos2->grec.grec_nsrcs] = pos->src_ip.ipv4.s_addr;
					pos2->grec.grec_nsrcs++;
				}	
				break;	
			}
		}
		
		if (new_grec)
		{
			int size = sizeof(struct grec_t) + sizeof(__u32) * 15;		// total 16 sources max
			struct grec_t *tmp = malloc_safe(size);
			memset(tmp, 0, size);
			tmp->grec.grec_type = pos->record_type;
			tmp->grec.grec_mca = pos->group_ip.ipv4.s_addr;
			if (pos->src_ip.ipv4.s_addr != 0)
			{
				tmp->grec.grec_src[0] = pos->src_ip.ipv4.s_addr;
				tmp->grec.grec_nsrcs++;
			}
						 
			INIT_LIST_NODE(&tmp->node);
			list_add(&tmp->node, &list);
			ngrec++;	
		}	
	}
	
	// rebuild igmp packet
	old_ngrec = ntohs(rpt->ngrec);
	rpt->ngrec = htons(ngrec);
	int data_length = sizeof(struct igmpv3_report);
	struct igmpv3_grec *igmpgrec = &rpt->grec[0], *grec = igmpgrec;
	// clear original grec to avoid checksum error
	for(i=0; i<old_ngrec; i++) {
		grec_nsrcs += grec->grec_nsrcs;
		grec = ((void *) grec) + (sizeof(struct igmpv3_grec) + grec->grec_nsrcs*sizeof(__u32));
	}
	memset(igmpgrec, 0, sizeof(struct igmpv3_grec)*old_ngrec+grec_nsrcs*sizeof(__u32));
	list_for_each_entry_safe(pos2, n2, &list, node) {
		igmpgrec->grec_type = pos2->grec.grec_type;
		igmpgrec->grec_auxwords = pos2->grec.grec_auxwords;
		igmpgrec->grec_nsrcs = htons(pos2->grec.grec_nsrcs>15 ? 15 : pos2->grec.grec_nsrcs);
		igmpgrec->grec_mca = pos2->grec.grec_mca;
		memcpy(&igmpgrec->grec_src[0], &pos2->grec.grec_src[0], pos2->grec.grec_nsrcs * sizeof(__u32));
		data_length += sizeof(struct igmpv3_grec) + pos2->grec.grec_nsrcs * sizeof(__u32);
		igmpgrec = (struct igmpv3_grec *)((unsigned char *)igmpgrec + sizeof(struct igmpv3_grec) + igmpgrec->grec_nsrcs * sizeof(__u32));	
	}
	rpt->csum = 0;
	rpt->csum = inetchksum((u_short *)rpt, data_length);
	
	// free grec list
	list_for_each_entry_safe(pos2, n2, &list, node) {
		list_del(&pos2->node);
		free_safe(pos2);
	}
	
	return 0;	
}

int igmp_v3_lw_mbship_report_worker(struct igmp_clientinfo_t **igmp_clientinfo)
{                
        struct igmp_group_t *pos, *n;
        
        if (igmp_clientinfo == NULL || list_empty(&(*igmp_clientinfo)->group_list))
		return 0;
	
        if(igmp_v3_lw_omci_filter(*igmp_clientinfo)) return 0;  // run incoming report records through OMCI filter
        
        igmp_v3_lw_record_parser(*igmp_clientinfo);      // make group records RFC 5790 compatible
	        
	if (strcmp("2511b", CHIP_NAME) == 0)
        {
                dbprintf_igmp(LOG_IGMPV3_LW, "2511b does not support SSM (x, G), ASM(*, G) is always used\n");
                igmp_v3_lw_2511b_filter(*igmp_clientinfo);          
        }
                
        list_for_each_entry_safe(pos, n, &(*igmp_clientinfo)->group_list, entry_node) {
		if (!igmp_switch_preassign_list_exist(&pos->group_ip) && igmp_omci_check_is_in_static_acl(&pos->group_ip)) continue;
					                
                dbprintf_igmp(LOG_IGMPV3_LW,"process group-0x%x source:-0x%x\n", pos->group_ip.ipv4.s_addr, pos->src_ip.ipv4.s_addr);
                struct igmp_mbship_t h;
                igmp_v3_lw_create_host(*igmp_clientinfo, &h, pos);

		// check exceed group count & group member count & group ssm count
        	if (igmp_v3_lw_max_group_filter(pos->record_type, &h) ||  		// exceed group count
        		igmp_v3_lw_max_group_member_filter(pos->record_type, &h) || 	// exceed group member count
			igmp_v3_lw_max_group_ssm_filter(pos->record_type, &h))  	// exceed group ssm count
		{
			list_del(&pos->entry_node);
			if (pos) free_safe(pos);
			(*igmp_clientinfo)->group_num--;			
			continue;
		}
		                
                struct igmp_v3_lw_mbship_t *g = igmp_v3_lw_locate_group(pos);
                if (g == NULL) g = igmp_v3_lw_create_group(pos);
                
                struct igmp_v3_lw_src_t *s = igmp_v3_lw_locate_group_source(g, pos);
                if (s == NULL) s = igmp_v3_lw_create_group_source(g, pos);
				                                           
                igmp_v3_lw_record_processor(pos->record_type, g, s, &h);
                if(!g->vid && h.ani_vid) g->vid = h.ani_vid;
                igmp_v3_lw_mbship_t_forward(g);
                igmp_v3_lw_mbship_t_tidy(pos);       
        }
        
        int ret = igmp_v3_lw_pkt_repack((*igmp_clientinfo));
        if (ret) return 0;
        
        if ((*igmp_clientinfo)->pktinfo->rx_desc.router_mode && omci_env_g.igmp_proxy_snooping)
                igmp_v3_lw_acceptigmp((*igmp_clientinfo)->pktinfo);
        else        
                igmp_switch_frame_send(*igmp_clientinfo);
        
        return 0;
}

int igmp_mbship_lw_v3_db_timeout_update(long timer_interval)
{
        struct igmp_mbship_t *pos=NULL, *n=NULL;
        
        long current_time = igmp_timer_get_cur_time();
        
	list_for_each_entry_safe(pos, n, &igmp_mbship_db_g.mbship_list, entry_node) {
                if (pos->lw_igmpv3_signature != IGMP_V3_LW_SIGNATURE)
                        continue;
                if (current_time - pos->access_time > timer_interval) {
                        dbprintf_igmp(LOG_IGMPV3_LW,"timeout group-0x%x source:-0x%x host:-0x%x\n", pos->group_ip.ipv4.s_addr, pos->src_ip.ipv4.s_addr, pos->client_ip.ipv4.s_addr);
                        igmp_v3_lw_omci_active_group_list_remove(pos);
                        if (omci_env_g.igmp_proxy_snooping == 2)
			     igmp_v3_lw_router_member_update(pos);
			mcast_mbship_db_remove_member(pos);
                }
	}
	
        return 0;
}

static struct igmp_mbship_t *igmp_v3_lw_locate_host(struct igmp_mbship_t *dummy)
{
        struct igmp_mbship_t *pos= NULL,*n= NULL;
        
	list_for_each_entry_safe(pos, n, &igmp_mbship_db_g.mbship_list, entry_node)
        {
		if (pos->src_port_meid != dummy->src_port_meid)
			continue;
		if (pos->logical_port_id != dummy->logical_port_id)
			continue;
		if (pos->uni_vid != dummy->uni_vid)
			continue;
		if (igmp_pkt_ip_cmp(&pos->group_ip, &dummy->group_ip))
			continue;
		if (igmp_pkt_ip_cmp(&pos->client_ip, &dummy->client_ip))
			continue;
		if (igmp_pkt_ip_cmp(&pos->src_ip, &dummy->src_ip))
			continue;
		if (pos->lw_igmpv3_signature != IGMP_V3_LW_SIGNATURE)
                        continue;
		return pos;
	}
	
	return NULL;
}

int 
igmp_v3_lw_main_normal_leave_update(void *data_ptr)
{
        // locate real member from a dummy
	struct igmp_mbship_t *m = igmp_v3_lw_locate_host((struct igmp_mbship_t *)data_ptr);

        if (!m) return 0;       // member no longer exist
        
        dbprintf_igmp(LOG_IGMPV3_LW,"normal leave-0x%x source:-0x%x, host-0x%x\n", m->group_ip.ipv4.s_addr, m->src_ip.ipv4.s_addr, m->client_ip.ipv4.s_addr);
        
        igmp_v3_lw_omci_active_group_list_remove(m);
        if (omci_env_g.igmp_proxy_snooping == 2)
                igmp_v3_lw_router_member_update(m);
        mcast_mbship_db_remove_member(m);
        free_safe(data_ptr);   // free dummy
	
	return 0;
}

int igmp_v3_lw_mbship_db_deinit()
{
        struct igmp_v3_lw_mbship_t *pos, *n;
        struct igmp_v3_lw_src_t *pos1, *n1;
        struct igmp_mbship_t *pos2, *n2;
        
        list_for_each_entry_safe(pos, n, &igmp_mbship_db_g.mcast_snoop_list, entry_node) {
                list_for_each_entry_safe(pos1, n1, &pos->src_list, entry_node) {
                        list_for_each_entry_safe(pos2, n2, &pos1->host_ssm_list, entry_node_v3) {
                                igmp_v3_lw_free_host(pos2);      // free source hosts
                        }
                        list_del(&pos1->entry_node);
                        free_safe(pos1);      // free source
                }

                list_for_each_entry_safe(pos2, n2, &pos->host_asm_list, entry_node_v3) {
                        igmp_v3_lw_free_host(pos2);      // free group hosts
                }                
                list_del(&pos->entry_node);
                free_safe(pos);        // free group                          
	}
	
	return 0;
}
