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
 * Module  : proprietary_calix
 * File    : batchtab_cb_calix_tlan.c
 *
 ******************************************************************/

#include <stdio.h>
#include <string.h>
#include "meinfo.h"
#include "list.h"
#include "util.h"
#include "util_run.h"
#include "batchtab.h"
#include "batchtab_cb.h"
#include "proprietary_calix.h"

unsigned int calix_tlan_total_g = 0;
unsigned char calix_tlan_g[4096] = {0};

////////////////////////////////////////////////////

static int
batchtab_cb_calix_tlan_init(void)
{
	calix_tlan_total_g = 0;
	memset(calix_tlan_g, 0, sizeof(unsigned char)*4096);
	return 0;
}

static int
batchtab_cb_calix_tlan_finish(void)
{
	calix_tlan_total_g = 0;
	memset(calix_tlan_g, 0, sizeof(unsigned char)*4096);
	return 0;
}

static int
batchtab_cb_calix_tlan_gen(void **table_data)
{
	struct meinfo_t *miptr = NULL;
   	struct me_t *meptr = NULL;
   	struct attr_value_t attr = {0, NULL};
	
	if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX) != 0)
		return 0;
	
	// Calculate whether TLAN function is enabled
	calix_tlan_total_g = 0;
	memset(calix_tlan_g, 0, sizeof(unsigned char)*4096);
	miptr = meinfo_util_miptr(65313);
	if(miptr) {
		list_for_each_entry(meptr, &miptr->me_instance_list, instance_node) {
			attr.data = 0;
			meinfo_me_attr_get(meptr, 4, &attr);
			if(attr.data == 1) {
				calix_tlan_total_g++;
				calix_tlan_g[meptr->meid] = 1;
			}
		}
	}
	
	// Allow mcast pass to wan if TLAN (transparent lan) service is there
	if(calix_tlan_total_g) {
		omci_env_g.cpuport_im_enable = 0;
		omci_env_g.cpuport_mcast_drop = 0;
		//omci_env_g.classf_ds_unmatch = 0;
	} else {
		omci_env_g.cpuport_im_enable = 1;
		omci_env_g.cpuport_mcast_drop = 1;
		//omci_env_g.classf_ds_unmatch = 1;
	}
	//batchtab_crosstab_update("classf");
	
	*table_data = &calix_tlan_total_g;
	return 0;
}

static int
batchtab_cb_calix_tlan_release(void *table_data)
{
	return 0;
}

static int
batchtab_cb_calix_tlan_dump(int fd, void *table_data)
{
	int i, *total = table_data;
	
	util_fdprintf(fd, "[calix_tlan] found=%d\n", (table_data) ? *total : 0);
	for(i=0; i<4096; i++) {
		if(calix_tlan_g[i])
			util_fdprintf(fd, "tlan[%d]=%d\n", i, calix_tlan_g[i]);
	}
	return 0;
}

static int
batchtab_cb_calix_tlan_hw_sync(void *table_data)
{
	return 0;
}

// public register function /////////////////////////////////////
int
batchtab_cb_calix_tlan_register(void)
{
	return batchtab_register("calix_tlan", BATCHTAB_OMCI_UPDATE_NOAUTO, 3,
		batchtab_cb_calix_tlan_init,
		batchtab_cb_calix_tlan_finish,
		batchtab_cb_calix_tlan_gen,
		batchtab_cb_calix_tlan_release,
		batchtab_cb_calix_tlan_dump,
		batchtab_cb_calix_tlan_hw_sync);
}
