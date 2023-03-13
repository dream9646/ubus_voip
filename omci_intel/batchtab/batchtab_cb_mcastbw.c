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
 * File    : batchtab_cb_mcastbw.c
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
#include "batchtab_cb.h"
#include "batchtab.h"

#include "meinfo.h"
#include "me_related.h"
#include "switch.h"
#include "classf.h"
#include "vacl_util.h"

#define MCASTBW_UNI_MAX	16
#define MCASTBW_ACL_MAX	8

struct mcastbw_uni_t {
	struct me_t *me_47;	// bport
	struct me_t *me_veip;	// veip in same bridge, referred if me47 is private me
	struct me_t *me_uni0;	// uni0 in same bridge, referred in alu p2p mode
	struct me_t *me_310;	// mcast subscriber config info
};

struct mcastbw_acl_t {
	unsigned int portmask;
	unsigned int bandwidth;
	unsigned int meter_rate;
};
	
struct mcastbw_info_t {
	struct mcastbw_uni_t uni[MCASTBW_UNI_MAX];
	struct mcastbw_acl_t acl[MCASTBW_ACL_MAX];
	struct mcastbw_acl_t acl_commited[MCASTBW_ACL_MAX];
	int acl_meter_id4[MCASTBW_ACL_MAX];
	int acl_meter_id6[MCASTBW_ACL_MAX];
};

static struct mcastbw_info_t mcastbw_info_g;


static struct me_t *
get_me_310_related(struct me_t *me_47)
{
	struct meinfo_t *miptr_310=meinfo_util_miptr(310);
	struct me_t *me;
	list_for_each_entry(me, &miptr_310->me_instance_list, instance_node) {
		if (me_related(me_47, me))
			return me;
	}
	return NULL;
}

static struct me_t *
get_me_veip_in_same_bridge(struct me_t *me_47)
{
	unsigned short bridge_meid = (unsigned short) meinfo_util_me_attr_data(me_47, 1);
	struct meinfo_t *miptr_47=meinfo_util_miptr(47);
	struct me_t *me;
	
	if (bridge_meid == 0xffff)
		return NULL;	// this bridge doesnt belong to any bridge		
	
	list_for_each_entry(me, &miptr_47->me_instance_list, instance_node) {
		if ((unsigned short) meinfo_util_me_attr_data(me, 1) != bridge_meid)	// not in same bridge
			continue;
		if ((unsigned char)meinfo_util_me_attr_data(me, 3) == 11) // the bport me is veip
			return me;
	}
	
	return NULL;
}

static struct me_t *
get_me_uni0_in_same_bridge(struct me_t *me_47)
{
	unsigned short bridge_meid = (unsigned short) meinfo_util_me_attr_data(me_47, 1);
	struct meinfo_t *miptr_47=meinfo_util_miptr(47);
	struct me_t *me;
	struct switch_port_info_t port_info;
	
	if (bridge_meid == 0xffff)
		return NULL;	// this bridge doesnt belong to any bridge		
	
	list_for_each_entry(me, &miptr_47->me_instance_list, instance_node) {
		if ((unsigned short) meinfo_util_me_attr_data(me, 1) != bridge_meid)	// not in same bridge
			continue;
		if (switch_get_port_info_by_me(me, &port_info) != 0)	// no portinfo for this bport
			continue;
		if (port_info.port_type == ENV_BRIDGE_PORT_TYPE_UNI &&
		    port_info.logical_port_id == 0)
			return me;
	}
	
	return NULL;
}

// return -1: not uni bport, 0..n: uni
static int
get_me_uni_index(struct me_t *me_47)
{
	unsigned short bridge_meid = (unsigned short) meinfo_util_me_attr_data(me_47, 1);
	struct switch_port_info_t port_info;

	if (bridge_meid == 0xffff)
		return -1;	// this bridge doesnt belong to any bridge		

	if (switch_get_port_info_by_me(me_47, &port_info) != 0)	// no portinfo for this bport
		return -1;

	if (port_info.port_type == ENV_BRIDGE_PORT_TYPE_UNI)
		return port_info.logical_port_id;

	return -1;
}

static int
add_port_bw_to_acl(struct mcastbw_acl_t acl_table[], unsigned char port, unsigned int bandwidth)
{
	int i;
	
	if (bandwidth == 0)
		return -1;
	
	for (i=0; i< MCASTBW_UNI_MAX; i++) {
		if (acl_table[i].bandwidth == bandwidth) {
			acl_table[i].portmask |= (1 << port );
			return i;
		}
		// since rule are added to acl table from 0..n
		// so if we encounter an entry with portmask 0, which means it is end of table
		if (acl_table[i].portmask == 0) {
			acl_table[i].portmask |= (1 << port );
			acl_table[i].bandwidth = bandwidth;
			return i;
		}
	}
	return -1;
}

static int
add_acl_to_hw(struct mcastbw_acl_t *acl, int acl_meter_id4[], int acl_meter_id6[])
{
	int id4, id6, ret4, ret6;
	int total = 0, i;
	
	for (i=0; i<MCASTBW_ACL_MAX; i++) {
		if (acl[i].portmask) {
			struct vacl_user_node_t rule;			

			vacl_util_multicast_node_compose(&rule, acl[i].portmask, acl[i].meter_rate, 0);
			ret4 = vacl_add(&rule, &id4);
			vacl_util_multicast_node_compose(&rule, acl[i].portmask, acl[i].meter_rate, 1);
			ret6 = vacl_add(&rule, &id6);

			if (ret4 != 0) {
				dbprintf_bat(LOG_ERR, "fail to add ipv4 multicast mac, ret=%d\n", ret4);
				acl_meter_id4[i]= -1;
			} else {
				dbprintf_bat(LOG_INFO, "succ to add ipv4 multicast mac, id/count=%d\n", id4);
				acl_meter_id4[i]= id4;
			}
			if (ret6 != 0) {
				dbprintf_bat(LOG_ERR, "fail to add ipv6 multicast mac, ret=%d\n", ret6);
				acl_meter_id6[i]= -1;
			} else {
				dbprintf_bat(LOG_INFO, "succ to add ipv6 multicast mac, id/count=%d\n", id6);
				acl_meter_id6[i]= id6;
			}
			total++;
		}
	}
	return total;
}

static int
del_acl_to_hw(struct mcastbw_acl_t *acl)
{
	int id4, id6, ret4, ret6;
	int total = 0, i;
	
	for (i=0; i<MCASTBW_ACL_MAX; i++) {
		if (acl[i].portmask) {
			struct vacl_user_node_t rule;			

			vacl_util_multicast_node_compose(&rule, acl[i].portmask, acl[i].meter_rate, 0);
			ret4 = vacl_del(&rule, &id4);
			vacl_util_multicast_node_compose(&rule, acl[i].portmask, acl[i].meter_rate, 1);
			ret6 = vacl_del(&rule, &id6);

			if (ret4 != 0)
				dbprintf_bat(LOG_ERR, "fail to del ipv4 multicast mac, ret=%d\n", ret4);
			else
				dbprintf_bat(LOG_INFO, "succ to del ipv4 multicast mac, id/count=%d\n", id4);
			if (ret6 != 0)
				dbprintf_bat(LOG_ERR, "fail to del ipv6 multicast mac, ret=%d\n", ret6);
			else
				dbprintf_bat(LOG_INFO, "succ to del ipv6 multicast mac, id/count=%d\n", id6);
			total++;
		}
	}
	return total;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////

static int
batchtab_cb_mcastbw_init(void)
{
	memset(&mcastbw_info_g, 0x00, sizeof(struct mcastbw_info_t));
	return 0;
}

static int
batchtab_cb_mcastbw_finish(void)
{
	return 0;
}

static int
batchtab_cb_mcastbw_gen(void **table_data)
{
	struct mcastbw_info_t *m = &mcastbw_info_g;
	struct meinfo_t *miptr_47=meinfo_util_miptr(47);
	struct me_t *me, *me_310, *me_veip, *me_uni0;
	int uni_index;
	int i;
	
	if (table_data == NULL)
		return 0;

	// done in batchtab_cb_mcastbw_release() by batchtab framework
	// memset(&mcastbw_info_g.uni, 0, sizeof(mcastbw_info_g.uni));
	// memset(&mcastbw_info_g.acl, 0, sizeof(mcastbw_info_g.acl));

	// update uni table
	list_for_each_entry(me, &miptr_47->me_instance_list, instance_node) {
		if ((uni_index = get_me_uni_index(me)) <0)
			continue;

		if (m->uni[uni_index].me_47 == NULL)
			m->uni[uni_index].me_47 = me;

		me_310 = get_me_310_related(me);	
		if (me_310) {	// uni has me310 by itself
			m->uni[uni_index].me_47 = me;
			m->uni[uni_index].me_310 = me_310;
			continue;
		}

		if ((me_veip = get_me_veip_in_same_bridge(me)) != NULL) {
			me_310 = get_me_310_related(me_veip);
			if (me_310) {	// uni refers the me310 from veip
				m->uni[uni_index].me_47 = me;
				m->uni[uni_index].me_veip = me_veip;
				m->uni[uni_index].me_310 = me_310;
				continue;
			}
		}
		
		if ((me_uni0 = get_me_uni0_in_same_bridge(me)) != NULL) {
			me_310 = get_me_310_related(me_uni0);
			if (me_310) {	// uni refers the me310 from uni0
				m->uni[uni_index].me_47 = me;
				m->uni[uni_index].me_uni0 = me_uni0;
				m->uni[uni_index].me_310 = me_310;
				continue;
			}
		}		
	}

	// undate acl table for portmask and bandwidth
	for (i = 0; i<MCASTBW_UNI_MAX; i++) {
		if (m->uni[i].me_47 && m->uni[i].me_310) {
			unsigned int bandwidth = (unsigned int) meinfo_util_me_attr_data(m->uni[i].me_310, 4);
			add_port_bw_to_acl(m->acl, i, bandwidth);
		}
	}
	// undate acl table for meter_rate
	for (i = 0; i<MCASTBW_ACL_MAX; i++) {
		if (m->acl[i].portmask) {
			vacl_util_mcast_bw_to_meter(m->acl[i].bandwidth, &m->acl[i].meter_rate);
		}
	}
	
	*table_data = m;
	return 0;
}

static int
batchtab_cb_mcastbw_release(void *table_data)
{
	struct mcastbw_info_t *m = table_data;

	if (m == NULL)
		return 0;
		
	memset(m->uni, 0, sizeof(m->uni));
	memset(m->acl, 0, sizeof(m->acl));

	return 0;
}

static int
batchtab_cb_mcastbw_dump(int fd, void *table_data)
{
	struct mcastbw_info_t *m = table_data;
	int i;

	if (m == NULL)
		return 0;

	for (i=0; i<MCASTBW_UNI_MAX; i++) {
		unsigned int bandwidth;

		if (m->uni[i].me_47 == NULL)
			continue;

		if (m->uni[i].me_310)
			bandwidth = (unsigned int) meinfo_util_me_attr_data(m->uni[i].me_310, 4);
			
		if (m->uni[i].me_veip) {
			util_fdprintf(fd, "uni%d: me47:0x%x, me310:0x%x, through veip:0x%x, bw=%d byte/s\n", 
				i, m->uni[i].me_47->meid, m->uni[i].me_310->meid, m->uni[i].me_veip->meid, bandwidth);
		} else if (m->uni[i].me_uni0) {
			util_fdprintf(fd, "uni%d: me47:0x%x, me310:0x%x, through uni0:0x%x, bw=%d byte/s\n", 
				i, m->uni[i].me_47->meid, m->uni[i].me_310->meid, m->uni[i].me_uni0->meid, bandwidth);
		} else if (m->uni[i].me_310) {	// both me_veip & me_uni0 == NULL
			util_fdprintf(fd, "uni%d: me47:0x%x, me310:0x%x, bw=%d byte/s\n", 
				i, m->uni[i].me_47->meid, m->uni[i].me_310->meid, bandwidth);
		} else {
			util_fdprintf(fd, "uni%d: me47:0x%x, me310:not found, bw undefined\n", 
				i, m->uni[i].me_47->meid);
		}
	}

	util_fdprintf(fd, "\nacl meter:\n");
	for (i=0; i<MCASTBW_ACL_MAX; i++) {
		if (m->acl[i].portmask) {
			util_fdprintf(fd, "portmask=0x%02x, bw=%lu byte/s, meter_rate=%lu kbit/s, through acl %d,%d\n", 
				m->acl[i].portmask, m->acl[i].bandwidth, m->acl[i].meter_rate, m->acl_meter_id4[i], m->acl_meter_id6[i]);
		}
	}	

	return 0;
}

static int
batchtab_cb_mcastbw_hw_sync(void *table_data)
{
	struct mcastbw_info_t *m = table_data;
	
	if (m == NULL)
		return 0;

	// do nothing if new acl is the same as commited one
	if (memcmp(&m->acl_commited, &m->acl, sizeof(m->acl)) == 0)
		return 0;
		
	del_acl_to_hw(m->acl_commited);

	memcpy(&m->acl_commited, &m->acl, sizeof(m->acl));
	add_acl_to_hw(m->acl_commited, m->acl_meter_id4, m->acl_meter_id6);

	{
		int count;

		switch_hw_g.vacl_commit(&count);
		dbprintf_bat(LOG_INFO, "%d rules hw_commit-ed.\n", count);
	}

	return 0;
}

// public register function /////////////////////////////////////
int
batchtab_cb_mcastbw_register(void)
{
	return batchtab_register("mcastbw", BATCHTAB_OMCI_UPDATE_NOAUTO, 3,
			batchtab_cb_mcastbw_init,
			batchtab_cb_mcastbw_finish,
			batchtab_cb_mcastbw_gen,
			batchtab_cb_mcastbw_release,
			batchtab_cb_mcastbw_dump,
			batchtab_cb_mcastbw_hw_sync);
}
