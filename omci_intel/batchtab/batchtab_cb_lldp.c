/******************************************************************
 *
 * Copyright (C) 2018 5V Technologies Ltd.
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
 * File    : batchtab_cb_lldp.c
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
#include <errno.h>

#include "util.h"
#include "util_run.h"
#include "list.h"
#include "batchtab_cb.h"
#include "batchtab.h"
#include "hwresource.h"
#include "meinfo.h"
#include "switch.h"
#include "omciutil_misc.h"
#include "omcimsg.h"
#include "tm.h"

#include "metacfg_adapter.h"
#include "lldp_core.h"

struct lldp_info_t {
	enum lldp_adminStatus admin;
	lldp_network_policy_t np;
};

struct lldp_info_t lldp_info_g[SWITCH_LOGICAL_PORT_TOTAL];
int lldp_mgt_g = 0;

////////////////////////////////////////////////////
static int batchtab_cb_lldp_get_mgt(void)
{
#if 1	
#ifdef OMCI_METAFILE_ENABLE
	struct metacfg_t  kv_config;
	int lldp_mgt;

	memset(&kv_config, 0x0, sizeof(struct metacfg_t));
	metacfg_adapter_config_init(&kv_config);
	metacfg_adapter_config_file_load_part(&kv_config, "/etc/wwwctrl/metafile.dat", "lldp_management_capability", "lldp_management_capability");

	lldp_mgt = util_atoi(metacfg_adapter_config_get_value(&kv_config, "lldp_management_capability", 1));

	metacfg_adapter_config_release(&kv_config);

	return lldp_mgt;
#else
	return 0;
#endif
#else
	struct kv_config_t kv_config;
	int lldp_mgt;

	kv_config_init(&kv_config);
	kv_config_file_load_part(&kv_config, "/etc/wwwctrl/metafile.dat", "lldp_management_capability", "lldp_management_capability");

	lldp_mgt = util_atoi(kv_config_get_value(&kv_config, "lldp_management_capability", 1));

	kv_config_release(&kv_config);

	return lldp_mgt;
#endif	
}

static int
batchtab_cb_lldp_init(void)
{
	int port_id;
	for (port_id=0;port_id < SWITCH_LOGICAL_PORT_TOTAL;port_id++) {
		lldp_info_g[port_id].admin = LLDP_RXTX;
		lldp_info_g[port_id].np.type = 1;
		lldp_info_g[port_id].np.policy = 0;
		lldp_info_g[port_id].np.tagged = 0;
		lldp_info_g[port_id].np.vid  = LLDP_DEFAULT_VID;
		lldp_info_g[port_id].np.pbit = LLDP_DEFAULT_PBIT;
		lldp_info_g[port_id].np.dscp = LLDP_DEFAULT_DSCP;
	}
	return 0;
}

static int
batchtab_cb_lldp_finish(void)
{
	return 0;
}

static int
batchtab_cb_lldp_gen(void **table_data)
{
	struct meinfo_t *me65319_miptr = meinfo_util_miptr(65319);
	struct lldp_info_t *m = lldp_info_g;
	unsigned char port_id;

	if (table_data == NULL)
		return 0;

	lldp_mgt_g = batchtab_cb_lldp_get_mgt();

	if (lldp_mgt_g == 0) {
		if (me65319_miptr)
		{
			struct me_t *me65319 = NULL;
			list_for_each_entry(me65319, &me65319_miptr->me_instance_list, instance_node)
			{
				struct me_t *pptp_me = meinfo_me_get_by_instance_num(11, me65319->instance_num);
				struct me_t *ipptp_me = hwresource_public2private(pptp_me);
				port_id = meinfo_util_me_attr_data(ipptp_me, 4);
				if (((1<<port_id) & switch_get_uni_logical_portmask()) && (port_id <  SWITCH_LOGICAL_PORT_TOTAL)) {
					m[port_id].admin = (unsigned char) meinfo_util_me_attr_data(me65319, 2);
					m[port_id].np.policy =(unsigned char) meinfo_util_me_attr_data(me65319, 4);
					m[port_id].np.vid = (unsigned short)meinfo_util_me_attr_data(me65319, 5);
					if (m[port_id].np.vid > 4095) {
						m[port_id].np.vid = LLDP_DEFAULT_VID;
						m[port_id].np.tagged = 0; /* 4096 = untagged */
					} else {
						m[port_id].np.tagged = 1;
					}
					m[port_id].np.pbit = (unsigned char)meinfo_util_me_attr_data(me65319, 6);
					m[port_id].np.dscp = (unsigned char)meinfo_util_me_attr_data(me65319, 7);
				}
			}
		}
	}

	*table_data = m;
	return 0;
}

static int
batchtab_cb_lldp_release(void *table_data)
{
	int port_id;
	struct lldp_info_t *m = table_data;

	if (m == NULL)
		return 0;

	for (port_id=0;port_id < SWITCH_LOGICAL_PORT_TOTAL;port_id++) {
		lldp_info_g[port_id].admin = LLDP_RXTX;
		lldp_info_g[port_id].np.type = 1;
		lldp_info_g[port_id].np.policy = 0;
		lldp_info_g[port_id].np.tagged = 0;
		lldp_info_g[port_id].np.vid  = LLDP_DEFAULT_VID;
		lldp_info_g[port_id].np.pbit = LLDP_DEFAULT_PBIT;
		lldp_info_g[port_id].np.dscp = LLDP_DEFAULT_DSCP;
	}

	return 0;
}

static int
batchtab_cb_lldp_dump(int fd, void *table_data)
{
	unsigned int port_id;
	struct lldp_info_t *m = table_data;
	lldp_network_policy_t* np_p;

	if (m == NULL)
		return 0;

	util_fdprintf(fd, "Management Owner : %s\n", (lldp_mgt_g==0?"OMCI":"Non-OMCI"));

	for (port_id= 0; port_id < SWITCH_LOGICAL_PORT_TOTAL; port_id ++) {
		if ((1<<port_id) & switch_get_uni_logical_portmask()) {
			np_p = &(m[port_id].np);
			util_fdprintf(fd, "%4s\t%6s\t%6s\t%6s\t%8s\t%4s\n", "Port", "Status", "Tagged", "VLANID", "Priority", "DSCP");
			util_fdprintf(fd, "%4d\t%6s\t%6d\t%6d\t%8d\t%4d\n", port_id,
					(m[port_id].admin==0?"Enable":"Disable"), np_p->tagged, np_p->vid, np_p->pbit, np_p->dscp);
		}
	}

	return 0;
}

static int
batchtab_cb_lldp_hw_sync(void *table_data)
{
#ifdef OMCI_CMD_DISPATCHER_ENABLE
	unsigned char port_id;
	char cmd[256], submit_kv_filename[128];
	struct lldp_info_t *m = table_data;

	if (m == NULL)
		return 0;

	if (lldp_mgt_g == 0) {
		snprintf(submit_kv_filename, 128, "/var/run/lldp.diff.%s", util_uptime_str());

		FILE *fp = fopen(submit_kv_filename, "w");

		if (fp == NULL) {
			dbprintf_bat(LOG_ERR, "%s open err: %s\n", submit_kv_filename, strerror(errno));
			return -1;
		}

		for (port_id=0;port_id < SWITCH_LOGICAL_PORT_TOTAL;port_id++) {
			fprintf(fp, "lldp_port%d_admin_state=%d\n", port_id, m[port_id].admin);
			fprintf(fp, "lldp_port%d_network_policy=%d\n", port_id, m[port_id].np.policy);
			fprintf(fp, "lldp_port%d_tagged=%d\n", port_id, m[port_id].np.tagged);
			if (m[port_id].np.tagged == 1) {
				fprintf(fp, "lldp_port%d_vlan_vid=%d\n", port_id, m[port_id].np.vid);
				fprintf(fp, "lldp_port%d_pbit=%d\n", port_id, m[port_id].np.pbit);
			}
			fprintf(fp, "lldp_port%d_dscp=%d\n", port_id, m[port_id].np.dscp);
		}

		fclose(fp);

		snprintf(cmd, sizeof(cmd), "/etc/init.d/cmd_dispatcher.sh diff %s; rm %s", submit_kv_filename, submit_kv_filename);
		util_run_by_system(cmd);
	}
#endif
	return 0;
}

// public register function /////////////////////////////////////
int
batchtab_cb_lldp_register(void)
{
	return batchtab_register("lldp", BATCHTAB_OMCI_UPDATE_NOAUTO, 3,
			batchtab_cb_lldp_init,
			batchtab_cb_lldp_finish,
			batchtab_cb_lldp_gen,
			batchtab_cb_lldp_release,
			batchtab_cb_lldp_dump,
			batchtab_cb_lldp_hw_sync);
}
