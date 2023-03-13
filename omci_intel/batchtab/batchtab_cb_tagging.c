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
 * Module  : batchtab
 * File    : batchtab_cb_tagging.c
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

#include "util.h"
#include "list.h"
#include "batchtab_cb.h"
#include "batchtab.h"
#include "meinfo.h"
#include "er_group.h"
#include "omciutil_vlan.h"
#include "omciutil_misc.h"
#include "hwresource.h"
#include "switch.h"

// generate tagging table for software pkt process in cpu port
static struct batchtab_cb_tagging_t batchtab_cb_tagging_g;	

static int
batchtab_cb_tagging_init(void)
{
	return 0;
}
static int
batchtab_cb_tagging_finish(void)
{
	return 0;
}
static int
batchtab_cb_tagging_gen(void **table_data)
{
	struct batchtab_cb_tagging_t *t = &batchtab_cb_tagging_g;	
	struct meinfo_t *miptr=meinfo_util_miptr(47);
	struct me_t *me;
	int ret=0;

	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		struct switch_port_info_t port_info;
		struct batchtab_cb_tagging_bport_t *tb;

		ret=switch_get_port_info_by_me(me, &port_info);
		if( ret != 0 )
			continue;

		if (port_info.port_type == ENV_BRIDGE_PORT_TYPE_WAN || 
			port_info.port_type == ENV_BRIDGE_PORT_TYPE_DS_BCAST){ // tagging is not supported on wan port 
			continue;
		}

		// collect vlan for per bridge port
		tb = &(t->tagging_bport[t->total]);
		tb->bport_meid = me->meid;
		if (omciutil_vlan_collect_rules_by_port(ER_ATTR_GROUP_HW_OP_NONE, me, NULL, &tb->rule_list_us, &tb->rule_list_ds, 0) < 0) {
			dbprintf_bat(LOG_ERR, "vlan collect rule by port err: classid=%u, meid=%u\n", me->classid, me->meid);
		}
		t->total++;
		if (t->total >= 63) {
			dbprintf_bat(LOG_ERR, "batch cb tagging found 64 bridge port?\n");
			break;
		}
	}
	
	// update pointer for batchtab
	*table_data =t;
	return 0;
}
static int
batchtab_cb_tagging_release(void *table_data)
{
	struct batchtab_cb_tagging_t *t = (struct batchtab_cb_tagging_t *)table_data;
	int i;
	
	if(!table_data)
	{
		return 0;
	}

	for (i=0; i < t->total; i++) {
		struct batchtab_cb_tagging_bport_t *tb = &(t->tagging_bport[i]);
		omciutil_vlan_release_rule_list(&(tb->rule_list_us));
		omciutil_vlan_release_rule_list(&(tb->rule_list_ds));
	}
	t->total = 0;
	return 0;
}
static int
batchtab_cb_tagging_dump(int fd, void *table_data)
{
	struct batchtab_cb_tagging_t *t = (struct batchtab_cb_tagging_t *)table_data;
	struct omciutil_vlan_rule_fields_t *rule_fields_pos;
	char *devname;
	int i;
	
	if(t == NULL)
		return 0;

	for (i=0; i < t->total; i++) {
		struct batchtab_cb_tagging_bport_t *tb = &(t->tagging_bport[i]);
		struct me_t *bport_me = meinfo_me_get_by_meid(47, tb->bport_meid);

		util_fdprintf(fd, "\n<[47]bport:0x%x", tb->bport_meid); 
		if (bport_me && (devname=hwresource_devname(bport_me)) != NULL)
			util_fdprintf(fd, " (%s)", devname); 

		list_for_each_entry(rule_fields_pos, &(tb->rule_list_us), rule_node) {
			util_fdprintf(fd, "\n\tUS %c%s", rule_fields_pos->unreal_flag? '*': ' ',omciutil_vlan_str_desc(rule_fields_pos,"\n\t\t"," ", 0));
		}
		list_for_each_entry(rule_fields_pos, &(tb->rule_list_ds), rule_node) {					
			util_fdprintf(fd, "\n\tDS %c%s", rule_fields_pos->unreal_flag? '*': ' ', omciutil_vlan_str_desc(rule_fields_pos,"\n\t\t"," ", 0));
		}
	}
	util_fdprintf(fd, "\n"); 

	return 0;
}
static int
batchtab_cb_tagging_hw_sync(void *table_data)
{
	return 0;
}


// public routeins for other batchtab
int
batchtab_cb_tagging_is_ready()
{
	struct batchtab_cb_tagging_t *t = batchtab_table_data_get("tagging");

	if (t == NULL)
		return 0;
	// t has value since here
	if (t->total) {	// bat tagging has found some bport (veip/iphost/uni...)
		batchtab_table_data_put("tagging");
		return 1;
	}
	batchtab_table_data_put("tagging");

	{ // me 47 has veip/uni related bport but vtagging doesn't found any one of them
		struct meinfo_t *miptr=meinfo_util_miptr(329);
		struct me_t *me;
		list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
			if (omciutil_misc_find_me_related_bport(me) != NULL)	// veip bport exist, but bat tagging found no bport?
				return 0;
		}
	}
	// veip bport not exist and vtagging found no bport either
	return 1;
}

// public register function /////////////////////////////////////

int
batchtab_cb_tagging_register(void)
{
	return batchtab_register("tagging", BATCHTAB_OMCI_UPDATE_NOAUTO, 3,
			batchtab_cb_tagging_init,
			batchtab_cb_tagging_finish,
			batchtab_cb_tagging_gen,
			batchtab_cb_tagging_release,
			batchtab_cb_tagging_dump,
			batchtab_cb_tagging_hw_sync);
}
