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
 * File    : batchtab_cb_filtering.c
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
#include "hwresource.h"
#include "switch.h"

// generate filtering table for software pkt process in cpu port
static struct batchtab_cb_filtering_t batchtab_cb_filtering_g;	

static int
batchtab_cb_filtering_init(void)
{
	return 0;
}
static int
batchtab_cb_filtering_finish(void)
{
	return 0;
}
static int
batchtab_cb_filtering_gen(void **table_data)
{
	struct batchtab_cb_filtering_t *f = &batchtab_cb_filtering_g;	
	struct meinfo_t *miptr=meinfo_util_miptr(84);
	struct me_t *filter_me=NULL, *bridge_port_me=NULL;
	unsigned char *tci_map;
	unsigned char i = 0;

	list_for_each_entry(filter_me, &miptr->me_instance_list, instance_node) {
		struct switch_port_info_t port_info;
		struct batchtab_cb_filtering_bport_t *fb;

		bridge_port_me=meinfo_me_get_by_meid(47, filter_me->meid); //me 84
		if (bridge_port_me) {
			switch_get_port_info_by_me(bridge_port_me, &port_info);

			// collect vlan for per bridge port
			fb = &(f->filtering_bport[f->total]);
			fb->bport_meid = bridge_port_me->meid;

			tci_map=filter_me->attr[1].value.ptr;
			fb->op_mode = meinfo_util_me_attr_data(filter_me,2);
			fb->entry_num = meinfo_util_me_attr_data(filter_me,3);

			for ( i = 0; i < fb->entry_num ; i++ ) {
				fb->pri[i] = util_bitmap_get_value(tci_map, 8*24, i*8*2, 3);
				fb->cfi[i] = util_bitmap_get_value(tci_map, 8*24, i*8*2+3, 1);
				fb->vid[i] = util_bitmap_get_value(tci_map, 8*24, i*8*2+4, 12);
			}

			f->total++;
			if (f->total >= 63) {
				dbprintf_bat(LOG_ERR, "batch cb filter found 64 bridge port?\n");
				break;
			}
		}
	}
	
	// update pointer for batchtab
	*table_data =f;
	return 0;
}

static int
batchtab_cb_filtering_release(void *table_data)
{
	struct batchtab_cb_filtering_t *f = (struct batchtab_cb_filtering_t *)table_data;
	int i;

	if(!table_data)
	{
		return 0;
	}

	for (i=0; i < f->total; i++) {
		struct batchtab_cb_filtering_bport_t *fb = &(f->filtering_bport[i]);
		fb->bport_meid = 0xffff;
	}
	f->total = 0;
	return 0;
}

static int
batchtab_cb_filtering_dump(int fd, void *table_data)
{
	struct batchtab_cb_filtering_t *f = (struct batchtab_cb_filtering_t *)table_data;
	char *devname;
	int i;
	int j;
	
	if(f == NULL)
		return 0;

	for (i=0; i < f->total; i++) {
		struct batchtab_cb_filtering_bport_t *fb = &(f->filtering_bport[i]);
		struct me_t *bport_me = meinfo_me_get_by_meid(47, fb->bport_meid);

		util_fdprintf(fd, "\n<[47]bport:0x%x", fb->bport_meid); 
		if (bport_me && (devname=hwresource_devname(bport_me))!=NULL)
				util_fdprintf(fd, " (%s)", devname); 

		util_fdprintf(fd, "\nop=0x%x\n[pri,cfi,vid]:",fb->op_mode); 

		for (j=0;j< fb->entry_num; j++) {
			util_fdprintf(fd, "[%d,%d,%d]", fb->pri[j], fb->cfi[j], fb->vid[j]);
		}
		util_fdprintf(fd, "\n");

	}
	util_fdprintf(fd, "\n"); 

	return 0;
}

static int
batchtab_cb_filtering_hw_sync(void *table_data)
{
	return 0;
}

// public register function /////////////////////////////////////
int
batchtab_cb_filtering_register(void)
{
	return batchtab_register("filtering", BATCHTAB_OMCI_UPDATE_NOAUTO, 3,
			batchtab_cb_filtering_init,
			batchtab_cb_filtering_finish,
			batchtab_cb_filtering_gen,
			batchtab_cb_filtering_release,
			batchtab_cb_filtering_dump,
			batchtab_cb_filtering_hw_sync);			
}
