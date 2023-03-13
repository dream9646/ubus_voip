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
 * Module  : igmp counter module - calix 15mins log
 * File    : igmp_counter_calix_15mins.c
 *
 ******************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
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
#include "igmp_counter_calix_15mins.h"
#include "fwk_timer.h"

#define TIMER_CALIX_15MINS  TIMER_EVENT_CUSTOM | 0x01

static struct igmp_pkt_calix_15mins_t counter;

#define IGMP_COUNTER_MODULE_NAME        "calix_log_15mins"

#define CALIX_PERIOD		15

static void downsize_log(int lines)
{
	FILE *slog = NULL;
	FILE *slog2 = NULL;
	slog = fopen("/storage/igmp_15m.log", "r");
	if (slog == NULL) return;
	slog2 = fopen("/storage/new_igmp_15m.log", "w+");
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
	
	snprintf(tmp, 512, "%-16s %-10s %-10s %-10s %-10s %-10s %-10s %-10s %-10s %-10s\n",
		"uptime(mins)",
		"GenQry1", "GenQry2", "GenQry3", "GrpQry2", "GrpQry3",
		"Rpt1", "Rpt2", "Rpt3",
		"Leave");
	fputs(tmp, slog2);
			
	while (fgets(tmp, sizeof(tmp), slog) != NULL)
	{
		j++;
		if (j < i) continue;
		fputs(tmp, slog2);
	}
	
	fclose(slog);
	fclose(slog2);
	rename("/storage/new_igmp_15m.log", "/storage/igmp_15m.log");
}

static void igmp_counter_calix_15mins_tolog()
{
	FILE *slog = NULL;
	struct stat fstat={0};
	char tmp[512];

        stat("/storage/igmp_15m.log", &fstat);
        if (fstat.st_size > 8096)	// exceeed 4K
        {
        	// down size to at least 80 lines
        	downsize_log(40);	
	}
		
	slog = fopen("/storage/igmp_15m.log", "a+");
	if (slog && fstat.st_size == 0)
	{
		snprintf(tmp, 512, "%-16s %-10s %-10s %-10s %-10s %-10s %-10s %-10s %-10s %-10s\n",
			"uptime(mins)",
			"GenQry1", "GenQry2", "GenQry3", "GrpQry2", "GrpQry3",
			"Rpt1", "Rpt2", "Rpt3",
			"Leave");
			fputs(tmp, slog);
	}
	
	long newtimestamp = igmp_timer_get_cur_time();
	if (slog)
	{	
		snprintf(tmp, 512, "%-16lu %-10u %-10u %-10u %-10u %-10u %-10u %-10u %-10u %-10u\n",
			counter.timestamp/1000/60, 
			counter.rxGenQry1, counter.rxGenQry2, counter.rxGenQry3, counter.rxGrpQry2, counter.rxGrpQry3,
			counter.rxRpt1, counter.rxRpt2, counter.rxRpt3,
			counter.rxLeave);
		fputs(tmp, slog);		
		fclose(slog);
	}
	counter.timestamp = newtimestamp;				
}

static int calix_15mins_cb(void *data)
{
	// log this interval statistic
	igmp_counter_calix_15mins_tolog();
	
	// reset counter & register cb 15 mins later
	int new_timestamp = counter.timestamp;
	memset(&counter, 0, sizeof(struct igmp_pkt_calix_15mins_t));
	counter.timestamp = new_timestamp;
	igmp_register_custom_timer(TIMER_CALIX_15MINS, &counter.timer_g, CALIX_PERIOD*60*1000, NULL, calix_15mins_cb);
	
	return 0;
}


// module functions
int
igmp_counter_calix_15mins_reset(void)
{
	FILE *slog = NULL;
	struct stat fstat={0};

        stat("/storage/igmp_15m.log", &fstat);
	slog = fopen("/storage/igmp_15m.log", "w"); // trucate file to zero length
	if (slog)
  	        fclose(slog);
	return 0;
}

int
igmp_counter_calix_15mins_init(void)
{	
	memset(&counter, 0, sizeof(struct igmp_pkt_calix_15mins_t));
	counter.timestamp = igmp_timer_get_cur_time();
	igmp_register_custom_timer(TIMER_CALIX_15MINS, &counter.timer_g, CALIX_PERIOD*60*1000, NULL, calix_15mins_cb);
	
	return 0;
}

const char *igmp_counter_calix_15mins_name()
{
        return IGMP_COUNTER_MODULE_NAME;
}

int igmp_counter_calix_15mins_handler(struct igmp_clientinfo_t *igmp_clientinfo)
{
        struct igmp_group_t *g = NULL;
        
        switch (igmp_clientinfo->igmp_msg_type) 
	{
                case IGMP_V1_MEMBERSHIP_REPORT:
                        counter.rxRpt1++;
			break;
		case IGMP_MEMBERSHIP_QUERY:
                        // determine if it's IGMPv3 query, IGMPv3 query packet length is longer                        
                        list_for_each_entry(g, &igmp_clientinfo->group_list, entry_node) {}
                        
                        if (igmp_clientinfo->igmp_version & IGMP_V3_BIT)
                        {
                                if (g->group_ip.ipv4.s_addr || g->src_ip.ipv4.s_addr)
                                        counter.rxGrpQry3++;
                                else
                                        counter.rxGenQry3++;               
                        }
                        else
                        {
                                if (igmp_clientinfo->igmp_version & IGMP_V2_BIT) // igmpv2 query
                                {
                                        if (g->group_ip.ipv4.s_addr)    // check if it's group query
                                                counter.rxGrpQry2++;
                                        else
                                                counter.rxGenQry2++;
                                }
                                else
                                        counter.rxGenQry1++;
                        }    
			break;
		case IGMP_V2_MEMBERSHIP_REPORT:
                        counter.rxRpt2++;
			break;	
		case IGMP_V3_MEMBERSHIP_REPORT:
                        counter.rxRpt3++;
			break;
		case IGMP_V2_LEAVE_GROUP:
                        counter.rxLeave++;
			break;	
		default:
			dbprintf_igmp(LOG_WARNING,"unhandled igmp msg (%d)\n", igmp_clientinfo->igmp_msg_type);
			return -1;
	}  
                
					
	return 0;
}

int igmp_cli_counter_calix_15mins(int fd)
{
	return 0;
}

int igmp_counter_iterator_calix_15mins(int (*iterator_function)(struct igmp_pkt_calix_15mins_t *pos, void *data), void *iterator_data)
{       
        return 0;
}


struct igmp_counter_module_t calix_15mins =
{
        .module_name = igmp_counter_calix_15mins_name,
        .init = igmp_counter_calix_15mins_init,
        .reset = igmp_counter_calix_15mins_reset,
        .igmp_counter_handler = igmp_counter_calix_15mins_handler,
        .igmp_cli_handler = igmp_cli_counter_calix_15mins,
        .igmp_counter_iterator = igmp_counter_iterator_calix_15mins
};
