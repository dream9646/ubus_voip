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
 * Module  : meinfo_cb
 * File    : meinfo_cb_47.c
 *
 ******************************************************************/

#include "util.h"
#include "meinfo_cb.h"
#include "meinfo_cb.h"
#include "me_related.h"
#include "batchtab.h"
#include "batchtab_cb.h"
#include "omcimsg.h"

//classid 47 9.3.4 MAC bridge port ocnfiguration data

static int
get_classid_by_47_tp_type(unsigned char tp_type)
{
	switch (tp_type) {
	case 1:	return 11;	// pptp_ethernet_uni
	case 2: return 14;	// iw VCC tp
	case 3: return 130;	// 802.1p mapper service
	case 4: return 134;	// ip host config data
	case 5: return 266;	// gem iw tp
	case 6: return 281;	// mcast gem iwtp
	case 7: return 98;	// pptp_xdsl_part1
	case 8: return 98;	// pptp_vdsl_uni (vdsl_uni not found, use xdsl instead)
	case 9: return 286;	// ethernet flow tp
	case 10: return 91;	// pptp_802.11_uni
	case 11: return 329;	// virtual_ethernet
	case 12: return 162;	// pptp_moca_uni
	}
	dbprintf(LOG_ERR, "tp_type %d is undefined\n", tp_type);	
	return -1;
}

static int 
meinfo_cb_47_get_pointer_classid(struct me_t *me, unsigned char attr_order)
{
	if (attr_order==4) {
		return get_classid_by_47_tp_type(meinfo_util_me_attr_data(me, 3));
	}
	return -1;
}

static int 
meinfo_cb_47_is_attr_valid(struct me_t *me, unsigned char attr_order, struct attr_value_t *attr)
{
	if (attr_order==3) {		// tp type
		if (get_classid_by_47_tp_type(attr->data)<0)
			return 0;
		if (omci_env_g.autouni_enable) {
			if ((unsigned char)attr->data==1 && !me->is_private)	// this me is an non-private uni bport
				batchtab_cb_autouni_detach();
		}
		if (omci_env_g.autoveip_enable) {
			if ((unsigned char)attr->data==11 && !me->is_private)	// this me is an non-private veip bport
				batchtab_cb_autoveip_detach();
		}
	}
	return 1;
}

static int 
meinfo_cb_47_create(struct me_t *me, unsigned char attr_mask[2])
{
	if (util_attr_mask_get_bit(attr_mask, 3)) {	// tptype
		if (omci_env_g.olt_workaround_enable == ENV_OLT_WORKAROUND_ERICSSON) {
			struct attr_value_t attr;
			meinfo_me_attr_get(me, 3, &attr);
			// change tptype from 2(iw VCC tp) to 5(gem iwtp)
			if ((unsigned char)attr.data==2) {
				attr.data=5;
				meinfo_me_attr_set(me, 3, &attr, 0);
				dbprintf(LOG_ERR, "classid=%u, meid=%u, change attr2(tptype) from 2(iw_VCC) to 5(gem_iwtp), workaround for ericsson!\n",
					me->classid, me->meid); 
			}
		}
	}
	if (!me->is_private) {
		batchtab_omci_update("autouni");
		batchtab_omci_update("autoveip");
	}
	return 0;
}

static int 
meinfo_cb_47_delete(struct me_t *me)
{
	if (!me->is_private) {
		if (omcimsg_mib_reset_clear_is_in_progress()) {
			// if mibreset detected, del autouni also, or the autouni state maybe not sync
			// eg: while autouni me are deleted after mib_reset but autouni_info_g.is_attached==1
			batchtab_cb_autouni_detach();
			batchtab_cb_autoveip_detach();
		        // the above func should be called for non private me47 to avoid endless recursive
		        // eg: mib-_reset -> me47_delete_public -> autouni_detach -> me47_delete_private
			
		} else {
			batchtab_omci_update("autouni");
			batchtab_omci_update("autoveip");
		}
	}
	return 0;
}

static int 
meinfo_cb_47_set(struct me_t *me, unsigned char attr_mask[2])
{
	return meinfo_cb_47_create(me, attr_mask);
}


static int
meinfo_cb_47_is_ready(struct me_t *me)
{
	unsigned short classid;
	
	if (me == NULL)
		return 0;
	classid = get_classid_by_47_tp_type(meinfo_util_me_attr_data(me, 3));
	// a bport me is ready only if it has related parent me
	return  (classid > 0 && me_related_find_parent_me(me, classid));
}

#if 0
static int 
meinfo_cb_47_is_ready(struct me_t *me)
{
	int tp_type;

	if (me == NULL)
		return 0;

	// check 3 tp type on wan side 
	tp_type = meinfo_util_me_attr_data(me, 3);
	switch (tp_type) {
	case 3:		// 130 802.1p mapper service
		if (me_related_find_parent_me(me, 130)==NULL)
			return 0;
		break;
	case 5:		// 266 gem iw tp
		if (me_related_find_parent_me(me, 266)==NULL)
			return 0;
		break;
	case 6:		// 281 mcast gem iwtp
		if (me_related_find_parent_me(me, 281)==NULL)
			return 0;
		break;
	}

	return 1;
}
#endif

struct meinfo_callback_t meinfo_cb_47 = {
	.get_pointer_classid_cb	= meinfo_cb_47_get_pointer_classid,
	.is_attr_valid_cb	= meinfo_cb_47_is_attr_valid,
	.create_cb		= meinfo_cb_47_create,
	.delete_cb		= meinfo_cb_47_delete,
	.set_cb			= meinfo_cb_47_set,
	.is_ready_cb		= meinfo_cb_47_is_ready,
};
