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
 * Module  : oltsim
 * File    : oltsim.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>

#include <pthread.h>
#include <sched.h>

#include <signal.h>
#include <assert.h>
#include <time.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include <termios.h>

#include "util.h"
#include "omcistr.h"
#include "logplay.h"
#include "omcilog.h"

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

int 
cli_input(char *input_line)
{
	static char lastcmd[256]={0};
	char cmd[256], buff[256];
	int argc;
	char *argv[16];

	if ( strcmp(input_line, "\n")==0 || strcmp(input_line, "\r")==0) {
		return -1;
	} else if (strcmp(input_line, "")==0) {
		if (strlen(lastcmd)>0) {
			printf("%s", lastcmd);
			strcpy(cmd, lastcmd);
		} else {
			printf("\n");
			return -1;
		}
	} else {
		strcpy(cmd, input_line);
	}	
	lastcmd[0]=0;

	strncpy(buff, cmd, 256); buff[255]=0;
	argc = util_str2array(buff, " ", argv, 16);	
	if (argc==0)
		return -1;
		
	if (strcmp(argv[0], "load")==0 && argc==2) {
		if (omcilog_load(argv[1])<0) {
			printf("load %s failed, %s\n", argv[1], strerror(errno));
			return -1;
		}
		return 0;
			
	} else if (strcmp(argv[0], "clearall")==0 && argc==1) {
		omcilog_clearall();
		return 0;
			
	} else if (strncmp(argv[0], "list", 4)==0 && argc<=3) {
		unsigned short start=0;
		unsigned short end=OMCILOG_TOTAL_MAX-1;
		char *p;	
		if (argc>=2)
			start=(unsigned short) strtol(argv[1], &p, 0);
		if (argc>=3)
			end=(unsigned short) strtol(argv[2], &p, 0);		
		if (strcmp(argv[0], "list")==0) {	
			return omcilog_list(start, end);
		} else if (strcmp(argv[0], "listw")==0) {	
			return omcilog_listw(start, end);
		} else if (strcmp(argv[0], "listerr")==0) {	
			return omcilog_listerr(start, end);
		} else if (strcmp(argv[0], "listz")==0) {	
			return omcilog_listz(start, end);
		}

	} else if (strcmp(argv[0], "run")==0 && argc<=3) {
		unsigned short start=0;
		unsigned short end=OMCILOG_TOTAL_MAX-1;
		char *p;
		if (argc>=2)
			start=(unsigned short) strtol(argv[1], &p, 0);
		if (argc>=3)
			end=(unsigned short) strtol(argv[2], &p, 0);
		logplay_send_start(start, end);
		return 0;

	} else if (strcmp(argv[0], "n")==0 && argc<=2) {
		unsigned short start=logplay_get_i();
		unsigned short total=1;
		unsigned short end;
		char *p;
		if (argc>=2)
			total=(unsigned short) strtol(argv[1], &p, 0);
		end=start+total-1;	
		logplay_send_start(start, end);
		strncpy(lastcmd, cmd, 256);
		return 0;

	} else if (strcmp(argv[0], "w")==0 && argc==1) {
		unsigned short start=logplay_get_i();
		unsigned short end=logplay_get_i_before_next_write(start);
		logplay_send_start(start, end);
		strncpy(lastcmd, cmd, 256);
		return 0;

	} else if (strcmp(argv[0], "z")==0 && argc==1) {
		unsigned short start=logplay_get_i();
		unsigned short end=logplay_get_i_before_next_mibreset(start);
		logplay_send_start(start, end);
		strncpy(lastcmd, cmd, 256);
		return 0;

	} else if (strcmp(argv[0], "goto")==0 && argc==2) {
		logplay_set_i(atoi(argv[1]));
		return 0;

	} else if (strcmp(argv[0], "stop")==0 && argc==1) {
		logplay_send_stop();
		return 0;

	} else if (strcmp(argv[0], "state")==0 && argc==1) {
		logplay_show_state();
		strncpy(lastcmd, cmd, 256);
		return 0;

	} else if (strcmp(argv[0], "onu")==0 && argc==4) {
		char *onuipstr=argv[1];
		unsigned char ponid=atoi(argv[2]);
		unsigned char onuid=atoi(argv[3]);
		logplay_set_onuinfo(onuipstr, ponid, onuid);
		return 0;

	} else if (strcmp(argv[0], "timeout")==0 && argc==2) {
		logplay_set_timeout(atoi(argv[1]));
		return 0;

	} else if (strcmp(argv[0], "skip_mibreset")==0 && argc==2) {
		logplay_set_skip_mibreset(atoi(argv[1]));
		return 0;

	} else if (strcmp(argv[0], "debug_level")==0 && argc==2) {
		util_set_debug_level(atoi(argv[1]));
		return 0;

	} else if (argv[0][0]=='!') {
		if (strcmp(argv[0], "!")==0 && argc==1) {
			system("/bin/bash");
		} else {
			system(cmd+1);
		}
		return 0;

	} else if (strcmp(argv[0], "quit")==0 && argc==1) {
		exit(0);

	} else if (strcmp(argv[0], "help")==0 || strcmp(argv[0], "?")==0) {
		printf("Version 1.00 (build on %s, %s)\n",
				__TIME__, __DATE__);
		printf("\n"
			"load [log file]\n"
			"clearall\n"
			"list|listw|listz|listerr [start] [end]\n"
			"run [start] [end]\n"
			"n [num] (run next num pkt)\n"
			"w (run till before next write)\n"
			"z (run till before next mibreset)\n"
			"goto [index]\n"
			"stop\n"
			"state\n"
			"onu [ip] [ponid] [onuid]\n"
			"timeout [seconds]\n"
			"debug_level [n]\n"
			"![shell command]\n"
			"quit\n"
			"help\n"
			"\nps: Ctrl-D can be used to repeat last n, z, state cmd\n\n");
		return 0;
	}
	
	printf("Invalid command.\n");
	return -1;
}			

///////////////////////////////////////////////////////////////////

static int mainloop=1;

static void
sigint_handler(int signo)
{
	switch(signo) {
	case SIGINT:
		dbprintf(LOG_ERR, "sigint_handler: captured ctrl+c\n");
		mainloop=0;
		break;
	case SIGUSR1:
		dbprintf(LOG_ERR, "sigint_handler: captured usr1\n");
		break;
	}
}

int
main(int argc, char *argv[])
{
	static struct sigaction sigact_l;
	char input_line[1024];
	unsigned int cmdindex=1;
	unsigned short oltport=0;

	if (argc==2) {
		oltport=atoi(argv[1]);
	}
	if (!oltport) {
		fprintf(stderr, "%s [oltport]\n", argv[0]);
		exit(1);
	}

	/* set signal handler for SIGINT to catch ctrl-c */
	sigact_l.sa_handler = sigint_handler;
	sigaction(SIGINT, &sigact_l, NULL);
	sigaction(SIGUSR1, &sigact_l, NULL);

	cli_init_terminal();

	logplay_init(oltport);
	logplay_start();
	
	for (mainloop=1; mainloop; input_line[0]=0) {
		printf("oltsim:%d> ", cmdindex);	
		fgets(input_line, 1024, stdin);
		if (cli_input(input_line)==0)
			cmdindex++;
	}

	logplay_stop();
	logplay_shutdown();

	return 0;
}
