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
 * File    : batchtab_cb_gemflow.c
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
#include "tm.h"

// map between gemport <->streamid 
static struct batchtab_cb_gemflow_t batchtab_cb_gemflow_g;

static int
batchtab_cb_gemflow_init(void)
{
	return 0;
}
static int
batchtab_cb_gemflow_finish(void)
{
	return 0;
}
static int
batchtab_cb_gemflow_gen(void **table_data)
{
	struct batchtab_cb_gemflow_t *flow= &batchtab_cb_gemflow_g;
	int ret = gemflow_generator(flow);
	*table_data = flow;
	if (ret != 0)
		ret = -1;
	return ret;
}
static int
batchtab_cb_gemflow_release(void *table_data)
{
	struct batchtab_cb_gemflow_t *flow = (struct batchtab_cb_gemflow_t *)table_data;

	if(!table_data)
	{
		return 0;
	}

	gemflow_release(flow);
	return 0;
}
static int
batchtab_cb_gemflow_dump(int fd, void *table_data)
{
	struct batchtab_cb_gemflow_t *flow = (struct batchtab_cb_gemflow_t *)table_data;

	if (table_data == NULL)
		return 0;

	gemflow_dump(fd, flow);
	return 0;
}
static int
batchtab_cb_gemflow_hw_sync(void *table_data)
{
	struct batchtab_cb_gemflow_t *flow = (struct batchtab_cb_gemflow_t *)table_data;
	int ret;

	if(flow == NULL)
		return 0;

	//batchtab_cb_tm_protect_start();
	ret = gemflow_sync_to_hardware(flow);
	// batchtab_cb_tm_protect_stop() was not called intensionally
	// as classf will be issued because of batchtab_crosstab_update classf
	// and batchtab_cb_tm_protect_stop() will be called at the end of class hwsync

	// while sync_to_hardware() != 0
	// which means part of the sync has err, however, other part might be still ok
	// so the related crosstab update should be triggered anyway

	// the 'gemflow' hw sync will change usflow/dsflow which are referred by batchtab 'classf'
	batchtab_crosstab_update("classf");
	// the 'gemflow' hw sync will change usflow/dsflow which are referred by batchtab 'mcastmode'
	batchtab_crosstab_update("mcastmode");
	// the 'gemflow' hw sync will change usflow/dsflow which are referred by batchtab 'wan'
	batchtab_crosstab_update("wan");
	// the 'gemflow' hw sync will change usflow/dsflow which are referred by batchtab 'linkready'
	batchtab_crosstab_update("linkready");

	if (ret != 0)
		ret = -1;
	return ret;
}

// public register function /////////////////////////////////////
int
batchtab_cb_gemflow_register(void)
{
	return batchtab_register("gemflow", BATCHTAB_OMCI_UPDATE_NOAUTO, 3,
			batchtab_cb_gemflow_init,
			batchtab_cb_gemflow_finish,
			batchtab_cb_gemflow_gen,
			batchtab_cb_gemflow_release,
			batchtab_cb_gemflow_dump,
			batchtab_cb_gemflow_hw_sync);
}
