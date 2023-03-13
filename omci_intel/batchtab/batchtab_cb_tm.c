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
 * File    : batchtab_cb_tm.c
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

// map tcont/ts/pq to 2510 tcont/pq 
static struct batchtab_cb_tm_t batchtab_cb_tm_g;	

static int
batchtab_cb_tm_init(void)
{
	int i;
	for(i=0; i < GPON_TCONT_ID_TOTAL; i++) {
		batchtab_cb_tm_g.multi_prio_rr_tree[i].tm_tcont_node_ptr=NULL;
		INIT_LIST_HEAD(&batchtab_cb_tm_g.multi_prio_rr_tree[i].tm_equiv_prio_list);
	}

	return 0;
}
static int
batchtab_cb_tm_finish(void)
{
	return 0;
}
static int
batchtab_cb_tm_gen(void **table_data)
{
	struct batchtab_cb_tm_t *tm = &batchtab_cb_tm_g;
	int ret = tm_generator(tm);
	*table_data =tm;
	if (ret !=0)
		ret = -1;
	return ret;
}
static int
batchtab_cb_tm_release(void *table_data)
{
	struct batchtab_cb_tm_t *tm = (struct batchtab_cb_tm_t *)table_data;

	batchtab_table_data_release("classf");
	if(!table_data)
	{
		return 0;
	}

	tm_release(tm);
	return 0;
}
static int
batchtab_cb_tm_dump(int fd, void *table_data)
{
	struct batchtab_cb_tm_t *tm = (struct batchtab_cb_tm_t *)table_data;
	
	if (tm == NULL)
		return 0;
	tm_dump(fd, tm);
	return 0;
}
static int
batchtab_cb_tm_hw_sync(void *table_data)
{
	struct batchtab_cb_tm_t *tm = (struct batchtab_cb_tm_t *)table_data;
	int onu_state, ret;

	if(tm == NULL)
		return 0;
	if (gpon_hw_g.onu_state_get(&onu_state) != 0)
		return 0;
	if (onu_state != GPON_ONU_STATE_O5)
		return 0;       // do tm hw sync only if in o5

	//batchtab_cb_tm_protect_start();
 
	ret = tm_sync_to_hardware(tm);
	// batchtab_cb_tm_protect_stop() was not called intensionally
	// as classf will be issued through batchtab_crosstab_update  gemflow -> classf
	// and batchtab_cb_tm_protect_stop() will be called at the end of class hwsync

	// while sync_to_hardware() != 0
	// which means part of the sync has err, however, other part might be still ok
	// so the related crosstab update should be triggered anyway
	
	// the 'tm' hw sync will change pq which is referred by batchtab 'td'
	batchtab_crosstab_update("td");

	// the 'tm' hw sync will change pq which is referred by batchtab 'gemflow'
	batchtab_crosstab_update("gemflow");

	if (ret != 0)
		ret = -1;
	return ret;
}

// public register function /////////////////////////////////////
int
batchtab_cb_tm_register(void)
{
	return batchtab_register("tm", BATCHTAB_OMCI_UPDATE_NOAUTO, 3,
			batchtab_cb_tm_init,
			batchtab_cb_tm_finish,
			batchtab_cb_tm_gen,
			batchtab_cb_tm_release,
			batchtab_cb_tm_dump,
			batchtab_cb_tm_hw_sync);
}
