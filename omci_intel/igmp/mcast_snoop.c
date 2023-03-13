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
 * File    : mcast_snoop.c
 *
 ******************************************************************/
 
#include <string.h>
#include "list.h"
#include "util.h"
#include "mcast_const.h"
#include "igmp_mbship.h"
#include "mcast.h"
#include "mcast_timer.h"
#include "fwk_timer.h"
#include "mcast_proxy.h"
#include "mcast_router.h"
#include "mcast_snoop.h"
#include "mcast_cli.h"
#include "platform.h"

extern int timer_id_query_interval_g;
static int query_resp_timeout_id =0;
static int timer_id_non_fast_leave_g = 0;
int timer_id_router_non_fast_leave_g = 0;

struct igmp_mbship_db_t igmp_mbship_db_g;

#define NOT_ZERO_ADDR(ip) ((ip).ipv6.s6_addr32[0] | (ip).ipv6.s6_addr32[1] | (ip).ipv6.s6_addr32[2] | (ip).ipv6.s6_addr32[3])

static int mcast_mbship_db_event(short event, struct mcast_mbship_t *h)
{
	if (igmp_mbship_db_g.mcast_mbship_db_event) igmp_mbship_db_g.mcast_mbship_db_event(event, h);
	
	return 0;
}

static int
mcast_mbship_db_init()
{
	INIT_LIST_HEAD(&igmp_mbship_db_g.mbship_list);
	INIT_LIST_HEAD(&igmp_mbship_db_g.mcast_snoop_list);
	INIT_LIST_HEAD(&igmp_mbship_db_g.mcast_proxy_list);
	igmp_mbship_db_g.mcast_mbship_db_event = NULL;
	return 0;
}

static inline void mcast_snoop_free_host(struct mcast_mbship_t *host)
{
        if (!list_node_is_poison_or_null(&host->entry_node_v3)) list_del(&host->entry_node_v3);
	if (!list_node_is_poison_or_null(&host->entry_node)) list_del(&host->entry_node);
        free_safe(host);
}

static struct mcast_snoop_grp_t *mcast_snoop_locate_group(struct igmp_group_t *group)
{
        struct mcast_snoop_grp_t *pos, *n;
        
        list_for_each_entry_safe(pos, n, &igmp_mbship_db_g.mcast_snoop_list, entry_node) {
                if (mcast_pkt_ip_cmp(&group->group_ip, &pos->group_ip) == 0)
                        return pos;                               
	}
        
        return NULL;        
}

static struct mcast_snoop_grp_t *mcast_snoop_create_group(struct igmp_group_t *group)
{
        struct mcast_snoop_grp_t *g;
        
        g = malloc_safe(sizeof(struct mcast_snoop_grp_t));
        if (g == NULL) return NULL;
        INIT_LIST_NODE(&g->entry_node);
        mcast_pkt_ip_cpy(&g->group_ip, &group->group_ip);
        INIT_LIST_HEAD(&g->src_list);
        INIT_LIST_HEAD(&g->host_asm_list);
        list_add(&g->entry_node, &igmp_mbship_db_g.mcast_snoop_list);
        
        return g;        
}

static struct mcast_snoop_src_t *mcast_snoop_locate_group_source(struct mcast_snoop_grp_t *g, struct igmp_group_t *group)
{
        struct mcast_snoop_src_t *pos, *n;
        
        list_for_each_entry_safe(pos, n, &g->src_list, entry_node) {
                if (mcast_pkt_ip_cmp(&group->src_ip, &pos->src_ip) == 0)
                        return pos;                               
	}
        
        return NULL;        
}

static struct mcast_snoop_src_t *mcast_snoop_create_group_source(struct mcast_snoop_grp_t *g, struct igmp_group_t *group)
{
        struct mcast_snoop_src_t *s;
        
        if (!NOT_ZERO_ADDR(group->src_ip)) return NULL; 
        s = malloc_safe(sizeof(struct mcast_snoop_src_t));
        if (s == NULL) return NULL;
        mcast_pkt_ip_cpy(&s->src_ip, &group->src_ip);
        INIT_LIST_HEAD(&s->host_ssm_list);
        INIT_LIST_NODE(&s->entry_node);
        list_add(&s->entry_node, &g->src_list);
        
        return s;        
}

static struct mcast_mbship_t *mcast_snoop_create_host(struct igmp_clientinfo_t *igmp_clientinfo, struct mcast_mbship_t *h, struct igmp_group_t *group)
{
        memset(h, 0, sizeof(struct mcast_mbship_t));
         
	M_SET_SRC_PORTID(h->src_port_id, igmp_clientinfo)
	h->logical_port_id = igmp_clientinfo->rx_desc.logical_port_id;
	h->hw_port_mask = h->logical_port_id; 
	h->p2p_cfg = igmp_clientinfo->rx_desc.p2p_cfg;
	h->uni_vid = igmp_clientinfo->vid;
	h->ani_vid = igmp_clientinfo->ani_vid;
	
	mcast_pkt_ip_cpy(&h->client_ip, &igmp_clientinfo->client_ip);
	mcast_pkt_ip_cpy(&h->group_ip, &group->group_ip);
        mcast_pkt_ip_cpy(&h->src_ip, &group->src_ip);
        //h->allowed_type = igmp_group->allowed_type;
	h->access_time = igmp_clientinfo->access_time;

	if (THREAD_ENV.igmp_proxy_snooping == 0) {
                memcpy(h->client_mac, igmp_clientinfo->client_mac, 6);
	}     
        
        return h;        
}

/*
        This function translates IGMPv3 record types to types supported by lightweight IGMPv3 (RFC5790)
        Remove EXCLUDE filter-mode record types i.e. (*, G) join
        For more details, refer to sec 4.4 of RFC 5790 spec
        
*/ 
static int mcast_snoop_record_parser(struct igmp_clientinfo_t *igmp_clientinfo)
{
	struct igmp_group_t *pos, *n;
	
        list_for_each_entry_safe(pos, n, &igmp_clientinfo->group_list, entry_node) {
        	if (IS_IPV6_ADDR(pos->group_ip))
        	{	// TODO
		}
		else
		{
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
		}
		
                
                // IS_EX(x) and TO_EX(x) arent supported
                if (pos->record_type == CHANGE_TO_EXCLUDE_MODE || pos->record_type == MODE_IS_EXCLUDE)
                {
                        if (NOT_ZERO_ADDR(pos->src_ip))
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
static int mcast_snoop_mbship_is_equal(struct mcast_mbship_t *a, struct mcast_mbship_t *b)
{

	if (mcast_pkt_ip_cmp(&a->client_ip, &b->client_ip) == 0
	       && mcast_pkt_ip_cmp(&a->group_ip, &b->group_ip) == 0 
	       //&& mcast_pkt_ip_cmp(&a->src_ip, &b->src_ip) == 0
	       && a->uni_vid == b->uni_vid
	       && a->logical_port_id == b->logical_port_id)
	       return 1;
	
        return 0;
}

static void mcast_snoop_source_host_list_del(struct mcast_snoop_src_t *s, struct mcast_mbship_t *h)
{
        struct mcast_mbship_t *pos, *n;
        
        list_for_each_entry_safe(pos, n, &s->host_ssm_list, entry_node_v3) {
                if (mcast_snoop_mbship_is_equal(pos, h))
                {
                        mcast_mbship_db_event(IGMP_MEMBER_EVENT_DEL, pos);
                        mcast_snoop_free_host(pos);
                        return;
                } 
        }                   
}

/*
        This function processes source list and take necessary action(querier or proxy)
        For more details, refer to sec 5.3 & 5.4 of RFC 5790 spec
        
*/

static void mcast_snoop_group_host_list_add(struct mcast_snoop_grp_t *g, struct mcast_snoop_src_t *s, struct mcast_mbship_t *h)
{
        struct mcast_mbship_t *pos, *n;
       
        list_for_each_entry_safe(pos, n, &g->host_asm_list, entry_node_v3) {
                if (mcast_snoop_mbship_is_equal(pos, h))
                {
                        pos->access_time = h->access_time;  // existing entry
                        return;
                } 
        }
        
        struct mcast_mbship_t *new_host;
        new_host = malloc_safe(sizeof(struct mcast_mbship_t));
        memcpy(new_host, h, sizeof(struct mcast_mbship_t));
        INIT_LIST_NODE(&new_host->entry_node);
        INIT_LIST_NODE(&new_host->entry_node_v3);
        list_add(&new_host->entry_node_v3, &g->host_asm_list);      // new entry
        list_add(&new_host->entry_node, &igmp_mbship_db_g.mbship_list);
        mcast_mbship_db_event(IGMP_MEMBER_EVENT_ADD, new_host);
        if(!h->ani_vid && new_host->ani_vid) h->ani_vid = new_host->ani_vid;
                
        // sanity check, remove host from all source list
        struct mcast_snoop_src_t *pos1, *n1;
        
        
        list_for_each_entry_safe(pos1, n1, &g->src_list, entry_node) {
                mcast_snoop_source_host_list_del(pos1, h);                              
	}                    
}

static void mcast_snoop_group_host_list_del(struct mcast_snoop_grp_t *g, struct mcast_mbship_t *h)
{
        struct mcast_mbship_t *pos, *n;
        
        list_for_each_entry_safe(pos, n, &g->host_asm_list, entry_node_v3) {
                if (mcast_snoop_mbship_is_equal(pos, h))
                {
                        mcast_mbship_db_event(IGMP_MEMBER_EVENT_DEL, pos);
                        mcast_snoop_free_host(pos);
                        return;
                }    
        }                 
}

static void mcast_snoop_source_host_list_add(struct mcast_snoop_grp_t *g, struct mcast_snoop_src_t *s, struct mcast_mbship_t *h)
{
        struct mcast_mbship_t *pos, *n;
       
        list_for_each_entry_safe(pos, n, &s->host_ssm_list, entry_node_v3) {
                if (mcast_snoop_mbship_is_equal(pos, h))
                {
                        pos->access_time = h->access_time;  // existing entry
                        return;
                } 
        }
        
        struct mcast_mbship_t *new_host;
        new_host = malloc_safe(sizeof(struct mcast_mbship_t));
        memcpy(new_host, h, sizeof(struct mcast_mbship_t));
        INIT_LIST_NODE(&new_host->entry_node);
        INIT_LIST_NODE(&new_host->entry_node_v3);
        list_add(&new_host->entry_node_v3, &s->host_ssm_list);      // new entry
        list_add(&new_host->entry_node, &igmp_mbship_db_g.mbship_list);
        mcast_mbship_db_event(IGMP_MEMBER_EVENT_ADD, h);
        if(!h->ani_vid && new_host->ani_vid) h->ani_vid = new_host->ani_vid;
        
        // sanity check, remove host from group list
        mcast_snoop_group_host_list_del(g, h);                     
}

static void mcast_snoop_mbship_t_tidy(struct igmp_group_t *r)
{
        struct mcast_snoop_grp_t *pos, *n;
        struct mcast_snoop_src_t *pos1, *n1;
        
        if (r == NULL)
		return;
        
        list_for_each_entry_safe(pos, n, &igmp_mbship_db_g.mcast_snoop_list, entry_node) {
                if (mcast_pkt_ip_cmp(&r->group_ip, &pos->group_ip) == 0)
                {
                        list_for_each_entry_safe(pos1, n1, &pos->src_list, entry_node) {

                                if (list_empty(&pos1->host_ssm_list))
                                {
                                        list_del(&pos1->entry_node);
                                        free_safe(pos1);        // remove source                                                
                                }
                        }
                                 
                        if (list_empty(&pos->host_asm_list)
                        && list_empty(&pos->src_list))
                        {
                                list_del(&pos->entry_node);
                                free_safe(pos);         // remove group
                        }
                }                                
	}
        
}
 
static int mcast_snoop_record_processor(int record_type, struct mcast_snoop_grp_t *g, struct mcast_snoop_src_t *s, struct mcast_mbship_t *h)
{
        switch (record_type)
        {
        case MODE_IS_INCLUDE: // source A -> A + B, (B) = GMI
                if (s == NULL)
                {
                        dbprintf_igmp(LOG_INFO, "query response to (*, G) leave\n");        // should never happen
                }
                else
                {
                        dbprintf_igmp(LOG_INFO, "query response to (x, G) join\n");
                        mcast_snoop_source_host_list_add(g, s, h);
                }
                break;
        case MODE_IS_EXCLUDE:   // source A -> A, G_Timer = GMI
                if (s == NULL)
                {
                        dbprintf_igmp(LOG_INFO, "query response to (*, G) join\n");
                        mcast_snoop_group_host_list_add(g, s, h);
                }
                else
                        dbprintf_igmp(LOG_INFO, "query response to EXCLUDE (x, G) join is not supported\n");
                break;
        case CHANGE_TO_INCLUDE_MODE: // source A -> A + B, Send Q(G, A-B), If G_Timer > 0, Send Q(G)
                if (s == NULL)
                {
                        dbprintf_igmp(LOG_INFO, "(*, G) leave\n");
                        if (igmp_config_g.immediate_leave)
                                mcast_snoop_group_host_list_del(g, h);
                        else
                        {
                                struct mcast_mbship_t *copy = malloc_safe(sizeof(struct mcast_mbship_t));
                                memcpy(copy, h, sizeof(struct mcast_mbship_t));
                                timer_id_non_fast_leave_g = fwk_timer_create(igmp_timer_qid_g, TIMER_EVENT_NONE_FAST_LEAVE, igmp_config_g.last_member_query_interval*100, (void *)copy);
                        }
                }
                else
                {
                        dbprintf_igmp(LOG_INFO, "change to INCLUDE (x, G) join\n");
                        mcast_snoop_source_host_list_add(g, s, h);
                }
                break;
        case CHANGE_TO_EXCLUDE_MODE:    // source A -> A, G_Timer = GMI
                if (s == NULL) // (*,G) join
                {
                        dbprintf_igmp(LOG_INFO, "(*, G) join\n");
                        mcast_snoop_group_host_list_add(g, s, h);
                }
                else
                        dbprintf_igmp(LOG_INFO, "change to EXCLUDE (x, G) join is not supported\n"); 
                break;
        case ALLOW_NEW_SOURCES: // soruce A -> A + B, (B) = GMI
                if (s != NULL)
                {
                	dbprintf_igmp(LOG_INFO, "INCLUDE(x,G) join\n");
                	mcast_snoop_source_host_list_add(g, s, h);
                }
                else
                	 dbprintf_igmp(LOG_INFO, "INCLUDE(x,G) join contains no source\n");
                break;
        case BLOCK_OLD_SOURCES: // source A -> A, Send Q(G, A-B) 
                if (s == NULL)
			dbprintf_igmp(LOG_INFO, "INCLUDE(x,G) leave contains no source\n");
		else
		{
			dbprintf_igmp(LOG_INFO, "INCLUDE(x,G) leave\n");
	                if (igmp_config_g.immediate_leave)
	                        mcast_snoop_source_host_list_del(s,h);
	                else
	                {
	                        struct mcast_mbship_t *copy = malloc_safe(sizeof(struct mcast_mbship_t));
	                        memcpy(copy, h, sizeof(struct mcast_mbship_t));
	                        timer_id_non_fast_leave_g = fwk_timer_create(igmp_timer_qid_g, TIMER_EVENT_NONE_FAST_LEAVE, igmp_config_g.last_member_query_interval*100, (void *)copy);
	                }		
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
static unsigned int mcast_snoop_get_host_portmask(struct list_head *list)
{
        struct mcast_mbship_t *pos, *n;
        unsigned int portmask = 0;
        
        list_for_each_entry_safe(pos, n, list, entry_node_v3) {
        	FILTER_EXECUTE(forward, pos)
        	if (pos->hw_port_mask == MCAST_FILTER_INVALID_MASK) continue;
        	dbprintf_igmp(LOG_INFO, "update %s to gip %s @ %d to %d\n", inet_ipaddr(pos->client_ip, s1), inet_ipaddr(pos->group_ip, s2), (1<<pos->logical_port_id), pos->hw_port_mask);
                portmask |= pos->hw_port_mask;
        }
        
        return  portmask;     
}

static void mcast_snoop_grp_t_change_mode(struct mcast_snoop_grp_t *group, int mode)
{
        // ASM - mode > 0, SSM - mode < 0
        int i = 1;
        struct switch_mac_tab_entry_t mac_tab_entry;
        int num=0;
        
        do {
		memset(&mac_tab_entry, 0, sizeof(struct switch_mac_tab_entry_t) );
		if (switch_hw_g.mac_ipmc_get_next(&num, &mac_tab_entry ) != 1)
			break;

                if (mcast_pkt_ip_cmp(&mac_tab_entry.ipmcast.dst_ip, &group->group_ip) == 0)
                {
                        if (mode)       // ASM remove all sip != 0.0.0.0
                        {
                                if (mac_tab_entry.ipmcast.src_ip.ipv4.s_addr != 0)
                                {
                                        switch_hw_g.ipmcastaddr_del(&mac_tab_entry);
                                        dbprintf_igmp(LOG_INFO,"remove sip!=0, %d\n", num);
                                }
                        }
                        else            // SSM remove sip == 0.0.0.0
                        {
                                if (mac_tab_entry.ipmcast.src_ip.ipv4.s_addr == 0)
                                {
                                        switch_hw_g.ipmcastaddr_del(&mac_tab_entry);
                                        dbprintf_igmp(LOG_INFO,"remove sip==0, %d\n", num);
                                }
                        }
                }
		num++;
		i++;

	} while (i<8192);	// mactab entry range 0..2047, use 8192 for safety, better than while (1)

        if (mode)       switch_hw_g.l2_impc_group_del(*(ipaddr_t *)&group->group_ip);
}

static void mcast_snoop_switch_forward6(struct mcast_snoop_grp_t *group)
{
	struct switch_mac_tab_entry_t mac_entry;
	
	if (group->flag & (1 << NO_IGMP_SWITCH_FORWARD)) return;
		
	memset(&mac_entry, 0, sizeof(struct switch_mac_tab_entry_t));
	mac_entry.vid = group->vid;

        struct mcast_snoop_grp_t *pos, *n;
        
        list_for_each_entry_safe(pos, n, &igmp_mbship_db_g.mcast_snoop_list, entry_node) {
        	if (pos->group_ip.ipv6.s6_addr32[3] != group->group_ip.ipv6.s6_addr32[3]) continue; 
		// compare MAC address that transform from IPv6 - one to many
                mac_entry.port_bitmap |= mcast_snoop_get_host_portmask(&group->host_asm_list);            
	}
		
	mcast_pkt_ip_cpy(&mac_entry.ipmcast.dst_ip, &group->group_ip);
	
	if (mac_entry.port_bitmap)
	{
		if (switch_hw_g.ipmcastaddr_add) switch_hw_g.ipmcastaddr_add(&mac_entry);
	}
	else
	{
		if (switch_hw_g.ipmcastaddr_del) switch_hw_g.ipmcastaddr_del(&mac_entry);
	}	
}

static void mcast_snoop_switch_forward(struct mcast_snoop_grp_t *group)
{
        struct mcast_snoop_src_t *pos, *n;
        struct switch_ipv4mcast_entry_t mac_entry;
        short bDontcaresrc = 0;
	
	if (group->flag & (1 << NO_IGMP_SWITCH_FORWARD))
		return;
	
	memset(&mac_entry, 0, sizeof(struct switch_mac_tab_entry_t));
	mac_entry.vid = group->vid;
	mcast_pkt_ip_cpy(&mac_entry.dst_ip, &group->group_ip);	
	
	mac_entry.src_portmask[0] = mcast_snoop_get_host_portmask(&group->host_asm_list);
	
	bDontcaresrc = (mac_entry.src_portmask[0])?1:0;
                	
        if (bDontcaresrc)
	{
                mac_entry.filter_mode = SWITCH_IPMC_MODE_ASM;
                list_for_each_entry_safe(pos, n, &group->src_list, entry_node) {
                        mac_entry.src_portmask[0] |= mcast_snoop_get_host_portmask(&pos->host_ssm_list);
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
                        mac_entry.src_portmask[i] = mcast_snoop_get_host_portmask(&pos->host_ssm_list);
                        i++;     
               }              
        }

#ifdef RG_ENABLE        
        if (switch_hw_g.rg_ipv4mcastaddr_add != NULL)
                switch_hw_g.rg_ipv4mcastaddr_add(&mac_entry);
        else
#else   
        if (switch_hw_g.ipv4mcastaddr_add != NULL)
                switch_hw_g.ipv4mcastaddr_add(&mac_entry);
        else
#endif
        dbprintf_igmp(LOG_WARNING,"mcast_snoop_switch_forward error");
                        
        mcast_snoop_grp_t_change_mode(group, bDontcaresrc);
            
}

/*
        This function translates IGMPv2/IGMPv1/MLD report/leave messages to IGMPv3/MLDv2 comptaible ones
        For more details, refer to sec 6.2.2 of RFC 5790 spec
        
*/
static void mcast_snoop_legacy_report_modify(struct list_head *list, int msg_type)
{
        struct igmp_group_t *pos, *n;
        int i = 0;
        
        list_for_each_entry_safe(pos, n, list, entry_node) {
                pos->record_type = msg_type;
                memset(&pos->src_ip, 0, sizeof(union ipaddr)); // null source
                i++;
        }
        dbprintf_igmp(LOG_INFO, "%d IGMP v1/v2 legacy report modified\n", i);
}

/* static */ void mcast_mbship_db_remove_member(struct mcast_mbship_t *m)
{
        struct igmp_group_t r;

        memset(&r, 0, sizeof(struct igmp_group_t));
        mcast_pkt_ip_cpy(&r.group_ip, &m->group_ip);   
        
        struct mcast_snoop_grp_t *g = mcast_snoop_locate_group(&r);
        if(!g->vid && m->ani_vid) g->vid = m->ani_vid;
        mcast_snoop_free_host(m);        // remove host member
                        
        (IS_IPV6_ADDR(r.group_ip))?mcast_snoop_switch_forward6(g):mcast_snoop_switch_forward(g); // update forwarding
        mcast_snoop_mbship_t_tidy(&r);                    
}

int
igmp_mbship_port_clear(struct igmp_mbship_db_t *igmp_mbship_db, unsigned short src_port_id)
{
	struct mcast_mbship_t *pos=NULL, *n=NULL;

	list_for_each_entry_safe(pos,n,&igmp_mbship_db->mbship_list, entry_node) {
		if (pos->src_port_id != src_port_id)
			continue;

        	mcast_mbship_db_event(IGMP_MEMBER_EVENT_DEL, pos);
        	if (THREAD_ENV.igmp_proxy_snooping == 2)
                	mcast_proxy_member_update(pos);
        	mcast_mbship_db_remove_member(pos);
	}
	return 0;
}

static void mcast_snoop_report_sanity_check(struct igmp_clientinfo_t **igmp_clientinfo)
{			
        switch ((*igmp_clientinfo)->igmp_msg_type)
        {
        case IGMP_V1_MEMBERSHIP_REPORT: // TO_EX({})
                (*igmp_clientinfo)->igmp_msg_type = IGMP_V3_MEMBERSHIP_REPORT;
                mcast_snoop_legacy_report_modify(&(*igmp_clientinfo)->group_list, CHANGE_TO_EXCLUDE_MODE); 
                break;
        case IGMP_V2_MEMBERSHIP_REPORT: // TO_EX({})
                (*igmp_clientinfo)->igmp_msg_type = IGMP_V3_MEMBERSHIP_REPORT;
                mcast_snoop_legacy_report_modify(&(*igmp_clientinfo)->group_list, CHANGE_TO_EXCLUDE_MODE);
                break;
        case IGMP_V2_LEAVE_GROUP:       // TO_IN({})
                (*igmp_clientinfo)->igmp_msg_type = IGMP_V3_MEMBERSHIP_REPORT;
                mcast_snoop_legacy_report_modify(&(*igmp_clientinfo)->group_list, CHANGE_TO_INCLUDE_MODE);
                break;
        case MLD_LISTENER_REPORT:	// TO_EX({})
        	(*igmp_clientinfo)->igmp_msg_type = MLD_V2_LISTENER_REPORT;
        	mcast_snoop_legacy_report_modify(&(*igmp_clientinfo)->group_list, CHANGE_TO_EXCLUDE_MODE);
        	break;
        case MLD_LISTENER_REDUCTION:	// TO_IN({})
        	(*igmp_clientinfo)->igmp_msg_type = MLD_V2_LISTENER_REPORT;
        	mcast_snoop_legacy_report_modify(&(*igmp_clientinfo)->group_list, CHANGE_TO_INCLUDE_MODE);
        	break;
        default:
                break;
        }
}

static int mcast_snoop_report_record_filter(struct igmp_grprec_t *r)
{
	FILTER_EXECUTE(filter, r)
		
	return 0;
}


static int mcast_snoop_report_worker(struct igmp_clientinfo_t **igmp_clientinfo)
{                
        struct igmp_group_t *pos, *n;        
        mcast_snoop_record_parser(*igmp_clientinfo);      // make group records RFC 5790 compatible
                
        list_for_each_entry_safe(pos, n, &(*igmp_clientinfo)->group_list, entry_node) {
        	struct igmp_grprec_t r = {.flag = 0, .rec = pos};
        	mcast_pkt_ip_cpy(&r.client_ip, &(*igmp_clientinfo)->client_ip);
        	if (mcast_snoop_report_record_filter(&r))
		{
			// remove this record
			list_del(&pos->entry_node);
			free_safe(pos);
			(*igmp_clientinfo)->group_num -= 1;			
			continue;
		}
			                
                dbprintf_igmp(LOG_WARNING, "process group-%s source:-%s\n", inet_ipaddr(pos->group_ip, s1), inet_ipaddr(pos->src_ip, s2));
                struct mcast_mbship_t h;
                mcast_snoop_create_host(*igmp_clientinfo, &h, pos);
		    
                struct mcast_snoop_grp_t *g = mcast_snoop_locate_group(pos);
                if (g == NULL) g = mcast_snoop_create_group(pos);
                g->flag = r.flag;
                struct mcast_snoop_src_t *s = mcast_snoop_locate_group_source(g, pos);
                if (s == NULL) s = mcast_snoop_create_group_source(g, pos);
				                                           
                mcast_snoop_record_processor(pos->record_type, g, s, &h);
                if(!g->vid && h.ani_vid) g->vid = h.ani_vid;
                (IS_IPV6_ADDR(g->group_ip))?mcast_snoop_switch_forward6(g):mcast_snoop_switch_forward(g);
                mcast_snoop_mbship_t_tidy(pos);       
        }
	  
        return 0;
}

static int mcast_snoop_db_timeout_update(long timer_interval)
{
        struct mcast_mbship_t *pos=NULL, *n=NULL;
        
        long current_time = igmp_timer_get_cur_time();
        
	list_for_each_entry_safe(pos, n, &igmp_mbship_db_g.mbship_list, entry_node) {
                if (current_time - pos->access_time > timer_interval) {
                	dbprintf_igmp(LOG_INFO, "timeout group-%s source:-%s host-%s\n", inet_ipaddr(pos->group_ip, s1), inet_ipaddr(pos->src_ip, s2), inet_ipaddr(pos->client_ip, s3));
                        mcast_mbship_db_event(IGMP_MEMBER_EVENT_DEL, pos);
                        if (THREAD_ENV.igmp_proxy_snooping == 2)
				mcast_proxy_member_update(pos);
			mcast_mbship_db_remove_member(pos);
                }
	}
	
        return 0;
}

static struct mcast_mbship_t *mcast_snoop_locate_host(struct mcast_mbship_t *dummy)
{
        struct mcast_mbship_t *pos = NULL,*n = NULL;
        
	list_for_each_entry_safe(pos, n, &igmp_mbship_db_g.mbship_list, entry_node)
        {
		if (pos->src_port_id != dummy->src_port_id)
			continue;
		if (pos->logical_port_id != dummy->logical_port_id)
			continue;
		if (pos->uni_vid != dummy->uni_vid)
			continue;
		if (mcast_pkt_ip_cmp(&pos->group_ip, &dummy->group_ip))
			continue;
		if (mcast_pkt_ip_cmp(&pos->client_ip, &dummy->client_ip))
			continue;
		if (mcast_pkt_ip_cmp(&pos->src_ip, &dummy->src_ip))
			continue;
		return pos;
	}
	
	return NULL;
}

static int 
mcast_snoop_normal_leave_update(void *data_ptr)
{
        // locate real member from a dummy
	struct mcast_mbship_t *m = mcast_snoop_locate_host((struct mcast_mbship_t *)data_ptr);

        if (!m) return 0;       // member no longer exist
        
        dbprintf_igmp(LOG_INFO, "normal leave-%s source:-%s host-%s\n", inet_ipaddr(m->group_ip, s1), inet_ipaddr(m->src_ip, s2), inet_ipaddr(m->client_ip, s3));
        
        mcast_mbship_db_event(IGMP_MEMBER_EVENT_DEL, m);
        if (THREAD_ENV.igmp_proxy_snooping == 2)
                mcast_proxy_member_update(m);
        mcast_mbship_db_remove_member(m);
        free_safe(data_ptr);   // free dummy
	
	return 0;
}

static int 
mcast_proxy_normal_leave_update(void *data_ptr)
{
        struct mcast_mbship_t *m = (struct mcast_mbship_t *)data_ptr;
        
	if (!m) return 0;       // member no longer exist
        
        dbprintf_igmp(LOG_INFO, "router normal leave-%s source:-%s host-%s\n", inet_ipaddr(m->group_ip, s1), inet_ipaddr(m->src_ip, s2), inet_ipaddr(m->client_ip, s3));
        
        if (THREAD_ENV.igmp_proxy_snooping == 1)
                mcast_proxy_member_update(m);
        
        free_safe(data_ptr);   // free dummy
	
	return 0;
}


static int mcast_snoop_db_deinit()
{
        struct mcast_snoop_grp_t *pos, *n;
        struct mcast_snoop_src_t *pos1, *n1;
        struct mcast_mbship_t *pos2, *n2;
        
        list_for_each_entry_safe(pos, n, &igmp_mbship_db_g.mcast_snoop_list, entry_node) {
                list_for_each_entry_safe(pos1, n1, &pos->src_list, entry_node) {
                        list_for_each_entry_safe(pos2, n2, &pos1->host_ssm_list, entry_node_v3) {
                                mcast_snoop_free_host(pos2);      // free source hosts
                        }
                        list_del(&pos1->entry_node);
                        free_safe(pos1);      // free source
                }

                list_for_each_entry_safe(pos2, n2, &pos->host_asm_list, entry_node_v3) {
                        mcast_snoop_free_host(pos2);      // free group hosts
                }                
                list_del(&pos->entry_node);
                free_safe(pos);        // free group                          
	}
	
	return 0;
}

static int
igmp_mbship_db_deinit()
{
	mcast_snoop_db_deinit();
	mcast_proxy_db_deinit();
	
	return 0;
}

struct grec_t {
	struct list_head node;
	struct igmpv3_grec grec;
};

static int igmp_v3_pkt_repack(struct igmp_clientinfo_t *igmp_clientinfo)
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
	
static int 
mcast_snoop_pkt_process(struct igmp_clientinfo_t *igmp_clientinfo)
{       
	int ret = 0;
	        
        switch (igmp_clientinfo->igmp_msg_type)
	{
	case IGMP_MEMBERSHIP_QUERY:
	case MLD_LISTENER_QUERY:
		if (THREAD_ENV.igmp_proxy_snooping != 0) ret = MCAST_FILTER_STOP;	// only L2 sees query
		if (igmp_clientinfo->rx_desc.logical_port_id != switch_get_wan_logical_portid()) ret = MCAST_FILTER_STOP; // query only comes from WAN
		break;
		
	case IGMP_V1_MEMBERSHIP_REPORT:
	case IGMP_V2_MEMBERSHIP_REPORT:
	case IGMP_V2_LEAVE_GROUP:
	case IGMP_V3_MEMBERSHIP_REPORT:
	case MLD_LISTENER_REPORT:
	case MLD_LISTENER_REDUCTION:
	case MLD_V2_LISTENER_REPORT:
		FILTER_EXECUTE(pre_filter, igmp_clientinfo)
		
		igmp_v3_lw_router_compatibility_filter(&igmp_clientinfo);
		mcast_snoop_report_sanity_check(&igmp_clientinfo);	// upgrade to igmpv3/mldv2
		
		mcast_snoop_report_worker(&igmp_clientinfo);
		ret = igmp_v3_pkt_repack(igmp_clientinfo);
		break;
		
	case IGMP_SPECIAL:
		break;
	default:
		dbprintf_igmp(LOG_WARNING, "Unknown Type\n");
		ret = -1;
	}
	
	return ret;
}

static int
mcast_main_clientinfo_free(struct igmp_clientinfo_t *igmp_clientinfo)
{
	if (igmp_clientinfo == NULL) return 0;

	if (!list_empty(&igmp_clientinfo->group_list))
	{
		struct igmp_group_t *group_pos=NULL, *group_n=NULL;

		list_for_each_entry_safe( group_pos, group_n, &igmp_clientinfo->group_list, entry_node) {
			list_del(&group_pos->entry_node);
			free_safe(group_pos);
		}
	}
        
	free_safe(igmp_clientinfo);
	       
	return 0;
}

static int mcast_tx_L3(struct igmp_clientinfo_t *igmp_clientinfo)
{
	mcast_proxy_acceptigmp(igmp_clientinfo->pktinfo);
	              
	return 0;
}

#define SM_INIT(name) struct igmp_pkt_stage_t name = { .parse = NULL,  .tagfilter = NULL, .snooping = NULL, .tx = NULL };  \
			int name##_ret;

#define SM_EXECUTE(name, stage, ptr_clientinfo)  name##_ret = 0;				\
					if (name.stage) name##_ret = name.stage(ptr_clientinfo);  \
					dbprintf_igmp(LOG_INFO, #stage" -> %s\n", (name##_ret == 0)?"success":"fail"); \
					ptr_clientinfo->state.exec_ret = (name##_ret == 0)?CONTINUE:STOP;
                                        
struct igmp_pkt_stage_t {
	int (*parse)(struct igmp_clientinfo_t *igmp_clientinfo);
        int (*tagfilter)(struct igmp_clientinfo_t *igmp_clientinfo);
        int (*snooping)(struct igmp_clientinfo_t *igmp_clientinfo);
        int (*tx)(struct igmp_clientinfo_t *igmp_clientinfo);
};

/*
	THREAD_ENV.igmp_proxy_snooping == 0 -> snooping
	THREAD_ENV.igmp_proxy_snooping == 1 -> proxy without snooping
	THREAD_ENV.igmp_proxy_snooping == 2 -> proxy with snooping

        snooping -> mcast_snoop_pkt_process()
        proxy without snooping -> mcast_proxy_acceptigmp()
        proxy with snooping -> mcast_snoop_pkt_process() first followed by mcast_proxy_acceptigmp()
*/

static int run_mcast_sm(struct cpuport_info_t *pktinfo) 
{
	struct igmp_clientinfo_t *igmp_clientinfo = NULL;
        
        SM_INIT(sm)
        
        int state = 0;
        
	igmp_clientinfo = malloc_safe(sizeof(struct igmp_clientinfo_t));
        
        if (igmp_clientinfo == NULL)
        {
                dbprintf_igmp(LOG_ERR, "igmp_clientinfo allocate failed! -> DROP\n");
                return -1;
        }
        
        memset(igmp_clientinfo, 0, sizeof(struct igmp_clientinfo_t));
        igmp_clientinfo->pktinfo = pktinfo; 
	INIT_LIST_HEAD(&igmp_clientinfo->group_list);

	if (pktinfo->rx_desc.router_mode && THREAD_ENV.igmp_proxy_snooping)
        {
                sm.parse = mcast_pkt_parse_clientinfo_router_mode;
                if (THREAD_ENV.igmp_proxy_snooping == 2) sm.snooping = mcast_snoop_pkt_process;
                sm.tx = mcast_tx_L3;
        }
        else
        {
                sm.parse = mcast_pkt_parse_clientinfo;
                sm.tagfilter = igmp_config_g.tag_filter_iteration;                                
                sm.snooping = mcast_snoop_pkt_process;
                sm.tx = igmp_config_g.mcast_TX_L2; 
        }
        
	state = IGMP_PKT_PARSE_CLIENTINFO;
        IGMP_PKT_STATE_SET(igmp_clientinfo, state);
        while (state != IGMP_PKT_DONE) {
                switch (state)
                {
                case IGMP_PKT_PARSE_CLIENTINFO:
                	SM_EXECUTE(sm, parse, igmp_clientinfo)
                        break;
                case IGMP_PKT_TAGFILTER:
                	SM_EXECUTE(sm, tagfilter, igmp_clientinfo)
                        break;
                case IGMP_PKT_BRIDGE:
                	SM_EXECUTE(sm, snooping, igmp_clientinfo)
                        break;
                case IGMP_PKT_TX:
                	SM_EXECUTE(sm, tx, igmp_clientinfo)
                        break;
                default:
                        break;        
                }

		struct igmp_counter_module_t *m=NULL;
        
        	list_for_each_entry(m,  &igmp_config_g.m_counter_list, m_node) {
        		m->igmp_counter_handler(igmp_clientinfo);            
		}
                
                if (igmp_clientinfo->state.exec_ret == STOP) break;
                
                state++;    // advance to next state
                IGMP_PKT_STATE_SET(igmp_clientinfo, state);     
	}        
        
	mcast_main_clientinfo_free(igmp_clientinfo);
        
	return 0;
}

static int mcast_pkt_validation(struct cpuport_info_t *pktinfo)
{
	if (pktinfo == NULL)
	{
		dbprintf_igmp(LOG_WARNING, "pktinfo is NULL\n");
		return -1;
	}
	
	if (M_HAS_SRC_PORTID(pktinfo) && pktinfo->rx_desc.router_mode == 1)
	{
		dbprintf_igmp(LOG_WARNING, "pkt comes from L3 path but has bridge port ME -> DROP\n");
		return -1;	
	}

        if (!pktinfo->rx_desc.router_mode && !M_HAS_SRC_PORTID(pktinfo))
	{
		dbprintf_igmp(LOG_WARNING, "pkt comes from L2 path but without valid bridge port ME -> DROP\n");
		return -1;	
	}
	
	if (THREAD_ENV.igmp_proxy_snooping && !pktinfo->rx_desc.router_mode)
	{
		dbprintf_igmp(LOG_WARNING, "L3 mode does not handle L2 path packet -> DROP\n");
		return -1;
	}

        if (!THREAD_ENV.igmp_proxy_snooping && pktinfo->rx_desc.router_mode)
	{
		dbprintf_igmp(LOG_WARNING, "L2 mode does not handle L3 path packet -> DROP\n");
		return -1;
	}
	
	return 0;
}

static int mcast_pkt_handler(struct cpuport_info_t *pktinfo)
{
	dbprintf_igmp(LOG_WARNING, "igmp function: %d, pkt router mode: %d, rx logic port: %d\n", 
			THREAD_ENV.igmp_proxy_snooping, pktinfo->rx_desc.router_mode, pktinfo->rx_desc.logical_port_id);
	
	
	if (mcast_pkt_validation(pktinfo)) return -1;	// drop invalid pkt
	
	dbprintf_igmp(LOG_WARNING, "mcast_pkt_validation PASS\n");
	FILTER_EXECUTE(raw_filter, pktinfo)
	
	run_mcast_sm(pktinfo);
	
	return 0;
}

static int mcast_query_handler(struct cpuport_info_t *pktinfo)
{
	return mcast_pkt_handler(pktinfo);
}
			
static int mcast_joinleave_handler(struct cpuport_info_t *pktinfo)
{
	return mcast_pkt_handler(pktinfo);
}

static int mcast_cmd_runpkt(struct cpuport_info_t *p)
{
	mcast_pkt_handler(p);
	if (p == NULL) return 0;
	if (p->buf_ptr) free_safe(p->buf_ptr);
	free_safe(p);
	
	return 0;   
}

static int mcast_env_update(struct env_info_t *p)
{
	if (p == NULL) return 0;

	/* Anything about struct igmp_env_g were removed, but the env update
	 * functions still remain, in case we need it one day.
	 */

	dbprintf_igmp(LOG_WARNING, "env update via cli: %s -> %d\n", p->env_name, p->value);	
	
	free_safe(p);
	
	return 0;   
}

static int mcast_cmd_handler(struct igmp_cmd_msg_t *msg)
{
	int ret = 0;
        
	switch(msg->cmd)
	{
        case IGMP_CMD_OMCI:
        	break;
	case IGMP_CMD_ME_RELOAD:
		mcast_mbship_db_event(IGMP_MEMBER_EVENT_RELOAD, NULL);
		break;
        case IGMP_CMD_ENV:
        	mcast_env_update((struct env_info_t *)msg->usr_data);	
        	break;
        case IGMP_CMD_RUN_PKT:
        	mcast_cmd_runpkt((struct cpuport_info_t *)msg->usr_data);
		break;
        case IGMP_CMD_TERMINATE:
		break;
	default:
		dbprintf_igmp(LOG_WARNING, "unknown cmd\n");
	}
	
	return ret;
}


static int mcast_timer_handler(struct fwk_timer_msg_t *timer_msg)
{
	int ret = 0;
	
	if (timer_msg->event == TIMER_EVENT_QUERY_INTERVAL)
        {
        	// make sure returned timer id is the last one set
                if (timer_msg->timer_id != timer_id_query_interval_g)
                {
                	dbprintf_igmp(LOG_ERR, "timer_id_query_interval_g mismatch(msg=%d, expect=%d), ignored\n",timer_msg->timer_id, timer_id_query_interval_g);
                        ret = -1;
                        return ret;
                }
                                
                if (igmp_config_g.query_interval > 0)
                	timer_id_query_interval_g = fwk_timer_create(igmp_timer_qid_g, TIMER_EVENT_QUERY_INTERVAL, igmp_config_g.query_interval*1000, NULL);
                if ( timer_id_query_interval_g < 0 )
                        dbprintf_igmp(LOG_ERR, "fwk_timer_create return(%d)\n", timer_id_query_interval_g );
				
                if (THREAD_ENV.igmp_proxy_snooping)
		{
                	if (igmp_config_g.igmp_version & IGMP_V3_BIT)
				sendigmpv3generalquery();
			else
				sendgeneralmembershipquery();
                }
                query_resp_timeout_id = fwk_timer_create(igmp_timer_qid_g, TIMER_EVENT_QUERY_TIMEOUT_UPDATE, igmp_config_g.max_respon_time*100, NULL);
                if ( query_resp_timeout_id < 0 )
                        dbprintf_igmp(LOG_ERR, "fwk_timer_create return(%d)\n", query_resp_timeout_id);
        }
        else if (timer_msg->event == TIMER_EVENT_QUERY_TIMEOUT_UPDATE)
        {	
                mcast_snoop_db_timeout_update(IGMPV3_QUERY_TIMEOUT_INTERVAL);
        }
	else if (timer_msg->event == TIMER_EVENT_NONE_FAST_LEAVE)
	{
                mcast_snoop_normal_leave_update(timer_msg->user_data);
        }
        else if (timer_msg->event == TIMER_EVENT_ROUTER_NONE_FAST_LEAVE)
	{
		mcast_proxy_normal_leave_update(timer_msg->user_data);
//		if ( THREAD_ENV.igmp_proxy_snooping )
//              sendgroupspecificmemberquery(timer_msg->user_data); 
	}
	else if (timer_msg->event == TIMER_EVENT_ROUTER_COMPATIBITY)
	{
                struct proxy_hpt_msg hpt_msg;
                hpt_msg.timer_id = timer_msg->timer_id;
                hpt_msg.mcastaddr = (uint32_t)timer_msg->user_data;
                if (igmp_config_g.igmp_version & IGMP_V3_BIT)
                        igmp_proxy_v3_compatibility_update(&hpt_msg);		     
        }
	else if (timer_msg->event & TIMER_EVENT_CUSTOM)
		igmp_custom_timer_handler(timer_msg); // call registered callbacks
	else
                dbprintf_igmp(LOG_ERR, "fwk_timer unknown event(%d)\n", timer_msg->event);			
	
	return ret;    
}

// daemon module hooks
static const char * mcast_daemon_name()
{
	return "Lightweight IGMP/MLD(RFC5790) daemon";
}

static void mcast_daemon_init()
{
	mcast_mbship_db_init();

        INIT_LIST_HEAD(&igmp_config_g.m_filter_list);
        INIT_LIST_HEAD(&igmp_config_g.m_counter_list);
}

static void mcast_mainloop_before()
{	
	module_filter_specific_add();
	
	struct igmp_filter_module_t *f=NULL;
        
        list_for_each_entry(f,  &igmp_config_g.m_filter_list, m_node) {
        	if (f->init != NULL) f->init();            
        }
	
	module_counter_specific_add();
			
	struct igmp_counter_module_t *m=NULL;
        
        list_for_each_entry(m,  &igmp_config_g.m_counter_list, m_node) {
        	if (m->init != NULL) m->init();            
        }
        
	if (igmp_config_g.query_interval > 0)
		timer_id_query_interval_g = fwk_timer_create(igmp_timer_qid_g, TIMER_EVENT_QUERY_INTERVAL, igmp_config_g.query_interval*1000, NULL);
}

static void mcast_mainloop_after()
{
	// don't do it in daemon_deinit as it is called via cli, which is another process
	igmp_mbship_db_deinit();
	
	struct igmp_counter_module_t *m=NULL, *pos=NULL;
        list_for_each_entry_safe(m, pos,  &igmp_config_g.m_counter_list, m_node) {
		list_del(&m->m_node);       
        }

	struct igmp_filter_module_t *f, *f_pos=NULL;
        
        list_for_each_entry_safe(f, f_pos,  &igmp_config_g.m_filter_list, m_node) {
        	if (f->deinit != NULL) f->deinit();
        	list_del(&f->m_node);
        }
}

static void mcast_daemon_deinit()
{
}

struct igmp_daemon_t igmpv3_lw =
{
        .daemon_name = mcast_daemon_name,
        .igmp_daemon_init = mcast_daemon_init,
        .igmp_mainloop_before = mcast_mainloop_before,
        .igmp_joinleave_hook = mcast_joinleave_handler,
        .igmp_query_hook = mcast_query_handler,
        .igmp_timer_hook = mcast_timer_handler,
        .igmp_cmd_hook = mcast_cmd_handler,
        .igmp_cli_hook = igmp_cli_cmdline,
        .igmp_mainloop_after = mcast_mainloop_after,
        .igmp_daemon_deinit = mcast_daemon_deinit  
};

// simplified
int mcast_mbship_register_event(int (*cb)(short, void *))
{
	igmp_mbship_db_g.mcast_mbship_db_event = cb;
	
	return 0;
}

int mcast_register_tag_filter(int (*cb)(struct igmp_clientinfo_t *))
{
	igmp_config_g.tag_filter_iteration = cb;
	
	return 0;
}

int mcast_register_TX_L2(int (*cb)(struct igmp_clientinfo_t *))
{
	igmp_config_g.mcast_TX_L2 = cb;
	
	return 0;
}  
