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
 * File    : meinfo_cb_268.c
 *
 ******************************************************************/

#include "util.h"
#include "meinfo_cb.h"
#include "me_related.h"
#include "hwresource.h"
#include "omciutil_misc.h"
#include "batchtab.h"

//classid 268 GEM_port_network_CTP

static int 
meinfo_cb_268_get_pointer_classid(struct me_t *me, unsigned char attr_order)
{
	if (attr_order==4) {	// Traffic_management_pointer_for_upstream
		if (omciutil_misc_traffic_management_option_is_rate_control())
			return 262;	// TCONT pointer for ratectrl mode
		else
			return 277;	// PQ pointer for prio mode, prio+ratectrl mode
	}
	return -1;
}


static int
meinfo_cb_268_find_valid_tcont(void)
{
	struct meinfo_t *miptr_tcont = meinfo_util_miptr(262);
	struct me_t *me_tcont;
	
	list_for_each_entry(me_tcont, &miptr_tcont->me_instance_list, instance_node) {
		struct attr_value_t attr_allocid={0};
		meinfo_me_attr_get(me_tcont, 1, &attr_allocid);
		if (attr_allocid.data!=255)
			return me_tcont->meid;
	}
	return -1;
}

static int 
meinfo_cb_268_is_attr_valid(struct me_t *me, unsigned char attr_order, struct attr_value_t *attr)
{
	if (attr_order==1) {		// Port_id_value
		struct meinfo_t *miptr = meinfo_util_miptr(me->classid);
		struct me_t *me_gemctp;

		if ((unsigned short)(attr->data)==0xffff)
			return 1;			

		// check if the specified gemport has already been used by other gem ctp me
		list_for_each_entry(me_gemctp, &miptr->me_instance_list, instance_node) {
			struct attr_value_t attr_gemportid={0};
			if (me->meid!=me_gemctp->meid) {
				meinfo_me_attr_get(me_gemctp, 1, &attr_gemportid);
				if (attr->data==attr_gemportid.data) {
					dbprintf(LOG_ERR, "classid=%u, meid=%u, gemport id %d is already used by %u:0x%x(%u)\n",
						me->classid, me->meid, (unsigned short)(attr->data), 
						me_gemctp->classid, me_gemctp->meid, me_gemctp->meid);
					return 0;
				}
			}
		}
	}
	return 1;
}

// workaround for DASAN OLT which doesn't set Traffic_management_pointer_for_upstream correctly for priority mode 
static int 
meinfo_cb_268_set(struct me_t *me, unsigned char attr_mask[2])
{
	if (util_attr_mask_get_bit(attr_mask, 2)) {	// Tcont pointer
		struct attr_value_t attr_tcont_pointer={0};
		meinfo_me_attr_get(me, 2, &attr_tcont_pointer);		

		if (meinfo_me_get_by_meid(262, attr_tcont_pointer.data)==NULL) {
			if (omci_env_g.olt_workaround_enable == ENV_OLT_WORKAROUND_DASAN) {
				int tcont_meid=meinfo_cb_268_find_valid_tcont();
				if (tcont_meid>=0) {
					dbprintf(LOG_ERR, "classid=%u, meid=%u, change attr2(T_CONT_pointer) from 0x%x(%u) to 0x%x(%u), workaround for dasan!\n", 
						me->classid, me->meid, 
						(unsigned short)(attr_tcont_pointer.data), (unsigned short)(attr_tcont_pointer.data), 
						tcont_meid, tcont_meid);
					attr_tcont_pointer.data=tcont_meid;
					meinfo_me_attr_set(me, 2, &attr_tcont_pointer, 0);
				} else {
					dbprintf(LOG_ERR, "classid=%u, meid=%u, attr2(T_CONT_pointer)=0x%x(%u), workaround failed!\n", 
						me->classid, me->meid, 
						(unsigned short)(attr_tcont_pointer.data), (unsigned short)(attr_tcont_pointer.data));
				}
			} else {
				dbprintf(LOG_ERR, "classid=%u, meid=%u, attr2(T_CONT_pointer)=0x%x(%u), null pointer?\n", 
					me->classid, me->meid, 
					(unsigned short)(attr_tcont_pointer.data), (unsigned short)(attr_tcont_pointer.data));
			}
		}
	}

	if (util_attr_mask_get_bit(attr_mask, 4)) {	// Traffic_management_pointer_for_upstream
		int us_pointer_classid=meinfo_cb_268_get_pointer_classid(me, 4);
		struct attr_value_t attr_tcont_pointer={0};
		struct attr_value_t attr_upstream_pointer={0};

		meinfo_me_attr_get(me, 2, &attr_tcont_pointer);
		meinfo_me_attr_get(me, 4, &attr_upstream_pointer);
	
		if (us_pointer_classid==262) {		// TCONT, this is rate ctrl mode
			if (attr_upstream_pointer.data!=attr_tcont_pointer.data) {
				if (omci_env_g.olt_workaround_enable == ENV_OLT_WORKAROUND_DASAN ||
				    omci_env_g.olt_workaround_enable == ENV_OLT_WORKAROUND_OCCAM ) {
					dbprintf(LOG_ERR, "classid=%u, meid=%u, change attr4(Traffic_mgmt_ptr_for_US) from 0x%x(%u) to 0x%x(%u), workaround for dasan/occam!\n", 
						me->classid, me->meid, 
						(unsigned short)(attr_upstream_pointer.data), (unsigned short)(attr_upstream_pointer.data), 
						(unsigned short)(attr_tcont_pointer.data), (unsigned short)(attr_tcont_pointer.data));
					meinfo_me_attr_set(me, 4, &attr_tcont_pointer, 0);
				} else {
					dbprintf(LOG_ERR, "classid=%u, meid=%u, attr4(Traffic_mgmt_ptr_for_US) 0x%x(%u) should be the same as attr2(Tcont pointer) 0x%x(%u)\n", 
						me->classid, me->meid, 
						(unsigned short)(attr_upstream_pointer.data), (unsigned short)(attr_upstream_pointer.data), 
						(unsigned short)(attr_tcont_pointer.data), (unsigned short)(attr_tcont_pointer.data));
				}
				
			}
		} else if (us_pointer_classid==277) {	// PQ, this is priority ctrl mode
			if (meinfo_me_get_by_meid(277, attr_upstream_pointer.data)==NULL) {
				if (omci_env_g.olt_workaround_enable == ENV_OLT_WORKAROUND_DASAN ||
				    omci_env_g.olt_workaround_enable == ENV_OLT_WORKAROUND_OCCAM ) {
					int pq_meid=omciutil_misc_most_idle_pq_under_same_tcont(me, attr_tcont_pointer.data);
					if (pq_meid>=0) {
						dbprintf(LOG_ERR, "classid=%u, meid=%u, change attr4(Traffic_mgmt_ptr_for_US) from 0x%x(%u) to 0x%x(%u), workaround for dasan/occam!\n", 
							me->classid, me->meid, 
							(unsigned short)(attr_upstream_pointer.data), (unsigned short)(attr_upstream_pointer.data), 
							pq_meid, pq_meid);
						attr_upstream_pointer.data= pq_meid;	
						meinfo_me_attr_set(me, 4, &attr_upstream_pointer, 0);
					} else {
						dbprintf(LOG_ERR, "classid=%u, meid=%u, attr4(Traffic_mgmt_ptr_for_US)=0x%x(%u), workaround failed!\n", 
							me->classid, me->meid, 
							(unsigned short)(attr_upstream_pointer.data), (unsigned short)(attr_upstream_pointer.data));
					}
				} else {
					dbprintf(LOG_ERR, "classid=%u, meid=%u, attr4(Traffic_mgmt_ptr_for_US)=0x%x(%u), null pq pointer?\n", 
						me->classid, me->meid, 
						(unsigned short)(attr_upstream_pointer.data), (unsigned short)(attr_upstream_pointer.data));				
				}
			}
		}			
	}

	if (util_attr_mask_get_bit(attr_mask, 2) ||	// Tcont pointer
	    util_attr_mask_get_bit(attr_mask, 4)) {	// Traffic_management_pointer_for_upstream
		if (omciutil_misc_traffic_management_option_is_rate_control())
		        omciutil_misc_reassign_pq_for_gem_ctp_in_rate_control_mode(me);
	}
	
	batchtab_omci_update("linkready");
	return 0;
}

static int 
meinfo_cb_268_create(struct me_t *me, unsigned char attr_mask[2])
{
	return meinfo_cb_268_set(me, attr_mask);
}

struct meinfo_callback_t meinfo_cb_268 = {
	.get_pointer_classid_cb	= meinfo_cb_268_get_pointer_classid,
	.is_attr_valid_cb	= meinfo_cb_268_is_attr_valid,
	.set_cb			= meinfo_cb_268_set,
	.create_cb		= meinfo_cb_268_create,
};
