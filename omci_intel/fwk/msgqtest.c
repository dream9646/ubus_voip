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
 * File    : msgqtest.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include "fwk_thread.h"
#include "fwk_sem.h"
#include "fwk_mutex.h"
#include "fwk_cond.h"
#include "util.h"
#include "fwk_msgq.h"
#include "fwk_rwlock.h"
#include "env.h"

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

struct fwk_thread_t thread0;
struct fwk_thread_t thread1;
struct fwk_thread_t thread2;
int thread1_var = 1;
int thread2_var = 1;
int thread3_var = 1;
int thread4_var = 1;
int thread5_var = 1;
int thread6_var = 1;

struct fwk_sem_t sem1;
struct fwk_mutex_t mutex1;
struct fwk_mutex_t cond1_mutex;
struct fwk_cond_t cond1;
struct fwk_rwlock_t rwlock1;
int cond1_variable1 = 1;
int cond1_variable2 = 1;

/*msgq test*/
struct fwk_thread_t thread3;
struct fwk_thread_t thread4;
struct fwk_thread_t thread5;
struct fwk_thread_t thread6;
int q_id_3_0;
int q_id_4_0;
int q_id_4_1;
int q_id_5_0;
int q_id_5_1;
int q_id_5_2;
int q_id_6_0;
int q_id_6_1;
int q_id_6_2;
int q_id_6_3;
int msgq_set_3;
int msgq_set_4;
int msgq_set_5;
int msgq_set_6;

struct node_3_t {
	int num;
	char name[32];
	struct list_head list;
};

struct node_4_0_t {
	char name[32];
	int num;
	struct list_head list;
};

struct node_4_1_t {
	char name[32];
	int num;
	char c;
	char phone[10];
	struct list_head list;
};

struct node_5_0_t {
	char name[32];
	int num;
	char c;
	char phone[10];
	struct list_head list;
};

struct node_5_1_t {
	char name[32];
	int num;
	char c;
	char phone[10];
	struct list_head list;
};

struct node_5_2_t {
	char name[32];
	int num;
	char c;
	char phone[10];
	struct list_head list;
};

struct node_6_0_t {
	char name[32];
	int num;
	char c;
	char phone[10];
	struct list_head list;
};

struct node_6_1_t {
	char name[32];
	int num;
	char c;
	char phone[10];
	struct list_head list;
};

struct node_6_2_t {
	char name[32];
	int num;
	char c;
	char phone[10];
	struct list_head list;
};

struct node_6_3_t {
	char name[32];
	int num;
	char c;
	char phone[10];
	struct list_head list;
};

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
semlock(int argc, char *argv[])
{
	printf("semlock\n");
	fwk_sem_lock(&sem1);
	return 0;
}

int
semunlock(int argc, char *argv[])
{
	printf("semunlock\n");
	fwk_sem_unlock(&sem1);
	return 0;
}

int
mutexlock(int argc, char *argv[])
{
	printf("mutexlock\n");
	fwk_mutex_lock(&mutex1);
	return 0;
}

int
mutexunlock(int argc, char *argv[])
{
	printf("mutexunlock\n");
	fwk_mutex_unlock(&mutex1);
	return 0;
}

int
rwlockrdlock(int argc, char *argv[])
{
	printf("rwlockrdlock\n");
	fwk_rwlock_rdlock(&rwlock1);
	return 0;
}

int
rwlockwrlock(int argc, char *argv[])
{
	printf("rwlockwrlock\n");
	fwk_rwlock_wrlock(&rwlock1);
	return 0;
}

int
rwlockunlock(int argc, char *argv[])
{
	printf("rwlockunlock\n");
	fwk_rwlock_unlock(&rwlock1);
	return 0;
}

int
condlock(int argc, char *argv[])
{
	printf("condlock\n");
	fwk_mutex_lock(cond1.mutex_ptr);
	cond1_variable1 = 0;
	fwk_mutex_unlock(cond1.mutex_ptr);

	return 0;
}

int
condunlock(int argc, char *argv[])
{
	printf("condunlock\n");
	//fwk_mutex_lock(cond1.mutex_ptr);
	cond1_variable1 = 1;
	fwk_cond_signal(&cond1);
	//fwk_mutex_unlock(cond1.mutex_ptr);

	return 0;
}

int
condtimedlock(int argc, char *argv[])
{
	printf("condtimedlock\n");
	fwk_mutex_lock(cond1.mutex_ptr);
	cond1_variable2 = 0;
	fwk_mutex_unlock(cond1.mutex_ptr);

	return 0;
}

int
condtimedunlock(int argc, char *argv[])
{
	printf("condtimedunlock\n");
	fwk_mutex_lock(cond1.mutex_ptr);
	cond1_variable2 = 1;
	fwk_cond_signal(&cond1);
	fwk_mutex_unlock(cond1.mutex_ptr);

	return 0;
}

int
sendq(int argc, char *argv[])
{
	unsigned int q_id;

	struct node_3_t *node_3;
	struct node_4_0_t *node_4_0;
	struct node_4_1_t *node_4_1;
	struct node_5_0_t *node_5_0;
	struct node_5_1_t *node_5_1;
	struct node_5_2_t *node_5_2;
	struct node_6_0_t *node_6_0;
	struct node_6_1_t *node_6_1;
	struct node_6_2_t *node_6_2;
	struct node_6_3_t *node_6_3;

	int count;
	unsigned int i, temp;

	if (argc < 1) {
		printf("sendq q_id\n");
		return E_HELP;
	}

	printf("snedq\n");

	if ((1 != sscanf(argv[0], "%d", &q_id))) {
		printf("%s is not number\n", argv[0]);
		return E_PARM;
	}

	switch (q_id) {
	case 30:
		

	count = 0;
	while(1)
	{
		node_3 = (struct node_3_t *) malloc_safe(sizeof (struct node_3_t));
		printf("+++++msgq 3 send: node addr(%.8X), list addr(%.8X)\n", (unsigned int)node_3, (unsigned int)&node_3->list);

		INIT_LIST_HEAD(&node_3->list);
		node_3->num = count;
		sprintf(node_3->name, "fjldsjfldlffjd");

		for (i = 0; i <= 0xFFFF; i++)
		{
			temp += i;
		}

		if (fwk_msgq_send(q_id_3_0, &node_3->list) == 0) {
			//fwk_thread_yield();
			printf("+++++snedq 3 success\n");
			count++;
		} else {
			printf("------snedq 3 fail\n");
			free_safe(node_3);
			sleep(1);
		}

		if (count == 30)
			break;
	}
		break;
	case 40:
		node_4_0 = (struct node_4_0_t *) malloc_safe(sizeof (struct node_4_0_t));
		printf("msgq 4_0 send: node addr(%.8X), list addr(%.8X)\n", (unsigned int)node_4_0, (unsigned int)&node_4_0->list);

		INIT_LIST_HEAD(&node_4_0->list);
		node_4_0->num = 40;
		sprintf(node_4_0->name, "queue_40");

		if (fwk_msgq_send(q_id_4_0, &node_4_0->list) == 0) {
			printf("snedq 4_0 success\n");
		} else {
			printf("snedq 4_0 fail\n");
		}
		break;
	case 41:
		node_4_1 = (struct node_4_1_t *) malloc_safe(sizeof (struct node_4_1_t));
		printf("msgq 4_1 send: node addr(%.8X), list addr(%.8X)\n", (unsigned int)node_4_1, (unsigned int)&node_4_1->list);

		INIT_LIST_HEAD(&node_4_1->list);
		node_4_1->num = 41;
		sprintf(node_4_1->name, "queue_41");
		node_4_1->c = 'a';
		sprintf(node_4_1->phone, "27888118");

		if (fwk_msgq_send(q_id_4_1, &node_4_1->list) == 0) {
			printf("snedq 4_1 success\n");
		} else {
			printf("snedq 4_1 fail\n");
		}
		break;
	case 50:
		node_5_0 = (struct node_5_0_t *) malloc_safe(sizeof (struct node_5_0_t));
		printf("msgq 5_0 send: node addr(%.8X), list addr(%.8X)\n", (unsigned int)node_5_0, (unsigned int)&node_5_0->list);

		INIT_LIST_HEAD(&node_5_0->list);
		node_5_0->num = 50;
		sprintf(node_5_0->name, "queue_50");
		node_5_0->c = 'b';
		sprintf(node_5_0->phone, "27888118");

		if (fwk_msgq_send(q_id_5_0, &node_5_0->list) == 0) {
			printf("snedq 5_0 success\n");
		} else {
			printf("snedq 5_0 fail\n");
		}
		break;
	case 51:
		node_5_1 = (struct node_5_1_t *) malloc_safe(sizeof (struct node_5_1_t));
		printf("msgq 5_1 send: node addr(%.8X), list addr(%.8X)\n", (unsigned int)node_5_1, (unsigned int)&node_5_1->list);

		INIT_LIST_HEAD(&node_5_1->list);
		node_5_1->num = 51;
		sprintf(node_5_1->name, "queue_51");
		node_5_1->c = 'c';
		sprintf(node_5_1->phone, "27888118");

		if (fwk_msgq_send(q_id_5_1, &node_5_1->list) == 0) {
			printf("snedq 5_1 success\n");
		} else {
			printf("snedq 5_1 fail\n");
		}
		break;
	case 52:
		node_5_2 = (struct node_5_2_t *) malloc_safe(sizeof (struct node_5_2_t));
		printf("msgq 5_2 send: node addr(%.8X), list addr(%.8X)\n", (unsigned int)node_5_2, (unsigned int)&node_5_2->list);

		INIT_LIST_HEAD(&node_5_2->list);
		node_5_2->num = 52;
		sprintf(node_5_2->name, "queue_52");
		node_5_2->c = 'd';
		sprintf(node_5_2->phone, "27888118");

		if (fwk_msgq_send(q_id_5_2, &node_5_2->list) == 0) {
			printf("snedq 5_2 success\n");
		} else {
			printf("snedq 5_2 fail\n");
		}
		break;
	case 60:
		node_6_0 = (struct node_6_0_t *) malloc_safe(sizeof (struct node_6_0_t));
		printf("msgq 6_0 send: node addr(%.8X), list addr(%.8X)\n", (unsigned int)node_6_0, (unsigned int)&node_6_0->list);

		INIT_LIST_HEAD(&node_6_0->list);
		node_6_0->num = 60;
		sprintf(node_6_0->name, "queue_60");
		node_6_0->c = 'e';
		sprintf(node_6_0->phone, "27888118");

		if (fwk_msgq_send(q_id_6_0, &node_6_0->list) == 0) {
			printf("snedq 6_0 success\n");
		} else {
			printf("snedq 6_0 fail\n");
		}
		break;
	case 61:
		node_6_1 = (struct node_6_1_t *) malloc_safe(sizeof (struct node_6_1_t));
		printf("msgq 6_1 send: node addr(%.8X), list addr(%.8X)\n", (unsigned int)node_6_1, (unsigned int)&node_6_1->list);

		INIT_LIST_HEAD(&node_6_1->list);
		node_6_1->num = 61;
		sprintf(node_6_1->name, "queue_61");
		node_6_1->c = 'f';
		sprintf(node_6_1->phone, "27888118");

		if (fwk_msgq_send(q_id_6_1, &node_6_1->list) == 0) {
			printf("snedq 6_1 success\n");
		} else {
			printf("snedq 6_1 fail\n");
		}
		break;
	case 62:
		node_6_2 = (struct node_6_2_t *) malloc_safe(sizeof (struct node_6_2_t));
		printf("msgq 6_2 send: node addr(%.8X), list addr(%.8X)\n", (unsigned int)node_6_2, (unsigned int)&node_6_2->list);

		INIT_LIST_HEAD(&node_6_2->list);
		node_6_2->num = 62;
		sprintf(node_6_2->name, "queue_62");
		node_6_2->c = 'g';
		sprintf(node_6_2->phone, "27888118");

		if (fwk_msgq_send(q_id_6_2, &node_6_2->list) == 0) {
			printf("snedq 6_2 success\n");
		} else {
			printf("snedq 6_2 fail\n");
		}
		break;
	case 63:
		node_6_3 = (struct node_6_3_t *) malloc_safe(sizeof (struct node_6_3_t));
		printf("msgq 6_3 send: node addr(%.8X), list addr(%.8X)\n", (unsigned int)node_6_3, (unsigned int)&node_6_3->list);

		INIT_LIST_HEAD(&node_6_3->list);
		node_6_3->num = 63;
		sprintf(node_6_3->name, "queue_63");
		node_6_3->c = 'h';
		sprintf(node_6_3->phone, "27888118");

		if (fwk_msgq_send(q_id_6_3, &node_6_3->list) == 0) {
			printf("snedq 6_3 success\n");
		} else {
			printf("snedq 6_3 fail\n");
		}
		break;
	default:
		;
	}
	return 0;
}

static CmdEntry cmds[] = {
	{"semlock", semlock, "sem lock"},
	{"semunlock", semunlock, "sem unlock"},
	{"mutexlock", mutexlock, "mutex lock"},
	{"mutexunlock", mutexunlock, "mutex unlock"},
	{"rwlockrdlock", rwlockrdlock, "rwlock read lock"},
	{"rwlockwrlock", rwlockwrlock, "rwlock write lock"},
	{"rwlockunlock", rwlockunlock, "rwlock unlock"},
	{"condlock", condlock, "cond lock"},
	{"condunlock", condunlock, "cond unlock"},
	{"condtimedlock", condtimedlock, "cond timed lock"},
	{"condtimedunlock", condtimedunlock, "cond timed unlock"},
	{"sendq", sendq, "send to queue"},
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
test_thread_1(void *ptr)
{
#if 1
	int count = 0;
	int ret;
#endif

	while (thread1_var > 0) {
#if 1
		fwk_sem_lock(&sem1);
		fwk_sem_unlock(&sem1);

		fwk_mutex_lock(&mutex1);
		fwk_mutex_unlock(&mutex1);

		fwk_mutex_lock(cond1.mutex_ptr);
		if (cond1_variable1 > 0) {
			fwk_mutex_unlock(cond1.mutex_ptr);
		} else {
			fwk_cond_wait(&cond1);
			fwk_mutex_unlock(cond1.mutex_ptr);
		}

		fwk_mutex_lock(cond1.mutex_ptr);
		if (cond1_variable2 > 0) {
			fwk_mutex_unlock(cond1.mutex_ptr);
		} else {
			ret = fwk_cond_timedwait(&cond1, 10000);	//condition wait!!
			if (ret == 0) {
				printf("test_thread_1 cond timed wait, success, %d\n", count++);
			} else if (ret == FWK_COND_ERROR_TIMEOUT) {
				printf("test_thread_1 cond timed wait, TIMEOUT, %d\n", count++);
			} else {
				printf("test_thread_1 cond timed wait, fail, %d\n", count++);
			}
			cond1_variable2 = 1;
			fwk_mutex_unlock(cond1.mutex_ptr);
		}

		fwk_rwlock_rdlock(&rwlock1);
		fwk_rwlock_unlock(&rwlock1);
#endif
		printf("test_thread_1(%d)\n", count++);

		usleep(500000);
	}

	return 0;
}

void *
test_thread_2(void *ptr)
{
	int count = 0;

	while (thread2_var > 0) {
		printf("test_thread_2(%d)\n", count++);
		usleep(1000000);
	}

	return 0;
}

void *
test_thread_3(void *ptr)
{
	int num;
	struct list_head *node;
	struct node_3_t *node_3;

	printf("test_thread_3 id(%d)\n", (int)pthread_self());

	while (thread3_var > 0) {
		num = fwk_msgq_select(msgq_set_3, FWK_MSGQ_WAIT_INFINITE);

		printf("test_thread_3 select(%d)\n", num);

		if (num > 0) {
			if (fwk_msgq_set_isset(msgq_set_3, q_id_3_0)) {
				printf("msgq 3 received (%d)\n", num);
				if (fwk_msgq_recv(q_id_3_0, &node) == 0) {
					printf("msgq 3 received: list addr(%.8X)\n", (unsigned int)node);
					printf("msgq 3 received: success 1\n");
					node_3 = list_entry(node, struct node_3_t, list);
					printf("msgq 3 received: success 2\n");
					printf("msgq 3 received: node addr(%.8X), list addr(%.8X)\n", (unsigned int)node_3, (unsigned int)node);
					//printf("msgq 3 received: num(%d)\n", node_3->num);
					printf("msgq 3 received: num(%d), name(%s)\n", node_3->num, node_3->name);

					free_safe(node_3);
				} else {
					printf("msgq 3 received fail\n");
				}
			}
		} else {
			printf("msgq 3 select fail\n");
			if (num == FWK_COND_ERROR_TIMEOUT) {
				printf("msgq 3 select fail: timeout\n");
			}
		}
	}

	return 0;
}

void *
test_thread_4(void *ptr)
{
	int num;
	struct list_head *node;
	struct node_4_0_t *node_4_0;
	struct node_4_1_t *node_4_1;

	printf("test_thread_4 id(%d)\n", (int)pthread_self());

	while (thread4_var > 0) {
		num = fwk_msgq_select(msgq_set_4, FWK_MSGQ_WAIT_INFINITE);

		printf("test_thread_4 select(%d)\n", num);

		if (num > 0) {
			if (fwk_msgq_set_isset(msgq_set_4, q_id_4_0)) {
				if (fwk_msgq_recv(q_id_4_0, &node) == 0) {
					node_4_0 = list_entry(node, struct node_4_0_t, list);
					printf("msgq 4_0 received: num(%d), name(%s)\n",
						 node_4_0->num, node_4_0->name);

					free_safe(node_4_0);
				} else {
					printf("msgq 4_0 received fail\n");
				}
			}
			if (fwk_msgq_set_isset(msgq_set_4, q_id_4_1)) {
				if (fwk_msgq_recv(q_id_4_1, &node) == 0) {
					node_4_1 = list_entry(node, struct node_4_1_t, list);
					printf("msgq 4_1 received: num(%d), name(%s), c(%c), phone(%s)\n",
						 node_4_1->num, node_4_1->name, node_4_1->c, node_4_1->phone);

					usleep(20000000);

					free_safe(node_4_1);
				} else {
					printf("msgq 4_1 received fail\n");
				}
			}
		} else {
			printf("msgq 4 select fail\n");
		}
	}

	return 0;
}

void *
test_thread_5(void *ptr)
{
	int num;
	struct list_head *node;
	struct node_5_0_t *node_5_0;
	struct node_5_1_t *node_5_1;
	struct node_5_2_t *node_5_2;

	printf("test_thread_5 id(%d)\n", (int)pthread_self());

	while (thread5_var > 0) {
		num = fwk_msgq_select(msgq_set_5, FWK_MSGQ_WAIT_INFINITE);
		//num = fwk_msgq_select(msgq_set_5, 30000);

		printf("test_thread_5 select(%d)\n", num);

		if (num > 0) {
			if (fwk_msgq_set_isset(msgq_set_5, q_id_5_0)) {
				if (fwk_msgq_recv(q_id_5_0, &node) == 0) {
					node_5_0 = list_entry(node, struct node_5_0_t, list);
					printf("msgq 5_0 received: num(%d), name(%s), c(%c), phone(%s)\n",
						 node_5_0->num, node_5_0->name, node_5_0->c, node_5_0->phone);

					free_safe(node_5_0);
				} else {
					printf("msgq 5_0 received fail\n");
				}
			}
			if (fwk_msgq_set_isset(msgq_set_5, q_id_5_1)) {
				if (fwk_msgq_recv(q_id_5_1, &node) == 0) {
					node_5_1 = list_entry(node, struct node_5_1_t, list);
					printf("msgq 5_1 received: num(%d), name(%s), c(%c), phone(%s)\n",
						 node_5_1->num, node_5_1->name, node_5_1->c, node_5_1->phone);

					free_safe(node_5_1);
				} else {
					printf("msgq 5_1 received fail\n");
				}
			}
			if (fwk_msgq_set_isset(msgq_set_5, q_id_5_2)) {
				if (fwk_msgq_recv(q_id_5_2, &node) == 0) {
					node_5_2 = list_entry(node, struct node_5_2_t, list);
					printf("msgq 5_2 received: num(%d), name(%s), c(%c), phone(%s)\n",
						 node_5_2->num, node_5_2->name, node_5_2->c, node_5_2->phone);

					free_safe(node_5_2);
				} else {
					printf("msgq 5_2 received fail\n");
				}
			}
		} else {
			printf("msgq 5 select fail\n");
		}
	}

	return 0;
}

void *
test_thread_6(void *ptr)
{
	int num;
	struct list_head *node;
	struct node_5_2_t *node_5_2;
	struct node_6_0_t *node_6_0;
	struct node_6_1_t *node_6_1;
	struct node_6_2_t *node_6_2;
	struct node_6_3_t *node_6_3;

	printf("test_thread_6 id(%d)\n", (int)pthread_self());

	while (thread6_var > 0) {
		num = fwk_msgq_select(msgq_set_6, FWK_MSGQ_WAIT_INFINITE);

		printf("test_thread_6 select(%d)\n", num);

		if (num > 0) {
			if (fwk_msgq_set_isset(msgq_set_5, q_id_5_2)) {
				if (fwk_msgq_recv(q_id_5_2, &node) == 0) {
					node_5_2 = list_entry(node, struct node_5_2_t, list);
					printf("msgq 5_2 received: num(%d), name(%s), c(%c), phone(%s)\n",
						 node_5_2->num, node_5_2->name, node_5_2->c, node_5_2->phone);

					free_safe(node_5_2);
				} else {
					printf("msgq 5_2 received fail\n");
				}
			}
			if (fwk_msgq_set_isset(msgq_set_6, q_id_6_0)) {
				if (fwk_msgq_recv(q_id_6_0, &node) == 0) {
					node_6_0 = list_entry(node, struct node_6_0_t, list);
					printf("msgq 6_0 received: num(%d), name(%s), c(%c), phone(%s)\n",
						 node_6_0->num, node_6_0->name, node_6_0->c, node_6_0->phone);

					free_safe(node_6_0);
				} else {
					printf("msgq 6_0 received fail\n");
				}
			}

			if (fwk_msgq_set_isset(msgq_set_6, q_id_6_1)) {
				if (fwk_msgq_recv(q_id_6_1, &node) == 0) {
					node_6_1 = list_entry(node, struct node_6_1_t, list);
					printf("msgq 6_1 received: num(%d), name(%s), c(%c), phone(%s)\n",
						 node_6_1->num, node_6_1->name, node_6_1->c, node_6_1->phone);

					free_safe(node_6_1);
				} else {
					printf("msgq 6_1 received fail\n");
				}
			}
			if (fwk_msgq_set_isset(msgq_set_6, q_id_6_2)) {
				if (fwk_msgq_recv(q_id_6_2, &node) == 0) {
					node_6_2 = list_entry(node, struct node_6_2_t, list);
					printf("msgq 6_2 received: num(%d), name(%s), c(%c), phone(%s)\n",
						 node_6_2->num, node_6_2->name, node_6_2->c, node_6_2->phone);

					free_safe(node_6_2);
				} else {
					printf("msgq 6_2 received fail\n");
				}
			}
			if (fwk_msgq_set_isset(msgq_set_6, q_id_6_3)) {
				if (fwk_msgq_recv(q_id_6_3, &node) == 0) {
					node_6_3 = list_entry(node, struct node_6_3_t, list);
					printf("msgq 6_3 received: num(%d), name(%s), c(%c), phone(%s)\n",
						 node_6_3->num, node_6_3->name, node_6_3->c, node_6_3->phone);

					free_safe(node_6_3);
				} else {
					printf("msgq 6_3 received fail\n");
				}
			}
		} else {
			printf("msgq 6 select fail\n");
		}
	}

	return 0;
}

int
main(void)
{
	char *sendqcmd[1];

	printf("test\n");

	sem1.value = 1;
	fwk_sem_create(&sem1);
	fwk_mutex_create(&mutex1);
	fwk_mutex_create(&cond1_mutex);
	cond1.mutex_ptr = &cond1_mutex;
	fwk_cond_create(&cond1, cond1.mutex_ptr);
	fwk_rwlock_create(&rwlock1);

	thread0.policy = FWK_THREAD_SP_NORMAL;
	thread0.priority = 10;
	thread0.stack_size = 128*1024;
	thread0.join = 1;
	sprintf(thread0.name, "test thread 0");

	thread1.policy = FWK_THREAD_SP_NORMAL;
	thread1.priority = 10;
	thread1.stack_size = 128*1024;
	thread1.join = 1;
	sprintf(thread1.name, "test thread 1");

	thread2.policy = FWK_THREAD_SP_NORMAL;
	thread2.priority = 10;
	thread2.stack_size = 128*1024;
	thread2.join = 1;
	sprintf(thread2.name, "test thread 2");

	fwk_thread_create(&thread0, cli_loop, NULL);
	fwk_thread_create(&thread1, test_thread_1, NULL);
	fwk_thread_create(&thread2, test_thread_2, NULL);

	fwk_msgq_module_init();
	q_id_3_0 = fwk_msgq_alloc(0, 20, "Q_3_0");
	q_id_4_0 = fwk_msgq_alloc(FWK_MSGQ_ALLOC_ID_UNASSIGN, 3, "Q_4_0");
	q_id_4_1 = fwk_msgq_alloc(FWK_MSGQ_ALLOC_ID_UNASSIGN, 10, "Q_4_1");
	q_id_5_0 = fwk_msgq_alloc(FWK_MSGQ_ALLOC_ID_UNASSIGN, 10, "Q_5_0");
	q_id_5_1 = fwk_msgq_alloc(FWK_MSGQ_ALLOC_ID_UNASSIGN, 10, "Q_5_1");
	q_id_5_2 = fwk_msgq_alloc(FWK_MSGQ_ALLOC_ID_UNASSIGN, 10, "Q_5_2");
	q_id_6_0 = fwk_msgq_alloc(FWK_MSGQ_ALLOC_ID_UNASSIGN, 10, "Q_6_0");
	q_id_6_1 = fwk_msgq_alloc(FWK_MSGQ_ALLOC_ID_UNASSIGN, 10, "Q_6_1");
	q_id_6_2 = fwk_msgq_alloc(FWK_MSGQ_ALLOC_ID_UNASSIGN, 10, "Q_6_2");
	q_id_6_3 = fwk_msgq_alloc(FWK_MSGQ_ALLOC_ID_UNASSIGN, 10, "Q_6_3");

	msgq_set_3 = fwk_msgq_set_alloc();
	msgq_set_4 = fwk_msgq_set_alloc();
	msgq_set_5 = fwk_msgq_set_alloc();
	msgq_set_6 = fwk_msgq_set_alloc();

	printf("q_3_0(%d), q_4_0(%d), q_4_1(%d), q_5_0(%d), q_5_1(%d), q_5_2(%d), q_6_0(%d), q_6_1(%d), q_6_2(%d), q_6_3(%d)\n",
		 q_id_3_0, q_id_4_0, q_id_4_1, q_id_5_0, q_id_5_1, q_id_5_2, q_id_6_0, q_id_6_1, q_id_6_2, q_id_6_3);

	fwk_msgq_set_zero(msgq_set_3);
	fwk_msgq_set_zero(msgq_set_4);
	fwk_msgq_set_zero(msgq_set_5);
	fwk_msgq_set_zero(msgq_set_6);

	printf("set 3(%d), set 4(%d), set 5(%d), set 6(%d)", msgq_set_3, msgq_set_4, msgq_set_5, msgq_set_6);

	fwk_msgq_set_set(msgq_set_3, q_id_3_0);
	fwk_msgq_set_set(msgq_set_4, q_id_4_0);
	fwk_msgq_set_set(msgq_set_4, q_id_4_1);
	fwk_msgq_set_set(msgq_set_5, q_id_5_0);
	fwk_msgq_set_set(msgq_set_5, q_id_5_1);
	fwk_msgq_set_set(msgq_set_5, q_id_5_2);
	fwk_msgq_set_set(msgq_set_6, q_id_5_2);
	fwk_msgq_set_set(msgq_set_6, q_id_6_0);
	fwk_msgq_set_set(msgq_set_6, q_id_6_1);
	fwk_msgq_set_set(msgq_set_6, q_id_6_2);
	fwk_msgq_set_set(msgq_set_6, q_id_6_3);

	thread3.policy = FWK_THREAD_SP_NORMAL;
	thread3.priority = 10;
	thread3.stack_size = 128*1024;
	thread3.join = 1;
	sprintf(thread3.name, "msgq thread 3");

	thread4.policy = FWK_THREAD_SP_NORMAL;
	thread4.priority = 10;
	thread4.stack_size = 128*1024;
	thread4.join = 1;
	sprintf(thread4.name, "msgq thread 4");

	thread5.policy = FWK_THREAD_SP_NORMAL;
	thread5.priority = 10;
	thread5.stack_size = 128*1024;
	thread5.join = 1;
	sprintf(thread5.name, "msgq thread 5");

	thread6.policy = FWK_THREAD_SP_NORMAL;
	thread6.priority = 10;
	thread6.stack_size = 128*1024;
	thread6.join = 1;
	sprintf(thread6.name, "msgq thread 6");

	//fwk_thread_create(&thread3, test_thread_3, NULL);
	//fwk_thread_create(&thread4, test_thread_4, NULL);
	//fwk_thread_create(&thread5, test_thread_5, NULL);
	//fwk_thread_create(&thread6, test_thread_6, NULL);

	fwk_thread_join(&thread0);

	thread3_var = 0;
	thread4_var = 0;
	thread5_var = 0;
	thread6_var = 0;

	sendqcmd[0] = "30";
	sendq(1, sendqcmd);
	sendqcmd[0] = "40";
	sendq(1, sendqcmd);
	sendqcmd[0] = "50";
	sendq(1, sendqcmd);
	sendqcmd[0] = "60";
	sendq(1, sendqcmd);

	//fwk_thread_join(&thread3);
	//fwk_thread_join(&thread4);
	//fwk_thread_join(&thread5);
	//fwk_thread_join(&thread6);

	fwk_msgq_release(q_id_3_0);
	fwk_msgq_release(q_id_4_0);
	fwk_msgq_release(q_id_4_1);
	fwk_msgq_release(q_id_5_0);
	fwk_msgq_release(q_id_5_1);
	fwk_msgq_release(q_id_5_2);
	fwk_msgq_release(q_id_6_0);
	fwk_msgq_release(q_id_6_1);
	fwk_msgq_release(q_id_6_2);
	fwk_msgq_release(q_id_6_3);

	fwk_msgq_set_release(msgq_set_3);
	fwk_msgq_set_release(msgq_set_4);
	fwk_msgq_set_release(msgq_set_5);
	fwk_msgq_set_release(msgq_set_6);

	fwk_msgq_module_shutdown();

	thread1_var = 0;
	thread2_var = 0;

	fwk_thread_join(&thread1);
	fwk_thread_join(&thread2);
	return 0;
}
