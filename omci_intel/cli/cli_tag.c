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
 * File    : cli_tag.c
 *
 ******************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <dirent.h>
#include <fcntl.h>
#include <netinet/ip.h>

#include "util.h"
#include "meinfo.h"
#include "cli.h"
#include "me_related.h"
#include "er_group.h"
#include "omciutil_vlan.h"
#include "cpuport.h"
#include "switch.h"
#include "hwresource.h"

static int 
cli_tag_helpf(int fd)
{
	util_fdprintf(fd, "%s\n",
		"opri(pr):                         ipri(pr):\n"
		"0..7 match                        0..7 match\n"
		"8 dont care                       8 dont care\n"
		"14 default rule for 2 tags        14 default rule for 1 tag\n"
		"15 rule for 0/1 tag         	   15 rule for 0 tag\n"
		"\n"
		"ovid:                             ivid:\n"
		"0..4094 match                     0..4094 match\n"
		"4096 dont care                    4096 dont care\n"
		"\n"
		"otpid_de(t):                      itpid_de(t):\n"
		"0 dont care                       0 dont care\n"
		"4 match 0x8100                    4 match 0x8100\n"
		"5 match input tpid, dont care de  5 match input tpid, dont care de\n"
		"6 match input tpid, de=0          6 match input tpid, de=0\n"
		"7 match input tpid, de=1          7 match input tpid, de=1\n"
		"\n"
		"ethernet type(e):\n"
		"0 dont care\n"
		"1 match 0x0800 (ip)\n"
		"2 match 0x8863/0x8864 (pppoe)\n"
		"3 match 0x0806 (arp)\n"
		);
	return CLI_OK;			
}

//util_fdprintf(fd, "dir f[pr, ovid, t, pr, ivid, t], e, r, t[pr, ovid, t, pr, ivid, t],  input, output\n");
static int 
cli_tag_helpt(int fd)
{
	util_fdprintf(fd, "%s\n",
		"rmtag(r):\n"
		"0..2 remove 0,1 or 2 tag\n"
		"3 discard\n"
		"\n"
		"opri(pr):                            ipri(pr):\n"
		"0..7 add otag, set value asis        0..7 add itag, set value asis\n"
		"8 add otag, cp from itag             8 add itag, cp from itag\n"
		"9 add otag, cp from otag             9 add itag, cp from otag\n"
		"10 add otag, p-bit mapped from dscp  10 add itag, p-bit mapped from dscp\n"
		"15 dont add otag                     15 dont add itag\n"
		"\n"
		"ovid:                                ivid:\n"
		"0..4094 set ovid asis                0..4094 set ivid asis\n"
		"4096 cp from itag                    4096 cp from itag\n"
		"4097 cp from otag                    4097 cp from otag\n"
		"\n"
		"otpid_de(t):                         itpid_de(t):\n"
		"0 cp from itag                       0 cp from itag\n"
		"1 cp from otag                       1 cp from otag\n"
		"2 otpid=output_tpid, de from itag    2 itpid=output_tpid, de from itag\n"
		"3 otpid=output_tpid, de from otag    3 itpid=output_tpid, de from otag\n"
		"4 set to 0x8100                      4 set to 0x8100\n"
		"6 otpid=output_tpid, de=0            6 itpid=output_tpid, de=0\n"
		"7 optid=output_tpid, de=1            7 iptid=output_tpid, de=1\n"
		);
	return CLI_OK;			
}

static void
cli_tag_print_rule_fields(int fd, char *prefix, struct omciutil_vlan_rule_fields_t *rule_fields)
{
	if (rule_fields==NULL)
		return;
	util_fdprintf(fd, "%2s:%s f[%2u, %4u, %1u, %2u, %4u, %1u], %1u, %1u, t[%2u, %4u, %1u, %2u, %4u, %1u], 0x%04x, 0x%04x %03u:0x%x(%u)\n",
		prefix,
		rule_fields->unreal_flag?"*":" ",
		rule_fields->filter_outer.priority,
		rule_fields->filter_outer.vid,
		rule_fields->filter_outer.tpid_de,
		rule_fields->filter_inner.priority,
		rule_fields->filter_inner.vid,
		rule_fields->filter_inner.tpid_de,
		rule_fields->filter_ethertype,
		rule_fields->treatment_tag_to_remove,
		rule_fields->treatment_outer.priority,
		rule_fields->treatment_outer.vid,
		rule_fields->treatment_outer.tpid_de,
		rule_fields->treatment_inner.priority,
		rule_fields->treatment_inner.vid,
		rule_fields->treatment_inner.tpid_de,
		rule_fields->output_tpid,
		rule_fields->input_tpid,
		rule_fields->owner_me?rule_fields->owner_me->classid:0,
		rule_fields->owner_me?rule_fields->owner_me->meid:0,
		rule_fields->owner_me?rule_fields->owner_me->meid:0);
}

static int 
cli_tag(int fd, unsigned short bridge_meid)
{
	struct meinfo_t *bridge_miptr, *port_miptr;
	struct me_t *bridge_me, *port_me;
	
	// 45 MAC_bridge_service_profile
	bridge_miptr= meinfo_util_miptr(45);	
	if (bridge_miptr==NULL) {
		util_fdprintf(fd, "class=%d, null miptr?\n", 45);
		return CLI_ERROR_INTERNAL;
	}		
	// 47 MAC_bridge_port_configuration_data
	port_miptr= meinfo_util_miptr(47);	
	if (port_miptr==NULL) {
		util_fdprintf(fd, "class=%d, null miptr?\n", 47);
		return CLI_ERROR_INTERNAL;
	}		

	// traverse all bridges
	list_for_each_entry(bridge_me, &bridge_miptr->me_instance_list, instance_node) {
		if (bridge_me->meid!=bridge_meid && bridge_meid!=0xffff)
			continue;
		
		util_fdprintf(fd, "bridge meid=0x%x(%u)\n", bridge_me->meid, bridge_me->meid);

		// find all ports for specific bridge
		list_for_each_entry(port_me, &port_miptr->me_instance_list, instance_node) {
			struct list_head rule_list_us, rule_list_ds;
			struct omciutil_vlan_rule_fields_t *rule_fields_pos;
			int is_port_printed=0;
	
			if (!me_related(port_me, bridge_me))
				continue;
				
			if (omciutil_vlan_collect_rules_by_port(ER_ATTR_GROUP_HW_OP_NONE, port_me, NULL, &rule_list_us, &rule_list_ds, 0) < 0) {
				util_fdprintf(fd, "bridge port meid=0x%x(%u), omciutil_vlan_manager_issue err?\n", port_me->meid, port_me->meid);
				continue;
			}
			
			list_for_each_entry(rule_fields_pos, &rule_list_us, rule_node) {
				if (!is_port_printed) {
					util_fdprintf(fd, "bport meid=0x%x(%u) id=%u \n", port_me->meid, port_me->meid, meinfo_util_me_attr_data(port_me, 2));
					util_fdprintf(fd, "dir  f[pr, ovid, t, pr, ivid, t], e, r, t[pr, ovid, t, pr, ivid, t],  input, output, owner me\n");
					is_port_printed=1;
				}
				cli_tag_print_rule_fields(fd, "US", rule_fields_pos);
				util_fdprintf(fd, "%s\n", omciutil_vlan_str_desc(rule_fields_pos, "\n\t", "", 0));
			}
			list_for_each_entry(rule_fields_pos, &rule_list_ds, rule_node) {					
				if (!is_port_printed) {
					util_fdprintf(fd, "bport meid=0x%x(%u) id=%u \n", port_me->meid, port_me->meid, meinfo_util_me_attr_data(port_me, 2));
					util_fdprintf(fd, "dir  f[pr, ovid, t, pr, ivid, t], e, r, t[pr, ovid, t, pr, ivid, t],  input, output, owner me\n");
					is_port_printed=1;
				}
				cli_tag_print_rule_fields(fd, "DS", rule_fields_pos);			
				util_fdprintf(fd, "%s\n", omciutil_vlan_str_desc(rule_fields_pos, "\n\t", "", 0));
			}

			//release all in list
			omciutil_vlan_release_rule_list(&rule_list_us);
			omciutil_vlan_release_rule_list(&rule_list_ds);
		}
	}
	return CLI_OK;			
}


// helper function used by cli.c /////////////////////////////////////////////////////////////////////////////////////////////

void
cli_tag_help(int fd)
{
	util_fdprintf(fd, 
		"tag helpf|helpt[bridge_meid]\n");
}

int
cli_tag_cmdline(int fd, int argc, char *argv[])
{
	unsigned short meid=0;

	if (strcmp(argv[0], "tag")==0 && argc <=2) {
		if (argc==1) {
			return cli_tag(fd, 0xffff);
		} else if (argc==2) {
			if (strcmp(argv[1], "helpf")==0) {
				return cli_tag_helpf(fd);
			} else if (strcmp(argv[1], "helpt")==0) {
				return cli_tag_helpt(fd);
			} else {
				meid = util_atoi(argv[1]);
				return cli_tag(fd, meid);
			}
		}
		return CLI_ERROR_SYNTAX;		

	} else {
		return CLI_ERROR_OTHER_CATEGORY;
	}	
}

