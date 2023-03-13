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
 * Module  : cli
 * File    : cli_misc.c
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
#include <sys/time.h>
#include <sys/ioctl.h>
#include <errno.h>

#include "util.h"
#include "util_run.h"
#include "pingtest.h"
#include "conv.h"
#include "cli.h"
#include "env.h"
#include "omcimsg.h"
#include "omcimain.h"
#include "meinfo.h"
#include "memdbg.h"
#include "switch.h"
#include "classf.h"
#include "fwk_msgq.h"
#include "clitask.h"
#include "wanif.h"
#include "coretask.h"
#include "crc.h"
#ifndef X86
#include "meinfo_hw_anig.h"
#include "meinfo_hw_rf.h"
#include "meinfo_hw_poe.h"
#endif
#if defined CONFIG_SDK_PRX126 || defined CONFIG_SDK_PRX120 || defined CONFIG_SDK_PRX321
#include "intel_px126_adapter.h" 
#endif
extern int recvtask_omcc_socket_fd_g;

static int
cli_script(int fd, char *script)
{
	FILE *fp;
	char buff[MAX_CLI_LINE];
	char *prompt;

	if ((fp = fopen(script, "r")) == NULL) {
		util_fdprintf(fd, "script %s open error: %s\n", script, strerror(errno)); 
		return CLI_ERROR_INTERNAL;
	}

	prompt = util_purename(script);
	while (fgets(buff, sizeof(buff), fp) != NULL) {
		if (buff[0] == '#')
			continue;
		util_fdprintf(fd, "%s> %s", prompt, buff); 
		omci_cli_input(fd, buff);
	}
	fclose(fp);

	return CLI_OK;
}

static int
cli_task_show(int fd)
{
	static char* policy_str[] = { "SCHED_OTHER", "SCHED_FIFO", "SCHED_RR", "SCHED_BATCH" };			
	unsigned long long loopcount[ENV_SIZE_TASK];
	unsigned long msec;
	int i;

	{
		static struct timeval prev_tv;
		struct timeval now;
		util_get_uptime(&now);
		msec = util_timeval_diff_msec(&now, &prev_tv);
		if (msec ==0) msec = 1;
		prev_tv = now;
	}

	for (i=0; i<ENV_SIZE_TASK; i++) {
		loopcount[i] = omci_env_g.taskloopcount[i];
		omci_env_g.taskloopcount[i] = 0;
	}
	
	for (i=0; i<ENV_SIZE_TASK; i++) {
		if (omci_env_g.taskname[i]!=NULL && omci_env_g.taskid[i]>0) {
			int policy;
			struct sched_param param;
			memset(&param, 0x0, sizeof(struct sched_param));
			pthread_getschedparam(omci_env_g.pthreadid[i], &policy, &param);

			unsigned int avg_loopcount = loopcount[i]*1000/msec;
			if (avg_loopcount) {
				util_fdprintf(fd, "%2d: %-16s, taskid=%5u%s, %-11s prio=%3d, t=%5lds ago, %s, loop=%llu (%llu/s)\n", 
					i, omci_env_g.taskname[i], omci_env_g.taskid[i], (omci_env_g.task_pause_mask &(1<<i))?"(pause)":"",
					policy_str[policy%4], param.sched_priority, 
					util_get_uptime_sec() - omci_env_g.taskts[i], omci_env_g.taskstate[i]?"busy":"idle", 
					loopcount[i], loopcount[i]*1000/msec);
			} else {
				util_fdprintf(fd, "%2d: %-16s, taskid=%5u%s, %-11s prio=%3d, t=%5lds ago, %s, loop=%llu\n", 
					i, omci_env_g.taskname[i], omci_env_g.taskid[i], (omci_env_g.task_pause_mask &(1<<i))?"(pause)":"",
					policy_str[policy%4], param.sched_priority, 
					util_get_uptime_sec() - omci_env_g.taskts[i], omci_env_g.taskstate[i]?"busy":"idle", 
					loopcount[i]);
			}
		}
	}
	return 0;
}

static int
cli_access_classid(int fd, int is_detail, int classid)
{
	struct meinfo_t *miptr = meinfo_util_miptr(classid); 
	int i, is_printed = 0;

	if (miptr != NULL)
	{
		for (i = 1; i <= miptr->attr_total; i++)
		{
			if (util_attr_mask_get_bit(miptr->access_create_mask, i) == 1 ||
			    util_attr_mask_get_bit(miptr->access_read_mask, i) == 1 ||
			    util_attr_mask_get_bit(miptr->access_write_mask, i) == 1) {
			    	if (!is_printed) {
			    		is_printed = 1;
			    		if (is_detail) {
						util_fdprintf(fd, "%d %s:\n", miptr->classid, miptr->name ? miptr->name:"?");
						util_fdprintf(fd, "\t%2d: %s (%s%s%s)\n", 
							i, miptr->attrinfo[i].name,
							util_attr_mask_get_bit(miptr->access_create_mask, i)?"C":"",
							util_attr_mask_get_bit(miptr->access_read_mask, i)?"R":"",
							util_attr_mask_get_bit(miptr->access_write_mask, i)?"W":"");
					} else {
						util_fdprintf(fd, "%d %s: %d", miptr->classid, miptr->name ? miptr->name:"?", i);
					}
				} else {
			    		if (is_detail) {
						util_fdprintf(fd, "\t%2d: %s (%s%s%s)\n", 
							i, miptr->attrinfo[i].name,
							util_attr_mask_get_bit(miptr->access_create_mask, i)?"C":"",
							util_attr_mask_get_bit(miptr->access_read_mask, i)?"R":"",
							util_attr_mask_get_bit(miptr->access_write_mask, i)?"W":"");
					} else {
						util_fdprintf(fd, ", %d", i);
					}
				}
			}
		}
		if (is_printed) {
			if (!is_detail)
				util_fdprintf(fd, "\n");
			return 1;
		}
	}
	return 0;
}
int
cli_access_clear(void)
{
	int i;
	struct meinfo_t *miptr;

	for (i = 0; i < MEINFO_INDEX_TOTAL; i++) {
		miptr = meinfo_util_miptr(meinfo_index2classid(i)); 
		if (miptr && miptr->config.is_inited && miptr->config.support != ME_SUPPORT_TYPE_NOT_SUPPORTED) {
			memset(miptr->access_create_mask, 0x00, sizeof(miptr->access_create_mask));
			memset(miptr->access_read_mask, 0x00, sizeof(miptr->access_read_mask));
			memset(miptr->access_write_mask, 0x00, sizeof(miptr->access_write_mask));		
		}
	}
	return CLI_OK;
}

static int
cli_access(int fd, int is_detail, int classid)
{
	int i;
	struct meinfo_t *miptr;

	if (classid != 0) {
		cli_access_classid(fd, is_detail, classid);
	} else {
		for (i = 0; i < MEINFO_INDEX_TOTAL; i++)
		{
			miptr = meinfo_util_miptr(meinfo_index2classid(i)); 
			if (miptr && miptr->config.is_inited && miptr->config.support != ME_SUPPORT_TYPE_NOT_SUPPORTED) {
				cli_access_classid(fd, is_detail, meinfo_index2classid(i));
			}
		}
	}
	return CLI_OK;
}

static int
cli_msgcount(int fd, int is_clear)
{
	int i;

	if(is_clear)
		memset(&omcimsg_count_g, 0, sizeof(omcimsg_count_g));
	for (i=0; i<32; i++) {
		if (omcimsg_type_attr[i].name)
			util_fdprintf(fd, "%8u 0x%02x(%02d) %s\n", 
				omcimsg_count_g[i], i, i, omcimsg_type_attr[i].name);
	}
	return CLI_OK;
}

static int 
cli_msgmask(int fd, unsigned int mask) 
{
	int i;
	for (i=0; i<32; i++) {
		if (omcimsg_type_attr[i].name)
			util_fdprintf(fd, "%c 0x%02x(%02d) %s\n", 
				(mask & (1<<i))?'+':'-', i, i, omcimsg_type_attr[i].name);
	}
	return CLI_OK;
}

static int 
cli_meid2inst(int fd, unsigned short classid, unsigned short meid) 
{
	struct meinfo_t *miptr = meinfo_util_miptr(classid); 
	int ret=meinfo_instance_meid_to_num(classid, meid);

	if (miptr==NULL) {
		util_fdprintf(fd, "classid %u invalid.\n", classid);
		return CLI_ERROR_RANGE;
	}
	if (ret<0) {
		util_fdprintf(fd, "meid %u not found.\n", meid);
		return CLI_ERROR_RANGE;
	}

	util_fdprintf(fd, "0x%x(%u)\n", ret, ret);
	return CLI_OK; 
}
#if defined CONFIG_SDK_PRX126 || defined CONFIG_SDK_PRX120 || defined CONFIG_SDK_PRX321

static int 
cli_misc_intel_dbglvl(int fd, unsigned short is_set, unsigned short dbgmodule,unsigned short dbglvl)
{
	unsigned short tmp_dbglvl;
	int ret = 0;

	tmp_dbglvl = dbglvl;

	ret = intel_adapter_config_dbglvl(is_set,dbgmodule,&tmp_dbglvl);
		
	util_fdprintf(fd, "%d\n",tmp_dbglvl);
	
	return 0;	
		
}

static int
cli_misc_intel_pa_mapper_dump(void)
{
	int ret = 0;

	ret = intel_adapter_pa_mapper_dump();
	return 0;
}

#endif
static int 
cli_inst2meid(int fd, unsigned short classid, unsigned short instance) 
{
	struct meinfo_t *miptr = meinfo_util_miptr(classid); 
	int ret=meinfo_instance_num_to_meid(classid, instance);

	if (miptr==NULL) {
		util_fdprintf(fd, "classid %u invalid.\n", classid);
		return CLI_ERROR_RANGE;
	}
	if (ret<0) {
		util_fdprintf(fd, "instance %u not found.\n", instance);
		return CLI_ERROR_RANGE;
	}

	util_fdprintf(fd, "0x%x(%u)\n", ret, ret);
	return CLI_OK; 
}

static int
cli_fields2ventry(int fd, char *str)
{
	int ret, i;
	unsigned char buf[16];
	struct attr_value_t attr;

	attr.ptr = buf;

	if ((ret=meinfo_conv_string_to_attr(171, 6, &attr, str)) <0) {
		util_fdprintf(fd, "classid=%u, attr_order=%u, err=%d\n", 171, 6, ret);
		return CLI_ERROR_INTERNAL;
	}

	util_fdprintf(fd, "vlan tagging entry is:\n");
	for (i = 0; i < 16; i++)
	{
		util_fdprintf(fd, "%.2x", buf[i]);
	}
	util_fdprintf(fd, "\n");

	return 0;
}

static int
cli_asc2hex(int fd, char *str)
{
	int i,len=strlen(str);

	for (i=0; i<len; i++) {
		util_fdprintf(fd,"%2c ", str[i]);
	}
	util_fdprintf(fd,"\n");

	for (i=0; i<len; i++) {
		util_fdprintf(fd,"%2X ", str[i]);
	}
	util_fdprintf(fd,"\n");

	return 0;
}

static int
cli_hex2asc(int fd, char *hexstr)
{
	int i, val, len=strlen(hexstr);
	char buff[3] = {0};

	for (i=0; i<len; i+=2) {
		util_fdprintf(fd,"%c%c ", hexstr[i], hexstr[i+1]);
	}
	util_fdprintf(fd,"\n");

	for (i=0; i<len; i+=2) {
		buff[0] = hexstr[i];
		buff[1] = hexstr[i+1];
		sscanf(buff, "%x", &val);
		util_fdprintf(fd,"%2c ", val);
	}
	util_fdprintf(fd,"\n");
	return 0;
}

static int
cli_sec2datetime(int fd, char *str)
{
	long seconds;
	struct tm *t;

	if (str==NULL) {
		seconds=time(0);
		t=(struct tm *)localtime(&seconds);
		util_fdprintf(fd, "%04d/%02d/%02d %02d:%02d:%02d (%ld), uptime %ss\n", 
			t->tm_year+1900, t->tm_mon+1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec, seconds, util_uptime_str());
	} else {
		seconds=util_atoll(str);
		t=(struct tm *)localtime(&seconds);
		util_fdprintf(fd, "%04d/%02d/%02d %02d:%02d:%02d\n", 
			t->tm_year+1900, t->tm_mon+1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);
	}
	return 0;
}

int
cli_anig(int fd, int anig_type)
{
#ifndef X86
	struct anig_threshold_t diag_threshold;
	struct anig_result_t diag_result;
	struct anig_raw_t diag_raw;

	if (meinfo_hw_anig_get_threshold_result(anig_type, &diag_threshold, &diag_result, &diag_raw)<0)
		return-1;	    
	meinfo_hw_anig_print_threshold_result(fd, anig_type, &diag_threshold, &diag_result, &diag_raw);
#else
	util_fdprintf(fd, "anig cmd not supported in X86 mode\n");
#endif
	return 0;
}

int
cli_rf(int fd)
{
#ifndef X86
	struct rf_result_t rf_result;

	if (meinfo_hw_rf_get_threshold_result(&rf_result)<0)
		return-1;	    
	meinfo_hw_rf_print_threshold_result(fd, &rf_result);
#else
	util_fdprintf(fd, "rf cmd not supported in X86 mode\n");
#endif
	return 0;
}

int
cli_poe(int fd)
{
#ifndef X86
	struct poe_result_t poe_result;

	if (meinfo_hw_poe_get_threshold_result(&poe_result)<0)
		return-1;	    
	meinfo_hw_poe_print_threshold_result(fd, &poe_result);
#else
	util_fdprintf(fd, "poe cmd not supported in X86 mode\n");
#endif
	return 0;
}

int
cli_ping(int fd, char *srcip_str, char *dstip_str, int count)
{
	int srcip = inet_addr(srcip_str);
	int dstip = inet_addr(dstip_str);
	
	int recv = pingtest(ntohl(dstip), ntohl(srcip), count, NULL, NULL, 0);
	util_fdprintf(fd, "send %d ping from %s to %s, recv %d replies\n", count, srcip_str, dstip_str, recv);
	
	return 0;
}

static void
cli_misc_long_help(int fd)
{
	util_fdprintf(fd, 
		"misc omci_init|init|omci_exit|task|wanif|reboot|version\n"
		"     script [filename]\n"
		"     msgcount [clear]\n"
		"     msgmask [mask]\n"
		"     asc2hex|hex2asc|sec2datetime|tm [string]\n"
		"     anig [anig_type]\n"
		"     console [on|off|client]\n"
		"misc session add|del|DEL [sessname]\n"
		"     meid2inst [classid] [meid]\n"
		"     inst2meid [classid] [instance]\n"
		"     fields2ventry [string]\n"
		"     alarm_seq [seq_num]\n"
		"     mib_reset|omcc_mr_show|sw_download_state\n"
		);
}

// helper function used by cli.c /////////////////////////////////////////////////////////////////////////////////////////////

void
cli_misc_help(int fd)
{
	 util_fdprintf(fd, "misc help|[subcmd...]\n");
}

int
cli_misc_before_cmdline(int fd, int argc, char *argv[])
{
	if (argc == 1 && strcmp(argv[0], "misc")==0) {
		cli_misc_long_help(fd);
		return CLI_OK;
	
	} else if (argc>=2 && strcmp(argv[0], "misc")==0) {
		if (strcmp(argv[1], "help")==0) {
			cli_misc_long_help(fd);
			return CLI_OK;
			
		} else if (strcmp(argv[1], "version")==0) {
			util_fdprintf(fd, "%s\n", omci_cli_version_str());
			return CLI_OK;

		} else if (strcmp(argv[1], "omci_init")==0 || strcmp(argv[1], "init")==0) {
			if (omci_init_start((argc==3)?argv[2]:NULL)<0)
				return CLI_ERROR_INTERNAL;
			return CLI_OK;

		} else if (strcmp(argv[1], "omci_exit")==0) {
			if (omci_stop_shutdown()<0)
				return CLI_ERROR_INTERNAL;
			exit(0);
			return CLI_OK;

		} else if (strcmp(argv[1], "task")==0) {
			return cli_task_show(fd);

		} else if (strcmp(argv[1], "msgq")==0) {
			fwk_msgq_dump(fd);
			return CLI_OK;

		} else if (strcmp(argv[1], "memdbg")==0 && omci_env_g.memdbg_enable) {
			memdbg_dump(fd);
			return CLI_OK;

		} else if (strcmp(argv[1], "reboot")==0 ) {
			if (omci_stop_shutdown()<0)
				return CLI_ERROR_INTERNAL;
			util_run_by_system("reboot");	//todo: make an exception of X86
			return CLI_OK;

		} else if (strcmp(argv[1], "access")==0) {
			if (argc==2) {
				return cli_access(fd, 0, 0);
			} else if (argc==3) {
				if (strcmp(argv[2], "clear") == 0) {
					return cli_access_clear();
				} else if (strcmp(argv[2], "detail") == 0) {
					return cli_access(fd, 1, 0);
				} else {
					unsigned short classid=util_atoll(argv[2]);
					return cli_access(fd, 1, classid);
				}
			} else if (argc==4 && strcmp(argv[2], "detail") == 0) {
				unsigned short classid=util_atoll(argv[3]);
				return cli_access(fd, 1, classid);
			} 
			return CLI_ERROR_SYNTAX;;
		
		} else if (strcmp(argv[1], "msgcount")==0) {
			int is_clear=0;
			if (argc==3 && strcmp (argv[2], "clear")==0)
				is_clear=1;
			return cli_msgcount(fd, is_clear);

		} else if (strcmp(argv[1], "msgmask")==0) {
			unsigned int mask;
			if (argc==3) {
				mask=util_atoll(argv[2]);
			} else {
				mask=omci_env_g.omcidump_msgmask;
				util_fdprintf(fd, "use env omcidump_msgmask %08x\n", mask);
			}
			return cli_msgmask(fd, mask);

		} else if (argc == 3 && strcmp(argv[1], "asc2hex")==0) {
			return cli_asc2hex(fd, argv[2]);

		} else if (argc == 3 && strcmp(argv[1], "hex2asc")==0) {
			return cli_hex2asc(fd, argv[2]);

		} else if (strcmp(argv[1], "sec2datetime")==0 || strcmp(argv[1], "tm")==0) {
			if (argc==3) {
				return cli_sec2datetime(fd, argv[2]);
			} else {
				return cli_sec2datetime(fd, NULL);
			}

		} else if (strcmp(argv[1], "anig")==0) {
			unsigned int anig_type=omci_env_g.anig_type;
			if (argc==3)
				anig_type=util_atoi(argv[2]);
			if (cli_anig(fd, anig_type)<0)
				return CLI_ERROR_SYNTAX;
			return CLI_OK;

		} else if (strcmp(argv[1], "rf")==0) {
			if (cli_rf(fd)<0)
				return CLI_ERROR_SYNTAX;
			return CLI_OK;

		} else if (strcmp(argv[1], "poe")==0) {
			if (cli_poe(fd)<0)
				return CLI_ERROR_SYNTAX;
			return CLI_OK;

		} else if (argc >= 4 && strcmp(argv[1], "ping")==0) {
			int count = 1;			
			if (argc == 5) {
				if ((count = util_atoi(argv[4])) <= 0)
					count = 1;
			}
			cli_ping(fd, argv[2], argv[3], count);
			return CLI_ERROR_SYNTAX;

		} else if (argc == 3 && strcmp(argv[1], "script")==0) {
			return cli_script(fd, argv[2]);

		} else if (strcmp(argv[1], "console")==0) {
			struct clitask_client_info_t *client_info = clitask_get_thread_private();
			if (argc==3 && strcmp(argv[2], "off")==0) {
				util_dbprintf_set_console_fd(clitask_nullfd_g);
			} else if (argc==3 && strcmp(argv[2], "on")==0) {
				util_dbprintf_set_console_fd(clitask_consolefd_g);
			} else if (argc==3 && strcmp(argv[2], "client")==0) {
				util_dbprintf_set_console_fd(client_info->fd);
			} else {
				int omci_console_fd = util_dbprintf_get_console_fd();	
				util_fdprintf(fd, "client_info: 0x%p, socket=%d, console ",
					client_info, client_info->fd);				
				if (omci_console_fd == clitask_nullfd_g)
					util_fdprintf(fd, "off");
				else if (omci_console_fd == clitask_consolefd_g)
					util_fdprintf(fd, "on");
				else if (omci_console_fd == client_info->fd)
					util_fdprintf(fd, "client");
				else
					util_fdprintf(fd, "other(fd %d)", omci_console_fd);
				util_fdprintf(fd, "\n");
			}
			return 0;
			
		} else if (strcmp(argv[1], "session")==0) {
			if (argc==4 && strcmp(argv[2], "add")==0) {
				int ret = cli_session_add(argv[3], getpid());
				if (ret <0) {
					util_fdprintf(fd, "add session %s err (already exist)\n", argv[3]);
					return CLI_EXIT;	// force the termination of cli servlet
				}
				
			} else if (argc==4 && strcmp(argv[2], "del")==0) {
				int ret = cli_session_del(argv[3], getpid());
				if (ret == -1) {
					util_fdprintf(fd, "del session %s err (not found)\n", argv[3]);
				} else if (ret == -2) {
					util_fdprintf(fd, "del session %s err (not owner)\n", argv[3]);
				}
				
			} else if (argc==4 && strcmp(argv[2], "DEL")==0) {	// force del, dont check pid
				int ret = cli_session_del(argv[3], 0);
				if (ret == -1) {
					util_fdprintf(fd, "DEL session %s err (not found)\n", argv[3]);
				}
				
			} else if (argc==3 && strcmp(argv[2], "list")==0) {
				util_fdprintf(fd, "mypid:%d\n", getpid());
				cli_session_list_print(fd);
			} else if (argc==2) {
				util_fdprintf(fd, "mypid:%d\n", getpid());
				cli_session_list_print(fd);
			}
			return 0;
			
		} else if (strcmp(argv[1], "check_ready")==0) {
			util_fdprintf(fd, "is_omci_ready=%d\n", omci_is_ready());
			return 0;
			
		} else if (strcmp(argv[1], "wanif")==0 ) {
			wanif_print_status(fd);
			return CLI_OK;
		}

	// below is for backward compatible
	} else if (argc == 1 && strcmp(argv[0], "task")==0) {	
		return cli_task_show(fd);
	} else if (argc >= 1 && (strcmp(argv[0], "omci_init")==0 || strcmp(argv[0], "init")==0)) {
		if (omci_init_start((argc==2)?argv[1]:NULL)<0)
			return CLI_ERROR_INTERNAL;
		return CLI_OK;
	} else if (argc == 1 && strcmp(argv[0], "omci_exit")==0) {
		if (omci_stop_shutdown()<0)
			return CLI_ERROR_INTERNAL;
		exit(0);
		return CLI_OK;
	}

	return CLI_ERROR_OTHER_CATEGORY;
}
/////////////////////////////////////////////////////////////////

void
cli_misc_after_help(int fd)
{
}

int
cli_misc_after_cmdline(int fd, int argc, char *argv[])
{
	unsigned short classid=0, instance=0, meid=0;

	if (argc>=2 && strcmp(argv[0], "misc")==0) {
		if (argc==4 && strcmp(argv[1], "meid2inst")==0) {
			classid=util_atoi(argv[2]);
			meid=util_atoi(argv[3]);
			return cli_meid2inst(fd, classid, meid);

		} else if (argc==4 && strcmp(argv[1], "inst2meid")==0) {
			classid=util_atoi(argv[2]);
			instance=util_atoi(argv[3]);
			return cli_inst2meid(fd, classid, instance);

		} else if (argc==3 && strcmp(argv[1], "fields2ventry")==0) {
			return cli_fields2ventry(fd, argv[2]);

		} else if (strcmp(argv[1], "sw_download_state")==0 || strcmp(argv[1], "swdl")==0) {
			return omcimsg_sw_download_dump_state(fd);

		} else if (argc==4 && strcmp(argv[1], "crc")==0) {
			char *filepath = argv[2];
			char *endian = argv[3];
			unsigned int crc = 0, size = 0;
			FILE *fp = fopen(filepath, "r");
			if(fp) {
				char *file = NULL;
				fseek(fp, 0, SEEK_END);
				size = ftell(fp);
				fseek(fp, 0, SEEK_SET);
				file = malloc_safe(size+1);
				if(file) {
					unsigned int accum = 0xFFFFFFFF;
					fread(file, size, 1, fp);
					if(strncmp(endian, "le", 2) == 0)
						crc = crc_le_update(accum, file, size);
					else
						crc = crc_be_update(accum, file, size);
					free_safe(file);
				}
				fclose(fp);
			}
			util_fdprintf(fd, "size=%u crc=0x%x (%u)\n", size, ~crc, ~crc);
			return CLI_OK;

		}
#if defined CONFIG_SDK_PRX126 || defined CONFIG_SDK_PRX120 || defined CONFIG_SDK_PRX321		
		else if (argc>=4  && strcmp(argv[1], "intel_dbglvl")==0 ) {
			if(strcmp(argv[2],"get")==0)
				cli_misc_intel_dbglvl(fd,0,util_atoi(argv[3]),0);
			else if(strcmp(argv[2],"set")==0)
				cli_misc_intel_dbglvl(fd,1,util_atoi(argv[3]),util_atoi(argv[4]));
			
			return CLI_OK;
		}
		else if (argc==2 && strcmp(argv[1], "intel_mapper_dump")==0 ) {
			cli_misc_intel_pa_mapper_dump();
			return CLI_OK;
		}
#endif		
		else if (strcmp(argv[1], "alarm_seq")==0) {
			if (argc==3) {
				omcimsg_alarm_set_sequence_num(util_atoi(argv[2]));
			} else {
				util_fdprintf(fd, "%d\n", omcimsg_alarm_get_sequence_num());
			}
			return CLI_OK;

		} 
		else if (strcmp(argv[1], "mib_reset")==0 ) {
			coretask_cmd(CORETASK_CMD_MIB_RESET);
			return CLI_OK;
		}
	}
	return CLI_ERROR_OTHER_CATEGORY;
}

