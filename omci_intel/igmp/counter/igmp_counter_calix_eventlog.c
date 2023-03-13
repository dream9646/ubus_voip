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
 * Module  : igmp counter module - calix event log
 * File    : igmp_counter_calix_eventlog.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "meinfo.h"
#include "util.h"
#include "util_inet.h"
#include "hwresource.h"
#include "switch.h"
#include "mcast_const.h"
#include "mcast_pkt.h"
#include "mcast.h"
#include "mcast_timer.h"
#include "igmp_counter_calix_eventlog.h"

#define IGMP_COUNTER_MODULE_NAME        "calix_eventlog"
#define MAX_GROUP_RECORD		16

static int is_valid_uni_or_wan(struct me_t *me47)
{
        // verify RX is either WAN or LAN port
        struct me_t *me422 = hwresource_public2private(me47);
        
        if (me422 == NULL) return -1;
        
        if (meinfo_util_me_attr_data(me422 , 4) > 2) return -1;        // neither LAN nor WAN
        
        if (meinfo_util_me_attr_data(me422 , 4) == 2 && meinfo_util_me_attr_data(me422, 3) == 6) return -1;
        
        return 0;
}

static void downsize_log(int lines)
{
	FILE *slog = NULL;
	FILE *slog2 = NULL;
	slog = fopen("/storage/igmpevent.log", "r");
	if (slog == NULL) return;
	slog2 = fopen("/storage/new_igmpevent.log", "w+");
	if (slog2 == NULL)
	{
		fclose(slog);
		return;
	}
	
	char tmp[512];
	int i = 0;
		
	while(!feof(slog))
	{
  		char ch = fgetc(slog);
  		if(ch == '\n')
 		{
		 	i++;
		 	
  		}
	}
	
	i-=lines;
	
	rewind(slog);
	int j = 0;
	while (fgets(tmp, sizeof(tmp), slog) != NULL)
	{
		j++;
		if (j < i) continue;
		fputs(tmp, slog2);
	}
	
	fclose(slog);
	fclose(slog2);
	rename("/storage/new_igmpevent.log", "/storage/igmpevent.log");
}

static void igmp_counter_calix_eventlog_tolog(struct igmp_pkt_calix_eventlog_t *events)
{
	FILE *slog = NULL;
	struct stat fstat={0};
	char tmp[256];
	struct igmp_pkt_calix_eventlog_t *e;

        stat("/storage/igmpevent.log", &fstat);
        if (fstat.st_size > 16384)	// exceeed 16K
        {
        	// down size to at least 80 lines
        	downsize_log(80);	
	}
	
	slog = fopen("/storage/igmpevent.log", "a+");
	if (slog)
	{

        	int i;
		for (i = 0; i < MAX_GROUP_RECORD;i++)
		{
			e = events + i;
			if (e->mode == 0) break; //end
			if (strcmp(e->event, "") == 0) continue;
			
			char *mode;
			switch(e->mode)
			{
			case MODE_IS_INCLUDE:
				mode = "IS_INCLUDE";
				break;
			case MODE_IS_EXCLUDE:
				mode = "IS_EXCLUDE";
				break;
			case CHANGE_TO_INCLUDE_MODE:
				mode = "TO_INCLUDE_MODE";
				break;
			case CHANGE_TO_EXCLUDE_MODE:
				mode = "TO_EXCLUDE_MODE";
				break;
			case ALLOW_NEW_SOURCES:
				mode = "ALLOW_NEW_SOURCES";
				break;
			case BLOCK_OLD_SOURCES:
				mode = "BLOCK_OLD_SOURCES";
				break;
			default:
				mode = "-";
			}
			//snprintf(tmp, 256, "%s %-10s %-16s %s %-16s %-4s %-16s %-2d %s\n",e->time, e->event, e->device, 
			//				e->dir,e->groupip, e->ver, mode,
			//				e->num_src, e->sourceip);
			snprintf(tmp, 256, "{time:\"%s\",event:\"%s\",dev:\"%s\",tx:\"%s\",grpIp:\"%s\",ver:\"%s\",mode:\"%s\",numSrc:\"%d\",srcIp:\"%s\"}\n",e->time, e->event, e->device, 
							e->dir,e->groupip, e->ver, mode,
							e->num_src, e->sourceip);
			fputs(tmp, slog);			
		}
		fclose(slog);
	}				
}

// module functions
int
igmp_counter_calix_eventlog_reset(void)
{
//	FILE *slog = NULL;
//	struct stat fstat={0};

//      stat("/storage/igmpevent.log", &fstat);
//	slog = fopen("/storage/igmpevent.log", "w"); // trucate file to zero length
	
//	fclose(slog);
	return 0;
}

int
igmp_counter_calix_eventlog_init(void)
{	
	return 0;
}

const char *igmp_counter_calix_eventlog_name()
{
        return IGMP_COUNTER_MODULE_NAME;
}

static void parse_event(struct igmp_clientinfo_t *igmp_clientinfo, struct igmp_group_t *g, struct igmp_pkt_calix_eventlog_t *event)
{
	int ver = 0;
	struct tm *tm;
	time_t t;
	
	t = time(NULL);
	tm = localtime(&t);
	strftime(event->time, sizeof(event->time), "%H:%M:%S", tm);
	if (igmp_clientinfo->rx_desc.logical_port_id == 5)
	{
		strcpy(event->dir, "Tx");
		strcpy(event->device, "ONT");
	}
	else
	{
		strcpy(event->dir, "Rx");
		util_inet_ntop (AF_INET, (void *)(&igmp_clientinfo->client_ip) , event->device, sizeof(event->device));
	}
	
        switch (igmp_clientinfo->igmp_msg_type) 
        {
        case IGMP_V1_MEMBERSHIP_REPORT:
        	strcpy(event->event, "Report");
        	util_inet_ntop (AF_INET, (void *)(&g->group_ip) , event->groupip, sizeof(event->groupip));
        	ver = 1;
		break;
	case IGMP_V2_MEMBERSHIP_REPORT:
		strcpy(event->event, "Report");
        	util_inet_ntop (AF_INET, (void *)(&g->group_ip) , event->groupip, sizeof(event->groupip));
		ver = 2;
		break;	
	case IGMP_V3_MEMBERSHIP_REPORT:
		strcpy(event->event, "Report");
		// v3 report can have multiple group, each with different mode & different source
		util_inet_ntop (AF_INET, (void *)(&g->group_ip) , event->groupip, sizeof(event->groupip));
        	if (g->src_ip.ipv4.s_addr)
        	{
        		event->num_src++;
        		// append source ip
        		char tmp[32];
        		util_inet_ntop (AF_INET, (void *)(&g->src_ip) , tmp, sizeof(tmp));
        		if (strlen(event->sourceip) > 0)
        			strncat(event->sourceip, ",", sizeof(event->sourceip));
        		strncat(event->sourceip, tmp, sizeof(event->sourceip)); 
		}
		event->mode = g->record_type;
		ver = 3;
		break;
	case IGMP_V2_LEAVE_GROUP:
		strcpy(event->event, "Leave");
        	util_inet_ntop (AF_INET, (void *)(&g->group_ip) , event->groupip, sizeof(event->groupip));
		ver = 2;
		break;
	case IGMP_MEMBERSHIP_QUERY:                                              
                if (igmp_clientinfo->igmp_version & IGMP_V3_BIT)
                {
                        ver = 3;
			if (g->group_ip.ipv4.s_addr || g->src_ip.ipv4.s_addr)
			{
				strcpy(event->event, "GrpQuery");
        			util_inet_ntop (AF_INET, (void *)(&g->group_ip) , event->groupip, sizeof(event->groupip));
			}
			else
			{
				strcpy(event->event, "GenQuery");
				strcpy(event->groupip, "-");
			}
			// igmpv3 query can have sources but Calix ONT does not report them
        		if (g->src_ip.ipv4.s_addr)
        		{
        			event->num_src++;
        			// append source ip
        			char tmp[32];
        			util_inet_ntop (AF_INET, (void *)(&g->src_ip) , tmp, sizeof(tmp));
        			if (strlen(event->sourceip) > 0)
        				strncat(event->sourceip, ",", sizeof(event->sourceip));
        			strncat(event->sourceip, tmp, sizeof(event->sourceip)); 
			}           
                }
                else
                {	
			if (igmp_clientinfo->igmp_version & IGMP_V2_BIT) // igmpv2 query
                        {
                        	ver = 2;
                                if (g->group_ip.ipv4.s_addr)    // check if it's group query
                                {
                                	strcpy(event->event, "GrpQuery");
                                	util_inet_ntop (AF_INET, (void *)(&g->group_ip) , event->groupip, sizeof(event->groupip));
				}
				else
				{
					strcpy(event->event, "GenQuery");
					strcpy(event->groupip, "-");
				}
                        }
                        else
                        {
                                ver = 1;
                                strcpy(event->event, "GenQuery");
                                strcpy(event->groupip, "-");
                        }
                }    
		break;	
	default:
		dbprintf_igmp(LOG_WARNING,"unhandled igmp msg (%d)\n", igmp_clientinfo->igmp_msg_type);
        }
        
        if (ver != 3)
        {
        	event->num_src = 0;
        	strcpy(event->sourceip, "-");
	}
	
	if (ver == 1)
		strcpy(event->ver, "v1");
	else if (ver == 2)
		strcpy(event->ver, "v2");
	else if (ver == 3)
		strcpy(event->ver, "v3");
	else
		strcpy(event->ver, "-");
}

int igmp_counter_calix_eventlog_handler(struct igmp_clientinfo_t *igmp_clientinfo)
{
	struct igmp_group_t *g = NULL;


	if (IGMP_PKT_PARSE_CLIENTINFO != igmp_clientinfo->state.exec_state) return 0;
	if (is_valid_uni_or_wan(igmp_clientinfo->pktinfo->rx_desc.bridge_port_me)) return 0;
	struct igmp_pkt_calix_eventlog_t events[MAX_GROUP_RECORD];
	memset(&events, 0 ,sizeof(events));
	
	list_for_each_entry(g, &igmp_clientinfo->group_list, entry_node) {
		struct igmp_pkt_calix_eventlog_t *e = NULL;
		char cip[128];
		char dip[128];
		util_inet_ntop (AF_INET, (void *)(&g->group_ip) , dip, sizeof(dip));
		util_inet_ntop (AF_INET, (void *)(&igmp_clientinfo->client_ip) , cip, sizeof(cip));
		
		int i;
		for (i = 0; i < MAX_GROUP_RECORD;i++)
		{
			e = &events[i];
			if (e->mode == 0) break;	// reach free
			if (strcmp(e->groupip, dip) == 0 && strcmp(e->device, cip) == 0 && e->mode == g->record_type)
			{
				break;
			}
			if (i == (MAX_GROUP_RECORD - 1)) return 0;	// array full			
		}	
		e->mode = -1;
		parse_event(igmp_clientinfo, g, e);
	}
			
	igmp_counter_calix_eventlog_tolog(&events[0]);
			
	return 0;
}

int igmp_cli_counter_calix_eventlog(int fd)
{
	return 0;
}

int igmp_counter_iterator_calix_eventlog(int (*iterator_function)(struct igmp_pkt_calix_eventlog_t *pos, void *data), void *iterator_data)
{       
        return 0;
}


struct igmp_counter_module_t calix_eventlog =
{
        .module_name = igmp_counter_calix_eventlog_name,
        .init = igmp_counter_calix_eventlog_init,
        .reset = igmp_counter_calix_eventlog_reset,
        .igmp_counter_handler = igmp_counter_calix_eventlog_handler,
        .igmp_cli_handler = igmp_cli_counter_calix_eventlog,
        .igmp_counter_iterator = igmp_counter_iterator_calix_eventlog
};
