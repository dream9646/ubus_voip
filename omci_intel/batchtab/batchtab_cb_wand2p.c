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
 * File    : batchtab_cb_wand2p.c
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
#include "hwresource.h"
#include "switch.h"

// generate wand2p table for omci me 171 rules collection
static struct batchtab_cb_wand2p_t batchtab_cb_wand2p_g;

static int
batchtab_cb_wand2p_init(void)
{
	memset(&batchtab_cb_wand2p_g, 0x00, sizeof(batchtab_cb_wand2p_g));
	fwk_mutex_create(&batchtab_cb_wand2p_g.mutex);
	return 0;
}

static int
batchtab_cb_wand2p_finish(void)
{
	fwk_mutex_destroy(&batchtab_cb_wand2p_g.mutex);
	return 0;
}

#define CLASSF_WAN_DSCP2PBIT_INTERFACE_TEMPLETE "dev_wan%u_interface"
#define CLASSF_WAN_DSCP2PBIT_ENABLE_TEMPLETE "wan%u_dscp2pbit_enable"
#define CLASSF_BRWAN_DSCP2PBIT_ENABLE_TEMPLETE "brwan%u_dscp2pbit_enable"

static int
batchtab_cb_wand2p_gen(void **table_data)
{
#if 1
	struct batchtab_cb_wand2p_t *t = &batchtab_cb_wand2p_g;
#ifdef OMCI_METAFILE_ENABLE
	struct metacfg_t kv_config_metafile;
	int i, wanif_vlan;
	char key_str_interface[32], key_str_enable[32], key_str_enable_br[32];
	char *value_ptr_interface, *value_ptr_enable, *value_ptr_enable_br;

	//load kvconfig for metafile.dat
	 memset(&kv_config_metafile, 0x0, sizeof(struct metacfg_t));
	 metacfg_adapter_config_init(&kv_config_metafile);
	
	//load from file to hash list
	if( metacfg_adapter_config_file_load_safe(&kv_config_metafile, "/etc/wwwctrl/metafile.dat") !=0)
	{
		//release metafile kvconfig
		 metacfg_adapter_config_release(&kv_config_metafile);
		*table_data =t;
		return 0;
	}

	fwk_mutex_lock(&t->mutex);
	
	//get value from input kvconfig
	for (i = 0; i < WANIF_INDEX_TOTAL; i++)
	{
		sprintf(key_str_interface, CLASSF_WAN_DSCP2PBIT_INTERFACE_TEMPLETE, i);
		sprintf(key_str_enable, CLASSF_WAN_DSCP2PBIT_ENABLE_TEMPLETE, i);
		sprintf(key_str_enable_br, CLASSF_BRWAN_DSCP2PBIT_ENABLE_TEMPLETE, i);

		//interface name must length > 0, and could parse vid from it, otherwise, forget it
		if(strlen(value_ptr_interface =  metacfg_adapter_config_get_value(&kv_config_metafile, key_str_interface, 1)) > 0 &&
			(wanif_vlan = wanif_devname_vlan(value_ptr_interface)) >= 0)
		{
			t->wand2p[i].enable = 1;
			t->wand2p[i].vid = wanif_vlan;
			
			if ((strlen(value_ptr_enable =  metacfg_adapter_config_get_value(&kv_config_metafile, key_str_enable, 1)) > 0 &&
				atoi(value_ptr_enable) == 1) ||
				(strlen(value_ptr_enable_br =  metacfg_adapter_config_get_value(&kv_config_metafile, key_str_enable_br, 1)) > 0 &&
				atoi(value_ptr_enable_br) == 1))
			{
				t->wand2p[i].dscp2pbit_enable = 1;
			} //otherwise, keep to 0					
		} //otherwise, continue to next one
	}
	
	fwk_mutex_unlock(&t->mutex);

	//release metafile kvconfig
	 metacfg_adapter_config_release(&kv_config_metafile);
#endif
	
	// update pointer for batchtab
	*table_data =t;
	return 0;
#else
	struct batchtab_cb_wand2p_t *t = &batchtab_cb_wand2p_g;
	struct kv_config_t kv_config_metafile;
	int i, wanif_vlan;
	char key_str_interface[32], key_str_enable[32], key_str_enable_br[32];
	char *value_ptr_interface, *value_ptr_enable, *value_ptr_enable_br;

	//load kvconfig for metafile.dat
	kv_config_init(&kv_config_metafile);
	
	//load from file to hash list
	if(kv_config_file_load_safe(&kv_config_metafile, "/etc/wwwctrl/metafile.dat") !=0)
	{
		//release metafile kvconfig
		kv_config_release(&kv_config_metafile);
		*table_data =t;
		return 0;
	}

	fwk_mutex_lock(&t->mutex);
	
	//get value from input kvconfig
	for (i = 0; i < WANIF_INDEX_TOTAL; i++)
	{
		sprintf(key_str_interface, CLASSF_WAN_DSCP2PBIT_INTERFACE_TEMPLETE, i);
		sprintf(key_str_enable, CLASSF_WAN_DSCP2PBIT_ENABLE_TEMPLETE, i);
		sprintf(key_str_enable_br, CLASSF_BRWAN_DSCP2PBIT_ENABLE_TEMPLETE, i);

		//interface name must length > 0, and could parse vid from it, otherwise, forget it
		if(strlen(value_ptr_interface = kv_config_get_value(&kv_config_metafile, key_str_interface, 1)) > 0 &&
			(wanif_vlan = wanif_devname_vlan(value_ptr_interface)) >= 0)
		{
			t->wand2p[i].enable = 1;
			t->wand2p[i].vid = wanif_vlan;
			
			if ((strlen(value_ptr_enable = kv_config_get_value(&kv_config_metafile, key_str_enable, 1)) > 0 &&
				atoi(value_ptr_enable) == 1) ||
				(strlen(value_ptr_enable_br = kv_config_get_value(&kv_config_metafile, key_str_enable_br, 1)) > 0 &&
				atoi(value_ptr_enable_br) == 1))
			{
				t->wand2p[i].dscp2pbit_enable = 1;
			} //otherwise, keep to 0					
		} //otherwise, continue to next one
	}
	
	fwk_mutex_unlock(&t->mutex);

	//release metafile kvconfig
	kv_config_release(&kv_config_metafile);
	
	// update pointer for batchtab
	*table_data =t;
	return 0;
#endif
}

static int
batchtab_cb_wand2p_release(void *table_data)
{
	struct batchtab_cb_wand2p_t *t = (struct batchtab_cb_wand2p_t *)table_data;

	if(!table_data)
	{
		return 0;
	}

	fwk_mutex_lock(&t->mutex);
	memset(t->wand2p, 0x00, sizeof(t->wand2p));
	fwk_mutex_unlock(&t->mutex);
	return 0;
}

static int
batchtab_cb_wand2p_dump(int fd, void *table_data)
{
	struct batchtab_cb_wand2p_t *t = (struct batchtab_cb_wand2p_t *)table_data;
	int i;
	
	if(t == NULL)
		return 0;

	util_fdprintf(fd, "Wan  Vid DSCP-2-PBIT\n");
	util_fdprintf(fd, "====================\n");

	fwk_mutex_lock(&t->mutex);

	for (i = 0; i < WANIF_INDEX_TOTAL; i++)
	{
		if (t->wand2p[i].enable == 1)
		{
			util_fdprintf(fd, "%3d %4d %11d\n", i, t->wand2p[i].vid, t->wand2p[i].dscp2pbit_enable);
		}
	}
	
	fwk_mutex_unlock(&t->mutex);
	
	util_fdprintf(fd, "\n"); 

	return 0;
}

static int
batchtab_cb_wand2p_hw_sync(void *table_data)
{
	return 0;
}

// public register function /////////////////////////////////////
int
batchtab_cb_wand2p_register(void)
{
	return batchtab_register("wand2p", BATCHTAB_OMCI_UPDATE_NOAUTO, 3,
			batchtab_cb_wand2p_init,
			batchtab_cb_wand2p_finish,
			batchtab_cb_wand2p_gen,
			batchtab_cb_wand2p_release,
			batchtab_cb_wand2p_dump,
			batchtab_cb_wand2p_hw_sync);
}

