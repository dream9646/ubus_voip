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
 * File    : cli_arp.c
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
#include "cpuport_arp.h"
#include "util.h"
#include "cli.h"

static int
cli_arp_limit_change(unsigned int arp_limit)
{
	if ( arp_limit != cpuport_arper_g.arp_limit)
	{
		struct arp_cmd_msg_t *arp_cmd_msg =(struct arp_cmd_msg_t *)malloc_safe(sizeof(struct arp_cmd_msg_t));
		arp_cmd_msg->arp_limit = arp_limit;
		if (fwk_msgq_send( arp_cmd_qid_g, &arp_cmd_msg->q_entry) <0 )
		{
			free_safe( arp_cmd_msg );
		}
	}
	return 0;
}

static int
cli_arp_limit_get(int fd)
{
	util_fdprintf(fd, "arp limit=%d\n",cpuport_arper_g.arp_limit);
	return 0;
}

// helper function used by cli.c /////////////////////////////////////////////////////////////////////////////////////////////

void
cli_arp_help(int fd)
{
	util_fdprintf(fd, 
		"arp limitget|limitset [num]\n");
}

int
cli_arp_cmdline(int fd, int argc, char *argv[])
{
	if (strcmp(argv[0], "arp")==0) {
		if (argc==1) {
			cli_arp_help(fd);		
			return 0;
		} else if (argc==2 && strcmp(argv[1], "limitget") == 0) {
			cli_arp_limit_get(fd);
			return 0;
		} else if (argc==3 && strcmp(argv[1], "limitset") == 0) {
			cli_arp_limit_change(util_atoi(argv[2]));
			return 0;
		}
		return CLI_ERROR_SYNTAX;		
	} else {
		return CLI_ERROR_OTHER_CATEGORY;
	}	
}

