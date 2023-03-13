/******************************************************************
 *
 * Copyright (C) 2014 5V Technologies Ltd.
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
 * File    : igmp_v3_lw_router.c
 *
 ******************************************************************/
 
#include "mcast_proxy.h"
#include <errno.h>

#include "mroute.h"
#include "mcast_const.h"
#include "igmp_mbship.h"
#include "mcast_router.h"
#include "mcast_timer.h"
#include "fwk_timer.h"
#include "fwk_timer.h"
#include <linux/if_ether.h>
#include "platform.h"


extern struct igmp_config_t igmp_config_g;
extern struct igmp_mbship_db_t igmp_mbship_db_g;
extern char *send_buf;
extern int timer_id_router_non_fast_leave_g;

static unsigned char tbuffer[128];	// packt buffer for igmp v1/v2 & mld v1 trasnformation

// Globals                  
extern uint32_t   allhosts_group;          /* All hosts addr in net order */
extern uint32_t   allrouters_group;          /* All hosts addr in net order */


extern mcast_proxy mproxy;

#define GREC_NSRC_CHK(b , s)     {if (b) {dbprintf_igmp(LOG_WARNING, s);return -1;}}

#define GREC_SIZE(numsrc) \
         (sizeof(struct mcast_grec) + (numsrc) * sizeof(union ipaddr))
         
struct query_obj
{
        struct mcast_grec *old;     // last socket group state
        struct mcast_grec *result;       // new query content        
};

struct mcast_proxy_hst_t {
        union ipaddr host_ip;
        struct list_head entry_node;    
};

struct mcast_proxy_src_t
{
        union ipaddr src_ip;
        struct list_head host_ssm_list;
        struct list_head entry_node;    
};

static inline void mcast_proxy_free_host(struct mcast_proxy_hst_t *host)
{
        list_del(&host->entry_node);
        free_safe(host);
}


/**
*   Finds the textual name of the supplied IGMP request.
*/
static char *mcastpacketkind(u_int type, u_int code) {
	static char unknown[20];

	switch (type)
	{
	case IGMP_HOST_MEMBERSHIP_QUERY:    return "IGMP query  ";
	case IGMP_HOST_MEMBERSHIP_REPORT:   return "IGMPv1 report  ";
	case IGMPV2_HOST_MEMBERSHIP_REPORT: return "IGMPv2 report  ";
	case IGMPV3_HOST_MEMBERSHIP_REPORT: return "IGMPv3 report  ";
	case IGMP_HOST_LEAVE_MESSAGE:       return "IGMPv2 leave  ";
	case MLD_LISTENER_QUERY:		return "MLD query  ";
	case MLD_LISTENER_REPORT:		return "MLD report  ";
	case MLD_V2_LISTENER_REPORT:	return "MLDv2 report  ";
	case MLD_LISTENER_REDUCTION:        return "MLD done  ";
	default:
		sprintf(unknown, "unk: 0x%02x/0x%02x    ", type, code);
		return unknown;
    	}
}

static struct mcast_proxy_grp_t *mcast_proxy_locate_group(union ipaddr *group)
{
        struct mcast_proxy_grp_t *pos, *n;
        
        list_for_each_entry_safe(pos, n, &igmp_mbship_db_g.mcast_proxy_list, entry_node) {
        	if (mcast_pkt_ip_cmp(group, &pos->group_ip) == 0)
                        return pos;                               
	}
        
        return NULL;        
}

static struct mcast_proxy_grp_t *mcast_proxy_create_group(union ipaddr *group)
{
        struct mcast_proxy_grp_t *g;
        
        g = malloc_safe(sizeof(struct mcast_proxy_grp_t));
        if (g == NULL) return NULL;
        INIT_LIST_NODE(&g->entry_node);
        mcast_pkt_ip_cpy(&g->group_ip, group);
        g->mode = IGMP_MODE_V3;         // default querier mode is igmpv3
        INIT_LIST_HEAD(&g->src_list);
        INIT_LIST_HEAD(&g->host_asm_list);
        list_add(&g->entry_node, &igmp_mbship_db_g.mcast_proxy_list);
          
        return g;        
}

static struct mcast_proxy_src_t *mcast_mcast_proxy_locate_group_source(struct mcast_proxy_grp_t *g, union ipaddr *src)
{
        struct mcast_proxy_src_t *pos, *n;
        
        list_for_each_entry_safe(pos, n, &g->src_list, entry_node) {
        	if (mcast_pkt_ip_cmp(src, &pos->src_ip) == 0)
                        return pos;                               
	}
        
        return NULL;        
}

static struct mcast_proxy_src_t *mcast_proxy_create_group_source(struct mcast_proxy_grp_t *g, union ipaddr *src)
{
        struct mcast_proxy_src_t *s;
        
        if (src == NULL) return NULL;
        s = malloc_safe(sizeof(struct mcast_proxy_src_t));
        if (s == NULL) return NULL;
        mcast_pkt_ip_cpy(&s->src_ip, src);
        INIT_LIST_HEAD(&s->host_ssm_list);
        INIT_LIST_NODE(&s->entry_node);
        list_add(&s->entry_node, &g->src_list);
        
        return s;        
}

static void proxy_create_host(struct mcast_proxy_hst_t *h, union ipaddr *hst)
{
        mcast_pkt_ip_cpy(&h->host_ip, hst);      
}

static void mcast_proxy_source_host_list_del(struct mcast_proxy_src_t *s, struct mcast_proxy_hst_t *h)
{
        struct mcast_proxy_hst_t *pos, *n;
        
        list_for_each_entry_safe(pos, n, &s->host_ssm_list, entry_node) {
        	if (mcast_pkt_ip_cmp(&pos->host_ip, &h->host_ip) == 0)
                {
                        mcast_proxy_free_host(pos);
                        return;
                } 
        }                   
}

static void mcast_proxy_group_host_list_add(struct mcast_proxy_grp_t *g, struct mcast_proxy_src_t *s, struct mcast_proxy_hst_t *h)
{
        struct mcast_proxy_hst_t *pos, *n;
       
        list_for_each_entry_safe(pos, n, &g->host_asm_list, entry_node) {
        	if (mcast_pkt_ip_cmp(&pos->host_ip, &h->host_ip) == 0)
                {
                        //pos->access_time = h->access_time;  // existing entry
                        return;
                } 
        }
        
        struct mcast_proxy_hst_t *new_host;
        new_host = malloc_safe(sizeof(struct mcast_proxy_hst_t));
        memcpy(new_host, h, sizeof(struct mcast_proxy_hst_t));
        INIT_LIST_NODE(&new_host->entry_node);
        list_add(&new_host->entry_node, &g->host_asm_list);      // new entry
                
        // sanity check, remove host from all source list
        struct mcast_proxy_src_t *pos1, *n1;
        
        
        list_for_each_entry_safe(pos1, n1, &g->src_list, entry_node) {
                mcast_proxy_source_host_list_del(pos1, h);                              
	}                    
}

static void mcast_proxy_group_host_list_del(struct mcast_proxy_grp_t *g, struct mcast_proxy_hst_t *h)
{
        struct mcast_proxy_hst_t *pos, *n;
        list_for_each_entry_safe(pos, n, &g->host_asm_list, entry_node) {
        	if (mcast_pkt_ip_cmp(&pos->host_ip, &h->host_ip) == 0)
                {
                        mcast_proxy_free_host(pos);
                        return;
                }    
        }                 
}

static void mcast_proxy_source_host_list_add(struct mcast_proxy_grp_t *g, struct mcast_proxy_src_t *s, struct mcast_proxy_hst_t *h)
{
        struct mcast_proxy_hst_t *pos, *n;
       
        list_for_each_entry_safe(pos, n, &s->host_ssm_list, entry_node) {
        	if (mcast_pkt_ip_cmp(&pos->host_ip, &h->host_ip) == 0)
                {
                        //pos->access_time = h->access_time;  // existing entry
                        return;
                } 
        }
        
        struct mcast_proxy_hst_t *new_host;
        new_host = malloc_safe(sizeof(struct mcast_proxy_hst_t));
        memcpy(new_host, h, sizeof(struct mcast_proxy_hst_t));
        INIT_LIST_NODE(&new_host->entry_node);
        list_add(&new_host->entry_node, &s->host_ssm_list);      // new entry
        
        // sanity check, remove host from group list
        mcast_proxy_group_host_list_del(g, h);                     
}

static void grec_difference(struct query_obj *query, struct mcast_grec *B)
{          
        struct mcast_grec *A = query->old;
        int size = A->grec_nsrcs;
        
        query->result = (struct mcast_grec *)malloc_safe(GREC_SIZE(size));
        if (query->result == NULL)     return;
        
        memset(query->result, 0, GREC_SIZE(size));
        query->result->grec_mca = A->grec_mca;
        
        int i,j;
        for (i=0;i < A->grec_nsrcs ;i++)
        {
                int found = 0;
                for (j=0;j < B->grec_nsrcs;j++)
                {
                        if (mcast_pkt_ip_cmp(&A->grec_src[i], &B->grec_src[j]) == 0)
                        {
                                found = 1;
                                break;      
                        }
                }
                if (!found)
                {
                        mcast_pkt_ip_cpy(&query->result->grec_src[query->result->grec_nsrcs], &A->grec_src[i]);
                        query->result->grec_nsrcs++;    
                }
        }
        
        dbprintf_igmp(LOG_WARNING, "(%d - %d)=%d\n", A->grec_nsrcs, B->grec_nsrcs, query->result->grec_nsrcs);           
}

static void grec_intersection(struct query_obj *query, struct mcast_grec *B)
{      
        struct mcast_grec *A = query->old;
        int size = (A->grec_nsrcs > B->grec_nsrcs)?A->grec_nsrcs:B->grec_nsrcs;
         
        query->result = (struct mcast_grec *)malloc_safe(GREC_SIZE(size));
        if (query->result == NULL)     return;
        
        memset(query->result, 0, GREC_SIZE(size));
        query->result->grec_mca = A->grec_mca;
        
        int i,j;
        for (i=0;i < A->grec_nsrcs ;i++)
        {
                for (j=0;j < B->grec_nsrcs;j++)
                {
                        if (mcast_pkt_ip_cmp(&A->grec_src[i], &B->grec_src[j]) == 0)
                        {
                                mcast_pkt_ip_cpy(&query->result->grec_src[query->result->grec_nsrcs], &A->grec_src[i]);
				query->result->grec_nsrcs++;        
                        }
                }
        }
        dbprintf_igmp(LOG_WARNING, "(%d * %d)=%d\n", A->grec_nsrcs, B->grec_nsrcs, query->result->grec_nsrcs);     
}


static int mcast_proxy_process_report(int grec_type, union ipaddr *grp, union ipaddr *src, union ipaddr *hst)
{
        struct mcast_proxy_hst_t h;
        proxy_create_host(&h, hst);
        
        struct mcast_proxy_grp_t *g = mcast_proxy_locate_group(grp);
        if (g == NULL) g = mcast_proxy_create_group(grp);
        
        struct mcast_proxy_src_t *s = mcast_mcast_proxy_locate_group_source(g, src);
        if (s == NULL) s = mcast_proxy_create_group_source(g, src);        
        
        switch(grec_type)
        {
        case MODE_IS_INCLUDE: // source A -> A + B, (B) = GMI
                GREC_NSRC_CHK((s == NULL),"query response to (*, G) leave\n") // should never happen
                dbprintf_igmp(LOG_WARNING,"query response to (x, G) join\n");
                mcast_proxy_source_host_list_add(g, s, &h);
                break;
        case MODE_IS_EXCLUDE:   // source A -> A, G_Timer = GMI
                GREC_NSRC_CHK((s != NULL),"query response to EXCLUDE (x, G) join is not supported\n")
                dbprintf_igmp(LOG_WARNING,"query response to (*, G) join\n");
                mcast_proxy_group_host_list_add(g, s, &h);
                break;
        case CHANGE_TO_INCLUDE_MODE: // source A -> A + B, Send Q(G, A-B), If G_Timer > 0, Send Q(G)
                if (s == NULL)
                {
                        dbprintf_igmp(LOG_WARNING,"(*, G) leave\n");
                        if (igmp_config_g.immediate_leave)
                                mcast_proxy_group_host_list_del(g, &h);
                        else
                        {
                        	if (THREAD_ENV.igmp_proxy_snooping == 1)
                        	{
	                                struct mcast_mbship_t *copy = malloc_safe(sizeof(struct mcast_mbship_t));
	                                memset(copy, 0, sizeof(struct mcast_mbship_t));
	                                mcast_pkt_ip_cpy(&copy->group_ip, &g->group_ip);
	                                if (s != NULL) mcast_pkt_ip_cpy(&copy->src_ip, &s->src_ip);
	                                mcast_pkt_ip_cpy(&copy->client_ip, &h.host_ip);
	                                timer_id_router_non_fast_leave_g = fwk_timer_create(igmp_timer_qid_g, TIMER_EVENT_ROUTER_NONE_FAST_LEAVE, igmp_config_g.last_member_query_interval*100, (void *)copy);
				}
			}
                }
                else
                {
                        dbprintf_igmp(LOG_WARNING,"change to INCLUDE (x, G) join\n");
                        mcast_proxy_source_host_list_add(g, s, &h);
                }
                break;
        case CHANGE_TO_EXCLUDE_MODE:    // source A -> A, G_Timer = GMI
                GREC_NSRC_CHK((s != NULL),"change to EXCLUDE (x, G) join is not supported\n")
                dbprintf_igmp(LOG_WARNING,"(*, G) join\n");  // (*,G) join
                mcast_proxy_group_host_list_add(g, s, &h);
                break;
        case ALLOW_NEW_SOURCES: // soruce A -> A + B, (B) = GMI
                dbprintf_igmp(LOG_WARNING,"INCLUDE(x,G) join\n");
                GREC_NSRC_CHK((s == NULL), "INCLUDE(x,G) join contains no source\n")
                mcast_proxy_source_host_list_add(g, s, &h);
                break;
        case BLOCK_OLD_SOURCES: // source A -> A, Send Q(G, A*B) 
                dbprintf_igmp(LOG_WARNING,"INCLUDE(x,G) leave\n");
                GREC_NSRC_CHK((s == NULL), "INCLUDE(x,G) leave contains no source\n")
                if (igmp_config_g.immediate_leave)
			mcast_proxy_source_host_list_del(s, &h);
		else
                {
                        if (THREAD_ENV.igmp_proxy_snooping == 1)
                        {
	                        struct mcast_mbship_t *copy = malloc_safe(sizeof(struct mcast_mbship_t));
	                        memset(copy, 0, sizeof(struct mcast_mbship_t));
	                        mcast_pkt_ip_cpy(&copy->group_ip, &g->group_ip);
	                        if (s != NULL) mcast_pkt_ip_cpy(&copy->src_ip, &s->src_ip);
	                        mcast_pkt_ip_cpy(&copy->client_ip, &h.host_ip);
	                        timer_id_router_non_fast_leave_g = fwk_timer_create(igmp_timer_qid_g, TIMER_EVENT_ROUTER_NONE_FAST_LEAVE, igmp_config_g.last_member_query_interval*100, (void *)copy);
			}
                }
                break;
        default:
                break;
        }
        
        return 0;             
}

static int sanitycheck_downstream(union ipaddr *src, union ipaddr *group)
{
/*   
	// Sanitycheck the group adress...
	if(!IN_MULTICAST(ntohl(group)))
	{
		dbprintf_igmp(LOG_WARNING, "The group address %s is not a valid Multicast group.\n", inetfmt(group, s1));
		return 0;
	}

	// Find the interface on which the report was recieved.
	struct ifdesc   *sourcevif = getifbyaddress(src);
	if(sourcevif == NULL)
	{
        	dbprintf_igmp(LOG_WARNING, "No interfaces found for source %s\n", inetfmt(src,s1));
        	return 0;
	}

	if(sourcevif->inadr.s_addr == src)
	{
		dbprintf_igmp(LOG_WARNING, "The IGMP message was from myself. Ignoring.\n");
		return 0;
	}

	if (sourcevif->state != IF_STATE_DOWNSTREAM)
    	{
		// Log the state of the interface the report was recieved on.
		dbprintf_igmp(LOG_WARNING, "Mebership message was recieved on %s. Ignoring.\n",
        		sourcevif->state==IF_STATE_UPSTREAM?"the upstream interface":"a disabled interface");
        	return 0;
	}
*/        
	return 1;
}

static int proxy_get_group_source_count(union ipaddr *group)
{
        int count = 0;
        struct mcast_proxy_grp_t *grp = mcast_proxy_locate_group(group);
        
        if (grp == NULL) return count;
        
        struct mcast_proxy_src_t *pos, *n;
        list_for_each_entry_safe(pos, n, &grp->src_list, entry_node) {
                count++;
        }
        
        return count;        
}

static int proxy_get_group_host_count(union ipaddr *group)
{
        int count = 0;
        struct mcast_proxy_grp_t *grp = mcast_proxy_locate_group(group);
        
        if (grp == NULL) return count;
        
        struct mcast_proxy_hst_t *pos, *n;
        list_for_each_entry_safe(pos, n, &grp->host_asm_list, entry_node) {
                count++;
        }
        
        return count;        
}

static struct group_filter *proxy_get_msfilter_by_group(union ipaddr *group)
{
        int count = proxy_get_group_source_count(group);
        int i = 0;
        
        struct group_filter *imsfp = (struct group_filter *)malloc_safe(GROUP_FILTER_SIZE(count));
        imsfp->gf_numsrc = count;
        imsfp->gf_fmode = (proxy_get_group_host_count(group))?MCAST_EXCLUDE:MCAST_INCLUDE;
        
        struct mcast_proxy_src_t *pos, *n;
        struct mcast_proxy_grp_t *grp = mcast_proxy_locate_group(group);
        if (grp == NULL) return imsfp;
        
        list_for_each_entry_safe(pos, n, &grp->src_list, entry_node) {
        	struct sockaddr_in *in_addr = (struct sockaddr_in*)&imsfp->gf_slist[i];
		struct sockaddr_in6 *in6_addr = (struct sockaddr_in6*)&imsfp->gf_slist[i];
        
        	if (IS_IPV6_ADDR(pos->src_ip))
        	{
			imsfp->gf_slist[i].ss_family = AF_INET6;
        		in6_addr->sin6_addr = pos->src_ip.ipv6;
		}
		else
		{
        		imsfp->gf_slist[i].ss_family = AF_INET;
        		in_addr->sin_addr = pos->src_ip.ipv4;
		}
		
		i++;
        }
        
        return imsfp;
}


static void poxy_log_status()
{
        struct mcast_proxy_grp_t *pos, *n;
        int i = 0;
	char group_ip[INET6_ADDRSTRLEN];
	char client_ip[INET6_ADDRSTRLEN];
	
	if (omci_env_g.debug_level_igmp <= LOG_WARNING) return;
	
	int fd = util_dbprintf_get_console_fd();
        list_for_each_entry_safe(pos, n, &igmp_mbship_db_g.mcast_proxy_list, entry_node) {
                struct mcast_proxy_src_t *pos1, *n1;
                struct mcast_proxy_hst_t *pos2, *n2;
                util_fdprintf(fd, "[%d] Group-%-15s (%s), querier-%d\n\t-ASM hosts:\n", i, inet_ipaddr(pos->group_ip, group_ip), (list_empty(&pos->host_asm_list))?"SSM":"ASM", pos->mode);
                list_for_each_entry_safe(pos2, n2, &pos->host_asm_list, entry_node) {
                                util_fdprintf(fd, "\t\t-%-15s\n", inet_ipaddr(pos2->host_ip, client_ip));
                } 
                
                list_for_each_entry_safe(pos1, n1, &pos->src_list, entry_node) {
                        dbprintf_igmp(LOG_WARNING,"\t-Source %-15s, SSM hosts:\n",  inet_ipaddr(pos1->src_ip, group_ip));
                        list_for_each_entry_safe(pos2, n2, &pos1->host_ssm_list, entry_node) {
                                util_fdprintf(fd, "\t\t-%-15s\n", inet_ipaddr(pos2->host_ip, client_ip));
                        }       
                }
                i++;
                util_fdprintf(fd, "\n");                              
	}
}

static void msfilter_to_grec(struct group_filter *imsfp, struct query_obj *query)
{        
        query->old = (struct mcast_grec *)malloc_safe(GREC_SIZE(imsfp->gf_numsrc));
        if (query->old == NULL) return;

	struct sockaddr_in *in_addr = (struct sockaddr_in *)&imsfp->gf_group;
	struct sockaddr_in6 *in6_addr = (struct sockaddr_in6 *)&imsfp->gf_group;
        
        
        if (imsfp->gf_group.ss_family == AF_INET)
		query->old->grec_mca.ipv4 = in_addr->sin_addr;	 	
	else
		query->old->grec_mca.ipv6 = in6_addr->sin6_addr;
        query->old->grec_nsrcs = imsfp->gf_numsrc;
        
        int i;
        for (i=0;i < imsfp->gf_numsrc;i++)
        {
        	in_addr = (struct sockaddr_in*)&imsfp->gf_slist[i];

	        if (imsfp->gf_group.ss_family == AF_INET)
			query->old->grec_src[i].ipv4 = in_addr->sin_addr;	 	
		else
			query->old->grec_src[i].ipv6 = in6_addr->sin6_addr;
        }
}

#define MAX_SOURCES     64
extern mcast_proxy mproxy;
 
static void mcast_proxy_forward(union ipaddr *ifaddr, union ipaddr *mcastaddr, struct query_obj *query)
{
        // backup current source membership
        int sol = (IS_IPV6_ADDR(*mcastaddr))?SOL_IPV6:SOL_IP;
        unsigned char buffer[GROUP_FILTER_SIZE(MAX_SOURCES)];
        struct group_filter *imsfp = (struct group_filter *)buffer;
        int socket = 0;
	       
	if (sol == SOL_IPV6)
        {
	        imsfp->gf_group.ss_family = AF_INET6;
	        socket = mproxy.mld_udp_socket;
	        ((struct sockaddr_in6*)&imsfp->gf_group)->sin6_addr = mcastaddr->ipv6;
	}
	else
	{
	        imsfp->gf_group.ss_family = AF_INET;
	        socket = mproxy.igmp_udp_socket;
	        ((struct sockaddr_in *)&imsfp->gf_group)->sin_addr = mcastaddr->ipv4;
	}
			        
	imsfp->gf_interface = mproxy.upif_index;
        imsfp->gf_numsrc = MAX_SOURCES;         // must set this else kernel will use it
                                        	// instead of actual source count
        
        int size = sizeof(buffer);         
        if(getsockopt(socket, sol, MCAST_MSFILTER, (void *)imsfp, &size)) 
        {
                dbprintf_igmp(LOG_WARNING, "getsockopt MCAST_MSFILTER failed:%s(%s, %s)\n", strerror(errno), inet_ipaddr(*mcastaddr, s1), inet_ipaddr(*ifaddr, s2));
        }
        else
        {
	        dbprintf_igmp(LOG_WARNING, "%s mode is %s, numsrc %d\n", inet_ipaddr(*mcastaddr, s1), (imsfp->gf_fmode == MCAST_EXCLUDE)?"exclude":"include", imsfp->gf_numsrc);
                msfilter_to_grec(imsfp, query);
        }
         
        // recompute source membership & update
        int isJoin = (mcast_proxy_locate_group(mcastaddr))?1:0;
        
        if(isJoin && setsockopt(socket, sol, MCAST_JOIN_GROUP, imsfp, sizeof(struct group_req))) 
        {
                dbprintf_igmp(LOG_WARNING, "%s failed:%s\n", "MCAST_JOIN_GROUP", strerror(errno));
        }
                
        imsfp = proxy_get_msfilter_by_group(mcastaddr);
        if (imsfp == NULL) return;
        
	imsfp->gf_interface = mproxy.upif_index;
        
        if (sol == SOL_IPV6)
        {
	        imsfp->gf_group.ss_family = AF_INET6;
	        ((struct sockaddr_in6*)&imsfp->gf_group)->sin6_addr = mcastaddr->ipv6;
	}
	else
	{
	        imsfp->gf_group.ss_family = AF_INET;
	        ((struct sockaddr_in *)&imsfp->gf_group)->sin_addr = mcastaddr->ipv4;
	}
	
        
        if (isJoin)
        {
                if (imsfp->gf_fmode == MCAST_EXCLUDE)
                        imsfp->gf_numsrc = 0; // RFC 5970
        }
        else
        {
                imsfp->gf_fmode = MCAST_INCLUDE;
                imsfp->gf_numsrc = 0;        
        }
        
        if(setsockopt(socket, sol, MCAST_MSFILTER, imsfp, GROUP_FILTER_SIZE(imsfp->gf_numsrc))) 
        {
                dbprintf_igmp(LOG_WARNING, "setsockopt MCAST_MSFILTER failed:%s(%s, %s)\n", strerror(errno), inet_ipaddr(*mcastaddr, s1), inet_ipaddr(*ifaddr, s2));
        }
        else
                dbprintf_igmp(LOG_WARNING, "Set %s MCAST_MSFILTER to %s mode\n", inet_ipaddr(*mcastaddr, s1), (imsfp->gf_fmode == MCAST_EXCLUDE)?"exclude":"include");
        
        if(!isJoin && setsockopt(socket, sol, MCAST_LEAVE_GROUP, imsfp, sizeof(struct group_req))) 
        {
                dbprintf_igmp(LOG_WARNING, "%s failed:%s\n", "MCAST_LEAVE_GROUP", strerror(errno));
        }
	        
        free_safe(imsfp);              
}

static void mcast_proxy_tidy(union ipaddr *mcastaddr)
{
        struct mcast_proxy_grp_t *pos, *n;
        struct mcast_proxy_src_t *pos1, *n1;
        
        struct mcast_proxy_grp_t *g = mcast_proxy_locate_group(mcastaddr);
        if (g == NULL) return;
        
        list_for_each_entry_safe(pos, n, &igmp_mbship_db_g.mcast_proxy_list, entry_node) {
        	if (mcast_pkt_ip_cmp(&pos->group_ip, &g->group_ip) == 0)
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

static void proxy_querier(union ipaddr *src, struct query_obj *query, struct mcast_grec *grec)
{
        if (query->old == NULL) return;
        
	switch(grec->grec_type)
        {
        case MODE_IS_INCLUDE: // source A -> A + B, (B) = GMI
        case MODE_IS_EXCLUDE:   // source A -> A, G_Timer = GMI
        case CHANGE_TO_EXCLUDE_MODE:    // source A -> A, G_Timer = GMI
        case ALLOW_NEW_SOURCES: // soruce A -> A + B, (B) = GMI
                break;
        case CHANGE_TO_INCLUDE_MODE: // source A -> A + B, Send Q(G, A-B), If G_Timer > 0, Send Q(G)
                grec_difference(query, grec);
		if (query->result->grec_nsrcs) mcast_sendquery(src, query->result);
                //if (proxy_get_group_host_count(&grec->grec_mca))
		{
			// send group query
			struct mcast_grec gq;
		        gq.grec_nsrcs = 0;
		        gq.grec_mca = grec->grec_mca;
			mcast_sendquery(src, &gq);
		}
                break;
        case BLOCK_OLD_SOURCES: // source A -> A, Send Q(G, A*B)
                grec_intersection(query, grec);
		if (query->result->grec_nsrcs) mcast_sendquery(src, query->result); 
                break;
        default:
                break;
        }
}

static void query_obj_init(struct query_obj *obj)
{
        obj->result = NULL;
        obj->old = NULL;
}

static void query_obj_cleanup(struct query_obj *obj)
{
        if (obj->result) free_safe(obj->result);
        if (obj->old) free_safe(obj->old);
}

/*
In IGMP compatibility mode, the core runs in IGMPv3 but with following differences:
1. In IGMPv2 mode, ignore IGMPv3 reports and issue no group-specific queries.
2. In IGMPv1 mode, ignore IGMPv2 leave, IGMPv3 reports and issue no group specific
   or group-source specific queries.
Notes:
1. Host A first joins as IGMPv3(SSM). Host B later joins as IGMPv2(ASM) and 
   proxy downgrades to IGMPv2 mode. Host A issue IGMPv2 report upon next query to
   downgrade itself to IGMPv2(ASM).
2. Host A first joins as IGMPv2(ASM) and proxy downgrades to IGMPv2 mode. Host B
   joins as IGMPv3(SSM) before next query and join fails. Upon next query, host B
   reissue join as IGMPv2(ASM) and succeeds.
3. A downgraded proxy can be upgraded only if all hosts with lower earlier versions
   left.
*/
void igmp_proxy_v3_compatibility_update(struct proxy_hpt_msg *msg)
{
/*
        struct mcast_proxy_grp_t *g = mcast_proxy_locate_group(msg->mcastaddr);
        if (g == NULL) return;
        
        g->mode = IGMP_MODE_V3;
        
        if (msg->timer_id != g->hpt_timer_v1 && g->hpt_timer_v1)
        {
                g->mode = IGMP_MODE_V1;
        }
        else if (msg->timer_id != g->hpt_timer_v2 && g->hpt_timer_v2)
        {
                g->mode = IGMP_MODE_V2;
        }
        
        dbprintf_igmp(LOG_WARNING, "group %s, querier version %d\n", inetfmt(g->group_ip, s1), g->mode);
*/        
}

static void proxy_compatibility_mode(int type, union ipaddr *mcastaddr)
{
        struct mcast_proxy_grp_t *g = mcast_proxy_locate_group(mcastaddr);
        if (g == NULL) return;
        
        if (type == IGMP_HOST_MEMBERSHIP_REPORT)
        {
                g->mode = IGMP_MODE_V1;
                if (g->hpt_timer_v1)
                {
                        fwk_timer_delete(g->hpt_timer_v1);
                }
                g->hpt_timer_v1 = fwk_timer_create(igmp_timer_qid_g, TIMER_EVENT_ROUTER_COMPATIBITY, IGMPV3_QUERY_TIMEOUT_INTERVAL, (void *)mcastaddr);
                if (g->hpt_timer_v1 < 0)
                        dbprintf_igmp(LOG_ERR, "fwk_timer_create return(%d)\n", g->hpt_timer_v1);
        }
        else if (g->mode != IGMP_HOST_MEMBERSHIP_REPORT && type == IGMPV2_HOST_MEMBERSHIP_REPORT)
        {
                g->mode = IGMP_MODE_V2;
                if (g->hpt_timer_v2)
                {
                        fwk_timer_delete(g->hpt_timer_v2);
                }	
                g->hpt_timer_v2 = fwk_timer_create(igmp_timer_qid_g, TIMER_EVENT_ROUTER_COMPATIBITY, IGMPV3_QUERY_TIMEOUT_INTERVAL, (void *)mcastaddr);
                if (g->hpt_timer_v2 < 0)
                        dbprintf_igmp(LOG_ERR, "fwk_timer_create return(%d)\n", g->hpt_timer_v2);
        }
        
        dbprintf_igmp(LOG_WARNING, "group %s, querier version %d\n", inet_ipaddr(*mcastaddr, s1), g->mode);
}

/**
*   Handles incoming IGMPv3 membership reports
*   RFC 5790 lightweight IGMPv3 is implemented
*/

static mcast_report* mcast_to_mcast_report(short family, void *mcast)
{
	mcast_report *report = NULL;
	int i,j;
	int grec_size = sizeof(struct mcast_report);
	
	if (family == AF_INET)
	{
		struct igmpv3_report *org = (struct igmpv3_report *)mcast;
		
		struct igmpv3_grec *igmpgrec = &org->grec[0];
		
		for (i = 0;i < org->ngrec;i++)
		{
			grec_size += GREC_SIZE(org->grec[i].grec_nsrcs);
			igmpgrec = (struct igmpv3_grec *)((unsigned char *)igmpgrec + sizeof(struct igmpv3_grec) + igmpgrec->grec_nsrcs * sizeof(__u32));	
		}
		
		report = (mcast_report *)malloc_safe(grec_size);
		
		if (report == NULL) return NULL;
		
		memset(report, 0, grec_size);
		
		report->type = org->type;
		report->ngrec = org->ngrec;

		struct mcast_grec *mcastgrec = &report->grec[0];
		igmpgrec = &org->grec[0];	
				
		for (i = 0;i < org->ngrec;i++)
		{
			mcastgrec->grec_nsrcs = igmpgrec->grec_nsrcs;
			mcastgrec->grec_type = igmpgrec->grec_type;
			mcastgrec->grec_mca.ipv4.s_addr = igmpgrec->grec_mca;
			for (j = 0;j < org->grec[i].grec_nsrcs;j++)
			{
				mcastgrec->grec_src[j].ipv4.s_addr = igmpgrec->grec_src[j];		
			}
			mcastgrec = (struct mcast_grec *)((unsigned char *)mcastgrec + sizeof(struct mcast_grec) + mcastgrec->grec_nsrcs * sizeof(union ipaddr));
			igmpgrec = (struct igmpv3_grec *)((unsigned char *)igmpgrec + sizeof(struct igmpv3_grec) + igmpgrec->grec_nsrcs * sizeof(__u32));
		}
	}
	else if (family == AF_INET6)
	{
		struct mld2_report *org = (struct mld2_report *)mcast;

                struct mld2_grec *mldgrec = &org->mld2r_grec[0];
                
                for (i = 0;i < org->mld2r_ngrec;i++)
		{
			grec_size += GREC_SIZE(mldgrec->grec_nsrcs);
			mldgrec = (struct mld2_grec *)((unsigned char *)mldgrec + sizeof(struct mld2_grec) + mldgrec->grec_nsrcs * sizeof(struct in6_addr)); 	
		}
		
		report = (mcast_report *)malloc_safe(grec_size);
		
		if (report == NULL) return NULL;
		
		memset(report, 0, grec_size);

		report->type = org->mld2r_type;
		report->ngrec = org->mld2r_ngrec;
		
		struct mcast_grec *mcastgrec = &report->grec[0];
		mldgrec = &org->mld2r_grec[0];		
				
		for (i = 0;i < org->mld2r_ngrec;i++)
		{
			mcastgrec->grec_nsrcs = mldgrec->grec_nsrcs;
			mcastgrec->grec_type = mldgrec->grec_type;
			mcastgrec->grec_mca.ipv6 = mldgrec->grec_mca;
			for (j = 0;j < mldgrec->grec_nsrcs;j++)
			{
				mcastgrec->grec_src[j].ipv6 = mldgrec->grec_src[j];	
			}
			mcastgrec = (struct mcast_grec *)((unsigned char *)mcastgrec + sizeof(struct mcast_grec) + mcastgrec->grec_nsrcs * sizeof(union ipaddr));
			mldgrec = (struct mld2_grec *)((unsigned char *)mldgrec + sizeof(struct mld2_grec) + mldgrec->grec_nsrcs * sizeof(struct in6_addr));
		}
	}
		
	return report;
}

static void 
mcast_proxy_acceptgroupreport(union ipaddr *src, void *mcast) {
	short i, j;
    
	mcast_interface *ifup = mcast_interface_find(IS_IPV6_ADDR(*src)?AF_INET6:AF_INET, INTERFACE_UPSTREAM);
	
	mcast_report *report = mcast_to_mcast_report(IS_IPV6_ADDR(*src)?AF_INET6:AF_INET, mcast);
	
	if (report == NULL) return;
	
	struct mcast_grec *grec = &report->grec[0];
		
	for (i = 0;i < report->ngrec;i++)
	{
		if (!sanitycheck_downstream(src, &grec->grec_mca)) continue;	// skip this group
        	dbprintf_igmp(LOG_WARNING, "report from %s to %s, %d sources\n", inet_ipaddr(*src, s1), inet_ipaddr(grec->grec_mca, s2), grec->grec_nsrcs);
        
        	// treate IS_EX({}) as TO_EX({}) 
        	if (grec->grec_type == MODE_IS_EXCLUDE) grec->grec_type = CHANGE_TO_EXCLUDE_MODE;
                                                             
        	struct query_obj query;
        	query_obj_init(&query);
        
        	if (grec->grec_nsrcs == 0)
                	mcast_proxy_process_report(grec->grec_type, &grec->grec_mca, NULL, src);
        	else
                	for (j=0;j < grec->grec_nsrcs;j++)
                        	mcast_proxy_process_report(grec->grec_type, &grec->grec_mca, &grec->grec_src[j], src);
        
        	if (igmp_config_g.igmp_compatibity_mode) proxy_compatibility_mode(report->type, &grec->grec_mca);
        	mcast_proxy_tidy(&grec->grec_mca);
        	mcast_proxy_forward(&ifup->if_addr, &grec->grec_mca, &query);
        	proxy_querier(src, &query, grec);
        	query_obj_cleanup(&query);
        	
        	grec = (struct mcast_grec *)((unsigned char *)grec + sizeof(struct mcast_grec) + grec->grec_nsrcs * sizeof(union ipaddr));
                
        	poxy_log_status();
	}
	
	free_safe(report);
}

/**
*   Handles incoming membership IGMPv1/v2 reports, and
*   appends them to the routing table.
*/
static void 
acceptgroupreport(union ipaddr *src, union ipaddr *group, int type) {
        
	if (!sanitycheck_downstream(src, group)) return;
	dbprintf_igmp(LOG_WARNING, "Got IGMPv1/v2 from %s to %s. \n",inet_ipaddr(*src, s1), inet_ipaddr(*group, s2)); 
        
	// RFC 5970 requires all IGMPv1/v2 report to be treated as TO_EX({})
	struct igmpv3_report *r = (struct igmpv3_report *)tbuffer;
	r->type = type;
	r->ngrec = 1;
	r->grec[0].grec_type = CHANGE_TO_EXCLUDE_MODE;
	r->grec[0].grec_auxwords = 0;
	r->grec[0].grec_nsrcs = 0;
	r->grec[0].grec_mca = group->ipv4.s_addr; 
    
	mcast_proxy_acceptgroupreport(src, (void *)r);

}

static void 
acceptgroupreport6(union ipaddr *src, union ipaddr *group, int type) {
        
	if (!sanitycheck_downstream(src, group)) return;
	dbprintf_igmp(LOG_WARNING, "Got MLD report from %s to %s. \n", inet_ipaddr(*src, s1), inet_ipaddr(*group, s2)); 
        
	// RFC 5970 requires all IGMPv1/v2 report to be treated as TO_EX({})
	struct mld2_report *r = (struct mld2_report *)tbuffer;
	r->mld2r_type = IGMPV3_HOST_MEMBERSHIP_REPORT;
	r->mld2r_ngrec = 1;
	r->mld2r_resv1 = 0;
	r->mld2r_cksum = 0;
	r->mld2r_resv2 = 0;
	r->mld2r_grec[0].grec_type = CHANGE_TO_EXCLUDE_MODE;
	r->mld2r_grec[0].grec_auxwords = 0;
	r->mld2r_grec[0].grec_nsrcs = 0;
	r->mld2r_grec[0].grec_mca = group->ipv6; 
    
	mcast_proxy_acceptgroupreport(src, (void *)r);

}


/**
*   Recieves and handles a IGMPv2 group leave message.
*/
static void 
acceptleavemessage(union ipaddr *src, union ipaddr *group) {
    
	if (!sanitycheck_downstream(src, group)) return; 
	dbprintf_igmp(LOG_WARNING, "Got IGMPv2 leave from %s to %s.\n", inet_ipaddr(*src, s1), inet_ipaddr(*group, s2));
    
	// RFC 5970 requires all IGMPv1/v2 report to be treated as TO_IN({})
	struct igmpv3_report *r = (struct igmpv3_report *)tbuffer;
	r->type = IGMPV3_HOST_MEMBERSHIP_REPORT;
	r->ngrec = 1;
	r->grec[0].grec_type = CHANGE_TO_INCLUDE_MODE;
	r->grec[0].grec_auxwords = 0;
	r->grec[0].grec_nsrcs = 0;
	r->grec[0].grec_mca = group->ipv4.s_addr;
    
	mcast_proxy_acceptgroupreport(src, (void *)r);
}

static void
acceptleavemessage6(union ipaddr *src, union ipaddr *group) {
    
	if (!sanitycheck_downstream(src, group)) return; 
	dbprintf_igmp(LOG_WARNING, "Got IGMPv2 leave from %s to %s.\n", inet_ipaddr(*src, s1),  inet_ipaddr(*group, s2));
    
	// RFC 5970 requires all IGMPv1/v2 report to be treated as TO_IN({})

        struct mld2_report *r = (struct mld2_report *)tbuffer;
	r->mld2r_type = MLD_V2_LISTENER_REPORT;
	r->mld2r_ngrec = 1;
	r->mld2r_resv1 = 0;
	r->mld2r_cksum = 0;
	r->mld2r_resv2 = 0;
	r->mld2r_grec[0].grec_type = CHANGE_TO_INCLUDE_MODE;
	r->mld2r_grec[0].grec_auxwords = 0;
	r->mld2r_grec[0].grec_nsrcs = 0;
	r->mld2r_grec[0].grec_mca = group->ipv6; 
    
	mcast_proxy_acceptgroupreport(src, (void *)r);
}

void igmp_v3_lw_router_compatibility_filter(struct igmp_clientinfo_t **igmp_clientinfo)
{
        if (!THREAD_ENV.igmp_proxy_snooping)            return; // only for proxy
        if (!igmp_config_g.igmp_compatibity_mode)       return; // only for compatibility
        
        IGMP_MODE mode = igmp_v3_get_querier_version();
        
	// modify igmp message type to 0xff so they are dropped
	// we need to modify both parse and raw frame igmp type
        switch ((*igmp_clientinfo)->igmp_msg_type)
	{
	case IGMPV3_HOST_MEMBERSHIP_REPORT:
                if (mode == IGMP_MODE_V3) return;
                break; 
	case IGMP_HOST_LEAVE_MESSAGE:
		if (mode != IGMP_MODE_V1) return;
		break;
	default:
	        return;
	}
	(*igmp_clientinfo)->igmp_msg_type =0xff;
	struct igmphdr *igmp;
	int MAYBE_UNUSED ipdatalen;
	int iphdrlen;
	struct ip *ip = (struct ip *)((*igmp_clientinfo)->pktinfo->frame_ptr);
	iphdrlen  = ip->ip_hl << 2;
	ipdatalen = ntohs(ip->ip_len) - (ip->ip_hl << 2);
        igmp        = (struct igmphdr *)((*igmp_clientinfo)->pktinfo->frame_ptr + iphdrlen);
        igmp->type = 0xff;	
}

void mcast_proxy_acceptigmp(struct cpuport_info_t *pktinfo) 
{
	union ipaddr src, dst, group;
	int ipdatalen, iphdrlen, mcastdatalen;

        memset(&src, 0, sizeof(union ipaddr));
	memset(&dst, 0, sizeof(union ipaddr));
	memset(&group, 0, sizeof(union ipaddr));
		
	if (pktinfo->tci.ethertype == ETH_P_IP)
	{
		struct ip *ip;
		ip        = (struct ip *)pktinfo->frame_ptr;

		if (pktinfo->frame_len < sizeof(struct ip))
		{
			dbprintf_igmp(LOG_WARNING, "received packet too short (%u bytes) for IP header\n", pktinfo->frame_len);
			return;
		}
			
		struct igmphdr *igmp;
		
		src.ipv4.s_addr       = ip->ip_src.s_addr;
		dst.ipv4.s_addr       = ip->ip_dst.s_addr;

		/* 
		* this is most likely a message from the kernel indicating that
		* a new src grp pair message has arrived and so, it would be 
		* necessary to install a route into the kernel for this.
		*/
		struct igmpmsg *igmpmsg = (struct igmpmsg *)pktinfo->frame_ptr;
		if (igmpmsg->im_mbz == 0)
		{
			dbprintf_igmp(LOG_WARNING, "internal kernel message(%d:%s->%s@%d) via netlink.\n", 
	        	igmpmsg->im_msgtype, inetfmt(igmpmsg->im_src.s_addr, s1), inetfmt(igmpmsg->im_dst.s_addr, s1), igmpmsg->im_vif);
			return;
		}
	
		iphdrlen  = ip->ip_hl << 2;
		ipdatalen = ntohs(ip->ip_len) - (ip->ip_hl << 2);
	
		igmp        = (struct igmphdr *)(pktinfo->frame_ptr + iphdrlen);
		group.ipv4.s_addr       = igmp->group;
		mcastdatalen = ipdatalen - IGMP_MINLEN;
		if (mcastdatalen < 0) {
			dbprintf_igmp(LOG_WARNING, "received IP data field too short (%u bytes) for IGMP, from %s\n", ipdatalen, inetfmt(src.ipv4.s_addr, s1));
			return;
		}
	
		dbprintf_igmp(LOG_WARNING, "RECV %s from %-15s to %s\n", mcastpacketkind(igmp->type, igmp->code), inet_ipaddr(src, s1), inet_ipaddr(dst, s2));
		
		switch (igmp->type) {
		case IGMP_HOST_MEMBERSHIP_REPORT:
		case IGMPV2_HOST_MEMBERSHIP_REPORT:
			acceptgroupreport(&src, &group, igmp->type);
			break;
		case IGMPV3_HOST_MEMBERSHIP_REPORT:
	                mcast_proxy_acceptgroupreport(&src, (void *)igmp);
			break;
		case IGMP_HOST_LEAVE_MESSAGE:
			acceptleavemessage(&src, &group); // accept leave message only in IGMPv2/v3
			break;
		case IGMP_HOST_MEMBERSHIP_QUERY:
			break;
		default:
			dbprintf_igmp(LOG_WARNING, "ignoring unknown IGMP message type %x from %s to %s\n", igmp->type, inet_ipaddr(src, s1), inet_ipaddr(dst, s2));
		}		
	}
	else if (pktinfo->tci.ethertype == ETH_P_IPV6)
	{
		struct mld_hdr  *mldh = (struct mld_hdr  *)pktinfo->frame_ptr;

		struct in6_addr *ip6 = (struct in6_addr *)(pktinfo->frame_ptr + pktinfo->frame_len);
		src.ipv6 = *ip6;
		dst.ipv6 = *(ip6 + 1);
		
		group.ipv6     = mldh->mld_addr;
		
		dbprintf_igmp(LOG_WARNING, "RECV %s from %-15s to %s\n", mcastpacketkind(mldh->mld_type, mldh->mld_code), inet_ipaddr(src, s1), inet_ipaddr(dst, s2));
		
		switch (mldh->mld_type)
		{
		case MLD_LISTENER_REPORT:
			acceptgroupreport6(&src, &group, mldh->mld_type);
			break;
		case MLD_V2_LISTENER_REPORT:
			 mcast_proxy_acceptgroupreport(&src, (void *)mldh);
			break;
		case MLD_LISTENER_REDUCTION:
			acceptleavemessage6(&src, &group);
			break;
		case MLD_LISTENER_QUERY:
			break;
		default:
			dbprintf_igmp(LOG_WARNING, "ignoring unknown MLD message type %x from %s to %s\n", mldh->mld_type, inet_ipaddr(src, s1), inet_ipaddr(dst, s2));	
		}
		
	}
	else
	{
		dbprintf_igmp(LOG_ERR, "not supported ip version\n");
	}
	
}

static void mcast_proxy_remove_member(union ipaddr *grp, union ipaddr *src, union ipaddr *hst)
{
        struct mcast_proxy_grp_t *g = mcast_proxy_locate_group(grp);
        struct mcast_proxy_hst_t h;
        proxy_create_host(&h, hst);
        
        if (g == NULL) return;
        int hasSource = 0;
        
        if (IS_IPV6_ADDR(*grp))
        	hasSource = (IN6_IS_ADDR_UNSPECIFIED(&src->ipv6))?0:1;
	else
		hasSource = (src->ipv4.s_addr)?1:0;
	
	
        if (hasSource)
        {
                struct mcast_proxy_src_t *s = mcast_mcast_proxy_locate_group_source(g, src);
                if (s == NULL) return;
                mcast_proxy_source_host_list_del(s, &h);
        }
        else
        {
                mcast_proxy_group_host_list_del(g, &h);
        }
}

void mcast_proxy_member_update(struct mcast_mbship_t *m)
{
        // triggered by mcast_snoop.c
        dbprintf_igmp(LOG_WARNING, "remove %s, %s, %s\n", inet_ipaddr(m->group_ip, s1), inet_ipaddr(m->src_ip, s2), inet_ipaddr(m->client_ip, s3));
	mcast_interface *ifup = mcast_interface_find(IS_IPV6_ADDR(m->group_ip)?AF_INET6:AF_INET, INTERFACE_UPSTREAM);	 
        mcast_proxy_remove_member(&m->group_ip, &m->src_ip, &m->client_ip);
        struct query_obj query;
        query_obj_init(&query);
        mcast_proxy_tidy(&m->group_ip);
        mcast_proxy_forward(&ifup->if_addr, &m->group_ip, &query);
        query_obj_cleanup(&query);
}

int mcast_proxy_db_deinit()
{
        struct mcast_proxy_grp_t *pos, *n;
        struct mcast_proxy_src_t *pos1, *n1;
        struct mcast_proxy_hst_t *pos2, *n2;
        
        list_for_each_entry_safe(pos, n, &igmp_mbship_db_g.mcast_proxy_list, entry_node) {
                list_for_each_entry_safe(pos1, n1, &pos->src_list, entry_node) {
                        list_for_each_entry_safe(pos2, n2, &pos1->host_ssm_list, entry_node) {
                                mcast_proxy_free_host(pos2);      // free source hosts
                        }
                        list_del(&pos1->entry_node);
                        free_safe(pos1);      // free source
                }

                list_for_each_entry_safe(pos2, n2, &pos->host_asm_list, entry_node) {
                        mcast_proxy_free_host(pos2);      // free group hosts
                }                
                list_del(&pos->entry_node);
                free_safe(pos);        // free group                          
	}
	
	return 0;
}
