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
 * File    : cli_tcont.c
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
#include "hwresource.h"
#include "tm.h"

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
			util_fdprintf(fd, "\n\t\t\t\t\t\t\t\t>[280]traffic_desc:0x%x,cir=%uB/s(%s),pir=%uB/s(%s)", me->meid, 
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
			util_fdprintf(fd, "\n\t\t\t\t\t\t\t\t-[84]filter:0x%x,op=0x%x", 
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
cli_mapper_related_bridge_port(int fd, struct me_t *me_mapper)
{
	unsigned char get_attr_mask[2] = {0xff, 0xff};
	struct meinfo_t *miptr=meinfo_util_miptr(47);	// mapper
	struct me_t *me;
	char *devname;

	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		if (me_related(me, me_mapper)) {
			meinfo_me_refresh(me, get_attr_mask);
			util_fdprintf(fd, "\n\t\t\t\t\t\t\t<[47]bport:0x%x,is_private=%d", me->meid, me->is_private);
			if ((devname=hwresource_devname(me))!=NULL)
				util_fdprintf(fd, " (%s)", devname); 
			cli_bridge_port_related_traffic_descriptor(fd, me);
			cli_bridge_port_related_filter(fd, me);
		}
	}
	return CLI_OK;
}

static int
cli_iwtp_related_mapper(int fd, struct me_t *me_iwtp)
{
	unsigned char get_attr_mask[2] = {0xff, 0xff};
	struct meinfo_t *miptr=meinfo_util_miptr(130);	// mapper
	struct me_t *me;
	int i;

	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		if (me_related(me, me_iwtp)) {
			meinfo_me_refresh(me, get_attr_mask);
			util_fdprintf(fd, " <>[130]mapper:0x%x,pbit=", me->meid);
			for (i=0; i<8; i++) {
				if (meinfo_util_me_attr_data(me, i+2)==me_iwtp->meid)
					util_fdprintf(fd, "%d", i);
			}
			cli_mapper_related_bridge_port(fd, me);
		}
	}
	return CLI_OK;
}

static int
cli_iwtp_related_bridge_port(int fd, struct me_t *me_iwtp)
{
	unsigned char get_attr_mask[2] = {0xff, 0xff};
	struct meinfo_t *miptr=meinfo_util_miptr(47);	// mapper
	struct me_t *me;
	char *devname;

	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		if (me_related(me, me_iwtp)) {
			meinfo_me_refresh(me, get_attr_mask);
			util_fdprintf(fd, "\n\t\t\t\t\t<[47]bport:0x%x,is_private=%d", me->meid, me->is_private);
			if ((devname=hwresource_devname(me))!=NULL)
				util_fdprintf(fd, " (%s)", devname); 
		}
	}
	return CLI_OK;
}

static int
cli_gem_related_iwtp(int fd, struct me_t *me_gem)
{
	unsigned char get_attr_mask[2] = {0xff, 0xff};
	struct meinfo_t *miptr=meinfo_util_miptr(266);	// iwtp
	struct meinfo_t *miptr_mcast=meinfo_util_miptr(281);	// mcast iwtp
	struct me_t *me;

	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		if (me_related(me, me_gem)) {
			meinfo_me_refresh(me, get_attr_mask);
			util_fdprintf(fd, "\n\t\t\t\t<[266]iwtp:0x%x", me->meid);
			cli_iwtp_related_mapper(fd, me);
			cli_iwtp_related_bridge_port(fd, me);
		}
	}
	list_for_each_entry(me, &miptr_mcast->me_instance_list, instance_node) {
		if (me_related(me, me_gem)) {
			meinfo_me_refresh(me, get_attr_mask);
			util_fdprintf(fd, "\n\t\t\t\t<[281]mcast_iwtp:0x%x", me->meid);
			cli_iwtp_related_mapper(fd, me);
			cli_iwtp_related_bridge_port(fd, me);
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
			util_fdprintf(fd, "\n\t\t\t\t>[280]traffic_desc:0x%x,cir=%uB/s(%s),pir=%uB/s(%s)", me->meid, 
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
			util_fdprintf(fd, "\n\t\t\t>[280]traffic_desc:0x%x,cir=%uB/s(%s),pir=%uB/s(%s)", me->meid, 
				cir, util_bps_to_ratestr(cir*8ULL), pir, util_bps_to_ratestr(pir*8ULL));
			util_fdprintf(fd, ",US");
		}
	}
	return CLI_OK;
}

static int
cli_pq_related_gem(int fd, struct me_t *me_pq)
{
	unsigned char get_attr_mask[2] = {0xff, 0xff};
	struct meinfo_t *miptr=meinfo_util_miptr(268);
	struct me_t *me;
	char *devname;

	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		if (me_related(me, me_pq)) {
			meinfo_me_refresh(me, get_attr_mask);
			util_fdprintf(fd, "\n\t\t\t<[268]gem:0x%x,portid=0x%x", 
				me->meid, (unsigned short)meinfo_util_me_attr_data(me, 1));
			if ((devname=hwresource_devname(me))!=NULL)
				util_fdprintf(fd, " (%s)", devname); 

			{
				int is_yellow = 0;
				int tcont_id, queue_id, is_policy_wrr, hw_weight;
				if (tm_get_tcont_id_phy_queue_info_by_me_pq_gem(me_pq, me, is_yellow, &tcont_id, &queue_id, &is_policy_wrr, &hw_weight)>0) {
					if (is_policy_wrr)
						util_fdprintf(fd, " (hw tcont_id=%d, qid=%d, WRR=%d)", tcont_id, queue_id, hw_weight); 
					else
						util_fdprintf(fd, " (hw tcont_id=%d, qid=%d, SP)", tcont_id, queue_id); 
				} else {
					util_fdprintf(fd, " (hw pq unused)"); 
				}
			}

			cli_gem_related_traffic_descriptor(fd, me);
			cli_gem_related_iwtp(fd, me);
		}
	}
	return CLI_OK;
}

static int
is_pq_related_by_gem(struct me_t *me_pq)
{
	struct meinfo_t *miptr=meinfo_util_miptr(268);
	struct me_t *me;

	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		if (me_related(me, me_pq))
			return 1;
	}
	return 0;
}

static int
is_ts_related_by_gem(struct me_t *me_ts)
{
	struct meinfo_t *miptr_gem=meinfo_util_miptr(268);
	struct meinfo_t *miptr_pq=meinfo_util_miptr(277);
	struct me_t *me_gem, *me_pq;

	list_for_each_entry(me_gem, &miptr_gem->me_instance_list, instance_node) {
		list_for_each_entry(me_pq, &miptr_pq->me_instance_list, instance_node) {
			if (me_related(me_gem, me_pq) && me_related(me_pq, me_ts))
				return 1;
		}
	}
	return 0;
}

static int
ts_has_upstream_pq(struct me_t *me_ts)
{
  	struct meinfo_t *miptr=meinfo_util_miptr(277);
	struct me_t *me;
	int total=0;

	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		if (me->meid & 0x8000 &&	// upstream
			me_related(me, me_ts)) {
			total++;
		}
	}
	return total;
}

static int
cli_ts_related_pq(int fd, struct me_t *me_ts, int show_active_only)
{
	unsigned char get_attr_mask[2] = {0xff, 0xff};
	struct meinfo_t *miptr=meinfo_util_miptr(277);
	struct me_t *me;

	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		if (me_related(me, me_ts)) {
			unsigned int related_port;
			if (show_active_only && !is_pq_related_by_gem(me))
				continue;
			meinfo_me_refresh(me, get_attr_mask);
			related_port=ntohl(*(unsigned int*)meinfo_util_me_attr_ptr(me, 6));
			util_fdprintf(fd, "\n\t\t<[277]pq:0x%x", me->meid);
			if (meinfo_util_me_attr_data(me_ts, 3)==2) {	// ts is wrr
				util_fdprintf(fd, ",weight=%d", (unsigned char)meinfo_util_me_attr_data(me, 8));
			} else {					// ts is sp
				util_fdprintf(fd, ",prio=%d", (unsigned short)(related_port & 0xffff));
			}

			{
				char *devname;
				if ((devname=hwresource_devname(me))!=NULL)
					util_fdprintf(fd, " (%s)", devname); 
			}
			if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_ALU)==0)
				cli_pq_related_traffic_descriptor(fd, me);
			cli_pq_related_gem(fd, me);
		}
	}
	return CLI_OK;
}


static int
cli_tcont_related_ts(int fd, struct me_t *me_tcont, int show_active_only)
{
	unsigned char get_attr_mask[2] = {0xff, 0xff};
	struct meinfo_t *miptr=meinfo_util_miptr(278);
	struct me_t *me;

	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		if (me_related(me, me_tcont) && ts_has_upstream_pq(me)) {
			if (show_active_only && !is_ts_related_by_gem(me))
				continue;
			meinfo_me_refresh(me, get_attr_mask);
			util_fdprintf(fd, "\n\t<[278]ts:0x%x,%s", 
				me->meid, (meinfo_util_me_attr_data(me, 3)==2)?"WRR":"SP");
			if (meinfo_util_me_attr_data(me_tcont, 3)==2) {	// tcont is wrr
				util_fdprintf(fd, ",weight=%d", (unsigned char)meinfo_util_me_attr_data(me, 4));
			} else {					// tcont is sp
				util_fdprintf(fd, ",prio=%d", (unsigned char)meinfo_util_me_attr_data(me, 4));
			}
#if 0
			if ((devname=hwresource_devname(me))!=NULL)
				util_fdprintf(fd, " (%s)", devname); 
#endif
			cli_ts_related_pq(fd, me, show_active_only);
		}
	}
	return CLI_OK;
}

// meid: 0xffff: show all, other: show specific
int
cli_tcont(int fd, unsigned short meid, int show_active_only)
{
	unsigned char get_attr_mask[2] = {0xff, 0xff};
	struct meinfo_t *miptr=meinfo_util_miptr(262);
	struct me_t *me;
	unsigned short allocid;
	int schid;
	char *devname;

	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		if (meid==0xffff || meid==me->meid) {
			meinfo_me_refresh(me, get_attr_mask);
			if (show_active_only && meinfo_util_me_attr_data(me, 1)==0xff)
				continue;

			allocid = (unsigned short)meinfo_util_me_attr_data(me, 1);

			if(allocid==(unsigned short)0xffff) continue;  // 221013 LEO suppress unused tcont print

			util_fdprintf(fd, "[262]t-cont:0x%x,%s,allocid=0x%x", 
				me->meid, (meinfo_util_me_attr_data(me, 3)==2)?"WRR":"SP", 
				allocid);

			if ((devname=hwresource_devname(me))!=NULL) {
				if (gpon_hw_g.tm_tcont_get_by_allocid(allocid, &schid)>=0)
					util_fdprintf(fd, " (%s, tcont_id=%d)", devname, schid); 
				else
					util_fdprintf(fd, " (%s, unused)", devname); 
			}

			cli_tcont_related_ts(fd, me, show_active_only);
			util_fdprintf(fd, "\n");
		}
	}
	return CLI_OK;
}

// helper function used by cli.c /////////////////////////////////////////////////////////////////////////////////////////////

int
cli_tcont_cmdline(int fd, int argc, char *argv[])
{
	unsigned short meid;

	if (strcmp(argv[0], "tcont")==0 && argc <=2) {
		if (argc==1) {
			return cli_tcont(fd, 0xffff, 1);
		} else if (argc==2) {
			if (strcmp(argv[1], "*")==0) {
				return cli_tcont(fd, 0xffff, 0);
			} else {
				meid = util_atoi(argv[1]);
				return cli_tcont(fd, meid, 0);
			}
		}

		return CLI_ERROR_SYNTAX;
	} else {
		return CLI_ERROR_OTHER_CATEGORY;
	}
}

