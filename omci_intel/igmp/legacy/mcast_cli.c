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
 * File    : cli_igmp.c
 *
 ******************************************************************/

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include "list.h"
#include "util.h"
#include "util_inet.h"
#include "switch.h"
#include "cli.h"
#include "switch.h"
#include "cpuport.h"
#include "cpuport_frame.h"
#include "me_related.h"
#include "hwresource.h"

#include "mcast_const.h"
#include "mcast.h"
#include "mcast_pkt.h"
#include "igmp_mbship.h"
#include "mcast_main.h"
#include "mcast_timer.h"
#include "mcast_switch.h"
#include "igmp_omci.h"
#include "mcast_router.h"
#include "mcast_snoop.h"

#define MAYBE_UNUSED __attribute__((unused))
char igmp_clientinfo_str_g[2048];

int
cli_igmp_mcast_op_profile_igmp_tag_control(int fd, struct me_t *me, struct me_t *me_config)
{
	unsigned char us_tag_control = (unsigned char)meinfo_util_me_attr_data(me, 5);
	unsigned short us_tci = (unsigned short) util_bitmap_get_value(meinfo_util_me_attr_ptr(me, 4), 2*8, 0, 16);
	unsigned char ds_tag_control = (unsigned char) util_bitmap_get_value(meinfo_util_me_attr_ptr(me, 16), 3*8, 0, 8);
	unsigned short ds_tci = (unsigned short) util_bitmap_get_value(meinfo_util_me_attr_ptr(me, 16), 3*8, 8, 16);
	unsigned short vid_uni = 0xFFFF;
	struct attr_table_header_t *mspt = (struct attr_table_header_t *) meinfo_util_me_attr_ptr(me_config, 6);
	struct attr_table_entry_t *table_entry_pos;

	list_for_each_entry(table_entry_pos, &mspt->entry_list, entry_node)
	{
		if (table_entry_pos->entry_data_ptr != NULL)
		{
			unsigned short meid = (unsigned short)util_bitmap_get_value(table_entry_pos->entry_data_ptr, 20*8, 80, 16);
			if(meid == me->meid)
			{
				vid_uni = (unsigned short)util_bitmap_get_value(table_entry_pos->entry_data_ptr, 20*8, 16, 16);
				break;
			}
		}
	}

	switch (vid_uni)
	{
	case 4096:
		util_fdprintf(fd, "\n\t\t\t\tUS MATCH igmp untag(default)");
		break;
	case 4097:
		util_fdprintf(fd, "\n\t\t\t\tUS MATCH igmp 1tag(default)");
		break;
	case 0xFFFF:
		util_fdprintf(fd, "\n\t\t\t\tUS MATCH igmp (unspecified)");
		break;
	default:
		util_fdprintf(fd, "\n\t\t\t\tUS MATCH igmp 1tag: vid=%d", vid_uni);
		break;
	}

	switch (us_tag_control) 
	{
	case 0:
		util_fdprintf(fd, ": TREAT -1tag,+1tag: pri=pri,vid=vid");
		break;
	case 1:
		util_fdprintf(fd, ": TREAT +1tag: pri=%d,vid=%d", ((us_tci>>13)&0x7), (us_tci&0xfff));
		break;
	case 2:
		util_fdprintf(fd, ": TREAT -1tag,+1tag: pri=%d,vid=%d", ((us_tci>>13)&0x7), (us_tci&0xfff));
		break;
	case 3:
		util_fdprintf(fd, ": TREAT -1tag,+1tag: pri=pri,vid=%d", (us_tci&0xfff));
		break;
	default:
		util_fdprintf(fd, ": TREAT unknown (us_tag_control=%d)", us_tag_control);
		break;
	}

	switch (ds_tag_control)
	{
	case 0:
		util_fdprintf(fd, "\n\t\t\t\tDS TREAT igmp/mcast -1tag,+1tag: pri=pri,vid=vid");
		break;
	case 1:
		util_fdprintf(fd, "\n\t\t\t\tDS TREAT igmp/mcast -1tag");
		break;
	case 2:
		util_fdprintf(fd, "\n\t\t\t\tDS TREAT igmp/mcast +1tag: pri=%d,vid=%d", ((ds_tci>>12)&0x7), (ds_tci&0xfff));
		break;
	case 3:
		util_fdprintf(fd, "\n\t\t\t\tDS TREAT igmp/mcast -1tag,+1tag: pri=%d,vid=%d", ((ds_tci>>12)&0x7), (ds_tci&0xfff));
		break;
	case 4:
		util_fdprintf(fd, "\n\t\t\t\tDS TREAT igmp/mcast -1tag,+1tag: pri=pri,vid=%d", (ds_tci&0xfff));
		break;
	case 5:
		util_fdprintf(fd, "\n\t\t\t\tDS TREAT igmp/mcast +1tag: pri=%d,vid=%d", ((vid_uni>>12)&0x7), (vid_uni&0xfff));
		break;
	case 6:
		util_fdprintf(fd, "\n\t\t\t\tDS TREAT igmp/mcast -1tag,+1tag: pri=%d,vid=%d", ((vid_uni>>12)&0x7), (vid_uni&0xfff));
		break;
	case 7:
		util_fdprintf(fd, "\n\t\t\t\tDS TREAT igmp/mcast -1tag,+1tag: pri=pri,vid=%d", (vid_uni&0xfff));
		break;
	default:
		util_fdprintf(fd, "\n\t\t\t\tDS TREAT igmp/mcast unknown (ds_tag_control=%d)", ds_tag_control);
		break;
	}

	return CLI_OK;
}

static int
cli_igmp_mcast_op_profile_acl_table(int fd, struct me_t *me, unsigned short attr)
{
	struct attr_table_header_t *acl = (struct attr_table_header_t *) meinfo_util_me_attr_ptr(me, attr);
	struct attr_table_entry_t *table_entry_pos;

	list_for_each_entry(table_entry_pos, &acl->entry_list, entry_node)
	{
		if (table_entry_pos->entry_data_ptr != NULL)
		{
			unsigned char row_part = (unsigned char) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 24*8, 2, 3);
			switch (row_part)
			{
			case 0:
			default:
				util_fdprintf(fd, "\n\t\t\t\t[%s_acl]key:%d,gem=%d,vlan=%d,bw=%d,",
					(attr==7) ? "dynamic" : "static",
					(unsigned short) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 24*8, 6, 10),
					(unsigned short) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 24*8, 16, 16),
					(unsigned short) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 24*8, 32, 16),
					(unsigned int) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 24*8, 144, 32));
				util_fdprintf(fd, "sip=%d.%d.%d.%d,dsp=%d.%d.%d.%d~%d.%d.%d.%d", 
					(unsigned char) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 24*8, 48, 8),
					(unsigned char) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 24*8, 56, 8),
					(unsigned char) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 24*8, 64, 8),
					(unsigned char) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 24*8, 72, 8),
					(unsigned char) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 24*8, 80, 8),
					(unsigned char) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 24*8, 88, 8),
					(unsigned char) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 24*8, 96, 8),
					(unsigned char) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 24*8, 104, 8),
					(unsigned char) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 24*8, 112, 8),
					(unsigned char) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 24*8, 120, 8),
					(unsigned char) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 24*8, 128, 8),
					(unsigned char) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 24*8, 136, 8));
				break;
			case 1:
				util_fdprintf(fd, "\n\t\t\t\t[%s_acl]key:%d,preview_len=%d,preview_rep_t=%d,preview_rep_cnt=%d,preview_reset_t=%d,", 
					(attr==7) ? "dynamic" : "static",
					(unsigned short) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 24*8, 6, 10),
					(unsigned short) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 24*8, 112, 16),
					(unsigned short) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 24*8, 128, 16),
					(unsigned short) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 24*8, 144, 16),
					(unsigned short) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 24*8, 160, 16));
				util_fdprintf(fd, "sip=%x:%x:%x:%x:%x:%x:0:0", 
					(unsigned short) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 24*8, 16, 16),
					(unsigned short) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 24*8, 32, 16),
					(unsigned short) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 24*8, 48, 16),
					(unsigned short) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 24*8, 64, 16),
					(unsigned short) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 24*8, 80, 16),
					(unsigned short) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 24*8, 96, 16));
				break;
			case 2:
				util_fdprintf(fd, "\n\t\t\t\t[%s_acl]key:%d,", 
					(attr==7) ? "dynamic" : "static",
					(unsigned short) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 24*8, 6, 10));
				util_fdprintf(fd, "dip=%x:%x:%x:%x:%x:%x:0:0", 
					(unsigned short) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 24*8, 16, 16),
					(unsigned short) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 24*8, 32, 16),
					(unsigned short) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 24*8, 48, 16),
					(unsigned short) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 24*8, 64, 16),
					(unsigned short) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 24*8, 80, 16),
					(unsigned short) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 24*8, 96, 16));
				break;
			}
		}
	}

	return CLI_OK;
}

static int
cli_igmp_mcast_subscriber_cfg_related_mcast_op_profile(int fd, struct me_t *me_config)
{
	unsigned char get_attr_mask[2] = {0xff, 0xff};
	struct meinfo_t *miptr=meinfo_util_miptr(309);
	struct me_t *me;

	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		if (me_related(me, me_config))
		{
			meinfo_me_refresh(me, get_attr_mask);
			util_fdprintf(fd, "\n\t\t\t>[309]mcast_op_profile:0x%x,ver=%d,func=%d,fastlave=%d,rate=%d,robust=%d,q_interval=%d,q_max_resp_t=%d,last_member_q_interval=%d", 
				me->meid,
				(unsigned char)meinfo_util_me_attr_data(me, 1),
				(unsigned char)meinfo_util_me_attr_data(me, 2),
				(unsigned char)meinfo_util_me_attr_data(me, 3),
				(unsigned int)meinfo_util_me_attr_data(me, 6),
				(unsigned char)meinfo_util_me_attr_data(me, 10),
				(unsigned int)meinfo_util_me_attr_data(me, 12),
				(unsigned int)meinfo_util_me_attr_data(me, 13),
				(unsigned int)meinfo_util_me_attr_data(me, 14));
			cli_igmp_mcast_op_profile_igmp_tag_control(fd, me, me_config);
			cli_igmp_mcast_op_profile_acl_table(fd, me, 7);
			cli_igmp_mcast_op_profile_acl_table(fd, me, 8);
		} 
	}
	return CLI_OK;
}

static int
cli_igmp_bport_related_me(int fd, struct me_t *me_bport)
{
	unsigned char get_attr_mask[2] = {0xff, 0xff};
	unsigned short related_classid=meinfo_me_attr_pointer_classid(me_bport, 4);
	unsigned short related_pointer=meinfo_util_me_attr_data(me_bport, 4);
	struct meinfo_t *miptr=meinfo_util_miptr(related_classid);
	struct me_t *me=meinfo_me_get_by_meid(related_classid, related_pointer);
	char *devname;

	if (me)
	{
		meinfo_me_refresh(me, get_attr_mask);
		switch (related_classid)
		{
		case 11:
			util_fdprintf(fd, "\n\t\t>[11]uni:0x%x,admin=%d,op=%d", related_pointer, 
				(unsigned char)meinfo_util_me_attr_data(me, 5), 
				(unsigned char)meinfo_util_me_attr_data(me, 6));
			if ((devname=hwresource_devname(me))!=NULL)
				util_fdprintf(fd, " (%s)", devname); 
			break;
		case 130:
			util_fdprintf(fd, "\n\t\t>[130]mapper:0x%x", related_pointer);
			break;
		case 134:
			util_fdprintf(fd, "\n\t\t>[134]iphost:0x%x", related_pointer);
			{
				struct me_t *me_iphost=meinfo_me_get_by_meid(134, related_pointer);
				char *devname = meinfo_util_get_config_devname(me_iphost);
				util_fdprintf(fd, ",dev=%s", devname?devname:"");
			}
			break;
		case 266:
			util_fdprintf(fd, "\n\t\t>[266]iwtp:0x%x", related_pointer);
			break;
		case 281:
			util_fdprintf(fd, "\n\t\t>[281]mcast_iwtp:0x%x", related_pointer);
			break;
		case 329:
			util_fdprintf(fd, "\n\t\t>[329]veip:0x%x", related_pointer);
			{
				struct me_t *me_veip=meinfo_me_get_by_meid(329, related_pointer);
				if (me_veip) {
					char *devname = meinfo_util_get_config_devname(me_veip);
					util_fdprintf(fd, ",dev=%s,admin=%d,op=%d",
						devname?devname:"",
						(unsigned char)meinfo_util_me_attr_data(me_veip, 1), 
						(unsigned char)meinfo_util_me_attr_data(me_veip, 2));
				}
			}
			break;
		default:
			if (miptr->name)
				util_fdprintf(fd, "\n\t\t>[%d]%s:0x%x", related_classid, miptr->name, related_pointer);
			else
				util_fdprintf(fd, "\n\t\t>[%d]:0x%x", related_classid, related_pointer);
			break;
		}
	}
	return CLI_OK;
}

static int
cli_igmp_mcast_subscriber_cfg_service_package_table(int fd, struct me_t *me)
{
	struct attr_table_header_t *service_pkg = (struct attr_table_header_t *) meinfo_util_me_attr_ptr(me, 6);
	struct attr_table_entry_t *table_entry_pos;

	list_for_each_entry(table_entry_pos, &service_pkg->entry_list, entry_node)
	{
		if (table_entry_pos->entry_data_ptr != NULL)
		{
			unsigned short uni_vid = (unsigned short) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 20*8, 16, 16);
			util_fdprintf(fd, "\n\t\t\t[service_pkg] match ");
			if (uni_vid <4096)
				util_fdprintf(fd, "vid=%d", uni_vid);
			else if (uni_vid == 4096) 
				util_fdprintf(fd, "untag");
			else if (uni_vid == 4097)
				util_fdprintf(fd, "vid=*");
			else
				util_fdprintf(fd, "*");
			util_fdprintf(fd, ", max_grp=%u, max_bw=%ubyte/s, mcast_op_profile_meid=0x%x", 
				(unsigned short) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 20*8, 32, 16),
				(unsigned short) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 20*8, 48, 32),
				(unsigned short) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 20*8, 80, 16));
		}
	}
	return CLI_OK;
}

static int
cli_igmp_bport_related_mcast_subscriber_cfg(int fd, struct me_t *me_bport)
{
	unsigned char get_attr_mask[2] = {0xff, 0xff};
	struct meinfo_t *miptr=meinfo_util_miptr(310);
	struct me_t *me;

	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		if (me_related(me, me_bport))
		{
			meinfo_me_refresh(me, get_attr_mask);
			util_fdprintf(fd, "\n\t\t-[310]mcast_subscriber_cfg:0x%x,Max_Grp=%d,Max_BW=%d,BW_enforce=%d", 
				me->meid,
				(unsigned short)meinfo_util_me_attr_data(me, 3), 
				(unsigned int)meinfo_util_me_attr_data(me, 4), 
				(unsigned char)meinfo_util_me_attr_data(me, 5));
			cli_igmp_mcast_subscriber_cfg_service_package_table(fd, me);
			cli_igmp_mcast_subscriber_cfg_related_mcast_op_profile(fd, me);
		} 
	}
	return CLI_OK;
}

static int
cli_igmp_mcast_monitor_active_group_list_table(int fd, struct me_t *me, unsigned short attr)
{
	struct attr_table_header_t *acl = (struct attr_table_header_t *) meinfo_util_me_attr_ptr(me, attr);
	struct attr_table_entry_t *table_entry_pos;

	list_for_each_entry(table_entry_pos, &acl->entry_list, entry_node)
	{
		if (table_entry_pos->entry_data_ptr != NULL)
		{
			if(attr==5)
			{
				util_fdprintf(fd, "\n\t\t\t[ipv4_active_grp]vid=%d,join_time=%d,", 
					(unsigned short) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 24*8, 0, 16),
					(unsigned int) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 24*8, 144, 32));
				util_fdprintf(fd, "client_ip=%d.%d.%d.%d,sip=%d.%d.%d.%d,dip=%d.%d.%d.%d", 
					(unsigned short) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 24*8, 112, 8),
					(unsigned short) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 24*8, 120, 8),
					(unsigned short) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 24*8, 128, 8),
					(unsigned short) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 24*8, 136, 8),
					(unsigned short) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 24*8, 16, 8),
					(unsigned short) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 24*8, 24, 8),
					(unsigned short) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 24*8, 32, 8),
					(unsigned short) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 24*8, 40, 8),
					(unsigned short) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 24*8, 48, 8),
					(unsigned short) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 24*8, 56, 8),
					(unsigned short) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 24*8, 64, 8),
					(unsigned short) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 24*8, 72, 8));
			}
			else
			{
				util_fdprintf(fd, "\n\t\t\t[ipv6_active_grp]vid=%d,join_time=%d,", 
					(unsigned short) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 58*8, 0, 16),
					(unsigned int) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 58*8, 432, 32));
				util_fdprintf(fd, "client_ip=%x:%x:%x:%x:%x:%x:%x:%x,sip=%x:%x:%x:%x:%x:%x:%x:%x,dip=%x:%x:%x:%x:%x:%x:%x:%x", 
					(unsigned short) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 58*8, 304, 16),
					(unsigned short) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 58*8, 320, 16),
					(unsigned short) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 58*8, 336, 16),
					(unsigned short) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 58*8, 352, 16),
					(unsigned short) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 58*8, 368, 16),
					(unsigned short) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 58*8, 384, 16),
					(unsigned short) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 58*8, 400, 16),
					(unsigned short) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 58*8, 416, 16),
					(unsigned short) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 58*8, 16, 16),
					(unsigned short) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 58*8, 32, 16),
					(unsigned short) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 58*8, 48, 16),
					(unsigned short) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 58*8, 64, 16),
					(unsigned short) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 58*8, 80, 16),
					(unsigned short) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 58*8, 96, 16),
					(unsigned short) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 58*8, 112, 16),
					(unsigned short) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 58*8, 128, 16),
					(unsigned short) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 58*8, 144, 16),
					(unsigned short) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 58*8, 160, 16),
					(unsigned short) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 58*8, 176, 16),
					(unsigned short) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 58*8, 192, 16),
					(unsigned short) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 58*8, 208, 16),
					(unsigned short) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 58*8, 224, 16),
					(unsigned short) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 58*8, 240, 16),
					(unsigned short) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 58*8, 256, 16));
			}
		}
	}

	return CLI_OK;
}

static int
cli_igmp_bport_related_mcast_monitor(int fd, struct me_t *me_bport)
{
	unsigned char get_attr_mask[2] = {0xff, 0xff};
	struct meinfo_t *miptr=meinfo_util_miptr(311);
	struct me_t *me;

	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		if (me_related(me, me_bport))
		{
			meinfo_me_refresh(me, get_attr_mask);
			util_fdprintf(fd, "\n\t\t-[311]mcast_monitor:0x%x,current_bw=%d,join_cnt=%d,bw_exceed_cnt=%d", 
				me->meid,
				(unsigned int)meinfo_util_me_attr_data(me, 2), 
				(unsigned int)meinfo_util_me_attr_data(me, 3), 
				(unsigned int)meinfo_util_me_attr_data(me, 4));
			cli_igmp_mcast_monitor_active_group_list_table(fd, me, 5);
			cli_igmp_mcast_monitor_active_group_list_table(fd, me, 6);
			if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_FIBERHOME)==0)
			{
				struct meinfo_t *miptr_65311=meinfo_util_miptr(65311);
				struct me_t *me_65311;

				list_for_each_entry(me_65311, &miptr_65311->me_instance_list, instance_node) {
					if (me_related(me, me_65311))
					{
						meinfo_me_refresh(me_65311, get_attr_mask);
						util_fdprintf(fd, "\n\t\t-[65311]mcast_stats:0x%x,failjoins_cnt=%d,leave_cnt=%d,gereral_q_cnt=%d,specific_q_cnt=%d,invalid_cnt=%d", 
							me_65311->meid,
							(unsigned int)meinfo_util_me_attr_data(me_65311, 1), 
							(unsigned int)meinfo_util_me_attr_data(me_65311, 2), 
							(unsigned int)meinfo_util_me_attr_data(me_65311, 3), 
							(unsigned int)meinfo_util_me_attr_data(me_65311, 4), 
							(unsigned int)meinfo_util_me_attr_data(me_65311, 5));
					} 
				}
			}
		} 
	}
	return CLI_OK;
}

static int
cli_igmp_bridge_related_bport(int fd, struct me_t *me_bridge)
{
	unsigned char get_attr_mask[2] = {0xff, 0xff};
	struct meinfo_t *miptr=meinfo_util_miptr(47);
	struct me_t *me;

	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		if (me_related(me, me_bridge))
		{
			meinfo_me_refresh(me, get_attr_mask);
			util_fdprintf(fd, "\n\t<[47]bport:0x%x,is_private=%d", me->meid, me->is_private);
			cli_igmp_bport_related_me(fd, me);
			cli_igmp_bport_related_mcast_subscriber_cfg(fd, me);
			cli_igmp_bport_related_mcast_monitor(fd, me);
		} 
	}
	return CLI_OK;
}

static int
cli_igmp_related_bridge(int fd)
{
	struct meinfo_t *miptr=meinfo_util_miptr(45);
	struct me_t *me;

	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		util_fdprintf(fd, "[45]bridge:0x%x", me->meid);
		cli_igmp_bridge_related_bport(fd, me);
		util_fdprintf(fd, "\n"); 
	}
	return CLI_OK;
}

#ifndef X86

static int 
cli_igmp_joined_membership_list(int fd)
{
	struct igmp_mbship_t *pos=NULL, *n=NULL;
	long current = igmp_timer_get_cur_time();
	struct switch_mac_tab_entry_t mac_tab_entry;
	int num=0;

	if (igmpproxytask_loop_g  == 0) return 0;

	util_fdprintf(fd,"membership list\n");
        
	char str[1024];
	memset(str,0,1024);
	char group_ip[INET6_ADDRSTRLEN];
	char src_ip[INET6_ADDRSTRLEN];
	char client_ip[INET6_ADDRSTRLEN];
	unsigned short i=1;

	if (!list_empty(&igmp_mbship_db_g.mbship_list))
	{
		list_for_each_entry_safe(pos, n, &igmp_mbship_db_g.mbship_list, entry_node) {
			memset(group_ip,0,INET6_ADDRSTRLEN);
			memset(src_ip,0,INET6_ADDRSTRLEN);
			memset(client_ip,0,INET6_ADDRSTRLEN);

			if ( pos->group_ip.ipv6.s6_addr32[1] | pos->group_ip.ipv6.s6_addr32[2] | pos->group_ip.ipv6.s6_addr32[3] )
			{ //IPv6
				util_inet_ntop(AF_INET6, &pos->group_ip, (char *)group_ip, sizeof(group_ip) );
				util_inet_ntop(AF_INET6, &pos->src_ip, (char *)src_ip, sizeof(src_ip) );
				util_inet_ntop(AF_INET6, &pos->client_ip, (char *)client_ip, sizeof(client_ip) );
			}
			else
			{
				util_inet_ntop(AF_INET, &pos->group_ip, (char *)group_ip, sizeof(group_ip) );
				util_inet_ntop(AF_INET, &pos->src_ip, (char *)src_ip, sizeof(src_ip) );
				util_inet_ntop(AF_INET, &pos->client_ip, (char *)client_ip, sizeof(client_ip) );
			}

			//port_me=meinfo_me_get_by_meid(47, pos->src_port_meid);
			//iport_me = hwresource_public2private(port_me);
	    		if (omci_env_g.olt_alu_mcast_support)
			{
	                	if (!alu_igmp_get_group_signal(&igmp_mbship_db_g, &pos->group_ip) && !igmp_omci_check_is_in_static_acl(&pos->group_ip))
					continue;
	    		}

			if (omci_env_g.igmp_proxy_snooping == 0)
				snprintf(str,1024,"%d\tlogical_port_id:%d\tsrc_port_meid:%d(0x%x)\tuni_vid:%d\tani_vid:%d\n\tgroup_ip:%s\tsrc_ip:%s\tclient_ip:%s\t\nclient_mac:%02x-%02x-%02x-%02x-%02x-%02x\tage:%ld\n\n",
					i,pos->logical_port_id,pos->src_port_meid, pos->src_port_meid, pos->uni_vid, pos->ani_vid, 
					group_ip, src_ip, client_ip,
					pos->client_mac[0],pos->client_mac[1],pos->client_mac[2],
					pos->client_mac[3],pos->client_mac[4], pos->client_mac[5],
					current - pos->access_time);
			else
				snprintf(str,1024,"%d\tlogical_port_id:%d\tsrc_port_meid:%d(0x%x)\tuni_vid:%d\tani_vid:%d\n\tgroup_ip:%s\tsrc_ip:%s\tclient_ip:%s\tage:%ld\n\n",
					i,pos->logical_port_id,pos->src_port_meid, pos->src_port_meid, pos->uni_vid, pos->ani_vid, 
					group_ip, src_ip, client_ip,
					current - pos->access_time);
			util_fdprintf(fd,"%s",str);
			if (pos->lw_igmpv3_signature == IGMP_V3_LW_SIGNATURE)
	    		{
	    			util_fdprintf(fd,"\n");	// add another new line if it's a member by v3
			}
			i++;
		}
	}

	if (igmp_config_g.igmp_version & IGMP_V3_BIT)
	{
        	util_fdprintf(fd, "\n\nIGMPv3 Group/Source membership\n\n");
        	i = 0;
        	struct igmp_v3_lw_mbship_t *pos_lw, *n_lw;
	        list_for_each_entry_safe(pos_lw, n_lw, &igmp_mbship_db_g.mcast_snoop_list, entry_node) {
	                struct igmp_v3_lw_src_t *pos1, *n1;
	                struct igmp_mbship_t *pos2, *n2;
	                util_fdprintf(fd,"[%d] Group-%-15s (%s)\n\t-ASM hosts:\n", i, util_inet_ntop(AF_INET, (unsigned char *)(&pos_lw->group_ip), (char *)&group_ip, 16), (list_empty(&pos_lw->host_asm_list))?"SSM":"ASM");
	                list_for_each_entry_safe(pos2, n2, &pos_lw->host_asm_list, entry_node_v3) {
	                                util_fdprintf(fd,"\t\t-%-15s\n", util_inet_ntop(AF_INET, (unsigned char *)(&pos2->client_ip), (char *)&client_ip, 16));
	                } 
	                
	                list_for_each_entry_safe(pos1, n1, &pos_lw->src_list, entry_node) {
	                        util_fdprintf(fd,"\t-Source %-15s, SSM hosts:\n", util_inet_ntop(AF_INET, (unsigned char *)(&pos1->src_ip), (char *)&group_ip, 16));
	                        list_for_each_entry_safe(pos2, n2, &pos1->host_ssm_list, entry_node_v3) {
	                                util_fdprintf(fd,"\t\t-%-15s\n", util_inet_ntop(AF_INET, (unsigned char *)(&pos2->client_ip), (char *)&client_ip, 16));
	                        }       
	                }
	                i++;
	                util_fdprintf(fd, "\n");                              
		}
        }        
        
	i=1;
	util_fdprintf(fd, "\n\nFVT2510 mac table IPV4 multicast entry\n\n");
	util_fdprintf(fd, "idx\tentry\tgroup ip\tsource ip\tvid\tfid\tport mask\n");

	do
	{
		memset(&mac_tab_entry, 0, sizeof(struct switch_mac_tab_entry_t));
		if (switch_hw_g.mac_ipmc_get_next( &num, &mac_tab_entry ) != 1) break;

		util_fdprintf(fd, "%d\t%d\t%-15s\t%-15s\t%d\t%d\t0x%x\n",i,num, 
				util_inet_ntop (AF_INET, (unsigned char *)(&mac_tab_entry.ipmcast.dst_ip), 
					(char *)&group_ip, 16),
                                util_inet_ntop (AF_INET, (unsigned char *)(&mac_tab_entry.ipmcast.src_ip), 
					(char *)&src_ip, 16),
				mac_tab_entry.vid, mac_tab_entry.fid, 
				mac_tab_entry.port_bitmap);
		num++;
		i++;

	} while (i<8192);	// mactab entry range 0..2047, use 8192 for safety, better than while (1)
/*s
	i=1;
	num=0;
	do
	{
		memset(&mac_tab_entry, 0, sizeof(struct switch_mac_tab_entry_t));
		if (switch_hw_g.mac_l2mc_get_next(&num, &mac_tab_entry ) != 1) break;

		util_fdprintf(fd, "%d\t%d\t%02x:%02x:%02x:%02x:%02x:%02x\t%d\t%d\t0x%x\n",i,num, 
				mac_tab_entry.mac_addr[0],mac_tab_entry.mac_addr[1],mac_tab_entry.mac_addr[2],
				mac_tab_entry.mac_addr[3],mac_tab_entry.mac_addr[4],mac_tab_entry.mac_addr[5], 
				mac_tab_entry.vid, mac_tab_entry.fid, 
				mac_tab_entry.port_bitmap);
		num++;
		i++;

	} while (i<8192);	// mactab entry range 0..2047, use 8192 for safety, better than while (1)
*/
	return 0;
}

static int 
cli_igmp_delete_membership(int fd, char *str, char *data)
{
	struct igmp_mbship_t *pos=NULL, *n=NULL;
	union ipaddr delete_ip;
	struct me_t *bridge_port_me=NULL;
	struct switch_mac_tab_entry_t igmp_mac_tab_entry;
	int i, prototype = 4;

	memset (&delete_ip, 0, sizeof(union ipaddr));

	for (i= 0; i < 6; i++) igmp_mac_tab_entry.mac_mask[i] = 0xff;

	if (data!=NULL && strchr(data, ':')) prototype = 6;

	if (igmpproxytask_loop_g  == 0) return 0;

	util_fdprintf(fd," membership list\n",i);

	if (list_empty(&igmp_mbship_db_g.mbship_list))
	{
		util_fdprintf(fd,"membership list is empty\n");
		return 0;
	}

	if (!strcmp(str, "group"))
	{
		if(prototype == 4) 
			util_inet_pton(AF_INET, data, &delete_ip.ipv4);
		else if (prototype == 6) 
			util_inet_pton(AF_INET6, data, &delete_ip.ipv6);

		list_for_each_entry_safe(pos, n, &igmp_mbship_db_g.mbship_list, entry_node) {
			if(igmp_pkt_ip_cmp(&pos->group_ip, &delete_ip)) continue;
			if (pos->lw_igmpv3_signature == IGMP_V3_LW_SIGNATURE) continue;
			/*delete active_group_list table data*/
			bridge_port_me = meinfo_me_get_by_meid( 47, pos->src_port_meid );
			if(prototype == 4) 
				igmp_omci_ipv4_active_group_remove(bridge_port_me, pos);
			else if(prototype == 6) 
				igmp_omci_ipv6_active_group_remove(bridge_port_me, pos);
			/* End */

			igmp_pkt_ip_cpy(&igmp_mac_tab_entry.ipmcast.dst_ip, &pos->group_ip);

			list_del(&pos->entry_node);
			free_safe(pos);
			igmp_switch_mcast_entry_del(&igmp_mac_tab_entry);
		}
	}
	else if (!strcmp(str, "source"))
	{
		if(prototype == 4) 
			util_inet_pton(AF_INET, data, &delete_ip.ipv4);
		else if (prototype == 6) 
			util_inet_pton(AF_INET6, data, &delete_ip.ipv6);

		list_for_each_entry_safe(pos, n, &igmp_mbship_db_g.mbship_list, entry_node) {
			if(igmp_pkt_ip_cmp(&pos->src_ip, &delete_ip)) continue;
			if (pos->lw_igmpv3_signature == IGMP_V3_LW_SIGNATURE) continue;
			/*delete active_group_list table data*/
			bridge_port_me = meinfo_me_get_by_meid( 47, pos->src_port_meid );
			if(prototype == 4) 
				igmp_omci_ipv4_active_group_remove(bridge_port_me, pos);
			else if(prototype == 6) 
				igmp_omci_ipv6_active_group_remove(bridge_port_me, pos);
			/* End */

			igmp_pkt_ip_cpy(&igmp_mac_tab_entry.ipmcast.dst_ip, &pos->group_ip);

			list_del(&pos->entry_node);
			free_safe(pos);
			igmp_switch_mcast_entry_del(&igmp_mac_tab_entry);
		}
	}
	else if (!strcmp(str, "client"))
	{
		if(prototype == 4) 
			util_inet_pton(AF_INET, data, &delete_ip.ipv4);
		else if (prototype == 6) 
			util_inet_pton(AF_INET6, data, &delete_ip.ipv6);

		list_for_each_entry_safe(pos, n, &igmp_mbship_db_g.mbship_list, entry_node) {
			if (pos->lw_igmpv3_signature == IGMP_V3_LW_SIGNATURE) continue;
			if(igmp_pkt_ip_cmp(&pos->client_ip, &delete_ip)) continue;
			/*delete active_group_list table data*/
			bridge_port_me = meinfo_me_get_by_meid( 47, pos->src_port_meid );
			if(prototype == 4) 
				igmp_omci_ipv4_active_group_remove(bridge_port_me, pos);
			else if(prototype == 6) 
				igmp_omci_ipv6_active_group_remove(bridge_port_me, pos);
			/* End */

			igmp_pkt_ip_cpy(&igmp_mac_tab_entry.ipmcast.dst_ip, &pos->group_ip);

			list_del(&pos->entry_node);
			free_safe(pos);
			igmp_switch_mcast_entry_del(&igmp_mac_tab_entry);
		}
	}
	else if (!strcmp(str, "all"))
	{
		list_for_each_entry_safe(pos, n, &igmp_mbship_db_g.mbship_list, entry_node) {
			if (pos->lw_igmpv3_signature == IGMP_V3_LW_SIGNATURE) continue;
                        /*delete active_group_list table data*/
			bridge_port_me = meinfo_me_get_by_meid( 47, pos->src_port_meid );
			if(prototype == 4) 
				igmp_omci_ipv4_active_group_remove(bridge_port_me, pos);
			else if(prototype == 6) 
				igmp_omci_ipv6_active_group_remove(bridge_port_me, pos);
			/* End */
			igmp_pkt_ip_cpy(&igmp_mac_tab_entry.ipmcast.dst_ip, &pos->group_ip);
			list_del(&pos->entry_node);
			free_safe(pos);
			igmp_switch_mcast_entry_del(&igmp_mac_tab_entry);
		}
	}
	util_fdprintf(fd, "\n\nFVT2510 mac table IPV4 multicast entry\n\n");
	util_fdprintf(fd, "idx\tgroup ip\tvid\tfid\tsource mac\t\tport mask\n");
	struct switch_mac_tab_entry_t mac_tab_entry;
	memset(&mac_tab_entry, 0, sizeof(struct switch_mac_tab_entry_t));
	unsigned int num=0;
	char group_ip[INET6_ADDRSTRLEN];

	for (i=0 ; i< 2000 ; i++)
	{
		if (switch_hw_g.mac_ipmc_get_next(&num, &mac_tab_entry) != SWITCH_RET_OK) break;
		if ( pos->group_ip.ipv6.s6_addr32[1] | pos->group_ip.ipv6.s6_addr32[2] | pos->group_ip.ipv6.s6_addr32[3] )
			util_inet_ntop (AF_INET6, (unsigned char *)(&mac_tab_entry.ipmcast.dst_ip), (char *)&group_ip,INET6_ADDRSTRLEN );
		else
			util_inet_ntop (AF_INET, (unsigned char *)(&mac_tab_entry.ipmcast.dst_ip), (char *)&group_ip,INET6_ADDRSTRLEN );
		util_fdprintf(fd, "%d\t%s\t%d\t%d\t%s\t0x%x\n",i ,group_ip, 
				mac_tab_entry.vid, mac_tab_entry.fid, 
				util_str_macaddr(mac_tab_entry.mac_addr),
				mac_tab_entry.port_bitmap);
	}
	return 0;
}

static int
cli_igmp_config_list(int fd)
{
	if (igmpproxytask_loop_g  == 0) return 0;

	util_fdprintf(fd, "current configurations:\n");
	util_fdprintf(fd, "\tmax_response_time: %d (unit:0.1sec)\n",igmp_config_g.max_respon_time);
	util_fdprintf(fd, "\tquery_interval: %d (unit:sec)\n",igmp_config_g.query_interval);
	util_fdprintf(fd, "\tlast_member_query_interval: %d (unit:0.1sec)\n",igmp_config_g.last_member_query_interval);
	util_fdprintf(fd, "\trobustness: %d\n",igmp_config_g.robustness);
	util_fdprintf(fd, "\timmediate_leave: %d\n",igmp_config_g.immediate_leave);
	if (igmp_config_g.igmp_version & IGMP_V3_BIT)
		util_fdprintf(fd, "\tigmp_version: v3\n");
	else if (igmp_config_g.igmp_version & IGMP_V2_BIT)
		util_fdprintf(fd, "\tigmp_version: v2\n");
	else if (igmp_config_g.igmp_version & IGMP_V1_BIT)
		util_fdprintf(fd, "\tigmp_version: v1\n");
	else
		util_fdprintf(fd, "\tigmp_version: %d\n",igmp_config_g.igmp_version);
	util_fdprintf(fd, "\tigmp_function: %d\n",igmp_config_g.igmp_function);
	util_fdprintf(fd, "\tquerier_ip: %s\n",igmp_config_g.querier_ip);
	util_fdprintf(fd, "\tupstream_igmp_rate: %d\n",igmp_config_g.upstream_igmp_rate);
	util_fdprintf(fd, "\tupstream_igmp_tci: %x (0x%x, 0x%x, 0x%x)\n", igmp_config_g.upstream_igmp_tci, (igmp_config_g.upstream_igmp_tci&0xE000)>>13, (igmp_config_g.upstream_igmp_tci&0x1000)>>12, igmp_config_g.upstream_igmp_tci&0xFFF);
	util_fdprintf(fd, "\tupstream_igmp_tag_ctl: %d\n",igmp_config_g.upstream_igmp_tag_ctl);
	util_fdprintf(fd, "\tdownstream_igmp_tci: %x (0x%x, 0x%x, 0x%x)\n", igmp_config_g.downstream_igmp_tci, (igmp_config_g.downstream_igmp_tci&0xE000)>>13, (igmp_config_g.downstream_igmp_tci&0x1000)>>12, igmp_config_g.downstream_igmp_tci&0xFFF);
	util_fdprintf(fd, "\tdownstream_igmp_tag_ctl: %d\n",igmp_config_g.downstream_igmp_tag_ctl);
	util_fdprintf(fd, "\tigmp_compatibity_mode: %d\n",igmp_config_g.igmp_compatibity_mode);
	if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX) == 0)
	{
		util_fdprintf(fd, "\tmax group limit: %d\n",igmp_config_g.group_limit);
		util_fdprintf(fd, "\tmax group member limit: %d\n",igmp_config_g.max_group_member);
		util_fdprintf(fd, "\tmax ssm source limit: %d\n",igmp_config_g.max_ssm_source);
	}
	if (omci_env_g.igmp_proxy_snooping == 0)
		util_fdprintf(fd, "\tigmp mode: snooping\n");
	else if (omci_env_g.igmp_proxy_snooping == 1)
		util_fdprintf(fd, "\tigmp mode: proxy without snooping\n");
	else if (omci_env_g.igmp_proxy_snooping == 2)
		util_fdprintf(fd, "\tigmp mode: proxy\n");
	return 0;
}


static int
cli_igmp_counter(int fd)
{
	if (igmpproxytask_loop_g  == 0) return 0;

	struct igmp_counter_module_t *m=NULL;
        
        list_for_each_entry(m,  &igmp_config_g.m_counter_list, m_node) {
        	m->igmp_cli_handler(fd);            
        }
        
	return 0;
}

static int
cli_igmp_counter_list(int fd)
{
	if (igmpproxytask_loop_g  == 0) return 0;

	struct igmp_counter_module_t *m=NULL;
        
        util_fdprintf(fd, "counter modules: \n");
        list_for_each_entry(m,  &igmp_config_g.m_counter_list, m_node) {
        	util_fdprintf(fd, "\t%s\n",m->module_name());           
        }
        
	return 0;
}

static int
cli_igmp_counter_reset(int fd)
{
	struct igmp_counter_module_t *m=NULL;
        
        list_for_each_entry(m,  &igmp_config_g.m_counter_list, m_node) {
        	m->reset();            
        }
        
	return 0;
}

static int MAYBE_UNUSED
cli_igmp_all_info(int fd)
{
	cli_igmp_joined_membership_list(fd);
	cli_igmp_config_list(fd);
//	cli_igmp_lastinfo(fd);
	cli_igmp_counter(fd);
	return 0;
}

static int MAYBE_UNUSED
cli_igmp_set_igmp_config_last_member_query_interval(int fd, char *last_member_query_interval)
{
	igmp_config_g.last_member_query_interval = util_atoi(last_member_query_interval); 
	return 0;
}

static int MAYBE_UNUSED
cli_igmp_set_igmp_config_robustness(int fd, char *robustness)
{
	igmp_config_g.robustness = util_atoi(robustness);
	if (igmp_config_g.robustness < 2) igmp_config_g.robustness = 2;		// robustiness must not be 0 or 1 
	return 0;
}

static int MAYBE_UNUSED
cli_igmp_set_igmp_config_igmp_version(int fd, char *igmp_version)
{
	igmp_config_g.igmp_version = util_atoi(igmp_version); 
	return 0;
}

static int MAYBE_UNUSED
cli_igmp_set_igmp_config_querier_ip(int fd, char *querier_ip)
{
	memcpy(igmp_config_g.querier_ip ,querier_ip, strlen(querier_ip)+1); 
	return 0;
}

static int MAYBE_UNUSED
cli_igmp_set_igmp_config_upstream_igmp_rate(int fd, char *upstream_igmp_rate)
{
	igmp_config_g.upstream_igmp_rate = util_atoi(upstream_igmp_rate);
        igmp_omci_update_upstream_rate(igmp_config_g.upstream_igmp_rate); 
	return 0;
}

static int MAYBE_UNUSED
cli_igmp_set_igmp_config_upstream_igmp_tci(int fd, char *upstream_igmp_tci)
{
	int tci = 0;
	tci = util_atoi(upstream_igmp_tci); 
	if (tci < 0x10000)
		igmp_config_g.upstream_igmp_tci =htons((unsigned short )tci);
	else
		igmp_config_g.upstream_igmp_tci =0xffff;
	return 0;
}

static int MAYBE_UNUSED
cli_igmp_set_igmp_config_upstream_igmp_tag_ctl(int fd, char *upstream_igmp_tag_ctl)
{
	igmp_config_g.upstream_igmp_tag_ctl = util_atoi(upstream_igmp_tag_ctl); 
	return 0;
}

static int MAYBE_UNUSED
cli_igmp_set_igmp_config_igmp_function(int fd, char *igmp_function)
{
	igmp_config_g.igmp_function = util_atoi(igmp_function); 
	/*
	if (igmpproxytask_loop_g  == 1)
	{
		if (cli_mcast_op_profile_meid)
		{
			cli_attrset(fd, 309, cli_mcast_op_profile_meid, 2 , igmp_function);
		}
	}
	*/
	return 0;
}

static int MAYBE_UNUSED
cli_igmp_set_igmp_config_immediate_leave(int fd, char *immediate_leave )
{
	igmp_config_g.immediate_leave = util_atoi(immediate_leave) ; 
	/*
	if (igmpproxytask_loop_g  == 1)
	{
		if (cli_mcast_op_profile_meid)
		{
			cli_attrset(fd, 309, cli_mcast_op_profile_meid, 3 , immediate_leave );
		}
	}
	*/
	return 0;
}

static int MAYBE_UNUSED
cli_igmp_set_igmp_config_query_interval(int fd, char *query_interval)
{
	igmp_config_g.query_interval = util_atoi(query_interval) ; 
	fwk_timer_delete( timer_id_query_interval_g );
	timer_id_query_interval_g = fwk_timer_create( igmp_timer_qid_g, TIMER_EVENT_QUERY_INTERVAL, igmp_config_g.query_interval*1000, NULL);

/*
	if (igmpproxytask_loop_g  == 0)
	{
		if (cli_mcast_op_profile_meid)
		{
			cli_attrset(fd, 309, cli_mcast_op_profile_meid, 12 , query_interval);
		}
	}
*/
	return 0;
}

static int MAYBE_UNUSED
cli_igmp_set_igmp_config_max_respon_time(int fd, char *max_respon_time)
{
	igmp_config_g.max_respon_time = util_atoi(max_respon_time) ; 
	/*
	if (igmpproxytask_loop_g  == 0)
	{
		if (cli_mcast_op_profile_meid)
		{
			cli_attrset(fd, 309, cli_mcast_op_profile_meid, 13 , max_respon_time);
		}
	}
	*/
	return 0;
}
/*
static int
cli_igmp_timer_queue_list(int fd )
{
	igmp_timer_queue_list(fd);
	return 0;
}
*/


static int MAYBE_UNUSED
cli_igmp_set_igmp_compatibity_mode(int fd, char *compatibity_mode)
{
	int mode = 0;
	mode = util_atoi(compatibity_mode);
	if (mode >= 0 && mode < 3)
	       igmp_config_g.igmp_compatibity_mode = mode;
	return 0;
}


static int MAYBE_UNUSED
cli_igmp_bridge_port_assignment_list(int fd)
{
	return 0;
}

static int MAYBE_UNUSED
cli_igmp_mcast_group_dump(int fd)
{
	cli_igmp_joined_membership_list(fd);
	return 0;
}
static int MAYBE_UNUSED
cli_igmp_clientinfo(int fd)
{
	util_fdprintf(fd,"%s\n",igmp_clientinfo_str_g);
	return 0;
}

static int MAYBE_UNUSED
cli_igmp_total_group_limit_set(int fd, char *total_group_limit)
{
	igmp_config_g.group_limit = util_atoi(total_group_limit) ; 
	return 0;
}

static int MAYBE_UNUSED
cli_igmp_total_group_member_limit_set(int fd, char *total_member_limit)
{
	igmp_config_g.max_group_member = util_atoi(total_member_limit) ; 
	return 0;
}

static int MAYBE_UNUSED
cli_igmp_ssm_source_limit_set(int fd, char *total_source_limit)
{
	igmp_config_g.max_ssm_source = util_atoi(total_source_limit) ; 
	return 0;
}

static int MAYBE_UNUSED
cli_igmp_total_group_limit_get(int fd )
{
	util_fdprintf(fd,"total group limit: %d\n",igmp_config_g.group_limit);
	return 0;
}

#endif
// helper function used by cli.c /////////////////////////////////////////////////////////////////////////////////////////////

void
cli_igmp_help(int fd)
{
	util_fdprintf(fd, 
		"igmp help|[subcmd...]\n");
}

void
cli_igmp_help_long(int fd)
{
	util_fdprintf(fd, 
		"igmp [start|stop|config|member|lastpkt|counter|allinfo]\n"
		"     config [max_response_time|query_interval|immediate_leave|igmp_function|last_member_query_interval\n"
		"                              |robustness|igmp_version|querier_ip\n"
		"                              |upstream_igmp_rate|upstream_igmp_tci|upstream_igmp_tag_ctl] [value]\n"
		"     group_dump\n"
//		"     timeout_queue\n"
//		"     bridge_assign\n"
//		"     counter\n"
//		"     counter reset\n"
		"     total_group [group_num]\n"
		"     delete [-g|-s|-c|-a] [group_ip|src_ip|client_ip|delete all member]\n"
		"     clientinfo\n"
		);
}

int 
igmp_cli_cmdline(int fd, int argc, char *argv[])
{
	if (strcmp(argv[0], "igmp") == 0)
	{
		if (argc==1)
		{
			cli_igmp_related_bridge(fd);
			return 0;
		}
#ifndef X86
		else if (argc==2 && strcmp(argv[1], "clientinfo") == 0)
		{
			cli_igmp_clientinfo(fd);
			return 0;
		}
		else if (argc==2 && strcmp(argv[1], "help") == 0)
		{
			cli_igmp_help_long(fd);
			return 0;
		}
		else if (argc==2 && strcmp(argv[1], "start") == 0)
		{
			if (igmpproxytask_loop_g == 1)
			{
				util_fdprintf(fd, "igmp task had already started\n");
			}
			else
			{
				igmp_proxy_init();
				igmp_proxy_start();
			}
			return 0;
		}
		else if (argc==2 && strcmp(argv[1], "stop") == 0)
		{
			if (igmpproxytask_loop_g == 0)
			{
				util_fdprintf(fd, "igmp task did not run\n");
			}
			else
			{
				igmp_proxy_stop();
				igmp_proxy_shutdown();
			}
			return 0;
		}
		else if (argc==2 && strcmp(argv[1], "config") == 0)
		{
			return cli_igmp_config_list(fd);
		}
		else if (argc==2 && strcmp(argv[1], "member") == 0)
		{
			return cli_igmp_joined_membership_list(fd);
//		} else if (argc==2 && strcmp(argv[1], "lastpkt") == 0) {
//			return cli_igmp_lastinfo(fd);
		}
		else if (strcmp(argv[1], "counter") == 0)
		{
			if (argc ==2)
				return cli_igmp_counter(fd);
			if (argc == 3  && strcmp(argv[2], "reset") == 0)
				return cli_igmp_counter_reset(fd);
			else if (argc == 3  && strcmp(argv[2], "list") == 0)
				return cli_igmp_counter_list(fd);
		}
		else if (argc==2 && strcmp(argv[1], "timeout_queue") == 0)
		{
//			return cli_igmp_timer_queue_list(fd);
		}
		else if (argc==2 && strcmp(argv[1], "allinfo") == 0)
		{
			return cli_igmp_all_info(fd);
		}
		else if (argc==2 && strcmp(argv[1], "bridge_assign") == 0)
		{
			return cli_igmp_bridge_port_assignment_list(fd );
		}
		else if (argc==2 && strcmp(argv[1], "total_group") == 0)
		{
			return cli_igmp_total_group_limit_get( fd );
		}
		else if (argc==2 && strcmp(argv[1], "group_dump") == 0)
		{
			return cli_igmp_mcast_group_dump(fd);
		}
		else if (argc==3 && strcmp(argv[1], "total_group") == 0)
		{
			return cli_igmp_total_group_limit_set( fd ,argv[2]);
		}
		else if (argc==3 && strcmp(argv[1], "total_group_member") == 0)
		{
			return cli_igmp_total_group_member_limit_set( fd ,argv[2]);
		}
		else if (argc==3 && strcmp(argv[1], "ssm_source_limit") == 0)
		{
			return cli_igmp_ssm_source_limit_set( fd ,argv[2]);
		}
		else if (argc==4 && strcmp(argv[1], "config") == 0)
		{
			if (strcmp(argv[2], "max_response_time") == 0)
			{
				return cli_igmp_set_igmp_config_max_respon_time(fd,argv[3]);
			}
			else if (strcmp(argv[2], "query_interval") == 0)
			{
				return cli_igmp_set_igmp_config_query_interval(fd,argv[3]);
			}
			else if (strcmp(argv[2], "immediate_leave") == 0)
			{
				return cli_igmp_set_igmp_config_immediate_leave(fd,argv[3]);
			}
			else if (strcmp(argv[2], "igmp_function") == 0)
			{
				return cli_igmp_set_igmp_config_igmp_function(fd, argv[3]);
			}
			else if (strcmp(argv[2], "last_member_query_interval") == 0)
			{
				return cli_igmp_set_igmp_config_last_member_query_interval(fd, argv[3]);
			}
			else if (strcmp(argv[2], "robustness") == 0)
			{
				return cli_igmp_set_igmp_config_robustness(fd, argv[3]);
			}
			else if (strcmp(argv[2], "igmp_version") == 0)
			{
				return cli_igmp_set_igmp_config_igmp_version(fd, argv[3]);
			}
			else if (strcmp(argv[2], "querier_ip") == 0)
			{
				return cli_igmp_set_igmp_config_querier_ip(fd, argv[3]);
			}
			else if (strcmp(argv[2], "upstream_igmp_rate") == 0)
			{
				return cli_igmp_set_igmp_config_upstream_igmp_rate(fd, argv[3]);
			}
			else if (strcmp(argv[2], "upstream_igmp_tci") == 0)
			{
				return cli_igmp_set_igmp_config_upstream_igmp_tci(fd, argv[3]);
			}
			else if (strcmp(argv[2], "upstream_igmp_tag_ctl") == 0)
			{
				return cli_igmp_set_igmp_config_upstream_igmp_tag_ctl(fd, argv[3]);
			}
			else if (strcmp(argv[2], "igmp_compatibity_mode") == 0)
			{
				return cli_igmp_set_igmp_compatibity_mode(fd, argv[3]);
			}
		}
		else if (argc>=3 && strcmp(argv[1], "delete") == 0)
		{
			return cli_igmp_delete_membership(fd, argv[2], argv[3]);
		}
		else if (argc == 2 && strcmp(argv[1], "reload") == 0)
		{
			return igmp_proxy_reload();
		}
		else
		{
			return CLI_ERROR_OTHER_CATEGORY;	// ret CLI_ERROR_OTHER_CATEGORY as diag also has cli 'igmp'
		}
		return CLI_ERROR_SYNTAX;
#else
		return CLI_OK;
#endif
	}
	else
	{
		return CLI_ERROR_OTHER_CATEGORY;
	}
}
