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
 * File    : cli_bat.c
 *
 ******************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>
#include <dirent.h>
#include <fcntl.h>

#include "util.h"
#include "meinfo.h"
#include "er_group.h"
#include "omciutil_vlan.h"
#include "cli.h"
#include "me_related.h"
#include "hwresource.h"
#include "batchtab.h"
#include "coretask.h"

static int
cli_bat_summary(int fd)
{
	struct batchtab_t *batptr;	
	struct timeval now_time;
	
	util_get_uptime(&now_time);
	util_fdprintf(fd, "o5_sequence=%u\n", coretask_get_o5_sequence());
	util_fdprintf(fd, "external_event_time=%ds\n", now_time.tv_sec - batchtab_extevent_time.tv_sec);
	list_for_each_entry(batptr, &batchtab_list, batchtab_node) {
		util_fdprintf(fd, "%s  %s\n", batptr->name, batptr->enable?"":"(disabled)");
		util_fdprintf(fd, "\tomci_idle_timeout=%d, time: omci_updat=%d%s, table_gen=%d, hw_sync=%d sec ago\n",
			batptr->omci_idle_timeout, 
			now_time.tv_sec - batptr->omci_update_time.tv_sec, batptr->omci_update_auto?"(auto)":"",
			now_time.tv_sec - batptr->table_gen_time.tv_sec, 
			now_time.tv_sec - batptr->hw_sync_time.tv_sec);
		util_fdprintf(fd, "\tupdate: o5_seq=%u, omci=%d, table_gen=%d, crosstab=%d, accum_err: table_gen=%d, hw_sync=%d\n", 
			batptr->o5_sequence, batptr->omci_update_count, batptr->tablegen_update_count, batptr->crosstab_update_count, 
			batptr->table_gen_accum_err, batptr->hw_sync_accum_err);
		util_fdprintf(fd, "\ttable_data: %p, ref_count=%d, write_lock=%d, hw_sync_lock=%d\n", 
			batptr->table_data, batptr->ref_count, batptr->write_lock, batptr->hw_sync_lock);
		util_fdprintf(fd, "\texe_time_sw=%dus (max:%d), exe_time_hw=%dus (max:%d)\n", 
			batptr->exe_time_sw, batptr->exe_time_sw_max, batptr->exe_time_hw, batptr->exe_time_hw_max);
	}
	return CLI_OK;
}


// helper function used by cli.c /////////////////////////////////////////////////////////////////////////////////////////////

void
cli_bat_help(int fd)
{
	util_fdprintf(fd, 
		"bat [en|dis|a1|a0|ou|cu|tg|hs|dump] [batchtab|*]\n"
		"bat [enable|disable|auto|noauto|omci_update|crosstab_update|table_gen|hw_sync|dump] [batchtab|*]\n"
		);
}

int
cli_bat_cmdline(int fd, int argc, char *argv[])
{
	struct batchtab_t *batptr;	

	if (strcmp(argv[0], "bat")==0) {
		if (argc==1) {
			return cli_bat_summary(fd);

		} else if (argc==3 && (strcmp(argv[1], "enable")==0 || strcmp(argv[1], "en")==0)) {
			list_for_each_entry(batptr, &batchtab_list, batchtab_node) {
				if (strcmp(batptr->name, argv[2])==0 || strcmp("*", argv[2])==0)
					batchtab_enable(batptr->name, 1);
			}
			return CLI_OK;				
		} else if (argc==3 && (strcmp(argv[1], "disable")==0 || strcmp(argv[1], "dis")==0)) {
			list_for_each_entry(batptr, &batchtab_list, batchtab_node) {
				if (strcmp(batptr->name, argv[2])==0 || strcmp("*", argv[2])==0)
					batchtab_enable(batptr->name, 0);
			}
			return CLI_OK;				
		} else if (argc==3 && (strcmp(argv[1], "auto")==0 || strcmp(argv[1], "a1")==0)) {
			list_for_each_entry(batptr, &batchtab_list, batchtab_node) {
				if (strcmp(batptr->name, argv[2])==0 || strcmp("*", argv[2])==0)
					batptr->omci_update_auto = 1;
			}
			return CLI_OK;				
		} else if (argc==3 && (strcmp(argv[1], "noauto")==0 || strcmp(argv[1], "a0")==0)) {
			list_for_each_entry(batptr, &batchtab_list, batchtab_node) {
				if (strcmp(batptr->name, argv[2])==0 || strcmp("*", argv[2])==0)
					batptr->omci_update_auto = 0;
			}
			return CLI_OK;				
		} else if (argc==4 && (strcmp(argv[1], "omci_idle_timeout")==0)) {
			list_for_each_entry(batptr, &batchtab_list, batchtab_node) {
				if (strcmp(batptr->name, argv[2])==0 || strcmp("*", argv[2])==0) {
					int idle_timeout = atoi(argv[3]);
					if (idle_timeout>=1) {
						batptr->omci_idle_timeout = idle_timeout;
					} else {
						util_fdprintf(fd, "idle_time must >= 1\n"); 
						return CLI_ERROR_RANGE;
					}						
				}
			}
			return CLI_OK;				
		} else if (argc==3 && (strcmp(argv[1], "omci_update")==0 || strcmp(argv[1], "ou")==0)) {
			list_for_each_entry(batptr, &batchtab_list, batchtab_node) {
				if (strcmp(batptr->name, argv[2])==0 || strcmp("*", argv[2])==0)
					batchtab_omci_update(batptr->name);
			}
			return CLI_OK;				
		} else if (argc==3 && (strcmp(argv[1], "crosstab_update")==0 || strcmp(argv[1], "cu")==0)) {
			list_for_each_entry(batptr, &batchtab_list, batchtab_node) {
				if (strcmp(batptr->name, argv[2])==0 || strcmp("*", argv[2])==0)
					batchtab_crosstab_update(batptr->name);
			}
			return CLI_OK;				
		} else if (argc==3 && (strcmp(argv[1], "table_gen")==0 || strcmp(argv[1], "tg")==0)) {
			list_for_each_entry(batptr, &batchtab_list, batchtab_node) {
				if (strcmp(batptr->name, argv[2])==0 || strcmp("*", argv[2])==0)
					batchtab_table_gen(batptr->name);
			}
			return CLI_OK;				
		} else if (argc==3 && (strcmp(argv[1], "hw_sync")==0 || strcmp(argv[1], "hs")==0)) {
			list_for_each_entry(batptr, &batchtab_list, batchtab_node) {
				if (strcmp(batptr->name, argv[2])==0 || strcmp("*", argv[2])==0)
					batchtab_hw_sync(batptr->name);
			}
			return CLI_OK;				
		} else if (argc==3 && strcmp(argv[1], "dump")==0) {
			list_for_each_entry(batptr, &batchtab_list, batchtab_node) {
				if (strcmp(batptr->name, argv[2])==0 || strcmp("*", argv[2])==0) {
					util_fdprintf(fd, "batchtab %s:\n", batptr->name); 
					batchtab_table_data_dump(fd, batptr->name);
					util_fdprintf(fd, "\n"); 
				}
			}
			return CLI_OK;				
		}
		return CLI_ERROR_SYNTAX;
	} else {
		return CLI_ERROR_OTHER_CATEGORY;
	}	
}
