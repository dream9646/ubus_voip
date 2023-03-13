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
#include "util_inet.h"
#include "mcast_const.h"
#include "igmp_mbship.h"
#include "mcast_router.h"
#include "mcast_timer.h"
#include "fwk_timer.h"

//extern int timer_id_router_non_fast_leave_g;
extern struct igmp_config_t igmp_config_g;
extern struct igmp_mbship_db_t igmp_mbship_db_g;
extern char *send_buf; 
/*
 * Exported variables.
 */
char s1[19];        /* buffers to hold the string representations  */
char s2[19];        /* of IP addresses, to be passed to inet_fmt() */
char s3[19];        /* or inet_fmts().                             */
char s4[19];

// The upstream VIF index
extern int         upstreamvif;

// Globals                  
extern uint32_t   allhosts_group;          /* All hosts addr in net order */
extern uint32_t   allrouters_group;          /* All hosts addr in net order */

extern mcast_proxy mproxy;
extern int curttl;

#define GREC_NSRC_CHK(b , s)     {if (b) {dbprintf_igmp(LOG_WARNING, s);return -1;}}

#define GREC_SIZE(numsrc) \
         (sizeof(struct igmpv3_grec) + (numsrc) * sizeof(__u32))
         
struct query_obj
{
        struct igmpv3_grec *old;     // last socket group state
        struct igmpv3_grec *result;       // new query content        
};

extern int option;

struct mcast_proxy_hst_t{
        uint32_t host_ip;
        struct list_head entry_node;    
};

struct mcast_proxy_src_t{
        uint32_t src_ip;
        struct list_head host_ssm_list;
        struct list_head entry_node;    
};

typedef enum {
        IGMP_MODE_V3 = 4,
        IGMP_MODE_V2 = 2,
        IGMP_MODE_V1 = 1
}IGMP_MODE;


struct mcast_proxy_grp_t{
        uint32_t group_ip;
        IGMP_MODE mode;         // compatibility mode
        int hpt_timer_v1;       // host present timer for IGMPv1
        int hpt_timer_v2;       // host present timer for IGMPv2
        struct list_head host_asm_list;
        struct list_head src_list;
        struct list_head entry_node;    
};

static inline void igmp_v3_router_free_host(struct mcast_proxy_hst_t *host)
{
        list_del(&host->entry_node);
        free_safe(host);
}

static IGMP_MODE igmp_v3_get_querier_version()
{
        struct mcast_proxy_grp_t *pos, *n;
        IGMP_MODE mode = IGMP_MODE_V3;
        
        list_for_each_entry_safe(pos, n, &igmp_mbship_db_g.mcast_proxy_list, entry_node) {
                if (pos->mode < mode) mode = pos->mode;                                 
	}
	
	return mode;
}

/**
*   Finds the textual name of the supplied IGMP request.
*/
static char *igmppacketkind(u_int type, u_int code) {
    static char unknown[20];

    switch (type) {
    case IGMP_HOST_MEMBERSHIP_QUERY:    return "Membership query  ";
    case IGMP_HOST_MEMBERSHIP_REPORT:   return "V1 member report  ";
    case IGMPV2_HOST_MEMBERSHIP_REPORT: return "V2 member report  ";
    case IGMPV3_HOST_MEMBERSHIP_REPORT: return "V3 member report  ";
    case IGMP_HOST_LEAVE_MESSAGE:       return "Leave message     ";
    
    default:
        sprintf(unknown, "unk: 0x%02x/0x%02x    ", type, code);
        return unknown;
    }
}

static struct mcast_proxy_grp_t *proxy_locate_group(uint32_t group)
{
        struct mcast_proxy_grp_t *pos, *n;
        
        list_for_each_entry_safe(pos, n, &igmp_mbship_db_g.mcast_proxy_list, entry_node) {
                if (group == pos->group_ip)
                        return pos;                               
	}
        
        return NULL;        
}

static struct mcast_proxy_grp_t *proxy_create_group(uint32_t group)
{
        struct mcast_proxy_grp_t *g;
        
        g = malloc_safe(sizeof(struct mcast_proxy_grp_t));
        if (g == NULL) return NULL;
        INIT_LIST_NODE(&g->entry_node);
        g->group_ip = group;
        g->mode = IGMP_MODE_V3;         // default querier mode is igmpv3
        INIT_LIST_HEAD(&g->src_list);
        INIT_LIST_HEAD(&g->host_asm_list);
        list_add(&g->entry_node, &igmp_mbship_db_g.mcast_proxy_list);
          
        return g;        
}

static struct mcast_proxy_src_t *proxy_locate_group_source(struct mcast_proxy_grp_t *g, uint32_t src)
{
        struct mcast_proxy_src_t *pos, *n;
        
        list_for_each_entry_safe(pos, n, &g->src_list, entry_node) {
                if (src == pos->src_ip)
                        return pos;                               
	}
        
        return NULL;        
}

static struct mcast_proxy_src_t *proxy_create_group_source(struct mcast_proxy_grp_t *g, uint32_t src)
{
        struct mcast_proxy_src_t *s;
        
        if (src == 0) return NULL;
        s = malloc_safe(sizeof(struct mcast_proxy_src_t));
        if (s == NULL) return NULL;
        s->src_ip = src;
        INIT_LIST_HEAD(&s->host_ssm_list);
        INIT_LIST_NODE(&s->entry_node);
        list_add(&s->entry_node, &g->src_list);
        
        return s;        
}

static void proxy_create_host(struct mcast_proxy_hst_t *h, uint32_t hst)
{
        h->host_ip = hst;            
}

static void proxy_source_host_list_del(struct mcast_proxy_src_t *s, struct mcast_proxy_hst_t *h)
{
        struct mcast_proxy_hst_t *pos, *n;
        
        list_for_each_entry_safe(pos, n, &s->host_ssm_list, entry_node) {
                if (pos->host_ip == h->host_ip)
                {
                        igmp_v3_router_free_host(pos);
                        return;
                } 
        }                   
}

static void proxy_group_host_list_add(struct mcast_proxy_grp_t *g, struct mcast_proxy_src_t *s, struct mcast_proxy_hst_t *h)
{
        struct mcast_proxy_hst_t *pos, *n;
       
        list_for_each_entry_safe(pos, n, &g->host_asm_list, entry_node) {
                if (pos->host_ip == h->host_ip)
                {
                        //pos->access_time = h->access_time;  // existing entry
                        return;
                } 
        }
        
        struct mcast_proxy_hst_t *new_host;
        new_host = malloc_safe(sizeof(struct mcast_proxy_hst_t));
        new_host->host_ip = h->host_ip;
        memcpy(new_host, h, sizeof(struct mcast_proxy_hst_t));
        INIT_LIST_NODE(&new_host->entry_node);
        list_add(&new_host->entry_node, &g->host_asm_list);      // new entry
                
        // sanity check, remove host from all source list
        struct mcast_proxy_src_t *pos1, *n1;
        
        
        list_for_each_entry_safe(pos1, n1, &g->src_list, entry_node) {
                proxy_source_host_list_del(pos1, h);                              
	}                    
}

static void proxy_group_host_list_del(struct mcast_proxy_grp_t *g, struct mcast_proxy_hst_t *h)
{
        struct mcast_proxy_hst_t *pos, *n;
        list_for_each_entry_safe(pos, n, &g->host_asm_list, entry_node) {
                if (pos->host_ip == h->host_ip)
                {
                        igmp_v3_router_free_host(pos);
                        return;
                }    
        }                 
}

static void proxy_source_host_list_add(struct mcast_proxy_grp_t *g, struct mcast_proxy_src_t *s, struct mcast_proxy_hst_t *h)
{
        struct mcast_proxy_hst_t *pos, *n;
       
        list_for_each_entry_safe(pos, n, &s->host_ssm_list, entry_node) {
                if (pos->host_ip == h->host_ip)
                {
                        //pos->access_time = h->access_time;  // existing entry
                        return;
                } 
        }
        
        struct mcast_proxy_hst_t *new_host;
        new_host = malloc_safe(sizeof(struct mcast_proxy_hst_t));
        new_host->host_ip = h->host_ip;
        memcpy(new_host, h, sizeof(struct mcast_proxy_hst_t));
        INIT_LIST_NODE(&new_host->entry_node);
        list_add(&new_host->entry_node, &s->host_ssm_list);      // new entry
        
        // sanity check, remove host from group list
        proxy_group_host_list_del(g, h);                     
}

static void 
buildigmp(uint32_t src, uint32_t dst, int type, int code, struct igmpv3_grec *query, int *datalen) 
{
	struct ip *ip;
	struct igmphdr *igmp;
	extern int curttl;
	
	ip                      = (struct ip *)send_buf;
	ip->ip_v = IPVERSION;
	ip->ip_hl = (MIN_IP_HEADER_LEN + sizeof(option)) >> 2;
	ip->ip_src.s_addr       = src;
	ip->ip_dst.s_addr       = dst;

	if (IN_MULTICAST(ntohl(dst)))
		ip->ip_ttl = curttl;
	else
        	ip->ip_ttl = MAXTTL;
	ip->ip_tos = 0xc0;      /* Internet Control */
	ip->ip_p   = IPPROTO_IGMP;
            
	// fill router alert option
	unsigned char *p =  (unsigned char *)(send_buf + MIN_IP_HEADER_LEN);
	*((unsigned int *)p) = htonl(option); 
        
	igmp                    = (struct igmphdr *)(send_buf + MIN_IP_HEADER_LEN + sizeof(option));
	igmp->type         = type;
	igmp->code         = code;
	igmp->group        = query->grec_mca;
	*datalen = sizeof(struct igmphdr);
	igmp->csum = 0;         
	igmp->csum         = inetchksum((u_short *)igmp, *datalen);
	*datalen = MIN_IP_HEADER_LEN + sizeof(option) + *datalen;
	ip->ip_len = htons(*datalen);
	dbprintf_igmp(LOG_WARNING, "build igmp%s %s query.\n", (igmp->code)?"v2":"v1", (igmp->group)?"group specific":"general");
}

static void 
buildigmpv3(uint32_t src, uint32_t dst, int type, int code, struct igmpv3_grec *query, int *datalen) 
{
	struct ip *ip;
	struct igmpv3_query *igmp;
	extern int curttl;
	
	ip                      = (struct ip *)send_buf;
	ip->ip_v = IPVERSION;
	ip->ip_hl = (MIN_IP_HEADER_LEN + sizeof(option)) >> 2;
	ip->ip_src.s_addr       = src;
	ip->ip_dst.s_addr       = dst;

	if (IN_MULTICAST(ntohl(dst)))
		ip->ip_ttl = curttl;
	else
        	ip->ip_ttl = MAXTTL;
    	
	ip->ip_tos = 0xc0;      /* Internet Control */
	ip->ip_p   = IPPROTO_IGMP;
            
	// fill router alert option
	unsigned char *p =  (unsigned char *)(send_buf + MIN_IP_HEADER_LEN);
	*((unsigned int *)p) = htonl(option); 
        
	igmp                    = (struct igmpv3_query *)(send_buf + MIN_IP_HEADER_LEN + sizeof(option));
	igmp->type         = type;
	igmp->code         = code;
	igmp->group        = query->grec_mca;
	igmp->qrv          = igmp_config_g.robustness;
	igmp->qqic         = igmp_config_g.query_interval;
	igmp->nsrcs        = query->grec_nsrcs;
    
    	int i;
	for (i=0;i < query->grec_nsrcs;i++)
	{
        	igmp->srcs[i] = query->grec_src[i];
    	}
	*datalen = sizeof(struct igmpv3_query) + i * sizeof(igmp->srcs[0]);
	igmp->csum = 0;         
	igmp->csum         = inetchksum((u_short *)igmp, *datalen);
    	*datalen = MIN_IP_HEADER_LEN + sizeof(option) + *datalen;
	ip->ip_len = htons(*datalen);
	dbprintf_igmp(LOG_WARNING, "build igmpv3 %s%s %s query.\n", (igmp->group)?"group":"general",(igmp->nsrcs)?"-source":"", (igmp->group)?"specific":"");
}

static void grec_difference(struct query_obj *query, struct igmpv3_grec *B)
{          
        struct igmpv3_grec *A = query->old;
        int size = A->grec_nsrcs;
        
        query->result = (struct igmpv3_grec *)malloc_safe(GREC_SIZE(size));
        if (query->result == NULL)     return;
        
        memset(query->result, 0, GREC_SIZE(size));
        query->result->grec_mca = A->grec_mca;
        
        int i,j;
        for (i=0;i < A->grec_nsrcs ;i++)
        {
                int found = 0;
                for (j=0;j < B->grec_nsrcs;j++)
                {
                        if (A->grec_src[i] == B->grec_src[j])
                        {
                                found = 1;
                                break;      
                        }
                }
                if (!found)
                {
                        query->result->grec_src[query->result->grec_nsrcs] = A->grec_src[i];
                        query->result->grec_nsrcs++;    
                }
        }
        
        dbprintf_igmp(LOG_WARNING, "(%d * %d)=%d\n", A->grec_nsrcs, B->grec_nsrcs, query->result->grec_nsrcs);           
}

static void grec_intersection(struct query_obj *query, struct igmpv3_grec *B)
{      
        struct igmpv3_grec *A = query->old;
        int size = (A->grec_nsrcs > B->grec_nsrcs)?A->grec_nsrcs:B->grec_nsrcs;
         
        query->result = (struct igmpv3_grec  *)malloc_safe(GREC_SIZE(size));
        if (query->result == NULL)     return;
        
        memset(query->result, 0, GREC_SIZE(size));
        query->result->grec_mca = A->grec_mca;
        
        int i,j;
        for (i=0;i < A->grec_nsrcs ;i++)
        {
                for (j=0;j < B->grec_nsrcs;j++)
                {
                        if (A->grec_src[i] == B->grec_src[j])
                        {
                                query->result->grec_src[query->result->grec_nsrcs] = A->grec_src[i];
                                query->result->grec_nsrcs++;        
                        }
                }
        }
        dbprintf_igmp(LOG_WARNING, "(%d * %d)=%d\n", A->grec_nsrcs, B->grec_nsrcs, query->result->grec_nsrcs);     
}


static int process_lw_igmp_report(int grec_type, uint32_t grp, uint32_t src, uint32_t hst)
{
        struct mcast_proxy_hst_t h;
        proxy_create_host(&h, hst);
        
        struct mcast_proxy_grp_t *g = proxy_locate_group(grp);
        if (g == NULL) g = proxy_create_group(grp);
        
        struct mcast_proxy_src_t *s = proxy_locate_group_source(g, src);
        if (s == NULL) s = proxy_create_group_source(g, src);        
        
        switch(grec_type)
        {
        case IGMPV3_MODE_IS_INCLUDE: // source A -> A + B, (B) = GMI
                GREC_NSRC_CHK((s == NULL),"query response to (*, G) leave\n") // should never happen
                dbprintf_igmp(LOG_WARNING,"query response to (x, G) join\n");
                proxy_source_host_list_add(g, s, &h);
                break;
        case IGMPV3_MODE_IS_EXCLUDE:   // source A -> A, G_Timer = GMI
                GREC_NSRC_CHK((s != NULL),"query response to EXCLUDE (x, G) join is not supported\n")
                dbprintf_igmp(LOG_WARNING,"query response to (*, G) join\n");
                proxy_group_host_list_add(g, s, &h);
                break;
        case IGMPV3_CHANGE_TO_INCLUDE: // source A -> A + B, Send Q(G, A-B), If G_Timer > 0, Send Q(G)
                if (s == NULL)
                {
                        dbprintf_igmp(LOG_WARNING,"(*, G) leave\n");
                        if (igmp_config_g.immediate_leave)
                                proxy_group_host_list_del(g, &h);
                }
                else
                {
                        dbprintf_igmp(LOG_WARNING,"change to INCLUDE (x, G) join\n");
                        proxy_source_host_list_add(g, s, &h);
                }
                break;
        case IGMPV3_CHANGE_TO_EXCLUDE:    // source A -> A, G_Timer = GMI
                GREC_NSRC_CHK((s != NULL),"change to EXCLUDE (x, G) join is not supported\n")
                dbprintf_igmp(LOG_WARNING,"(*, G) join\n");  // (*,G) join
                proxy_group_host_list_add(g, s, &h);
                break;
        case IGMPV3_ALLOW_NEW_SOURCES: // soruce A -> A + B, (B) = GMI
                dbprintf_igmp(LOG_WARNING,"INCLUDE(x,G) join\n");
                GREC_NSRC_CHK((s == NULL), "INCLUDE(x,G) join contains no source\n")
                proxy_source_host_list_add(g, s, &h);
                break;
        case IGMPV3_BLOCK_OLD_SOURCES: // source A -> A, Send Q(G, A*B) 
                dbprintf_igmp(LOG_WARNING,"INCLUDE(x,G) leave\n");
                GREC_NSRC_CHK((s == NULL), "INCLUDE(x,G) leave contains no source\n")
                if (igmp_config_g.immediate_leave) proxy_source_host_list_del(s, &h);
                break;
        default:
                break;
        }
        
        return 0;             
}

static int sanitycheck_downstream(uint32_t src, uint32_t group)
{   
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
        
	return 1;
}

static int proxy_get_group_source_count(uint32_t group)
{
        int count = 0;
        struct mcast_proxy_grp_t *grp = proxy_locate_group(group);
        
        if (grp == NULL) return count;
        
        struct mcast_proxy_src_t *pos, *n;
        list_for_each_entry_safe(pos, n, &grp->src_list, entry_node) {
                count++;
        }
        
        return count;        
}

static int proxy_get_group_host_count(uint32_t group)
{
        int count = 0;
        struct mcast_proxy_grp_t *grp = proxy_locate_group(group);
        
        if (grp == NULL) return count;
        
        struct mcast_proxy_hst_t *pos, *n;
        list_for_each_entry_safe(pos, n, &grp->host_asm_list, entry_node) {
                count++;
        }
        
        return count;        
}

static struct ip_msfilter *proxy_get_msfilter_by_group(uint32_t group)
{
        int count = proxy_get_group_source_count(group);
        int i = 0;
        
        struct ip_msfilter *imsfp = (struct ip_msfilter *)malloc_safe(IP_MSFILTER_SIZE(count));
        imsfp->imsf_numsrc = count;
        imsfp->imsf_fmode = (proxy_get_group_host_count(group))?MCAST_EXCLUDE:MCAST_INCLUDE;
        
        struct mcast_proxy_src_t *pos, *n;
        struct mcast_proxy_grp_t *grp = proxy_locate_group(group);
        if (grp == NULL) return imsfp;
        list_for_each_entry_safe(pos, n, &grp->src_list, entry_node) {
               imsfp->imsf_slist[i].s_addr = pos->src_ip;
               i++;
        }
        
        return imsfp;
}

static void poxy_log_status()
{
        struct mcast_proxy_grp_t *pos_lw, *n_lw;
        int i = 0;
	char group_ip[INET6_ADDRSTRLEN];
	char client_ip[INET6_ADDRSTRLEN];
	
	if (omci_env_g.debug_level_igmp <= LOG_WARNING) return;
	
	int fd = util_dbprintf_get_console_fd();
        list_for_each_entry_safe(pos_lw, n_lw, &igmp_mbship_db_g.mcast_proxy_list, entry_node) {
                struct mcast_proxy_src_t *pos1, *n1;
                struct mcast_proxy_hst_t *pos2, *n2;
                util_fdprintf(fd, "[%d] Group-%-15s (%s), querier-%d\n\t-ASM hosts:\n", i, util_inet_ntop(AF_INET, (unsigned char *)(&pos_lw->group_ip), (char *)&group_ip, 16), (list_empty(&pos_lw->host_asm_list))?"SSM":"ASM", pos_lw->mode);
                list_for_each_entry_safe(pos2, n2, &pos_lw->host_asm_list, entry_node) {
                                util_fdprintf(fd, "\t\t-%-15s\n", util_inet_ntop(AF_INET, (unsigned char *)(&pos2->host_ip), (char *)&client_ip, 16));
                } 
                
                list_for_each_entry_safe(pos1, n1, &pos_lw->src_list, entry_node) {
                        dbprintf_igmp(LOG_WARNING,"\t-Source %-15s, SSM hosts:\n", util_inet_ntop(AF_INET, (unsigned char *)(&pos1->src_ip), (char *)&group_ip, 16));
                        list_for_each_entry_safe(pos2, n2, &pos1->host_ssm_list, entry_node) {
                                util_fdprintf(fd, "\t\t-%-15s\n", util_inet_ntop(AF_INET, (unsigned char *)(&pos2->host_ip), (char *)&client_ip, 16));
                        }       
                }
                i++;
                util_fdprintf(fd, "\n");                              
	}
}


static void msfilter_to_grec(struct ip_msfilter *imsfp, struct query_obj *query)
{        
        query->old = (struct igmpv3_grec *)malloc_safe(GREC_SIZE(imsfp->imsf_numsrc));
        if (query->old == NULL) return;
        query->old->grec_mca = imsfp->imsf_multiaddr.s_addr;
        query->old->grec_nsrcs = imsfp->imsf_numsrc;
        
        int i;
        for (i=0;i < imsfp->imsf_numsrc;i++)
                query->old->grec_src[i] = imsfp->imsf_slist[i].s_addr;   
}

#define MAX_SOURCES     64
 
static void proxy_forward(struct ifdesc *ifdp, uint32_t mcastaddr, struct query_obj *query)
{
        // backup current source membership
        unsigned char buffer[IP_MSFILTER_SIZE(MAX_SOURCES)];
        struct ip_msfilter *imsfp = (struct ip_msfilter *)buffer;
        imsfp->imsf_multiaddr.s_addr = mcastaddr;
        imsfp->imsf_interface.s_addr = ifdp->inadr.s_addr;
        imsfp->imsf_numsrc = MAX_SOURCES;        // must set this else kernel will use it
                                        // instead of actual source count
        int size = sizeof(buffer);         
        if(getsockopt(mproxy.igmp_udp_socket, IPPROTO_IP, IP_MSFILTER, (void *)imsfp, &size)) 
        {
                dbprintf_igmp(LOG_WARNING, "getsockopt IP_MSFILTER failed:%s(%s, %s)\n", strerror(errno), inetfmt(mcastaddr, s1), inetfmt(ifdp->inadr.s_addr, s2));
        }
        else
        {
	        dbprintf_igmp(LOG_WARNING, "%s mode is %s, numsrc %d\n", inetfmt(mcastaddr, s1), (imsfp->imsf_fmode == MCAST_EXCLUDE)?"exclude":"include", imsfp->imsf_numsrc);
                msfilter_to_grec(imsfp, query);
        }
         
        // recompute source membership & update
        int isJoin = (proxy_locate_group(mcastaddr))?1:0;
        struct ip_mreq ctlreq;
        ctlreq.imr_multiaddr.s_addr = mcastaddr;
        ctlreq.imr_interface.s_addr = ifdp->inadr.s_addr;
        if(isJoin && setsockopt(mproxy.igmp_udp_socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, 
                &ctlreq, sizeof(ctlreq))) 
        {
                dbprintf_igmp(LOG_WARNING, "%s failed:%s\n", "MRT_ADD_MEMBERSHIP", strerror(errno));
        }
                
        imsfp = proxy_get_msfilter_by_group(mcastaddr);
        if (imsfp == NULL) return;
        imsfp->imsf_multiaddr.s_addr = mcastaddr;
        imsfp->imsf_interface.s_addr = ifdp->inadr.s_addr;
        
        if (isJoin)
        {
                if (imsfp->imsf_fmode == MCAST_EXCLUDE)
                        imsfp->imsf_numsrc = 0; // RFC 5970
        }
        else
        {
                imsfp->imsf_fmode = MCAST_INCLUDE;
                imsfp->imsf_numsrc = 0;        
        }
        
        if(setsockopt(mproxy.igmp_udp_socket, IPPROTO_IP, IP_MSFILTER, 
                imsfp, IP_MSFILTER_SIZE(imsfp->imsf_numsrc))) 
        {
                dbprintf_igmp(LOG_WARNING, "setsockopt IP_MSFILTER failed:%s(%s, %s)\n", strerror(errno) , inetfmt(mcastaddr, s1), inetfmt(ifdp->inadr.s_addr, s2));
        }
        else
                dbprintf_igmp(LOG_WARNING, "Set %s IP_MSFILTER to %s mode\n", inetfmt(mcastaddr, s1), (imsfp->imsf_fmode == MCAST_EXCLUDE)?"exclude":"include");
        
        if(!isJoin && setsockopt(mproxy.igmp_udp_socket, IPPROTO_IP, IP_DROP_MEMBERSHIP, 
                &ctlreq, sizeof(ctlreq))) 
        {
                dbprintf_igmp(LOG_WARNING, "%s failed:%s\n", "MRT_DROP_MEMBERSHIP", strerror(errno));
        }
                
        free_safe(imsfp);
                    
}

static void proxy_tidy(uint32_t mcastaddr)
{
        struct mcast_proxy_grp_t *pos, *n;
        struct mcast_proxy_src_t *pos1, *n1;
        
        struct mcast_proxy_grp_t *g = proxy_locate_group(mcastaddr);
        if (g == NULL) return;
        
        list_for_each_entry_safe(pos, n, &igmp_mbship_db_g.mcast_proxy_list, entry_node) {
                if (g->group_ip == pos->group_ip)
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

static void sendigmpv3query(uint32_t src, struct igmpv3_grec *query, int code)
{
	struct sockaddr_in sdst;
	int setloop = 0, setigmpsource = 0;
	int datalen = 0;
	int dst = allhosts_group;
	struct ifdesc *dp = getifbyaddress(src);
    
	if (dp == NULL) {
		dbprintf_igmp(LOG_WARNING, "No interfaces found for source %s\n", inetfmt(src, s1));
        	return;
	}
    
	IGMP_MODE mode = igmp_v3_get_querier_version();
    
	unsigned int tmp = 0;
	switch(mode)
	{
	case IGMP_MODE_V1:
		// ignore group specific or group-source specific query in IGMPv1
		if (igmp_config_g.igmp_compatibity_mode && (query->grec_nsrcs || query->grec_mca)) return;
		buildigmp(dp->inadr.s_addr, dst, IGMP_MEMBERSHIP_QUERY, code, query, &datalen);
		break;
	case IGMP_MODE_V2:
        	// ignore group-source specific query in IGMPv2
        	if (igmp_config_g.igmp_compatibity_mode && query->grec_nsrcs)  return;
        	buildigmp(dp->inadr.s_addr, dst, IGMP_MEMBERSHIP_QUERY, code, query, &datalen);
        	break;
	case IGMP_MODE_V3:
		if (code < 128)
			tmp = code;
		else
		{
			int exp;
        		int mant;
        		tmp = code;
        		int pos = 0;
			for (pos = 15 ; pos >= 7; pos--)
			{
				if (tmp & (1 << pos))
				{
					exp = pos - 5 + 1;
		                	mant = (tmp & (0xF << exp)) >> exp;
		                	exp -= 3;
		                	tmp = (1 << 7) | (exp << 4) | mant;
		                	break;
	            		}
	        	}
		}
        	buildigmpv3(dp->inadr.s_addr, dst, IGMP_MEMBERSHIP_QUERY, tmp, query, &datalen);
        	break;
    	}
    
	if (IN_MULTICAST(ntohl(dst)))
	{
		k_set_if(dp->inadr.s_addr);
	        setigmpsource = 1;
		if ( dst == allhosts_group)
		{
	            setloop = 1;
	            k_set_loop(1);
	        }
	}
    
	memset(&sdst, 0, sizeof(sdst));
	sdst.sin_family = AF_INET;
#ifdef HAVE_STRUCT_SOCKADDR_IN_SIN_LEN
    	sdst.sin_len = sizeof(sdst);
#endif
    	sdst.sin_addr.s_addr = dst;
    
	if (sendto(mproxy.igmp_socket, send_buf, datalen, 0, (struct sockaddr *)&sdst, sizeof(sdst)) < 0)
		dbprintf_igmp(LOG_WARNING, "send error %s.\n", strerror(errno));
	else
		dbprintf_igmp(LOG_WARNING, "sendto built query to %s from %s\n", inetfmt(dst, s1), inetfmt(src, s2));

	if(setigmpsource)
	{
        	if (setloop)
		{
            		k_set_loop(0);
        	}
        	// Restore original...
        	k_set_if(INADDR_ANY);
	}
}

static void sendigmpv3groupquery(uint32_t src, uint32_t grp)
{
        struct igmpv3_grec gq;
        gq.grec_nsrcs = 0;
        gq.grec_mca = grp;
        sendigmpv3query(src, &gq, igmp_config_g.last_member_query_interval);
}

void sendigmpv3generalquery(uint32_t src)
{
	struct  ifdesc  *dp;
	int             ix;

	// loop through all downstream vifs...
	for (ix = 0; (dp = getifbyix(ix)); ix++)
	{
		if (dp->inadr.s_addr && ! (dp->flags & IFF_LOOPBACK))
		{
            		if (dp->state == IF_STATE_DOWNSTREAM)
			{
                		struct igmpv3_grec gq;
                		gq.grec_nsrcs = 0;
                		gq.grec_mca = 0;
                		sendigmpv3query(dp->inadr.s_addr, &gq, igmp_config_g.max_respon_time);
			}
        	}
	}
}

static void proxy_querier(uint32_t src, struct query_obj *query, struct igmpv3_grec *grec)
{
        if (query->old == NULL) return;
        switch(grec->grec_type)
        {
        case IGMPV3_MODE_IS_INCLUDE: // source A -> A + B, (B) = GMI
        case IGMPV3_MODE_IS_EXCLUDE:   // source A -> A, G_Timer = GMI
        case IGMPV3_CHANGE_TO_EXCLUDE:    // source A -> A, G_Timer = GMI
        case IGMPV3_ALLOW_NEW_SOURCES: // soruce A -> A + B, (B) = GMI
                break;
        case IGMPV3_CHANGE_TO_INCLUDE: // source A -> A + B, Send Q(G, A-B), If G_Timer > 0, Send Q(G)
                grec_difference(query, grec);
                sendigmpv3query(src, query->result, igmp_config_g.last_member_query_interval);
                //if (proxy_get_group_host_count(grec->grec_mca))
                        sendigmpv3groupquery(src, grec->grec_mca);
                break;
        case IGMPV3_BLOCK_OLD_SOURCES: // source A -> A, Send Q(G, A*B)
                grec_intersection(query, grec);
                if (query->result->grec_nsrcs)sendigmpv3query(src, query->result, igmp_config_g.last_member_query_interval); 
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
        struct mcast_proxy_grp_t *g = proxy_locate_group(msg->mcastaddr);
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
}

static void proxy_compatibility_mode(int type, uint32_t mcastaddr)
{
        struct mcast_proxy_grp_t *g = proxy_locate_group(mcastaddr);
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
        
        dbprintf_igmp(LOG_WARNING, "group %s, querier version %d\n", inetfmt(mcastaddr, s1), g->mode);
}

/**
*   Handles incoming IGMPv3 membership reports
*   RFC 5790 lightweight IGMPv3 is implemented
*/
static void 
acceptgroupreport3(uint32_t src, struct igmpv3_report *igmp) {
	short i, j;
	struct ifdesc*      upstrif;
    
	if (upstreamvif <0 || (upstrif = getifbyix( upstreamvif)) == NULL)
	{
		dbprintf_igmp(LOG_WARNING, "FATAL: Unable to get Upstream IF.\n");
		return;
	}
	
	struct igmpv3_grec *grec = &igmp->grec[0];
	    	
	for (i = 0;i < igmp->ngrec;i++)
	{
        	if (!sanitycheck_downstream(src, grec->grec_mca)) continue;  // skip this group
        	dbprintf_igmp(LOG_WARNING, "Got IGMPv3 report from %s to %s\n",inetfmt(src, s1), inetfmt(grec->grec_mca, s2));
        
        	// treate IS_EX({}) as TO_EX({}) 
        	if (grec->grec_type == IGMPV3_MODE_IS_EXCLUDE)
                	grec->grec_type = IGMPV3_CHANGE_TO_EXCLUDE;
        
        	struct query_obj query;
        	query_obj_init(&query);
        
        	if (grec->grec_nsrcs == 0)
                	process_lw_igmp_report(grec->grec_type, grec->grec_mca, 0, src);
        	else
                	for (j=0;j < grec->grec_nsrcs;j++)
                        	process_lw_igmp_report(grec->grec_type, grec->grec_mca, grec->grec_src[j], src);
        
        	if (igmp_config_g.igmp_compatibity_mode) proxy_compatibility_mode(igmp->type, grec->grec_mca);
        	proxy_tidy(grec->grec_mca);
        	proxy_forward(upstrif, grec->grec_mca, &query);
        	proxy_querier(src, &query, grec);
        	query_obj_cleanup(&query);
                
		grec = (struct igmpv3_grec *)((unsigned char *)grec + sizeof(struct igmpv3_grec) + grec->grec_nsrcs * sizeof(__u32));
                
        	poxy_log_status();
	}
}

static unsigned char tbuffer[128];
/**
*   Handles incoming membership IGMPv1/v2 reports, and
*   appends them to the routing table.
*/
static void 
acceptgroupreport(uint32_t src, uint32_t group, int type) {
        
	if (!sanitycheck_downstream(src, group)) return;
	dbprintf_igmp(LOG_WARNING, "Got IGMPv1/v2 report from %s to %s. \n",inetfmt(src, s1), inetfmt(group, s2)); 
        
	// RFC 5970 requires all IGMPv1/v2 report to be treated as TO_EX({})
	struct igmpv3_report *r = (struct igmpv3_report *)tbuffer;
	r->type = type;
	r->ngrec = 1;
	r->grec[0].grec_type = IGMPV3_CHANGE_TO_EXCLUDE;
	r->grec[0].grec_auxwords = 0;
	r->grec[0].grec_nsrcs = 0;
	r->grec[0].grec_mca = group; 
    
	acceptgroupreport3(src, r);

}

/**
*   Recieves and handles a IGMPv2 group leave message.
*/
static void 
acceptleavemessage(uint32_t src, uint32_t group) {
    
	if (!sanitycheck_downstream(src, group)) return; 
	dbprintf_igmp(LOG_WARNING, "Got IGMPv2 leave from %s to %s. \n",inetfmt(src, s1), inetfmt(group, s2));
    
	// RFC 5970 requires all IGMPv1/v2 report to be treated as TO_IN({})
	struct igmpv3_report *r = (struct igmpv3_report *)tbuffer;
	r->type = IGMPV3_HOST_MEMBERSHIP_REPORT;
	r->ngrec = 1;
	r->grec[0].grec_type = IGMPV3_CHANGE_TO_INCLUDE;
	r->grec[0].grec_auxwords = 0;
	r->grec[0].grec_nsrcs = 0;
	r->grec[0].grec_mca = group;
    
	acceptgroupreport3(src, r);
}

void igmp_v3_lw_router_compatibility_filter(struct igmp_clientinfo_t **igmp_clientinfo)
{
        if (!omci_env_g.igmp_proxy_snooping)            return; // only for proxy
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
	int ipdatalen, iphdrlen;
	struct ip *ip = (struct ip *)((*igmp_clientinfo)->pktinfo->frame_ptr);
	iphdrlen  = ip->ip_hl << 2;
	ipdatalen = ntohs(ip->ip_len) - (ip->ip_hl << 2);
        igmp        = (struct igmphdr *)((*igmp_clientinfo)->pktinfo->frame_ptr + iphdrlen);
        igmp->type = 0xff;	
}

void igmp_v3_lw_acceptigmp(struct cpuport_info_t *pktinfo) 
{
	register uint32_t src, dst, group;
	struct ip *ip;
	struct igmphdr *igmp;
	int ipdatalen, iphdrlen, igmpdatalen;

	if (pktinfo->frame_len < sizeof(struct ip))
	{
		dbprintf_igmp(LOG_WARNING, "received packet too short (%u bytes) for IP header\n", pktinfo->frame_len);
		return;
	}

	ip        = (struct ip *)pktinfo->frame_ptr;
	src       = ip->ip_src.s_addr;
	dst       = ip->ip_dst.s_addr;

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
	group       = igmp->group;
	igmpdatalen = ipdatalen - IGMP_MINLEN;
	if (igmpdatalen < 0) {
		dbprintf_igmp(LOG_WARNING, "received IP data field too short (%u bytes) for IGMP, from %s\n", ipdatalen, inetfmt(src, s1));
		return;
	}

	dbprintf_igmp(LOG_WARNING, "RECV %s from %-15s to %s\n", igmppacketkind(igmp->type, igmp->code), inetfmt(src, s1), inetfmt(dst, s2) );
	
	switch (igmp->type) {
	case IGMP_HOST_MEMBERSHIP_REPORT:
	case IGMPV2_HOST_MEMBERSHIP_REPORT:
		acceptgroupreport(src, group, igmp->type);
		break;
	case IGMPV3_HOST_MEMBERSHIP_REPORT:
                acceptgroupreport3(src, (struct igmpv3_report *)igmp);
		break;
	case IGMP_HOST_LEAVE_MESSAGE:
		acceptleavemessage(src, group); // accept leave message only in IGMPv2/v3
		break;
	case IGMP_HOST_MEMBERSHIP_QUERY:
		break;
	default:
		dbprintf_igmp(LOG_WARNING, "ignoring unknown IGMP message type %x from %s to %s\n", igmp->type, inetfmt(src, s1), inetfmt(dst, s2));
	}
}

static void proxy_remove_member(uint32_t grp, uint32_t src, uint32_t hst)
{
        struct mcast_proxy_grp_t *g = proxy_locate_group(grp);
        struct mcast_proxy_hst_t h;
        proxy_create_host(&h, hst);
        
        if (g == NULL) return;
        if (src)
        {
                struct mcast_proxy_src_t *s = proxy_locate_group_source(g, src);
                if (s == NULL) return;
                proxy_source_host_list_del(s, &h);
        }
        else
        {
                proxy_group_host_list_del(g, &h);
        }
}

void igmp_v3_lw_router_member_update(struct igmp_mbship_t *m)
{
        // triggered by igmp_v3_lw.c
	struct ifdesc*      upstrif;
    
	if (upstreamvif <0 || (upstrif = getifbyix( upstreamvif)) == NULL)
	{
		dbprintf_igmp(LOG_WARNING, "FATAL: Unable to get Upstream IF.\n");
		return;
	}
	
        dbprintf_igmp(LOG_WARNING, "remove %s, %s, %s\n", inetfmt(m->group_ip.ipv4.s_addr, s1), inetfmt(m->src_ip.ipv4.s_addr, s2), inetfmt(m->client_ip.ipv4.s_addr, s3)); 
        proxy_remove_member(m->group_ip.ipv4.s_addr, m->src_ip.ipv4.s_addr, m->client_ip.ipv4.s_addr);
        struct query_obj query;
        query_obj_init(&query);
        proxy_tidy(m->group_ip.ipv4.s_addr);
        proxy_forward(upstrif, m->group_ip.ipv4.s_addr, &query);
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
                                igmp_v3_router_free_host(pos2);      // free source hosts
                        }
                        list_del(&pos1->entry_node);
                        free_safe(pos1);      // free source
                }

                list_for_each_entry_safe(pos2, n2, &pos->host_asm_list, entry_node) {
                        igmp_v3_router_free_host(pos2);      // free group hosts
                }                
                list_del(&pos->entry_node);
                free_safe(pos);        // free group                          
	}
	
	return 0;
}
