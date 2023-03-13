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
 * File    : cli.c
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

#include "util.h"
#include "util_run.h"
#include "conv.h"
#include "cli.h"
#include "env.h"
#include "omcimsg.h"
#include "omcimain.h"
#include "meinfo.h"

#define DIAG_PROMPT_LEN	6	// "diag> "
#define OMCI_PROMPT_LEN	21	// "GTHG00000532:001:O5> "
#define PREFIX_SPACE_STR	"               "	// append 15 byte at the position dependent output of diag

// wrapper to /usr/bin/diag
static int
cli_diag(int fd, char *input_line)
{
	static char diag_cmd_invalid_char[] = { '|', '<', '>', '(', ')', '{', '}', '`', ';', '&', '$', '!', 0};
	char buff[1024], filename[64];
	char *input_line2 = util_trim(input_line);
	FILE *fp;
	int i, line_num = 0;

	// do nothing for comment
	if (strlen(input_line2)== 0 || input_line2[0] == '#')
		return 0;

	// check if command supported by diag, this is to eliminate the overhead of executing /usr/bin/diag
	{
		static char *
		diag_cmd_list[]= {
			"?", "acl", "auto-fallback", "bandwidth", "classf", "cpu", 
			"debug", "dot1x", "epon", "field-selector", "flowctrl", 
			"gpon", "i2c", "igmp", "interrupt", "iol", 
			"l2-table", "l34", "led", "meter", "mib", "mirror",
			"oam", "pbo", "pon", "port", "qos",
			"register", "rg", "rldp", "rlpp", "rma",
			"sdk", "security", "storm-control", "stp", "svlan", "switch", 
			"time", "trap", "trunk", "vlan", "extoam", 
			NULL
		};
		for (i = 0; diag_cmd_list[i]!=NULL; i++) {
			if (strncmp(input_line2, diag_cmd_list[i], strlen(diag_cmd_list[i])) == 0)
				break;
		}
		if (diag_cmd_list[i] == NULL && strchr(input_line2, '?') == NULL)
			return CLI_ERROR_OTHER_CATEGORY;
	}

	// check if command contains invalid chars
	for (i=0; diag_cmd_invalid_char[i] != 0; i++) {
		if (strchr(input_line2, diag_cmd_invalid_char[i])!= NULL) {
			util_fdprintf(fd, "invalid char '%c' in command\n", diag_cmd_invalid_char[i]);
			return -1;
		}
	}
		
	// exec cmd
	snprintf(filename, 64, "/tmp/tmpfile.%d.%06ld", getpid(), random()%1000000);
	snprintf(buff, 1024, "/usr/bin/diag %s > %s 2>/dev/null", input_line2, filename);
	system(buff);	

	if ((fp = fopen(filename, "r")) == NULL) {
		util_fdprintf(fd, "exec diag err?\n");
		return -1;
	}

	while ( (fgets(buff, 1024, fp)) != NULL ) {
		line_num++;

		// skip dummy lines
		if (line_num == 1)	// skip 1st line (which is the cmd itself)
			continue;
		if (line_num == 2) {	// break at 2nd line if there is position dependent error prompt
			if (strstr(buff, "^Parse error") || strstr(buff, "^Incomplete command")) {
			 	util_fdprintf(fd, "%s%s", PREFIX_SPACE_STR, buff);
				break;
			}
		}
		if (strstr(buff, "> command:"))	// skip last line (which is prompt)
			continue;		

		// skip line with prompt, which is supposed to be the input_line2 ////////////////////
		if (strlen(buff)>=DIAG_PROMPT_LEN && strncmp(buff, "diag> ", DIAG_PROMPT_LEN) ==0)
			continue;
		// ignore incomplete warning & following lines
		if (strstr(buff, "^Incomplete command"))
			break;
		// show parse error with input_line2, skip following lines
		if (strstr(buff, "^Parse error") && strlen(buff)>=DIAG_PROMPT_LEN) {
			// print input line
			util_fdprintf(fd, "%s\n", input_line2);	
			// skip space chars
			util_fdprintf(fd, "%s", buff+DIAG_PROMPT_LEN);
			break;
		}

		// print output asis
		util_fdprintf(fd, "%s", buff);
	}
	fclose(fp);
		
	unlink(filename);
	return 0;
}

static int
cli_shell(int fd, char *input_line)
{
	char buff[MAX_CLI_LINE];
	char cmd[MAX_CLI_LINE];
	int status;
	
	if (util_omcipath_filename(cmd, MAX_CLI_LINE, "script/omci_cli_shell.sh") >0 ) {
		snprintf(buff, MAX_CLI_LINE, "sh %s %s >&%d", cmd, input_line, fd);
		status = system(buff);
		if (status == -1) {	// fork err
#if 0
			return CLI_ERROR_OTHER_CATEGORY;
#else
			// the status value is always -1? try exit status from /var/run/omci_cli_shell.ret
			char *data = NULL;
			util_rootdir_filename(cmd, MAX_CLI_LINE, "/var/run/omci_cli_shell.ret");
			if (util_readfile(cmd, &data) > 0) {
				status = util_atoi(data);
				if (data)
					free_safe(data);
			} else {
				return CLI_ERROR_OTHER_CATEGORY;
			}
#endif
		} else {
			status = WEXITSTATUS(status);		// get exit status
		}
		//util_fdprintf(fd, "exit=%d\n", status);

		if (status == 127 || 	// cmd not found
		    status == 126 || 	// cmd is not executable
		    status == 10) {	// cmd is not recognized by the script
			return CLI_ERROR_OTHER_CATEGORY;
		} else if (status !=0) {
			return CLI_ERROR_SYNTAX;
		}
		return 0;
	} 
	return CLI_ERROR_OTHER_CATEGORY;
}

int 
omci_cli_input(int fd, char *input_line)
{
	int argc, ret;
	char buff[MAX_CLI_LINE];
	char *argv[128];

	if (strcmp(input_line, "")==0) {
		util_fdprintf(fd, "\n");
		return -1;
	} else if ( strcmp(input_line, "\n")==0 || strcmp(input_line, "\r")==0) {
		return -1;
	}
	input_line=util_trim(input_line);

	strncpy(buff, input_line, MAX_CLI_LINE); buff[MAX_CLI_LINE-1]=0;
	util_str_remove_dupspace(buff);
	dbprintf(LOG_INFO, "buff=%s\n", buff);
	argc = conv_str2array(buff, " ", argv, 128);
	argv[argc]=0;

	if (argc==0)
		return CLI_ERROR_SYNTAX;

#if 0	// comment out for safety
	if (argv[0][0]=='!') {
		if (strcmp(argv[0], "!")==0 && argc==1) {
			system("/bin/bash");
		} else {
			// redirect cmd stdout to fd
			snprintf(buff, 1024, "%s >&%d", input_line+1, fd);			
			util_run_by_system(buff);
		}
		return CLI_OK;

	} 
#endif	
	
	if (strcmp(argv[0], "diag")==0) {
		// this client is assumed to call diag by system(), so server do nothing here
		return CLI_OK;
	} 
	
	if (strcmp(argv[0], "help")==0) {
		cli_misc_help(fd);
		util_fdprintf(fd, "\n");
		cli_spec_help(fd);
		cli_config_help(fd);
		cli_attr_help(fd);
		cli_me_help(fd);
		util_fdprintf(fd, "\n");			
		cli_er_help(fd);
		cli_hwresource_help(fd);
		cli_bridge_help(fd);
		cli_tag_help(fd);
		cli_classf_help(fd);
		cli_vacl_help(fd);
		cli_bat_help(fd);
		cli_gpon_help(fd);
		cli_switch_help(fd);
		util_fdprintf(fd, "\n");
		cli_arp_help(fd);
		cli_dhcp_help(fd);
		cli_cpuport_help(fd);
		cli_portinfo_help(fd);
		cli_igmp_help(fd);
		cli_cfm_help(fd);
		cli_lldp_help(fd);
		cli_stp_help(fd);
		cli_gfast_help(fd);
		util_fdprintf(fd, "\n");
		cli_env_help(fd);
		cli_history_help(fd);
		cli_nat_help(fd);
		cli_shell(fd, "help");
		return CLI_OK;

	} else if (strcmp(argv[0], "exit")==0 || strcmp(argv[0], "quit")==0) {
		return CLI_EXIT;
	}

	if ((ret=cli_misc_before_cmdline(fd, argc, argv))!=CLI_ERROR_OTHER_CATEGORY)
		return ret;

	// the above command is safe before init, below is not
	if (!omci_is_ready()) {
		int i;
		util_fdprintf(fd, "wait init ");
		for (i=0; i<20; i++) {		// wait 10 sec
			if (omci_is_ready())
				break;
			util_fdprintf(fd, ".");	// output to keep client connected
			usleep(500*1000);	// 0.5 sec
		}
		util_fdprintf(fd, "\n");
		if (!omci_is_ready()) { 
			util_fdprintf(fd, "Issue 'omci_init' first?\n");
			return CLI_OK;
		} 
	}
	if ((ret=cli_misc_after_cmdline(fd, argc, argv))!=CLI_ERROR_OTHER_CATEGORY)
		return ret;
	if ((ret=cli_spec_cmdline(fd, argc, argv))	!=CLI_ERROR_OTHER_CATEGORY)
		return ret;
	if ((ret=cli_config_cmdline(fd, argc, argv))	!=CLI_ERROR_OTHER_CATEGORY)
		return ret;
	if ((ret=cli_attr_cmdline(fd, argc, argv))	!=CLI_ERROR_OTHER_CATEGORY)
		return ret;
	if ((ret=cli_me_cmdline(fd, argc, argv))	!=CLI_ERROR_OTHER_CATEGORY)
		return ret;
	if ((ret=cli_er_cmdline(fd, argc, argv))	!=CLI_ERROR_OTHER_CATEGORY)
		return ret;
	if ((ret=cli_hwresource_cmdline(fd, argc, argv))!=CLI_ERROR_OTHER_CATEGORY)
		return ret;
	if ((ret=cli_bridge_cmdline(fd, argc, argv))	!=CLI_ERROR_OTHER_CATEGORY)
		return ret;
	if ((ret=cli_tcont_cmdline(fd, argc, argv))	!=CLI_ERROR_OTHER_CATEGORY)
		return ret;
	if ((ret=cli_pots_cmdline(fd, argc, argv))	!=CLI_ERROR_OTHER_CATEGORY)
		return ret;
	if ((ret=cli_pots_mkey_cmdline(fd, argc, argv))	!=CLI_ERROR_OTHER_CATEGORY)
		return ret;
	if ((ret=cli_unrelated_cmdline(fd, argc, argv))	!=CLI_ERROR_OTHER_CATEGORY)
		return ret;
	if ((ret=cli_tag_cmdline(fd, argc, argv))	!=CLI_ERROR_OTHER_CATEGORY)
		return ret;
	if ((ret=cli_classf_cmdline(fd, argc, argv))	!=CLI_ERROR_OTHER_CATEGORY)
		return ret;
	if ((ret=cli_bat_cmdline(fd, argc, argv))	!=CLI_ERROR_OTHER_CATEGORY)
		return ret;
	if ((ret=cli_gpon_cmdline(fd, argc, argv))	!=CLI_ERROR_OTHER_CATEGORY)
		return ret;
	if ((ret=cli_switch_cmdline(fd, argc, argv))	!=CLI_ERROR_OTHER_CATEGORY)
		return ret;
#if 0	// edwin, temporary remove for ifsw
	if ((ret=cli_cpuport_cmdline(fd, argc, argv))	!=CLI_ERROR_OTHER_CATEGORY)
		return ret;
#endif
	if ((ret=cli_portinfo_cmdline(fd, argc, argv))	!=CLI_ERROR_OTHER_CATEGORY)
		return ret;
	if ((ret=cli_igmp_cmdline(fd, argc, argv))	!=CLI_ERROR_OTHER_CATEGORY)
		return ret;
#if 0	// edwin, temporary remove for ifsw
	if ((ret=cli_igmp_cmdline(fd, argc, argv))	!=CLI_ERROR_OTHER_CATEGORY)
		return ret;
	if ((ret=cli_cfm_cmdline(fd, argc, argv))	!=CLI_ERROR_OTHER_CATEGORY)
		return ret;
	if ((ret=cli_lldp_cmdline(fd, argc, argv))	!=CLI_ERROR_OTHER_CATEGORY)
		return ret;
	if ((ret=cli_stp_cmdline(fd, argc, argv))	!=CLI_ERROR_OTHER_CATEGORY)
		return ret;
	if ((ret=cli_arp_cmdline(fd, argc, argv))	!=CLI_ERROR_OTHER_CATEGORY)
		return ret;
	if ((ret=cli_dhcp_cmdline(fd, argc, argv))	!=CLI_ERROR_OTHER_CATEGORY)
		return ret;
	if ((ret=cli_vacl_cmdline(fd, argc, argv))	!=CLI_ERROR_OTHER_CATEGORY)
		return ret;
	if ((ret=cli_extoam_cmdline(fd, argc, argv))	!=CLI_ERROR_OTHER_CATEGORY)
		return ret;
#endif
	if ((ret=cli_history_cmdline(fd, argc, argv))	!=CLI_ERROR_OTHER_CATEGORY)
		return ret;
	if ((ret=cli_env_cmdline(fd, argc, argv))	!=CLI_ERROR_OTHER_CATEGORY)
		return ret;
        if ((ret=cli_gfast_cmdline(fd, argc, argv))	!=CLI_ERROR_OTHER_CATEGORY)
		return ret;
	if ((ret=cli_nat_cmdline(fd, argc, argv))	!=CLI_ERROR_OTHER_CATEGORY)
		return ret;
	if ((ret=cli_shell(fd, input_line))		!=CLI_ERROR_OTHER_CATEGORY)
		return ret;
	if ((ret=cli_diag(fd, input_line))		!=CLI_ERROR_OTHER_CATEGORY)
		return ret;		

	util_fdprintf(fd, "Invalid command.\n");
	return CLI_ERROR_SYNTAX;
}
