/******************************************************************
 *
 * Copyright (C) 2014 5V Technologies Ltd.
 * All Rights Reserved.
 *
 * This program is the proprietary software of 5V Technologies Ltd
 * and/or its licensors, and only be used, duplicated,
 * modified or distributed under the authorized license from 5VT.
 *
 * ANY USE OF THE SOFTWARE OTHER THAN AS AUTHORIZED UNDER
 * THIS LICENSE OR COPYRIGHT LAW IS PROHIBITED.
 *
 * Purpose : 5VT TRAF MANAGER
 * Module  : src
 * File    : main.c
 *
 ******************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>

#include <termios.h>

#include "util.h"
#include "fwk_msgq.h"
#include "fwk_timer.h"
#include "fwk_sem.h"
#include "xmlomci.h"
#include "omcimain.h"
#include "cli.h"

#include "intel_px126_util.h"


#ifdef START_CLI_IN_MAIN
static int mainloop=1;
#else
static struct fwk_sem_t sys_sem;
#endif

char intf_bind_g[32]={0};	// global to be used by omci protocol stack

////////////////////////////////////////////////////////////////////

static void
sig_handler(int signo)
{
	dbprintf(LOG_ERR, "signal %d detected\n", signo);
#ifdef START_CLI_IN_MAIN
	mainloop=0;
#else
	fwk_sem_unlock(&sys_sem);
#endif
}

void
cli_init_terminal(void)
{
	struct termios curr_tio;

	/* get the terminal settings for stdin */
	tcgetattr(STDIN_FILENO, &curr_tio);
	/* enable local echo, canonical mode (buffered i/o) */
	curr_tio.c_lflag |= (ECHO|ICANON);
	/* set the new settings immediately */
	tcsetattr(STDIN_FILENO, TCSANOW, &curr_tio);
}

// prototyping for platform dependent functions
#if 0	// edwin, need bemodified
#if defined(CONFIG_TARGET_BRCM63XX)
int switch_hw_brcm63xx_init_vlan(void);
#elif !defined(X86)
int rtk_core_init(void);
int switch_hw_trex_init_vlan(void);
#endif
#endif

static void
init_platform_lib_loop(void)
{
#ifndef X86
#if 0	// edwin, need be modified
	int i;
	for (i=0; i<10; i++) {
		int ret = rtk_core_init();
		if (ret == 0) // init sdk user lib, needs to be done by every user application
			break;
		printf("chip core init failed %d, ret=%d\n", i, ret);
		sleep(1);
	}
	if (i >= 10)
		exit(1);
#endif
#endif
}

int
main(int argc, char **argv)
{
	static struct sigaction sigact_l;
	char *env_file = NULL;
	int i;

	for (i=1; i< argc; i++) {
//		printf("i=%d, argc=%d, argv[i]=%s, argv[i+1]=%s\n", i, argc, argv[i], argv[i+1]);
		if (strcmp(argv[i], "-f")==0) { /* determine environment file path*/
			if(i + 1 < argc) {
				fprintf(stdout, "using env file %s\n", argv[i + 1]);
				env_file = argv[i + 1];
				i++;
			}
		}
		else if (strcmp(argv[i], "-t")==0) { /* test read of omcienv.xml */
			if(i + 1 < argc) {
				env_file = argv[i + 1];
				i++;
				if (xmlomci_env_init(env_file) == 0) {
					printf("%s test ok\n", env_file);
					return 0;
				} else {
					printf("%s test error\n", env_file);
					return 1;
				}
			}
		}
		else if (strcmp(argv[i], "-i")==0) {
			if(i + 1 < argc) {
				fprintf(stdout, "bind on interface %s\n", argv[i + 1]);
				strncpy(intf_bind_g, argv[i + 1], 32);
				i++;
			}
		}
		else if (strcmp(argv[i], "-I")==0) {
			if(i + 1 < argc) {
				intel_omci_save_init_param("soc_mac", argv[i + 1]);
				i++;
			}
		}
		else if (strcmp(argv[i], "-M")==0) {
			if(i + 1 < argc) {
				intel_omci_save_init_param("pon_mac", argv[i + 1]);
				i++;
			}
		}
		else if (strcmp(argv[i], "-h")==0) {
			printf("Options:\n");
			printf("  -h - this help\n");
			printf("  -v - show version\n");
			printf("  -t [envfile] - test read envfile\n");
			printf("  -i [ifname] - bind on specific interface\n");
			printf("  -f [envfile] - use specific envfile\n");
			printf("  --init_vlan - switch hw vlan init\n");
			return 0;
		}
		else if (strcmp(argv[i], "--init_vlan")==0) {
			init_platform_lib_loop();
#if 0	// edwin, need modified
#if !defined(X86)
			switch_hw_trex_init_vlan();
#endif
#endif
			return 0;
		}
		else if (strcmp(argv[i], "-v")==0) {
			printf("%s\n", omci_cli_version_str());
			return 0;
		}
		else {
			// Wrong paramter specified
			fprintf(stderr, "%s: wrong parameter \"%s\"\n", argv[0], argv[i]);
			return -1;
		}
	}

	init_platform_lib_loop();

	/* set signal handler for SIGINT/SIGTERM */
	sigact_l.sa_handler = sig_handler;
	sigaction(SIGINT, &sigact_l, NULL);	// ctrl-c
	sigaction(SIGTERM, &sigact_l, NULL);	// sig 15

#ifdef START_CLI_IN_MAIN
	{
		char input_line[MAX_CLI_LINE];
		unsigned int cmdindex=1;

		cli_init_terminal();
		/* init is done by user through cli cmd */
		for (mainloop=1; mainloop; input_line[0]=0) {
			printf("omci:%04d> ", cmdindex);
			if (fgets(input_line, MAX_CLI_LINE, stdin) == NULL) {
				printf("exit\n");
				break;
			}
			if (omci_cli_input(1, input_line)==0)
				cmdindex++;
			if (cmdindex >= 10000)
				cmdindex = 0;
		}
	}
#else
	{
		int ret;
		sys_sem.value = 0;
		fwk_sem_create(&sys_sem);

		// set process name to pthread mgmt before calling 1st pthread_creat().
		// so it would be inherited and appeared in the forked pthread manager
		util_set_thread_name("pthread-mgr");

		// init env & start threads: cli, kv, refresh
		// so cli client can connect to cli server thread
		if ( omci_init_start(env_file) != 0 ) exit(1);

		// set process name for this main
		util_set_thread_name("omci-main");
		while (1) {
			omci_env_g.taskloopcount[ENV_TASK_NO_MAIN]++;
			ret = fwk_sem_lock(&sys_sem);
			// when sem lock aborts due to EINTR, we continue the lock again
			// eg: when program is executed under gdb, 
			//     the program may receive unexpected signal and EINTE might happen
			if (ret == 0 || (ret <0 && errno != EINTR))
				break;
		}

		// omci_stop_shutdown();	 // this is done by sig handler in omcimain.c
	}
#endif
	exit(0);
}
