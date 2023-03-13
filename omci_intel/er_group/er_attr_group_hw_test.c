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
 * Module  : er_group
 * File    : er_attr_group_hw_test.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include "xmlomci.h"
#include "util.h"
#include "env.h"
#include "er_group.h"

/* a=1, b=2, c=3, d=4, e=5, f=6, g=7 */

int er_attr_group_hw_a_e(int action, struct er_attr_group_instance_t *er_attr_group_instance, struct me_t *me, unsigned char attr_mask[2])
{
	printf("%s\n", __FUNCTION__);
	return 0;
}
int er_attr_group_hw_a_f(int action, struct er_attr_group_instance_t *er_attr_group_instance, struct me_t *me, unsigned char attr_mask[2])
{
	printf("%s\n", __FUNCTION__);
	return 0;
}
int er_attr_group_hw_a_g(int action, struct er_attr_group_instance_t *er_attr_group_instance, struct me_t *me, unsigned char attr_mask[2])
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

int er_attr_group_hw_b_a(int action, struct er_attr_group_instance_t *er_attr_group_instance, struct me_t *me, unsigned char attr_mask[2])
{
	printf("%s\n", __FUNCTION__);
	return 0;
}
int er_attr_group_hw_b_c(int action, struct er_attr_group_instance_t *er_attr_group_instance, struct me_t *me, unsigned char attr_mask[2])
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

int er_attr_group_hw_b_d(int action, struct er_attr_group_instance_t *er_attr_group_instance, struct me_t *me, unsigned char attr_mask[2])
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

int main()
{
	//struct er_attr_group_hw_entity_t *er_attr_group_hw_entity;
	//int action=0; 
	//struct er_attr_group_instance_t er_attr_group_instance;

	env_set_to_default();
	omci_env_g.debug_level = LOG_DEBUG;

	er_attr_group_hw_preinit();

	/* a=1, b=2, c=3, d=4, e=5, f=6, g=7 */
	er_attr_group_hw_add("func_1_5",er_attr_group_hw_a_e);
	er_attr_group_hw_add("func_1_6",er_attr_group_hw_a_f);
	er_attr_group_hw_add("func_1_7",er_attr_group_hw_a_g);
	er_attr_group_hw_add("func_2_1",er_attr_group_hw_b_a);
	er_attr_group_hw_add("func_2_3",er_attr_group_hw_b_c);
	er_attr_group_hw_add("func_2_4",er_attr_group_hw_b_d);
	//test function duplicate
	//er_attr_group_hw_add("func_2_4",er_attr_group_hw_b_d);

	er_attr_group_hw_dump();
#if 0
	if ( (er_attr_group_hw_entity=er_attr_group_hw_find("func_1_6")) != 0 )
		er_attr_group_hw_entity->er_attr_group_hw_func(action, &er_attr_group_instance);
	else
		printf("func not found\n");
#endif

	er_attr_group_hw_del("func_1_5");
	er_attr_group_hw_del("func_1_6");
	er_attr_group_hw_del("func_1_7");
	er_attr_group_hw_del("func_2_1");
	er_attr_group_hw_del("func_2_3");
	//test function delete
	//er_attr_group_hw_del("func_2_4");
	er_attr_group_hw_dump();

	return 0;
}


