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
 * File    : batchtab_cb_mcastconf.c
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
#include "util_run.h"
#include "list.h"
#include "batchtab_cb.h"
#include "batchtab.h"

#include "meinfo.h"
#include "me_related.h"
#include "mcast.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////

struct mcastconf_info_t {
	unsigned short meid;
	int version;
	int function;
	int fastleave;
	int RV;
	int QI;
	int QRI;
	int LMQI;	
};

static struct mcastconf_info_t mcastbw_info_g;

static int
batchtab_cb_mcastconf_init(void)
{
        memset(&mcastbw_info_g, 0, sizeof(struct mcastconf_info_t));
        
	return 0;
}

static int
batchtab_cb_mcastconf_finish(void)
{
	return 0;
}

static int
batchtab_cb_mcastconf_gen(void **table_data)
{
	if (table_data == NULL) return 0;
	
	struct me_t *me;
	
	struct meinfo_t *miptr_309=meinfo_util_miptr(309);
	
	list_for_each_entry(me, &miptr_309->me_instance_list, instance_node) {
		mcastbw_info_g.meid    = me->meid;
		mcastbw_info_g.version = meinfo_util_me_attr_data(me, 1);
		mcastbw_info_g.function = meinfo_util_me_attr_data(me, 2);
		mcastbw_info_g.fastleave = meinfo_util_me_attr_data(me, 3);
		
		mcastbw_info_g.RV = meinfo_util_me_attr_data(me, 10);
		mcastbw_info_g.QI = meinfo_util_me_attr_data(me, 12);
		mcastbw_info_g.QRI = meinfo_util_me_attr_data(me, 13);
		mcastbw_info_g.LMQI = meinfo_util_me_attr_data(me, 14);
		
		break;
	}
	
	return 0;
}

static int
batchtab_cb_mcastconf_release(void *table_data)
{
	if(!table_data)
	{
		return 0;
	}

	if(switch_hw_g.ipmcast_me309_mode_set){
		switch_hw_g.ipmcast_me309_mode_set(mcastbw_info_g.meid,mcastbw_info_g.version,0);	
	}
	memset(&mcastbw_info_g,0x0,sizeof(struct mcastconf_info_t));
	return 0;
}

static int
batchtab_cb_mcastconf_dump(int fd, void *table_data)
{	

	return 0;
}

static int
batchtab_cb_mcastconf_hw_sync(void *table_data)
{
	// write conf to metafile
	if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX) == 0)
	{
		struct meinfo_t *me65317_miptr = meinfo_util_miptr(65317);
		short mode = 0;
		if (me65317_miptr)
		{
			struct me_t *me65317 = NULL;
			list_for_each_entry(me65317, &me65317_miptr->me_instance_list, instance_node)
			{ 
				mode = meinfo_util_me_attr_data(me65317, 1);
				break;
			}
		}
		
		if (!mode)
		{
			dbprintf_bat(LOG_ERR, "mcastconf changed in native mode\n"); 
			igmp_proxy_reload();
		}
	}

	if(switch_hw_g.ipmcast_me309_mode_set){
		switch_hw_g.ipmcast_me309_mode_set(mcastbw_info_g.meid,mcastbw_info_g.version,1);	
	}
	return 0;
}

// public register function /////////////////////////////////////
int
batchtab_cb_mcastconf_register(void)
{
	return batchtab_register("mcastconf", BATCHTAB_OMCI_UPDATE_NOAUTO, 3,
			batchtab_cb_mcastconf_init,
			batchtab_cb_mcastconf_finish,
			batchtab_cb_mcastconf_gen,
			batchtab_cb_mcastconf_release,
			batchtab_cb_mcastconf_dump,
			batchtab_cb_mcastconf_hw_sync);
}
