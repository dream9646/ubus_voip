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
 * Module  : cli
 * File    : cli_igmp.c
 *
 ******************************************************************/

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "list.h"
#include "util.h"
#include "util_inet.h"
#include "cli.h"

#include "mcast_const.h"
#include "mcast.h"
#include "igmp_mbship.h"
#include "mcast_main.h"
#include "mcast_timer.h"
#include "mcast_snoop.h"
#include "platform.h"

#define MAYBE_UNUSED __attribute__((unused))

#ifndef X86

static int 
cli_mld_joined_membership_list(int fd)
{
	struct switch_mac_tab_entry_t mac_tab_entry;
	int num=0, i = 1;
	struct mcast_mbship_t *pos=NULL, *n=NULL;
	char group_ip[INET6_ADDRSTRLEN];
	char src_ip[INET6_ADDRSTRLEN];
	char client_ip[INET6_ADDRSTRLEN];
	char str[1024];
	long current = igmp_timer_get_cur_time();
		
	util_fdprintf(fd, "%-5s%-8s%-8s%-8s%-32s%-32s%-32s%-5s\n", "idx", "port", "uni", "ani", "gip", "sip", "reporter", "age");

        list_for_each_entry_safe(pos, n, &igmp_mbship_db_g.mbship_list, entry_node) {
        	if (!IS_IPV6_ADDR(pos->group_ip)) continue;

		util_inet_ntop(AF_INET6, &pos->group_ip, (char *)group_ip, sizeof(group_ip));
		util_inet_ntop(AF_INET6, &pos->src_ip, (char *)src_ip, sizeof(src_ip));
		util_inet_ntop(AF_INET6, &pos->client_ip, (char *)client_ip, sizeof(client_ip));

                snprintf(str,1024,"%-5d%-8d%-8d%-8d%-32s%-32s%-32s%-5ld\n",
			i, pos->logical_port_id, pos->uni_vid, pos->ani_vid,
			group_ip, src_ip, client_ip, (current - pos->access_time)/1000
			);
						
		util_fdprintf(fd, "%s", str);
		i++;
	}

	util_fdprintf(fd, "\n\nFVT2510 mac table IPV6 multicast entry\n\n");
	util_fdprintf(fd, "idx\tentry\tgroup ip\t\tvid\tfid\tport mask\n");

	i=1;
	num=0;					
	do
	{
		memset(&mac_tab_entry, 0, sizeof(struct switch_mac_tab_entry_t));
		if (switch_hw_g.mac_l2mc_get_next(&num, &mac_tab_entry ) != 1) break;

		util_fdprintf(fd, "%d\t%d\t%02x:%02x:%02x:%02x:%02x:%02x\t\t%d\t%d\t0x%x\n",i,num, 
				mac_tab_entry.mac_addr[0],mac_tab_entry.mac_addr[1],mac_tab_entry.mac_addr[2],
				mac_tab_entry.mac_addr[3],mac_tab_entry.mac_addr[4],mac_tab_entry.mac_addr[5], 
				mac_tab_entry.vid, mac_tab_entry.fid, 
				mac_tab_entry.port_bitmap);
		num++;
		i++;

	} while (i<8192);	// mactab entry range 0..2047, use 8192 for safety, better than while (1)

	return 0;	
}

static int 
cli_igmp_joined_membership_list(int fd)
{
	struct mcast_mbship_t *pos=NULL, *n=NULL;
	long current = igmp_timer_get_cur_time();
	struct switch_mac_tab_entry_t mac_tab_entry;
	int num=0;

	if (igmpproxytask_loop_g  == 0) return 0;

	util_fdprintf(fd, "membership list\n");
        
	char str[1024];
	memset(str,0,1024);
	char group_ip[INET6_ADDRSTRLEN];
	char src_ip[INET6_ADDRSTRLEN];
	char client_ip[INET6_ADDRSTRLEN];
	unsigned short i=1;


	list_for_each_entry_safe(pos, n, &igmp_mbship_db_g.mbship_list, entry_node) {
		memset(group_ip,0,INET6_ADDRSTRLEN);
		memset(src_ip,0,INET6_ADDRSTRLEN);
		memset(client_ip,0,INET6_ADDRSTRLEN);

                if (IS_IPV6_ADDR(pos->group_ip)) continue;

		util_inet_ntop(AF_INET, &pos->group_ip, (char *)group_ip, sizeof(group_ip));
		util_inet_ntop(AF_INET, &pos->src_ip, (char *)src_ip, sizeof(src_ip));
		util_inet_ntop(AF_INET, &pos->client_ip, (char *)client_ip, sizeof(client_ip));
		
		if (THREAD_ENV.igmp_proxy_snooping == 0)
			snprintf(str,1024,"%d\tlogical_port_id:%d\tsrc_port_id:%d(0x%x)\tuni_vid:%d\tani_vid:%d\n\tgroup_ip:%s\tsrc_ip:%s\tclient_ip:%s\t\nclient_mac:%02x-%02x-%02x-%02x-%02x-%02x\tage:%ld\n\n",
				i,pos->logical_port_id,pos->src_port_id, pos->src_port_id, pos->uni_vid, pos->ani_vid, 
				group_ip, src_ip, client_ip,
				pos->client_mac[0],pos->client_mac[1],pos->client_mac[2],
				pos->client_mac[3],pos->client_mac[4], pos->client_mac[5],
				(current - pos->access_time)/1000);
		else
			snprintf(str,1024,"%d\tlogical_port_id:%d\tsrc_port_id:%d(0x%x)\tuni_vid:%d\tani_vid:%d\n\tgroup_ip:%s\tsrc_ip:%s\tclient_ip:%s\tage:%ld\n\n",
				i,pos->logical_port_id,pos->src_port_id, pos->src_port_id, pos->uni_vid, pos->ani_vid, 
				group_ip, src_ip, client_ip,
				(current - pos->access_time)/1000);				
		util_fdprintf(fd,"%s",str);
		i++;
	}
	
	if (igmp_config_g.igmp_version & IGMP_V3_BIT)
	{
        	util_fdprintf(fd, "\n\nIGMPv3 Group/Source membership\n\n");
        	i = 0;
        	struct mcast_snoop_grp_t *pos, *n;
	        list_for_each_entry_safe(pos, n, &igmp_mbship_db_g.mcast_snoop_list, entry_node) {
	                struct mcast_snoop_src_t *pos1, *n1;
	                struct mcast_mbship_t *pos2, *n2;
	                util_fdprintf(fd,"[%d] Group-%-15s (%s)\n\t-ASM hosts:\n", i, util_inet_ntop(AF_INET, (unsigned char *)(&pos->group_ip), (char *)&group_ip, 16), (list_empty(&pos->host_asm_list))?"SSM":"ASM");
	                list_for_each_entry_safe(pos2, n2, &pos->host_asm_list, entry_node_v3) {
	                                util_fdprintf(fd,"\t\t-%-15s\n", util_inet_ntop(AF_INET, (unsigned char *)(&pos2->client_ip), (char *)&client_ip, 16));
	                } 
	                
	                list_for_each_entry_safe(pos1, n1, &pos->src_list, entry_node) {
	                        util_fdprintf(fd,"\t-Source %-15s, SSM hosts:\n", util_inet_ntop(AF_INET, (unsigned char *)(&pos1->src_ip), (char *)&group_ip, 16));
	                        list_for_each_entry_safe(pos2, n2, &pos1->host_ssm_list, entry_node_v3) {
	                                util_fdprintf(fd,"\t\t-%-15s\n", util_inet_ntop(AF_INET, (unsigned char *)(&pos2->client_ip), (char *)&client_ip, 16));
	                        }       
	                }
	                i++;
	                util_fdprintf(fd, "\n");                              
		}
        }        
        
	i=1;
	util_fdprintf(fd, "\n\nFVT2510 mac table IPV4 multicast entry\n\n");
	util_fdprintf(fd, "idx\tentry\tgroup ip\tsource ip\tvid\tfid\tport mask\n");

	do
	{
		memset(&mac_tab_entry, 0, sizeof(struct switch_mac_tab_entry_t));
		if (switch_hw_g.mac_ipmc_get_next( &num, &mac_tab_entry ) != 1) break;

		util_fdprintf(fd, "%d\t%d\t%-15s\t%-15s\t%d\t%d\t0x%x\n",i,num, 
				util_inet_ntop (AF_INET, (unsigned char *)(&mac_tab_entry.ipmcast.dst_ip), 
					(char *)&group_ip, 16),
                                util_inet_ntop (AF_INET, (unsigned char *)(&mac_tab_entry.ipmcast.src_ip), 
					(char *)&src_ip, 16),
				mac_tab_entry.vid, mac_tab_entry.fid, 
				mac_tab_entry.port_bitmap);
		num++;
		i++;

	} while (i<8192);	// mactab entry range 0..2047, use 8192 for safety, better than while (1)
	
	return 0;
}

static int
cli_igmp_config_list(int fd)
{
	if (igmpproxytask_loop_g  == 0) return 0;

	util_fdprintf(fd, "current configurations:\n");
	util_fdprintf(fd, "\tmax_response_time: %d (unit:0.1sec)\n",igmp_config_g.max_respon_time);
	util_fdprintf(fd, "\tquery_interval: %d (unit:sec)\n",igmp_config_g.query_interval);
	util_fdprintf(fd, "\tlast_member_query_interval: %d (unit:0.1sec)\n",igmp_config_g.last_member_query_interval);
	util_fdprintf(fd, "\trobustness: %d\n",igmp_config_g.robustness);
	util_fdprintf(fd, "\timmediate_leave: %d\n",igmp_config_g.immediate_leave);

	if (igmp_config_g.igmp_version & IGMP_V3_BIT)
		util_fdprintf(fd, "\tigmp_version: v3\n");
	else if (igmp_config_g.igmp_version & IGMP_V2_BIT)
		util_fdprintf(fd, "\tigmp_version: v2\n");
	else if (igmp_config_g.igmp_version & IGMP_V1_BIT)
		util_fdprintf(fd, "\tigmp_version: v1\n");
	else
		util_fdprintf(fd, "\tigmp_version: 0x%x\n",igmp_config_g.igmp_version);
	util_fdprintf(fd, "\tigmp_function: %d\n",igmp_config_g.igmp_function);
	util_fdprintf(fd, "\tquerier_ip: %s\n",igmp_config_g.querier_ip);
	util_fdprintf(fd, "\tupstream_igmp_rate: %d\n",igmp_config_g.upstream_igmp_rate);
	util_fdprintf(fd, "\tupstream_igmp_tci: %x (0x%x, 0x%x, 0x%x)\n", igmp_config_g.upstream_igmp_tci, (igmp_config_g.upstream_igmp_tci&0xE000)>>13, (igmp_config_g.upstream_igmp_tci&0x1000)>>12, igmp_config_g.upstream_igmp_tci&0xFFF);
	util_fdprintf(fd, "\tupstream_igmp_tag_ctl: %d\n",igmp_config_g.upstream_igmp_tag_ctl);
	util_fdprintf(fd, "\tdownstream_igmp_tci: %x (0x%x, 0x%x, 0x%x)\n", igmp_config_g.downstream_igmp_tci, (igmp_config_g.downstream_igmp_tci&0xE000)>>13, (igmp_config_g.downstream_igmp_tci&0x1000)>>12, igmp_config_g.downstream_igmp_tci&0xFFF);
	util_fdprintf(fd, "\tdownstream_igmp_tag_ctl: %d\n",igmp_config_g.downstream_igmp_tag_ctl);
	util_fdprintf(fd, "\tigmp_compatibity_mode: %d\n",igmp_config_g.igmp_compatibity_mode);
	if (THREAD_ENV.igmp_proxy_snooping == 0)
		util_fdprintf(fd, "\tigmp mode: snooping\n");
	else if (THREAD_ENV.igmp_proxy_snooping == 1)
		util_fdprintf(fd, "\tigmp mode: proxy without snooping\n");
	else if (THREAD_ENV.igmp_proxy_snooping == 2)
		util_fdprintf(fd, "\tigmp mode: proxy\n");	
	return 0;
}


static int
cli_igmp_counter(int fd)
{
	if (igmpproxytask_loop_g  == 0) return 0;

	struct igmp_counter_module_t *m=NULL;
        
        list_for_each_entry(m,  &igmp_config_g.m_counter_list, m_node) {
        	m->igmp_cli_handler(fd);            
        }
        
	return 0;
}

static int
cli_igmp_counter_list(int fd)
{
	if (igmpproxytask_loop_g  == 0) return 0;

	struct igmp_counter_module_t *m=NULL;
        
        util_fdprintf(fd, "counter modules: \n");
        list_for_each_entry(m,  &igmp_config_g.m_counter_list, m_node) {
        	util_fdprintf(fd, "\t%s\n",m->module_name());           
        }
        
	return 0;
}

static int
cli_igmp_filter_list(int fd)
{
	if (igmpproxytask_loop_g  == 0) return 0;

	struct igmp_filter_module_t *m=NULL;
        
        util_fdprintf(fd, "filter modules: \n");
        list_for_each_entry(m,  &igmp_config_g.m_filter_list, m_node) {
        	util_fdprintf(fd, "\t%s\n",m->filter_name());           
        }
        
	return 0;
}


static int
cli_igmp_counter_reset(int fd)
{
	struct igmp_counter_module_t *m=NULL;
        
        list_for_each_entry(m,  &igmp_config_g.m_counter_list, m_node) {
        	m->reset();            
        }
        
	return 0;
}

static int MAYBE_UNUSED
cli_igmp_set_igmp_config_last_member_query_interval(int fd, char *last_member_query_interval)
{
	igmp_config_g.last_member_query_interval = util_atoi(last_member_query_interval); 
	return 0;
}

static int MAYBE_UNUSED
cli_igmp_set_igmp_config_robustness(int fd, char *robustness)
{
	igmp_config_g.robustness = util_atoi(robustness);
	if (igmp_config_g.robustness < 2) igmp_config_g.robustness = 2;		// robustiness must not be 0 or 1 
	return 0;
}

static int MAYBE_UNUSED
cli_igmp_set_igmp_config_igmp_version(int fd, char *igmp_version)
{
	igmp_config_g.igmp_version = util_atoi(igmp_version); 
	return 0;
}

static int MAYBE_UNUSED
cli_igmp_set_igmp_config_querier_ip(int fd, char *querier_ip)
{
	memcpy(igmp_config_g.querier_ip ,querier_ip, strlen(querier_ip)+1); 
	return 0;
}

static int MAYBE_UNUSED
cli_igmp_set_igmp_config_upstream_igmp_tci(int fd, char *upstream_igmp_tci)
{
	int tci = 0;
	tci = util_atoi(upstream_igmp_tci); 
	if (tci < 0x10000)
		igmp_config_g.upstream_igmp_tci =htons((unsigned short )tci);
	else
		igmp_config_g.upstream_igmp_tci =0xffff;
	return 0;
}

static int MAYBE_UNUSED
cli_igmp_set_igmp_config_upstream_igmp_tag_ctl(int fd, char *upstream_igmp_tag_ctl)
{
	igmp_config_g.upstream_igmp_tag_ctl = util_atoi(upstream_igmp_tag_ctl); 
	return 0;
}

static int MAYBE_UNUSED
cli_igmp_set_igmp_config_igmp_function(int fd, char *igmp_function)
{
	igmp_config_g.igmp_function = util_atoi(igmp_function);
	
	return 0;
}

static int MAYBE_UNUSED
cli_igmp_set_igmp_config_downstream_igmp_tci(int fd, char *downstream_igmp_tci)
{
	int tci = 0;
	tci = util_atoi(downstream_igmp_tci); 
	if (tci < 0x10000)
		igmp_config_g.downstream_igmp_tci =htons((unsigned short )tci);
	else
		igmp_config_g.downstream_igmp_tci =0xffff;
	return 0;
}

static int MAYBE_UNUSED
cli_igmp_set_igmp_config_downstream_igmp_tag_ctl(int fd, char *downstream_igmp_tag_ctl)
{
	igmp_config_g.downstream_igmp_tag_ctl = util_atoi(downstream_igmp_tag_ctl); 
	return 0;
}


static int MAYBE_UNUSED
cli_igmp_set_igmp_config_immediate_leave(int fd, char *immediate_leave )
{
	igmp_config_g.immediate_leave = util_atoi(immediate_leave);
	
	return 0;
}

static int MAYBE_UNUSED
cli_igmp_set_igmp_config_query_interval(int fd, char *query_interval)
{
	igmp_config_g.query_interval = util_atoi(query_interval) ; 
	fwk_timer_delete( timer_id_query_interval_g );
	timer_id_query_interval_g = fwk_timer_create( igmp_timer_qid_g, TIMER_EVENT_QUERY_INTERVAL, igmp_config_g.query_interval*1000, NULL);
	
	return 0;
}

static int MAYBE_UNUSED
cli_igmp_set_igmp_config_max_respon_time(int fd, char *max_respon_time)
{
	igmp_config_g.max_respon_time = util_atoi(max_respon_time);
	
	return 0;
}

static int MAYBE_UNUSED
cli_igmp_set_igmp_compatibity_mode(int fd, char *compatibity_mode)
{
	int mode = 0;
	mode = util_atoi(compatibity_mode);
	if (mode >= 0 && mode < 3)
	       igmp_config_g.igmp_compatibity_mode = mode;
	return 0;
}

#endif
// helper function used by cli.c /////////////////////////////////////////////////////////////////////////////////////////////

void
cli_igmp_help(int fd)
{
	util_fdprintf(fd, "igmp help|[subcmd...]\n");
}

void
cli_igmp_help_long(int fd)
{
	util_fdprintf(fd, 
		"igmp [start|stop|config|member|counter]\n"
		"     config [max_response_time|query_interval|immediate_leave|igmp_function|last_member_query_interval\n"
		"                              |robustness|igmp_version|querier_ip\n"
		"                              |upstream_igmp_rate|upstream_igmp_tci|upstream_igmp_tag_ctl] [value]\n"
		"     filter \n"
		"     counter \n"		
		"     counter [list|reset]\n"
		);
}

int 
igmp_cli_cmdline(int fd, int argc, char *argv[])
{
	int ret = CLI_ERROR_OTHER_CATEGORY;
	
	struct igmp_filter_module_t *m=NULL;

	// igmp_config_g.m_filter_list would be initialized after igmp task is started,
	// so if igmp_config_g.m_filter_list.next/prev is null, do nothing for igmp cli
	if (igmp_config_g.m_filter_list.next==NULL ||
	    igmp_config_g.m_filter_list.prev==NULL) {
		util_fdprintf(fd, "igmp/mld is not initialized, cli ignored\n");
	    	return CLI_OK;
	}
        
	if (strcmp(argv[0], "mld") == 0)
	{
#ifndef X86
		if (argc==2 && strcmp(argv[1], "member") == 0)
			ret = cli_mld_joined_membership_list(fd);
#endif		 
	}	
	else if (strcmp(argv[0], "igmp") == 0)
	{
		if (argc==1)
		{
			ret = CLI_OK;
		}
#ifndef X86
		else if (argc==2 && strcmp(argv[1], "help") == 0)
		{
			cli_igmp_help_long(fd);
			ret = CLI_OK;
		}
		else if (argc==2 && strcmp(argv[1], "start") == 0)
		{
			if (igmpproxytask_loop_g == 1)
			{
				util_fdprintf(fd, "igmp task had already started\n");
			}
			else
			{
				igmp_proxy_init();
				igmp_proxy_start();
			}
			ret = CLI_OK;
		}
		else if (argc==2 && strcmp(argv[1], "stop") == 0)
		{
			if (igmpproxytask_loop_g == 0)
			{
				util_fdprintf(fd, "igmp task did not run\n");
			}
			else
			{
				igmp_proxy_stop();
				igmp_proxy_shutdown();
			}
			ret = CLI_OK;
		}
		else if (argc==2 && strcmp(argv[1], "config") == 0)
		{
			ret = cli_igmp_config_list(fd);
		}
		else if (argc==2 && strcmp(argv[1], "member") == 0)
		{
			ret = cli_igmp_joined_membership_list(fd);
		}
		else if (strcmp(argv[1], "counter") == 0)
		{
			if (argc ==2)
				ret = cli_igmp_counter(fd);
			if (argc == 3  && strcmp(argv[2], "reset") == 0)
				ret = cli_igmp_counter_reset(fd);
			else if (argc == 3  && strcmp(argv[2], "list") == 0)
				ret = cli_igmp_counter_list(fd);
		}
		else if (strcmp(argv[1], "filter") == 0)
		{
			ret = cli_igmp_filter_list(fd);
		}
		else if (argc==4 && strcmp(argv[1], "config") == 0)
		{
			if (strcmp(argv[2], "max_response_time") == 0)
			{
				ret = cli_igmp_set_igmp_config_max_respon_time(fd,argv[3]);
			}
			else if (strcmp(argv[2], "query_interval") == 0)
			{
				ret = cli_igmp_set_igmp_config_query_interval(fd,argv[3]);
			}
			else if (strcmp(argv[2], "immediate_leave") == 0)
			{
				ret = cli_igmp_set_igmp_config_immediate_leave(fd,argv[3]);
			}
			else if (strcmp(argv[2], "igmp_function") == 0)
			{
				ret = cli_igmp_set_igmp_config_igmp_function(fd, argv[3]);
			}
			else if (strcmp(argv[2], "last_member_query_interval") == 0)
			{
				ret = cli_igmp_set_igmp_config_last_member_query_interval(fd, argv[3]);
			}
			else if (strcmp(argv[2], "robustness") == 0)
			{
				ret = cli_igmp_set_igmp_config_robustness(fd, argv[3]);
			}
			else if (strcmp(argv[2], "igmp_version") == 0)
			{
				ret = cli_igmp_set_igmp_config_igmp_version(fd, argv[3]);
			}
			else if (strcmp(argv[2], "querier_ip") == 0)
			{
				ret = cli_igmp_set_igmp_config_querier_ip(fd, argv[3]);
			}
			else if (strcmp(argv[2], "upstream_igmp_tci") == 0)
			{
				ret = cli_igmp_set_igmp_config_upstream_igmp_tci(fd, argv[3]);
			}
			else if (strcmp(argv[2], "upstream_igmp_tag_ctl") == 0)
			{
				ret = cli_igmp_set_igmp_config_upstream_igmp_tag_ctl(fd, argv[3]);
			}
			else if (strcmp(argv[2], "downstream_igmp_tci") == 0)
			{
				ret = cli_igmp_set_igmp_config_downstream_igmp_tci(fd, argv[3]);
			}
			else if (strcmp(argv[2], "downstream_igmp_tag_ctl") == 0)
			{
				ret = cli_igmp_set_igmp_config_downstream_igmp_tag_ctl(fd, argv[3]);
			}
			else if (strcmp(argv[2], "igmp_compatibity_mode") == 0)
			{
				ret = cli_igmp_set_igmp_compatibity_mode(fd, argv[3]);
			}
		}
		else if (argc == 2 && strcmp(argv[1], "reload") == 0)
		{
			return igmp_proxy_reload();
		}
		else
		{
			ret = CLI_ERROR_OTHER_CATEGORY;	// ret CLI_ERROR_OTHER_CATEGORY as diag also has cli 'igmp'
		}
		//ret = CLI_ERROR_OTHER_CATEGORY;
#else
		ret = CLI_OK;
#endif
	}
	
	
        list_for_each_entry(m,  &igmp_config_g.m_filter_list, m_node) {
		if (m->cli == NULL) continue;
		
		// run through module cli loop
		// if one succeed, then consider overall a success
		int ret2 = m->cli(fd, argc, argv);
		if (ret != CLI_OK && ret2 == CLI_OK) ret = CLI_OK;       
        }
		
	return ret;
}
