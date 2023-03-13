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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "cli.h"
#include "mcast.h"

extern struct igmp_config_t igmp_config_g;

int 
cli_igmp_cmdline(int fd, int argc, char *argv[])
{
	if (igmp_config_g.cli_hook == NULL) return CLI_ERROR_OTHER_CATEGORY;
	
	return (*igmp_config_g.cli_hook)(fd, argc, argv);
}
