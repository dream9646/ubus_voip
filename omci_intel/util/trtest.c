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
 * File    : trtest.c
 *
 ******************************************************************/

// ugly hack from busybox 1.01
// gcc -DHAS_MAIN -o trtest trtest.c to test the hacked routine for omci with sample main()
// gcc -DNORMAL_TRACEROUTE -o trtest trtest.c to compile normal traceroute

//#define CONFIG_FEATURE_TRACEROUTE_VERBOSE
//#define CONFIG_FEATURE_TRACEROUTE_SO_DEBUG   /* not in documentation man */

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>

//#include "inet_common.h"
#include <netdb.h>
#include <endian.h>
#include <netinet/udp.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>                     

#define MAXPACKET       65535   /* max ip packet size */
#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN  64
#endif

#if !defined(NORMAL_TRACEROUTE) && !defined(HAS_MAIN)
#define	fprintf(x...)	do {;} while (0)
#endif

/*
 * format of a (udp) probe packet.
 */
struct opacket {
	struct ip ip;
	struct udphdr udp;
	u_char seq;             /* sequence number of this packet */
	u_char ttl;             /* ttl packet left with */
	struct timeval tv;      /* time packet left */
};

/*
 * Definitions for internet protocol version 4.
 * Per RFC 791, September 1981.
 */
#define IPVERSION       4
                                       
static char *icmp_errbuff;
static int icmp_errbuff_size=0;

static u_char  packet[512] __attribute__ ((aligned));	/* last inbound (icmp) packet */
static struct opacket  *outpacket;     /* last output (udp) packet */

static int s;                          /* receive (icmp) socket file descriptor */
static int sndsock;                    /* send (udp) socket file descriptor */

static struct sockaddr_in whereto;        /* Who to try to reach */
static int datalen;                    /* How much data */

#if defined(NORMAL_TRACEROUTE) || defined(HAS_MAIN)
static char *hostname;
#endif

static u_short ident;
static u_short port = 32768+666;       /* start udp dest port # for probe packets */

#ifdef CONFIG_FEATURE_TRACEROUTE_VERBOSE
static int verbose;
#endif
static int waittime = 5;               /* time to wait for response (in seconds) */
static int nflag;                      /* print addresses numerically */

static int create_icmp_socket(void)
{
	struct protoent *proto;
	int sock;

	proto = getprotobyname("icmp");
	/* if getprotobyname failed, just silently force
	 * proto->p_proto to have the correct value for "icmp" */
	if ((sock = socket(AF_INET, SOCK_RAW,
			(proto ? proto->p_proto : 1))) < 0) {        /* 1 == ICMP */
		if (errno == EPERM) {
			fprintf(stderr, "you are not root!\n");
			return 0;
		} else {
			fprintf(stderr, "icmp raw sock create error!\n");
			return 0;
                }
	}

	/* drop root privs if running setuid */
	setuid(getuid());

	return sock;
}


/* cache */
struct addr {
	struct sockaddr_in addr;
	char *name;
	int host;
	struct addr *next;
};
                                
#define safe_strncpy	strncpy
static struct addr *INET_nn = NULL;
/* numeric: & 0x8000: default instead of *,
 *          & 0x4000: host instead of net,
 *          & 0x0fff: don't resolve
 */
static int INET_rresolve(char *name, size_t len, struct sockaddr_in *s_in,
				  int numeric, unsigned int netmask)
{
	struct hostent *ent;
	struct netent *np;
	struct addr *pn;
	unsigned long ad, host_ad;
	int host = 0;

	/* Grmpf. -FvK */
	if (s_in->sin_family != AF_INET) {
#ifdef DEBUG
		fprintf(stderr, "rresolve: unsupport address family %d !",
				  s_in->sin_family);
#endif
		errno = EAFNOSUPPORT;
		return (-1);
	}
	ad = (unsigned long) s_in->sin_addr.s_addr;
#ifdef DEBUG
	fprintf(stderr, "rresolve: %08lx, mask %08x, num %08x", ad, netmask, numeric);
#endif
	if (ad == INADDR_ANY) {
		if ((numeric & 0x0FFF) == 0) {
			if (numeric & 0x8000)
				safe_strncpy(name, "default", len);
			else
				safe_strncpy(name, "*", len);
			return (0);
		}
	}
	if (numeric & 0x0FFF) {
		safe_strncpy(name, inet_ntoa(s_in->sin_addr), len);
		return (0);
	}

	if ((ad & (~netmask)) != 0 || (numeric & 0x4000))
		host = 1;
#if 0
	INET_nn = NULL;
#endif
	pn = INET_nn;
	while (pn != NULL) {
		if (pn->addr.sin_addr.s_addr == ad && pn->host == host) {
			safe_strncpy(name, pn->name, len);
#ifdef DEBUG
			fprintf(stderr, "rresolve: found %s %08lx in cache",
					  (host ? "host" : "net"), ad);
#endif
			return (0);
		}
		pn = pn->next;
	}

	host_ad = ntohl(ad);
	np = NULL;
	ent = NULL;
	if (host) {
#ifdef DEBUG
		fprintf(stderr, "gethostbyaddr (%08lx)", ad);
#endif
		ent = gethostbyaddr((char *) &ad, 4, AF_INET);
		if (ent != NULL) {
			safe_strncpy(name, ent->h_name, len);
		}
	} else {
#ifdef DEBUG
		fprintf(stderr, "getnetbyaddr (%08lx)", host_ad);
#endif
		np = getnetbyaddr(host_ad, AF_INET);
		if (np != NULL) {
			safe_strncpy(name, np->n_name, len);
		}
	}
	if ((ent == NULL) && (np == NULL)) {
		safe_strncpy(name, inet_ntoa(s_in->sin_addr), len);
	}
	pn = (struct addr *) malloc(sizeof(struct addr));
	pn->addr = *s_in;
	pn->next = INET_nn;
	pn->host = host;
	pn->name = strdup(name);
	INET_nn = pn;

	return (0);
}

/*
 * Construct an Internet address representation.
 * If the nflag has been supplied, give
 * numeric value, otherwise try for symbolic name.
 */
static inline void
inetname(struct sockaddr_in *from)
{
	char *cp;
	static char domain[MAXHOSTNAMELEN + 1];
	char name[MAXHOSTNAMELEN + 1];
	static int first = 1;
	//const char *ina;

	if (first && !nflag) {
		first = 0;
		if (getdomainname(domain, MAXHOSTNAMELEN) != 0)
			domain[0] = 0;
	}
	cp = 0;
	if (!nflag && from->sin_addr.s_addr != INADDR_ANY) {
		if(INET_rresolve(name, sizeof(name), from, 0x4000, 0xffffffff) >= 0) {
			if ((cp = strchr(name, '.')) &&
			    !strcmp(cp + 1, domain))
				*cp = 0;
			cp = (char *)name;
		}
	}
	/*ina = inet_ntoa(from->sin_addr);
	if (nflag)
		fprintf(stdout, " %s", ina);
	else
		fprintf(stdout, " %s (%s)", (cp ? cp : ina), ina);*/
}

static inline void
print(u_char *buf, int cc, struct sockaddr_in *from)
{
	struct ip *ip;
	int hlen;

	ip = (struct ip *) buf;
	hlen = ip->ip_hl << 2;
	cc -= hlen;

	inetname(from);
#ifdef CONFIG_FEATURE_TRACEROUTE_VERBOSE
	if (verbose)
		fprintf (stdout, " %d bytes to %s", cc, inet_ntoa (ip->ip_dst));
#endif
}

static inline double
deltaT(struct timeval *t1p, struct timeval *t2p)
{
	double dt;

	dt = (double)(t2p->tv_sec - t1p->tv_sec) * 1000.0 +
	     (double)(t2p->tv_usec - t1p->tv_usec) / 1000.0;
	return (dt);
}

static inline int
wait_for_reply(int sock, struct sockaddr_in *from, int reset_timer)
{
	fd_set fds;
	static struct timeval wait;
	int cc = 0;
	unsigned int fromlen = sizeof (*from);

	FD_ZERO(&fds);
	FD_SET(sock, &fds);
	if (reset_timer) {
		/*
		 * traceroute could hang if someone else has a ping
		 * running and our ICMP reply gets dropped but we don't
		 * realize it because we keep waking up to handle those
		 * other ICMP packets that keep coming in.  To fix this,
		 * "reset_timer" will only be true if the last packet that
		 * came in was for us or if this is the first time we're
		 * waiting for a reply since sending out a probe.  Note
		 * that this takes advantage of the select() feature on
		 * Linux where the remaining timeout is written to the
		 * struct timeval area.
		 */
		wait.tv_sec = waittime;
		wait.tv_usec = 0;
	}

	if (select(sock+1, &fds, (fd_set *)0, (fd_set *)0, &wait) > 0)
		cc=recvfrom(s, (char *)packet, sizeof(packet), 0,
			    (struct sockaddr *)from, &fromlen);

	return(cc);
}

#ifdef CONFIG_FEATURE_TRACEROUTE_VERBOSE
/*
 * Convert an ICMP "type" field to a printable string.
 */
static inline const char *
pr_type(u_char t)
{
	static const char * const ttab[] = {
	"Echo Reply",   "ICMP 1",       "ICMP 2",       "Dest Unreachable",
	"Source Quench", "Redirect",    "ICMP 6",       "ICMP 7",
	"Echo",         "ICMP 9",       "ICMP 10",      "Time Exceeded",
	"Param Problem", "Timestamp",   "Timestamp Reply", "Info Request",
	"Info Reply"
	};

	if(t > 16)
		return("OUT-OF-RANGE");

	return(ttab[t]);
}
#endif

static inline int
packet_ok(u_char *buf, int cc, struct sockaddr_in *from, int seq)
{
	struct icmp *icp;
	u_char type, code;
	int hlen;
	struct ip *ip;

	ip = (struct ip *) buf;
	hlen = ip->ip_hl << 2;
	if (cc < hlen + ICMP_MINLEN) {
#ifdef CONFIG_FEATURE_TRACEROUTE_VERBOSE
		if (verbose)
			fprintf(stdout, "packet too short (%d bytes) from %s\n", cc,
				inet_ntoa(from->sin_addr));
#endif
		return (0);
	}
	cc -= hlen;
	icp = (struct icmp *)(buf + hlen);
	type = icp->icmp_type; code = icp->icmp_code;
	if ((type == ICMP_TIMXCEED && code == ICMP_TIMXCEED_INTRANS) ||
	    type == ICMP_UNREACH) {
		struct ip *hip;
		struct udphdr *up;

		hip = &icp->icmp_ip;
		hlen = hip->ip_hl << 2;
		up = (struct udphdr *)((u_char *)hip + hlen);
		if (hlen + 12 <= cc && hip->ip_p == IPPROTO_UDP &&
		    up->source == htons(ident) &&
		    up->dest == htons(port+seq))
			return (type == ICMP_TIMXCEED? -1 : code+1);
	}

	// copy unexpected icmp response for traceroute
	if (icmp_errbuff) {
		memcpy(icmp_errbuff, (char*)icp, (icmp_errbuff_size<cc)?icmp_errbuff_size:cc);
	}

#ifdef CONFIG_FEATURE_TRACEROUTE_VERBOSE
	if (verbose) {
		int i;
		u_long *lp = (u_long *)&icp->icmp_ip;

		fprintf(stdout, "\n%d bytes from %s to %s: icmp type %d (%s) code %d\n",
			cc, inet_ntoa(from->sin_addr), inet_ntoa(ip->ip_dst),
			type, pr_type(type), icp->icmp_code);
		for (i = 4; i < cc ; i += sizeof(long))
			fprintf(stdout, "%2d: x%8.8lx\n", i, *lp++);
	}
#endif
	return(0);
}

static void             /* not inline */
send_probe(int seq, int ttl)
{
	struct opacket *op = outpacket;
	struct ip *ip = &op->ip;
	struct udphdr *up = &op->udp;
	int i;
	struct timezone tz;

	ip->ip_off = 0;
	ip->ip_hl = sizeof(*ip) >> 2;
	ip->ip_p = IPPROTO_UDP;
	ip->ip_len = datalen;
	ip->ip_ttl = ttl;
	ip->ip_v = IPVERSION;
	ip->ip_id = htons(ident+seq);

	up->source = htons(ident);
	up->dest = htons(port+seq);
	up->len = htons((u_short)(datalen - sizeof(struct ip)));
	up->check = 0;

	op->seq = seq;
	op->ttl = ttl;
	(void) gettimeofday(&op->tv, &tz);

	i = sendto(sndsock, (char *)outpacket, datalen, 0, (struct sockaddr*)&whereto,
		   sizeof(struct sockaddr));
	if (i < 0 || i != datalen)  {
		if (i<0)
			fprintf(stderr, "sendto error!\n");
		fprintf(stdout, "traceroute: wrote %s %d chars, ret=%d\n", hostname,
			datalen, i);
		(void) fflush(stdout);
	}
}

int
trtest(unsigned int dstip, unsigned int srcip, int len, int tos, int max_ttl, int nprobes, int options, unsigned int *hostlog, char *errbuff, int errbuff_size)
{
	struct sockaddr_in from, *to;
	int on = 1;
	int seq = 0;
	int i, probe, ttl;

	if (hostlog)
		memset((char*)hostlog, 0, sizeof(unsigned int)*max_ttl);

	icmp_errbuff=errbuff;
	icmp_errbuff_size=errbuff_size;
	if (errbuff)
		memset(errbuff, 0, errbuff_size);
		
	to = &whereto;
	memset(&whereto, 0, sizeof(struct sockaddr_in));
	to->sin_family = AF_INET ;
	to->sin_addr.s_addr=htonl(dstip);

	if (len>=0)
		datalen = len;
	if (datalen < 0 || datalen >= MAXPACKET - sizeof(struct opacket)) {
		fprintf(stderr, "packet size must be 0 <= s < %d.", 
			(int)(MAXPACKET - sizeof(struct opacket)));
		return -2;
	}		    
	datalen += sizeof(struct opacket);

	outpacket = (struct opacket *)malloc((unsigned)datalen);
	memset(outpacket, 0, datalen);
	outpacket->ip.ip_dst = to->sin_addr;
	outpacket->ip.ip_tos = tos;
	outpacket->ip.ip_v = IPVERSION;
	outpacket->ip.ip_id = 0;

	ident = (getpid() & 0xffff) | 0x8000;

	if ((sndsock = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0) {
		fprintf(stderr, "can not create raw socket!\n");
		return -3;
	}

	s = create_icmp_socket();
	if (s==0) {
		fprintf(stderr, "can not create icmp socket!\n");
		return -4;
	}	

#ifdef CONFIG_FEATURE_TRACEROUTE_SO_DEBUG
	if (options & SO_DEBUG)
		(void) setsockopt(s, SOL_SOCKET, SO_DEBUG,
				  (char *)&on, sizeof(on));
#endif
	if (options & SO_DONTROUTE)
		(void) setsockopt(s, SOL_SOCKET, SO_DONTROUTE,
				  (char *)&on, sizeof(on));
#ifdef SO_SNDBUF
	if (setsockopt(sndsock, SOL_SOCKET, SO_SNDBUF, (char *)&datalen,
		       sizeof(datalen)) < 0) {
		fprintf(stderr, "SO_SNDBUF error!\n");
		return -5;
	}
#endif
#ifdef IP_HDRINCL
	if (setsockopt(sndsock, IPPROTO_IP, IP_HDRINCL, (char *)&on,
		       sizeof(on)) < 0) {
		fprintf(stderr, "IP_HDRINCL error!\n");
		return -6;
	}
#endif
#ifdef CONFIG_FEATURE_TRACEROUTE_SO_DEBUG
	if (options & SO_DEBUG)
		(void) setsockopt(sndsock, SOL_SOCKET, SO_DEBUG,
				  (char *)&on, sizeof(on));
#endif
	if (options & SO_DONTROUTE)
		(void) setsockopt(sndsock, SOL_SOCKET, SO_DONTROUTE,
				  (char *)&on, sizeof(on));

	if (srcip) {
		memset(&from, 0, sizeof(struct sockaddr));
		from.sin_family = AF_INET;
		from.sin_addr.s_addr = htonl(srcip);
		outpacket->ip.ip_src = from.sin_addr;
#ifndef IP_HDRINCL
		if (bind(sndsock, (struct sockaddr *)&from, sizeof(from)) < 0)
			fprintf(stderr, "bind error!\n");
			return -8;
		}
#endif
	}

	fprintf(stdout, "traceroute to %s (%s)", hostname,
		inet_ntoa(to->sin_addr));
	/*if (srcip) {
		struct in_addr ipaddr;
		ipaddr.s_addr=htonl(srcip);
		fprintf(stdout, " from %s", inet_ntoa(ipaddr));
	}*/
	fprintf(stdout, ", %d hops max, %d byte packets\n", max_ttl, datalen);

	for (ttl = 1; ttl <= max_ttl; ++ttl) {
		u_long lastaddr = 0;
		int got_there = 0;
		int unreachable = 0;

		fprintf(stdout, "%2d ", ttl);
		for (probe = 0; probe < nprobes; ++probe) {
			int cc, reset_timer;
			struct timeval t1, t2;
			struct timezone tz;
			struct ip *ip;

			(void) gettimeofday(&t1, &tz);
			send_probe(++seq, ttl);
			reset_timer = 1;
			while ((cc = wait_for_reply(s, &from, reset_timer)) != 0) {
				(void) gettimeofday(&t2, &tz);
				if ((i = packet_ok(packet, cc, &from, seq))) {
					reset_timer = 1;
					if (from.sin_addr.s_addr != lastaddr) {
						if (hostlog)
							hostlog[ttl-1]=ntohl(from.sin_addr.s_addr);
						print(packet, cc, &from);
						lastaddr = from.sin_addr.s_addr;
					}
					fprintf(stdout, "  %g ms", deltaT(&t1, &t2));

					switch(i - 1) {
					case ICMP_UNREACH_PORT:
						ip = (struct ip *)packet;
						if (ip->ip_ttl <= 1)
							fprintf(stdout, " !");
						++got_there;
						break;
					case ICMP_UNREACH_NET:
						++unreachable;
						fprintf(stdout, " !N");
						break;
					case ICMP_UNREACH_HOST:
						++unreachable;
						fprintf(stdout, " !H");
						break;
					case ICMP_UNREACH_PROTOCOL:
						++got_there;
						fprintf(stdout, " !P");
						break;
					case ICMP_UNREACH_NEEDFRAG:
						++unreachable;
						fprintf(stdout, " !F");
						break;
					case ICMP_UNREACH_SRCFAIL:
						++unreachable;
						fprintf(stdout, " !S");
						break;
					}
					break;
				} else
					reset_timer = 0;
			}
			if (cc == 0)
				fprintf(stdout, " *");
			(void) fflush(stdout);
		}
		fprintf(stdout, "\n");
		if (got_there || unreachable >= nprobes-1)
			return ttl;
	}

	return ttl;
}

#ifdef NORMAL_TRACEROUTE
void show_usage(void) {
	fprintf(stderr, "%s -dnrv -m max_ttl -p port -q nprobe -s srcip -t tos -w waittime [host]\n", argv[0]);
	exit(1);
}

int
main(int argc, char *argv[])
{
	extern char *optarg;
	extern int optind;
	
	unsigned int srcip=0;
	unsigned int dstip=0;
	
	int len = 0;
	int tos = 0;
	int max_ttl = 30;	
	int nprobes = 3;
	int options = 0;                /* socket options */

	int ch; 

	while ((ch = getopt(argc, argv, "dm:np:q:rs:t:w:v")) != EOF)
		switch(ch) {
		case 'd':
#ifdef CONFIG_FEATURE_TRACEROUTE_SO_DEBUG
			options |= SO_DEBUG;
#endif
			break;
		case 'm':
			max_ttl = atoi(optarg);
			if (max_ttl <= 1) {
				fprintf(stderr, "max ttl must be >1.\n");
				exit(1);
			}
			break;
		case 'n':
			nflag++;
			break;
		case 'p':
			port = atoi(optarg);
			if (port < 1) {
				fprintf(stderr, "port must be >0.\n");
				exit(1);
			}				
			break;
		case 'q':
			nprobes = atoi(optarg);
			if (nprobes < 1) {
				fprintf(stderr, "nprobes must be >0.\n");
				exit(1);
			}
			break;
		case 'r':
			options |= SO_DONTROUTE;
			break;
		case 's':
			/*
			 * set the ip source address of the outbound
			 * probe (e.g., on a multi-homed host).
			 */
			srcip=ntohl(inet_addr(optarg));
			if (srcip == -1) {
				fprintf(stderr, "invalid hostip %s", optarg);
				exit(1);
			}
			break;
		case 't':
			tos = atoi(optarg);
			if (tos < 0 || tos > 255) {
				fprintf(stderr, "tos must be 0 to 255.\n");
				exit(1);
			}
			break;
		case 'v':
#ifdef CONFIG_FEATURE_TRACEROUTE_VERBOSE
			verbose++;
#endif
			break;
		case 'w':
			waittime = atoi(optarg);
			if (waittime <= 1) {
				fprintf(stderr, "wait must be >1 sec.\n");
				exit(1);
			}
			break;
		default:
			show_usage();
		}
	argc -= optind;
	argv += optind;

	if (argc < 1)
		show_usage();

	setlinebuf (stdout);	

	{
		struct hostent *hp = gethostbyname(argv[0]);
		if (hp==NULL) {
			fprintf(stderr, "gethostbyname error!\n");
			exit(1);
		}		
		hostname = (char *)hp->h_name;
		dstip=ntohl(*(unsigned int*)hp->h_addr);
	
		if (argv[1]) {
			len=atoi(argv[1]);
		}
		// dstip, datalen, srcip, nprobes, options
		trtest(dstip, srcip, len, tos, max_ttl, nprobes, options, NULL, NULL, 0);
	}	
}
#else

#ifdef HAS_MAIN
int
main(int argc, char *argv[])
{
	int len = 0;
	int tos = 0;
	int options = 0;                /* socket options */
	int max_ttl = 20;
	int nprobes = 3;

	unsigned int dstip=0x0a14000b;
	unsigned int srcip=0;
	
	unsigned int hostlog[20];
	char errbuff[64];

	if (argc<2) {
		printf("%s [host]\n", argv[0]);
		exit(1);
	} else {
		struct hostent *hp = gethostbyname(argv[1]);
		if (hp==NULL) {
			printf("gethostbyname error!\n");
			exit(1);
		}		
		hostname = (char *)hp->h_name;
		dstip=ntohl(*(unsigned int*)hp->h_addr);
	}

	int n=trtest(dstip, srcip, len, tos, max_ttl, nprobes, options, hostlog, errbuff, 64);
	int i;
	
	printf("%d hosts\n", n);
	for (i=0; i<n; i++) {
		struct in_addr ipaddr;
		ipaddr.s_addr=htonl(hostlog[i]);
		printf("%d %s\n", i, inet_ntoa(ipaddr));
	}
	
}
#endif

#endif
