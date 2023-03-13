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
 * File    : batchtab_cb_autoveip.c
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

static struct autoveip_info_t autoveip_info_g;

int 
autoveip_get_uni_meid_by_bridge_me(struct me_t *bridge_me)
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
			if ((unsigned char)meinfo_util_me_attr_data(bport_me, 3) == 1)	// this is a uni bport
				return (unsigned short)meinfo_util_me_attr_data(bport_me, 4);	// uni meid
		} 
	}
	return -1;
}

// a. number of bridge with uni == 1
// b. number of veip == 0
// c. number of uni >0
// d. number of wan port >0
// e. admin state of veip == 0

unsigned int
autoveip_is_required(unsigned short *single_bridge_meid)
{
	struct meinfo_t *miptr;
	struct me_t *me;
	struct me_t *found_uni_bridge_me = NULL;
	int uni_bridge_total = 0;		// uni bridge
	int uni_bport_total = 0;		// uni on all bridge
	int wan_bport_total = 0;		// wan on all bridge
	int uni_wan_bport_total = 0;		// wan on uni bridge
	int veip_bport_total = 0;		// veip on all bridge
	int iphost_bport_total = 0;

	if (!omci_env_g.autoveip_enable)
		return 0;

	miptr = meinfo_util_miptr(45);
	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
	
		if (me->is_private)
			continue;		
		if (autoveip_get_uni_meid_by_bridge_me(me)>=0) {
			uni_bridge_total++;
			if (found_uni_bridge_me == NULL)
				found_uni_bridge_me = me;
		}
	}
	if (uni_bridge_total!=1)
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
				if (me_related(me, found_uni_bridge_me))
					uni_wan_bport_total++;
				break;
			case 4:	// IP host config data
				iphost_bport_total++;
				break;
			case 11:// VEIP
				veip_bport_total++;
				break;
		}
	}		
	if (omci_env_g.autoveip_enable==1 && veip_bport_total > 0)
		return 0;
	if (uni_wan_bport_total == 0)
		return 0;
	if (uni_bport_total == 0)
		return 0;

	if (found_uni_bridge_me)		
		*single_bridge_meid = found_uni_bridge_me->meid;
	else
		*single_bridge_meid = 0xffff;
	return 1;
}

static int
autoveip_allocate_private_me(unsigned short classid, struct me_t **allocated_me)
{
	short instance_num;

	if ((instance_num = meinfo_instance_alloc_free_num(classid)) < 0)
		return -1;
	if ((*allocated_me = meinfo_me_alloc(classid, instance_num)) == NULL)
		return -1;

	// set default from config
	meinfo_me_load_config(*allocated_me);
	// assign meid
	if (meinfo_me_allocate_rare2_meid(classid, &(*allocated_me)->meid) < 0)
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
autoveip_detach(struct autoveip_info_t *autoveip_info)
{
	struct meinfo_t *miptr = meinfo_util_miptr(329);
	struct me_t *veip_bport_me, *me;

	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		if (me->instance_num>=16)	// assume veip < 16
			continue;		
		if (autoveip_info->bport_meid[me->instance_num] != 0xffff) {
			veip_bport_me=meinfo_me_get_by_meid(47, autoveip_info->bport_meid[me->instance_num]);
			// del veip bport me
			if (veip_bport_me && veip_bport_me->is_private)
				meinfo_me_delete_by_instance_num(47, veip_bport_me->instance_num);
			autoveip_info->bport_meid[me->instance_num] = 0xffff;
		}
	}

	dbprintf_bat(LOG_ERR, "detach VEIPs from bridge 0x%x\n", autoveip_info->bridge_meid);

	autoveip_info->bridge_meid = 0xffff;
	autoveip_info->is_attached = 0;

	return 0;
}

static int
autoveip_attach(struct autoveip_info_t *autoveip_info)
{
	struct meinfo_t *miptr = meinfo_util_miptr(329);
	struct me_t *bridge_me = meinfo_me_get_by_meid(45, autoveip_info->bridge_meid);
	int uni_meid = autoveip_get_uni_meid_by_bridge_me(bridge_me);
	struct me_t *me;

	if (!bridge_me || uni_meid <0)
		return -1;

	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		unsigned char MAYBE_UNUSED attr_mask[2]={0, 0};
		struct me_t *veip_bport_me;
		
		if ((unsigned char)meinfo_util_me_attr_data(me, 1) != 0)	// attach veip which admin_state==0
			continue;

		if (me->instance_num>=16)	// assume veip < 16
			continue;		
		
		// create class 47 MAC_bridge_port_configuration_data
		if (autoveip_allocate_private_me(47, &veip_bport_me) < 0) {
			if(omci_env_g.autoveip_enable==2) {
				continue;
			} else {
				autoveip_detach(autoveip_info);
				return -2;
			}
		}
			
		//assign the attrs
		veip_bport_me->attr[1].value.data = autoveip_info->bridge_meid; //assign bridge
		veip_bport_me->attr[2].value.data = 0xf0 + me->instance_num; //portnum
		veip_bport_me->attr[3].value.data = 11; //tp type, VEIP
		veip_bport_me->attr[4].value.data = me->meid; //pointer to this veip

		// attach private veip port to uni bridge only if related veip isn't used in other veip bridge
		if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX) == 0) {
			struct meinfo_t *miptr_bport = meinfo_util_miptr(47);
			struct me_t *me_bport = NULL;
			int has_veip_bport_me = 0;
			list_for_each_entry(me_bport, &miptr_bport->me_instance_list, instance_node) {
				struct attr_value_t attr_tptype;
				struct attr_value_t attr_tppointer;
				// get public me attr3 (TP type)
				meinfo_me_attr_get(me_bport, 3, &attr_tptype);
				// get public me attr4 (TP pointer)
				meinfo_me_attr_get(me_bport, 4, &attr_tppointer);
				if(attr_tptype.data==11 && attr_tppointer.data==me->meid) {
					has_veip_bport_me = 1;
					break;
				}
			}
		   	if (has_veip_bport_me) {
	   			//veip_bport_me->attr[1].value.data = 0xffff;	// disconnect the bport from the uni bridge
	   			continue;	// do not create the private veip bport is the veip is not uni related
			}
			if(omci_env_g.olt_workaround_enable == ENV_OLT_WORKAROUND_CALIX) {
				// workaround to skip meid 0x302 (the real veip used for rg service)
				if(me->instance_num==1) veip_bport_me->meid |= 0xf000;
			}
		}

		//attach to instance list
		if (meinfo_me_create(veip_bport_me) < 0) {
			if(omci_env_g.autoveip_enable==2) {
				continue;
			} else {
				//release
				meinfo_me_delete_by_instance_num(veip_bport_me->classid, veip_bport_me->instance_num);
				meinfo_me_release(veip_bport_me);
				autoveip_detach(autoveip_info);
				return -3;
			}
		}

		autoveip_info->bport_meid[me->instance_num] = veip_bport_me->meid;
#if 0
		// flush the veip me to trigger add event for er_group
		// not necessary, this is already done in meinfo_me_create()
		util_attr_mask_set_bit(attr_mask, 1);	// Administrative_state 
		util_attr_mask_set_bit(attr_mask, 2);	// Operational_state
		meinfo_me_flush(me, attr_mask);
#endif
	}
	
	autoveip_info->is_attached = 1;

	dbprintf_bat(LOG_ERR, "attach VEIPs to bridge 0x%x\n", autoveip_info->bridge_meid);	
	return 0;
}

#if 0
static int
autoveip_update(struct autoveip_info_t *autoveip_info)
{
	struct meinfo_t *miptr = meinfo_util_miptr(329);
	struct me_t *veip_bport_me, *me;

	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		if (me->instance_num>=16)	// assume veip < 16
			continue;		
		if (autoveip_info->bport_meid[me->instance_num] != 0xffff) {
			veip_bport_me=meinfo_me_get_by_meid(47, autoveip_info->bport_meid[me->instance_num]);
			// update bridge_meid in veip bport me
			if (veip_bport_me && veip_bport_me->is_private)
				veip_bport_me->attr[1].value.data = autoveip_info->bridge_meid;
		}
	}

	dbprintf_bat(LOG_ERR, "update VEIPs with bridge 0x%x\n", autoveip_info->bridge_meid);
	return 0;
}
#endif

////////////////////////////////////////////////////

// auto create veip bport me for single bridge

static int
batchtab_cb_autoveip_init(void)
{
	int i;

	autoveip_info_g.bridge_meid = 0xffff;	
	for (i=0; i<16; i++)
		autoveip_info_g.bport_meid[i] = 0xffff;
	autoveip_info_g.is_attached = 0;
	
	return 0;
}

static int
batchtab_cb_autoveip_finish(void)
{
	autoveip_detach(&autoveip_info_g);
	return 0;
}

static int
batchtab_cb_autoveip_gen(void **table_data)
{
	struct autoveip_info_t *a = &autoveip_info_g;
	unsigned short bridge_meid;
#if 0
	if (autoveip_is_required(&bridge_meid)) {
		if (!a->is_attached) {				// autoveip not attached
			a->bridge_meid = bridge_meid;
			autoveip_attach(a);
		} else if ( a->bridge_meid != bridge_meid ) {	// autoveip attached but beidge meid is different
			a->bridge_meid = bridge_meid;
			autoveip_update(a);
		}
	} else {
		if (a->is_attached)
			autoveip_detach(a);
	}
#endif
	if (a->is_attached)
		autoveip_detach(a);
	if (autoveip_is_required(&bridge_meid)) {
		a->bridge_meid = bridge_meid;
		autoveip_attach(a);
	}
	*table_data = a;
	return 0;
}

static int
batchtab_cb_autoveip_release(void *table_data)
{
	return 0;
}

static int
batchtab_cb_autoveip_dump(int fd, void *table_data)
{
	struct autoveip_info_t *a = table_data;
	int i;

	if (a == NULL)
		return 0;
	
	util_fdprintf(fd, "is_attached=%d\n", a->is_attached);
	util_fdprintf(fd, "uni bridge meid=0x%x\n", a->bridge_meid);
	for (i=0; i<16; i++) {
		if (a->bport_meid[i] != 0xffff)
			util_fdprintf(fd, "bport meid=0x%x\n", a->bport_meid[i]);
	}
	return 0;
}

static int
batchtab_cb_autoveip_hw_sync(void *table_data)
{
	return 0;
}

// public register function /////////////////////////////////////
int
batchtab_cb_autoveip_register(void)
{
	return batchtab_register("autoveip", BATCHTAB_OMCI_UPDATE_NOAUTO, 3,
			batchtab_cb_autoveip_init,
			batchtab_cb_autoveip_finish,
			batchtab_cb_autoveip_gen,
			batchtab_cb_autoveip_release,
			batchtab_cb_autoveip_dump,
			batchtab_cb_autoveip_hw_sync);
}

// quick function to detach autoveip when 
// a. olt wants to create veip port 
// b. olt deletes bport because of mib_reset
int
batchtab_cb_autoveip_detach(void)
{
	// is_deteaching flag is to avoid endless recursive between me47_delete_cb & autoveip_detach
	// eg: mib_reset -> me47_delete -> autoveip_detach -> me47_delete -> autoveip_detach....	
	//static int is_detaching = 0;
	if (autoveip_info_g.is_attached) {// && !is_detaching) {
		//is_detaching = 1;	
		autoveip_detach(&autoveip_info_g);
	}
	//is_detaching = 0;
	return 0;
}
