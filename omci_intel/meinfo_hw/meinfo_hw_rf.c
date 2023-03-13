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
 * File    : meinfo_hw_rf.c
 *
 ******************************************************************/

#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <math.h>

#include "meinfo.h"
#include "meinfo_hw.h"
#include "util.h"
#include "util_run.h"
#include "hwresource.h"
#include "meinfo_hw_rf.h"
#include "meinfo_hw_anig.h"

// rf result from rf report binary ///////////////////////////////////////////
#define RF_REPORT_CMD_GEN	"/etc/init.d/rf.sh report_cmd"
#define RF_START_CMD_GEN	"/etc/init.d/rf.sh start"
#define RF_STOP_CMD_GEN		"/etc/init.d/rf.sh stop"
#define RF_OPT_START_CMD_GEN	"/etc/init.d/rf.sh opt_start"
#define RF_OPT_STOP_CMD_GEN	"/etc/init.d/rf.sh opt_stop"
#define RF_AGC_START_CMD_GEN	"/etc/init.d/rf.sh agc_start"
#define RF_AGC_STOP_CMD_GEN	"/etc/init.d/rf.sh agc_stop"
#define RF_AGC_OFFSET_CMD_GEN	"/etc/init.d/rf.sh agc_offset"

int
meinfo_hw_rf_set_admin_state(int enable)
{
	if(enable)
		util_run_by_thread(RF_START_CMD_GEN, 0);
	else
		util_run_by_thread(RF_STOP_CMD_GEN, 0);
	return 0;
}

int
meinfo_hw_rf_set_opt_state(int enable)
{
	if(enable)
		util_run_by_thread(RF_OPT_START_CMD_GEN, 0);
	else
		util_run_by_thread(RF_OPT_STOP_CMD_GEN, 0);
	return 0;
}

int
meinfo_hw_rf_set_agc_state(int enable)
{
	if(enable)
		util_run_by_thread(RF_AGC_START_CMD_GEN, 0);
	else
		util_run_by_thread(RF_AGC_STOP_CMD_GEN, 0);
	return 0;
}

int
meinfo_hw_rf_set_agc_offset(char value)
{
	char cmd[64];
	memset(cmd, 0, sizeof(cmd));
	sprintf(cmd, "%s %d", RF_AGC_OFFSET_CMD_GEN, value);
	util_run_by_thread(cmd, 0);
	return 0;
}

static int
meinfo_hw_rf_get_result_from_rf_report(struct rf_result_t *rf_result)
{
	static char report_cmd[128]={0};
	char *output=NULL;
	int ret;
	
	static struct rf_result_t rf_result_prev={0};
	static int rf_result_prev_time = 0;
	int now = time(0);

	// as calling /etc/init.d/rf.sh is slow
	// if request is within same second, reply with previous result directly
	if (now == rf_result_prev_time) {
		*rf_result = rf_result_prev;
		return 0;
	}	

	memset((void*)rf_result, 0, sizeof(struct rf_result_t));	

	if (strlen(report_cmd)==0) {
		char *s;
		if (util_readcmd(RF_REPORT_CMD_GEN, &output) <0 || output==NULL) {
			if (output)
				free_safe(output);
			dbprintf(LOG_WARNING, "cmd %s error\n", RF_REPORT_CMD_GEN);
			return -1;
		}
		s=util_trim(output);
		strncpy(report_cmd, s, 128);
		free_safe(output);
	}

	if (util_readcmd(report_cmd, &output) <0 || output==NULL) {
		if (output)
			free_safe(output);
		dbprintf(LOG_WARNING, "cmd %s error\n", report_cmd);
		return -2;
	}

	ret = sscanf(output, "%lf %lf %lf %d %d %d",
		&rf_result->rf_tx,
		&rf_result->opt_rx,
		&rf_result->agc,
		&rf_result->rf_en,
		&rf_result->opt_en,
		&rf_result->agc_en);
	
	if (ret<6) {
		dbprintf(LOG_WARNING, "output %s error\n", output);
		free_safe(output);
		return -3;
	}

	// cache result for repeated access with same second
	rf_result_prev = *rf_result;
	rf_result_prev_time = time(0);	

	free_safe(output);
	return 0;
}

// get function wrapper //////////////////////////////////////////////////

int
meinfo_hw_rf_get_threshold_result(struct rf_result_t *r)
{
	int ret=0;

	// init threshold/result to 0
	memset(r, 0, sizeof(struct rf_result_t));

	util_create_lockfile("/var/run/rf.lock", 30);
	
	if (meinfo_hw_rf_get_result_from_rf_report(r)<0)
		ret = -1;
	
	util_release_lockfile("/var/run/rf.lock");

	if (ret <0)
		return ret;

	return 0;			
}

// print function /////////////////////////////////////////////////////////////////////////////

int
meinfo_hw_rf_print_threshold_result(int fd, struct rf_result_t *r)
{
	util_fdprintf(fd, "\n");
	util_fdprintf(fd, "rf_en \t%s\n", (r->rf_en==1) ? "enable" : "disable");
	util_fdprintf(fd, "opt_en \t%s\n", (r->opt_en==1) ? "enable" : "disable");
	util_fdprintf(fd, "agc_en \t%s\n", (r->agc_en==1) ? "enable" : "disable");
	if (r->rf_tx == 0xffff)
		util_fdprintf(fd, "rf_tx \tnan dBmV\n");
	else
		util_fdprintf(fd, "rf_tx \t%.3f dBmV\n", r->rf_tx);
	if (r->opt_rx == 0xffff)
		util_fdprintf(fd, "opt_rx \tnan dBm\n");
	else
		util_fdprintf(fd, "opt_rx \t%.3f dBm\n", r->opt_rx);
	if (r->agc == 0xffff)
		util_fdprintf(fd, "agc \tnan (0.1 dB)\n");
	else
		util_fdprintf(fd, "agc \t%.3f (0.1 dB)\n", r->agc);
	util_fdprintf(fd, "\n");
		
	return 0;
}
