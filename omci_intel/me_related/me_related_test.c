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
 * Module  : me_related
 * File    : me_related_test.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include "me_related.h"
#include "meinfo.h"
#include "xmlomci.h"
#include "util.h"
#include "env.h"

extern struct list_head me_related_hash[ME_RELATED_HASH_SIZE];
/* a=1, b=2, c=3, d=4, e=5, f=6, g=7 */

int me_related_a_e(struct me_t *me_ptr1, struct me_t *me_ptr2)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}
int me_related_a_f(struct me_t *me_ptr1, struct me_t *me_ptr2)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}
int me_related_a_g(struct me_t *me_ptr1, struct me_t *me_ptr2)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

int me_related_b_a(struct me_t *me_ptr1, struct me_t *me_ptr2)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}
int me_related_b_c(struct me_t *me_ptr1, struct me_t *me_ptr2)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

int me_related_b_d(struct me_t *me_ptr1, struct me_t *me_ptr2)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

int main()
{
	env_set_to_default();
	omci_env_g.debug_level = LOG_DEBUG;

	me_related_preinit();
	me_related_init();

	/* a=1, b=2, c=3, d=4, e=5, f=6, g=7 */
	me_related_add(ME_RELATED_FUNC_SPEC, 1,5,me_related_a_e);
	me_related_add(ME_RELATED_FUNC_SPEC, 1,6,me_related_a_f);
	me_related_add(ME_RELATED_FUNC_SPEC, 1,7,me_related_a_g);
	me_related_add(ME_RELATED_FUNC_SPEC, 2,1,me_related_b_a);
	me_related_add(ME_RELATED_FUNC_SPEC, 2,3,me_related_b_c);
	me_related_add(ME_RELATED_FUNC_SPEC, 2,4,me_related_b_d);
	//test function duplicate
	me_related_add(ME_RELATED_FUNC_HW, 2,4,me_related_b_d);
	//me_related_add(ME_RELATED_FUNC_SPEC, 2,4,me_related_b_d);

	me_related_dump();
#if 1
	struct me_related_entity_t *me_related_entity;
	struct me_t *me1=0, *me2=0;
	unsigned char mode=ME_RELATED_FUNC_SPEC;

	me_related_entity=me_related_find(1, 6);
	if (me_related_entity){
		if(mode == ME_RELATED_FUNC_SPEC)
			me_related_entity->spec_related_func(me1, me2);//1: related, 0: not related
		else if(mode == ME_RELATED_FUNC_HW)
			me_related_entity->hw_related_func(me1, me2);//1: related, 0: not related
		else
			printf("mode not found\n");
	} else
		printf("func not found\n");
#endif

	me_related_del(1,5);
	me_related_del(1,6);
	me_related_del(1,7);
	me_related_del(2,1);
	me_related_del(2,3);
	//test function delete
	//me_related_del(2,4);
	me_related_dump();

	return 0;
}


