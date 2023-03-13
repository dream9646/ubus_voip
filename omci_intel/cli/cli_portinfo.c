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
 * File    : cli_portinfo.c
 *
 ******************************************************************/

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "stdio.h"
#include "list.h"
#include "util.h"
#include "cli.h"
#include "meinfo.h"
#include "switch.h"

static int 
cli_portinfo(int fd, struct switch_port_info_t *port_info)
{
	char *port_type_str[] = { "cpu(veip)", "uni", "wan", "iphost", "bs bcast", NULL };
	char *s = "?";

	if (port_info->port_type <= 4)
		s = port_type_str[port_info->port_type];

	util_fdprintf(fd, "port_type=%d(%s)\n", port_info->port_type, s);
	util_fdprintf(fd, "port_type_index=%d\n", port_info->port_type_index);
	util_fdprintf(fd, "logical_port_id=%d\n", port_info->logical_port_id);
	if (port_info->bridge_port_me) {
		util_fdprintf(fd, "bport_me=%p, classid=0x%x(%u), meid=0x%x(%u)\n", 
			port_info->bridge_port_me,
			port_info->bridge_port_me->classid, port_info->bridge_port_me->classid,
			port_info->bridge_port_me->meid, port_info->bridge_port_me->meid);
	} else {
		util_fdprintf(fd, "bport_me=NULL\n");
	}

	return 0;	
}

// helper function used by cli.c /////////////////////////////////////////////////////////////////////////////////////////////

void
cli_portinfo_help(int fd)
{
	util_fdprintf(fd, 
		"portinfo me [classid] [meid]\n"
		"portinfo logical [id]\n"
		"portinfo uni [id]\n");
}

int
cli_portinfo_cmdline(int fd, int argc, char *argv[])
{
	struct switch_port_info_t port_info;
	
	if (argc >=1 && (strcmp(argv[0], "portinfo")==0 || strcmp(argv[0], "pinfo")==0)) {
		if (argc==4 && strcmp(argv[1], "me")==0) {
			unsigned short classid = util_atoi(argv[2]);
			unsigned short meid = util_atoi(argv[3]);
			struct me_t *me=meinfo_me_get_by_meid(classid, meid);
			if (!me) {
				util_fdprintf(fd, "me not found, classid=0x%x(%u), meid=0x%x(%u)\n",
					classid, classid, meid, meid);
				return CLI_ERROR_RANGE;
			}

			switch_get_port_info_by_me(me, &port_info);	
			cli_portinfo(fd, &port_info);
			return CLI_OK;
		
		} else if (argc==3 && strcmp(argv[1], "logical")==0) {
			unsigned int logical_port_id = util_atoi(argv[2]);

			switch_get_port_info_by_logical_port_id(logical_port_id, &port_info);
			cli_portinfo(fd, &port_info);
			return CLI_OK;						
			
		} else if (argc==3 && strcmp(argv[1], "uni")==0) {
			unsigned int uni = util_atoi(argv[2]);

			switch_get_port_info_by_uni_num(uni, &port_info);
			cli_portinfo(fd, &port_info);
			return CLI_OK;
			
		} else {
			cli_portinfo_help(fd);
			return 0;
		}

	} else {
		return CLI_ERROR_OTHER_CATEGORY;
	}	
}

