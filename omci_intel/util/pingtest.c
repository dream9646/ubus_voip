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
 * File    : pingtest.c
 *
 ******************************************************************/

//
// ping.c
//
// Send ICMP echo request to network host
//
// Copyright (C) 2002 Michael Ringgaard. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 
// 1. Redistributions of source code must retain the above copyright 
//    notice, this list of conditions and the following disclaimer.  
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.  
// 3. Neither the name of the project nor the names of its contributors
//    may be used to endorse or promote products derived from this software
//    without specific prior written permission. 
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
// OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
// OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF 
// SUCH DAMAGE.
// 

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <sys/time.h>
#include <sys/times.h>
//#include <sys/signal.h>
#include <signal.h>


#include <netinet/in.h>
//#include <netinet/ip.h>
//#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>


struct iphdr {
#if __BYTE_ORDER == __LITTLE_ENDIAN
	unsigned int h_len:4;	// length of the header 
	unsigned int version:4;	// Version of IP 
#elif __BYTE_ORDER == __BIG_ENDIAN
	unsigned int version:4;	// Version of IP 
	unsigned int h_len:4;	// length of the header 
#else
# error "Please fix <bits/endian.h>"
#endif
	unsigned char tos;	// Type of service 
	unsigned short total_len;	// total length of the packet 
	unsigned short ident;	// unique identifier 
	unsigned short frag_and_flags;	// flags 
	unsigned char ttl;
	unsigned char proto;	// protocol (TCP, UDP etc) 
	unsigned short checksum;	// IP checksum 

	unsigned int source_ip;
	unsigned int dest_ip;
};

struct icmphdr {
	unsigned char i_type;
	unsigned char i_code;	// type sub code
	unsigned short i_cksum;
	unsigned short i_id;
	unsigned short i_seq;

	// This is not the std header, but we reserve space for time
	unsigned long long timestamp;
};

struct pingstat {
	int tmin;
	int tmax;
	int tsum;
	int ntransmitted;
	int nreceived;
};

#define ICMP_ECHO 8
#define ICMP_ECHOREPLY 0
#define ICMP_MIN 8		// Minimum 8 byte icmp packet (just header)

#define DEF_PACKET_SIZE 32
#define MAX_PACKET 1024
#define PKTSIZ (sizeof(struct icmphdr) + MAX_PACKET)

static unsigned short
checksum(unsigned short *buffer, int size)
{
	unsigned long cksum = 0;

	while (size > 1) {
		cksum += *buffer++;
		size -= sizeof(unsigned short);
	}

	if (size)
		cksum += *(unsigned char *)buffer;

	cksum = (cksum >> 16) + (cksum & 0xffff);
	cksum += (cksum >> 16);
	return (unsigned short)(~cksum);
}

static unsigned long long
gettime_in_tenth_ms(void)
{
	struct timeval tv;
	unsigned long long now;	
	gettimeofday(&tv, NULL);
	now=tv.tv_sec*10000+tv.tv_usec/100;

	//printf("%lu\n", now);	
	return now;
}

static void
fill_icmp_data(char *icmp_data, int datasize)
{
	struct icmphdr *icmp_hdr;
	char *datapart;

	icmp_hdr = (struct icmphdr *)icmp_data;

	icmp_hdr->i_type = ICMP_ECHO;
	icmp_hdr->i_code = 0;
	icmp_hdr->i_id = (unsigned short)getpid();
	icmp_hdr->i_cksum = 0;
	icmp_hdr->i_seq = 0;

	datapart = icmp_data + sizeof(struct icmphdr);
	memset(datapart, 'E', datasize - sizeof(struct icmphdr));
}

static void
decode_resp(int seq, char *buf, int bytes, struct sockaddr_in *from, struct pingstat *stat, 
	unsigned long *triptime_log, char *errbuff, int errbuff_size)
{
	struct iphdr *iphdr;
	struct icmphdr *icmphdr;
	unsigned short iphdrlen;
	int triptime;

	iphdr = (struct iphdr *)buf;
	iphdrlen = iphdr->h_len * 4;	// number of 32-bit words *4 = bytes 
	if (bytes < iphdrlen + ICMP_MIN) {
		fprintf(stderr, "too few bytes from %s\n",
		       inet_ntoa(from->sin_addr));
		return;
	}

	icmphdr = (struct icmphdr *)(buf + iphdrlen);
	if (icmphdr->i_type != ICMP_ECHOREPLY) {
		fprintf(stderr, "non-echo type %d recvd\n",
			icmphdr->i_type);
		if (errbuff) {
			if (errbuff_size < bytes - iphdrlen)
				memcpy(errbuff, (void *)icmphdr, errbuff_size);
			else
				memcpy(errbuff, (void *)icmphdr, bytes-iphdrlen);			
		}
		return;
	}

	if (icmphdr->i_id != (unsigned short)getpid()) {
		fprintf(stderr, "someone else's packet!\n");
		return;
	}

	if (icmphdr->i_seq > seq) {
		fprintf(stderr, "seq too large?\n");
		return;
	}

	triptime = gettime_in_tenth_ms() - icmphdr->timestamp;	// unit: 0.1ms
	if (triptime_log)
		triptime_log[icmphdr->i_seq-1]=triptime;			

	stat->tsum += triptime;
	if (triptime < stat->tmin)
		stat->tmin = triptime;
	if (triptime > stat->tmax)
		stat->tmax = triptime;
	stat->nreceived++;

	bytes -= iphdrlen + sizeof(struct icmphdr);
#ifdef HAS_MAIN
	printf("%d bytes from %s:", bytes, inet_ntoa(from->sin_addr));
	printf(" icmp_seq=%d", icmphdr->i_seq);
	printf(" ttl=%d", iphdr->ttl);
	printf(" time=%d.%d ms", triptime/10, triptime%10);
	printf("\n");
#endif	
}

static int
ping(unsigned int dstip, unsigned int srcip, int datasize, int count, int timeout, struct pingstat *stat,
	unsigned long *triptime_log, char *errbuff, int errbuff_size)
{
	int sockraw;
	struct sockaddr_in dest, from;
	int bread, bwrote;
	unsigned int fromlen = sizeof(from);
	char icmp_data[PKTSIZ];
	char recvbuf[PKTSIZ];
	unsigned short seq_no = 0;
	fd_set readMask;
	struct timeval t;
	int i;

	memset(&dest, 0, sizeof(dest));
	dest.sin_family = AF_INET;
	dest.sin_addr.s_addr = dstip;
	dest.sin_port = htons(53);

	if (datasize + sizeof(struct icmphdr) > PKTSIZ) {
		fprintf(stderr, "ping: packet size too large\n");
		return -1;
	}

	sockraw = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if (sockraw < 0) {
		perror("ping: socket");
		return -2;
	}
	if (srcip) {
		struct sockaddr_in srcaddr;
		srcaddr.sin_addr.s_addr=srcip;
		bind(sockraw, (struct sockaddr*)&srcaddr, sizeof(srcaddr));
	}

	if (triptime_log)
		memset(triptime_log, 0xffffffff, sizeof(unsigned long)*count);
	if (errbuff)
		memset(errbuff, 0, errbuff_size);
	
	stat->tmin = 999999999;
	stat->tmax = 0;
	stat->tsum = 0;
	stat->ntransmitted = 0;
	stat->nreceived = 0;

	memset(icmp_data, 0, MAX_PACKET);
	fill_icmp_data(icmp_data, datasize + sizeof(struct icmphdr));

	for (i = 0; i < count; i++) {
		struct icmphdr *icmphdr = (struct icmphdr *)&icmp_data;

		seq_no++;
		icmphdr->i_cksum = 0;
		icmphdr->timestamp = gettime_in_tenth_ms();
		icmphdr->i_seq = seq_no;
		icmphdr->i_cksum = checksum((unsigned short *)icmp_data, datasize + sizeof(struct icmphdr));

		bwrote = sendto(sockraw, icmp_data, datasize + sizeof(struct icmphdr), 0, 
				(struct sockaddr *)&dest, sizeof(dest));
		if (bwrote < 0) {
			close(sockraw);
			perror("ping: sendto");
			return -3;
		}
		if (bwrote < datasize) {
			fprintf(stdout, "Wrote %d bytes\n", bwrote);
		}
		fflush(stdout);

		stat->ntransmitted++;

		t.tv_sec=timeout/1000;
		t.tv_usec=(timeout%1000)*1000;
		FD_ZERO (&readMask);
		FD_SET (sockraw, &readMask);
		if ( select(sockraw+1, &readMask,NULL,NULL, &t) == -1 ) {
			perror("ping: select err?");
			continue;
		}
		if (FD_ISSET(sockraw, &readMask)==0) {
			fprintf(stderr, "ping: select timeout\n");
			continue;
		}

		bread = recvfrom(sockraw, recvbuf, MAX_PACKET, 0, (struct sockaddr *)&from, &fromlen);
		if (bread < 0) {
			close(sockraw);
			perror("ping: recvfrom");
			return -4;
		}
		decode_resp(seq_no, recvbuf, bread, &from, stat, triptime_log, errbuff, errbuff_size);
	}

	while (stat->nreceived < stat->ntransmitted) {
		t.tv_sec=3;
		t.tv_usec=0;
		FD_ZERO (&readMask);
		FD_SET (sockraw, &readMask);
		if ( select(sockraw+1, &readMask,NULL,NULL, &t) == -1 ) {
			perror("ping: select err?");
			continue;
		}
		if (FD_ISSET(sockraw, &readMask)==0) {
			fprintf(stderr, "ping: select timeout\n");
			break;
		}

		bread = recvfrom(sockraw, recvbuf, MAX_PACKET, 0, (struct sockaddr *)&from, &fromlen);
		if (bread < 0) {
			close(sockraw);
			perror("ping: recvfrom");
			return -4;
		}
		decode_resp(seq_no, recvbuf, bread, &from, stat, triptime_log, errbuff, errbuff_size);
	}

	close(sockraw);
	return stat->nreceived;
}

#ifdef HAS_MAIN
int
cmd_ping(int argc, char *argv[])
{
	struct hostent *hp;
	unsigned int dstip;
	struct in_addr dstaddr;
	char *hostname;
	int datasize, i;
	struct pingstat stat;

	if (argc < 2) {
		fprintf(stderr, "usage: %s <host> [data_size]\n", argv[0]);
		return 0;
	}
	hostname = argv[1];

	hp = gethostbyname(hostname);
	if (hp) {
		dstip=*(unsigned int*)hp->h_addr;
	} else {
		dstip = inet_addr(hostname);
		if (dstip == INADDR_NONE) {
			fprintf(stderr, "%s: unknown host %s\n", argv[0], hostname);
			return -1;
		}
	}
	dstaddr.s_addr=dstip;

	if (argc > 2) {
		datasize = atoi(argv[2]);
		if (datasize == 0)
			datasize = DEF_PACKET_SIZE;
	} else
		datasize = DEF_PACKET_SIZE;

	memset(&stat, 0, sizeof(struct pingstat));

	printf("PING %s (%s): %d data bytes\n\n", 
		hostname, inet_ntoa(dstaddr), datasize);

	// call the ping test routine
	ping(dstip, 0, datasize, 15, 100, &stat, NULL, NULL, 0);

	printf("----%s PING Statistics----\n", hostname);
	printf("%d packets transmitted, ", stat.ntransmitted);
	printf("%d packets received, ", stat.nreceived);
	if (stat.ntransmitted) {
		if (stat.nreceived > stat.ntransmitted)
			printf("-- somebody's printing up packets!");
		else
			printf("%d%% packet loss",
			       (int)(((stat.ntransmitted -
				       stat.nreceived) * 100) /
				     stat.ntransmitted));
		printf("\n");
	}

	if (stat.nreceived) {
		printf("round-trip (ms)  min/avg/max = %d.%d/%d.%d/%d.%d\n",
		       stat.tmin/10, stat.tmin%10, 
		       (stat.tsum / stat.nreceived) /10, (stat.tsum / stat.nreceived) %10, 
		       stat.tmax/10, stat.tmax%10);
	}
}

int
main(int argc, char **argv)
{
	return cmd_ping(argc, argv);
}
#else
int 
pingtest(unsigned int dstip, unsigned int srcip, int count, unsigned long *triptime_log, char *errbuff, int errbuff_size)
{
	struct pingstat stat;
	return ping(htonl(dstip), htonl(srcip), DEF_PACKET_SIZE, count, 100, &stat, triptime_log, errbuff, errbuff_size);
}
#endif
