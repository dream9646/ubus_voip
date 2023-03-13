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
 * Module  : meinfo
 * File    : meinfo_instance.c
 *
 ******************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>
                                          
#include "meinfo.h"
#include "notify_chain.h"
#include "list.h"
#include "util.h"

short
meinfo_instance_alloc_free_num(unsigned short classid)
{
	struct meinfo_t *miptr=meinfo_util_miptr(classid);
	int i;

	if (miptr==NULL) {
		dbprintf(LOG_ERR, "%s, classid=%u\n", meinfo_util_strerror(MEINFO_ERROR_CLASSID_UNDEF), classid);
		return MEINFO_ERROR_CLASSID_UNDEF;	//class_id out of range
	}

	for (i = 0; i < miptr->config.instance_total_max; i++) {
		struct me_t *me;
		int found = 0;
		list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
			if (me->instance_num == i) {
				found = 1;
				break;
			}
		}
		if (!found)
			return i;
	}

	return MEINFO_ERROR_ME_INSTANCE_FULL;
}

struct meinfo_instance_def_t *
meinfo_instance_find_def(unsigned short classid, unsigned short instance_num)
{
	struct meinfo_t *miptr=meinfo_util_miptr(classid);
	struct meinfo_instance_def_t *instance_def_ptr;
	struct meinfo_instance_def_t *default_ptr=NULL;
	struct meinfo_instance_def_t *zero_ptr=NULL;

	if (miptr==NULL) {
		dbprintf(LOG_ERR, "%s, classid=%u, instance_num=%d\n",
			 meinfo_util_strerror(MEINFO_ERROR_CLASSID_UNDEF), classid, instance_num);
		return NULL;	//class_id out of range
	}
	// search instance def 
	if (miptr->config.is_inited) {
		list_for_each_entry(instance_def_ptr, &miptr->config.meinfo_instance_def_list, instance_node) {
			if (instance_def_ptr->instance_num == instance_num) {
				return instance_def_ptr;
			} else if (instance_def_ptr->instance_num == MEINFO_INSTANCE_DEFAULT) {
				default_ptr = instance_def_ptr;
			} else if (instance_def_ptr->instance_num == 0) {
				zero_ptr = instance_def_ptr;
			}
		}
		if (default_ptr)
			return default_ptr;
		if (zero_ptr)	// if default instance definition is not found, use config form instance 0 instead
			return zero_ptr;
	}
	return NULL;
}

int
meinfo_instance_meid_to_num(unsigned short classid, unsigned short meid)
{
	struct meinfo_t *miptr=meinfo_util_miptr(classid);
	struct me_t *me;

	if (miptr==NULL) {
		dbprintf(LOG_ERR, "%s, classid=%u, meid=%u\n",
			meinfo_util_strerror(MEINFO_ERROR_CLASSID_UNDEF), classid, meid);
		return MEINFO_ERROR_CLASSID_UNDEF;	//class_id out of range
	}

	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		if (me->meid == meid)
			return (me->instance_num);
	}

	dbprintf(LOG_ERR, "%s, classid=%u, meid=%u\n",
		meinfo_util_strerror(MEINFO_ERROR_ME_NOT_FOUND), classid, meid);

	return MEINFO_ERROR_ME_NOT_FOUND;
}

// since this function is used by cli 'inst2meid' and cli has its own error msg, 
// so we eliminate error msg to LOG_NOTICE instead of LOG_ERR
int
meinfo_instance_num_to_meid(unsigned short classid, unsigned short instance_num)
{
	struct meinfo_t *miptr=meinfo_util_miptr(classid);
	struct me_t *me;

	if (miptr==NULL) {
		dbprintf(LOG_NOTICE, "%s, classid=%u, instance_num=%d\n",
			meinfo_util_strerror(MEINFO_ERROR_CLASSID_UNDEF), classid, instance_num);
		return MEINFO_ERROR_CLASSID_UNDEF;	//class_id out of range
	}
	
	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		if (me->instance_num == instance_num) {
			return (me->meid);
		}
	}

	dbprintf(LOG_NOTICE, "%s, classid=%u, instance_num=%d\n",
		 meinfo_util_strerror(MEINFO_ERROR_ME_NOT_FOUND), classid, instance_num);

	return MEINFO_ERROR_ME_NOT_FOUND;
}
