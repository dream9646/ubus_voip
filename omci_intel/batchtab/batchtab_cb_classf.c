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
 * File    : batchtab_cb_classf.c
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
#include "switch.h"
#include "classf.h"

// classifiction
static struct classf_info_t batchtab_cb_classf_info_g;

static int
batchtab_cb_classf_init(void)
{
	memset(&batchtab_cb_classf_info_g, 0x00, sizeof(struct classf_info_t));
	INIT_LIST_HEAD(&batchtab_cb_classf_info_g.bridge_info_list);
	INIT_LIST_HEAD(&batchtab_cb_classf_info_g.system_info.protocol_vlan_rule_list);
	INIT_LIST_HEAD(&batchtab_cb_classf_info_g.system_info.tlan_info.uni_tlan_list);
	INIT_LIST_HEAD(&batchtab_cb_classf_info_g.system_info.tlan_info.uni_nontlan_list);
	INIT_LIST_HEAD(&batchtab_cb_classf_info_g.system_info.tlan_info.ani_tlan_list);
	INIT_LIST_HEAD(&batchtab_cb_classf_info_g.system_info.tlan_info.ani_nontlan_list);
	fwk_mutex_create(&batchtab_cb_classf_info_g.mutex);
	return 0;
}
static int
batchtab_cb_classf_finish(void)
{
	fwk_mutex_destroy(&batchtab_cb_classf_info_g.mutex);
	return 0;
}
static int
batchtab_cb_classf_gen(void **table_data)
{
	*table_data = &batchtab_cb_classf_info_g;
	return classf_rule_collect(&batchtab_cb_classf_info_g);
}
static int
batchtab_cb_classf_release(void *table_data)
{
	struct classf_info_t *classf_info = table_data;

	if(!table_data)
	{
		return 0;
	}

	classf_info->is_hwsync = 0;
	switch_hw_g.classf_manage(classf_info);

	classf_rule_release(classf_info);

	return 0;
}
static int
batchtab_cb_classf_dump(int fd, void *table_data)
{
	struct classf_info_t *classf_info = table_data;
	
	if (classf_info == NULL)
		return 0;

	//show all
	classf_print_show_bridge(fd, 0, classf_info, 0xffff);
	return 0;
}

static int
batchtab_cb_classf_hw_sync(void *table_data)
{
	struct classf_info_t *classf_info = table_data;
	int ret;
#ifdef OMCI_CMD_DISPATCHER_ENABLE
	static unsigned char dscp2pbit_table[64]={127};
#endif

	if(classf_info == NULL)
		return 0;

	classf_info->is_hwsync = 1;
	ret = switch_hw_g.classf_manage(classf_info);

	batchtab_crosstab_update("wanif");
	if (ret == 0) {
#ifdef OMCI_CMD_DISPATCHER_ENABLE
		int i;
		char cmd[256];
		unsigned char dscp2pbit_str[65];
		if (dscp2pbit_table[0] != 127 && 0 == memcmp(classf_info->system_info.dscp2pbit_info.table, dscp2pbit_table, 64)) {
			dbprintf_bat(LOG_WARNING, "dscp2pbit same value and not first time, skip it!\n", cmd);
		} else {
			memcpy(dscp2pbit_table, classf_info->system_info.dscp2pbit_info.table, 64);
			for (i = 0; i < 64; i++)
				dscp2pbit_str[i]='0'+dscp2pbit_table[i];
			dscp2pbit_str[i]='\0';
			dbprintf_bat(LOG_ERR, "cmd_dispatcher.sh set system_dscp2pbit_map=[%s]\n", dscp2pbit_str);
			snprintf(cmd, 255, "/etc/init.d/cmd_dispatcher.sh set system_dscp2pbit_map=%s", dscp2pbit_str);
			util_run_by_thread(cmd, 0);
		}
#endif
	}
	return ret;
}

// public register function /////////////////////////////////////
int
batchtab_cb_classf_register(void)
{
	return batchtab_register("classf", BATCHTAB_OMCI_UPDATE_NOAUTO, 3,
			batchtab_cb_classf_init,
			batchtab_cb_classf_finish,
			batchtab_cb_classf_gen,
			batchtab_cb_classf_release,
			batchtab_cb_classf_dump,
			batchtab_cb_classf_hw_sync);
}
