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
 * File    : cli_bridge.c
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
#include "er_group.h"
#include "omciutil_vlan.h"
#include "cli.h"
#include "me_related.h"
#include "hwresource.h"

static int
cli_bridge_port_related_vlan_translation(int fd, struct me_t *me_bport)
{
	struct list_head rule_list_us, rule_list_ds;
	struct omciutil_vlan_rule_fields_t *rule_fields_pos;

	if (omciutil_vlan_collect_rules_by_port(ER_ATTR_GROUP_HW_OP_NONE, me_bport, NULL, &rule_list_us, &rule_list_ds, 0) < 0) {
		return -1;
	}
	list_for_each_entry(rule_fields_pos, &rule_list_us, rule_node) {
		util_fdprintf(fd, "\n\t\tUS %c%s", rule_fields_pos->unreal_flag? '*': ' ',omciutil_vlan_str_desc(rule_fields_pos, "\n\t\t\t", "", 0));
	}
	list_for_each_entry(rule_fields_pos, &rule_list_ds, rule_node) {
		util_fdprintf(fd, "\n\t\tDS %c%s", rule_fields_pos->unreal_flag? '*': ' ', omciutil_vlan_str_desc(rule_fields_pos, "\n\t\t\t", "", 0));
	}

	//release all in list
	omciutil_vlan_release_rule_list(&rule_list_us);
	omciutil_vlan_release_rule_list(&rule_list_ds);
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
			util_fdprintf(fd, "\n\t\t\t\t>[134]iphost:0x%x,dev=%s", me->meid, devname?devname:"");
			util_fdprintf(fd, ",static_ip=%s", inet_ntoa(in_static));
			util_fdprintf(fd, ",dhcp_ip=%s", inet_ntoa(in_dhcp));
		}
	}
	return CLI_OK;
}

static int
cli_veip_related_tcpudp(int fd, struct me_t *me_veip)
{
	unsigned char get_attr_mask[2] = {0xff, 0xff};
	struct meinfo_t *miptr=meinfo_util_miptr(136);
	struct me_t *me;

	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		if (me_related(me_veip, me)) {
			unsigned short ip_proto=(unsigned short)meinfo_util_me_attr_data(me, 2);
			unsigned short portnum=(unsigned short)meinfo_util_me_attr_data(me, 1);
			meinfo_me_refresh(me, get_attr_mask);
			util_fdprintf(fd, "\n\t\t\t>[136]tcpudp:0x%x,", me->meid);
			if (ip_proto==0x11) {
				util_fdprintf(fd, "udp:%d", portnum);
			} else if (ip_proto==0x6) {
				util_fdprintf(fd, "tcp:%d", portnum);
			} else {
				util_fdprintf(fd, "ip proto 0x%x:%d", ip_proto, portnum);
			}
			cli_tcpudp_related_iphost(fd, me);
		}
	}
	return CLI_OK;
}

static int
cli_veip_related_pq(int fd, struct me_t *me_veip)
{
	unsigned char get_attr_mask[2] = {0xff, 0xff};
	struct meinfo_t *miptr=meinfo_util_miptr(277);
	struct me_t *me;
	int pq_total=0;

	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		if (me_related(me, me_veip)) {
			meinfo_me_refresh(me, get_attr_mask);
			if (pq_total==0)
				util_fdprintf(fd, "\n\t\t\t<[277]pq:0x%x", me->meid);
			else
				util_fdprintf(fd, ",0x%x", me->meid);
			pq_total++;
		}
	}
	return CLI_OK;
}

static int
cli_gem_related_traffic_descriptor(int fd, struct me_t *me_gem)
{
	unsigned char get_attr_mask[2] = {0xff, 0xff};
	struct meinfo_t *miptr=meinfo_util_miptr(280);
	struct me_t *me;
	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		if (me_related(me_gem, me)) {
			unsigned int cir = (unsigned int)meinfo_util_me_attr_data(me, 1);
			unsigned int pir = (unsigned int)meinfo_util_me_attr_data(me, 2);
			meinfo_me_refresh(me, get_attr_mask);
			util_fdprintf(fd, "\n\t\t\t\t\t\t\t>[280]traffic_desc:0x%x,cir=%uB/s(%s),pir=%uB/s(%s)", me->meid, 
				cir, util_bps_to_ratestr(cir*8ULL), pir, util_bps_to_ratestr(pir*8ULL));
			if ((unsigned short)meinfo_util_me_attr_data(me_gem, 5)==me->meid)
				util_fdprintf(fd, ",US");
			else
				util_fdprintf(fd, ",DS");
		}
	}
	return CLI_OK;
}

static int
cli_pq_related_traffic_descriptor(int fd, struct me_t *me_pq)
{
	unsigned char get_attr_mask[2] = {0xff, 0xff};
	struct meinfo_t *miptr=meinfo_util_miptr(280);
	struct me_t *me, *meptr=meinfo_me_get_by_meid(65323, me_pq->meid);

	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		if (meptr && me_related(meptr, me)) {
			unsigned int cir = (unsigned int)meinfo_util_me_attr_data(me, 1);
			unsigned int pir = (unsigned int)meinfo_util_me_attr_data(me, 2);
			meinfo_me_refresh(me, get_attr_mask);
			util_fdprintf(fd, "\n\t\t\t\t\t\t\t>[280]traffic_desc:0x%x,cir=%uB/s(%s),pir=%uB/s(%s)", me->meid, 
				cir, util_bps_to_ratestr(cir*8ULL), pir, util_bps_to_ratestr(pir*8ULL));
			util_fdprintf(fd, ",US");
		}
	}
	return CLI_OK;
}

static int
cli_gem_related_pq(int fd, struct me_t *me_gem)
{
	unsigned char get_attr_mask[2] = {0xff, 0xff};
	struct meinfo_t *miptr=meinfo_util_miptr(277);
	struct me_t *me;
	char *devname;
	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		if (me_related(me_gem, me)) {
			meinfo_me_refresh(me, get_attr_mask);
			util_fdprintf(fd, "\n\t\t\t\t\t\t>[277]pq:0x%x,%s", 
					me->meid, (me->meid&0x8000)?"US":"DS");
			if ((devname=hwresource_devname(me))!=NULL)
				util_fdprintf(fd, " (%s)", devname); 
			if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_ALU)==0)
				cli_pq_related_traffic_descriptor(fd, me);
		}
	}
	return CLI_OK;
}

static int
cli_gem_related_tcont(int fd, struct me_t *me_gem)
{
	unsigned char get_attr_mask[2] = {0xff, 0xff};
	struct meinfo_t *miptr=meinfo_util_miptr(262);
	struct me_t *me;
	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		if (me_related(me_gem, me)) {
			meinfo_me_refresh(me, get_attr_mask);
			util_fdprintf(fd, " >[262]tcont:0x%x,allocid=0x%x", 
				me->meid, (unsigned short)meinfo_util_me_attr_data(me,1));
		}
	}
	return CLI_OK;
}

static int
cli_iwtp_related_gem(int fd, struct me_t *me_iwtp)
{
	unsigned char get_attr_mask[2] = {0xff, 0xff};
	struct meinfo_t *miptr=meinfo_util_miptr(268);
	struct me_t *me;
	char *devname;

	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		if (me_related(me_iwtp, me)) {
			meinfo_me_refresh(me, get_attr_mask);
			util_fdprintf(fd, " >[268]gem:0x%x,portid=0x%x", 
				me->meid, (unsigned short)meinfo_util_me_attr_data(me, 1));
			if ((devname=hwresource_devname(me))!=NULL)
				util_fdprintf(fd, " (%s)", devname); 

			cli_gem_related_tcont(fd, me);
			cli_gem_related_traffic_descriptor(fd, me);
			cli_gem_related_pq(fd, me);
		}
	}
	return CLI_OK;
}

static int
cli_mapper_related_gem_iwtp(int fd, struct me_t *me_mapper)
{
	unsigned char get_attr_mask[2] = {0xff, 0xff};
	struct meinfo_t *miptr=meinfo_util_miptr(266);
	struct me_t *me;
	int i;

	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		if (me_related(me_mapper, me)) {
			meinfo_me_refresh(me, get_attr_mask);
			for (i=0; i<8; i++) {
				if (meinfo_util_me_attr_data(me_mapper, i+2)==me->meid) { 
					util_fdprintf(fd, "\n\t\tpbit=%d <>[266]iwtp:0x%x", i, me->meid);
					cli_iwtp_related_gem(fd, me);
				}
			}
		}
	}
	return CLI_OK;
}

static int
cli_uni_related_pq(int fd, struct me_t *me_uni)
{
	unsigned char get_attr_mask[2] = {0xff, 0xff};
	struct meinfo_t *miptr=meinfo_util_miptr(277);
	struct me_t *me;
	int pq_total=0;

	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		if (me_related(me, me_uni)) {
			meinfo_me_refresh(me, get_attr_mask);
			if (pq_total==0)
				util_fdprintf(fd, "\n\t\t\t<[277]pq:0x%x", me->meid);
			else
				util_fdprintf(fd, ",0x%x", me->meid);
			pq_total++;
		}
	}
	return CLI_OK;
}

static int
cli_bridge_port_related_me(int fd, struct me_t *me_bridge_port)
{
	unsigned short related_classid=meinfo_me_attr_pointer_classid(me_bridge_port, 4);
	unsigned short related_pointer=meinfo_util_me_attr_data(me_bridge_port, 4);

	unsigned char get_attr_mask[2] = {0xff, 0xff};
	struct meinfo_t *miptr=meinfo_util_miptr(related_classid);
	struct me_t *me=meinfo_me_get_by_meid(related_classid, related_pointer);
	char *devname;

	if (me) {
		meinfo_me_refresh(me, get_attr_mask);
		switch (related_classid) {
		case 11:
			util_fdprintf(fd, "\n\t\t>[11]uni:0x%x,admin=%d,op=%d,ip_ind=%d", related_pointer, 
				(unsigned char)meinfo_util_me_attr_data(me, 5), 
				(unsigned char)meinfo_util_me_attr_data(me, 6),
				(unsigned char)meinfo_util_me_attr_data(me, 11));
			if ((devname=hwresource_devname(me))!=NULL)
				util_fdprintf(fd, " (%s)", devname); 

			cli_uni_related_pq(fd, me);
			break;
		case 130:
			util_fdprintf(fd, "\n\t\t>[130]mapper:0x%x", related_pointer);
			cli_mapper_related_gem_iwtp(fd, me);
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
			cli_iwtp_related_gem(fd, me);
			break;
		case 281:
			util_fdprintf(fd, "\n\t\t>[281]mcast_iwtp:0x%x", related_pointer);
			cli_iwtp_related_gem(fd, me);
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
			cli_veip_related_pq(fd, me);
			cli_veip_related_tcpudp(fd, me);
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
cli_bridge_port_related_traffic_descriptor(int fd, struct me_t *me_bridge_port)
{
	unsigned char get_attr_mask[2] = {0xff, 0xff};
	struct meinfo_t *miptr=meinfo_util_miptr(280);
	struct me_t *me;

	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		if (me_related(me_bridge_port, me)) {
			unsigned int cir = (unsigned int)meinfo_util_me_attr_data(me, 1);
			unsigned int pir = (unsigned int)meinfo_util_me_attr_data(me, 2);
			meinfo_me_refresh(me, get_attr_mask);
			util_fdprintf(fd, "\n\t\t>[280]traffic_desc:0x%x,cir=%uB/s(%s),pir=%uB/s(%s)", me->meid, 
				cir, util_bps_to_ratestr(cir*8ULL), pir, util_bps_to_ratestr(pir*8ULL));
			if ((unsigned short)meinfo_util_me_attr_data(me_bridge_port, 11)==me->meid)
				util_fdprintf(fd, ",OUT");
			if ((unsigned short)meinfo_util_me_attr_data(me_bridge_port, 12)==me->meid)
				util_fdprintf(fd, ",IN");
		}
	}
	return CLI_OK;
}

static int
cli_bridge_port_related_filter(int fd, struct me_t *me_bridge_port)
{
	unsigned char get_attr_mask[2] = {0xff, 0xff};
	struct meinfo_t *miptr=meinfo_util_miptr(84);
	struct me_t *me;

	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		if (me_related(me_bridge_port, me)) {
			unsigned char count, i;
			unsigned char *vidlist=meinfo_util_me_attr_ptr(me, 1);
			meinfo_me_refresh(me, get_attr_mask);
			util_fdprintf(fd, "\n\t\t-[84]filter:0x%x,op=0x%x", 
				me->meid, (unsigned char)meinfo_util_me_attr_data(me, 2));
			if ((count=meinfo_util_me_attr_data(me, 3))>0 && vidlist != NULL) {
				util_fdprintf(fd, ",vid="); 
				for (i=0; i<count; i++) {
					unsigned short vid = util_bitmap_get_value(vidlist, 24*8, i*16+4, 12);
					util_fdprintf(fd, "%s%u", (i>0)?",":"", vid); 
				}
			}
		}
	}
	return CLI_OK;
}

static int
cli_bridge_port_related_vtag(int fd, struct me_t *me_bridge_port)
{
	unsigned short related_classid=meinfo_me_attr_pointer_classid(me_bridge_port, 4);
	unsigned short related_pointer=meinfo_util_me_attr_data(me_bridge_port, 4);
	struct me_t *me_pointed_by_bridge_port=meinfo_me_get_by_meid(related_classid, related_pointer);

	unsigned char get_attr_mask[2] = {0xff, 0xff};
	struct meinfo_t *miptr=meinfo_util_miptr(78);
	struct me_t *me;

	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		int related=0, indirect=0;
		if (me_related(me, me_bridge_port)) {
			related = 1;
		} else if (me_pointed_by_bridge_port && me_related(me, me_pointed_by_bridge_port)) {
			related = 1; indirect = 1;
		}
		if (related) {
			unsigned char *tci, pri, de;
			unsigned short vid;
			meinfo_me_refresh(me, get_attr_mask);
			if ((tci=me->attr[2].value.ptr) != NULL)
			{
				pri=util_bitmap_get_value(tci, 16, 0, 3);
				de=util_bitmap_get_value(tci, 16, 3, 1);
				vid=util_bitmap_get_value(tci, 16, 4, 12);
			} else {
				pri = 0;
				de = 0;
				vid = 0;
			}
			util_fdprintf(fd, "\n\t\t<[78]vtag:0x%x,us_mode=%d,tci=(%u,%u,%u),ds_mode=%d", 
				me->meid, 
				(unsigned char)meinfo_util_me_attr_data(me, 1),
				pri, de, vid,
				(unsigned char)meinfo_util_me_attr_data(me, 3));
			if (indirect)
				util_fdprintf(fd, " (through [%d])", related_classid);
		}
	}
	return CLI_OK;
}

static int
cli_bridge_port_related_extvtag(int fd, struct me_t *me_bridge_port)
{
	unsigned short related_classid=meinfo_me_attr_pointer_classid(me_bridge_port, 4);
	unsigned short related_pointer=meinfo_util_me_attr_data(me_bridge_port, 4);
	struct me_t *me_pointed_by_bridge_port=meinfo_me_get_by_meid(related_classid, related_pointer);

	unsigned char get_attr_mask[2] = {0xff, 0xff};
	struct meinfo_t *miptr=meinfo_util_miptr(171);
	struct me_t *me;

	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		int related=0, indirect=0;
		if (me_related(me, me_bridge_port)) {
			related = 1;
		} else if (me_pointed_by_bridge_port && me_related(me, me_pointed_by_bridge_port)) {
			related = 1; indirect = 1;
		}
		if (related) {
			meinfo_me_refresh(me, get_attr_mask);
			util_fdprintf(fd, "\n\t\t<[171]extVtag:0x%x,ds=%d", 
				me->meid, (unsigned char)meinfo_util_me_attr_data(me, 5));
			if (indirect)
				util_fdprintf(fd, " (through [%d])", related_classid);
		}
	}
	return CLI_OK;
}

static int
cli_bridge_related_port(int fd, struct me_t *me_bridge)
{
	unsigned char get_attr_mask[2] = {0xff, 0xff};
	struct meinfo_t *miptr=meinfo_util_miptr(47);
	struct me_t *me;
	char *devname;

	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		if (me_related(me, me_bridge)) {
			meinfo_me_refresh(me, get_attr_mask);
			util_fdprintf(fd, "\n\t<[47]bport:0x%x,is_private=%d", me->meid, me->is_private); 
			if ((devname=hwresource_devname(me))!=NULL)
				util_fdprintf(fd, " (%s)", devname); 

			cli_bridge_port_related_traffic_descriptor(fd, me);
			cli_bridge_port_related_filter(fd, me);
			cli_bridge_port_related_vtag(fd, me);
			cli_bridge_port_related_extvtag(fd, me);
			cli_bridge_port_related_vlan_translation(fd, me);
			cli_bridge_port_related_me(fd, me);
		}
	}
	return CLI_OK;
}

// meid: 0xffff: show all, other: show specific
int
cli_bridge(int fd, unsigned short meid)
{
	unsigned char get_attr_mask[2] = {0xff, 0xff};
	struct meinfo_t *miptr=meinfo_util_miptr(45);
	struct me_t *me;

	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		if (meid==0xffff || meid==me->meid) {
			meinfo_me_refresh(me, get_attr_mask);
			util_fdprintf(fd, "[45]bridge:0x%x,stp=%d,learn=%d,local_bridging=%d,uknown_discard=%d,uni_mac_depth=%d", 
				me->meid, 
				(unsigned char)meinfo_util_me_attr_data(me, 1),
				(unsigned char)meinfo_util_me_attr_data(me, 2),
				(unsigned char)meinfo_util_me_attr_data(me, 3),
				(unsigned char)meinfo_util_me_attr_data(me, 8),
				(unsigned char)meinfo_util_me_attr_data(me, 9));
			cli_bridge_related_port(fd, me);
			util_fdprintf(fd, "\n"); 
		}
	}
	return CLI_OK;
}

// helper function used by cli.c /////////////////////////////////////////////////////////////////////////////////////////////

void
cli_bridge_help(int fd)
{
	util_fdprintf(fd, 
		"tcont|bridge|pots|unrelated [meid|*]\n");
}

int
cli_bridge_cmdline(int fd, int argc, char *argv[])
{
	unsigned short meid;

	if (strcmp(argv[0], "bridge")==0 && argc <=2) {
		if (argc==1) {
			return cli_bridge(fd, 0xffff);
		} else if (argc==2) {
			meid = util_atoi(argv[1]);
			return cli_bridge(fd, meid);
		}
		return CLI_ERROR_SYNTAX;
	} else {
		return CLI_ERROR_OTHER_CATEGORY;
	}
}

