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
 * File    : cpuport_sniffer.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <netinet/ip_icmp.h>
#include <netinet/igmp.h>


//#include <linux/icmpv6.h>
//#include <linux/ppp_defs.h>
//#include <linux/icmp.h>
#include <linux/igmp.h>

//#include <linux/if.h>
//#include <linux/if_ether.h>
//#include <linux/if_pppox.h>
//#include <linux/if_arp.h>

#include <linux/ppp_defs.h>



#include "env.h"
#include "util.h"
#include "list.h"
#include "cfm_pkt.h"
#include "cfm_print.h"
#include "cpuport_ipv6.h"

#include <linux/types.h>

struct pppoe_tag {
        __be16 tag_type;
        __be16 tag_len;
        char tag_data[0];
} __attribute__ ((packed));


struct pppoe_hdr {
#if defined(__LITTLE_ENDIAN_BITFIELD)
        __u8 type : 4;
        __u8 ver : 4;
#elif defined(__BIG_ENDIAN_BITFIELD)
        __u8 ver : 4;
        __u8 type : 4;
#else
#error  "Please fix <asm/byteorder.h>"
#endif
        __u8 code;
        __be16 sid;
        __be16 length;
        struct pppoe_tag tag[0];
} __attribute__((packed));


static int sniffer_level=2;
static unsigned char filter_mac_addr[6]={0};
static unsigned short filter_vid=0;
static unsigned short filter_eth_proto=0;
static unsigned char filter_ip_addr[4]={0};
static unsigned short filter_ip_proto=0;
static unsigned short filter_port=0;
static int sip_debug=0;		// 0: disable: 1: 100byte, 2:all

// definition from others /////////////////////////////////////////////////////

// definition from kernel
#define be16_to_cpup(x)		ntohs(*(unsigned short*)(x))
#define be32_to_cpup(x)		ntohl(*(unsigned int*)(x))

// definition from kernel net/ipv4/ipconfig.c bootp_pkt
struct bootphdr {			/* for dhcpv4 */
	unsigned char op;		/* 1=request, 2=reply */
	unsigned char htype;		/* HW address type */
	unsigned char hlen;		/* HW address length */
	unsigned char hops;		/* Used only by gateways */
	unsigned int xid;		/* Transaction ID */
	unsigned short secs;		/* Seconds since we started */
	unsigned short flags;		/* Just what it says */
	unsigned int client_ip;		/* Client's IP address if known */
	unsigned int your_ip;		/* Assigned IP address */
	unsigned int server_ip;		/* (Next, e.g. NFS) Server's IP address */
	unsigned int relay_ip;		/* IP address of BOOTP relay */
	unsigned char hw_addr[16];	/* Client's HW address */
	unsigned char serv_name[64];	/* Server host name */
	unsigned char boot_file[128];	/* Name of boot file */
	unsigned char exten[312];	/* DHCP options / BOOTP vendor extensions */
};

// definition from kernel net/ipv6/mcast.c
/* MLDv2 Report */
struct mld2_grec {
	unsigned char	grec_type;
	unsigned char	grec_auxwords;
	unsigned short	grec_nsrcs;
	struct in6_addr	grec_mca;
	struct in6_addr	grec_src[0];
};
struct mld2_report {
	struct icmp6hdr		mld2r_hdr;
	struct mld2_grec	mld2r_grec[0];
};
#define mld2r_ngrec		mld2r_hdr.icmp6_dataun.un_data16[1]
/* MLDv2 Query */
struct mld2_query {
	struct icmp6hdr		mld2q_hdr;
	struct in6_addr		mld2q_mca;
#if defined(__LITTLE_ENDIAN_BITFIELD)
	unsigned char		mld2q_qrv:3,
				mld2q_suppress:1,
				mld2q_resv2:4;
#elif defined(__BIG_ENDIAN_BITFIELD)
	unsigned char		mld2q_resv2:4,
				mld2q_suppress:1,
				mld2q_qrv:3;
#endif
	unsigned char		mld2q_qqic;
	unsigned short		mld2q_nsrcs;
	struct in6_addr		mld2q_srcs[0];
};

// definition from ISC DHCP
// dhcp6 msg type for relay
#define DHCPV6_RELAY_FORW	   12
#define DHCPV6_RELAY_REPL	   13
// dhcp6 options
#define D6O_CLIENTID				1 /* RFC3315 */
#define D6O_SERVERID				2
#define D6O_IA_NA				3
#define D6O_IA_TA				4
#define D6O_IAADDR				5
#define D6O_ORO					6
#define D6O_PREFERENCE				7
#define D6O_RELAY_MSG				9
#define D6O_INTERFACE_ID			18
#define D6O_RECONFIG_MSG			19
#define D6O_RECONFIG_ACCEPT			20
#define D6O_RECCURSIVE_DNS_SERVER		23 /* RFC3646 */
#define D6O_DOMAIN_SEARCH			24 /* RFC3646 */
#define D6O_IA_PD				25 /* RFC3633 */
#define D6O_IAPREFIX				26 /* RFC3633 */

struct dhcpv6_packet {
	unsigned char msg_type;
	unsigned char transaction_id[3];
	unsigned char options[0];
};

struct dhcpv6_relay_packet {
	unsigned char msg_type;
	unsigned char hop_count;
	unsigned char link_address[16];
	unsigned char peer_address[16];
	unsigned char options[0];
};

struct dhcpv6_opt_hdr {
	unsigned short opt_type;	
	unsigned short opt_data_len;
	unsigned char opt_data[0];
};

// end of definition from others

// L2 related /////////////////////////////////////////////////////////////////

struct mac_desc_t {
	char mac[8];
	char *desc;
};
static struct mac_desc_t mac_desc[]={
	{{0x01, 0x80, 0xc2, 0x00, 0x00, 0x00}, "STP"},
	{{0x01, 0x80, 0xc2, 0x00, 0x00, 0x00}, "PauseFrame"},
	{{0x01, 0x00, 0x0c, 0xcc, 0xcc, 0xcc}, "CDP"},
	{{0x03, 0x00, 0x00, 0x00, 0x00, 0x01}, "NetBios"},
	{{}, NULL}
};
static char ipx_ethernet_8023[]={0xff, 0xff};
static char ipx_ethernet_8022[]={0xe0, 0xe0, 0x03};
static char ipx_ethernet_snap[]={0xaa, 0xaa, 0x03, 0,0,0,0x81,0x37};

static char *
eth_proto_str(unsigned short proto)
{
	switch (proto) {
		case 0x0800:	return("IP");
		case 0x0806:	return("ARP");
		case 0x8035:	return("RARP");
		case 0x809b:	return("ATalk");
		case 0x80f3:	return("ATalk_ARP");
		case 0x8100:	return("8021Q");
		case 0x8137:	return("IPX_EthernetII");
		case 0x86dd:	return("IPV6");
		case 0x8863:	return("PPPoE_DISC");
		case 0x8864:	return("PPPoE_SES");
		case 0x880b:	return("PPP");
		case 0x888e:	return("EAPOL");
		case 0x8902:	return("CFM");
	}
	return("");
}

static char *
ether_addr_str(unsigned char *m)
{
	static char str[32];
	sprintf(str, "%02x:%02x:%02x:%02x:%02x:%02x", m[0], m[1], m[2], m[3], m[4], m[5]); 
	return str;
}

static char *
ppp_proto_str(unsigned short proto)
{
	switch (proto) {
		case PPP_IP: return("IP");
		case PPP_IPX: return("IPX");
		case PPP_IPV6: return("IPV6");
		case PPP_IPCP: return("IPCP");
		case PPP_IPXCP: return("IPXCP");
		case PPP_IPV6CP: return("IPV6CP");
		case PPP_CCP: return("CCP");
		case PPP_LCP: return("LCP");
		case PPP_PAP: return("PAP");
		case PPP_LQR: return("LQR");
		case PPP_CHAP: return("CHAP");
	}
	return("");
}

static char *
pppoe_code_str(unsigned char code)
{
	static char code_str[16];
	switch (code) {
		case 0x7: return("PADO");
		case 0x9: return("PADI");
		case 0xa: return("PADG");
		case 0xb: return("PADC");
		case 0xc: return("PADQ");
		case 0x19: return("PADR");
		case 0x65: return("PADS");
		case 0xa7: return("PADT");
		case 0xd3: return("PADM");
		case 0xd4: return("PADN");
	}
	sprintf(code_str,"code=0x%x", code);
	return code_str;
}

// EAPOL ////////////////////////////////////////////////////////////////////////

#define EAPOL_TYPE_EAP		0 // EAP authentication handshake packet
#define EAPOL_TYPE_START	1 // Request by Peer to begin (no data)
#define EAPOL_TYPE_LOGOFF	2 // Request by Peer to terminate (no data)
#define EAPOL_TYPE_KEY		3 // EAPOL-Key packet

#define EAPOL_CODE_REQUEST	1 
#define EAPOL_CODE_RESPONSE	2 
#define EAPOL_CODE_SUCCESS	3 
#define EAPOL_CODE_FAILURE	4

#define EAPOL_REQRES_TYPE_ID		1	// identify
#define EAPOL_REQRES_TYPE_NOTIFICATION	2	// notification
#define EAPOL_REQRES_TYPE_NAK		3	// nak, response only
#define EAPOL_REQRES_TYPE_MD5		4	// md5 challenge
#define EAPOL_REQRES_TYPE_OTP		5	// one time password
#define EAPOL_REQRES_TYPE_GTC		6	// Generic Token Card
#define EAPOL_REQRES_TYPE_TLS		13	// TLS challenge
#define EAPOL_REQRES_TYPE_EXPENDED	254	// Expanded Types
#define EAPOL_REQRES_TYPE_EXPERIMENTAL	255	// Experimental use

struct eapol_hdr_t {
	unsigned char version;	// EAPOL HDR: version identifier, always 1
	unsigned char type;	// EAPOL HDR: archetype identifier indicating format of payload
	unsigned short length;	// EAPOL HDR: length of payload, in network byte order
	unsigned char code;		// EAPOL PAYLOAD: Code
	unsigned char id;		// EAPOL PAYLOAD: Id
	unsigned short length_payload;	// EAPOL PAYLOAD: Length of payload
	unsigned char reqres_type; 	// EAPOL PAYLOAD: challenge type
	unsigned char data[0];
} __attribute__ (( packed ));

static char *
eapol_type_str(unsigned char type)
{
	switch (type) {
		case EAPOL_TYPE_EAP: return "EAP";
		case EAPOL_TYPE_START: return "EAP-Start";
		case EAPOL_TYPE_LOGOFF: return "EAP-Logoff";
		case EAPOL_TYPE_KEY: return "EAP-Key";
	}
	return "EAP-?";
}

static char *
eapol_code_str(unsigned char code)
{
	switch (code) {
		case EAPOL_CODE_REQUEST: return "Request";
		case EAPOL_CODE_RESPONSE: return "Response";
		case EAPOL_CODE_SUCCESS: return "Success";
		case EAPOL_CODE_FAILURE: return "Failure";
	}
	return "Code-?";
}

static char *
eapol_reqres_type_str(unsigned char reqres_type)
{
	switch (reqres_type) {
		case EAPOL_REQRES_TYPE_ID:		return "identify";
		case EAPOL_REQRES_TYPE_NOTIFICATION:	return "notification";
		case EAPOL_REQRES_TYPE_NAK:		return "NAK";
		case EAPOL_REQRES_TYPE_MD5:		return "MD5 challenge";
		case EAPOL_REQRES_TYPE_OTP:		return "one time password";
		case EAPOL_REQRES_TYPE_GTC:		return "generic token card";
		case EAPOL_REQRES_TYPE_TLS:		return "TLS challenge";
		case EAPOL_REQRES_TYPE_EXPENDED:	return "expanded types";
		case EAPOL_REQRES_TYPE_EXPERIMENTAL:	return "experimental";
	}
	return "ReqRes-?";
}

// ipv4 related /////////////////////////////////////////////////////////////////

static char *
ip_proto_str(unsigned char proto)
{
	switch (proto) {
		case IPPROTO_ICMP: return("ICMP");
		case IPPROTO_IGMP: return("IGMP");
		case IPPROTO_IPIP: return("IP tunnel");
		case IPPROTO_TCP: return("TCP");
		case IPPROTO_EGP: return("EGP");
		case IPPROTO_UDP: return("UDP");
		case IPPROTO_RSVP: return("IRSVP");
		case IPPROTO_GRE: return("Cisco GRE tunnel");
		case IPPROTO_IPV6: return("IPV6 over IPV4");
		case IPPROTO_ESP: return("ESP");
		case IPPROTO_AH: return("AH");
		case IPPROTO_PIM: return("PIM");
		case IPPROTO_RAW: return("RAW");
	}
	return("");
}

static char *
tcp_flag_str(char flag)
{
	static char *flagstr[]={"fin", "syn", "rst", "psh", "ack", "urg", "ece", "cwr"};
	static char buf[32];
	int i, len=0;

	for (i=0; i<8; i++) {
		if (flag & (1<<i)) {
			if (len>0) len+=sprintf(buf+len, "%s", ",");
			len+=sprintf(buf+len, "%s", flagstr[i]);
		}
	}
	return(buf);
}

static char *
icmp_type_str(unsigned char type)
{
	static char type_str[16];
	switch (type) {
		case ICMP_ECHOREPLY: 	return "Ping Reply";
		case ICMP_ECHO: 	return "Ping Req";
		case ICMP_DEST_UNREACH: return "Dst Unreachable";
		case ICMP_REDIRECT: 	return "Redirect";
	}
	sprintf(type_str, "Type %x", type);
	return type_str;
}

static char *
igmp_type_str(unsigned char type)
{
	static char type_str[16];
	switch (type) {
		case IGMP_HOST_MEMBERSHIP_QUERY:	return "MbShip Query";
		case IGMP_HOST_MEMBERSHIP_REPORT:	return "MbShip Rpt";
		case IGMP_TRACE:			return "Trace";
		case IGMP_HOST_LEAVE_MESSAGE:		return "Leave";
#ifdef IGMPV2_HOST_MEMBERSHIP_REPORT
		case IGMPV2_HOST_MEMBERSHIP_REPORT:	return "MbShip Rpt V2";
		case IGMPV3_HOST_MEMBERSHIP_REPORT:	return "MbShip Rpt V3";
#else
		case IGMP_HOST_NEW_MEMBERSHIP_REPORT:	return "MbShip Rpt V2";
#endif
	}	
	sprintf(type_str, "Type %x", type);
	return type_str;
}

static char *dhcptypestr[]={
	"",		// 0
	"discovery",	// 1
	"offer",	// 2
	"request",	// 3
	"decline",	// 4
	"ack",		// 5
	"nak",		// 6
	"release"	// 7
};

// ipv6 related /////////////////////////////////////////////////////////////////
static int
is_zero_ipv6_addr(struct in6_addr *ipv6addr)
{
	if (	ipv6addr->s6_addr32[0]==0 && 
		ipv6addr->s6_addr32[1]==0 &&
		ipv6addr->s6_addr32[2]==0 &&
		ipv6addr->s6_addr32[3]==0)
		return 1;
	return 0;
}

static char *
ipv6_addr_str(unsigned char *m)
{
	static char str[64];
	struct in6_addr *h=(struct in6_addr *)m;
	sprintf(str, "%x:%x:%x:%x:%x:%x:%x:%x", 
		ntohs(h->s6_addr16[0]), ntohs(h->s6_addr16[1]),
		ntohs(h->s6_addr16[2]), ntohs(h->s6_addr16[3]),
		ntohs(h->s6_addr16[4]), ntohs(h->s6_addr16[5]),
		ntohs(h->s6_addr16[6]), ntohs(h->s6_addr16[7]));
	return str;
}

static char *
ipv6_nexthdr_str(unsigned char nexthdr)
{
	switch (nexthdr) {
		case NEXTHDR_HOP: return "HOP";
		case NEXTHDR_TCP: return "TCP";
		case NEXTHDR_UDP: return "UDP";
		case NEXTHDR_IPV6: return "IPV6";
		case NEXTHDR_ROUTING: return "ROUTING";
		case NEXTHDR_FRAGMENT: return "FRAGMENT";
		case NEXTHDR_ESP: return "ESP";
		case NEXTHDR_AUTH: return "AUTH";
		case NEXTHDR_ICMP: return "ICMP";
		case NEXTHDR_NONE: return "NONE";
		case NEXTHDR_DEST: return "DEST";
		case NEXTHDR_OSPFV3: return "OSPFV3";
		case NEXTHDR_MOBILITY: return "MOBILITY";
	}
	return "";
}

static char *
icmp6_type_str(unsigned char type)
{
	static char type_str[16];
	switch (type) {
		case ICMPV6_DEST_UNREACH:	return "Dst Unreachable";		// 1
		case ICMPV6_PKT_TOOBIG:		return "Pkt too long";			// 2
		case ICMPV6_TIME_EXCEED:	return "time exceed";			// 3
		case ICMPV6_PARAMPROB:		return "Parm problem";			// 4
		case ICMPV6_ECHO_REQUEST:	return "Ping Req";			// 128
		case ICMPV6_ECHO_REPLY:		return "Ping Reply";			// 129
		case ICMPV6_MGM_QUERY:		return "Mcast Listener Query";		// 130, query v1/2
		case ICMPV6_MGM_REPORT:		return "Mcast Listener Report";		// 131, report v1
		case ICMPV6_MLD2_REPORT:	return "MLD Report v2";			// 143, report v2
		case NDISC_ROUTER_SOLICITATION:	return "Router Solicit";		// 133
		case NDISC_ROUTER_ADVERTISEMENT:return "Router Advertis";		// 134
		case NDISC_NEIGHBOUR_SOLICITATION:	return "Neighbor Solicit";	// 135
		case NDISC_NEIGHBOUR_ADVERTISEMENT:	return "Neighbor Advertise";	// 136
		case NDISC_REDIRECT:		return "Redirect";			// 137
	}
	sprintf(type_str, "Type %x", type);
	return type_str;
}

static char *
icmp6_unreach_code_str(unsigned char code)
{
	unsigned char *codestr[] ={
		"no route", 
		"adm prohibit", 
		"no neighbor",
		"host unreach",
		"port unreach",
		"5",
		"6",
		"7"
	};
	return codestr[code&0x7];
}

static char *dhcp6typestr[]={
	"",		// 0 
	"solicit",	// 1
	"advertise",	// 2
	"req",		// 3
	"confirm",	// 4
	"renew",	// 5
	"rebind",	// 6
	"reply",	// 7
	"release",	// 8
	"decline",	// 9
	"reconfig",	// 10
	"info_req",	// 11
	"RELAY_forward",// 12
	"RELAY_reply",	// 13
	"lease_qry",	// 14
	"lease_qry_reply" // 15
};

static char *dhcp6_opt_str(unsigned char optid)
{
	static char str[16];
	static char *dhcp6optstr[]={
		// 0..9
		"0", "clientid", "serverid", "ia_na", "ia_ta",
		"iaaddr", "oro", "prefer", "elapsed_time", "relay_msg",
		// 10..19
		"10", "auth", "unicast", "status_code", "rapid_commit", 
		"user_class", "vendor_class", "vendor_opts", "interface_id", "reconf_msg",
		// 20..29
		"reconf_accept", "sip_servers_dns", "sip_servers_addr", "name_srv", "domain_search",
		"ia_pd", "iaprefix", "nis_servers", "nisp_servers", "nis_domain_name",
		// 30..39
		"nisp_domain_name", "sntp_servers", "info_refresh_time", "bcmcs_server_d", "bcmcs_server_a",
		"35", "geoconf_civic", "remote_id", "subscriber_id", "client_fqdn",
		// 40..49
		"pana_agent",	 "new_posix_timezone", "new_tzdb_timezone", "ero", "lq_query",
		"client_data", "clt_time", "lq_relay_data", "lq_client_link", "49"
	};

	if (optid <= 49)
		return dhcp6optstr[optid];
	sprintf(str, "opt%d", optid);
	return str;
};

// parser main /////////////////////////////////////////////////////////////////
#ifndef IP_OFFSET
#define IP_OFFSET IP_OFFMASK
#endif
static int 
sniffer_parser_ipv4(char *line, void *ip_start, int ip_len)
{
	char *str = line;

	struct iphdr *ip = ip_start;
	unsigned char *s=(unsigned char*)&(ip->saddr);
	unsigned char *d=(unsigned char*)&(ip->daddr);
	char frag_more;
	unsigned short frag_offset;

	if (filter_ip_proto && filter_ip_proto!=ip->protocol)
		return -1;
	if (*(int*)filter_ip_addr && *(int*)filter_ip_addr!=ip->saddr && *(int*)filter_ip_addr!=ip->daddr)
		return -1;

	str+=sprintf(str, " len=%d dscp=0x%x", ntohs(ip->tot_len), (ip->tos)&0x3f);

	frag_more=(ip->frag_off & htons(IP_MF))?1:0;
	frag_offset=(ntohs(ip->frag_off) & IP_OFFSET) << 3;		

	if (frag_offset>0) {
		str+=sprintf(str, " [0x%02x]%s %d.%d.%d.%d > %d.%d.%d.%d Frag_ofs=%d",
			ip->protocol, ip_proto_str(ip->protocol),
			s[0],s[1],s[2],s[3], d[0],d[1],d[2],d[3], frag_offset);

	} else if (ip->protocol==IPPROTO_TCP || ip->protocol==IPPROTO_UDP) {
		struct tcphdr *tcp=(void *)ip+ip->ihl*4;
		struct udphdr *udp=(void *)ip+ip->ihl*4;	
		unsigned short sp=0, dp=0;				

		if (ip->protocol==IPPROTO_TCP) {
			sp=ntohs(tcp->source);
			dp=ntohs(tcp->dest);
		} else {
			sp=ntohs(udp->source);
			dp=ntohs(udp->dest);
		}	
		if (filter_port && filter_port!=sp && filter_port!=dp)
			return -1;
		str+=sprintf(str, " [0x%02x]%s %d.%d.%d.%d:%d > %d.%d.%d.%d:%d%s",
			ip->protocol, ip_proto_str(ip->protocol),
			s[0],s[1],s[2],s[3], sp, d[0],d[1],d[2],d[3], dp, frag_more?" MF":"");
			
		if (ip->protocol==IPPROTO_TCP) {
			 str+=sprintf(str, " (%s)", tcp_flag_str(((char*)tcp)[13]));
		} else {
			if ((sp==68 && dp==67) || (sp==67 && dp==68)) {
				if (sp==68 && dp==67) str+=sprintf(str, "\nDHCP Req");
				else str+=sprintf(str, "\nDHCP Reply");
				if (!frag_more && frag_offset==0) {
					struct bootphdr *bootp=(void *)udp+sizeof(struct udphdr);
					unsigned char *h=(unsigned char*)&(bootp->hw_addr);
					unsigned char *c=(unsigned char*)&(bootp->client_ip);
					unsigned char *y=(unsigned char*)&(bootp->your_ip);
					unsigned char *e=(unsigned char*)&(bootp->exten);
					str+=sprintf(str, ", xid=%08x, hw=%s, ", bootp->xid, ether_addr_str(h));
					if (bootp->op==1) // request
						str+=sprintf(str, "clientip=%d.%d.%d.%d", c[0],c[1],c[2],c[3]);
					else // ack
						str+=sprintf(str, "yourip=%d.%d.%d.%d", y[0],y[1],y[2],y[3]);
					if (e[0]==99 && e[1]==130 && e[2]==83 && e[3]==99) {// dhcp cookie
						// assume dhcp type is 1st option
						if (e[4]==0x35 && e[5]==1 && e[6]>=1 && e[6]<=7)
							str+=sprintf(str, " type=%s", dhcptypestr[e[6]]);
					}
				}
			} else if (sip_debug >0 && (sp==5060 || dp==5060)) {
				char *sip=(void *)udp+sizeof(struct udphdr);
				unsigned int sip_len = udp->len - 8;
				if (sip_debug == 1) {
					if (sip_len > 100) {
						char buff[100];
						int i;
						for (i=0; i<99; i++) {
							if (sip[i] == 0xd)		// \n
								buff[i]=',';
							else if (sip[i] == 0xa)	// \r
								buff[i]=' ';
							else
								buff[i]=sip[i];
							buff[99]=0;
						}
						str+=sprintf(str, "\n%s...", buff);
					} else {
						str+=snprintf(str, sip_len+2, "\n%s", sip);
					}
				} else if (sip_debug == 2) {
					str+=snprintf(str, sip_len+2, "\n%s", sip);
				}
			}
		}
	} else {	// ip protocol other than tcp/udp
		if (filter_port)	// once port is specified, assume tcp/udp
			return -1;
		str+=sprintf(str, " [0x%02x]%s %d.%d.%d.%d > %d.%d.%d.%d%s",
			ip->protocol, ip_proto_str(ip->protocol),
			s[0],s[1],s[2],s[3], d[0],d[1],d[2],d[3], frag_more?" MF":"");

		if (ip->protocol==IPPROTO_ICMP) {			
			struct icmphdr *icmp=(void *)ip+ip->ihl*4;

			str+=sprintf(str, "\n%s", icmp_type_str(icmp->type));
			if (icmp->type==ICMP_DEST_UNREACH ||icmp->type==ICMP_REDIRECT) {
				struct iphdr *ip2=(void *)icmp+8;
				unsigned char *d2=(unsigned char*)&(ip2->daddr);

				str+=sprintf(str, ", Dst=%d.%d.%d.%d", d2[0],d2[1],d2[2],d2[3]);
				if (ip2->protocol==IPPROTO_TCP) {
					struct tcphdr *tcp2=(void *)ip2+ip2->ihl*4;
					unsigned short dp2=ntohs(tcp2->dest);
					str+=sprintf(str, ":%d tcp", dp2);
				} else if (ip2->protocol==IPPROTO_UDP) {
					struct tcphdr *udp2=(void *)ip2+ip2->ihl*4;
					unsigned short dp2=ntohs(udp2->dest);
					str+=sprintf(str, ":%d udp", dp2);
				}
			}
		} else if (ip->protocol==IPPROTO_IGMP) {
			struct igmphdr *igmp=(void *)ip+ip->ihl*4;
			str+=sprintf(str, "\n%s", igmp_type_str(igmp->type));
			if (	igmp->type==IGMP_HOST_MEMBERSHIP_QUERY ||
				igmp->type==IGMP_HOST_MEMBERSHIP_REPORT ||
				igmp->type==IGMP_TRACE ||
#ifdef IGMPV2_HOST_MEMBERSHIP_REPORT
				igmp->type==IGMPV2_HOST_MEMBERSHIP_REPORT ||
#else
				igmp->type==IGMP_HOST_NEW_MEMBERSHIP_REPORT ||
#endif
				igmp->type==IGMP_HOST_LEAVE_MESSAGE ) {
					unsigned char *g=(unsigned char*)&(igmp->group);
					str+=sprintf(str, ", grp=%d.%d.%d.%d", g[0],g[1],g[2],g[3]);
#ifdef IGMPV3_HOST_MEMBERSHIP_REPORT
			} else if (igmp->type==IGMPV3_HOST_MEMBERSHIP_REPORT) {
				static char *grectypestr[8]= { 
					"0", "include", "exclude", "chg_to_include", 
					"chg_to_exclude", "allow_new_src", "blk_old_src", "7" };				
				struct igmpv3_report *igmpv3_report=(void *)ip+ip->ihl*4;
				struct igmpv3_grec *grec = &igmpv3_report->grec[0];
				unsigned short gtotal = ntohs(igmpv3_report->ngrec);
				int i, j;
				// dump first 3 groups, and each group dump first 3 srcip
				for (i=0; i< gtotal; i++) {
					unsigned short stotal = ntohs(grec->grec_nsrcs);
					unsigned char *g=(unsigned char*)&grec->grec_mca;
					str+=sprintf(str, ", %s grp=%d.%d.%d.%d", grectypestr[grec->grec_type&0x7], g[0],g[1],g[2],g[3]);
					for (j=0; j<stotal; j++) {
						unsigned char *s=(unsigned char*)&grec->grec_src[0];
						if (j==0) {
							str+=sprintf(str, " %usrc=%d.%d.%d.%d", stotal, s[0],s[1],s[2],s[3]);
						} else if (j<=2) {
							str+=sprintf(str, " %d.%d.%d.%d", s[0],s[1],s[2],s[3]);
						} else {
							str+=sprintf(str, "...");
							break;
						}
					}
					if (i==2 && gtotal>i) {
						str+=sprintf(str, ", total %ugrp...\n", gtotal);
						break;
					}
					grec = (void*)grec + sizeof(struct igmpv3_grec) + stotal * 4;
				}
#endif					
			}
		}
	}
	return str - line;
}

static int
sniffer_parser_ipv6_dhcp6_options(char *line, void *dhcp6opts_start, int dhcp6opts_len)
{
	char *str = line;	
	struct dhcpv6_opt_hdr *o = dhcp6opts_start;

	while ((void*)o < (void *)dhcp6opts_start+dhcp6opts_len) {
		unsigned short opt_type = ntohs(o->opt_type);
		unsigned short opt_data_len = ntohs(o->opt_data_len);
		void *o_end = (void *)o + sizeof(struct dhcpv6_opt_hdr) + opt_data_len;		

		if (o_end > (void *)dhcp6opts_start+dhcp6opts_len)
			break;
			
		if (opt_type == D6O_CLIENTID || opt_type == D6O_SERVERID) {	// 1,2
			unsigned short duid_type=be16_to_cpup((void *)(o->opt_data));
			unsigned short hw_type=be16_to_cpup((void *)(o->opt_data+2));
			str+=sprintf(str, ", %s:ia=%x", dhcp6_opt_str(opt_type), be32_to_cpup((void *)(o->opt_data)));
			if (hw_type == 1)  {// ethernet
				if (duid_type == 1) { // link addr + time
					str+=sprintf(str, " hw=%s", ether_addr_str(o->opt_data + 8));
				} else if (duid_type == 3) {	// link addr
					str+=sprintf(str, " hw=%s", ether_addr_str(o->opt_data + 4));
				} else {
					str+=sprintf(str, " hw=?(duid_type=0x%x)", duid_type);
				}
			} else {
				str+=sprintf(str, " hw=?(hw_type=0x%x)", hw_type);
			}
			
		} else if (opt_type == D6O_IA_NA || opt_type == D6O_IA_TA) {	// 3,4   
			unsigned char *start2 = (unsigned char *)(o->opt_data + ((opt_type == D6O_IA_NA)?12:4));
			int len2 = sizeof(struct dhcpv6_opt_hdr) + opt_data_len - (start2 - (unsigned char *)o);
			str+=sprintf(str, ", %s:iaid=0x%x", dhcp6_opt_str(opt_type), be32_to_cpup((void *)o->opt_data));
			str+=sniffer_parser_ipv6_dhcp6_options(str, start2, len2);
		} else if (opt_type == D6O_IAADDR) {		// 5
			str+=sprintf(str, ", %s:%s", dhcp6_opt_str(opt_type), ipv6_addr_str(o->opt_data));
#if 0
		} else if (opt_type == D6O_ORO) {		// 6
			int i;
			str+=sprintf(str, ", %s:", dhcp6_opt_str(opt_type));
			for (i=0; i<opt_data_len/2; i++) {
				unsigned short optid = be16_to_cpup((void *)(o->opt_data+i*2));
				str+=sprintf(str, "%s%s", (i>0)?" ":"", dhcp6_opt_str(optid));
			}
		} else if (opt_type == D6O_PREFERENCE) {	// 7
			str+=sprintf(str, ", %s:%u", dhcp6_opt_str(opt_type), o->opt_data[0]);
		} else if (opt_type == D6O_INTERFACE_ID) {	// 18
			int i;
			str+=sprintf(str, ", %s:", dhcp6_opt_str(opt_type));
			for (i=0; i<opt_data_len; i++) {
				if (i>=16) {
					str+=sprintf(str, "...");
					break;
				}
				str+=sprintf(str, "%s%02x", (i>0)?" ":"", o->opt_data[i]);
			}
#endif
		} else if (opt_type == D6O_RECONFIG_MSG || opt_type == D6O_RECONFIG_ACCEPT) {	// 19/20
			str+=sprintf(str, ", %s", dhcp6_opt_str(opt_type));
		
		} else if (opt_type == D6O_RECCURSIVE_DNS_SERVER) {	// 23
			int i, n = opt_data_len/16;
			str+=sprintf(str, ", %s:", dhcp6_opt_str(opt_type));
			for (i=0; i<n && i<3; i++) {
				str+=sprintf(str, "%s%s", (i>0)?" ":"", ipv6_addr_str(o->opt_data));
			}
#if 0
		} else if (opt_type == D6O_DOMAIN_SEARCH) {	// 24
			str+=sprintf(str, ", %s:", dhcp6_opt_str(opt_type));
			str+=snprintf(str, (opt_data_len<32)?opt_data_len:32, "%s", o->opt_data);
#endif
		} else if (opt_type == D6O_IA_PD) {		// 25
			unsigned char *start2 = (void *)(o->opt_data + 16);
			int len2 = sizeof(struct dhcpv6_opt_hdr) + opt_data_len - (start2 - (unsigned char *)o);
			str+=sprintf(str, ", %s:iaid=0x%x", dhcp6_opt_str(opt_type), be32_to_cpup((void *)(o->opt_data)));
			str+=sniffer_parser_ipv6_dhcp6_options(str, start2, len2);
		} else if (opt_type == D6O_IAPREFIX) {		// 26
			str+=sprintf(str, ", %s:%s/%d", dhcp6_opt_str(opt_type), ipv6_addr_str(o->opt_data+9), o->opt_data[8]);
		}
		
		o=(struct dhcpv6_opt_hdr *)o_end;
	}
	return str - line;
}

static int
sniffer_parser_ipv6_dhcp6(char *line, void *dhcp6_start, int dhcp6_len)
{
	char *str = line;

	struct dhcpv6_relay_packet *dhcp6_relay=dhcp6_start;
	struct dhcpv6_packet *dhcp6;
	void *dhcp6opts_start;
	int dhcp6opts_len;

	// determine the start of dhcp6 msg
	if (dhcp6_relay->msg_type == DHCPV6_RELAY_FORW || dhcp6_relay->msg_type == DHCPV6_RELAY_REPL ) {// relay_forw, relay_reply
		struct dhcpv6_opt_hdr *o = (struct dhcpv6_opt_hdr *)dhcp6_relay->options;

		str+=sprintf(str, "\nDHCPv6 %s", dhcp6typestr[dhcp6_relay->msg_type&0xf]);
		str+=sprintf(str, ", link=%s", ipv6_addr_str(dhcp6_relay->link_address));
		str+=sprintf(str, ", peer=%s", ipv6_addr_str(dhcp6_relay->peer_address));

		if (ntohs(o->opt_type) != D6O_RELAY_MSG) {
			str+=sprintf(str, ", missing relayed msg?");
			return str - line;
		}
		// dhcp6 point to the relayed msg
		dhcp6 = (struct dhcpv6_packet *) o->opt_data;
		str+=sprintf(str, ", msg=%s", dhcp6typestr[dhcp6->msg_type&0xf]);
	} else {
		// dhcp6 point to the end of udp header
		dhcp6=dhcp6_start;
		str+=sprintf(str, "\nDHCPv6 %s", dhcp6typestr[dhcp6->msg_type&0xf]);
	}
	str+=sprintf(str, ", xid=%02x%02x%02x", dhcp6->transaction_id[0],  dhcp6->transaction_id[1], dhcp6->transaction_id[2]);

	dhcp6opts_start = (void *) dhcp6->options;
	dhcp6opts_len = dhcp6_len - (dhcp6opts_start - (void *)dhcp6_start);
	str+=sniffer_parser_ipv6_dhcp6_options(str, dhcp6opts_start, dhcp6opts_len);

	return str - line;
}

static int
sniffer_parser_ipv6_icmp6(char *line, void *icmp6_start, int icmp6_len)
{
	char *str = line;	
	struct icmp6hdr *icmp6=icmp6_start;
	str+=sprintf(str, " %s", icmp6_type_str(icmp6->icmp6_type));
	if (icmp6->icmp6_type == NDISC_ROUTER_SOLICITATION ||		// 133
	    icmp6->icmp6_type == NDISC_ROUTER_ADVERTISEMENT ||		// 134
	    icmp6->icmp6_type == NDISC_NEIGHBOUR_SOLICITATION ||	// 135
	    icmp6->icmp6_type == NDISC_NEIGHBOUR_ADVERTISEMENT ||	// 136
	    icmp6->icmp6_type == NDISC_REDIRECT) {			// 137
		// the 5 icmp msgs that have options support
		struct nd_opt_hdr *o;
	    	if (icmp6->icmp6_type == NDISC_ROUTER_SOLICITATION) {	// 133
			struct rs_msg *rs = (struct rs_msg *)icmp6;
			o=(struct nd_opt_hdr *)((void *)rs+sizeof(struct rs_msg));					
		} else if (icmp6->icmp6_type == NDISC_ROUTER_ADVERTISEMENT) {	// 134
			struct ra_msg *ra = (struct ra_msg *)icmp6;
			str+=sprintf(str, ", lifetime=%u", ntohs(icmp6->icmp6_dataun.u_nd_ra.rt_lifetime));			
			str+=sprintf(str, ", flags M=%d O=%d H=%d Prf=%d, ", 
				icmp6->icmp6_dataun.u_nd_ra.managed,
				icmp6->icmp6_dataun.u_nd_ra.other,
				icmp6->icmp6_dataun.u_nd_ra.home_agent,
				icmp6->icmp6_dataun.u_nd_ra.router_pref);
			o=(struct nd_opt_hdr *)((void *)ra+sizeof(struct ra_msg));					
		} else if (icmp6->icmp6_type == NDISC_NEIGHBOUR_SOLICITATION) {	// 135
			struct nd_msg *nd = (struct nd_msg *)icmp6;
			str+=sprintf(str, ", targetaddr=%s", ipv6_addr_str((void *)&(nd->target)));
			o=(struct nd_opt_hdr *)((void *)nd+sizeof(struct nd_msg));					
		} else if (icmp6->icmp6_type == NDISC_NEIGHBOUR_ADVERTISEMENT) {	// 136
			struct nd_msg *nd = (struct nd_msg *)icmp6;
			str+=sprintf(str, ", targetaddr=%s", ipv6_addr_str((void *)&(nd->target)));
			o=(struct nd_opt_hdr *)((void *)nd+sizeof(struct nd_msg));					
		} else if (icmp6->icmp6_type == NDISC_REDIRECT) {	// 137
			// redirect msg has one more dest addr than struct the nd_msg
			struct nd_msg *nd = (struct nd_msg *)icmp6;
			str+=sprintf(str, ", targetaddr=%s", ipv6_addr_str((void *)&(nd->target)));
			str+=sprintf(str, ", dstaddr=%s", ipv6_addr_str((void *)nd+sizeof(struct nd_msg)));
			o=(struct nd_opt_hdr *)((void *)nd+sizeof(struct nd_msg)+16);
		}

		while ((void*)o<(void *)icmp6+icmp6_len && (o->nd_opt_len<<3)>sizeof(struct nd_opt_hdr)) {
			unsigned char *m= (void *)o+2;
			void *o_end = (void *)o+(o->nd_opt_len<<3);

			if (o_end > (void *)icmp6+icmp6_len)
				break;
			if (o->nd_opt_type == ND_OPT_SOURCE_LL_ADDR) {	// 1
				str+=sprintf(str, " linkaddr=%s", ether_addr_str(m));
			} else if (o->nd_opt_type == ND_OPT_TARGET_LL_ADDR) {	// 2
				str+=sprintf(str, " targetaddr=%s", ether_addr_str(m));
			} else if (o->nd_opt_type == ND_OPT_PREFIX_INFO) {	// 3
				unsigned char prefix_len=m[0];
				unsigned char flag=m[1];
				unsigned int valid_lifetime=ntohl(*(unsigned int *)(m+2));
				unsigned int preferred_lifetime=ntohl(*(unsigned int *)(m+6));
				if (valid_lifetime == 0xffffffff)
					str+=sprintf(str, " valid_lifetime=forever");
				else
					str+=sprintf(str, " valid_lifetime=0x%x", valid_lifetime);
				if (preferred_lifetime == 0xffffffff)
					str+=sprintf(str, " prefer_lifetime=forever");
				else
					str+=sprintf(str, " prefer_lifetime=0x%x", valid_lifetime);
				str+=sprintf(str, " flags L=%d A=%d X=%d", (flag&0x80)?1:0, (flag&0x40)?1:0, (flag&0x20)?1:0);
				str+=sprintf(str, " prefix=%s/%d", ipv6_addr_str(m+14), prefix_len);
			} else if (o->nd_opt_type == ND_OPT_MTU) {	// 5
				str+=sprintf(str, " mtu=%u", be32_to_cpup((void *)m+2));
			}

			o=o_end;
		}

	} else if (icmp6->icmp6_type == ICMPV6_MGM_REPORT) {	// 131
		str+=sprintf(str, ", grp=%s", ipv6_addr_str((void *)icmp6+sizeof(struct icmp6hdr)));
		
	} else if (icmp6->icmp6_type == ICMPV6_MLD2_REPORT) {	// 143
		static char *grectypestr[8]= { 
			"0", "include", "exclude", "chg_to_include", 
			"chg_to_exclude", "allow_new_src", "blk_old_src", "7" };
		struct mld2_report *mld2 = (struct mld2_report *)icmp6;
		struct mld2_grec *grec = &mld2->mld2r_grec[0];
		unsigned short gtotal = ntohs(mld2->mld2r_ngrec);	// number of group
		int i, j;
		// dump first 3 groups, and each group dump first 3 srcip
		for (i=0; i< gtotal; i++) {
			unsigned short stotal = ntohs(grec->grec_nsrcs);	// number of src
			str+=sprintf(str, ", %s grp=%s", grectypestr[grec->grec_type&0x7], ipv6_addr_str((void *)&(grec->grec_mca)));
			for (j=0; j<stotal; j++) {
				if (j==0) {
					str+=sprintf(str, " %usrc=%s", stotal, ipv6_addr_str((void *)&(grec->grec_src[j])));
				} else if (j<=2) {
					str+=sprintf(str, " %s", ipv6_addr_str((void *)&(grec->grec_src[j])));
				} else {
					str+=sprintf(str, "...");
					break;
				}
			}
			if (i==2 && gtotal>i) {
				str+=sprintf(str, ", total %ugrp...\n", gtotal);
				break;
			}
			grec = (void*)grec + sizeof(struct mld2_grec) + stotal * sizeof(struct in6_addr);
		}
	} else if (icmp6->icmp6_type == ICMPV6_MGM_QUERY) {	// 130
		struct mld2_query *mld2q = (struct mld2_query*)icmp6;

		if ( !is_zero_ipv6_addr(&mld2q->mld2q_mca) ) {
			unsigned short stotal=ntohs(mld2q->mld2q_nsrcs);	// number of src
			int i;
			str+=sprintf(str, "(specific), grp=%s", ipv6_addr_str((void *)&(mld2q->mld2q_mca)));
			// dump first 3 srcip
			for (i=0; i< stotal; i++) {
				if (i==0) {
					str+=sprintf(str, " %usrc=%s", stotal, ipv6_addr_str((void *)&(mld2q->mld2q_srcs[i])));
				} else if (i<=2) {
					str+=sprintf(str, " %s", ipv6_addr_str((void *)&(mld2q->mld2q_srcs[i])));
				} else {
					str+=sprintf(str, "...");
					break;
				}
			}
		} else {
			str+=sprintf(str, "(generic), grp=%s", ipv6_addr_str((void *)&(mld2q->mld2q_mca)));
		}

	} else if (icmp6->icmp6_type == ICMPV6_DEST_UNREACH) {	// 1
		struct ipv6hdr *ip62=(void *)icmp6+sizeof(struct icmp6hdr);
		struct tcphdr *tcp2=(void *)ip62+sizeof(struct ipv6hdr);
		struct udphdr *udp2=(void *)ip62+sizeof(struct ipv6hdr);

		str+=sprintf(str, ", %s, msg=[0x%02x]%s [%s]",
			icmp6_unreach_code_str(icmp6->icmp6_code),
			ip62->nexthdr, ipv6_nexthdr_str(ip62->nexthdr),
			ipv6_addr_str((unsigned char *)&(ip62->saddr)));
		if (ip62->nexthdr==NEXTHDR_TCP) {
			str+=sprintf(str, ":%d", ntohs(tcp2->source));
		} else if (ip62->nexthdr==NEXTHDR_UDP) {
			str+=sprintf(str, ":%d", ntohs(udp2->source));
		}
		str+=sprintf(str, " > [%s]", ipv6_addr_str((unsigned char*)&(ip62->daddr)));
		if (ip62->nexthdr==NEXTHDR_TCP) {
			str+=sprintf(str, ":%d", ntohs(tcp2->dest));
		} else if (ip62->nexthdr==NEXTHDR_UDP) {
			str+=sprintf(str, ":%d", ntohs(udp2->dest));
		}
	}
	return str - line;
}


static int
sniffer_parser_ipv6(char *line, void *ip6_start, int ip6_len)
{
	char *str = line;
	struct ipv6hdr *ip6 = ip6_start;
	unsigned char *s=(unsigned char*)&(ip6->saddr);
	unsigned char *d=(unsigned char*)&(ip6->daddr);

	int is_ipv6_printed = 0;

	unsigned char nexthdr = ip6 ->nexthdr; 
	struct ipv6_exthdr *e = (void *)ip6+sizeof(struct ipv6hdr);	// exthdr is valid only if not payload(tcp/udp/icmp/ospfv3) after ipv6 hdr 
	void *exthdr_end = (void *)e + 8 + (e->optlen <<3);
	int tclass = (ntohl(*(__be32 *)ip6_start) >> 20) & 0xff;

	if (filter_ip_proto && filter_ip_proto!=ip6->nexthdr)
		return -1;

	while (nexthdr != NEXTHDR_NONE) {
		
		if (nexthdr==NEXTHDR_TCP || nexthdr==NEXTHDR_UDP) {
			struct tcphdr *tcp = (void *)e;
			struct udphdr *udp = (void *)e;
			unsigned short sp=0, dp=0;
	
			if  (nexthdr==NEXTHDR_TCP) {
				sp=ntohs(tcp->source);
				dp=ntohs(tcp->dest);
			} else {
				sp=ntohs(udp->source);
				dp=ntohs(udp->dest);
			}
			if (filter_port && filter_port!=sp && filter_port!=dp)
				return -1;
				
			if (!is_ipv6_printed) {
				is_ipv6_printed = 1;
				str+=sprintf(str, " dscp=0x%x [0x%02x]%s [%s]:%d > ",
					tclass&0x3f, nexthdr, ipv6_nexthdr_str(nexthdr), ipv6_addr_str(s), sp);
				str+=sprintf(str, "[%s]:%d", ipv6_addr_str(d), dp);
			} else { 
				str+=sprintf(str, "\n[0x%02x]%s %d > %d",
					nexthdr, ipv6_nexthdr_str(nexthdr), sp,  dp);
			}
	
			if (nexthdr==NEXTHDR_TCP) {
				str+=sprintf(str, " (%s)", tcp_flag_str(((char*)tcp)[13]));
			} else {	// udp
				if ((sp==546 && dp==547) || (sp==547 && dp==546)) {
					int dhcp6_len = (void*)ip6+ ip6_len - ((void *)udp+sizeof(struct udphdr));
					int tmp_len = sniffer_parser_ipv6_dhcp6(str, (void *)udp+sizeof(struct udphdr), dhcp6_len);
					if (tmp_len <0)
						return -1;
					str+=tmp_len;
				}
			}
	
			return str -line;
		}

		if (!is_ipv6_printed) {
			is_ipv6_printed = 1;
			str+=sprintf(str, " dscp=0x%x [%s] > ", tclass&0x3f, ipv6_addr_str(s));
			str+=sprintf(str, "[%s]", ipv6_addr_str(d));
		}
		
		str+=sprintf(str, "\n[0x%02x]%s", nexthdr, ipv6_nexthdr_str(nexthdr));

		if (nexthdr == NEXTHDR_ICMP) {
  			int tmp_len;
  			if (filter_ip_proto && filter_ip_proto!=nexthdr)
  				return -1;
			if (filter_port)	// once port is specified, assume tcp/udp
				return -1;
  			if ((tmp_len = sniffer_parser_ipv6_icmp6(str, (void *)e, ip6_start + ip6_len - (void *)e)) <0)
				return -1;
			str+=tmp_len;
			return str -line;
		} else if (nexthdr == NEXTHDR_OSPFV3) {
			return str -line;
		} else if (nexthdr == NEXTHDR_FRAGMENT) { 
			unsigned short frag_offset = be16_to_cpup((void *)e->data);
			unsigned int frag_id = be32_to_cpup((void *)(e->data)+2);
			str+=sprintf(str, " id=0x%x, offset=%u%s", frag_id, frag_offset & 0x1fff, (frag_offset & 0x8000)?", More":"");
		}

		if (exthdr_end > ip6_start + ip6_len)
		{
			break;
		} else {
			nexthdr = e->nexthdr;		
			e = exthdr_end;
			//check next extend header boundary
			if (((void *)e + 8) > ip6_start + ip6_len)
			{
				break; //process end
			} else {
				exthdr_end = (void *)e + 8 + (e->optlen <<3);
			}
		}

	}
	return str - line;
}

static int
sniffer_parser_ether(char *line, unsigned char *pkt, int pkt_len)
{
	char *str=line;
	int is_pppoe_pkt = 0;

	unsigned short eth_proto=0;
	char *eth_proto_ptr, *nh;

	if (*(unsigned int*)filter_mac_addr || *(unsigned short*)(filter_mac_addr+4)) {
		if (memcmp(filter_mac_addr, pkt, 6)!=0 && memcmp(filter_mac_addr, pkt+6, 6)!=0)
			return -1;
	}

	eth_proto_ptr=pkt+12;
	eth_proto=be16_to_cpup((void *)eth_proto_ptr);

	if ((eth_proto == ETH_P_8021Q || eth_proto == 0x88a8 || eth_proto ==0x9100) ||	// outer tag
	    ((eth_proto&0xff00)==ETH_P_8021Q && eth_proto!=ETH_P_IPX) ) {		// 81xx but not 8137, for icplus 175d with source port in 81xx
		unsigned short tci=be16_to_cpup((void *)eth_proto_ptr+2);
	    	eth_proto_ptr+=4;
	    	eth_proto=be16_to_cpup((void *)eth_proto_ptr);
		if (eth_proto == ETH_P_8021Q || eth_proto == 0x88a8 || eth_proto ==0x9100) {	// inner tag
			unsigned short tci2=be16_to_cpup((void *)eth_proto_ptr+2);
			if (filter_vid && filter_vid != (tci&0xfff) && filter_vid != (tci2&0xfff))
				return -1;
			str+=sprintf(str, "ovlan %d p%d ", tci & 0xfff, (tci&0xe000)>>13);
			str+=sprintf(str, "ivlan %d p%d ", tci2& 0xfff, (tci2&0xe000)>>13);
			eth_proto_ptr+=4;
		    	eth_proto=be16_to_cpup((void *)eth_proto_ptr);
		} else {
			if (filter_vid && filter_vid != (tci&0xfff))
				return -1;
			str+=sprintf(str, "vlan %d p%d ", tci & 0xfff, (tci&0xe000)>>13);
		}
	} else {
		if (filter_vid)
			return -1;
	}

	if (eth_proto==ETH_P_PPP_DISC || eth_proto==ETH_P_PPP_SES) {
		struct pppoe_hdr *pppoe = (void *)eth_proto_ptr + 2;
		str+=sprintf(str, "[0x%04x]%s ", eth_proto, eth_proto_str(eth_proto));
		if (pppoe->code >0)
			str+=sprintf(str, "%s ", pppoe_code_str(pppoe->code));
		str+=sprintf(str, "id=%x ",  ntohs(pppoe->sid));
		if (eth_proto==ETH_P_PPP_SES) {
			unsigned short pppoe_proto = ntohs(pppoe->tag[0].tag_type);
			str+=sprintf(str, "subp=[0x%04x]%s ", pppoe_proto, ppp_proto_str(pppoe_proto));
			is_pppoe_pkt = 1;
			if (pppoe_proto == PPP_IP) {
				eth_proto=ETH_P_IP;	// fake eth_proto and eth_proto_ptr for later processing
				eth_proto_ptr+=8;
			} else if (pppoe_proto == PPP_IPV6) {
				eth_proto=ETH_P_IPV6;	// fake eth_proto and eth_proto_ptr for later processing
				eth_proto_ptr+=8;
			} else {
				if (filter_eth_proto && filter_eth_proto!=eth_proto)
					return -1;
				if (filter_ip_proto ||filter_port ||filter_ip_addr[0] ||filter_ip_addr[1] ||filter_ip_addr[2] ||filter_ip_addr[3]) 
					return -1;
				str+=sprintf(str, "%s > ", ether_addr_str(pkt+6));
				str+=sprintf(str, "%s", ether_addr_str(pkt));
				return str - line;
			}
			if (filter_eth_proto && filter_eth_proto!= ETH_P_PPP_SES && filter_eth_proto!=eth_proto)
				return -1;
		} else {	
			if (filter_eth_proto && filter_eth_proto!=eth_proto)
				return -1;
			if (filter_ip_proto ||filter_port ||filter_ip_addr[0] ||filter_ip_addr[1] ||filter_ip_addr[2] ||filter_ip_addr[3]) 
				return -1;
			str+=sprintf(str, "%s > ", ether_addr_str(pkt+6));
			str+=sprintf(str, "%s", ether_addr_str(pkt));
			return str - line;
		}
	}
			
	nh=eth_proto_ptr+2;

	if (filter_eth_proto && 
	    filter_eth_proto!=ETH_P_PPP_DISC && filter_eth_proto!=ETH_P_PPP_SES && 
	    filter_eth_proto!=eth_proto)
		return -1;
	if (filter_ip_proto ||filter_port) {	// once ip_proto or tcp/dup port is specified, assume ipv4/ipv6
		if (eth_proto != ETH_P_IP && eth_proto != ETH_P_IPV6)
			return -1;
	}
	if (filter_ip_addr[0] ||filter_ip_addr[1] ||filter_ip_addr[2] ||filter_ip_addr[3]) {	// once ipaddr specified, assue ipv4/ipv6/arp
		if (eth_proto != ETH_P_IP && eth_proto != ETH_P_IPV6 && eth_proto != ETH_P_ARP)
			return -1;
	}	

	if (eth_proto==ETH_P_IP) {	// IP
		int tmp_len;
		if (!is_pppoe_pkt)
			str+=sprintf(str, "[0x%04x]%s", eth_proto, eth_proto_str(eth_proto));
		tmp_len = sniffer_parser_ipv4(str, nh, pkt_len-((void*)nh-(void*)pkt));
		if (tmp_len < 0)
			return -1;
		str += tmp_len;

	} else if (eth_proto==ETH_P_IPV6) {	// IPV6
		int tmp_len;
		if (!is_pppoe_pkt)
			str+=sprintf(str, "[0x%04x]%s", eth_proto, eth_proto_str(eth_proto));		
		tmp_len = sniffer_parser_ipv6(str, nh, pkt_len-((void*)nh-(void*)pkt));
		if (tmp_len < 0)
			return -1;
		str += tmp_len;

	} else if (eth_proto==ETH_P_ARP) {	// ARP
		struct arphdr *arp=(struct arphdr*)(nh);
		unsigned char *smac=(void *)arp + sizeof(struct arphdr);
		unsigned char *s=smac+6;
		unsigned char *dmac=s+4;
		unsigned char *d=dmac+6;
		unsigned short op=ntohs(arp->ar_op);

		if (*(int*)filter_ip_addr && *(int*)filter_ip_addr!=*(int*)s && *(int*)filter_ip_addr!=*(int*)d)
			return -1;
		str+=sprintf(str, "[0x%04x]%s ", eth_proto, eth_proto_str(eth_proto));
		if (op==ARPOP_REQUEST) {
			if (memcmp(s, d, 4)==0) {
				str+=sprintf(str, "Gratuitous ARP for %d.%d.%d.%d",
					s[0],s[1],s[2],s[3]);
			} else {
				str+=sprintf(str, "Who has %d.%d.%d.%d? tell %d.%d.%d.%d",
					d[0],d[1],d[2],d[3], s[0],s[1],s[2],s[3]);
			}
			str+=sprintf(str, "\nop=request");
		} else if (op==ARPOP_REPLY) {
			str+=sprintf(str, "%d.%d.%d.%d is %s", s[0],s[1],s[2],s[3], ether_addr_str(smac));
			str+=sprintf(str, "\nop=reply");
		} else {
			str+=sprintf(str, "\nop=0x%x", op);
		}
		str+=sprintf(str, " s=%s, sip=%d.%d.%d.%d,", ether_addr_str(smac), s[0],s[1],s[2],s[3]);
		str+=sprintf(str, " t=%s, tip=%d.%d.%d.%d", ether_addr_str(dmac), d[0],d[1],d[2],d[3]);

	} else if (eth_proto==0x888e) {	// EAPOL 0x888e
		struct eapol_hdr_t *eapol=(struct eapol_hdr_t *)nh;
		str+=sprintf(str, "[0x%04x]%s ", eth_proto, eth_proto_str(eth_proto));
		str+=sprintf(str, "version=%d, type=%d(%s)", eapol->version, eapol->type, eapol_type_str(eapol->type));
		if (eapol->length >=4) {
			str+=sprintf(str, "; id=%d, code=%d(%s), len=%d", eapol->id, eapol->code, eapol_code_str(eapol->code), eapol->length_payload);
			if ((eapol->code == EAPOL_CODE_REQUEST || eapol->code == EAPOL_CODE_RESPONSE) && eapol->length >=5) {
				str+=sprintf(str, "; %stype=%d(%s)", (eapol->code == EAPOL_CODE_REQUEST)?"req":"res", 
					eapol->reqres_type, eapol_reqres_type_str(eapol->reqres_type));
				if (eapol->code == EAPOL_CODE_RESPONSE) {
					if (eapol->reqres_type == EAPOL_REQRES_TYPE_ID) {
						int i, id_len = eapol->length_payload - 5;
						if (id_len > 31)
							id_len=31;
						str+=sprintf(str, "\nid_len=%d, id=", id_len);
						for (i=0; i<id_len; i++)
							str+=sprintf(str, "%c", eapol->data[i]);
					} else if (eapol->reqres_type == EAPOL_REQRES_TYPE_MD5) {
						int i, md5_len = eapol->data[0];
						str+=sprintf(str, "\nmd5_len=%d, md5=0x", md5_len);
						for (i=0; i<md5_len; i++)
							str+=sprintf(str, "%02x", eapol->data[1+i]);
					} else if (eapol->reqres_type == EAPOL_REQRES_TYPE_TLS) {
						int tls_flag = eapol->data[0];
						str+=sprintf(str, "\nlen_included=%d, more_frag=%d", (tls_flag & 0x80)?1:0, (tls_flag & 0x40)?1:0);
						if (tls_flag & 0x80) {
							int tls_len = eapol->data[1]<<8 | eapol->data[2];
							int tls_ver = eapol->data[4]<<8 | eapol->data[5];
							char *tls_ver_str="TLS?";
							if (tls_ver == 0x303)
								tls_ver_str="TLS1.2";
							else if (tls_ver == 0x302)
								tls_ver_str="TLS1.1";
							else if (tls_ver == 0x301)
								tls_ver_str="TLS1.0";
							str+=sprintf(str, ", tls_len=%d, %s", tls_len, tls_ver_str);
						}
					}
				}
			}
		}
	
	} else if (eth_proto==CFM_ETHERTYPE) {	// CFM
		cfm_hdr_t *cfm=(cfm_hdr_t*)(nh);
		unsigned char *tlv_start=(unsigned char *)(nh+sizeof(cfm_hdr_t)+cfm->first_tlv_offset);
		int tlv_total_len = (unsigned char*)pkt + pkt_len - tlv_start;
		char *tlv_str = cfm_print_get_tlv_str(tlv_start, tlv_total_len);
		
		str+=sprintf(str, "[0x%04x]%s ", eth_proto, eth_proto_str(eth_proto));
		str+=sprintf(str, "mdl=%d, %s(%d), flag=0x%02x", cfm->md_level, cfm_print_get_opcode_str(cfm->opcode), cfm->opcode, cfm->flags);
		switch (cfm->opcode) {
			case CFM_PDU_CCM:
			{
				cfm_pdu_ccm_t *ccm=(cfm_pdu_ccm_t *)(nh+sizeof(cfm_hdr_t));
				str+=sprintf(str, " rdi=%d, period=%d, seq=0x%x, mepid=0x%x, ma=%s", 
					(cfm->flags>>7)&1, cfm->flags&7,
					be32_to_cpup(&ccm->seq), be16_to_cpup(&ccm->mep_id), cfm_print_get_maid_str(ccm->ma_id));
			}
			break;		
			case CFM_PDU_LBR:
			case CFM_PDU_LBM:	// same format as lbr
			{
				cfm_pdu_lbr_t *lbr=(cfm_pdu_lbr_t *)(nh+sizeof(cfm_hdr_t));
				str+=sprintf(str, " seq=0x%x", be32_to_cpup(&lbr->xid));
			}
			break;		
			case CFM_PDU_LTR:
			{
				cfm_pdu_ltr_t *ltr=(cfm_pdu_ltr_t *)(nh+sizeof(cfm_hdr_t));
				str+=sprintf(str, " hw=%d, fwd=%d, by_mep=%d, xid=0x%x, ttl=%d, relay=%d", 
					(cfm->flags>>7)&1, (cfm->flags>>6)&1, (cfm->flags>>5)&1, 
					be32_to_cpup(&ltr->xid), ltr->ttl, ltr->relay_action);
			}
			break;		
			case CFM_PDU_LTM:
			{
				cfm_pdu_ltm_t *ltm=(cfm_pdu_ltm_t *)(nh+sizeof(cfm_hdr_t));
				str+=sprintf(str, " hw=%d, xid=0x%x, ttl=%d, ", (cfm->flags>>7)&1, be32_to_cpup(&ltm->xid), ltm->ttl);
				str+=sprintf(str, "o=%s,", ether_addr_str(ltm->origin_addr));
				str+=sprintf(str, "t=%s", ether_addr_str(ltm->target_addr));
			}
			break;		
			case CFM_PDU_AIS:
			case CFM_PDU_LCK:
			{
				str+=sprintf(str, " period=0x%x(1%s)", cfm->flags&7, ((cfm->flags&7)==4)?"second":"minute");
			}
		}
		if (strlen(tlv_str))
			str+=sprintf(str, "\nTLV:%s", tlv_str);		

	} else {	// else
		char *desc=NULL;
		int i;
		for (i=0; mac_desc[i].desc!=NULL; i++) {	// chk special mac
			if (memcmp(mac_desc[i].mac, pkt, 6)==0) {
				desc=mac_desc[i].desc;
				break;
			}
		}
		if (desc==NULL) {				// chk IPX
			if (memcmp(nh, ipx_ethernet_8023, 2)==0)
				desc="IPX Ethernet 802.3";
			else if (memcmp(nh, ipx_ethernet_8022, 3)==0)
				desc="IPX Ethernet 802.2";
			else if (memcmp(nh, ipx_ethernet_snap, 8)==0)
				desc="IPX Ethernet SNAP";
		}
		if (desc) {
			str+=sprintf(str, "[-] %s pkt", desc);
		} else {
			str+=sprintf(str, "[0x%04x]%s", eth_proto, eth_proto_str(eth_proto));
		}
		str+=sprintf(str, " %s > ", ether_addr_str(pkt+6));
		str+=sprintf(str, "%s", ether_addr_str(pkt));
	}
	return str - line;
}

// cpuport_sniffer wrapper for callers ///////////////////////////////////////////////////////////////

int
cpuport_sniffer_config_set(int fd, int argc, char *argv[])
{
	if (argc>=1) {
		if (strcmp(argv[0], "level")==0 && argc==2) {
			int i=util_atoi(argv[1]);
			if (i>=0 && i<=3)
				sniffer_level=i;
			return 0;
		} else if (strcmp(argv[0], "mac_addr")==0 && argc==2) {
			unsigned int addr[6];
			if (sscanf(argv[1], "%x:%x:%x:%x:%x:%x", addr,addr+1,addr+2,addr+3, addr+4, addr+5)==6) {
				filter_mac_addr[0]=addr[0];
				filter_mac_addr[1]=addr[1];
				filter_mac_addr[2]=addr[2];
				filter_mac_addr[3]=addr[3];
				filter_mac_addr[4]=addr[4];
				filter_mac_addr[5]=addr[5];
				return 0;
			}
		} else if (strcmp(argv[0], "vid")==0 && argc==2) {
			filter_vid=util_atoi(argv[1]);
			return 0;
		} else if (strcmp(argv[0], "eth_proto")==0 && argc==2) {
			filter_eth_proto=util_atoi(argv[1]);
			return 0;
		} else if (strcmp(argv[0], "ip_addr")==0 && argc==2) {
			int addr[4];
			if (sscanf(argv[1], "%d.%d.%d.%d", addr,addr+1,addr+2,addr+3)==4) {
				if (addr[0]>=0 && addr[0]<=255 &&
				    addr[1]>=0 && addr[1]<=255 &&
				    addr[2]>=0 && addr[2]<=255 &&
				    addr[3]>=0 && addr[3]<=255) {
				    	filter_ip_addr[0]=addr[0];
				    	filter_ip_addr[1]=addr[1];
				    	filter_ip_addr[2]=addr[2];
				    	filter_ip_addr[3]=addr[3];
				    	return 0;
				}
			}
		} else if (strcmp(argv[0], "ip_proto")==0 && argc==2) {
			filter_ip_proto=util_atoi(argv[1]);
			return 0;
		} else if (strcmp(argv[0], "port")==0 && argc==2) {
			filter_port=util_atoi(argv[1]);
			return 0;
		} else if (strcmp(argv[0], "sip_debug")==0 && argc==2) {
			sip_debug=util_atoi(argv[1]);
			return 1;
		} else if (strcmp(argv[0], "reset")==0 && argc==1) {
			sniffer_level=0;
			filter_vid=filter_eth_proto=filter_ip_proto=filter_port=0;
			*(unsigned int*)filter_ip_addr=0;
			*(unsigned int*)filter_mac_addr=0;
			*(unsigned short*)(filter_mac_addr+4)=0;
			return 0;
		}
	}

	util_fdprintf(fd, "sniffer subcmd:\n"
		"level [level]\n"
		"mac_addr [mac_addr]\n"
		"vid [vid]\n"
		"eth_proto [eth_proto]\n"
		"ip_proto [ip_proto]\n"
		"ip_addr [ip_addr]\n"
		"port [port]\n"
		"sip_debug [0|1]\n"
		"reset\n"
	);
	return -1;
}

void
cpuport_sniffer_config_get(int fd)
{
	util_fdprintf(fd, "level=%d\n", sniffer_level);
	util_fdprintf(fd, "mac_addr=%02x:%02x:%02x:%02x:%02x:%02x\n",
		filter_mac_addr[0],filter_mac_addr[1],filter_mac_addr[2],
		filter_mac_addr[3],filter_mac_addr[4],filter_mac_addr[5]);
	util_fdprintf(fd, "vid=%d\n", filter_vid);
	util_fdprintf(fd, "eth_proto=0x%x\n", filter_eth_proto);
	util_fdprintf(fd, "ip_addr=%u.%u.%u.%u\n",
		filter_ip_addr[0],filter_ip_addr[1],filter_ip_addr[2],filter_ip_addr[3]);
	util_fdprintf(fd, "ip_proto=0x%x\n", filter_ip_proto);
	util_fdprintf(fd, "port=%d\n", filter_port);
}

static inline void
dbprint_mem(int debug_level, unsigned char *pkt, int pkt_len)
{
	// use same size of buffer as underlying util_dbprintf
	// each memdump line (16byte hex) needs 54 byte buffer for print, thus 2048 could cover 592 byte hex

	char *line = malloc_safe(2048);
	char *s;
	int i;

	s = line;
	for (i = 0; i < pkt_len; i++) {	
		if (i % 16 == 0) s += sprintf(s, "%04x", i);
		s += sprintf(s, " %02x", *pkt); pkt++;
		if (i % 8 == 7) s += sprintf(s, " ");
		if (i % 16 == 15) s += sprintf(s, "\n");
		
		if (i %512 == 511) {	// flush line to util_dbprinf for per 512hex
			util_dbprintf(omci_env_g.debug_level_cpuport, debug_level, 0, "%s", line);	
			s = line; line[0] = 0;
		}
	}
	if (i % 16 != 0) s += sprintf(s, "\n");
	util_dbprintf(omci_env_g.debug_level_cpuport, debug_level, 0, "%s", line);
	free_safe(line);
}

static inline void
fdprint_mem(int fd, unsigned char *pkt, int pkt_len)
{
	// use same size of buffer as underlying util_dbprintf
	// each memdump line (16byte hex) needs 54 byte buffer for print, thus 2048 could cover 592 byte hex

	char *line = malloc_safe(2048);
	char *s;
	int i;

	s = line;
	for (i = 0; i < pkt_len; i++) {	
		if (i % 16 == 0) s += sprintf(s, "%04x", i);
		s += sprintf(s, " %02x", *pkt); pkt++;
		if (i % 8 == 7) s += sprintf(s, " ");
		if (i % 16 == 15) s += sprintf(s, "\n");
		
		if (i %512 == 511) {	// flush line to util_dbprinf for per 512hex
			util_fdprintf(fd, "%s", line);	
			s = line; line[0] = 0;
		}
	}
	if (i % 16 != 0) s += sprintf(s, "\n");
	util_fdprintf(fd, "%s", line);
	free_safe(line);
}

// we use 4096 as normally pkt is 1518, with some additional desc, this should be enough
void
cpuport_sniffer_print_pkthdr(int fd, char *prompt, unsigned char *pkt, int pkt_len, int is_ip)
{
	char *line = malloc_safe(4096);
	int ret = 0;
	
	if (is_ip) {
		if ((pkt[0]&0xf0) == 0x40)	// ipv4
			ret = sniffer_parser_ipv4(line, pkt, pkt_len);
		else	// ipv6
			ret = sniffer_parser_ipv6(line, pkt, pkt_len);
	} else {	
		ret = sniffer_parser_ether(line, pkt, pkt_len);
	}
	if (ret >0)
		util_fdprintf(fd, "%s %p len %d, %s\n", prompt, pkt, pkt_len, line);
		
	free_safe(line);
}

void
cpuport_sniffer_print_pkt(int fd, char *prompt, unsigned char *pkt, int pkt_len, int is_ip)
{
	char *line = malloc_safe(4096);
	int ret = 0;
	
	if (is_ip) {
		if ((pkt[0]&0xf0) == 0x40)	// ipv4
			ret = sniffer_parser_ipv4(line, pkt, pkt_len);
		else	// ipv6
			ret = sniffer_parser_ipv6(line, pkt, pkt_len);
	} else {	
		ret = sniffer_parser_ether(line, pkt, pkt_len);
	}
	if (ret >0) {
		util_fdprintf(fd, "%s %p len %d, %s\n", prompt, pkt, pkt_len, line);
		if (sniffer_level>=2)
			fdprint_mem(fd, pkt, (pkt_len<64||sniffer_level==3)?pkt_len:64);
	}
	free_safe(line);
}

void
cpuport_sniffer_dbprint_pkt(int debug_level, char *prompt, unsigned char *pkt, int pkt_len, int is_ip)
{
	char *line;
	int ret = 0;

	if (debug_level > omci_env_g.debug_level_cpuport || sniffer_level==0)
		return;

	line = malloc_safe(4096);
	
	if (is_ip) {
		if ((pkt[0]&0xf0) == 0x40)	// ipv4
			ret = sniffer_parser_ipv4(line, pkt, pkt_len);
		else	// ipv6
			ret = sniffer_parser_ipv6(line, pkt, pkt_len);
	} else {	
		ret = sniffer_parser_ether(line, pkt, pkt_len);
	}
	if (ret >0) {
#if 0
		if (sniffer_level>=2)
			util_dbprintf(omci_env_g.debug_level_cpuport, debug_level, 0, "\n");
#endif
		util_dbprintf(omci_env_g.debug_level_cpuport, debug_level, 0, "%s %p len %d, %s\n", prompt, pkt, pkt_len, line);
		if (sniffer_level>=2)
			dbprint_mem(debug_level, pkt, (pkt_len<64||sniffer_level==3)?pkt_len:64);
	}
	free_safe(line);
}
