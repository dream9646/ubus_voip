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
 * File    : batchtab_cb_linkready.c
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
#include "fwk_thread.h"
#include "fwk_mutex.h"
#include "batchtab_cb.h"
#include "batchtab.h"
#include "meinfo.h"
#include "extoam.h"
#include "extoam_queue.h"
#include "extoam_link.h"
#include "extoam_event.h"
#include "omcimsg.h"

extern struct extoam_link_status_t cpe_link_status_g;

// send the event trigger to restart wan.sh when upstream data gemport becomes ready
static struct batchtab_cb_linkready_t batchtab_cb_linkready_g;	
// util_check_linkready_change_and_send_event() may be called through
// a. the batchtab dump cli cmd by cli server thread
// b. the batchtab gen callback by coretask
// c. the batchtab_cb_linkready_check_debounce() in per sec period by coretask
// since case a may happen concurrently with b or c, we use mutex to 
// keep the 'read & update' of prev_uplink_is_ready in util_check_linkready_change_and_send_event() atomic
static struct fwk_mutex_t mutex_linkready;

static int
util_send_cmd_dispatcher_event(char *event)
{
	char cmd[256];
	snprintf(cmd, 255, "/etc/init.d/cmd_dispatcher.sh event %s", event);
	return util_run_by_thread(cmd, 0);
}

static void
util_update_tcont2allocid_map(int onuid, struct batchtab_cb_linkready_t *l)
{
	// refresh active allocid -> tcont_id map
	struct meinfo_t *miptr=meinfo_util_miptr(262);
	struct me_t *me;
	unsigned short allocid;
	int tcont_id;
	// data tcont
	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		allocid = (unsigned short)meinfo_util_me_attr_data(me, 1);
		if (allocid !=0xff && allocid !=0xffff) {
			if (gpon_hw_g.tm_tcont_get_by_allocid(allocid, &tcont_id)==0) {
				if (tcont_id < GPON_TCONT_ID_TOTAL)
					l->allocid[tcont_id]=allocid;
			}
		}
	}
	// omci tcont
#if 1///tcont 0 alway is onuid
	allocid = onuid;
	l->allocid[0]=allocid;
#else
	allocid = onuid;
	if (gpon_hw_g.tm_tcont_get_by_allocid(allocid, &tcont_id)==0) {
		if (tcont_id < GPON_TCONT_ID_TOTAL)
			l->allocid[tcont_id]=allocid;
	}
#endif	
}

static void
util_update_onuid_datagem_connect_disconnect_time(int onuid, int data_gem_total, struct batchtab_cb_linkready_t *l)
{
	if (data_gem_total > 0 && onuid != 255)	{
		if (l->uplink_disconnect_time) {
			long now = util_get_uptime_sec();
			dbprintf_bat(LOG_ERR, "event: uplink_connect (onuid %d, data_gem_total %d, elapsed disconnect time: %ldsec)\n", 
				l->onuid, l->data_gem_total, now - l->uplink_disconnect_time);
			util_send_cmd_dispatcher_event("uplink_connect");
			l->uplink_connect_time = now;
			l->uplink_disconnect_time = 0;
			l->uplink_connect_post_event_sent = 0;
		}
		if (omci_env_g.linkdown_debounce_time == 0)	// set uplink_is_ready to 1 immediately if debounce is disabled
			l->uplink_is_ready = 1;
	} else {
		if (l->uplink_disconnect_time ==0) {
			long now = util_get_uptime_sec();
			dbprintf_bat(LOG_ERR, "event: uplink_disconnect (onuid %d, data_gem_total %d, elapsed connect time: %ldsec)\n", 
				l->onuid, l->data_gem_total, now - l->uplink_connect_time);
			util_send_cmd_dispatcher_event("uplink_disconnect");
			l->uplink_connect_time = 0;
			l->uplink_disconnect_time = now;
		}
		if (omci_env_g.linkdown_debounce_time == 0)	// set uplink_is_ready to 0 immediately if debounce is disabled
			l->uplink_is_ready = 0;
		// 'l->uplink_is_ready = 0' is deferred until disconnect happens for more than debounce time
		// this is done in batchtab_cb_linkready_check_debounce() in per sec period
	}
	l->onuid = onuid;
	l->data_gem_total = data_gem_total;
}	

static int
util_check_linkready_change_and_send_event(struct batchtab_cb_linkready_t *l)
{	
	fwk_mutex_lock(&mutex_linkready);

	if (l->prev_uplink_is_ready == l->uplink_is_ready &&
	    l->prev_onuid == l->onuid &&
	    l->prev_data_gem_total == l->data_gem_total) {
		fwk_mutex_unlock(&mutex_linkready);
		return 0;	    	
	}

	if (l->uplink_is_ready) {	// ready
		if (!l->prev_uplink_is_ready) {	// not-ready to ready
			batchtab_cb_linkready_g.uplink_ready_time = util_get_uptime_sec();
			dbprintf_bat(LOG_ERR, "event: uplink_ready (linkready 0->1)\n");
			// for wan.sh stop/start 0/1/2/3
			util_send_cmd_dispatcher_event("uplink_ready");
			if ( omci_env_g.extoam_enable && cpe_link_status_g.link_state == LINK_ACK ) {
				extoam_event_send( &cpe_link_status_g, EXTOAM_EVENT_GPON_LINK_UP );
			}
		} else {			// ready to ready	
			if (l->prev_onuid == l->onuid) {
				if (l->prev_data_gem_total != l->data_gem_total) {
					dbprintf_bat(LOG_WARNING, "linkready 1, onuid %d, data_gem_total %d->%d\n", l->onuid, l->prev_data_gem_total, l->data_gem_total);
					// we remove this case as for specific onuid, its data gem may from1->2, 2->3, ..., 
					// however, each increase of data gem should not trigger a uplinkready event
					//util_send_cmd_dispatcher_event("uplink_ready");
				} else {
					dbprintf_bat(LOG_WARNING, "linkready 1, data_gem_total %d\n", l->data_gem_total);
					// we dont trigger uplink_ready event if onuid & datagem are not changed
				}
				if (l->uplink_connect_time >0) {
					long now = util_get_uptime_sec();
					if (!l->uplink_connect_post_event_sent && now-l->uplink_connect_time >30) {
				    		l->uplink_connect_post_event_sent = 1;
						dbprintf_bat(LOG_ERR, "event: uplink_connect_post_event (elapsed up time: %ldsec)\n", now - l->uplink_connect_time);
						util_send_cmd_dispatcher_event("uplink_connect_post_event");
					}
				}
				
			} else { //l->prev_onuid != l->onuid
				if (l->onuid!= 255 && l->data_gem_total>0) {
					dbprintf_bat(LOG_ERR, "event: uplink_down & uplink_ready (linkready 1->1, onuid %d->%d, data_gem_total %d)\n", 
						l->prev_onuid, l->onuid, l->data_gem_total);
					// for wan.sh stop/start 0/1/2/3
					util_send_cmd_dispatcher_event("uplink_down");
					util_send_cmd_dispatcher_event("uplink_ready");
				}
			}
		}

	} else {			// not ready
		if (l->prev_uplink_is_ready && !l->uplink_is_ready) {	// ready to not-ready
			batchtab_cb_linkready_g.uplink_down_time = util_get_uptime_sec();
			dbprintf_bat(LOG_ERR, "event: uplink_down (linkready 1->0)\n");
			// for wan.sh stop/start 0/1/2/3
			util_send_cmd_dispatcher_event("uplink_down");
			if ( omci_env_g.extoam_enable && cpe_link_status_g.link_state == LINK_ACK ) {
				extoam_event_send( &cpe_link_status_g, EXTOAM_EVENT_GPON_LINK_DOWN );
			}
		}
	}

	l->prev_onuid = l->onuid;
	l->prev_data_gem_total = l->data_gem_total;
	l->prev_uplink_is_ready = l->uplink_is_ready;

	fwk_mutex_unlock(&mutex_linkready);
	
	return 0;
}


static int
batchtab_cb_linkready_init(void)
{
	memset(&batchtab_cb_linkready_g, 0, sizeof(struct batchtab_cb_linkready_t));
	batchtab_cb_linkready_g.uplink_ready_time = batchtab_cb_linkready_g.uplink_down_time = util_get_uptime_sec();
	batchtab_cb_linkready_g.uplink_disconnect_time = util_get_uptime_sec();	// assume default is down
	batchtab_cb_linkready_g.onuid = 255;
	fwk_mutex_create(&mutex_linkready);
	return 0;
}

static int
batchtab_cb_linkready_finish(void)
{
	memset(&batchtab_cb_linkready_g, 0, sizeof(struct batchtab_cb_linkready_t));	
	fwk_mutex_destroy(&mutex_linkready);
	return 0;
}

static int
batchtab_cb_linkready_gen(void **table_data)
{
	struct batchtab_cb_linkready_t *l = &batchtab_cb_linkready_g;
	unsigned short onuid = 0x3FF;//255;
	int data_gem_total = 0;
	int onu_state;
	static int defer_count = 0;

	// skip wanif auto gen if mibreset till now < 20sec
	if (omcimsg_mib_reset_finish_time_till_now() < 20) {
		// keep the trigger for future and do nothing for now
		if (defer_count == 0)
			dbprintf_bat(LOG_ERR, "DEFER linkready gen\n");
		defer_count++;
		// return -1 to make this table gen as error, so batchtab framework will keep retry this table gen
		return -1;
	}
	if (defer_count>0) {
		dbprintf_bat(LOG_ERR, "resume DEFERRED linkready gen\n");
		defer_count = 0;
	}
	
	gpon_hw_g.onu_state_get(&onu_state);
	if (onu_state == GPON_ONU_STATE_O5) {
		struct gpon_tm_usflow_config_t *usflow_config;
		int tcont_id, i;

		gpon_hw_g.onu_id_get(&onuid);
	
		// fill allocid to tcont_id mapping table
		for (tcont_id=0; tcont_id<GPON_TCONT_ID_TOTAL; tcont_id++)
			l->allocid[tcont_id] = -1;
		util_update_tcont2allocid_map(onuid, l);
	
		// get active flows 
		for (i=0; i<GPON_FLOW_ID_TOTAL; i++) {
			struct gpon_tm_pq_config_t pq_config;		
			l->flowinfo[i].pq_id = -1;
			l->flowinfo[i].tcont_id = -1;
	
			gpon_hw_g.tm_usflow2pq_get(i, &l->flowinfo[i].pq_id);	
			if (l->flowinfo[i].pq_id >=0 && gpon_hw_g.tm_pq_config_get(l->flowinfo[i].pq_id, &pq_config)==0 && pq_config.enable)
				l->flowinfo[i].tcont_id = pq_config.tcont_id;
	
			usflow_config=&l->flowinfo[i].usflow_config;
	
			if (gpon_hw_g.tm_usflow_get(i, usflow_config)==0 && usflow_config->enable) {
			    	if (usflow_config->flow_type%4 == 1) {	// 0:omci, 1:data, 2:tdm, 3:undef
					if (l->flowinfo[i].tcont_id != -1 && l->flowinfo[i].pq_id != GPON_OMCI_PQ_ID)
						data_gem_total++;	// germflow of omci & telnet/ssh debug use pq of GPON_OMCI_PQ_ID
				}
			}
		}
	}	
	util_update_onuid_datagem_connect_disconnect_time(onuid, data_gem_total, l);

	util_check_linkready_change_and_send_event(l);

	*table_data = l;
	return 0;
}

static int
batchtab_cb_linkready_release(void *table_data)
{
	return 0;
}

static int
batchtab_cb_linkready_hw_sync(void *table_data)
{
	return 0;
}

static int
batchtab_cb_linkready_dump(int fd, void *table_data)
{
	struct batchtab_cb_linkready_t *l = &batchtab_cb_linkready_g;
	unsigned short onuid = 0x3FF;//255;
	int data_gem_total = 0;
	int onu_state;

	gpon_hw_g.onu_state_get(&onu_state);
	if (onu_state == GPON_ONU_STATE_O5) {
		static char *flow_type_str[] = { "omci", "data", "tdm", "?" };
		int i;
		
		gpon_hw_g.onu_id_get(&onuid);
		util_update_tcont2allocid_map(onuid, l);
	
		for (i=0; i<GPON_FLOW_ID_TOTAL; i++) {
			struct gpon_tm_usflow_config_t *usflow_config=&l->flowinfo[i].usflow_config;	
			char *type_str = flow_type_str[usflow_config->flow_type%4];
			int alloc_id = -1, tcont_id = -1;
			
			if (!usflow_config->enable)
				continue;
				
			if (l->flowinfo[i].tcont_id>=0 && l->flowinfo[i].tcont_id < GPON_TCONT_ID_TOTAL) {
				alloc_id = l->allocid[l->flowinfo[i].tcont_id];
				if (alloc_id>=0) {
					// as allocid may be del by olt through ploam but not omci
					// so we need to validate the data gem by checking if the related allocid is till active
					gpon_hw_g.tm_tcont_get_by_allocid(alloc_id, &tcont_id);	// check the status of hw tcont_id on the fly
				}
			}
			if (usflow_config->flow_type%4 == 1) {	// 0:omci, 1:data, 2:tdm, 3:undef	
				if (tcont_id != -1 && l->flowinfo[i].pq_id != GPON_OMCI_PQ_ID)
					data_gem_total++;	// germflow of omci & telnet/ssh debug use pq of GPON_OMCI_PQ_ID
			}
	
			if (i == GPON_OMCI_FLOW_ID)
				type_str = "omci";
			else if (i == GPON_SSHD_FLOW_ID)
				type_str = "sshd";
							
			util_fdprintf(fd, "usflow=%d(%s), gem=%d, pq=%d, alloc_id=%d, tcont_id=%d(%d)\n", 
				i, type_str, usflow_config->gemport, l->flowinfo[i].pq_id, 
				alloc_id, l->flowinfo[i].tcont_id, tcont_id);
		}
	}

	util_update_onuid_datagem_connect_disconnect_time(onuid, data_gem_total, l);
	
	if (l->prev_uplink_is_ready != l->uplink_is_ready ||
	    l->prev_onuid != l->onuid ||
	    l->prev_data_gem_total != l->data_gem_total) {
		util_check_linkready_change_and_send_event(l);
	}

	{
		long now = util_get_uptime_sec();
		if (batchtab_cb_linkready_g.uplink_disconnect_time)
			util_fdprintf(fd, "uplink_disconnect time=%ld sec ago\n", now - batchtab_cb_linkready_g.uplink_disconnect_time);
		if (batchtab_cb_linkready_g.uplink_connect_time)
			util_fdprintf(fd, "uplink_connect    time=%ld sec ago\n", now - batchtab_cb_linkready_g.uplink_connect_time);
		util_fdprintf(fd, "uplink_ready time=%ld sec ago\n", now - batchtab_cb_linkready_g.uplink_ready_time);
		util_fdprintf(fd, "uplink_down  time=%ld sec ago\n", now - batchtab_cb_linkready_g.uplink_down_time);
	}
	util_fdprintf(fd, "onuid=%d, data_gem_total=%d, uplink_is_rady=%d\n",
	        l->onuid,
	        l->data_gem_total,
                l->uplink_is_ready);

	return 0;
}

int
batchtab_cb_linkready_check_debounce()
{
	struct batchtab_cb_linkready_t *l = &batchtab_cb_linkready_g;
	int onu_state;

	gpon_hw_g.onu_state_get(&onu_state);
	if (onu_state != GPON_ONU_STATE_O5) {
		if (l->uplink_disconnect_time == 0) {	// curr state is link up
			// set onuid=255, data_gem_total=0 to change curr state to link down
			util_update_onuid_datagem_connect_disconnect_time(255, 0, l);
		}
	}

	if (l->uplink_is_ready && l->uplink_disconnect_time >0) {		// up to down
		if (util_get_uptime_sec() - l->uplink_disconnect_time > omci_env_g.linkdown_debounce_time) {
			l->uplink_is_ready = 0;
			util_check_linkready_change_and_send_event(l);
			return 1;
		}
	} else if (l->uplink_is_ready==0 && l->uplink_connect_time >0) {	// down to up
		if (util_get_uptime_sec() - l->uplink_connect_time > omci_env_g.linkup_debounce_time) {
			l->uplink_is_ready = 1;
			util_check_linkready_change_and_send_event(l);
			return 1;
		}
	} else if (l->uplink_is_ready && l->uplink_connect_time >0) {	// up
		if (!l->uplink_connect_post_event_sent &&
		    util_get_uptime_sec() - l->uplink_connect_time > 30) {
			util_check_linkready_change_and_send_event(l);
		}
		return 1;
	}
	    	
	return 0;
}

int
batchtab_cb_linkready_state(void)
{
	return batchtab_cb_linkready_g.uplink_is_ready;
}

// public register function /////////////////////////////////////
int
batchtab_cb_linkready_register(void)
{
	return batchtab_register("linkready", BATCHTAB_OMCI_UPDATE_NOAUTO, 1,
			batchtab_cb_linkready_init,
			batchtab_cb_linkready_finish,
			batchtab_cb_linkready_gen,
			batchtab_cb_linkready_release,
			batchtab_cb_linkready_dump,
			batchtab_cb_linkready_hw_sync);
}
