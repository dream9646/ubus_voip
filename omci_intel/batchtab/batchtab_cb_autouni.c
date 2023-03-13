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
 * File    : batchtab_cb_autouni.c
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
#include "omciutil_misc.h"

static struct autouni_info_t autouni_info_g;

int 
autouni_get_veip_meid_by_bridge_me(struct me_t *bridge_me)
{
	struct meinfo_t *miptr = meinfo_util_miptr(47);
	struct me_t *bport_me;
	
	if (bridge_me == NULL)
		return -1;	
	// iterate all bport to find if this bridge has veip
	list_for_each_entry(bport_me, &miptr->me_instance_list, instance_node) {
		if (bport_me->is_private)
			continue;	
		if (me_related(bport_me, bridge_me)) {	// bport me related to bridge found
			if ((unsigned char)meinfo_util_me_attr_data(bport_me, 3) == 11)	// this is a veip bport
				return (unsigned short)meinfo_util_me_attr_data(bport_me, 4);	// veip meid
		} 
	}
	return -1;
}

// a. number of bridge with veip == 1
// b. number of uni == 0
// c. number of veip >0
// d. number of wan port >0

unsigned int
autouni_is_required(unsigned short *single_bridge_meid)
{
	struct meinfo_t *miptr;
	struct me_t *me;
	struct me_t *found_veip_bridge_me = NULL;
	int veip_bridge_total = 0;		// veip bridge
	int uni_bport_total = 0;		// uni on all bridge
	int wan_bport_total = 0;		// wan on all bridge
	int veip_wan_bport_total = 0;		// wan on veip bridge
	int veip_bport_total = 0;		// veip on all bridge
	int iphost_bport_total = 0;

	if (!omci_env_g.autouni_enable)
		return 0;

	miptr = meinfo_util_miptr(45);
	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
	
		if (me->is_private)
			continue;		
		if (autouni_get_veip_meid_by_bridge_me(me)>=0) {
			veip_bridge_total++;
			if (found_veip_bridge_me == NULL)
				found_veip_bridge_me = me;
		}
	}
	if (veip_bridge_total!=1)
		return 0;
		
	miptr = meinfo_util_miptr(47);
	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		struct attr_value_t attr_tptype;
		struct attr_value_t attr_tppointer;
		struct me_t *pptp_uni_me;

		if (me->is_private)
			continue;

		// get public me attr3 (TP type)
		meinfo_me_attr_get(me, 3, &attr_tptype);
		// get public me attr4 (TP pointer)
		meinfo_me_attr_get(me, 4, &attr_tppointer);
	
		switch (attr_tptype.data) {
			case 1:	// Physical path termination point Ethernet UNI
			case 2:	// Interworking VCC termination point
			case 7:	// Physical path termination point xDSL UNI part 1
			case 8:	// Physical path termination point VDSL UNI
			case 9:	// Ethernet flow termination point
			case 10:// Physical path termination point 802.11 UNI
				// get the meptr of the 'Physical path termination point Ethernet UNI'
				pptp_uni_me = meinfo_me_get_by_meid(11, attr_tppointer.data);
				if (pptp_uni_me) {
					struct me_t * veip_me=omciutil_misc_find_pptp_related_veip(pptp_uni_me);
					if (veip_me) {	// if the ppup uni has same devname as specific veip, treat it as veip
						veip_bport_total++;
					} else {
						uni_bport_total++;
					}
				}
				break;
			case 5:	// GEM interworking termination point
			case 3:	// 802.1p mapper service profile
			case 6:	// Multicast GEM interworking termination point
				wan_bport_total++;
				if (me_related(me, found_veip_bridge_me))
					veip_wan_bport_total++;
				break;
			case 4:	// IP host config data
				iphost_bport_total++;
				break;
			case 11:// VEIP
				veip_bport_total++;
				break;
		}
	}		
	if (omci_env_g.autouni_enable==1 && uni_bport_total > 0)
		return 0;
	if (veip_wan_bport_total == 0)
		return 0;
	if (veip_bport_total == 0)
		return 0;

	if (found_veip_bridge_me)		
		*single_bridge_meid = found_veip_bridge_me->meid;
	else
		*single_bridge_meid = 0xffff;
	return 1;
}

static int
autouni_allocate_private_me(unsigned short classid, struct me_t **allocated_me)
{
	short instance_num;

	if ((instance_num = meinfo_instance_alloc_free_num(classid)) < 0)
		return -1;
	if ((*allocated_me = meinfo_me_alloc(classid, instance_num)) == NULL)
		return -1;

	// set default from config
	meinfo_me_load_config(*allocated_me);
	// assign meid
	if (meinfo_me_allocate_rare_meid(classid, &(*allocated_me)->meid) < 0)
	{
		meinfo_me_release(*allocated_me);
		return -1;
	}
	//create by ont
	(*allocated_me)->is_cbo = 0;
	//set it as private
	(*allocated_me)->is_private = 1;
	return 0;
}

static int
autouni_detach(struct autouni_info_t *autouni_info)
{
	struct meinfo_t *miptr = meinfo_util_miptr(11);
	struct me_t *uni_bport_me, *me;

	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		if (me->instance_num>=16)	// assume pptp uni < 16
			continue;		
		if (autouni_info->bport_meid[me->instance_num] != 0xffff) {
			uni_bport_me=meinfo_me_get_by_meid(47, autouni_info->bport_meid[me->instance_num]);
			// del uni bport me
			if (uni_bport_me && uni_bport_me->is_private)
				meinfo_me_delete_by_instance_num(47, uni_bport_me->instance_num);
			autouni_info->bport_meid[me->instance_num] = 0xffff;
		}
	}

	dbprintf_bat(LOG_ERR, "detach UNIs from bridge 0x%x\n", autouni_info->bridge_meid);

	autouni_info->bridge_meid = 0xffff;
	autouni_info->is_attached = 0;

	return 0;
}

static int
autouni_attach(struct autouni_info_t *autouni_info)
{
	struct meinfo_t *miptr = meinfo_util_miptr(11);
	struct me_t *bridge_me = meinfo_me_get_by_meid(45, autouni_info->bridge_meid);
	int veip_meid = autouni_get_veip_meid_by_bridge_me(bridge_me);
	struct me_t *me;

	if (!bridge_me || veip_meid <0)
		return -1;

	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		unsigned char MAYBE_UNUSED attr_mask[2]={0, 0};
		struct me_t *uni_bport_me;

		if (me->instance_num>=16)	// assume pptp uni < 16
			continue;		
		
		// create class 47 MAC_bridge_port_configuration_data
		if (autouni_allocate_private_me(47, &uni_bport_me) < 0) {
			if(omci_env_g.autouni_enable==2) {
				continue;
			} else {
				autouni_detach(autouni_info);
				return -2;
			}
		}
			
		//assign the attrs
		uni_bport_me->attr[1].value.data = autouni_info->bridge_meid; //assign bridge
		uni_bport_me->attr[2].value.data = 0xf0 + me->instance_num; //portnum
		uni_bport_me->attr[3].value.data = 1; //tp type, Physical path termination point Ethernet UNI
		uni_bport_me->attr[4].value.data = me->meid; //pointer to this pptp uni

		// attach private uni port to veip bridge only if related unig veip pointer matches the bridge veip
		if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX) == 0) {
			struct me_t *uni_g_me = meinfo_me_get_by_meid(264, me->meid);
		   	if (!uni_g_me || veip_meid != (unsigned short)meinfo_util_me_attr_data(uni_g_me, 4)) {
	   			//uni_bport_me->attr[1].value.data = 0xffff;	// disconnect the bport from the veip bridge
	   			continue;	// do not create the private uni bport is the uni is not veip related
			}
		}

		//attach to instance list
		if (meinfo_me_create(uni_bport_me) < 0) {
			if(omci_env_g.autouni_enable==2) {
				continue;
			} else {
				//release
				meinfo_me_delete_by_instance_num(uni_bport_me->classid, uni_bport_me->instance_num);
				meinfo_me_release(uni_bport_me);
				autouni_detach(autouni_info);
				return -3;
			}
		}

		autouni_info->bport_meid[me->instance_num] = uni_bport_me->meid;
#if 0		
		// flush the pptp_uni me to trigger add event for er_group
		// not necessary, this is already done in meinfo_me_create()
		util_attr_mask_set_bit(attr_mask, 5);	// Administrative_state 
		util_attr_mask_set_bit(attr_mask, 6);	// Operational_state
		meinfo_me_flush(me, attr_mask);
#endif
	}
	
	autouni_info->is_attached = 1;

	dbprintf_bat(LOG_ERR, "attach UNIs to bridge 0x%x\n", autouni_info->bridge_meid);	
	return 0;
}

#if 0
static int
autouni_update(struct autouni_info_t *autouni_info)
{
	struct meinfo_t *miptr = meinfo_util_miptr(11);
	struct me_t *uni_bport_me, *me;

	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		if (me->instance_num>=16)	// assume pptp uni < 16
			continue;		
		if (autouni_info->bport_meid[me->instance_num] != 0xffff) {
			uni_bport_me=meinfo_me_get_by_meid(47, autouni_info->bport_meid[me->instance_num]);
			// update bridge_meid in uni bport me
			if (uni_bport_me && uni_bport_me->is_private)
				uni_bport_me->attr[1].value.data = autouni_info->bridge_meid;
		}
	}

	dbprintf_bat(LOG_ERR, "update UNIs with bridge 0x%x\n", autouni_info->bridge_meid);
	return 0;
}
#endif

////////////////////////////////////////////////////

// auto create uni bport me for single bridge

static int
batchtab_cb_autouni_init(void)
{
	int i;

	autouni_info_g.bridge_meid = 0xffff;	
	for (i=0; i<16; i++)
		autouni_info_g.bport_meid[i] = 0xffff;
	autouni_info_g.is_attached = 0;
	
	return 0;
}

static int
batchtab_cb_autouni_finish(void)
{
	autouni_detach(&autouni_info_g);
	return 0;
}

static int
batchtab_cb_autouni_gen(void **table_data)
{
	struct autouni_info_t *a = &autouni_info_g;
	unsigned short bridge_meid;
	struct me_t *bridge_me;
#if 0
	if (autouni_is_required(&bridge_meid)) {
		if (!a->is_attached) {				// autouni not attached
			a->bridge_meid = bridge_meid;
			autouni_attach(a);
		} else if ( a->bridge_meid != bridge_meid ) {	// autouni attached but beidge meid is different
			a->bridge_meid = bridge_meid;
			autouni_update(a);
		}
	} else {
		if (a->is_attached)
			autouni_detach(a);
	}
#endif
	if (a->is_attached)
		autouni_detach(a);
	if (autouni_is_required(&bridge_meid)) {
		a->bridge_meid = bridge_meid;
		autouni_attach(a);
	}
	*table_data = a;

	//trigger classf/tagging/filtering
	batchtab_omci_update("classf");
	batchtab_omci_update("tagging");
	batchtab_omci_update("filtering");
	
	// trigger mac limiting settings in er_group_45_47 for veip related uni
	// which would ensure veip related uni is detected by autouni and the sa-learning action is set to 'trap to cpu'
	if ((bridge_me = meinfo_me_get_by_meid(45, bridge_meid)) != NULL) {
		unsigned char MAYBE_UNUSED attr_mask[2]={0, 0};
		util_attr_mask_set_bit(attr_mask, 2);	// Learning_ind
		meinfo_me_flush(bridge_me, attr_mask);
		dbprintf_bat(LOG_WARNING, "flush bridge me 0x%x, attr 2 (Learning_ind) for mac limiting\n", bridge_meid);
	}
		
	return 0;
}

static int
batchtab_cb_autouni_release(void *table_data)
{
	return 0;
}

static int
batchtab_cb_autouni_dump(int fd, void *table_data)
{
	struct autouni_info_t *a = table_data;
	int i;

	if (a == NULL)
		return 0;
	
	util_fdprintf(fd, "is_attached=%d\n", a->is_attached);
	util_fdprintf(fd, "veip bridge meid=0x%x\n", a->bridge_meid);
	for (i=0; i<16; i++) {
		if (a->bport_meid[i] != 0xffff)
			util_fdprintf(fd, "bport meid=0x%x\n", a->bport_meid[i]);
	}
	return 0;
}

static int
batchtab_cb_autouni_hw_sync(void *table_data)
{
	return 0;
}

// public register function /////////////////////////////////////
int
batchtab_cb_autouni_register(void)
{
	return batchtab_register("autouni", BATCHTAB_OMCI_UPDATE_NOAUTO, 3,
			batchtab_cb_autouni_init,
			batchtab_cb_autouni_finish,
			batchtab_cb_autouni_gen,
			batchtab_cb_autouni_release,
			batchtab_cb_autouni_dump,
			batchtab_cb_autouni_hw_sync);
}

// quick function to detach autouni when 
// a. olt wants to create uni port 
// b. olt deletes bport because of mib_reset
int
batchtab_cb_autouni_detach(void)
{
	// is_deteaching flag is to avoid endless recursive between me47_delete_cb & autouni_detach
	// eg: mib_reset -> me47_delete -> autouni_detach -> me47_delete -> autouni_detach....	
	//static int is_detaching = 0;
	if (autouni_info_g.is_attached) {// && !is_detaching) {
		//is_detaching = 1;	
		autouni_detach(&autouni_info_g);
	}
	//is_detaching = 0;
	return 0;
}
