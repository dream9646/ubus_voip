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
 * File    : meinfo_cb_6.c
 *
 ******************************************************************/

#include "meinfo.h"
#include "meinfo_cb.h"
#include "me_related.h"
#include "omcimsg.h"
#include "util.h"

//classid 6 9.1.6 Circuit_pack

static int
meinfo_cb_6_create(struct me_t *me, unsigned char attr_mask[2])
{
	struct attr_value_t attr;
	struct meinfo_t *miptr = NULL;
	struct me_t *meptr = NULL;
	int port_count = 0;
	unsigned char unit_type = (unsigned char)meinfo_util_me_attr_data(me, 1);
	
	if (me == NULL || 
		(unit_type == 32 && omci_env_g.voip_enable == ENV_VOIP_DISABLE))
	{
		return -1;
	}
	
	// Number_of_ports
	switch (unit_type) {
		case 22:	// 10BASE-T
		case 23:	// 100BASE-T
		case 24:	// 10-100BASE-T
		case 34:	// Gigabit optical Ethernet
		case 47:	// Ethernet BASE-T
			miptr= meinfo_util_miptr(11);
			list_for_each_entry(meptr, &miptr->me_instance_list, instance_node)
				port_count++;
			attr.ptr = NULL;
			attr.data = port_count;
			meinfo_me_attr_set(me, 2, &attr, 0);
			attr.data = (port_count) ? 0 : 1;
			meinfo_me_attr_set(me, 6, &attr, 0);
			attr.data = (port_count) ? 0 : 1;
			meinfo_me_attr_set(me, 7, &attr, 0);
			break;
		case 32:	// POTS
			if(omci_env_g.voip_enable != ENV_VOIP_DISABLE) {
				miptr= meinfo_util_miptr(53);
				list_for_each_entry(meptr, &miptr->me_instance_list, instance_node)
					port_count++;
			}
			attr.ptr = NULL;
			attr.data = port_count;
			meinfo_me_attr_set(me, 2, &attr, 0);
			attr.data = (port_count) ? 0 : 1;
			meinfo_me_attr_set(me, 6, &attr, 0);
			attr.data = (port_count) ? 0 : 1;
			meinfo_me_attr_set(me, 7, &attr, 0);
			break;
		case 38:	// VIDEO
			miptr= meinfo_util_miptr(82);
			list_for_each_entry(meptr, &miptr->me_instance_list, instance_node)
				port_count++;
			attr.ptr = NULL;
			attr.data = port_count;
			meinfo_me_attr_set(me, 2, &attr, 0);
			attr.data = (port_count) ? 0 : 1;
			meinfo_me_attr_set(me, 6, &attr, 0);
			attr.data = (port_count) ? 0 : 1;
			meinfo_me_attr_set(me, 7, &attr, 0);
			break;
		case 48:	// VEIP
			miptr= meinfo_util_miptr(329);
			list_for_each_entry(meptr, &miptr->me_instance_list, instance_node)
				port_count++;
			attr.ptr = NULL;
			attr.data = port_count;
			meinfo_me_attr_set(me, 2, &attr, 0);
			attr.data = (port_count) ? 0 : 1;
			meinfo_me_attr_set(me, 6, &attr, 0);
			attr.data = (port_count) ? 0 : 1;
			meinfo_me_attr_set(me, 7, &attr, 0);
			break;
		default:
			break;
	}
	return 0;
}

static int
meinfo_cb_6_set_admin_lock(struct me_t *me, int lockval)
{
	struct attr_value_t attr={ .data=lockval?1:0, .ptr=NULL, };	
	meinfo_me_attr_set(me, 6, &attr, 0);
	
	// lock child class, all types of pptp uni/ani...
	meinfo_me_set_child_admin_lock(me, 11, lockval);	// pptp etherbet uni
	//meinfo_me_set_child_admin_lock(me, 12, lockval);	// pptp ces uni
	meinfo_me_set_child_admin_lock(me, 53, lockval);	// pptp pots uni
	//meinfo_me_set_child_admin_lock(me, 80, lockval);	// pptp isdn uni
	//meinfo_me_set_child_admin_lock(me, 82, lockval);	// pptp video uni
	//meinfo_me_set_child_admin_lock(me, 83, lockval);	// ppt lct uni
	//meinfo_me_set_child_admin_lock(me, 90, lockval);	// pptp video ani
	//meinfo_me_set_child_admin_lock(me, 91, lockval);	// pptp 802.11 uni
	//meinfo_me_set_child_admin_lock(me, 98, lockval);	// pptp xdsl uni
	//meinfo_me_set_child_admin_lock(me, 162, lockval);	// pptp moca uni
	return 0;
}

static int 
meinfo_cb_6_is_admin_locked(struct me_t *me)
{
	struct me_t *parent_me;

	if( meinfo_util_me_attr_data(me, 6))
		return 1;

	parent_me=me_related_find_parent_me(me, 256);	//circuit pack's parent is ONT-G classid 256
	if(parent_me){
		struct meinfo_t *miptr=meinfo_util_miptr(parent_me->classid);
		if (miptr && miptr->callback.is_admin_locked_cb)
			return miptr->callback.is_admin_locked_cb(parent_me);
	}
	return 0;
}

/*
struct omcimsg_test_ont_ani_circuit_t {
	unsigned char select_test;
	unsigned char padding[31];
} __attribute__ ((packed));
*/
static int 
meinfo_cb_6_test_is_supported(struct me_t *me, unsigned char *req)
{
	struct omcimsg_test_ont_ani_circuit_t *test_req = (struct omcimsg_test_ont_ani_circuit_t *) req;

	if (test_req->select_test==7)		// selft test
		return 1;
	if (test_req->select_test>=8 && test_req->select_test<=15)	// vendor test
		return 0;
	
	return 0;
}

/*
struct omcimsg_test_result_ont_circuit_selftest_t {
	unsigned char unused;
	unsigned char self_test_result;
	unsigned char padding[30];
} __attribute__ ((packed));
*/
static int 
meinfo_cb_6_test(struct me_t *me, unsigned char *req, unsigned char *result)
{
	struct omcimsg_test_ont_ani_circuit_t *test_req = (struct omcimsg_test_ont_ani_circuit_t *)req;
	struct omcimsg_test_result_ont_circuit_selftest_t *test_result =(struct omcimsg_test_result_ont_circuit_selftest_t *)result;

	if (test_req->select_test==7)	// always return pass for selft test
		test_result->self_test_result=OMCIMSG_TEST_RESULT_SELFTEST_PASSED;
	else
		test_result->self_test_result=OMCIMSG_TEST_RESULT_SELFTEST_FAILED;
	return 0;
}

struct meinfo_callback_t meinfo_cb_6 = {
	.create_cb		= meinfo_cb_6_create,
	.set_admin_lock_cb	= meinfo_cb_6_set_admin_lock,
	.is_admin_locked_cb	= meinfo_cb_6_is_admin_locked,
	.test_is_supported_cb	= meinfo_cb_6_test_is_supported,
	.test_cb		= meinfo_cb_6_test,
};

