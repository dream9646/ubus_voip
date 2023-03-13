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
 * Module  : meinfo_hw
 * File    : meinfo_hw_poe.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <math.h>

#include "meinfo.h"
#include "meinfo_hw.h"
#include "util.h"
#include "util_run.h"
#include "hwresource.h"
#include "meinfo_hw_poe.h"
#include "metacfg_adapter.h"

// poe result from poe report binary ///////////////////////////////////////////
#define POE_REPORT_CMD_GEN	"/etc/init.d/poe.sh report_cmd"
#define POE_POWER_SET_CMD_GEN	"/etc/init.d/poe_cmd.sh maxpower"
#define POE_POWER_GET_CMD_GEN	"/etc/init.d/poe.sh maxpower"
#define POE_ENABLE_SET_CMD_GEN	"/etc/init.d/poe_cmd.sh enable"
#define POE_CLASS_SET_CMD_GEN	"/etc/init.d/poe_cmd.sh class"
#define POE_PRIORITY_SET_CMD_GEN	"/etc/init.d/poe_cmd.sh priority"
#define POE_MGTCAP_SET_CMD_GEN	"/etc/init.d/poe_cmd.sh management_capability"

static int update_poe_mgt_cap_with_config(int *mgt_cap)
{
#ifdef OMCI_METAFILE_ENABLE
	struct metacfg_t kv_config;
	if (mgt_cap == NULL)
		return -1;

	memset(&kv_config, 0x0, sizeof(struct metacfg_t));
	metacfg_adapter_config_init(&kv_config);
	metacfg_adapter_config_file_load_part(&kv_config, "/etc/wwwctrl/metafile.dat", "poe_management_capability", "poe_management_capability");
	*mgt_cap = util_atoi(metacfg_adapter_config_get_value(&kv_config, "poe_management_capability", 1));
	metacfg_adapter_config_release(&kv_config);
#endif
	return 0;
}

int
meinfo_hw_poe_set_power_budget(int power_budget)
{
	char cmd[64];
	memset(cmd, 0, sizeof(cmd));
	sprintf(cmd, "%s %d", POE_POWER_SET_CMD_GEN, power_budget);
	util_run_by_thread(cmd, 1);
	return 0;
}

int
meinfo_hw_poe_set_admin_state(int port, int enable)
{
	char cmd[64];
	memset(cmd, 0, sizeof(cmd));
	sprintf(cmd, "%s %d %d", POE_ENABLE_SET_CMD_GEN, port, enable);
	util_run_by_thread(cmd, 1);
	
	return 0;
}

int
meinfo_hw_poe_set_class(int port, int value)
{
	char cmd[64];
	memset(cmd, 0, sizeof(cmd));
	sprintf(cmd, "%s %d %d", POE_CLASS_SET_CMD_GEN, port, value);
	util_run_by_thread(cmd, 1);
	return 0;
}

int
meinfo_hw_poe_set_priority(int port, int value)
{
	char cmd[64];
	memset(cmd, 0, sizeof(cmd));
	sprintf(cmd, "%s %d %d", POE_PRIORITY_SET_CMD_GEN, port, value);
	util_run_by_thread(cmd, 1);
	return 0;
}

static int
meinfo_hw_poe_get_result_from_poe_report(struct poe_result_t *poe_result)
{
	static char report_cmd[128]={0};
	char *output=NULL;
	int ret;
	FILE *fp = NULL;
	
	static struct poe_result_t poe_result_prev;
	static int poe_result_prev_time = 0;
	int now = time(0);

	// as calling /etc/init.d/poe.sh is slow
	// if request is within same second, reply with previous result directly
	if (now == poe_result_prev_time) {
		*poe_result = poe_result_prev;
		return 0;
	}	

	memset((void*)poe_result, 0, sizeof(struct poe_result_t));	

	if (strlen(report_cmd)==0) {
		char *s;
		if (util_readcmd(POE_REPORT_CMD_GEN, &output) <0 || output==NULL) {
			if (output)
				free_safe(output);
			dbprintf(LOG_ERR, "cmd %s error\n", POE_REPORT_CMD_GEN);
			return -1;
		}
		s=util_trim(output);
		strncpy(report_cmd, s, 128);
		free_safe(output);
	}

	if((fp = popen(report_cmd, "r")) != NULL) {
		char buf[64]={0};
		while (fgets(buf, 64, fp) > 0) {
			int port_id, power, enable, classify, priority, status, over_current, invd_cnt, pwrd_cnt, ovl_cnt, udl_cnt, sc_cnt, mps_abs_cnt;
			ret = sscanf(buf, "%d %d %d %d %d %d %d %d %d %d %d %d %d",
				&port_id, &power, &enable, &classify, &priority, &status, &over_current,
				&invd_cnt, &pwrd_cnt, &ovl_cnt, &udl_cnt, &sc_cnt, &mps_abs_cnt);
			if (ret<13) {
				dbprintf(LOG_WARNING, "buf %s error (ret=%d)\n", buf, ret);
				pclose(fp);
				return -1;
			} else {
				poe_result->poe_report[port_id].power = power;
				poe_result->poe_report[port_id].enable = enable;
				poe_result->poe_report[port_id].classify = classify;
				poe_result->poe_report[port_id].priority = priority;
				poe_result->poe_report[port_id].status = status;
				poe_result->poe_report[port_id].over_current = over_current;
				poe_result->poe_report[port_id].invd_cnt = invd_cnt;
				poe_result->poe_report[port_id].pwrd_cnt = pwrd_cnt;
				poe_result->poe_report[port_id].ovl_cnt = ovl_cnt;
				poe_result->poe_report[port_id].udl_cnt = udl_cnt;
				poe_result->poe_report[port_id].sc_cnt = sc_cnt;
				poe_result->poe_report[port_id].mps_abs_cnt = mps_abs_cnt;
			}
		}
		pclose(fp);
	}
	
	// cache result for repeated access with same second
	poe_result_prev = *poe_result;
	poe_result_prev_time = time(0);	

	return 0;
}

int
meinfo_hw_poe_get_power_budget(int *poe_power)
{
	char *output=NULL;
	
	static int poe_power_prev=0;
	static int poe_power_prev_time = 0;
	int now = time(0);

	// as calling /etc/init.d/poe.sh is slow
	// if request is within same second, reply with previous result directly
	if (now == poe_power_prev_time) {
		*poe_power = poe_power_prev;
		return 0;
	}	

	if (util_readcmd(POE_POWER_GET_CMD_GEN, &output) <0 || output==NULL) {
		if (output)
			free_safe(output);
		dbprintf(LOG_WARNING, "cmd %s error\n", POE_POWER_GET_CMD_GEN);
		return -1;
	}

	// cache result for repeated access with same second
	*poe_power = poe_power_prev = atoi(output);
	poe_power_prev_time = time(0);	

	free_safe(output);
	return 0;
}

int
meinfo_hw_poe_get_management_capability(int *poe_mgt_cap)
{
	static int poe_mgt_cap_prev=0;
	static int poe_mgt_cap_prev_time = 0;
	int now = time(0);

	// as calling /etc/init.d/poe.sh is slow
	// if request is within same second, reply with previous result directly
	if (now == poe_mgt_cap_prev_time) {
		*poe_mgt_cap = poe_mgt_cap_prev;
		return 0;
	}

	// cache result for repeated access with same second
	update_poe_mgt_cap_with_config(poe_mgt_cap);
	poe_mgt_cap_prev = *poe_mgt_cap;
	poe_mgt_cap_prev_time = time(0);

	return 0;
}


int
meinfo_hw_poe_get_admin(int *poe_admin, int port_id)
{
#ifdef OMCI_METAFILE_ENABLE
	struct metacfg_t kv_config;
	char* meta_state_str[] = {"poe_port0_admin_state", "poe_port1_admin_state", "poe_port2_admin_state", "poe_port3_admin_state"};

	if (poe_admin == NULL || port_id > 3)
		return -1;

	memset(&kv_config, 0x0, sizeof(struct metacfg_t));
	metacfg_adapter_config_init(&kv_config);
	metacfg_adapter_config_file_load_part(&kv_config, "/etc/wwwctrl/metafile.dat", meta_state_str[port_id], meta_state_str[port_id]);
	*poe_admin = util_atoi(metacfg_adapter_config_get_value(&kv_config, meta_state_str[port_id], 1));
	metacfg_adapter_config_release(&kv_config);
#endif
	return 0;
}


// get function wrapper //////////////////////////////////////////////////

int
meinfo_hw_poe_get_threshold_result(struct poe_result_t *r)
{
	int ret=0;

	// init threshold/result to 0
	memset(r, 0, sizeof(struct poe_result_t));

	util_create_lockfile("/var/run/poe.lock", 30);
	
	if (meinfo_hw_poe_get_result_from_poe_report(r)<0)
		ret = -1;
	
	util_release_lockfile("/var/run/poe.lock");

	if (ret <0)
		return ret;

	return 0;			
}

// print function /////////////////////////////////////////////////////////////////////////////

int
meinfo_hw_poe_print_threshold_result(int fd, struct poe_result_t *p)
{
	int port_id, power = 0;
	
	for (port_id= 0; port_id < SWITCH_LOGICAL_PORT_TOTAL; port_id ++) {
		if ((1<<port_id) & switch_get_uni_logical_portmask()) {
			power += p->poe_report[port_id].power;
		}
	}
	util_fdprintf(fd, "pwr_consumption \t %d \t", power);
	util_fdprintf(fd, "\n\t");
	
	for (port_id= 0; port_id < SWITCH_LOGICAL_PORT_TOTAL; port_id ++) {
		if ((1<<port_id) & switch_get_uni_logical_portmask()) {
			util_fdprintf(fd, "\t uni%d \t", port_id);
		}
	}
	util_fdprintf(fd, "\n");
	
	util_fdprintf(fd, "poe_enable ");
	for (port_id= 0; port_id < SWITCH_LOGICAL_PORT_TOTAL; port_id ++) {
		if ((1<<port_id) & switch_get_uni_logical_portmask()) {
			util_fdprintf(fd, "\t %d \t", p->poe_report[port_id].enable);
		}
	}
	util_fdprintf(fd, "\n");
	
	util_fdprintf(fd, "classify ");
	for (port_id= 0; port_id < SWITCH_LOGICAL_PORT_TOTAL; port_id ++) {
		if ((1<<port_id) & switch_get_uni_logical_portmask()) {
			util_fdprintf(fd, "\t %d \t", p->poe_report[port_id].classify);
		}
	}
	util_fdprintf(fd, "\n");
	
	util_fdprintf(fd, "priority ");
	for (port_id= 0; port_id < SWITCH_LOGICAL_PORT_TOTAL; port_id ++) {
		if ((1<<port_id) & switch_get_uni_logical_portmask()) {
			util_fdprintf(fd, "\t %d \t", p->poe_report[port_id].priority);
		}
	}
	util_fdprintf(fd, "\n");
	
	util_fdprintf(fd, "det_status ");
	for (port_id= 0; port_id < SWITCH_LOGICAL_PORT_TOTAL; port_id ++) {
		if ((1<<port_id) & switch_get_uni_logical_portmask()) {
			util_fdprintf(fd, "\t %d \t", p->poe_report[port_id].status);
		}
	}
	util_fdprintf(fd, "\n");
	
	util_fdprintf(fd, "over_current ");
	for (port_id= 0; port_id < SWITCH_LOGICAL_PORT_TOTAL; port_id ++) {
		if ((1<<port_id) & switch_get_uni_logical_portmask()) {
			util_fdprintf(fd, "\t %d \t", p->poe_report[port_id].over_current);
		}
	}
	util_fdprintf(fd, "\n");
	
	util_fdprintf(fd, "invd_cnt ");
	for (port_id= 0; port_id < SWITCH_LOGICAL_PORT_TOTAL; port_id ++) {
		if ((1<<port_id) & switch_get_uni_logical_portmask()) {
			util_fdprintf(fd, "\t %d \t", p->poe_report[port_id].invd_cnt);
		}
	}
	util_fdprintf(fd, "\n");
	
	util_fdprintf(fd, "pwrd_cnt ");
	for (port_id= 0; port_id < SWITCH_LOGICAL_PORT_TOTAL; port_id ++) {
		if ((1<<port_id) & switch_get_uni_logical_portmask()) {
			util_fdprintf(fd, "\t %d \t", p->poe_report[port_id].pwrd_cnt);
		}
	}
	util_fdprintf(fd, "\n");
	
	util_fdprintf(fd, "ovl_cnt ");
	for (port_id= 0; port_id < SWITCH_LOGICAL_PORT_TOTAL; port_id ++) {
		if ((1<<port_id) & switch_get_uni_logical_portmask()) {
			util_fdprintf(fd, "\t %d \t", p->poe_report[port_id].ovl_cnt);
		}
	}
	util_fdprintf(fd, "\n");
	
	util_fdprintf(fd, "udl_cnt ");
	for (port_id= 0; port_id < SWITCH_LOGICAL_PORT_TOTAL; port_id ++) {
		if ((1<<port_id) & switch_get_uni_logical_portmask()) {
			util_fdprintf(fd, "\t %d \t", p->poe_report[port_id].udl_cnt);
		}
	}
	util_fdprintf(fd, "\n");
	
	util_fdprintf(fd, "sc_cnt \t");
	for (port_id= 0; port_id < SWITCH_LOGICAL_PORT_TOTAL; port_id ++) {
		if ((1<<port_id) & switch_get_uni_logical_portmask()) {
			util_fdprintf(fd, "\t %d \t", p->poe_report[port_id].sc_cnt);
		}
	}
	util_fdprintf(fd, "\n");
	
	util_fdprintf(fd, "mps_abs_cnt ");
	for (port_id= 0; port_id < SWITCH_LOGICAL_PORT_TOTAL; port_id ++) {
		if ((1<<port_id) & switch_get_uni_logical_portmask()) {
			util_fdprintf(fd, "\t %d \t", p->poe_report[port_id].mps_abs_cnt);
		}
	}
	util_fdprintf(fd, "\n");
	
	return 0;
}
