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
 * Module  : src
 * File    : omcimain.c
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
#include <pthread.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "meinfo.h"
#include "meinfo_cb.h"
#include "list.h"
#include "fwk_msgq.h"
#include "fwk_timer.h"
#include "fwk_sem.h"
#include "fwk_thread.h"
#include "util.h"
#include "util_run.h"
#include "env.h"
#include "xmlomci.h"
#include "notify_chain.h"
#include "me_createdel_table.h"
#include "notify_chain_cb.h"

#include "clitask.h"
#include "coretask.h"
#include "recvtask.h"
#include "alarmtask.h"
#include "pmtask.h"
#include "avctask.h"
#include "tasks.h"
#include "restore.h"

#include "cli.h"
#include "er_group.h"
#include "me_related.h"
#include "hwresource.h"
#include "batchtab.h"
#include "batchtab_cb.h"
#include "switch.h"
#include "gpon_sw.h"
#include "cpuport.h"
#include "vacl.h"
#include "omciutil_stp.h"
#include "omciutil_misc.h"

#ifdef OMCI_BM_ENABLE
#include "omci_bm.h"
#endif

#ifndef X86
#include "meinfo_hw.h"
#include "meinfo_hw_anig.h"
#include "er_group_hw.h"
#include "er_group_hw_util.h"
#include "voip_hw.h"
#endif

#include "proprietary_fiberhome.h"
#include "proprietary_zte.h"
#include "proprietary_alu.h"
#include "proprietary_huawei.h"
#include "proprietary_calix.h"
#include "proprietary_dasan.h"
#include "proprietary_tellion.h"
#include "proprietary_ericsson.h"


#if defined CONFIG_SDK_PRX126 || defined CONFIG_SDK_PRX120 || defined CONFIG_SDK_PRX321
#include "intel_px126_adapter.h"
#endif


#ifndef	X86
//#if 0
#define CPUL_INSTANCE_NUM 0
#define CPUL_PID_FILE "/var/run/cpul_%d.pid"

unsigned int orig_pause_mask_g;
static int
omci_pause_tasks()
{
	// FIXME, KEVIN
	// why pasue tasks do nothing when ENV_OLT_PROPRIETARY_ZTE is set?
	//if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_ZTE) != 0)
		return 0;

	//save original pause mask
	orig_pause_mask_g = omci_env_g.task_pause_mask;
	omci_env_g.task_pause_mask = 0xFFFFFFFF;

	return 0;
}

static int
omci_cpul_and_resume_tasks()
{
	char buf[128], buf1[16];
	int i, n = 0;
	struct stat state;

	// FIXME, KEVIN
	// why pasue tasks do nothing when ENV_OLT_PROPRIETARY_ZTE is set?
	//if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_ZTE) != 0)
		return 0;

	sprintf(buf, "/usr/bin/cpul %d %d", CPUL_INSTANCE_NUM, omci_env_g.cpul_total_usage);

	//check limiting tasks are ready
	for (i = 0; i < ENV_SIZE_TASK; i++)
	{
		if (omci_env_g.cpul_task_no[i] != -1)
		{
			while(1)
			{
				if (omci_env_g.taskid[(int)omci_env_g.cpul_task_no[i]] == 0)
				{
					usleep(10000);
				} else {
					break;
				}
			}
		}
	}

	//start cpul
	for (i = 0; i < ENV_SIZE_TASK; i++)
	{
		if (omci_env_g.cpul_task_no[i] != -1)
		{
			sprintf(buf1, " %d", omci_env_g.taskid[(int)omci_env_g.cpul_task_no[i]]);
			strcat(buf, buf1);
			n++;
		}
	}

	if (n > 0)
	{
		sprintf(buf1, " &");
		strcat(buf, buf1);
		util_run_by_system(buf);

		sprintf(buf, CPUL_PID_FILE, CPUL_INSTANCE_NUM);
		while(1)
		{
			if (stat(buf, &state) < 0)
			{
				usleep(10000);
			} else {
				break;
			}
		}
	}

	//restore pause mask
	omci_env_g.task_pause_mask = orig_pause_mask_g;

	return 0;
}

static int
omci_cpul_stop()
{
	char buf[128], buf1[16];
	FILE *fp;

	// FIXME, KEVIN
	// why pasue tasks do nothing when ENV_OLT_PROPRIETARY_ZTE is set?
	//if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_ZTE) != 0)
		return 0;

	sprintf(buf, CPUL_PID_FILE, CPUL_INSTANCE_NUM);
	if ((fp = fopen(buf, "r")) == NULL)
	{
		dbprintf(LOG_ERR, "Can not open cpul pid file=%s\n", buf);
		return -1;
	} else {
		if (fgets(buf1, sizeof(buf1), fp) == NULL)
		{
			dbprintf(LOG_ERR, "Can not get cpul pid\n");
			return -1;
		}
		fclose(fp);
		sprintf(buf, "kill -15 %d", atoi(buf1));
		util_run_by_system(buf);
		return 0;
	}
}
#endif

int
omci_get_olt_vendor(void)
{
	struct me_t *me_131;

	//get olt-g to check vendor
	if ((me_131 = meinfo_me_get_by_instance_num(131, 0)) == NULL ||
		me_131->attr[1].value.ptr == NULL)
	{
		dbprintf(LOG_ERR, "get 131 me error\n");
		return -1;
	}

	if (strncmp(me_131->attr[1].value.ptr, "ALCL", 4) == 0)
		return ENV_OLT_WORKAROUND_ALU;
	else if (strncmp(me_131->attr[1].value.ptr, "HWTC", 4) == 0)
		return ENV_OLT_WORKAROUND_HUAWEI;
	else if (strncmp(me_131->attr[1].value.ptr, "ZTEG", 4) == 0)
		return ENV_OLT_WORKAROUND_ZTE;
	else if (strncmp(me_131->attr[1].value.ptr, "DSNW", 4) == 0)
		return ENV_OLT_WORKAROUND_DASAN;
	else if (strncmp(me_131->attr[1].value.ptr, "CALX", 4) == 0)
		return ENV_OLT_WORKAROUND_CALIX;
	else if (strncmp(me_131->attr[1].value.ptr, "ENTR", 4) == 0)
		return ENV_OLT_WORKAROUND_ERICSSON;
	else if (strncmp(me_131->attr[1].value.ptr, "ADTR", 4) == 0)
		return ENV_OLT_WORKAROUND_ADTRAN;
	else if (strncmp(me_131->attr[1].value.ptr, "FHTT", 4) == 0)
		return ENV_OLT_WORKAROUND_FIBERHOME;
	else
		return ENV_OLT_WORKAROUND_NONE;
}

unsigned char
omci_get_omcc_version(void)
{
	struct me_t *me_257;

	//get onu2-g to check omcc version
	if ((me_257 = meinfo_me_get_by_instance_num(257, 0)) == NULL)
	{
		dbprintf(LOG_ERR, "get 257 me error\n");
		return -1;
	}

	return me_257->attr[2].value.data;
}

void
omci_proprietary_init(void)
{
	if(strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_FIBERHOME)==0)
		proprietary_fiberhome();
	else if(strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_ZTE)==0)
		proprietary_zte();
	else if(strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_ALU)==0)
		proprietary_alu();
	else if(strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_HUAWEI)==0)
		proprietary_huawei();
	else if(strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX)==0)
		proprietary_calix();
	else if(strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_DASAN)==0)
		proprietary_dasan();
	else if(strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_TELLION)==0)
		proprietary_tellion();
	else if(strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_ERICSSON)==0)
		proprietary_ericsson();
}

int omci_stop_shutdown(void);
static struct sigaction sigact_orig, sigact_curr;
static void 
omci_sig_handler(int sig)
{
	if(fwk_thread_gettid() != omci_env_g.taskid[0])
		return;
#ifndef	X86
	// only omcimain need to do stp disable
	// core port stp_disable, extsw port stp_off
	omciutil_stp_set_before_shutdown();
#endif
	omci_stop_shutdown();
	if (sigact_orig.sa_handler)
		sigact_orig.sa_handler(sig);	// cascade to orig signal handler
}

///////////////////////////////////////////////////////////////////////

static int omci_init(char *env_file_path);
static int omci_start(void);
static int omci_stop(void);
static int omci_shutdown(void);

static int
omci_init(char *env_file_path)
{
	static int omci_once_is_inited=0;
	char filename[512], len;

	util_logprintf("/var/run/omcimain.init.tmp", "pid=%d, start=%s\n", getpid(), util_uptime_str());

	// ensure platform & include file are same enidan
	util_check_endian();
	// ensure platform & program are same SNAT/HNAT
	util_check_hwnat();

	// trick to eliminate zmobie
	signal(SIGCHLD, SIG_IGN);
	// trick to maskoff alarm
	signal(SIGALRM, SIG_IGN);
	// trick to maskoff pipe error
	signal(SIGPIPE, SIG_IGN);
	// trick to maskoff SIGUSR1
	signal(SIGUSR1, SIG_IGN);

	sigact_curr.sa_handler = omci_sig_handler;
	sigaction(SIGINT, &sigact_curr, &sigact_orig);
	sigaction(SIGTERM, &sigact_curr, &sigact_orig);

	// block signal SIGUSR1 for this thread
	{
		//sigset_t sigmask;
		//sigemptyset(&sigmask);
		//sigaddset(&sigmask, SIGUSR1);
		//pthread_sigmask(SIG_BLOCK, &sigmask, NULL);
	}

	if (!omci_once_is_inited) {
		struct timeval now;

		// init random number generator
		gettimeofday(&now, NULL);
		srand(now.tv_usec);
		srandom(now.tv_usec);
	
		// init data structures
		meinfo_preinit();
		er_me_group_preinit();
		er_attr_group_hw_preinit();
		notify_chain_preinit();
		me_related_preinit();
		hwresource_preinit();
		batchtab_preinit();

		// load env.xml, spec.xml, config.xml, er_group.xml
		if (xmlomci_init(env_file_path, "xmlomci", "xmlomci", "xmlomci")<0) {
			dbprintf(LOG_ERR, "omci xml init error\n");
			return -1;
		}

		// init callbacks
		meinfo_cb_init();
		me_related_init();
		hwresource_cb_init();
#ifndef	X86
		meinfo_hw_init();
		er_group_hw_init();
#endif

#ifdef OMCI_BM_ENABLE
		omci_bm_init();
#endif		
		omci_proprietary_init();
		util_omcipath_filename(filename, 512, "me_aliasid.conf");
		meinfo_util_load_alias_file(filename);

		cli_access_clear();	// clear per class statistics access_read_mask, access_write_mask

		omci_once_is_inited=1;
	}

	// save main task pid
	omci_env_g.taskname[ENV_TASK_NO_MAIN] = "MAIN";
	omci_env_g.taskid[ENV_TASK_NO_MAIN] = fwk_thread_gettid();
	omci_env_g.pthreadid[ENV_TASK_NO_MAIN] = pthread_self();

	// register empty function all hw hooks, for cli debugging
	switch_hw_register(NULL);
	gpon_hw_register(NULL);
	cpuport_hw_register(NULL);

#if defined CONFIG_SDK_PRX126 || defined CONFIG_SDK_PRX120 || defined CONFIG_SDK_PRX321
	intel_adapter_omci_init();
#endif

#ifndef	X86
	extern struct switch_hw_t switch_hw_fvt_g;
	extern struct gpon_hw_t gpon_hw_fvt_g;

	switch_hw_register(&switch_hw_fvt_g);
	gpon_hw_register(&gpon_hw_fvt_g);
	#ifndef X86_CROSS
	extern struct cpuport_hw_t cpuport_hw_fvt_g;
	// only register cpuport hw if it is on real non x86 platform
	cpuport_hw_register(&cpuport_hw_fvt_g);
	#endif

	if (voip_hw_init() < 0) {
		dbprintf(LOG_ERR, "omci hw init voip error\n");
		// this might happen on platform not supporting voip
	}
	if (switch_hw_g.init() <0) {
		dbprintf(LOG_ERR, "omci hw init switch error\n");
		return -1;
	}
	if (gpon_hw_g.init() <0) {
		dbprintf(LOG_ERR, "omci hw init gpon error\n");
		return -1;
	}

	if (switch_hw_g.classf_init() < 0) {
		dbprintf(LOG_ERR, "omci hw init classf error\n");
		return -1;
	}

	vacl_init();

	// BOSA auto detection
	if (omci_env_g.anig_type==ENV_ANIG_TYPE_BOSA_AUTO)
		omci_env_g.anig_type=meinfo_hw_anig_detect_type();

	switch_init_wan_logical_portid();
#endif
	batchtab_cb_init();
	cli_history_init();
	cli_session_list_init();

	// override config.xml value loaded in meinfo config 
	if (omci_env_g.tcont_map_enable) {
		meinfo_config_tcont_ts_pq_remap();
		if (omci_env_g.tcont_map_enable>=2)
			meinfo_config_tcont_ts_pq_dump();
	}
	if (omci_env_g.mib_uni_adminlock_default==ENV_MIB_UNI_ADMINLOCK_DEFAULT_DISABLE ||
	    omci_env_g.mib_uni_adminlock_default==ENV_MIB_UNI_ADMINLOCK_DEFAULT_ENABLE) {
	    	meinfo_config_pptp_uni(omci_env_g.mib_uni_adminlock_default);
	}
	meinfo_config_card_holder();
	meinfo_config_circuit_pack();
#if 0 // Remove VEIP related workaround
	meinfo_config_iphost();
	meinfo_config_veip();
#endif
	meinfo_config_load_custom_mib();

	// load me_chain.conf
	util_omcipath_filename(filename, 512, "me_chain/me_chain.conf");
	if (me_createdel_table_load(filename) < 0) {
		cli_session_list_shutdown();
		cli_history_shutdown();
		dbprintf(LOG_ERR, "load %s err\n", filename);
		return -1;
	}
	// optionally load me_chain_99.conf
	len=util_omcipath_filename(filename, 512, "me_chain/me_chain_99.conf");
	if (len>0 && me_createdel_table_load(filename) < 0) {
		dbprintf(LOG_ERR, "load %s err\n", filename);
	}

	// init tasks and msgq used by tasks
	if (fwk_msgq_module_init() < 0) {
		me_createdel_table_free();
		cli_session_list_shutdown();
		cli_history_shutdown();
		dbprintf(LOG_ERR, "omci msgq init error\n");
		return -1;
	}
	if (fwk_timer_init() < 0) {
		fwk_msgq_module_shutdown();
		me_createdel_table_free();
		cli_session_list_shutdown();
		cli_history_shutdown();
		dbprintf(LOG_ERR, "omci timer init error\n");
		return -1;
	}
	if (tasks_init()<0) {
		fwk_timer_shutdown();
		fwk_msgq_module_shutdown();
		me_createdel_table_free();
		cli_session_list_shutdown();
		cli_history_shutdown();
		return -1;
	}

	util_logprintf("/var/run/omcimain.init.tmp", "pid=%d, end=%s\n", getpid(), util_uptime_str());
	// mark file for external program to know omcimain init is done
	rename("/var/run/omcimain.init.tmp", "/var/run/omcimain.init.done");

	return 0;
}

static int
omci_start(void)
{
#ifndef	X86
	//pause all tasks
	omci_pause_tasks();
#endif

	util_thread_sched(SCHED_RR, 10);
	// start tasks
	if (fwk_timer_start() < 0) {
		omci_stop();
		omci_shutdown();
		dbprintf(LOG_ERR, "omci timer start error\n");
		return -1;
	}
	if (tasks_start() < 0) {
		omci_stop();
		omci_shutdown();
		return -1;
	}

#ifndef	X86
	//start cpu limitation , then resume all tasks
	omci_cpul_and_resume_tasks();
#endif

	// clear msg_count
	memset(&omcimsg_count_g, 0, sizeof(omcimsg_count_g));

	// register me_createdel functions to notify chain handler
	notify_chain_register(NOTIFY_CHAIN_EVENT_ME_CREATE, me_createdel_create_handler, NULL);
	notify_chain_register(NOTIFY_CHAIN_EVENT_ME_DELETE, me_createdel_delete_handler, NULL);
	notify_chain_register(NOTIFY_CHAIN_EVENT_SHUTDOWN, me_createdel_shutdown_handler, NULL);
	notify_chain_register(NOTIFY_CHAIN_EVENT_BOOT, me_createdel_boot_handler, NULL);
	// todo: It's maybe too early,need to discuess
	notify_chain_register(NOTIFY_CHAIN_EVENT_BOOT, alarmtask_boot_notify, NULL);

	// trigger me creation through boot event
	notify_chain_notify(NOTIFY_CHAIN_EVENT_BOOT, 256, 0, 0);

	// set all port stp state to STP_DISABLE or STP_OFF
#ifndef X86
	// spanning tree 
	omciutil_stp_set_before_omci();
	// loop detection enable/disable 
	if(switch_loop_detect_set(omci_env_g.rldp_enable, omci_env_g.rldp_def_portmask) != 0)
		dbprintf(LOG_ERR, "switch_loop_detect_set error\n");
#endif

	// issue restore.log to recover mib to the state before last boot
	if (omci_env_g.restore_log_enable)
		restore_load_omcclog(omci_env_g.restore_log_file);

#ifndef X86
	// active onu, so it begins to process ploam message
	// this is to defer onu entering O5 state
	// so onu omci would be ready before olt deliveries omci message
	gpon_hw_g.onu_activate(GPON_ONU_STATE_O1);
#endif

	// assign allocid for tcont me and prepare faked gpon hw routine
	if (omci_env_g.sim_tcont_enable)
		env_set_sim_tcont_enable(1);

	return 0;
}

// stop thread function
static int
omci_stop(void)
{
	// set all port stp state to STP_OFF
#ifndef	X86
	omciutil_stp_set_after_omci();
#endif
	notify_chain_notify(NOTIFY_CHAIN_EVENT_SHUTDOWN	, 256, 0, 0);

	// unregister me_createdel functions to notify chain handler
	notify_chain_unregister(NOTIFY_CHAIN_EVENT_ME_CREATE, me_createdel_create_handler, NULL);
	notify_chain_unregister(NOTIFY_CHAIN_EVENT_ME_DELETE, me_createdel_delete_handler, NULL);
	notify_chain_unregister(NOTIFY_CHAIN_EVENT_SHUTDOWN, me_createdel_shutdown_handler, NULL);
	notify_chain_unregister(NOTIFY_CHAIN_EVENT_BOOT, me_createdel_boot_handler, NULL);
	notify_chain_unregister(NOTIFY_CHAIN_EVENT_BOOT, alarmtask_boot_notify, NULL);

	tasks_stop();
	fwk_timer_stop();

	return 0;
}

// release data stru
static int
omci_shutdown(void)
{
	tasks_shutdown();
	fwk_timer_shutdown();

#ifndef	X86
	//kill cpu limitation task
	omci_cpul_stop();
#endif

	batchtab_cb_finish();
	/* clear acl rule have to be after batchtab_cb_finish()
	   rg_vacl_del_all() will delete all rg acl rules
	*/

	vacl_del_all();
	fwk_msgq_module_shutdown();
	me_createdel_table_free();
	cli_history_shutdown();
#ifndef	X86
	switch_hw_g.shutdown();
	gpon_hw_g.shutdown();
	voip_hw_destory();
	vacl_shutdown();
#endif
	sigaction(SIGINT, &sigact_orig, NULL);
	sigaction(SIGTERM, &sigact_orig, NULL);

	unlink("/var/run/omcimain.init.done");

	return 0;
}

///////////////////////////////////////////////////////////////////////

static int is_omci_ready=0;

int
omci_is_ready(void)
{
	return is_omci_ready;
}

int
omci_init_start(char *env_file_path)
{
	if (!is_omci_ready) {
		if (omci_init(env_file_path) < 0)
			return -1;
		if (omci_start() < 0)
			return -1;
		is_omci_ready=1;
	}

	// load cmd from start_cmd_file
	if (strlen(omci_env_g.start_cmd_file) >0) {
		static char line[MAX_CLI_LINE];
		FILE *fp = fopen(omci_env_g.start_cmd_file, "r");
		if (fp) {
			usleep(5000);	// wait clitask to init clitask_nullfd_g
			dbprintf(LOG_ERR, "load %s\n", omci_env_g.start_cmd_file);
			while (fgets(line, MAX_CLI_LINE, fp) != NULL) {
				if (omci_env_g.debug_level >= LOG_WARNING) {
					util_dbprintf(omci_env_g.debug_level, LOG_WARNING, 0, "cmd> %s", line);
					usleep(1000); 
					omci_cli_input(clitask_consolefd_g, line);
				} else {
					omci_cli_input(clitask_nullfd_g, line);
				}
			}
			fclose(fp);
		}
	}
	return 0;
}

int
omci_stop_shutdown(void)
{
	if (is_omci_ready) {
		is_omci_ready=0;
		omci_stop();
		omci_shutdown();
	}
	return 0;
}
