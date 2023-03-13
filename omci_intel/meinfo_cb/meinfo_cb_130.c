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
 * File    : meinfo_cb_130.c
 *
 ******************************************************************/

#include <string.h>
#include "meinfo_cb.h"
#include "me_related.h"
#include "util.h"
#include "proprietary_alu.h"

//classid 130 9.3.10 802.1p_mapper_service_profile

static int
get_classid_by_130_tp_type(unsigned char tp_type)
{
	switch (tp_type) {
	case 0:	return 0;	// ignore
	case 1:	return 11;	// pptp_ethernet_uni
	case 2: return 134;	// ip host config data
	case 3: return 286;	// ethernet flow tp
	case 4: return 98;	// pptp_xdsl_part1
	case 5: return 91;	// pptp_802.11_uni
	case 6: return 162;	// pptp moca uni
	case 7: return 329;     // virtual ethernet interface point
	case 8: return 14;      // Interworking_VCC_termination_point
	}
	dbprintf(LOG_ERR, "tp_type %d is undefined, assume pptp ethernet uni\n", tp_type);	
	return 11;	// not support, treat as pptp_ethernet_uni
}

static int
meinfo_cb_130_get_pointer_classid(struct me_t *me, unsigned char attr_order)
{
	if (attr_order==1)
		return get_classid_by_130_tp_type(meinfo_util_me_attr_data(me, 13));
	return -1;
}

static int 
meinfo_cb_130_is_attr_valid(struct me_t *me, unsigned char attr_order, struct attr_value_t *attr)
{
	if (attr_order==13) {		// tp type
		if (get_classid_by_130_tp_type(attr->data)<0)
			return 0;
	}
	return 1;
}

static int
meinfo_cb_130_allocate_private_me(unsigned short classid, struct me_t **allocated_me)
{
	short instance_num;

	if ((instance_num = meinfo_instance_alloc_free_num(classid)) < 0) {
		return -1;
	}

	if ((*allocated_me = meinfo_me_alloc(classid, instance_num)) == NULL) {
		return -1;
	}

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

//0: not 1:M model, 1: yes
static int
meinfo_cb_130_check_1_to_M_model_condition(struct me_t *me)
{
	struct attr_value_t attr_value_type, attr_value_pointer;

	if (me == NULL || me->classid != 130)
	{
		return 0;
	}

	meinfo_me_attr_get(me, 13, &attr_value_type);
	meinfo_me_attr_get(me, 1, &attr_value_pointer);

	if (attr_value_type.data == 1 &&
		meinfo_me_is_exist(11, attr_value_pointer.data))
	{
		return 1;
	} else {
		return 0;
	}
}

static int 
meinfo_cb_130_create_1_to_M_model(struct me_t *me)
{
	struct attr_value_t attr_value_type, attr_value_pointer;
	struct me_t *me_bridge, *me_bridge_port0, *me_bridge_port1;

	if (me == NULL)
	{
		return -1;
	}
	
	//check 130 pointer type = class 11, pptp ethernet uni
	if (meinfo_cb_130_check_1_to_M_model_condition(me))
	{
		meinfo_me_attr_get(me, 13, &attr_value_type);
		meinfo_me_attr_get(me, 1, &attr_value_pointer);

		//create one bridge and two ports
		//create class 45 MAC_bridge_service_profile
		if (meinfo_cb_130_allocate_private_me(45, &me_bridge) < 0)
		{
			return -1;
		}

		//attach to instance list
		if (meinfo_me_create(me_bridge) < 0) {
			//release
			meinfo_me_release(me_bridge);
			return -1;
		}

		//create class 47 MAC_bridge_port_configuration_data
		if (meinfo_cb_130_allocate_private_me(47, &me_bridge_port0) < 0)
		{
			meinfo_me_delete_by_instance_num(me_bridge->classid, me_bridge->instance_num);
			return -1;
		}

		//assign the relation, bridge port 0
		me_bridge_port0->attr[1].value.data = me_bridge->meid; //assign bridge
		me_bridge_port0->attr[3].value.data = 3; //tp type, 802.1p mapper service profile
		me_bridge_port0->attr[4].value.data = me->meid; //pointer to this me

		//attach to instance list
		if (meinfo_me_create(me_bridge_port0) < 0) {
			//release
			meinfo_me_delete_by_instance_num(me_bridge->classid, me_bridge->instance_num);
			meinfo_me_release(me_bridge_port0);
			return -1;
		}

		//create class 47 MAC_bridge_port_configuration_data
		if (meinfo_cb_130_allocate_private_me(47, &me_bridge_port1) < 0)
		{
			meinfo_me_delete_by_instance_num(me_bridge->classid, me_bridge->instance_num);
			meinfo_me_delete_by_instance_num(me_bridge_port0->classid, me_bridge_port0->meid);
			return -1;
		}

		//assign the relation, bridge port 1
		me_bridge_port1->attr[1].value.data = me_bridge->meid; //assign bridge
		me_bridge_port1->attr[3].value.data = 1; //tp type, pptp ethernet uni
		me_bridge_port1->attr[4].value.data = attr_value_pointer.data; //pointer to me's tp pointer

		//attach to instance list
		if (meinfo_me_create(me_bridge_port1) < 0) {
			//release
			meinfo_me_delete_by_instance_num(me_bridge->classid, me_bridge->meid);
			meinfo_me_delete_by_instance_num(me_bridge_port0->classid, me_bridge_port0->meid);
			meinfo_me_release(me_bridge_port1);
			return -1;
		}

		return 0;
	}else {
		return -1;
	}
}

//0: not exist, 1: exist
static int
meinfo_cb_130_check_1_to_M_model_exist(struct me_t *me,
	struct me_t **me_bridge,
	struct me_t **me_bridge_port0,
	struct me_t **me_bridge_port1)
{
	struct meinfo_t *miptr;
	struct me_t *pptp_bridge_port;

	if (me == NULL)
	{
		return -1;
	}

	//check all bridge port pointer to this me, is_private is true;
	if ((*me_bridge_port0 = me_related_find_child_me(me, 47)) == NULL
		|| (*me_bridge_port0)->is_private == 0)
	{
		return 0;
	}
	
	//check all bridge that this me pointer to, is_private is true
	if ((*me_bridge = me_related_find_parent_me(*me_bridge_port0, 45)) == NULL
		|| (*me_bridge)->is_private == 0)
	{
		return 0;
	}

	if ((miptr=meinfo_util_miptr(47)) == NULL)
	{
		return 0;
	}

	//check all bridge port
	list_for_each_entry(pptp_bridge_port, &miptr->me_instance_list, instance_node)
	{
		if (pptp_bridge_port->is_private == 1 && 
			pptp_bridge_port->attr[1].value.data == (*me_bridge)->meid &&
			pptp_bridge_port->attr[3].value.data == 1 && //tp type is pptp ethernet uni
			meinfo_me_is_exist(11, pptp_bridge_port->attr[4].value.data)) //??
		{
			*me_bridge_port1=pptp_bridge_port;
			return 1; //exist
		}
	}

	return 0;
}

static int 
meinfo_cb_130_create(struct me_t *me, unsigned char attr_mask[2])
{
	if (me == NULL)
	{
		return -1;
	}

	if (meinfo_cb_130_check_1_to_M_model_condition(me) == 1)
	{
		if (meinfo_cb_130_create_1_to_M_model(me) < 0)
		{
			dbprintf(LOG_ERR, "create 1:M model error: classid=%u, meid=%u\n", me->classid, me->meid);
			return -1;
		}
	}
	
	return 0;
}

static int 
meinfo_cb_130_delete(struct me_t *me)
{
	struct me_t *me_bridge, *me_bridge_port0, *me_bridge_port1;

	if (me == NULL)
	{
		return -1;
	}

	if (meinfo_cb_130_check_1_to_M_model_condition(me) == 1)
	{
		if (meinfo_cb_130_check_1_to_M_model_exist(me, &me_bridge, &me_bridge_port0, &me_bridge_port1) == 1)
		{
			meinfo_me_delete_by_instance_num(me_bridge->classid, me_bridge->meid);
			meinfo_me_delete_by_instance_num(me_bridge_port0->classid, me_bridge_port0->meid);
			meinfo_me_delete_by_instance_num(me_bridge_port1->classid, me_bridge_port1->meid);
		}
	}

	return 0;
}

static int 
meinfo_cb_130_set(struct me_t *me, unsigned char attr_mask[2])
{
	struct me_t *me_bridge, *me_bridge_port0, *me_bridge_port1;

	if (me == NULL)
	{
		return -1;
	}

	if (meinfo_cb_130_check_1_to_M_model_condition(me) == 1)
	{
		if (meinfo_cb_130_check_1_to_M_model_exist(me, &me_bridge, &me_bridge_port0, &me_bridge_port1) == 0)
		{
			if (meinfo_cb_130_create_1_to_M_model(me) < 0)
			{
				dbprintf(LOG_ERR, "create 1:M model error: classid=%u, meid=%u\n", me->classid, me->meid);
				return -1;
			}
		}
	} else {
		if (meinfo_cb_130_check_1_to_M_model_exist(me, &me_bridge, &me_bridge_port0, &me_bridge_port1) == 1)
		{
			meinfo_me_delete_by_instance_num(me_bridge->classid, me_bridge->meid);
			meinfo_me_delete_by_instance_num(me_bridge_port0->classid, me_bridge_port0->meid);
			meinfo_me_delete_by_instance_num(me_bridge_port1->classid, me_bridge_port1->meid);
		}
	}
	
	return 0;
}

struct meinfo_callback_t meinfo_cb_130 = {
	.create_cb			= meinfo_cb_130_create,
	.delete_cb			= meinfo_cb_130_delete,
	.set_cb				= meinfo_cb_130_set,
	.get_pointer_classid_cb	= meinfo_cb_130_get_pointer_classid,
	.is_attr_valid_cb		= meinfo_cb_130_is_attr_valid,
};

