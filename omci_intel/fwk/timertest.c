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
 * Module  : msgq
 * File    : timertest.c
 *
 ******************************************************************/

#include "fwk_thread.h"
#include "fwk_msgq.h"
#include "fwk_timer.h"
#include <stdio.h>
#include <sys/time.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <unistd.h>

#define TIMER_EVENT_A 1
#define TIMER_EVENT_B 2
#define TIMER_EVENT_C 3

#define MAX_ARGV        8
#define CLI_BUFFER      128
#define CLI_PROMPT      "<CMD>"
#define CMD_MAX_NUM     32

#define E_EXIT  2
#define E_HELP  1
#define E_OK    0
#define E_PARM  -1
#define E_LAST  -2
#define E_OPER  -3
#define		CMD_MAIN

// CmdEntry
typedef struct {
	char *cmd;		// Input command string
	int (*func) (int argc, char *argv[]);
	char *msg;		// Help message
} CmdEntry;

#ifdef  CMD_MAIN
char *err_code[] = {
	"PARM",
};
#else
extern char *err_code[];
#endif

int get_argc(const char *string);
char **get_argv(const char *string);
int cmd_proc(int argc, char **argv);
void *cli_loop(void *ptr);
int cmd_help(int argc, char *argv[]);
void cmd_init();
int cmd_add(CmdEntry * ent);

CmdEntry main_cmds[CMD_MAX_NUM];

static int enter_level = 0;
static char *argvs[MAX_ARGV];

struct fwk_thread_t thread_0;
struct fwk_thread_t thread_1;
struct fwk_thread_t thread_2;
struct fwk_thread_t thread_3;

int q_id_1;
int q_id_2;
int q_id_3;

int msgq_set_1;
int msgq_set_2;
int msgq_set_3;

int thread_1_var = 0;
int thread_2_var = 0;
int thread_3_var = 0;

int
get_cmd(char *buf)
{
	char cc;
	char *bp;
	bp = buf;
	*bp = 0;
	do {
		cc = getchar();
		putchar(cc);
		if ((cc == 0x0a) | (cc == 0x0d))
			break;
		if ((cc == 0x08) & (bp > buf)) {
			bp--;	// backspace = remove 1 char
		} else if (isprint(cc)) {
			*bp = cc;
			bp++;
		}
	}
	while ((bp - buf) < (250));
	*bp = 0;
	return (int) (bp - buf);
}

int
get_argc(const char *string)
{
	int argc;
	char *p;

	argc = 0;
	p = (char *) string;
	while (*p) {
		if (*p != ' ' && *p) {
			argc++;
			while (*p != ' ' && *p)
				p++;
			continue;
		}
		p++;
	}
	if (argc >= MAX_ARGV)
		argc = MAX_ARGV - 1;
	return argc;
}

char **
get_argv(const char *string)
{
	char *p;
	int n;

	n = 0;
	memset(argvs, 0, MAX_ARGV * sizeof (char *));
	p = (char *) string;
	while (*p) {
		argvs[n] = p;
		while (*p != ' ' && *p)
			p++;
		*p++ = '\0';
		while (*p == ' ' && *p)
			p++;
		n++;
		if (n == MAX_ARGV)
			break;
	}
	return (char **) &argvs;
}

int
cmd_proc(int argc, char **argv)
{
	int rc;
	int i, j;
	int match_len;
	CmdEntry *cp, *mp;

	if (argc < 1)
		return -1;

	if (!strcasecmp(argv[0], "exit")) {
		return E_EXIT;
	}

	cp = &main_cmds[0];
	for (mp = NULL, match_len = i = 0; i < CMD_MAX_NUM; i++, cp++) {
		if (!cp->cmd)
			continue;
		if (!strcasecmp(argv[0], cp->cmd))	//complete match
		{
			mp = cp;
			goto match;
		}
		for (j = 0; j < 8; j++)
			if (argv[0][j] != cp->cmd[j])
				break;
		if (j > match_len)	// partial match
		{
			match_len = j;
			mp = cp;
		}
	}

	if (mp) {
	      match:
		rc = mp->func(argc - 1, argv + 1);
		switch (rc) {
		case E_OK:
			printf("OK");
			break;

		case E_HELP:
			printf("%s", mp->msg);
			break;

		default:
			if (rc < 0) {
				printf("ERR");
				if (rc < 0 && rc >= E_LAST)
					printf(":%s", err_code[-rc - 1]);
			}
		}
		printf("\n\r");
		return rc;
	} else
		printf("Unknown CMD!\n\r");
	return -1;
}

int
timerdump(int argc, char *argv[])
{
	fwk_timer_dump();

	return 0;
}

static CmdEntry cmds[] = {
	{"timerdump", timerdump, "dump timer"},
	{0, 0, 0}
};

void *
cli_loop(void *ptr)
{
	char buffer[CLI_BUFFER + 1];
	int rc;
	int argc;
	char **argv;
	enter_level++;
	CmdEntry *p;

	cmd_init();

	for (p = &cmds[0]; p->cmd; p++) {
		rc = cmd_add(p);
	}

	while (1) {
		printf(CLI_PROMPT);
		memset(buffer, 0, CLI_BUFFER);

		get_cmd(buffer);

		if (buffer[0] == 0 || buffer[0] == '\n' || buffer[0] == '\r')
			continue;
		//printf( "\n" );
		argc = get_argc((const char *) buffer);
		argv = get_argv((const char *) buffer);
		//printf("cmdline=%s  argc=%d cmd=%s\n",buffer,argc,argv[0]);
		rc = cmd_proc(argc, argv);
		if (rc == E_EXIT) {
			if (enter_level > 0)
				enter_level--;
			return 0;
		}
	}
}

int
cmd_help(int argc, char *argv[])
{
	int i;

	if (argc == 0) {
		printf("Commands list:\n\r");
		for (i = 0; i < CMD_MAX_NUM; i++) {
			if (!main_cmds[i].cmd)
				continue;
			printf("%-10s", main_cmds[i].cmd);
			if ((i % 5) == 4)
				printf("\n");
		}
		printf("\n");
		return 0;
	} else {
		for (i = 0; i < CMD_MAX_NUM; i++) {
			if (!main_cmds[i].cmd)
				continue;
			if (!strcasecmp(main_cmds[i].cmd, argv[0])) {
				printf("%s \n", main_cmds[i].msg);
				return 0;
			}
		}
	}
	return E_PARM;
}

/*
---------------------------------------------------------------------------
; add a command handler
---------------------------------------------------------------------------
*/
int
cmd_add(CmdEntry * ent)
{
	int i;

	for (i = 0; i < CMD_MAX_NUM; i++) {
		/*
		   if ( main_cmds[i].cmd==0 || !strcmp(main_cmds[i].cmd, ent->cmd) )
		   {
		   memcpy(&main_cmds[i] , ent, sizeof(CmdEntry));
		   //printf("cmd%d: %s added\n", i, ent->cmd);
		   return 0;
		   }
		 */
		if (main_cmds[i].cmd == 0 || !strcmp(main_cmds[i].cmd, ent->cmd)) {
			main_cmds[i].cmd = strdup(ent->cmd);
			main_cmds[i].func = ent->func;
			main_cmds[i].msg = strdup(ent->msg);
			return 0;
		}
	}
	return -1;
}

void
cmd_init()
{
	memset(&main_cmds[0], 0x00, sizeof (main_cmds));

	main_cmds[0].cmd = strdup("?");
	main_cmds[0].func = cmd_help;
	main_cmds[0].msg = strdup("? cmd");

	main_cmds[1].cmd = strdup("help");
	main_cmds[1].func = cmd_help;
	main_cmds[1].msg = strdup("H<elp> cmd");
}

void *
thread_func_1(void *ptr)
{
	struct list_head *list_node;
	struct fwk_timer_msg_t *timer_msg;
	int num;
	struct timeval time, old_time;
	unsigned int timeout, e_time, r_time;

	gettimeofday(&time, NULL);

	srand(time.tv_usec);

	while (thread_1_var > 0) {
		timeout = rand() % 1000 * 10;

		fwk_timer_create(q_id_1, TIMER_EVENT_A, timeout, NULL);

		gettimeofday(&old_time, NULL);

		printf("1 create timer: timeout(%d), time(%u.%u)\n", timeout,
		       (unsigned int) old_time.tv_sec, (unsigned int) old_time.tv_usec);

		num = fwk_msgq_select(msgq_set_1, FWK_MSGQ_WAIT_INFINITE);

		if (num > 0) {
			fwk_msgq_recv(q_id_1, &list_node);
			timer_msg = list_entry(list_node, struct fwk_timer_msg_t, node);

			gettimeofday(&time, NULL);
			//printf("1 timer event: %d, time(%u.%u)\n", timer_msg->event, (unsigned int)time.tv_sec, (unsigned int)time.tv_usec);
			e_time = (time.tv_sec - old_time.tv_sec) * 1000 + (time.tv_usec - old_time.tv_usec) / 1000;
			if ((r_time = abs(e_time - timeout)) < 100) {
				printf
				    ("1 timer event: %d, +++++,abs(%d), old_time(%u.%u), time(%u.%u)\n",
				     timer_msg->event, r_time, (unsigned int) old_time.tv_sec,
				     (unsigned int) old_time.tv_usec, (unsigned int) time.tv_sec,
				     (unsigned int) time.tv_usec);
			} else {
				printf
				    ("1 timer event: %d, -----,abs(%d), old_time(%u.%u), time(%u.%u)\n",
				     timer_msg->event, r_time, (unsigned int) old_time.tv_sec,
				     (unsigned int) old_time.tv_usec, (unsigned int) time.tv_sec,
				     (unsigned int) time.tv_usec);
			}

			free(timer_msg);
		}
	}

	return 0;
}

void *
thread_func_2(void *ptr)
{
	struct list_head *list_node;
	struct fwk_timer_msg_t *timer_msg;
	int num;
	struct timeval time, old_time;
	unsigned int timeout, e_time, r_time;

	gettimeofday(&time, NULL);

	srand(time.tv_usec);

	while (thread_2_var > 0) {
		timeout = rand() % 10000 * 10;

		fwk_timer_create(q_id_2, TIMER_EVENT_A, timeout, NULL);

		gettimeofday(&old_time, NULL);

		printf("2 create timer: timeout(%d), time(%u.%u)\n", timeout,
		       (unsigned int) old_time.tv_sec, (unsigned int) old_time.tv_usec);

		num = fwk_msgq_select(msgq_set_2, FWK_MSGQ_WAIT_INFINITE);

		if (num > 0) {
			fwk_msgq_recv(q_id_2, &list_node);
			timer_msg = list_entry(list_node, struct fwk_timer_msg_t, node);

			gettimeofday(&time, NULL);
			//printf("2 timer event: %d, time(%u.%u)\n", timer_msg->event, (unsigned int)time.tv_sec, (unsigned int)time.tv_usec);
			e_time = (time.tv_sec - old_time.tv_sec) * 1000 + (time.tv_usec - old_time.tv_usec) / 1000;
			if ((r_time = abs(e_time - timeout)) < 100) {
				printf
				    ("2 timer event: %d, +++++,abs(%d), old_time(%u.%u), time(%u.%u)\n",
				     timer_msg->event, r_time, (unsigned int) old_time.tv_sec,
				     (unsigned int) old_time.tv_usec, (unsigned int) time.tv_sec,
				     (unsigned int) time.tv_usec);
			} else {
				printf
				    ("2 timer event: %d, -----,abs(%d), old_time(%u.%u), time(%u.%u)\n",
				     timer_msg->event, r_time, (unsigned int) old_time.tv_sec,
				     (unsigned int) old_time.tv_usec, (unsigned int) time.tv_sec,
				     (unsigned int) time.tv_usec);
			}

			free(timer_msg);
		}
	}

	return 0;
}

void *
thread_func_3(void *ptr)
{
	struct list_head *list_node;
	struct fwk_timer_msg_t *timer_msg;
	int num;
	struct timeval time, old_time;
	unsigned int timeout, e_time, r_time;

	gettimeofday(&time, NULL);

	srand(time.tv_usec);

	while (thread_3_var > 0) {
		timeout = rand() % 10000 * 10;

		fwk_timer_create(q_id_3, TIMER_EVENT_A, timeout, NULL);

		gettimeofday(&old_time, NULL);

		printf("3 create timer: timeout(%d), time(%u.%u)\n", timeout,
		       (unsigned int) old_time.tv_sec, (unsigned int) old_time.tv_usec);

		num = fwk_msgq_select(msgq_set_3, FWK_MSGQ_WAIT_INFINITE);

		if (num > 0) {
			fwk_msgq_recv(q_id_3, &list_node);
			timer_msg = list_entry(list_node, struct fwk_timer_msg_t, node);

			gettimeofday(&time, NULL);
			//printf("3 timer event: %d, time(%u.%u)\n", timer_msg->event, (unsigned int)time.tv_sec, (unsigned int)time.tv_usec);
			e_time = (time.tv_sec - old_time.tv_sec) * 1000 + (time.tv_usec - old_time.tv_usec) / 1000;
			if ((r_time = abs(e_time - timeout)) < 100) {
				printf
				    ("3 timer event: %d, +++++,abs(%d), old_time(%u.%u), time(%u.%u)\n",
				     timer_msg->event, r_time, (unsigned int) old_time.tv_sec,
				     (unsigned int) old_time.tv_usec, (unsigned int) time.tv_sec,
				     (unsigned int) time.tv_usec);
			} else {
				printf
				    ("3 timer event: %d, -----,abs(%d), old_time(%u.%u), time(%u.%u)\n",
				     timer_msg->event, r_time, (unsigned int) old_time.tv_sec,
				     (unsigned int) old_time.tv_usec, (unsigned int) time.tv_sec,
				     (unsigned int) time.tv_usec);
			}

			free(timer_msg);
		}
	}

	return 0;
}

int
main(void)
{
	fwk_msgq_module_init();

	fwk_timer_init();

	fwk_timer_start();

	q_id_1 = fwk_msgq_alloc(FWK_MSGQ_ALLOC_ID_UNASSIGN, 10, "Q_1");
	q_id_2 = fwk_msgq_alloc(FWK_MSGQ_ALLOC_ID_UNASSIGN, 10, "Q_2");
	q_id_3 = fwk_msgq_alloc(FWK_MSGQ_ALLOC_ID_UNASSIGN, 10, "Q_3");

	msgq_set_1 = fwk_msgq_set_alloc();
	msgq_set_2 = fwk_msgq_set_alloc();
	msgq_set_3 = fwk_msgq_set_alloc();

	fwk_msgq_set_zero(msgq_set_1);
	fwk_msgq_set_zero(msgq_set_2);
	fwk_msgq_set_zero(msgq_set_3);

	fwk_msgq_set_set(msgq_set_1, q_id_1);
	fwk_msgq_set_set(msgq_set_2, q_id_2);
	fwk_msgq_set_set(msgq_set_3, q_id_3);

	thread_0.policy = FWK_THREAD_SP_NORMAL;
	thread_0.priority = 10;
	thread_0.stack_size = 128*1024;
	thread_0.join = 1;
	sprintf(thread_0.name, "test thread 0");

	thread_1.policy = FWK_THREAD_SP_NORMAL;
	thread_1.priority = 10;
	thread_1.stack_size = 128*1024;
	thread_1.join = 1;
	sprintf(thread_1.name, "test thread 1");

	thread_2.policy = FWK_THREAD_SP_NORMAL;
	thread_2.priority = 10;
	thread_2.stack_size = 128*1024;
	thread_2.join = 1;
	sprintf(thread_2.name, "test thread 2");

	thread_3.policy = FWK_THREAD_SP_NORMAL;
	thread_3.priority = 10;
	thread_3.stack_size = 128*1024;
	thread_3.join = 1;
	sprintf(thread_3.name, "test thread 3");

	thread_1_var = 1;
	thread_2_var = 1;
	thread_3_var = 1;

	fwk_thread_create(&thread_0, cli_loop, NULL);
	fwk_thread_create(&thread_1, thread_func_1, NULL);
	fwk_thread_create(&thread_2, thread_func_2, NULL);
	fwk_thread_create(&thread_3, thread_func_3, NULL);

	fwk_thread_join(&thread_0);

	thread_1_var = 0;
	thread_2_var = 0;
	thread_3_var = 0;

	fwk_timer_stop();
	fwk_timer_shutdown();

	fwk_thread_join(&thread_1);
	fwk_thread_join(&thread_2);
	fwk_thread_join(&thread_3);

	return 0;
}
