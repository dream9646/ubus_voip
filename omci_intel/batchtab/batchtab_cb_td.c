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
 * File    : batchtab_cb_td.c
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
#include "switch.h"
#include "tm.h"

// apply td configuration to uni bport ingress/egress and 2510 per PQ cir/pir
static struct batchtab_cb_td_t batchtab_cb_td_g;

static int
batchtab_cb_td_init(void)
{
	int pos;
	memset(&batchtab_cb_td_g, 0, sizeof(batchtab_cb_td_g));
	for(pos=0; pos < TD_DSGEM_MAX; pos++) {
		batchtab_cb_td_g.td_dsgem_map[pos].hw_rule_entry = -1; // hw_rule_entry has to be -1 because 0 is a valid entry
	}
	return 0;
}
static int
batchtab_cb_td_finish(void)
{
	return 0;
}
static int
batchtab_cb_td_gen(void **table_data)
{
	struct batchtab_cb_td_t *td= &batchtab_cb_td_g;
	td_uniport_generator(td);	//for UNI
	td_dsgem_generator(td);		//for DS GEM
	td_usgem_wanport_generator(td);	//for US GEM/WAN
	*table_data = td;
	return 0;
}
static int
batchtab_cb_td_release(void *table_data)
{
	struct batchtab_cb_td_t *td= (struct batchtab_cb_td_t *)table_data;

	if(!table_data)
	{
		return 0;
	}

	td_uniport_release(td);
	td_dsgem_release(td);
	td_usgem_wanport_release(td);
	return 0;
}
static int
batchtab_cb_td_dump(int fd, void *table_data)
{
	struct batchtab_cb_td_t *td= (struct batchtab_cb_td_t *)table_data;
	
	if (td == NULL)
		return 0;
	
	util_fdprintf(fd, "\n");
	td_uniport_dump(fd, td);
	td_dsgem_dump(fd, td);
	td_usgem_wanport_dump(fd, td);
	return 0;
}
static int
batchtab_cb_td_hw_sync(void *table_data)
{
	struct batchtab_cb_td_t *td= (struct batchtab_cb_td_t *)table_data;

	if(td == NULL)
		return 0;

	//batchtab_cb_tm_protect_start();

	td_uniport_sync_to_hardware(td);
	td_dsgem_sync_to_hardware(td);
	td_usgem_wanport_sync_to_hardware(td);

	// the 'td' hw sync will change pq which is referred by batchtab 'gemflow'
	batchtab_crosstab_update("gemflow");

	return 0;
}

// public register function /////////////////////////////////////
int
batchtab_cb_td_register(void)
{
	return batchtab_register("td", BATCHTAB_OMCI_UPDATE_NOAUTO, 3,
			batchtab_cb_td_init,
			batchtab_cb_td_finish,
			batchtab_cb_td_gen,
			batchtab_cb_td_release,
			batchtab_cb_td_dump,
			batchtab_cb_td_hw_sync);
}
