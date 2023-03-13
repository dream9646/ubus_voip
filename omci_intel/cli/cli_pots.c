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
 * Module  : cli
 * File    : cli_pots.c
 *
 ******************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <dirent.h>
#include <fcntl.h>

#include "util.h"
#include "meinfo.h"
#include "cli.h"
#include "me_related.h"

static char * get_dialplan_format_str(int number) {
	static char *format_str[4]={ "undefined", "h.248", "nsc", "vendor-specific" };
	if (number>=0 && number<4)
		return format_str[number];
	return format_str[0];
}

static int
cli_sip_user_related_dialplan(int fd, struct me_t *me_sip_user)
{
	unsigned char get_attr_mask[2] = {0xff, 0xff};
	struct meinfo_t *miptr=meinfo_util_miptr(145);
	struct me_t *me;

	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		if (me_related(me_sip_user, me)) {
			meinfo_me_refresh(me, get_attr_mask);
			util_fdprintf(fd, "\n\t\t>[145]dialplan:0x%x", me->meid);
			util_fdprintf(fd, ",num=%d", (unsigned short)meinfo_util_me_attr_data(me, 1));
			util_fdprintf(fd, ",format=%d(%s)", 
				(unsigned char)meinfo_util_me_attr_data(me, 5),
				get_dialplan_format_str((unsigned char)meinfo_util_me_attr_data(me, 5)));
		}
	}
	return CLI_OK;
}

static int
cli_sip_user_related_voip_app_service_profile(int fd, struct me_t *me_sipuser)
{
	unsigned char get_attr_mask[2] = {0xff, 0xff};
	struct meinfo_t *miptr=meinfo_util_miptr(146);
	struct me_t *me;

	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		if (me_related(me_sipuser, me)) {
			meinfo_me_refresh(me, get_attr_mask);
			util_fdprintf(fd, "\n\t\t>[146]voip_app_service_profile:0x%x", me->meid);
			util_fdprintf(fd, ",cid=0x%x", *(unsigned char*)meinfo_util_me_attr_ptr(me, 1));
			util_fdprintf(fd, ",call_wait=0x%x", *(unsigned char*)meinfo_util_me_attr_ptr(me, 2));
			util_fdprintf(fd, ",call_proc_xfer=0x%x", ntohs(*(unsigned short*)meinfo_util_me_attr_ptr(me, 3)));
		}
	}
	return CLI_OK;
}

static int
cli_sip_user_related_voip_feature_access_codes(int fd, struct me_t *me_sipuser)
{
	struct meinfo_t *miptr=meinfo_util_miptr(147);
	struct me_t *me;

	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		if (me_related(me_sipuser, me)) {
			util_fdprintf(fd, "\n\t\t>[147]voip_feature_access_codes:0x%x", me->meid);
		}
	}
	return CLI_OK;
}

static int
cli_tcpudp_related_iphost(int fd, struct me_t *me_tcpudp)
{
	unsigned char get_attr_mask[2] = {0xff, 0xff};
	struct meinfo_t *miptr=meinfo_util_miptr(134);
	struct me_t *me;

	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		if (me_related(me_tcpudp, me)) {
			char *devname = meinfo_util_get_config_devname(me);
			struct in_addr in_static;
			struct in_addr in_dhcp;

			meinfo_me_refresh(me, get_attr_mask);
			in_static.s_addr=htonl(meinfo_util_me_attr_data(me, 4));
			in_dhcp.s_addr=htonl(meinfo_util_me_attr_data(me, 9));

			util_fdprintf(fd, "\n\t\t\t\t>[134]iphost:0x%x", me->meid);
			util_fdprintf(fd, ",dev=%s", devname?devname:"");
			util_fdprintf(fd, ",static_ip=%s", inet_ntoa(in_static));
			util_fdprintf(fd, ",dhcp_ip=%s", inet_ntoa(in_dhcp));
		}
	}
	return CLI_OK;
}

static int
cli_sip_agent_related_registar_netaddr(int fd, struct me_t *me_sip_agent)
{
	unsigned char get_attr_mask[2] = {0xff, 0xff};
	struct me_t *me=meinfo_me_get_by_meid(137, meinfo_util_me_attr_data(me_sip_agent, 5));

	if (me) {
		struct me_t *me_registar_auth=meinfo_me_get_by_meid(148, meinfo_util_me_attr_data(me, 1));
		struct me_t *me_registar_addr=meinfo_me_get_by_meid(157, meinfo_util_me_attr_data(me, 2));

		meinfo_me_refresh(me, get_attr_mask);
		util_fdprintf(fd, "\n\t\t\t>[137]netaddr(registar):0x%x", me->meid);
		if (me_registar_addr) {
			char *registar_addr=meinfo_util_me_attr_ptr(me_registar_addr, 2);
			if (registar_addr)
				util_fdprintf(fd, ",addr=%s", registar_addr);
		}
		if (me_registar_auth) {
			char *user = meinfo_util_me_attr_ptr(me_registar_auth, 2);
			char *pass = meinfo_util_me_attr_ptr(me_registar_auth, 3);
			util_fdprintf(fd, "\n\t\t\t\t>[148]auth:0x%x,user=%s,pass=%s", 
				me_registar_auth->meid, user?user:"?", pass?pass:"?");
		}
	}
	return CLI_OK;
}

static int
cli_sip_agent_related_tcpudp(int fd, struct me_t *me_sip_agent)
{
	unsigned char get_attr_mask[2] = {0xff, 0xff};
	struct me_t *me=meinfo_me_get_by_meid(136, meinfo_util_me_attr_data(me_sip_agent, 5));

	if (me) {
		meinfo_me_refresh(me, get_attr_mask);
		util_fdprintf(fd, "\n\t\t\t>[136]tcpudp:0x%x,port=%d,proto=%d,dscp=0x%x",
			me->meid,
			(unsigned short)meinfo_util_me_attr_data(me, 1),
			(unsigned char)meinfo_util_me_attr_data(me, 2),
			(unsigned char)meinfo_util_me_attr_data(me, 3));
			cli_tcpudp_related_iphost(fd, me);
	}
	return CLI_OK;
}

static char *
get_agent_status_str(int number)
{
	static char *status_str[6]={ "initial", "connected", "icmp_err", "malformed_resp", "inadequate_resp", "timeout" };
	if (number>=0 && number<6)
		return status_str[number];
	return status_str[3];
}

static int
cli_sip_user_related_agent(int fd, struct me_t *me_sip_user)
{
	unsigned char get_attr_mask[2] = {0xff, 0xff};
	struct meinfo_t *miptr=meinfo_util_miptr(150);
	struct me_t *me;

	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		if (me_related(me_sip_user, me)) {
			struct me_t *me_proxy=meinfo_me_get_by_meid(157, meinfo_util_me_attr_data(me, 1));
			struct me_t *me_obproxy=meinfo_me_get_by_meid(157, meinfo_util_me_attr_data(me, 2));
			struct me_t *me_hosturi=meinfo_me_get_by_meid(157, meinfo_util_me_attr_data(me, 8));

			meinfo_me_refresh(me, get_attr_mask);
			util_fdprintf(fd, "\n\t\t>[150]agent:0x%x", me->meid);
			util_fdprintf(fd, ",status=%d(%s)", 
				(unsigned char)meinfo_util_me_attr_data(me, 9),
				get_agent_status_str((unsigned char)meinfo_util_me_attr_data(me, 9)));

			if (me_proxy) {
				char *proxy;
				meinfo_me_refresh(me_proxy, get_attr_mask);
				if ((proxy=meinfo_util_me_attr_ptr(me_proxy, 2))!=NULL)
					util_fdprintf(fd, ",proxy=%s", proxy);
			}
			if (me_obproxy) {
				char *obproxy;
				meinfo_me_refresh(me_obproxy, get_attr_mask);
				if ((obproxy=meinfo_util_me_attr_ptr(me_obproxy, 2))!=NULL)
					util_fdprintf(fd, ",obproxy=%s", obproxy);
			}
			if (me_hosturi) {
				char *hosturi;
				meinfo_me_refresh(me_hosturi, get_attr_mask);
				if ((hosturi=meinfo_util_me_attr_ptr(me_hosturi, 2))!=NULL)
					util_fdprintf(fd, ",hosturi=%s", hosturi);
			}
			cli_sip_agent_related_tcpudp(fd, me);
			cli_sip_agent_related_registar_netaddr(fd, me);
		}
	}
	return CLI_OK;
}

static int
cli_sip_user_related_auth(int fd, struct me_t *me_sip_user)
{
	unsigned char get_attr_mask[2] = {0xff, 0xff};
	struct me_t *me=meinfo_me_get_by_meid(148, meinfo_util_me_attr_data(me_sip_user, 4));

	if (me) {
		char *user, *pass;
		meinfo_me_refresh(me, get_attr_mask);
		user = meinfo_util_me_attr_ptr(me, 2);
		pass = meinfo_util_me_attr_ptr(me, 3);
		util_fdprintf(fd, "\n\t\t>[148]auth:0x%x,user=%s,pass=%s", 
			me->meid, user?user:"?", pass?pass:"?");
	}
	return CLI_OK;
}

static int
cli_pots_related_sip_user(int fd, struct me_t *me_pots)
{
	unsigned char get_attr_mask[2] = {0xff, 0xff};
	struct meinfo_t *miptr=meinfo_util_miptr(153);
	struct me_t *me;

	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		if (me_related(me, me_pots)) {
			char *dispname;
			meinfo_me_refresh(me, get_attr_mask);
			dispname= meinfo_util_me_attr_ptr(me, 3);
			util_fdprintf(fd, "\n\t<[153]sip_user:0x%x,disp=%s", 
				me->meid, dispname?dispname:"null");
			cli_sip_user_related_auth(fd, me);
			cli_sip_user_related_dialplan(fd, me);
			cli_sip_user_related_voip_app_service_profile(fd, me);
			cli_sip_user_related_agent(fd, me);
			cli_sip_user_related_voip_feature_access_codes(fd, me);
		}
	}
	return CLI_OK;
}

static int
cli_media_profile_related_rtp_profile_data(int fd, struct me_t *me_media_profile)
{
	unsigned char get_attr_mask[2] = {0xff, 0xff};
	struct meinfo_t *miptr=meinfo_util_miptr(143);
	struct me_t *me;

	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		if (me_related(me_media_profile, me)) {
			meinfo_me_refresh(me, get_attr_mask);
			util_fdprintf(fd, "\n\t\t\t>[143]rtp_profile_data:0x%x", me->meid);
			util_fdprintf(fd, ",port_min=%d", (unsigned short)meinfo_util_me_attr_data(me, 1));
			util_fdprintf(fd, ",port_max=%d", (unsigned short)meinfo_util_me_attr_data(me, 2));
			util_fdprintf(fd, ",dscp=0x%x", (unsigned char)meinfo_util_me_attr_data(me, 3));
		}
	}
	return CLI_OK;
}

static int
cli_media_profile_related_voice_service_profile(int fd, struct me_t *me_media_profile)
{
	unsigned char get_attr_mask[2] = {0xff, 0xff};
	struct meinfo_t *miptr=meinfo_util_miptr(58);
	struct me_t *me;

	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		if (me_related(me_media_profile, me)) {
			meinfo_me_refresh(me, get_attr_mask);
			util_fdprintf(fd, "\n\t\t\t>[58]voice_service_profile:0x%x", me->meid);
			util_fdprintf(fd, ",jitter_buf_max=%d", (unsigned short)meinfo_util_me_attr_data(me, 3));
			util_fdprintf(fd, ",echo_cancel=%d", (unsigned char)meinfo_util_me_attr_data(me, 4));
		}
	}
	return CLI_OK;
}

static char *
get_codec_str(int number)
{
	static char *codec_str[19]={
		"pcmu", "reserved", "reserved", "gsm", "g723",
		"dvi4-8k", "dvi4-16k", "lpc", "pcma", "g722",
		"l16_2ch", "l16_1ch", "qcelp", "cn", "mpa",
		"g728", "dvi4-11k", "dvi4-22k", "g729"
	};
	if (number>=0 && number<19)
		return codec_str[number];
	return codec_str[1];
}

static int
cli_voice_ctp_related_media_profile(int fd, struct me_t *me_voice_ctp)
{
	unsigned char get_attr_mask[2] = {0xff, 0xff};
	struct meinfo_t *miptr=meinfo_util_miptr(142);
	struct me_t *me;

	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		if (me_related(me_voice_ctp, me)) {
			meinfo_me_refresh(me, get_attr_mask);
			util_fdprintf(fd, "\n\t\t>[142]voip_media_profile:0x%x,fax=%s", 
				me->meid, (meinfo_util_me_attr_data(me, 1))?"T.38":"Passthr");
			util_fdprintf(fd, ",codec=%d(%s)", 
				(unsigned char)meinfo_util_me_attr_data(me, 3),
				get_codec_str((unsigned char)meinfo_util_me_attr_data(me, 3)));
			util_fdprintf(fd, ",%d(%s)", 
				(unsigned char)meinfo_util_me_attr_data(me, 6),
				get_codec_str((unsigned char)meinfo_util_me_attr_data(me, 6)));
			util_fdprintf(fd, ",%d(%s)", 
				(unsigned char)meinfo_util_me_attr_data(me, 9),
				get_codec_str((unsigned char)meinfo_util_me_attr_data(me, 9)));
			util_fdprintf(fd, ",%d(%s)", 
				(unsigned char)meinfo_util_me_attr_data(me, 12),
				get_codec_str((unsigned char)meinfo_util_me_attr_data(me, 12)));
			cli_media_profile_related_rtp_profile_data(fd, me);
			cli_media_profile_related_voice_service_profile(fd, me);
		}
	}
	return CLI_OK;
}

static int
cli_pots_related_voice_ctp(int fd, struct me_t *me_pots)
{
	unsigned char get_attr_mask[2] = {0xff, 0xff};
	struct meinfo_t *miptr=meinfo_util_miptr(139);
	struct me_t *me;

	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		if (me_related(me, me_pots)) {
			meinfo_me_refresh(me, get_attr_mask);
			util_fdprintf(fd, "\n\t<[139]voice_ctp:0x%x,sigcode=%d", 
				me->meid, (unsigned char)meinfo_util_me_attr_data(me, 4));
			cli_voice_ctp_related_media_profile(fd, me);
		}
	}
	return CLI_OK;
}

static char *
get_session_type_str(int number)
{
	static char *type_str[6]={ "none", "2way", "3way", "fax", "telem", "conference" };
	if (number>=0 && number<5)
		return type_str[number];
	return type_str[0];
}

static int
cli_pots_related_line_status(int fd, struct me_t *me_pots)
{
	unsigned char get_attr_mask[2] = {0xff, 0xff};
	struct meinfo_t *miptr=meinfo_util_miptr(141);
	struct me_t *me;

	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		if (me_related(me, me_pots)) {
			meinfo_me_refresh(me, get_attr_mask);
			util_fdprintf(fd, "\n\t-[141]voip_line_status:0x%x", me->meid); 
			util_fdprintf(fd, ",codec=%d(%s)", 
				(unsigned char)meinfo_util_me_attr_data(me, 1),
				get_codec_str((unsigned char)meinfo_util_me_attr_data(me, 1)));
			util_fdprintf(fd, ",section_type=%d(%s)", 
				(unsigned char)meinfo_util_me_attr_data(me, 3),
				get_session_type_str((unsigned char)meinfo_util_me_attr_data(me, 3)));
		}
	}
	return CLI_OK;
}

// meid: 0xffff: show all, other: show specific
int
cli_pots(int fd, unsigned short meid)
{
	unsigned char get_attr_mask[2] = {0xff, 0xff};
	struct meinfo_t *miptr=meinfo_util_miptr(53);
	struct me_t *me;

	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		if (meid==0xffff || meid==me->meid) {
			meinfo_me_refresh(me, get_attr_mask);
			util_fdprintf(fd, "[53]pots:0x%x,admin=%d,rxgain=%d,txgain=%d,op=%d,offhook=%d", 
				me->meid, 
				(unsigned char)meinfo_util_me_attr_data(me, 1),
				(char)meinfo_util_me_attr_data(me, 7),
				(char)meinfo_util_me_attr_data(me, 8),
				(unsigned char)meinfo_util_me_attr_data(me, 9),
				(unsigned char)meinfo_util_me_attr_data(me, 10));
			cli_pots_related_sip_user(fd, me);
			cli_pots_related_voice_ctp(fd, me);
			cli_pots_related_line_status(fd, me);
			util_fdprintf(fd, "\n"); 
		}
	}
	return CLI_OK;
}

// helper function used by cli.c /////////////////////////////////////////////////////////////////////////////////////////////

int
cli_pots_cmdline(int fd, int argc, char *argv[])
{
	unsigned short meid;

	if (strcmp(argv[0], "pots")==0 && argc <=2) {
		if (argc==1) {
			return cli_pots(fd, 0xffff);
		} else if (argc==2) {
			meid = util_atoi(argv[1]);
			return cli_pots(fd, meid);
		}

		return CLI_ERROR_SYNTAX;
	} else {
		return CLI_ERROR_OTHER_CATEGORY;
	}
}

