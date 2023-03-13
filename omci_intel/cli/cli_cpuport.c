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
 * File    : cli_cpupkt.c
 *
 ******************************************************************/

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <ctype.h>
#include <netinet/ip.h>
#include "list.h"
#include "util.h"
#include "cli.h"
#include "switch.h"
#include "cpuport.h"
#include "cpuport_recv.h"
#include "cpuport_sniffer.h"
#include "cpuport_util.h"
#include "cpuport_history.h"
#include "cpuport_extract_filter.h"

// helper function used by cli.c /////////////////////////////////////////////////////////////////////////////////////////////

void
cli_cpuport_history_help (int fd)
{
	util_fdprintf(fd, "cpuport history\n");
	util_fdprintf(fd, "cpuport history [n|keyword|*] - show specific pkt, list pkt of keyword, list all pkt\n");
	util_fdprintf(fd, "                list - list pkt \n");
	util_fdprintf(fd, "                list [n1] [n2] [keyword]\n");
	util_fdprintf(fd, "                find [mac_addr|mac_mask|eth_proto|ip_addr|ip_proto|tcpudp_port=...]\n");
	util_fdprintf(fd, "                show [n] - show pkt detail\n");
	util_fdprintf(fd, "                show [n1] [n2] [keyword]\n");
	util_fdprintf(fd, "                defer [n] - list pkt deferred more than n us\n");
	util_fdprintf(fd, "                clear - clear log\n");
	util_fdprintf(fd, "                clear [n] - clear log until n pkt remained\n");
	util_fdprintf(fd, "                run [n] - execute pkt as if thery are input from outside\n");
	util_fdprintf(fd, "                run [n1] [n2] [keyword]\n");
	util_fdprintf(fd, "                load|save [filename]\n");
	util_fdprintf(fd, "                save [filename] [n1] [n2] [keyword]\n");
	util_fdprintf(fd, "ps: [keyword] is optional\n");
}

void
cli_cpuport_help (int fd)
{
	cli_cpuport_history_help(fd);
	util_fdprintf(fd, "cpuport sniffer [subcmd...]\n");
	util_fdprintf(fd, "cpuport extract\n");
}

int 
cli_cpuport_cmdline(int fd, int argc, char *argv[])
{
	if (strcmp(argv[0], "cpuport")==0) {
#ifndef X86
		if (argc==1) {
			cli_cpuport_help(fd);
			return 0;		
		} else if (argc>=2 && strcmp(argv[1], "history") == 0) {
			if (argc == 2) {				
				cpuport_history_summary(fd);
				return 0;
			} else if (argc == 3) {
				if (strcmp(argv[2], "help") == 0) {
					cli_cpuport_history_help(fd);
					return 0;				
				} else if (strcmp(argv[2], "clear") == 0) {
					cpuport_history_clear(0);
					return 0;				
				} else if (strcmp(argv[2], "list") == 0) {
					cpuport_history_list(fd, 0, 0, NULL);
					return 0;				
				} else if (strcmp(argv[2], "listp") == 0) {
					cpuport_history_listp(fd, 0, 0, NULL);
					return 0;				
				} else if (strcmp(argv[2], "defer") == 0) {
					cpuport_history_defer(fd, 1000000UL);
					return 0;				
				} else if (strcmp(argv[2], "find") == 0 ||
					   strcmp(argv[2], "show") == 0 ||
					   strcmp(argv[2], "run") == 0 ||
					   strcmp(argv[2], "load") == 0 ||
					   strcmp(argv[2], "save") == 0 ) {
					cli_cpuport_history_help(fd);
					return 0;								
				} else if (!isdigit(argv[2][0])) {	// argv is keyword
					if (strcmp(argv[2], "*") == 0) {
						cpuport_history_list(fd, 0, 0, NULL);
				} else {
						cpuport_history_list(fd, 0, 0, argv[2]);
					}
					return 0;
				} else {	// argv is number
					int index = util_atoi(argv[2]);
					cpuport_history_show(fd, index, index, NULL);
					return 0;
				}
			} else if (argc >3) {
				if (strcmp(argv[2], "clear") == 0) {
					if (argc == 4) {
						int size = util_atoi(argv[3]);
						cpuport_history_clear(size);
						return 0;
					}
				} else if (strcmp(argv[2], "list") == 0) {
					if (argc >= 4) {
						int start = util_atoi(argv[3]);
						int end = start;
						char *keyword = NULL;
						if (argc >= 5)
							end = util_atoi(argv[4]);
						if (argc >=6)
							keyword = argv[5];
						cpuport_history_list(fd, start, end, keyword);
						return 0;
					}
				} else if (strcmp(argv[2], "listp") == 0) {
					if (argc >= 4) {
						int start = util_atoi(argv[3]);
						int end = start;
						char *keyword = NULL;
						if (argc >= 5)
							end = util_atoi(argv[4]);
						if (argc >=6)
							keyword = argv[5];
						cpuport_history_listp(fd, start, end, keyword);
						return 0;
					}
				} else if (strcmp(argv[2], "find") == 0) {
					if (argc >=4) {
						struct cpuport_history_find_t find;
						cpuport_history_prepare_find(fd, &find, argc-3, argv+3);
						cpuport_history_list_find(fd, &find);
						return 0;
					}
				} else if (strcmp(argv[2], "show") == 0) {
					if (argc >= 4) {
						int start = util_atoi(argv[3]);
						int end = start;
						char *keyword = NULL;
						if (argc >= 5)
							end = util_atoi(argv[4]);
						if (argc >=6)
							keyword = argv[5];
						cpuport_history_show(fd, start, end, keyword);
						return 0;
					}
				} else if (strcmp(argv[2], "defer") == 0) {
					if (argc == 4) {
						unsigned long defer_us = util_atoi(argv[3]);
						cpuport_history_defer(fd, defer_us);
						return 0;
					}
				} else if (strcmp(argv[2], "run") == 0) {
					if (argc >= 4) {
						int start = util_atoi(argv[3]);
						int end = start;
						char *keyword = NULL;
						if (argc >= 5)
							end = util_atoi(argv[4]);
						if (argc >=6)
							keyword = argv[5];
						cpuport_history_run(fd, start, end, keyword);
						return 0;
					}
				} else if (strcmp(argv[2], "save") == 0) {
					if (argc == 4 || argc >=6) {	// 'save fname' or 'save fname n1 n2'
						char *filename = argv[3];
						int start = 0;
						int end = 0;
						char *keyword = NULL;
						if (argc >= 6) {
							start = util_atoi(argv[4]);
							end = util_atoi(argv[5]);
						}
						if (argc >=7)
							keyword = argv[6];
						cpuport_history_save(fd, filename, start, end, keyword);
						return 0;
					}
				} else if (strcmp(argv[2], "load") == 0) {
					if (argc == 4) {
						char *filename = argv[3];
						cpuport_history_load(fd, filename);
						return 0;
					}
				}
			}
			
		} else if (argc>=2 && strcmp(argv[1], "sniffer") == 0) {
			if (argc==2) {
				cpuport_sniffer_config_get(fd);
				return  0;
			} else {
				cpuport_sniffer_config_set(fd, argc-2, &argv[2]);
				return  0;
			}

		} else if (argc==2 && strcmp(argv[1], "extract") == 0) {
			cpuport_extract_filter_dump(fd);
			return 0;
		}
#endif
	}
	return CLI_ERROR_OTHER_CATEGORY;
}
