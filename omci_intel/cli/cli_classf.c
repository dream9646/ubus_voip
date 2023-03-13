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
 * File    : cli_classf.c
 *
 ******************************************************************/

#include <string.h>

#include "cli.h"
#include "util.h"
#include "classf.h"
#include "batchtab.h"
#include "switch.h"

static int 
cli_classf(int fd, unsigned char flag, unsigned short meid1, unsigned short meid2)
{
	struct classf_info_t *classf_info;

	//get classf table
	if ((classf_info = (struct classf_info_t *)batchtab_table_data_get("classf")) == NULL)
	{
		return CLI_ERROR_INTERNAL;
	}

	if (meid1 != 0xffff && meid2 != 0xffff)
	{
		//show lan-wan bridge port pair, meid1->lan port, meid2->wan port
		classf_print_show_port_pair(fd, flag, classf_info, meid1, meid2);
	} else if (meid1 != 0xffff) {
		//show bridge info
		classf_print_show_bridge(fd, flag, classf_info, meid1);
	} else if (meid1 == 0xffff && meid2 == 0xffff) {
		//show all
		classf_print_show_bridge(fd, flag, classf_info, 0xffff);
	} else {
		//show error msg
		batchtab_table_data_put("classf");
		return CLI_ERROR_INTERNAL;
	}

	batchtab_table_data_put("classf");
	return CLI_OK;			
}

static int 
cli_classf_hw(int fd, unsigned char type, unsigned char flag)
{

	if (switch_hw_g.classf_print(fd, type, flag) < 0)
	{
		//show error msg
		return CLI_ERROR_INTERNAL;
	}

	return CLI_OK;			
}

int
cli_classf_gem(int fd, int is_reset)
{
	struct classf_info_t *classf_info;

	//get classf table
	if ((classf_info = (struct classf_info_t *)batchtab_table_data_get("classf")) == NULL)
	{
		return CLI_ERROR_INTERNAL;
	}

	if (switch_hw_g.classf_gem_print(fd, classf_info, 0) < 0)
	{
		batchtab_table_data_put("classf");
		//show error msg
		return CLI_ERROR_INTERNAL;
	}

	batchtab_table_data_put("classf");
	return CLI_OK;			
}

/*
flag -
	0: show statistic table
	1: show individual vid
	2: show individual vid with bridge/router and upstream/downstream
	3: reset
vid -
	0~4095, 4097: untag
type -
	0: bridge
	1: router/nat
dir -
	0: upstream
	1: downstream
*/
int
cli_classf_stat(int fd, unsigned char flag, unsigned short vid, unsigned char type, unsigned char dir)
{
	struct classf_info_t *classf_info;

	//get classf table
	if ((classf_info = (struct classf_info_t *)batchtab_table_data_get("classf")) == NULL)
	{
		return CLI_ERROR_INTERNAL;
	}

	if (switch_hw_g.classf_stat_print(fd, classf_info, flag, vid, type, dir) < 0)
	{
		batchtab_table_data_put("classf");
		//show error msg
		return CLI_ERROR_INTERNAL;
	}

	batchtab_table_data_put("classf");
	return CLI_OK;			
}

static int 
cli_classf_hidden(int fd, unsigned char type, unsigned char flag)
{
	if (switch_hw_g.mac_table_print_valid(fd, 0) < 0) //type: RTK_LUT_L2UC
	{
		//show error msg
		return CLI_ERROR_INTERNAL;
	}

	return CLI_OK;			
}

// helper function used by cli.c /////////////////////////////////////////////////////////////////////////////////////////////

void
cli_classf_help(int fd)
{
	util_fdprintf(fd, 
		"classf [bridge_meid | lan_port_meid wan_bport_meid]\n"
		"	b\n" //show all bridges info
		"	r\n" // show all rules
		"	g\n" // show classf gem counter
		"	stat [vid] [b|r u|d]\n" //show statistic
		"	stat reset\n" //reset statistic
		"	dbg [bridge_meid | lan_port_meid wan_bport_meid]\n"
		"	hw [aclhit|ethertype|basic] [help|h|?]\n");
}

int
cli_classf_cmdline(int fd, int argc, char *argv[])
{
	unsigned short meid1=0, meid2=0;
	unsigned short vid;

	if (strcmp(argv[0], "classf")==0)
	{
		switch(argc)
		{
		case 1:
			return cli_classf(fd, 0, 0xffff, 0xffff);
		case 2:
			if (strcmp(argv[1], "hw")==0)
			{
				//show hardware tables
				return cli_classf_hw(fd, 0, 0);
			}else if (strcmp(argv[1], "dbg")==0) {
				return cli_classf(fd, 1, 0xffff, 0xffff);
			}else if (strcmp(argv[1], "b")==0) {
				return cli_classf(fd, 2, 0xffff, 0xffff);
			}else if (strcmp(argv[1], "r")==0) {
				return cli_classf(fd, 3, 0xffff, 0xffff);	
			}else if (strcmp(argv[1], "g")==0) {
				return cli_classf_gem(fd, 0);
			}else if (strcmp(argv[1], "stat")==0) {
				return cli_classf_stat(fd, 0, 0, 0, 0);	//show statistic table
			} else {
				meid1 = util_atoi(argv[1]);
				return cli_classf(fd, 0, meid1, 0xffff);
			}
			break;
		case 3:
			if (strcmp(argv[1], "hw")==0)
			{
				if (strcmp(argv[2], "aclhit")==0)
				{
					//show hardware tables, acl-hit part
					return cli_classf_hw(fd, 1, 0);
				} else if (strcmp(argv[2], "ethertype")==0) {
					//show hardware tables, ethertype part
					return cli_classf_hw(fd, 2, 0);
				} else if (strcmp(argv[2], "basic")==0) {
					//show hardware tables, basic part
					return cli_classf_hw(fd, 3, 0);
				} else if (strcmp(argv[2], "help")==0 ||
					strcmp(argv[2], "h")==0) {
					//show hardware help
					return cli_classf_hw(fd, 0, 1);
				}else if (strcmp(argv[2], "mt")==0) {
					return cli_classf_hidden(fd, 0, 0);
				}
			} else if (strcmp(argv[1], "dbg")==0) {
				meid1 = util_atoi(argv[1]);
				return cli_classf(fd, 1, meid1, 0xffff);
			} else if (strcmp(argv[1], "stat")==0) {
				if (strcmp(argv[2], "reset")==0)
				{
					return cli_classf_stat(fd, 3, 0, 0, 0);	//reset all
				}
				vid = util_atoi(argv[2]);
				return cli_classf_stat(fd, 1, vid, 0, 0);	//show individual vid
			} else {
				meid1 = util_atoi(argv[1]);
				meid2 = util_atoi(argv[2]);
				return cli_classf(fd, 0, meid1, meid2);
			}
			break;
		case 4:
			if (strcmp(argv[1], "hw")==0 &&
				(strcmp(argv[3], "help")==0 ||
				strcmp(argv[3], "h")==0))
			{
				if (strcmp(argv[2], "aclhit")==0)
				{
					//show hardware help, acl-hit part
					return cli_classf_hw(fd, 1, 1);
				} else if (strcmp(argv[2], "ethertype")==0) {
					//show hardware help, ethertype part
					return cli_classf_hw(fd, 2, 1);
				} else if (strcmp(argv[2], "basic")==0) {
					//show hardware help, basic part
					return cli_classf_hw(fd, 3, 1);
				}				
			} else if (strcmp(argv[1], "dbg")==0) {
				meid1 = util_atoi(argv[2]);
				meid2 = util_atoi(argv[3]);
				return cli_classf(fd, 1, meid1, meid2);
			} else {
				//return CLI_ERROR_SYNTAX;
				return CLI_ERROR_OTHER_CATEGORY;	// ret CLI_ERROR_OTHER_CATEGORY as diag also has cli 'classf'
			}
			break;
		case 5:
			if (strcmp(argv[1], "stat")==0 &&
				strcmp(argv[3], "b")==0 &&
				strcmp(argv[4], "u")==0)
			{
				vid = util_atoi(argv[2]);
				return cli_classf_stat(fd, 2, vid, 0, 0);	//show individual vid with bridge/router and upstream/downstream
			} else if (strcmp(argv[1], "stat")==0 &&
				strcmp(argv[3], "b")==0 &&
				strcmp(argv[4], "d")==0) {
				vid = util_atoi(argv[2]);
				return cli_classf_stat(fd, 2, vid, 0, 1);	//show individual vid with bridge/router and upstream/downstream
			} else if (strcmp(argv[1], "stat")==0 &&
				strcmp(argv[3], "r")==0 &&
				strcmp(argv[4], "u")==0) {
				vid = util_atoi(argv[2]);
				return cli_classf_stat(fd, 2, vid, 1, 0);	//show individual vid with bridge/router and upstream/downstream
			} else if (strcmp(argv[1], "stat")==0 &&
				strcmp(argv[3], "r")==0 &&
				strcmp(argv[4], "d")==0) {
				vid = util_atoi(argv[2]);
				return cli_classf_stat(fd, 2, vid, 1, 1);	//show individual vid with bridge/router and upstream/downstream
			}
			break;
		default:
			;
		}
		//return CLI_ERROR_SYNTAX;
		return CLI_ERROR_OTHER_CATEGORY;	// ret CLI_ERROR_OTHER_CATEGORY as diag also has cli 'classf'
	} else {
		return CLI_ERROR_OTHER_CATEGORY;
	}
}

