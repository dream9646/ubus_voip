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
 * Module  : util
 * File    : util_socket.c
 *
 ******************************************************************/

#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#include <sys/ioctl.h>
#include <sys/socket.h>

#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <linux/if_ether.h>
#include <linux/if_packet.h>

#include "util.h"
#include "util_socket.h"

int
util_create_raw_socket(char *ifname, struct sockaddr_ll *sockaddr)
{
	struct ifreq ifr;
	int fd;
	int value, ret;

	/* create raw socket */
	if ((fd = socket(AF_PACKET, SOCK_RAW, ETH_P_ALL)) < 0) {
		dbprintf(LOG_ERR, "%s create raw socket error(%s)\n", ifname, strerror(errno));
		return (-1);
	}

	value = 1;
	if ((ret = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &value, sizeof (value))) < 0) {
		dbprintf(LOG_ERR, "%s sockopt SO_REUSEADDR error(%s)\n", ifname, strerror(errno));
		close(fd);
		return (-1);
	}

	{
		int rbuf_size = 1024*1024;
		int optval = 0;
		socklen_t socklen = 4;
		setsockopt(fd, SOL_SOCKET, SO_RCVBUFFORCE, (void *)&rbuf_size, sizeof(rbuf_size));
		getsockopt(fd, SOL_SOCKET, SO_RCVBUF, &optval, &socklen);
		dbprintf_cpuport(LOG_ERR, "%s rcvbuf is set to %dk bytes\n", ifname, optval/1024);
	}

	if ((ret = fcntl(fd, F_SETFL, O_NONBLOCK)) < 0) {
		dbprintf(LOG_ERR, "%s fcntl O_NONBLOCK error(%s)\n", ifname, strerror(errno));
		close(fd);
		return (-1);
	}

	strncpy(ifr.ifr_name, ifname, IFNAMSIZ);

	if ((ret = ioctl(fd, SIOCGIFINDEX, &ifr)) < 0) {
		dbprintf(LOG_ERR, "%s ioctl SIOCGIFINDEX error (%s)\n", ifname, strerror(errno));
		close(fd);
		return (-1);
	}

	memset(sockaddr, 0x00, sizeof(struct sockaddr_ll));
	sockaddr->sll_family = PF_PACKET;
	sockaddr->sll_protocol = htons(ETH_P_ALL);
	sockaddr->sll_halen = 0;
	sockaddr->sll_ifindex = ifr.ifr_ifindex;

	if ((ret =
	     bind(fd,
		  (struct sockaddr *) sockaddr, sizeof (struct sockaddr_ll))) < 0) {
		dbprintf(LOG_ERR, "%s bind error(%s)\n", ifname, strerror(errno));
		close(fd);
		return (-1);
	}

	return fd;
}

int
util_create_udp_socket(unsigned int ip, unsigned short port, struct sockaddr_in *sockaddr)
{
	int fd;
	struct in_addr ipaddr;
	int value, ret;

	ipaddr.s_addr=htonl(ip);	//inet_ntoa(ipaddr)

	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		dbprintf(LOG_ERR, "%s:%d create udp socket error(%s)\n", inet_ntoa(ipaddr), port, strerror(errno));
		return (-1);
	}

	value = 1;
	if ((ret = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &value, sizeof (value))) < 0) {
		dbprintf(LOG_ERR, "%s:%d set sockopt SO_REUSEADDR error(%s)\n", inet_ntoa(ipaddr), port, strerror(errno));
		close(fd);
		return (-1);
	}

	if ((ret = fcntl(fd, F_SETFL, O_NONBLOCK)) < 0) {
		dbprintf(LOG_ERR, "%s:%d fcntl O_NONBLOCK error(%s)\n", inet_ntoa(ipaddr), port, strerror(errno));
		close(fd);
		return (-1);
	}

	memset(sockaddr, 0x00, sizeof(struct sockaddr_in));
	sockaddr->sin_addr.s_addr = htonl(ip);
	sockaddr->sin_family = AF_INET;
	sockaddr->sin_port = htons(port);
	memset(sockaddr->sin_zero, 0, sizeof (sockaddr->sin_zero));

	if ((ret =
	     bind(fd,
		  (struct sockaddr *) sockaddr, sizeof (struct sockaddr_in))) < 0) {
		dbprintf(LOG_ERR, "%s:%d socket bind error(%s)\n", inet_ntoa(ipaddr), port, strerror(errno));
		close(fd);
		return (-1);
	}

	return fd;
}

int
util_create_tcp_socket(unsigned int ip, unsigned short port, struct sockaddr_in *sockaddr)
{
	int fd;
	int value=1, ret;

	if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		dbprintf(LOG_ERR, "create error(%d)\n", fd);
		return (-1);
	}

	if ((ret = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &value, sizeof (value))) < 0) {
		dbprintf(LOG_ERR, "opt error(%d)\n", ret);
		close(fd);
		return (-1);
	}

/*
	if ((ret = fcntl(fd, F_SETFL, O_NONBLOCK)) < 0) {
		dbprintf(LOG_ERR, "nonblock error(%d)\n", ret);
		close(fd);
		return (-1);
	}
*/

	memset(sockaddr, 0x00, sizeof(struct sockaddr_in));
	sockaddr->sin_family = AF_INET;
	sockaddr->sin_addr.s_addr = htonl(ip);
	sockaddr->sin_port = htons(port);
	memset(sockaddr->sin_zero, 0, sizeof (sockaddr->sin_zero));

	if ((ret = bind(fd, (struct sockaddr *) sockaddr, sizeof (struct sockaddr_in))) < 0) {
		dbprintf(LOG_ERR, "socket bind error(%d)\n", ret);
		close(fd);
		return (-1);
	}

	if ((ret = listen(fd, 10)) == -1) {
		dbprintf(LOG_ERR, "listen on tcp port %d error (%d)\n", port, ret);
		close(fd);
		return -1;
	}

	return fd;
}

