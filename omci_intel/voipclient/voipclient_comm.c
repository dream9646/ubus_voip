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
 * Module  : voipclient
 * File    : voipclient_comm.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/types.h>
#include <signal.h>
#include "util.h"
#include "util_run.h"
#include "voipclient_comm.h"
#include "fwk_mutex.h"

int Response_Timeout = UDP_TIMEOUT;
struct fwk_mutex_t mutex_voip;

extern char* voipclient_cmd_dump_buf(char *buf, char *string_g, int length);

struct voip_connect_t *
voipclient_comm_create()
{
	struct voip_connect_t *connect_p;

	connect_p = (struct voip_connect_t *)malloc_safe(sizeof(struct voip_connect_t));
	if (connect_p == NULL) {
		dbprintf_voip(LOG_ERR, "fail to allocat memory\n");
		return NULL;
	}

	connect_p->socket = socket(PF_INET, SOCK_DGRAM, 0);
	if (connect_p->socket < 0) {
		dbprintf_voip(LOG_ERR, "create socket error!!\n");
		free_safe(connect_p);
		return NULL;
	}

	return connect_p;
}

int
voipclient_comm_connect(struct voip_connect_t *connect_p, char *ip, int port)
{
	int ret;

	memset((char *)(&connect_p->remoteaddr), 0, sizeof(struct sockaddr_in));
	connect_p->remoteaddr.sin_family = AF_INET;
	connect_p->remoteaddr.sin_addr.s_addr = inet_addr(ip);
	connect_p->remoteaddr.sin_port = htons(port);

	ret = connect(connect_p->socket, 
				  (struct sockaddr *)&(connect_p->remoteaddr), 
				  sizeof(struct sockaddr_in));
	if(ret!=0) {
		dbprintf_voip(LOG_ERR, "connect error!!\n");
		return -1;
	}

	return 0;
}
#if 0
int
voipclient_comm_bind(struct voip_connect_t *connect_p, char *ip, int port)
{
	int ret;

	memset((char *)(&connect_p->localaddr), 0, sizeof(struct sockaddr_in));
	connect_p->localaddr.sin_family = AF_INET;
	connect_p->localaddr.sin_addr.s_addr = inet_addr(ip);
	connect_p->localaddr.sin_port = htons(port);

	ret = bind(connect_p->socket_resp,
			   (const struct sockaddr*)&(connect_p->localaddr),
			   sizeof(struct sockaddr_in));
	if(ret!=0) {
		dbprintf_voip(LOG_ERR, "bind error!!\n");
		return -1;
	}

	return 0;
}
#endif
void
voipclient_comm_destroy(struct voip_connect_t *connect_p)
{
	if (connect_p) {
		close(connect_p->socket);
		free_safe(connect_p);
	}
}

int
voipclient_comm_cli(const char *command, char *response, const int response_len)
{
	char *buf = NULL;
	int buf_len = BUFFER_SIZE-1;
	char *dump = NULL;
	int dump_len = BUFFER_SIZE-1;
	int ret = 0;
	int len;
	fd_set readMask;
	struct timeval time;
	int recved_len;
	int ret_socket;
	struct voip_connect_t *connect_p;
	int proxy_alive;
	int voip_alive = 2;
	int cmd_no_proxy;
	struct sockaddr_in remoteaddr;

	buf = malloc_safe(buf_len+1);
	if(buf == NULL)
		return -1;
	buf[buf_len] = '\0';
	dump = malloc_safe(dump_len+1);
	if(dump == NULL) {
		free_safe(buf);
		return -1;
	}
	dump[dump_len] = '\0';

	ret = fwk_mutex_lock(&mutex_voip);
	if(ret != 0) {
		ret = -1;
		goto out_free;
	}

	/* create a new socket to prevent recv old response */
	connect_p = voipclient_comm_create();
	if(connect_p == NULL) {
		fwk_mutex_unlock(&mutex_voip);
		ret = -1;
		goto out_free;
	}

	if (omci_env_g.debug_level_voip>= LOG_WARNING) {
		char *cmd2 = strdup(command);
		dbprintf_voip(LOG_WARNING, "send: %s\n", util_trim(cmd2));	// as cmd already contains \n, so we print it without \n
		free(cmd2);
	}

	/* see if this is a special command */
	if(strncmp(command, "get mlt", 7)==0) {
		cmd_no_proxy = 1;
	} else
		cmd_no_proxy = 0;
	if(cmd_no_proxy)
		dbprintf_voip(LOG_ERR, "This command bypass proxy [%s]\n", 
				 voipclient_cmd_dump_buf((char*)command, dump, dump_len));

	proxy_alive = voipclient_comm_check_proxy_exist();
	if(proxy_alive < 0 || cmd_no_proxy == 1) {
		voip_alive = voipclient_comm_check_ipphone_exist();
		if(voip_alive < 0 ) {
			ret = -1;
			goto out_socket;
		}
		/* directly to the voip */
#if 0
		ret_socket = voipclient_comm_connect(connect_p, "127.0.0.1", 8118);
		if (ret_socket < 0) {
			ret = -1;
			goto out_socket;
		}
#endif
		remoteaddr.sin_family = AF_INET;
		remoteaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
		remoteaddr.sin_port = htons(8118);
	} else {
		/* send via the proxy */
#if 0
		ret_socket = voipclient_comm_connect(connect_p, "127.0.0.1", 8119);
		if (ret_socket < 0) {
			ret = -1;
			goto out_socket;
		}
#endif
		remoteaddr.sin_family = AF_INET;
		remoteaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
		remoteaddr.sin_port = htons(8119);
	}

	/* if use connect and send, somehow it will select timeout */
	//ret_socket = send(connect_p->socket, command, strlen(command), 0);
	ret_socket = sendto(connect_p->socket, command, strlen(command), 0,
						(struct sockaddr *)&remoteaddr, sizeof(struct sockaddr_in));
	if (ret_socket < 0) {
		dbprintf_voip(LOG_ERR, "sendto failed!\n");
		ret = -1;
		goto out_socket;
	}

	FD_ZERO(&readMask);
	FD_SET(connect_p->socket, &readMask);
	time.tv_sec = Response_Timeout;
	time.tv_usec = 0;
	ret_socket = select(connect_p->socket + 1, &readMask, NULL, NULL, &time);
	if (ret_socket <= 0) {
		dbprintf_voip(LOG_ERR, "select %s! proxy_alive=%d->%d voip_alive=%d->%d\n",
				 ret_socket==0?"timeout":"error",
				 proxy_alive, voipclient_comm_check_proxy_exist(),
				 voip_alive, voipclient_comm_check_ipphone_exist());
		dbprintf_voip(LOG_ERR, "command might be lost [%s]\n", 
				 voipclient_cmd_dump_buf((char*)command, dump, dump_len));
		ret = -1;
		goto out_socket;
	}
	if (FD_ISSET(connect_p->socket, &readMask)==0) {
		ret = -1;
		goto out_socket;
	}

	recved_len = 0;
#if (SINGLE_PKT_RESP == 0)
	while(0 < (buf_len-recved_len)) {
		FD_ZERO(&readMask);
		FD_SET(socket_resp, &readMask);
		time.tv_sec = 0;
		time.tv_usec = 100;
		if (select(connect_p->socket + 1, &readMask, NULL, NULL, &time) <= 0)
			break;
		if ((len = recv(connect_p->socket, (void *)buf, buf_len-recved_len, 0)) <= 0)
			break;
		recved_len += len;
	}
#else
	if ((len = recv(connect_p->socket, (void *)buf, buf_len-recved_len, 0)) > 0) {
		recved_len += len;
		if (omci_env_g.debug_level_voip>= LOG_WARNING) {
			char *buf2 = strdup(buf);
			dbprintf_voip(LOG_WARNING, "recv: %s\n", util_trim(buf2));	// as buf already contains \n, so we print it without \n
			free(buf2);
		}
	}
#endif

	buf[recved_len]='\0';

	if(recved_len == buf_len)
		dbprintf_voip(LOG_ERR, "recv buffer too small!\n");

	/* the caller do not want the response */
	/* return success when the response begins with "1\n" or "0\n"*/
	if(response == NULL) {
		if ((buf[0]=='1' || buf[0]=='0') && buf[1]==0x0a)
			ret = 0;
		else
			ret = -1;
		goto out_socket;
	}

	if(recved_len < 3) {
		ret = -1;
		goto out_socket;
	}

	if((buf[0]=='1' || buf[0]=='0') && buf[1]==0x0a) {
		char *pos = strstr(buf+2, "\n\n");
		if(pos == NULL) {
			ret = -1;
			goto out_socket;
		}
		*pos = 0;
		strncpy((char*)response, buf+2, response_len);
	} else
		ret = -1;
	
out_socket:
	voipclient_comm_destroy(connect_p);
	fwk_mutex_unlock(&mutex_voip);

out_free:
	if(buf != NULL)
		free_safe(buf);
	if(dump != NULL)
		free_safe(dump);

	return ret;
}

int
voipclient_comm_set_timeout(int timeout)
{
	Response_Timeout = timeout;
	return Response_Timeout;
}

int
voipclient_comm_check_ipphone_exist(void)
{
	//static char *voipname[]={ "fvphone", "ipphone", "5vt_voip_mips", NULL};
	char *data=NULL;
	//int i, ret;

	util_readfile("/var/run/voip.pid", &data);
	if (data) {
		pid_t pid=atoi(data);
		free_safe(data);
		if (pid!=0 && kill(pid, 0)==0)	// process exist?
			return 0;
	}

	/* this is slow ? */
#if 0
	for (i=0; voipname[i]!=NULL; i++) {
		char cmd[128];
		snprintf(cmd, 128, "killall -0 %s", voipname[i]);
		if ((ret = util_run_by_system(cmd)) == 0) //system return 0 if ipphone exist, otherwise not exist
			return 0;
	}
#endif
	
	/* dbprintf_voip(LOG_ERR, "voip process not exist?\n"); */
	return -1;
}

int
voipclient_comm_check_proxy_exist(void)
{
#if 1
	char *data=NULL;
	util_readfile("/var/run/voipcmdd.pid", &data);
	if (data) {
		pid_t pid=atoi(data);
		free_safe(data);
		if (pid!=0 && kill(pid, 0)==0)	// process exist?
			return 0;
	}
#endif

	/* this is slow ? */
#if 0
	 //system return 0 if it exist, otherwise not exist
	if (util_run_by_system("killall -0 voipcmd_proxy") == 0)
		return 0;
#endif

	//dbprintf_voip(LOG_ERR, "voip process not exist?\n");
	return -1;
}

