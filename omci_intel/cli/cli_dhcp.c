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
 * File    : cli_dhcp.c
 *
 ******************************************************************/

#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <netinet/ip.h>
#include "list.h"
#include "fwk_msgq.h"
#include "fwk_thread.h"
#include "cpuport.h"
#include "cpuport_dhcp.h"
#include "util.h"
#include "cli.h"

// helper function used by cli.c /////////////////////////////////////////////////////////////////////////////////////////////

void
cli_dhcp_help(int fd)
{
	util_fdprintf(fd, 
		"dhcp_filter count_get|count_reset\n");
}

int
cli_dhcp_cmdline(int fd, int argc, char *argv[])
{
	if (strcmp(argv[0], "dhcp_filter")==0) {
		if (argc==1) {
			cli_dhcp_help(fd);	
			return 0;
		} else if (argc==2 && strcmp(argv[1], "count_get") == 0) {
			cpuport_dhcp_count_get(fd);
			return 0;
		} else if (argc==2 && strcmp(argv[1], "count_reset") == 0) {
			cpuport_dhcp_count_reset(fd);
			return 0;
		}
		return CLI_ERROR_SYNTAX;		
	} else {
		return CLI_ERROR_OTHER_CATEGORY;
	}	
}

