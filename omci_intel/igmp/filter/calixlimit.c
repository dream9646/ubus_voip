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
 * File    : calixlimit.c
 *
 ******************************************************************/
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/ip.h>

#include "util_inet.h"
#include "list.h"
#include "util.h"
#include "switch.h"
#include "cli.h"

#include "igmp_mbship.h"
#include "mcast_snoop.h"
#include "mcast.h"
#include "mcast_const.h"
#include "platform.h"

extern struct igmp_mbship_db_t igmp_mbship_db_g;

static short bEnableZeroMeansUnlimited = 1;	// enable 0 means unlimited number

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

static int igmp_v3_lw_max_group_filter(struct igmp_grprec_t *r)
{ 
	if (bEnableZeroMeansUnlimited && igmp_config_g.group_limit == 0)	return 0;	// let through
	
	if (!igmp_v3_lw_record_type_join(r->rec->record_type, &r->rec->src_ip)) return 0; // let through
	// iterates through host list

        struct mcast_snoop_grp_t *pos, *n;
        int grp_count = 0;
        
        list_for_each_entry_safe(pos, n, &igmp_mbship_db_g.mcast_snoop_list, entry_node) {
                if (memcmp(&pos->group_ip, &r->rec->group_ip, sizeof(union ipaddr)) == 0) return 0;	// exist already
		grp_count++;                               
	}
        
        if (grp_count >= igmp_config_g.group_limit) return 1;	// exceeed group limit		
	return 0;
}

static int igmp_v3_lw_max_group_ssm_filter(struct igmp_grprec_t *r)
{
	if (bEnableZeroMeansUnlimited && igmp_config_g.max_ssm_source == 0)	return 0;	// let through
	
	if (!igmp_v3_lw_record_type_join(r->rec->record_type, &r->rec->src_ip)) return 0; // let through
        
        int src_count = 0;
        struct mcast_snoop_grp_t *grp_pos, *grp_n;
        
        list_for_each_entry_safe(grp_pos, grp_n, &igmp_mbship_db_g.mcast_snoop_list, entry_node) {
                if (memcmp(&grp_pos->group_ip, &r->rec->group_ip, sizeof(union ipaddr)) == 0)
                {
                	struct mcast_snoop_src_t *src_pos, *src_n;
                	list_for_each_entry_safe(src_pos, src_n, &grp_pos->src_list, entry_node) {
				if (memcmp(&src_pos->src_ip, &r->rec->src_ip, sizeof(union ipaddr)) == 0) return 0;	// exisit already
				src_count++;       
                	}
                	break;	// found matching group
		}                   
	}
        
        if (src_count > igmp_config_g.max_ssm_source) return 1;	// exceeed source limit
		
	return 0;
}

static int igmp_v3_lw_max_group_member_filter(struct igmp_grprec_t *r)
{
	if (bEnableZeroMeansUnlimited && igmp_config_g.max_group_member == 0)	return 0;	// let through
	
	if (!igmp_v3_lw_record_type_join(r->rec->record_type, &r->rec->src_ip)) return 0; // let through

        int mem_count = 0;
        struct mcast_snoop_grp_t *grp_pos, *grp_n;  
        
        list_for_each_entry_safe(grp_pos, grp_n, &igmp_mbship_db_g.mcast_snoop_list, entry_node) {
                if (memcmp(&grp_pos->group_ip, &r->rec->group_ip, sizeof(union ipaddr)) == 0)
                {
                	struct mcast_snoop_src_t *src_pos, *src_n;
                	struct mcast_mbship_t *pos, *n;
                	
        	        list_for_each_entry_safe(pos, pos, &grp_pos->host_asm_list, entry_node_v3) {
                                if (memcmp(&pos->client_ip, &r->client_ip, sizeof(union ipaddr)) == 0) return 0;                                		
				mem_count++;
	                }	// ASM 
	                
                	list_for_each_entry_safe(src_pos, src_n, &grp_pos->src_list, entry_node) {
				
                        	list_for_each_entry_safe(pos, n, &src_pos->host_ssm_list, entry_node_v3) {
                                	if (memcmp(&pos->client_ip, &r->client_ip, sizeof(union ipaddr)) == 0) return 0;                                		
					mem_count++;			
                        	}
                	}	// SSM
		}                   
	}
	
        if (mem_count > igmp_config_g.max_group_member) return 1;	// exceeed source limit
		
	return 0;
}

static int
igmp_filter_calixlimit_filter(struct igmp_grprec_t *r)
{
	if ((strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX) != 0))	return MCAST_FILTER_CONTINUE;
	
        if (igmp_v3_lw_max_group_filter(r)) return MCAST_FILTER_STOP; // exceed group count
        if (igmp_v3_lw_max_group_member_filter(r)) return MCAST_FILTER_STOP; // exceed group member count
	if (igmp_v3_lw_max_group_ssm_filter(r)) return MCAST_FILTER_STOP; // exceed group ssm count
	
	return MCAST_FILTER_CONTINUE;
}

// module callbacks
static const char * igmp_filter_calixlimit_name(void)
{
	return "calixlimit";
}

static int igmp_filter_calixlimit_init(void)
{	
	
	return MCAST_FILTER_CONTINUE;
}

static int igmp_filter_calixlimit_deinit(void)
{	
	return MCAST_FILTER_CONTINUE;
}

static int
igmp_filter_calixlimit_raw_filter(struct cpuport_info_t *pktinfo)
{
	// in calix v1 proxy without snooping, multicast traffic is not forwarded
	if (THREAD_ENV.igmp_proxy_snooping == 1) return MCAST_FILTER_STOP;
	
	return MCAST_FILTER_CONTINUE;
	
}

static int
cli_igmp_config_list(int fd)
{
	if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX) == 0)
	{
		util_fdprintf(fd, "\tmax group limit: %d\n",igmp_config_g.group_limit);
		util_fdprintf(fd, "\tmax group member limit: %d\n",igmp_config_g.max_group_member);
		util_fdprintf(fd, "\tmax ssm source limit: %d\n",igmp_config_g.max_ssm_source);
		util_fdprintf(fd, "\tzero limit: %d\n", bEnableZeroMeansUnlimited);
	}
	
	return CLI_OK;
}

static int MAYBE_UNUSED
cli_igmp_total_group_limit_set(int fd, char *total_group_limit)
{
	int old = igmp_config_g.group_limit;
	igmp_config_g.group_limit = util_atoi(total_group_limit);
	if (igmp_config_g.group_limit < 0) igmp_config_g.group_limit = old;
	
	return CLI_OK;
}

static int MAYBE_UNUSED
cli_igmp_total_group_member_limit_set(int fd, char *total_member_limit)
{
	int old = igmp_config_g.max_group_member;
	igmp_config_g.max_group_member = util_atoi(total_member_limit);
	if (igmp_config_g.max_group_member < 0) igmp_config_g.max_group_member = old; 
	
	return CLI_OK;
}

static int MAYBE_UNUSED
cli_igmp_ssm_source_limit_set(int fd, char *total_source_limit)
{
	int old = igmp_config_g.max_ssm_source;
	igmp_config_g.max_ssm_source = util_atoi(total_source_limit);
	if (igmp_config_g.max_ssm_source < 0) igmp_config_g.max_ssm_source = old;
	 
	return CLI_OK;
}

static int
igmp_filter_calixlimit_cli(int fd, int argc, char *argv[])
{
	if (strcmp(argv[0], "igmp") == 0)
	{
		if (argc==2 && strcmp(argv[1], "config") == 0)
			return cli_igmp_config_list(fd);
		else if (argc==4 && strcmp(argv[1], igmp_filter_calixlimit_name()) == 0)
		{
			if (strcmp(argv[2], "zero_limit") == 0)
			{
				bEnableZeroMeansUnlimited = util_atoi(argv[3]);
				return CLI_OK;		
			}
			else if (strcmp(argv[2], "total_group") == 0)
				return cli_igmp_total_group_limit_set(fd ,argv[3]);
			else if (strcmp(argv[2], "total_group_member") == 0)
				return cli_igmp_total_group_member_limit_set(fd ,argv[3]);
			else if (strcmp(argv[2], "ssm_source_limit") == 0)
				return cli_igmp_ssm_source_limit_set(fd ,argv[3]);
		}
	}
	
	return CLI_ERROR_OTHER_CATEGORY;
}

struct igmp_filter_module_t calixlimit_filter =
{
        .priority = 10,
	.filter_name = igmp_filter_calixlimit_name,
        .init = igmp_filter_calixlimit_init,
        .deinit = igmp_filter_calixlimit_deinit,
        .raw_filter = igmp_filter_calixlimit_raw_filter,
        .filter = igmp_filter_calixlimit_filter,
        .cli = igmp_filter_calixlimit_cli
};


          

