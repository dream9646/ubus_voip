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
 * Module  : tasks
 * File    : clitask.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <assert.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/sockios.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>

#include "fwk_thread.h"
#include "fwk_msgq.h"
#include "util.h"
#include "util_socket.h"
#include "list.h"
#include "env.h"
#include "clitask.h"
#include "cli.h"
#include "gpon_sw.h"

int clitask_cli_socket_fd_g;
static int clitask_dummy_fd_g[2];

static int clitask_loop_g = 0;
static struct sockaddr_in clitask_cli_tcp_sa_g;
static struct fwk_thread_t clitask_thread_g;
static void *clitask_func(void *ptr);

int clitask_consolefd_g = -1;	// fd to /dev/console
int clitask_nullfd_g = -1;	// fd to /dev/null

pthread_key_t key_private;

static char *
clitask_promp(int cmdindex)		// for omci
{
	static char prompt[32];
	char vendor_id[4];
	unsigned int serial_number;
	unsigned short onuid;
	unsigned int state;
	
	if (gpon_hw_g.onu_serial_number_get) {
		gpon_hw_g.onu_serial_number_get(vendor_id, &serial_number);
	} else {
		vendor_id[0] = vendor_id[1] = vendor_id[2] = vendor_id[3] = '?';
		serial_number = 0;
	}
	if (gpon_hw_g.onu_id_get) {
		gpon_hw_g.onu_id_get(&onuid);
	} else {
		onuid = 255;
	}
	if (gpon_hw_g.onu_state_get) {
		gpon_hw_g.onu_state_get(&state);
	} else {
		state = 0;
	}
	snprintf(prompt, 32, "%c%c%c%c%08x:%03u:O%d> ",
		vendor_id[0], vendor_id[1], vendor_id[2], vendor_id[3],
		serial_number, onuid, state);
	return prompt;
}

/* servlet thread - get cli input and return result through tcp socket fd */
static void *
clitask_servlet_function(void *arg)
{
	char input_line[MAX_CLI_LINE];
	struct clitask_client_info_t client_info;
	int connfd = (int)arg;	
	FILE *fp;
	int i, cmdindex = 1;

	util_set_thread_name("omci-cli-srvlt");
		
	// try several times in case the syscall encounters EINTR
	for (i=0; i<10; i++) {
		if ((fp = fdopen(connfd, "r")) != NULL)
			break;
		usleep(100);
	}

	if (fp) {
		client_info.fd = connfd;
		// save client_info ptr as per thread private var
		// so functions called in per thread can get per thread variable properly
		pthread_setspecific(key_private, (void*)&client_info);	
	
		while (1) {
			util_fdprintf(connfd, "%s", clitask_promp(cmdindex));		// fix prompt len to 11
			if (fgets(input_line, MAX_CLI_LINE, fp) == NULL)	// EOF on tcp socket
				break;
			if (omci_cli_input(connfd, input_line) == CLI_EXIT)
				break;
			cmdindex++;
			if (cmdindex >=10000)
				cmdindex = 0;
		}	
		fclose(fp);

	} else {
		dbprintf(LOG_ERR, "fdopen(client_connfd) error %d(%s)\n", errno, strerror(errno));

		/* When fdopen produced fp (file pointer), it would not duplicate connfd (the original fd);
		 * so, if fp was closed, connfd would also be closed. That's why we have to put the 'close(connfd)'
		 * here to avoid closing connfd twice.
		 */
		close(connfd);
	}

	pthread_exit(0);	/* terminate the thread */
}

static void
clitask_servlet_cleanup(void *thread_private)
{
	struct clitask_client_info_t *client_info = thread_private;
	
	// if the closed thread owns omci console, 
	// return omci console to nullfd 
	if (util_dbprintf_get_console_fd() == client_info->fd)
		util_dbprintf_set_console_fd(clitask_nullfd_g);
	dbprintf(LOG_DEBUG, "connection closed on socket %d\n", client_info->fd);
}

// return per thread private data: the servlet_client_info
void *
clitask_get_thread_private(void)
{
	return pthread_getspecific(key_private);
}

int
clitask_deteach_stdin_stderr_redirect_consolefd(void)
{
	if ((clitask_nullfd_g = open("/dev/null", O_RDWR)) < 0 ) {
		dbprintf(LOG_ERR, "/dev/null open failed?\n");
		return -1;
	}
	if ((clitask_consolefd_g = open("/dev/console", O_RDWR)) < 0 ) {
		dbprintf(LOG_ERR, "/dev/console open failed, use stdout anyway\n");
		clitask_consolefd_g = dup(STDOUT_FILENO);		
	} else {
		dbprintf(LOG_ERR, "/dev/console open ok, consolefd %d\n", clitask_consolefd_g);
	}
	
#ifndef START_CLI_IN_MAIN
	// detach server with stdin/stdout/stderr so it won't be suspend becuae of no tty
	close(STDOUT_FILENO); open("/dev/null",O_RDWR);
	close(STDIN_FILENO); open("/dev/null",O_RDWR);
	close(STDERR_FILENO); open("/dev/null",O_RDWR);
#endif

	util_dbprintf_set_console_fd(clitask_consolefd_g);

	// trick to maskoff pipe err, which might happen when write to a closed fd
	signal(SIGPIPE, SIG_IGN);	

	return 0;
}

int
clitask_init(void)
{
	/*create tcp socket for omci cli */
	if ((clitask_cli_socket_fd_g=util_create_tcp_socket(omci_env_g.cli_ip, omci_env_g.cli_port, &clitask_cli_tcp_sa_g))<0) {
		dbprintf(LOG_ERR, "tcp socket error on ip %08x port %d\n", omci_env_g.cli_ip, omci_env_g.cli_port);
		return -1;
	}
	dbprintf(LOG_ERR, "cli server on tcp port %d\n", omci_env_g.cli_port);
		

	/*create dummy socket for control */
	if (socketpair(AF_UNIX, SOCK_DGRAM, 0, clitask_dummy_fd_g) < 0) {
		dbprintf(LOG_ERR, "dummy socket error(%d)\n", clitask_dummy_fd_g[0]);
		close(clitask_cli_socket_fd_g);
		return -1;
	}

	return 0;
}

int
clitask_start(void)
{
	int ret;

	/*create cli task */
	clitask_thread_g.policy = FWK_THREAD_POLICY_OMCI_CORE;
	clitask_thread_g.priority = FWK_THREAD_PRIO_OMCI_CORE;
	clitask_thread_g.stack_size = 128*1024;
	clitask_thread_g.join = 1;
	sprintf(clitask_thread_g.name, "omci-CLI");

	clitask_loop_g = 1;	//start loop

	if ((ret = fwk_thread_create(&clitask_thread_g, clitask_func, NULL)) < 0) {
		dbprintf(LOG_ERR, "thread create error(%d)\n", ret);
		return (-1);
	}

	return 0;
}

int
clitask_shutdown(void)
{
	int ret;

	if ((ret = close(clitask_cli_socket_fd_g)) < 0) {
		dbprintf(LOG_ERR, "cli close error(%d)\n", ret);
	}

	if ((ret = close(clitask_dummy_fd_g[0])) < 0 || (ret = close(clitask_dummy_fd_g[1])) < 0) {
		dbprintf(LOG_ERR, "dummy close error(%d)\n", ret);
	}

	clitask_cli_socket_fd_g = 0;
	clitask_dummy_fd_g[0] = 0;
	clitask_dummy_fd_g[1] = 0;

	return 0;
}

int
clitask_stop(void)
{
	int dummy;
	int ret;

	clitask_loop_g = 0;	//stop loop

	if ((ret = write(clitask_dummy_fd_g[1], &dummy, sizeof (dummy))) < 0) {
		dbprintf(LOG_ERR, "dummy write error(%d)\n", ret);
	}

	if ((ret = fwk_thread_join(&clitask_thread_g)) < 0) {
		dbprintf(LOG_ERR, "join error(%d)\n", ret);
	}

	return 0;
}

static void *
clitask_func(void *ptr)
{
	fd_set cli_fd_set;
	int maxfd = -1;
	int ret;

	util_set_thread_name("omci-cli");
	omci_env_g.taskname[ENV_TASK_NO_CLI] = "CLI";
	omci_env_g.taskid[ENV_TASK_NO_CLI] = fwk_thread_gettid();
	omci_env_g.pthreadid[ENV_TASK_NO_CLI] = pthread_self();
	omci_env_g.taskloopcount[ENV_TASK_NO_CLI] = 0;
	omci_env_g.taskts[ENV_TASK_NO_CLI] = 0;
	omci_env_g.taskstate[ENV_TASK_NO_CLI] = 0; //idle

	if (clitask_cli_socket_fd_g > maxfd) {
		maxfd = clitask_cli_socket_fd_g;
	}
	if (clitask_dummy_fd_g[0] > maxfd) {
		maxfd = clitask_dummy_fd_g[0];
	}

	clitask_deteach_stdin_stderr_redirect_consolefd();
	pthread_key_create(&key_private, clitask_servlet_cleanup);	// global key for all client thread

	fwk_thread_set_cancellable();
	while (clitask_loop_g) {
		omci_env_g.taskloopcount[ENV_TASK_NO_CLI]++;
		omci_env_g.taskts[ENV_TASK_NO_CLI] = util_get_uptime_sec();
		omci_env_g.taskstate[ENV_TASK_NO_CLI] = 0; //idle
	
		FD_ZERO(&cli_fd_set);
		FD_SET(clitask_cli_socket_fd_g, &cli_fd_set);
		FD_SET(clitask_dummy_fd_g[0], &cli_fd_set);

		if ((ret = select(maxfd + 1, &cli_fd_set, NULL, NULL, NULL)) < 0) {
			dbprintf(LOG_ERR, "select error(%d)\n", ret);
			continue;
		}

		omci_env_g.taskstate[ENV_TASK_NO_CLI] = 1; //running

		// for cli over raw socket
		if (FD_ISSET(clitask_cli_socket_fd_g, &cli_fd_set)) {
			struct sockaddr_in cliaddr_tcp;
			socklen_t clilen = sizeof(cliaddr_tcp);
			int connfd;
			pthread_attr_t pattr;
			pthread_t child;
		
			//dbprintf("wait for accept\n");
			if ((connfd = accept(clitask_cli_socket_fd_g, (struct sockaddr *)&cliaddr_tcp, &clilen)) < 0) {
				if (errno != EINTR) {
					dbprintf(LOG_ERR, "accept error\n");
					exit(1);
				}
			}

			dbprintf(LOG_DEBUG, "new connection accepted on socket %d\n", connfd);
			/* set thread create attributes */
			pthread_attr_init(&pattr);
			pthread_attr_setdetachstate(&pattr, PTHREAD_CREATE_DETACHED);
			if ((ret = pthread_create(&child, &pattr, clitask_servlet_function, (void *)connfd)) < 0) {
				dbprintf(LOG_ERR, "pthread create error %d\n", ret);
				close(connfd);
			}
		}
		
		// for dummy over unix domain socket
		if (FD_ISSET(clitask_dummy_fd_g[0], &cli_fd_set)) {
			char clibuf[CLITASK_BUFFER_SIZE];
			struct sockaddr_in cliaddr_udp;
			// socklen_t socklen = sizeof (cliaddr_udp);
			
			memset(&cliaddr_udp, 0, sizeof (struct sockaddr_in));	
			if ((ret = read(clitask_dummy_fd_g[0], clibuf, CLITASK_BUFFER_SIZE)) < 0) {
				dbprintf(LOG_ERR, "dummy cli error(%d)\n", ret);
				continue;
			}

			dbprintf(LOG_ERR, "task terminate\n");

			continue;
		}
	}

	omci_env_g.taskstate[ENV_TASK_NO_CLI] = 0; //idle
	omci_env_g.taskid[ENV_TASK_NO_CLI] = 0;
	return 0;
}
