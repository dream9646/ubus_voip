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
 * Module  : cpuport
 * File    : cpuport_null.c
 *
 ******************************************************************/

#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "list.h"
#include "util.h"
#include "cpuport.h"

static int null_dummy_fd[2];
static fd_set null_read_fdset;

static int
cpuport_null_init()
{
	socketpair(AF_UNIX, SOCK_DGRAM, 0, null_dummy_fd);
	return 0;
}

static int
cpuport_null_shutdown(void)
{
	close(null_dummy_fd[0]);
	close(null_dummy_fd[1]);
	return 0;
}

static int
cpuport_null_send_frame_xmit( struct cpuport_info_t *cpuport_info , unsigned char xmit_type )
{
	return 0;
}

static int
cpuport_null_frame_recv_is_available(struct timeval *timeout)
{
	int sel_rtn;

	FD_ZERO(&null_read_fdset);
	FD_SET(null_dummy_fd[0], &null_read_fdset);
	sel_rtn = select(null_dummy_fd[0] + 1, &null_read_fdset, NULL, NULL, timeout);
	if (sel_rtn > 0) {
		if (FD_ISSET(null_dummy_fd[0], &null_read_fdset))
			return 1;
		return 0;
	} else if (sel_rtn == 0) {
		return 0;
	}
	return -1;
}

static int
cpuport_null_frame_recv(struct cpuport_info_t *cpuport_info, unsigned short buf_len)
{
	int ret;
	if (FD_ISSET(null_dummy_fd[0], &null_read_fdset)) {
		if ((ret = read(null_dummy_fd[0], cpuport_info->buf_ptr, buf_len)) < 0) {
			dbprintf_cpuport(LOG_ERR, "dummy recv error(%d)\n", ret);
			return -1;
		}
		dbprintf_cpuport(LOG_ERR, "quick leave frame recv\n");
		return 1;
	}
	return 0;
}

static int
cpuport_null_frame_recv_abort_select(void)
{
	int dummy, ret = 0;
	if ((ret = write(null_dummy_fd[1], &dummy, sizeof (dummy))) < 0) {
		dbprintf_cpuport(LOG_ERR, "recv_abort_select_fd write error(%d)\n", ret);
	}
	return 0;
}

static int
cpuport_null_frame_recv_abort_loop(void)
{
	int dummy, ret = 0;
	if ((ret = write(null_dummy_fd[1], &dummy, sizeof (dummy))) < 0) {
		dbprintf_cpuport(LOG_ERR, "recv_abort_loop_fd write error(%d)\n", ret);
	}
	return 0;
}

static int
cpuport_null_frame_recv_fill_after_load(struct cpuport_info_t *cpuport_info)
{
	return 0;
}

struct cpuport_hw_t cpuport_null_g = {
	.init = cpuport_null_init,
	.shutdown = cpuport_null_shutdown,
	.frame_send = cpuport_null_send_frame_xmit,
	.frame_recv_is_available = cpuport_null_frame_recv_is_available,
	.frame_recv = cpuport_null_frame_recv,
	.frame_recv_abort_select = cpuport_null_frame_recv_abort_select,
	.frame_recv_abort_loop = cpuport_null_frame_recv_abort_loop,
	.frame_recv_fill_after_load = cpuport_null_frame_recv_fill_after_load,
};
