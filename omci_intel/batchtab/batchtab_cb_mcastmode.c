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
 * Module  : batchtab
 * File    : batchtab_cb_mcastmode.c
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
#include "util_run.h"
#include "list.h"
#include "batchtab_cb.h"
#include "batchtab.h"
#include "hwresource.h"
#include "meinfo.h"
#include "switch.h"
#include "omciutil_misc.h"
#include "omcimsg.h"
#include "tm.h"

#include "mcast_const.h"
#include "mcast_pkt.h"
#include "igmp_mbship.h"
#include "mcast.h"
#include "mcast_switch.h"
#include "mcast_timer.h"
#include "igmp_omci.h"

#include "metacfg_adapter.h"


static struct mcastmode_info_t mcastmode_info_g;



////////////////////////////////////////////////////

static int
batchtab_cb_mcastmode_init(void)
{
	memset(&mcastmode_info_g, 0x00, sizeof(struct mcastmode_info_t));
	mcastmode_info_g.mcast_mode_prev = 0xff;	// mark an impossible value to prev state
	mcastmode_info_g.mc_ext_vlan_cnt = 0;
	return 0;
}

static int
batchtab_cb_mcastmode_finish(void)
{
	memset(&mcastmode_info_g, 0x00, sizeof(struct mcastmode_info_t));
	return 0;
}

static int
is_valid_gem_iwtp_meid(unsigned short gem_iwtp_meid)
{
	struct me_t *me_iwtp, *me_gem_ctp, *me_tcont;
	struct attr_value_t attr1, attr2;
	
	me_iwtp = meinfo_me_get_by_meid(266, gem_iwtp_meid); //me 266
	if (me_iwtp == NULL)
		return 0;
	
	meinfo_me_attr_get(me_iwtp, 1, &attr1); // gem ctp pointer	
	me_gem_ctp = meinfo_me_get_by_meid(268, (unsigned short)attr1.data);
	if (me_gem_ctp == NULL)
		return 0;

	meinfo_me_attr_get(me_gem_ctp, 2, &attr2); // tcont pointer
	me_tcont = meinfo_me_get_by_meid(262, (unsigned short)attr2.data);
	if (me_tcont == NULL)
		return 0;
	
	return 1;
}

static int
get_datagem_total(void)
{
	struct meinfo_t *miptr = meinfo_util_miptr(47);	// bridge port me
	struct me_t *me_bport, *me_mapper;
	struct attr_value_t attr3, attr4, attr;
	int i, datagem_total = 0;

	list_for_each_entry(me_bport, &miptr->me_instance_list, instance_node) {
		meinfo_me_attr_get(me_bport, 3, &attr3); // tp type
		meinfo_me_attr_get(me_bport, 4, &attr4); // tp pointer

		if (attr3.data == 3) {	// 1p mapper 130
			me_mapper = meinfo_me_get_by_meid(130, (unsigned short)attr4.data ); 
			if (me_mapper == NULL)
				continue;
			for (i = 2; i<=9; i++) {
				meinfo_me_attr_get(me_mapper, i, &attr); // gem iwtp pointer
				if (is_valid_gem_iwtp_meid((unsigned short)attr.data))
					datagem_total++;
			}
		} else if (attr3.data == 5) {	// gem iwtp 266
			if (is_valid_gem_iwtp_meid((unsigned short)attr4.data))	
				datagem_total++;
		}
	}
	return datagem_total;
}

static int
get_mcastgem_total(unsigned short mcastgem_list[], int list_size)
{
	struct meinfo_t *miptr = meinfo_util_miptr(47);	// bridge port me
	struct me_t *me_bport, *me_mcastgem_iwtp, *me_gem_ctp;
	struct attr_value_t attr1, attr3, attr4;
	int mcastgem_total = 0;

	list_for_each_entry(me_bport, &miptr->me_instance_list, instance_node) {

		meinfo_me_attr_get(me_bport, 3, &attr3); // tp type
		if (attr3.data == 6) {	// 6 means Multicast GEM interworking termination point
			meinfo_me_attr_get(me_bport, 4, &attr4); // tp pointer
			me_mcastgem_iwtp = meinfo_me_get_by_meid(281, (unsigned short)attr4.data); // 281 means mcast gem iwtp
			if (me_mcastgem_iwtp == NULL)
				continue;
		
			meinfo_me_attr_get(me_mcastgem_iwtp, 1, &attr1); // gem ctp pointer
			me_gem_ctp = meinfo_me_get_by_meid(268, (unsigned short)attr1.data);	// 268 means gem ctp
			if (me_gem_ctp == NULL)
				continue;
				
			if (mcastgem_total < list_size) {
				// gem ctp attr1 is gem portid
				mcastgem_list[mcastgem_total] = meinfo_util_me_attr_data(me_gem_ctp, 1);
			}
			mcastgem_total++;
		}
	}
	return mcastgem_total;
}

static int
collect_mcast_subscriber_related_tp(struct mcastmode_info_t *m)
{
	struct meinfo_t *miptr310 = meinfo_util_miptr(310);	// mcast_subscriber me
	struct meinfo_t *miptr47 = meinfo_util_miptr(47);	// bridge port me
	struct me_t *subscriber_me, *bport_me, *tp_me;
	int i;	
	
	for (i=0; i< MCASTMODE_SUBSCRIBER_MAX; i++) {
		m->subscriber_meid[i] = 0xffff;
		m->bport_meid[i] = 0xffff;
		m->tp_classid[i] = 0;
		m->tp_meid[i] = 0xffff;
	}
	
	i = 0;
	list_for_each_entry(subscriber_me, &miptr310->me_instance_list, instance_node) {
		 m->subscriber_meid[i] = subscriber_me->meid;
		 bport_me = meinfo_me_get_by_meid(47, subscriber_me->meid);
		 if (bport_me) {
		 	m->bport_meid[i] = bport_me->meid;
		 	m->tp_classid[i] = miptr47->callback.get_pointer_classid_cb(bport_me, 4);
		 	tp_me = meinfo_me_get_by_meid(m->tp_classid[i], (unsigned short)meinfo_util_me_attr_data(bport_me, 4));
		 	if (tp_me)
		 		 m->tp_meid[i] = tp_me->meid;
		}
		i++;
		if (i == MCASTMODE_SUBSCRIBER_MAX)
			break;
	}
	return i;
}

static int
batchtab_cb_mcastmode_gen(void **table_data)
{
	struct mcastmode_info_t *m = &mcastmode_info_g;
	int i;
	static int defer_count = 0;

	// skip mcastmode if mibreset till now < 20sec
	if (omcimsg_mib_reset_finish_time_till_now() < 20) {
		// keep the trigger for future and do nothing for now
		if (defer_count == 0)
			dbprintf_bat(LOG_ERR, "DEFER mcastmode gen\n");
		defer_count++;
		// return -1 to make this table gen as error, so batchtab framework will keep retry this table gen
		return -1;
	}
	if (defer_count) {
		defer_count = 0;
		dbprintf_bat(LOG_ERR, "resume DEFERRED mcastmode gen\n");
	}	
	
	m->datagem_total = get_datagem_total();
	m->mcastgem_total = get_mcastgem_total(m->mcastgem_list, MCASTMODE_MCASTGEM_MAX);
	// find flowid for per mcast gemportid
	// ps: the hw flowid to mcast gemport mapping is treated as a software state for bat mcastmode
	//     as the purpose of mcastmode is to setup the acl trap/permit rule for mcastflow traffic, not the flowid to gemportid mapping
	for (i=0; i< m->mcastgem_total; i++)	
		m->mcastflowid_list[i] = gem_flowid_get_by_gem_portid(m->mcastgem_list[i], 0);

	//according igmp_mcast_mode_src to determine unknown multicast traffic mode
	switch(omci_env_g.igmp_mcast_mode_src)
	{
	case 0:
		m->mcast_mode = 0; //pass mode, iptv disabled
		break;
	case 1:
		m->mcast_mode = 1; //drop mode, iptv enabled
		break;
	case 2:
		if (m->datagem_total > 0 && m->mcastgem_total == 0) {
			m->mcast_mode = 0; //pass mode, iptv disabled
		} else {
			m->mcast_mode = 1; //drop mode, iptv enabled
		}
		break;
	case 3:
		//TODO, calix, according private me to assignment mcastmode result, add triggerred in private me er_group
		break;
	default:
		m->mcast_mode = 1; //drop mode, iptv enabled;
	}
	
	collect_mcast_subscriber_related_tp( &mcastmode_info_g);

	{

		struct meinfo_t *miptr = meinfo_util_miptr(310);	// bridge port me
		struct me_t *me_mc_pfr,*me309;
		unsigned short meid,mc_ptr;


		list_for_each_entry(me_mc_pfr, &miptr->me_instance_list, instance_node) {

			if(m->mc_ext_vlan_cnt >=	MCASTMODE_MCASTGEM_MAX){
				dbprintf_bat(LOG_ERR, "me 310 ext_vlan entry full(%d)\n",m->mc_ext_vlan_cnt);
				break;
			} 
			
			meid = me_mc_pfr->meid;
			mc_ptr = meinfo_util_me_attr_data(me_mc_pfr, 2);
			me309 = meinfo_me_get_by_meid(309,mc_ptr);

			if(me309 == NULL){
				dbprintf_bat(LOG_ERR, "DEFER mcastmode gen\n");
				continue;
			}
			m->mc_profile_ext_vlan[m->mc_ext_vlan_cnt].meid = meid;
			m->mc_profile_ext_vlan[m->mc_ext_vlan_cnt].lan_idx = 0;
			m->mc_profile_ext_vlan[m->mc_ext_vlan_cnt].us_igmp_tag_ctrl = meinfo_util_me_attr_data(me309, 5);
			m->mc_profile_ext_vlan[m->mc_ext_vlan_cnt].us_igmp_tci = meinfo_util_me_attr_data(me309, 4);
			m->mc_profile_ext_vlan[m->mc_ext_vlan_cnt].ds_igmp_mc_tag_ctrl =
								util_bitmap_get_value(meinfo_util_me_attr_ptr(me309, 16), 3*8, 0, 8);

			m->mc_profile_ext_vlan[m->mc_ext_vlan_cnt].ds_igmp_mc_tci =
								util_bitmap_get_value(meinfo_util_me_attr_ptr(me309, 16), 3*8, 8, 16);
			m->mc_ext_vlan_cnt++;
			
		}
	}
	
	*table_data = &mcastmode_info_g;

	return 0;
}

static int
batchtab_cb_mcastmode_release(void *table_data)
{
	return 0;
}

// the dump info is generated from prev variables, as prev means the state that has been sync to hw
static int
batchtab_cb_mcastmode_dump(int fd, void *table_data)
{
	struct mcastmode_info_t *m = table_data;
	char *igmp_str="";
	char mcastgem_list_str[128];
	int wanif_igmp_override = 0;
	int brwan_igmp_override = 0;
	
	if (m == NULL)
		return 0;
	
	if (m->mcast_mode_prev == 0) {
		igmp_str="pass mode, iptv disabled";
	} else if (m->mcast_mode_prev == 1) {
		igmp_str="drop mode, iptv enabled";
	} else {
		igmp_str="not synced yet";
	}

	{
		int i;
		char *s = mcastgem_list_str;
		s[0] = 0;
		for (i=0; i < m->mcastgem_total_prev && i<MCASTMODE_MCASTGEM_MAX; i++)
			s+=sprintf(s, "%s%d:flow%d", (i>0)?" ":"", m->mcastgem_list_prev[i], m->mcastflowid_list_prev[i]);
	}
	util_fdprintf(fd, "%d datagem\n%d mcastgem (%s)\n%s\n\n", 
		m->datagem_total_prev, m->mcastgem_total_prev, mcastgem_list_str, igmp_str);

	{
		int i;
		for (i=0; i<MCASTMODE_SUBSCRIBER_MAX; i++) {
			char *tp_name, *devname;
			struct me_t *bport_me;
			
			if (m->subscriber_meid[i] == 0xffff)
				continue;
			util_fdprintf(fd, "subscriber[310]0x%x", m->subscriber_meid[i]);
			if (m->bport_meid[i] == 0xffff) {
				util_fdprintf(fd, "\n");
				continue;
			}
			util_fdprintf(fd, " - bport[47]0x%x", m->subscriber_meid[i]);
			if (m->tp_meid[i] == 0xffff) {
				util_fdprintf(fd, "\n");
				continue;
			}
			switch (m->tp_classid[i]) {
				case  11: tp_name="pptp_uni"; brwan_igmp_override = 1; break;
				case  14: tp_name="iw_VCC_tp"; break;
				case 130: tp_name="8021p_mapper"; break;
				case 134: tp_name="iphost"; break;
				case 266: tp_name="gem_iwtp"; break;
				case 281: tp_name="mcastgem_iwtp"; break;
				case  98: tp_name="xdsl_uni"; break;
				case 286: tp_name="etherflow"; break;
				case  91: tp_name="80211_uni"; break;
				case 329: tp_name="veip"; wanif_igmp_override = 1; break;
				case 162: tp_name="moca_uni"; break;
			}
			util_fdprintf(fd, " -> %s[%d]0x%x", tp_name, m->tp_classid[i], m->tp_meid[i]);

			bport_me = meinfo_me_get_by_meid(47, m->bport_meid[i]);
			if (bport_me && (devname=hwresource_devname(bport_me))!=NULL)
				util_fdprintf(fd, " (%s)", devname); 
			util_fdprintf(fd, "\n");
		}
		util_fdprintf(fd, "igmp_enable control: wanif by %s, brwan by %s\n", 
			wanif_igmp_override?"omci":"/proc/igmpmon", brwan_igmp_override?"omci":"/proc/ifsw");
	}

	return 0;
}

#define PROC_FILE_IGMPMON	"/proc/igmpmon"
#define PROC_FILE_IFSW		"/proc/ifsw"
static int
batchtab_cb_mcastmode_hw_sync(void *table_data)
{
	struct mcastmode_info_t *m = table_data;
	char *igmp_str="", cmd[64];
	int is_mcast_mode_changed = 0;
	int is_mcast_gem_changed = 0;

	if (m == NULL)
		return 0;

	{ // compare curr & prev, then save curr to prev
		int i;
		if (m->mcast_mode != m->mcast_mode_prev)
			is_mcast_mode_changed = 1;
		if (m->mcastgem_total != m->mcastgem_total_prev) {
			is_mcast_gem_changed = 1;
		} else {
			for (i =0; i < m->mcastgem_total; i++) {
				if (m->mcastgem_list[i] != m->mcastgem_list_prev[i] ||
				    m->mcastflowid_list[i] != m->mcastflowid_list_prev[i]) {
					is_mcast_gem_changed = 1;
					break;
				}
			}
		}
		m->mcast_mode_prev = m->mcast_mode;
		m->datagem_total_prev = m->datagem_total;
		m->mcastgem_total_prev = m->mcastgem_total;
		for (i=0; i< m->mcastgem_total; i++) {
			m->mcastgem_list_prev[i] = m->mcastgem_list[i];
			m->mcastflowid_list_prev[i] = m->mcastflowid_list[i];
		}
	}
	
	// calix extension to determine if multicast subscriber is on UNI or VEI
	if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX) == 0)
	{
		struct meinfo_t *miptr310 = meinfo_util_miptr(310);	// mcast_subscriber me
		struct meinfo_t *miptr47 = meinfo_util_miptr(47);	// bridge port me
		struct me_t *subscriber_me, *bport_me;
		int mode = 0;	// 0->off, 1->snoopy, 2->proxy
		
		omci_env_g.igmp_query_pass_through=0;
				
		list_for_each_entry(subscriber_me, &miptr310->me_instance_list, instance_node) {
		 	bport_me = meinfo_me_get_by_meid(47, subscriber_me->meid);
		 	if (bport_me)	// find if thie bridge port has associated
			{
				if (meinfo_me_get_by_meid(45, (unsigned short)meinfo_util_me_attr_data(bport_me, 1)) == NULL) continue;	// must associate with a bridge
				unsigned short tp_classid = miptr47->callback.get_pointer_classid_cb(bport_me, 4);
				
				if (tp_classid != 329 && tp_classid != 11) continue; // not VEIP or PPTP
				
				mode = (tp_classid == 329)?2:1;
				
				break;
			}
		}

		struct meinfo_t *me65317_miptr = meinfo_util_miptr(65317);
		short external = 0;
		if (me65317_miptr)
		{
			struct me_t *me65317 = NULL;
			list_for_each_entry(me65317, &me65317_miptr->me_instance_list, instance_node)
			{ 
				external = meinfo_util_me_attr_data(me65317, 1);
				break;
			}
		}
		// use external value if it's external mdoe && L3 from igmp profile
		if (external && mode == 2)
		{
#if 1	
#ifdef OMCI_METAFILE_ENABLE
			
			struct metacfg_t kv_config;

			memset(&kv_config, 0x0, sizeof(struct metacfg_t));
			metacfg_adapter_config_init(&kv_config);
			metacfg_adapter_config_file_load_safe(&kv_config, "/etc/wwwctrl/metafile.dat");
	
			char tmp[64];
			int i = 0;
			int wan_enable = 0;
			int brwan_enable = 0;
			
			for (i=0;i < 8;i++)
			{
				sprintf(tmp,  "wan%d_igmp_enable", i);
				if (util_atoi(metacfg_adapter_config_get_value(&kv_config, tmp, 1)) == 1)
				{
					wan_enable = 1;
					break;
				}
			}

			for (i=0;i < 8;i++)
			{
				sprintf(tmp,  "brwan%d_igmp_enable", i);
				if (util_atoi(metacfg_adapter_config_get_value(&kv_config, tmp, 1)) == 1)
				{
					brwan_enable = 1;
					break;
				}	
			}
						
			if (wan_enable)
				mode = 2;
			else if (brwan_enable)
			{
				mode = 1;
				omci_env_g.igmp_query_pass_through=3;
			}
			else
				mode = 0;
			
			metacfg_adapter_config_release(&kv_config);			
#endif
#else
			struct kv_config_t kv_config;

			kv_config_init(&kv_config);
			kv_config_file_load_safe(&kv_config, "/etc/wwwctrl/metafile.dat");
	
			char tmp[64];
			int i = 0;
			int wan_enable = 0;
			int brwan_enable = 0;
			
			for (i=0;i < 8;i++)
			{
				sprintf(tmp,  "wan%d_igmp_enable", i);
				if (util_atoi(kv_config_get_value(&kv_config, tmp, 1)) == 1)
				{
					wan_enable = 1;
					break;
				}
			}

			for (i=0;i < 8;i++)
			{
				sprintf(tmp,  "brwan%d_igmp_enable", i);
				if (util_atoi(kv_config_get_value(&kv_config, tmp, 1)) == 1)
				{
					brwan_enable = 1;
					break;
				}	
			}
						
			if (wan_enable)
				mode = 2;
			else if (brwan_enable)
			{
				mode = 1;
				omci_env_g.igmp_query_pass_through=3;
			}
			else
				mode = 0;
			
			kv_config_release(&kv_config);
#endif			
		}
			
		char mode_s[16];
/*		
		if (!external)	// native
		{
			switch(mode)
			{
			case 1:	
			case 2:	// enable mc2u
				snprintf(cmd, sizeof(cmd), "/etc/init.d/cmd_dispatcher.sh set wlanac_mc2u_disable=%d", 0);
				util_run_by_system(cmd);
				snprintf(cmd, sizeof(cmd), "/etc/init.d/cmd_dispatcher.sh set wlanac_mc2u_drop_unknown=%d", 1);
				util_run_by_system(cmd);
				snprintf(cmd, sizeof(cmd), "/etc/init.d/cmd_dispatcher.sh set wlan_mc2u_disable=%d", 0);
				util_run_by_system(cmd);
				snprintf(cmd, sizeof(cmd), "/etc/init.d/cmd_dispatcher.sh set wlan_mc2u_drop_unknown=%d", 1);
				util_run_by_system(cmd);
				break;
			default:	// disable mc2u
				snprintf(cmd, sizeof(cmd), "/etc/init.d/cmd_dispatcher.sh set wlanac_mc2u_disable=%d", 1);
				util_run_by_system(cmd);
				snprintf(cmd, sizeof(cmd), "/etc/init.d/cmd_dispatcher.sh set wlanac_mc2u_drop_unknown=%d", 0);
				util_run_by_system(cmd);
				snprintf(cmd, sizeof(cmd), "/etc/init.d/cmd_dispatcher.sh set wlan_mc2u_disable=%d", 1);
				util_run_by_system(cmd);
				snprintf(cmd, sizeof(cmd), "/etc/init.d/cmd_dispatcher.sh set wlan_mc2u_drop_unknown=%d", 0);
				util_run_by_system(cmd);
				break;
			}
		}
*/			
		switch(mode)
		{
		case 1:
			sprintf(mode_s,  "%s", "snoopy");
			break;	
		case 2:
			sprintf(mode_s,  "%s", "proxy");
			break;
		default:
			sprintf(mode_s,  "%s", "off");
			break;
		}
		snprintf(cmd, sizeof(cmd), "/etc/init.d/cmd_dispatcher.sh set igmp_function=%s", mode_s);
		dbprintf_bat(LOG_ERR, "igmp function -> %s\n", mode_s);
		util_run_by_system(cmd);			
	}
	
	// when igmp is stopped, unknow mcast pkt will flood to all ports, including cpu port(becuase wifi)
	// so we keep igmp running as much as possible since the unknown mcast pkt is dropped by default
	if (m->mcast_mode == 0) {	//pass mode, IPTV disabled
		// stop igmp task
		if(igmpproxytask_loop_g && omci_env_g.igmp_enable) {
			if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX) != 0)
			{
				igmp_proxy_stop();
				igmp_proxy_shutdown();
			}
			igmp_str="pass mode, iptv disabled, stop igmp task";
		} else {
			igmp_str="pass mode, iptv disabled";
		}

		// flood unknown multicast traffic
		if (omci_env_g.igmp_enable)  {
			if (is_mcast_mode_changed || is_mcast_gem_changed) {
#ifdef OMCI_CMD_DISPATCHER_ENABLE
				dbprintf_bat(LOG_ERR, "mcast mode/gem changed, set acl for mcast_pass\n");
				snprintf(cmd, sizeof(cmd), "/etc/init.d/cmd_dispatcher.sh event mcast_pass");
				util_run_by_system(cmd);
#endif
			}
		}
	} else {//drop mode, IPTV enabled or omci incomplete
		// start igmp task
		if(!igmpproxytask_loop_g && omci_env_g.igmp_enable) {
			if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX) != 0)
			{
				igmp_proxy_init();
				igmp_proxy_start();
			}
			igmp_str="drop mode, iptv enabled, start igmp task";
		} else {
			igmp_str="drop mode, iptv enabled";
		}

		// drop unknown multicast traffic
		// add acl to trap non iptv mcast pkt (mcast not from ds mcastgem)
		if (omci_env_g.igmp_enable) {
			if (is_mcast_mode_changed || is_mcast_gem_changed) {
#ifdef OMCI_CMD_DISPATCHER_ENABLE
				dbprintf_bat(LOG_ERR, "mcast mode/gem changed, set acl for mcast_drop\n"); 
				snprintf(cmd, sizeof(cmd), "/etc/init.d/cmd_dispatcher.sh event mcast_drop");
				util_run_by_system(cmd);
#endif
			}
		}
	}
	dbprintf_bat(LOG_ERR, "%d datagem, %d mcastgem, %s\n", m->datagem_total, m->mcastgem_total, igmp_str);

	{
		static int prev_wanif_igmp_override = -1;
		static int prev_brwan_igmp_override = -1;
		int wanif_igmp_override = 0, brwan_igmp_override = 0, i;
		for (i=0; i<MCASTMODE_SUBSCRIBER_MAX; i++) {
			if (m->subscriber_meid[i] != 0xffff && m->bport_meid[i] != 0xffff) {
				// subscriber on veip found, override the wanif_igmp_enable in /proc/igmpmon
				// let omci control whether an routing igmp pkt should be forwarded or not
				if  (m->tp_classid[i] == 329)
					wanif_igmp_override = 1;
				// subscriber on pptp-uni found, override the igmp_vlan_drop in /proc/ifsw
				// let omci control whether an bridging igmp pkt should be forwarded or not
				else if (m->tp_classid[i] == 11)
					brwan_igmp_override = 1;
			}
		}
		if (wanif_igmp_override != prev_wanif_igmp_override) {
			if (util_write_file(PROC_FILE_IGMPMON, "wanif_igmp_enable_ignore %d\n", wanif_igmp_override) < 0)
				dbprintf_bat(LOG_ERR, "%s access error, not extst?\n", PROC_FILE_IGMPMON);
			prev_wanif_igmp_override = wanif_igmp_override;
		}
		if (brwan_igmp_override != prev_brwan_igmp_override) {
			if (util_write_file(PROC_FILE_IFSW, "igmp_vlan_drop_ignore %d\n", brwan_igmp_override) < 0)
				dbprintf_bat(LOG_ERR, "%s access error, not extst?\n", PROC_FILE_IFSW);
			prev_brwan_igmp_override = brwan_igmp_override;
		}
			
		dbprintf_bat(LOG_ERR, "igmp_enable control: wanif by %s, brwan by %s\n", 
			wanif_igmp_override?"omci":"/proc/igmpmon", brwan_igmp_override?"omci":"/proc/ifsw");
	}
	///////////////set intel extvlan
	{
		int j;
		for( j= 0; j<m->mc_ext_vlan_cnt;j++)
		{
			if (switch_hw_g.ipmcast_me310_extvlan_set) {
				switch_hw_g.ipmcast_me310_extvlan_set(&m->mc_profile_ext_vlan[j]);

			}			
			
		}	

	}


	return 0;
}

// public register function /////////////////////////////////////
int
batchtab_cb_mcastmode_register(void)
{
	return batchtab_register("mcastmode", BATCHTAB_OMCI_UPDATE_NOAUTO, 3,
			batchtab_cb_mcastmode_init,
			batchtab_cb_mcastmode_finish,
			batchtab_cb_mcastmode_gen,
			batchtab_cb_mcastmode_release,
			batchtab_cb_mcastmode_dump,
			batchtab_cb_mcastmode_hw_sync);
}
