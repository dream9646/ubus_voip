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
 * File    : batchtab_cb_wan.c
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
#include "list.h"
#include "fwk_thread.h"
#include "fwk_mutex.h"
#include "batchtab_cb.h"
#include "batchtab.h"
#include "meinfo.h"
#include "hwresource.h"
#include "switch.h"
#include "tm.h"

// map from streamid to wan port in omci bridge
static struct batchtab_cb_wan_t batchtab_cb_wan_g;

static int
batchtab_cb_wan_iwtp_meid_to_gem_portid(char tp_type, unsigned short iwtp_meid)
{
	struct me_t *iwtp_me=NULL, *gem_me=NULL;
	unsigned short gem_meid, gem_portid;

	if ( iwtp_meid == 65535)
		return -1;
	
	if(tp_type == 6) // multicast gem iwtp
		iwtp_me = meinfo_me_get_by_meid(281, iwtp_meid ); //me 281
	else // gem iwtp
		iwtp_me = meinfo_me_get_by_meid(266, iwtp_meid ); //me 266
	if (iwtp_me == NULL)
		return -2;

	gem_meid = meinfo_util_me_attr_data( iwtp_me , 1 );
	if ( gem_meid == 65535)
		return -3;
	
	gem_me = meinfo_me_get_by_meid(268, gem_meid ); //me 268
	if (gem_me == NULL)
		return -4;
	
	gem_portid = meinfo_util_me_attr_data( gem_me, 1 );
	if (gem_portid>=4096)
		return -5;
	
	return gem_portid;
}

static int
batchtab_cb_wan_init(void)
{
	int i;
	for (i = 0; i < 8; i++)
		INIT_LIST_HEAD(&batchtab_cb_wan_g.wan_list[i]);
	fwk_mutex_create(&batchtab_cb_wan_g.mutex);
	return 0;
}
static int
batchtab_cb_wan_finish(void)
{
	fwk_mutex_destroy(&batchtab_cb_wan_g.mutex);
	return 0;
}
static int
batchtab_cb_wan_gen(void **table_data)
{
	struct batchtab_cb_wan_t *t= &batchtab_cb_wan_g;
	struct meinfo_t *miptr=meinfo_util_miptr(47);
	struct me_t *me;
	unsigned short iwtp_meid;
	int gem_portid, stream_id;
	int ret = 0, i;

	fwk_mutex_lock(&t->mutex);
	
	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		char tp_type=meinfo_util_me_attr_data(me, 3);
		if ( tp_type == 5 || tp_type == 6 ) {	// GEM and Multicast GEM interworking TP
			iwtp_meid = meinfo_util_me_attr_data(me, 4);
			if (iwtp_meid == 0xffff)
				continue;
				
			gem_portid = batchtab_cb_wan_iwtp_meid_to_gem_portid(tp_type, iwtp_meid);
			if (gem_portid < 0) {
				dbprintf_bat(LOG_WARNING, "bport_me:0x%x, iwtp_meid 0x%x has no related upstream gemport? err %d\n",
					me->meid, iwtp_meid, gem_portid);
				ret = -1;
				continue; 
			}
			if ((stream_id = gem_flowid_get_by_gem_portid(gem_portid, 0)) < 0 || stream_id > GPON_OMCI_FLOW_ID) {
				dbprintf_bat(LOG_WARNING, "bport_me:0x%x, iwtp_me=0x%x, gem_port=0x%x(%d), stream_id=%d?\n",
					me->meid, iwtp_meid, gem_portid, gem_portid, stream_id);
				ret = -1;
				continue;
			}
			for ( i = 0 ; i < 8 ; i++ ) {
				struct batchtab_cb_wan_node_t *wan_node = malloc_safe(sizeof(struct batchtab_cb_wan_node_t));
				wan_node->bport_meid = me->meid;
				wan_node->pbit = i;
				wan_node->stream_id = stream_id;
				list_add_tail(&wan_node->node, &t->wan_list[i]);
			}

		} else if (tp_type == 3) {	// 802.1p mapper service
			struct me_t *mapper_me= meinfo_me_get_by_meid(130, meinfo_util_me_attr_data(me, 4)); //me 266
			int cached_gem_portid=-1, cached_stream_id=-1;
			
			for ( i = 2 ; i < 10 ; i++ ) {
				iwtp_meid = meinfo_util_me_attr_data( mapper_me, i );
				if (iwtp_meid == 0xffff)
					continue;

				gem_portid = batchtab_cb_wan_iwtp_meid_to_gem_portid(tp_type, iwtp_meid);
				if (gem_portid < 0) {
					dbprintf_bat(LOG_ERR, "bport_me:0x%x, iwtp_meid 0x%x has no related gemport? err %d\n",
						me->meid, iwtp_meid, gem_portid);
					ret = -1;
					continue; 
				}
								
				if (gem_portid == cached_gem_portid) {
					stream_id = cached_stream_id;
					if (stream_id  < 0 || stream_id > GPON_OMCI_FLOW_ID ) {
						ret = -1;
						continue;
					}								
				} else {
					stream_id = gem_flowid_get_by_gem_portid(gem_portid, 0);
					cached_gem_portid = gem_portid;
					cached_stream_id = stream_id;				
					if (stream_id  < 0 || stream_id > GPON_OMCI_FLOW_ID ) {
						dbprintf_bat(LOG_WARNING, "bport_me:0x%x, iwtp_me=0x%x, gem_port=0x%x(%d), stream_id=%d?\n",
							me->meid, iwtp_meid, gem_portid, gem_portid, stream_id);
						ret = -1;
						continue;
					}
				}
				{
					struct batchtab_cb_wan_node_t *wan_node = malloc_safe(sizeof(struct batchtab_cb_wan_node_t));
					wan_node->bport_meid = me->meid;
					wan_node->pbit = i-2;
					wan_node->stream_id = stream_id;
					list_add_tail(&wan_node->node, &t->wan_list[i-2]);
				}
			}
		} else
			continue;
	}

	fwk_mutex_unlock(&t->mutex);

	*table_data =t;
	return ret;
}
static int
batchtab_cb_wan_release(void *table_data)
{
	struct batchtab_cb_wan_t *t = (struct batchtab_cb_wan_t *)table_data;
	int i;

	if(!table_data)
	{
		return 0;
	}

	fwk_mutex_lock(&t->mutex);
	// clear old wan node on 8 list (each list for one pbit)
	for ( i = 0 ; i < 8 ; i++ ) {
		struct batchtab_cb_wan_node_t *w, *n;
		list_for_each_entry_safe(w, n, &t->wan_list[i], node) {
			list_del(&w->node);
			free_safe(w);
		}
	}
	fwk_mutex_unlock(&t->mutex);
	return 0;
}
static int
batchtab_cb_wan_dump(int fd, void *table_data)
{
	struct batchtab_cb_wan_t *t= (struct batchtab_cb_wan_t *)table_data;
	int i;

	if(t == NULL)
		return 0;

	util_fdprintf(fd, "\n%-15s %-6s\tpbit\tstream id\n", "wanport name", "meid"); 

	fwk_mutex_lock(&t->mutex);
	for (i=0; i<8; i++) {
		struct batchtab_cb_wan_node_t *w;
		list_for_each_entry(w, &t->wan_list[i], node) {
			struct me_t *bport_me = meinfo_me_get_by_meid(47, w->bport_meid);
			if (bport_me) {
				char *devname = hwresource_devname(bport_me);
				if (devname == NULL)
					devname = "?";
				util_fdprintf(fd, "%-15s 0x%04x\t%d\t%d\n", 
					devname, w->bport_meid, w->pbit, w->stream_id); 
			}
		}
	}

	fwk_mutex_unlock(&t->mutex);
	return 0;
}
static int
batchtab_cb_wan_hw_sync(void *table_data)
{
	return 0;
}

// public register function /////////////////////////////////////
int
batchtab_cb_wan_register(void)
{
	return batchtab_register("wan", BATCHTAB_OMCI_UPDATE_NOAUTO, 3,
			batchtab_cb_wan_init,
			batchtab_cb_wan_finish,
			batchtab_cb_wan_gen,
			batchtab_cb_wan_release,
			batchtab_cb_wan_dump,
			batchtab_cb_wan_hw_sync);
}
