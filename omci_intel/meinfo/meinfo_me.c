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
 * File    : meinfo_me.c
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
#include "util_run.h"
#include "me_related.h"
#include "er_group.h"
#include "hwresource.h"
#include "omcimsg.h"

static inline int
meinfo_me_nonready_to_ready(struct me_t *me, unsigned char attr_mask[2])
{
	int ret=0;

	//set ready flag
	me->is_ready = 1;
	//allocate hw resource
	if (hwresource_is_related(me->classid) && hwresource_public2private(me)==NULL)
		hwresource_alloc(me);
	//call er_attr_group_me_add
	if (er_attr_group_me_add(me, attr_mask) < 0)
	{
		dbprintf(LOG_INFO,
			"call er_attr_group_me_add error, classid=%u, instance_num=%d, meid=%u\n",
	 		me->classid, me->instance_num, me->meid);
		ret=-1;
	}
	return ret;
}

static inline int
meinfo_me_ready_to_nonready(struct me_t *me)
{
	int ret=0;

	//set ready flag
	me->is_ready = 0;
	//call er_attr_group_me_delete
	if (er_attr_group_me_delete(me) < 0)
	{
		dbprintf(LOG_ERR,
			 "call er_attr_group_me_delete error, classid=%u, instance_num=%d, meid=%u\n",
	 		me->classid, me->instance_num, me->meid);
		ret=-1;
	}
	//free hw resource
	if (hwresource_is_related(me->classid))
		hwresource_free(me);
	return ret;
}

static inline int
meinfo_me_ready_to_ready(struct me_t *me, unsigned char attr_mask[2])
{
	int ret=0;

	//allocate hw resource
	if (hwresource_is_related(me->classid) && hwresource_public2private(me)==NULL)
		hwresource_alloc(me);
	//call er_attr_group_me_update
	if (er_attr_group_me_update(me, attr_mask) < 0)
	{
		dbprintf(LOG_ERR,
			"call er_attr_group_me_update error, classid=%u, instance_num=%d, meid=%u\n",
			me->classid, me->instance_num, me->meid);
		ret=-1;
	}
	return ret;
}

int
meinfo_me_check_ready_state(struct me_t *me, unsigned char attr_mask[2])
{
	int ready;

	if (me == NULL)
		return -1;

	ready = meinfo_me_is_ready(me);
	if (me->is_ready == 0 && ready == 1) {
		meinfo_me_nonready_to_ready(me, attr_mask);
	} else if (me->is_ready == 1 && ready == 0) {
		meinfo_me_ready_to_nonready(me);
	} else if (me->is_ready == 1 && ready == 1) {
		//do nothing
	} else if (me->is_ready == 0 && ready == 0) {
		//do nothing
	}
	return 0;
}

int
meinfo_me_issue_self_er_attr_group(struct me_t *me, int action, unsigned char attr_mask[2])
{
	int ready;

	if (me == NULL)
		return -1;

	ready = meinfo_me_is_ready(me);
	switch(action) {
	case ER_ATTR_GROUP_HW_OP_ADD:
		if (me->is_ready == 0 && ready == 1) {
			meinfo_me_nonready_to_ready(me, attr_mask);
		} else if (me->is_ready == 1 && ready == 0) {
			dbprintf(LOG_ERR,	"unreasonable issue er_attr_group, classid=%u, meid=%u, old_ready=%u, current_ready=%u, action=%u\n",
				me->classid, me->meid, me->is_ready, ready, action);
		} else if (me->is_ready == 1 && ready == 1) {
			dbprintf(LOG_ERR,	"unreasonable issue er_attr_group, classid=%u, meid=%u, old_ready=%u, current_ready=%u, action=%u\n",
				me->classid, me->meid, me->is_ready, ready, action);
		} else if (me->is_ready == 0 && ready == 0) {
			//do nothing
		}
		break;
	case ER_ATTR_GROUP_HW_OP_UPDATE:
		if (me->is_ready == 0 && ready == 1) {
			meinfo_me_nonready_to_ready(me, attr_mask);
		} else if (me->is_ready == 1 && ready == 0) {
			meinfo_me_ready_to_nonready(me);
		} else if (me->is_ready == 1 && ready == 1) {
			meinfo_me_ready_to_ready(me, attr_mask);
		} else if (me->is_ready == 0 && ready == 0) {
			//do nothing
		}
		break;
	case ER_ATTR_GROUP_HW_OP_DEL:
		meinfo_me_ready_to_nonready(me);
		break;
	default:
		dbprintf(LOG_ERR, "unreasonable issue er_attr_group, classid=%u, meid=%u, old_ready=%u, current_ready=%u, action=%u\n",
			me->classid, me->meid, me->is_ready, ready, action);
	}

	return 0;
}

int
meinfo_me_issue_related_er_attr_group(unsigned short classid)
{
	struct meinfo_t *miptr=meinfo_util_miptr(classid);
	struct meinfo_t *related_miptr;
	struct meinfo_related_ready_class_t *related_ready_class_pos;
	struct me_t *me_pos;
	unsigned char tmp_attr_mask[2] = {0xff, 0xff};

	if (miptr==NULL) {
		dbprintf(LOG_ERR, "%s, classid=%u\n", meinfo_util_strerror(MEINFO_ERROR_CLASSID_UNDEF), classid);
		return -1;	//class_id out of range
	}

	// check in mib_reset
	if (omcimsg_mib_reset_clear_is_in_progress() == 1) 
		return 0;

	list_for_each_entry(related_ready_class_pos, &miptr->related_ready_class_list, related_ready_class_node) {
		//get miptr of the classid
		if ((related_miptr = meinfo_util_miptr(related_ready_class_pos->classid)) != NULL) {
			//check all instance of this class
			list_for_each_entry(me_pos, &related_miptr->me_instance_list, instance_node)
				meinfo_me_check_ready_state(me_pos, tmp_attr_mask);
		}
	}

	return 0;
}

struct me_t *
meinfo_me_alloc(unsigned short classid, unsigned short instance_num)
{
	int i;
	struct meinfo_t *miptr=meinfo_util_miptr(classid);
	struct me_t *me;
	struct attr_table_header_t *table_header_ptr;


	if (miptr==NULL) {
		dbprintf(LOG_ERR, "%s, classid=%u, instance_num=%u\n",
			 meinfo_util_strerror(MEINFO_ERROR_CLASSID_UNDEF), classid, instance_num);
		return NULL;	//class_id out of range
	} else {
		dbprintf(LOG_INFO, "classid=%u, instance_num=%u\n", classid, instance_num);
	}

	if (miptr->config.support == ME_SUPPORT_TYPE_NOT_SUPPORTED) {
		dbprintf(LOG_ERR, "%s, classid=%u, instance_num=%d\n",
			 meinfo_util_strerror(MEINFO_ERROR_CLASSID_NOT_SUPPORT), classid, instance_num);
		return (NULL);
	}

	//allocate me instance memory
	me = malloc_safe(sizeof (struct me_t));

	//init me instance
	INIT_LIST_NODE(&me->instance_node);
	INIT_LIST_NODE(&me->mibupload_clone_node);
	INIT_LIST_NODE(&me->alarm_clone_node);

	// alloc the space for me
	for (i = 1; i <= miptr->attr_total; i++) {
		switch (meinfo_util_attr_get_format(classid, i)) {
		case ATTR_FORMAT_BIT_FIELD:
		case ATTR_FORMAT_STRING:	// alloc space for bitfield, string
			me->attr[i].value.ptr = malloc_safe(miptr->attrinfo[i].byte_size);
			if (miptr->attrinfo[i].spec_default_value.ptr)	// copy default for bitfields/string
				memcpy(me->attr[i].value.ptr, miptr->attrinfo[i].spec_default_value.ptr, miptr->attrinfo[i].byte_size);
			break;

		case ATTR_FORMAT_TABLE:		// alloc table header, init list of table entry
			table_header_ptr=(struct attr_table_header_t *)malloc_safe(sizeof (struct attr_table_header_t));
			table_header_ptr->entry_count = 0;
			INIT_LIST_HEAD(&table_header_ptr->entry_list);
			me->attr[i].value.ptr = table_header_ptr;
			break;
		case ATTR_FORMAT_POINTER:
		case ATTR_FORMAT_UNSIGNED_INT:
		case ATTR_FORMAT_ENUMERATION:
		case ATTR_FORMAT_SIGNED_INT:
			me->attr[i].value.data = miptr->attrinfo[i].spec_default_value.data; // copy default for simple data type
			break;
		default:
			;	//do nothing for else format
		}
	}

	me->classid = classid;
	me->instance_num = instance_num;
	me->refcount = 0;
	me->is_private = miptr->is_private;

	// get meid from meid_gen callback
	// the meid may be override by meid defined in config xml in meinfo_me_load_config()
	if (miptr->callback.meid_gen_cb != NULL) {
		me->meid = miptr->callback.meid_gen_cb(instance_num);
	} else {
		me->meid = instance_num;
	}

	return me;
}

int
meinfo_me_load_config(struct me_t *me)
{
	struct meinfo_t *miptr;
	struct meinfo_instance_def_t *instance_def_ptr;
	int i;

	if (me == NULL) {
		dbprintf(LOG_ERR, "%s, classid=%u\n", meinfo_util_strerror(MEINFO_ERROR_ME_PTR_NULL), me->classid);
		return (MEINFO_ERROR_ME_PTR_NULL);
	}

	miptr=meinfo_util_miptr(me->classid);
	if (miptr==NULL) {
		dbprintf(LOG_ERR, "%s, classid=%u\n", meinfo_util_strerror(MEINFO_ERROR_CLASSID_UNDEF), me->classid);
		return MEINFO_ERROR_CLASSID_UNDEF;	//class_id out of range
	}

	if (miptr->config.support == ME_SUPPORT_TYPE_NOT_SUPPORTED) {
		dbprintf(LOG_ERR, "%s, classid=%u, instance_num=%d\n",
			 meinfo_util_strerror(MEINFO_ERROR_CLASSID_NOT_SUPPORT), me->classid, me->instance_num);
		return (MEINFO_ERROR_CLASSID_NOT_SUPPORT);
	}

	// load value from meinfo->config
	instance_def_ptr = meinfo_instance_find_def(me->classid, me->instance_num);
	if (instance_def_ptr) {
		// OVERRIDE meid with value specified in config xml
		if (instance_def_ptr->default_meid!=0xffff)
			me->meid = instance_def_ptr->default_meid;

		// copy pm_is_accumulated_mask for easier future access
		memcpy(me->pm_is_accumulated_mask, instance_def_ptr->pm_is_accumulated_mask, 2);

		// set me to private if either class is private or instance is private
		if (instance_def_ptr->is_private)
			me->is_private = instance_def_ptr->is_private;

		for (i = 1; i <= miptr->attr_total; i++) {
			//regular value, not table
			switch (meinfo_util_attr_get_format(me->classid, i)) {
			case ATTR_FORMAT_POINTER:
			case ATTR_FORMAT_UNSIGNED_INT:
			case ATTR_FORMAT_ENUMERATION:
			case ATTR_FORMAT_SIGNED_INT:
				me->attr[i].value.data = instance_def_ptr->custom_default_value[i].data;
				break;
			case ATTR_FORMAT_BIT_FIELD:	//not sure
			case ATTR_FORMAT_STRING:
				if (instance_def_ptr->custom_default_value[i].ptr != NULL) {
					memcpy(me->attr[i].value.ptr,
					       instance_def_ptr->custom_default_value[i].ptr,
					       miptr->attrinfo[i].byte_size);
				}
				break;
			case ATTR_FORMAT_TABLE:
				if (instance_def_ptr->custom_default_value[i].ptr != NULL) {
					struct attr_table_header_t *header1=instance_def_ptr->custom_default_value[i].ptr;
					struct attr_table_entry_t *entry1; //, *entry2;
					//copy all entries
					list_for_each_entry(entry1, &header1->entry_list, entry_node) {
						meinfo_me_attr_set_table_entry(me, i, entry1->entry_data_ptr, 0);
					}
				}
				break;
			default:
				;	//do nothing
			}
		}
	}
	return 0;
}

// 0: the same as config
int
meinfo_me_compare_config(struct me_t *me)
{
	struct meinfo_t *miptr;
	struct meinfo_instance_def_t *instance_def_ptr;
	int i;

	if (me == NULL) {
		dbprintf(LOG_ERR, "%s, classid=%u\n", meinfo_util_strerror(MEINFO_ERROR_ME_PTR_NULL), me->classid);
		return (MEINFO_ERROR_ME_PTR_NULL);
	}

	miptr=meinfo_util_miptr(me->classid);
	if (miptr==NULL) {
		dbprintf(LOG_ERR, "%s, classid=%u\n", meinfo_util_strerror(MEINFO_ERROR_CLASSID_UNDEF), me->classid);
		return MEINFO_ERROR_CLASSID_UNDEF;	//class_id out of range
	}

	if (miptr->config.support == ME_SUPPORT_TYPE_NOT_SUPPORTED) {
		dbprintf(LOG_ERR, "%s, classid=%u, instance_num=%d\n",
			 meinfo_util_strerror(MEINFO_ERROR_CLASSID_NOT_SUPPORT), me->classid, me->instance_num);
		return (MEINFO_ERROR_CLASSID_NOT_SUPPORT);
	}

	// load value from meinfo->config
	instance_def_ptr = meinfo_instance_find_def(me->classid, me->instance_num);
	if (instance_def_ptr) {
		// overide meid with value specified in config xml
		if (instance_def_ptr->default_meid!=0xffff)
			me->meid = instance_def_ptr->default_meid;

		for (i = 1; i <= miptr->attr_total; i++) {
			//regular value, not table
			switch (meinfo_util_attr_get_format(me->classid, i)) {
			case ATTR_FORMAT_POINTER:
			case ATTR_FORMAT_UNSIGNED_INT:
			case ATTR_FORMAT_ENUMERATION:
			case ATTR_FORMAT_SIGNED_INT:
				if (me->attr[i].value.data != instance_def_ptr->custom_default_value[i].data)
					return 1;
				break;
			case ATTR_FORMAT_BIT_FIELD:	//not sure
			case ATTR_FORMAT_STRING:
				if (instance_def_ptr->custom_default_value[i].ptr != NULL) {
					if (strncmp(me->attr[i].value.ptr,
					       instance_def_ptr->custom_default_value[i].ptr,
					       miptr->attrinfo[i].byte_size)!=0)
						return 1;
				} else {
					if (me->attr[i].value.ptr && strlen(me->attr[i].value.ptr)>0)
						return 1;
				}
				break;
			case ATTR_FORMAT_TABLE:
				if (instance_def_ptr->custom_default_value[i].ptr != NULL) {
					struct attr_table_header_t *header1=instance_def_ptr->custom_default_value[i].ptr;
					int entry_in_mib=meinfo_me_attr_get_table_entry_count(me, i);
					// a very rough comparison, just check if the total entry count in mib are equal to the one in config
					if (header1->entry_count!=entry_in_mib)
						return 1;
				} else {
					int entry_in_mib=meinfo_me_attr_get_table_entry_count(me, i);
					if (entry_in_mib>0)
						return 1;
				}

				break;
			default:
				;	//do nothing
			}
		}
	}
	return 0;
}

// put me into mib
int
meinfo_me_create(struct me_t *me)
{
	int ready;
	struct meinfo_t *miptr;
	struct me_t *me_hw;

	dbprintf(LOG_NOTICE, "classid=%u, instance_num=%d\n", me->classid, me->instance_num);

	if (me == NULL) {
		dbprintf(LOG_ERR, "%s, classid=%u\n", meinfo_util_strerror(MEINFO_ERROR_ME_PTR_NULL), me->classid);
		return (MEINFO_ERROR_ME_PTR_NULL);
	}

	miptr=meinfo_util_miptr(me->classid);
	if (miptr==NULL) {
		dbprintf(LOG_ERR, "%s, classid=%u\n", meinfo_util_strerror(MEINFO_ERROR_CLASSID_UNDEF), me->classid);
		return MEINFO_ERROR_CLASSID_UNDEF;	//class_id out of range
	}

	if (miptr->config.support == ME_SUPPORT_TYPE_NOT_SUPPORTED) {
		dbprintf(LOG_ERR, "%s, classid=%u, instance_num=%d\n",
			 meinfo_util_strerror(MEINFO_ERROR_CLASSID_NOT_SUPPORT), me->classid, me->instance_num);
		return (MEINFO_ERROR_CLASSID_NOT_SUPPORT);
	}

	//check ready and bind hw resource, if not success, return fail
	if ((ready = meinfo_me_is_ready(me)) == 1 &&
		hwresource_is_related(me->classid) && 
		hwresource_public2private(me)==NULL)
	{
		if ((me_hw = hwresource_alloc(me)) == NULL)
		{
			dbprintf(LOG_ERR,
				 "hw alloc error, classid=%u, instance_num=%d, meid=%u\n",
				me->classid, me->instance_num, me->meid);
			return MEINFO_ERROR_HW_ME_CREATE;
		}
	}

	//set the refcount
	me->refcount = 1;
	//add this me to instance list
	list_add_tail(&me->instance_node, &miptr->me_instance_list);

	//cb
	if (miptr->callback.create_cb) {
		unsigned char attr_mask[2]={0,0};	// mask of set by create attr
		int i;

		for (i = 1; i <= miptr->attr_total; i++) {
			if (meinfo_util_attr_is_sbc(me->classid, i))
				util_attr_mask_set_bit(attr_mask, i);
		}
		if (miptr->callback.create_cb(me, attr_mask) < 0) {
			dbprintf(LOG_ERR, "cb create error, classid=%u, instance_num=%d, meid=%u\n",
				me->classid, me->instance_num, me->meid);

			//free hw resource
			hwresource_free(me);

			//remove from instance list
			list_del(&me->instance_node);

			return MEINFO_ERROR_CB_ME_CREATE;
		}
	}

	// hw related init
	if (omci_env_g.mib_rw_mode == ENV_MIB_RW_MODE_HWHW || omci_env_g.mib_rw_mode == ENV_MIB_RW_MODE_SWHW)
	{
		unsigned char attr_mask[2]={0,0};
		// put attr_mask[] to avoid null ptr access, er group hw func should not refer to it anyway
		if (meinfo_me_issue_self_er_attr_group(me, ER_ATTR_GROUP_HW_OP_ADD, attr_mask) < 0)
		{
			dbprintf(LOG_ERR,
				 "issue self er_attr_group error, classid=%u, instance_num=%d, meid=%u\n",
				me->classid, me->instance_num, me->meid);
		}

		//check me ready state related with this class
		if (meinfo_me_issue_related_er_attr_group(me->classid) < 0)
		{
			dbprintf(LOG_ERR,
				 "issue related er_attr_group error, classid=%u, instance_num=%d, meid=%u\n",
				 me->classid, me->instance_num, me->meid);
		}
	}

	// call notifychain issue() for event create
	if (notify_chain_notify(NOTIFY_CHAIN_EVENT_ME_CREATE, me->classid, me->instance_num, me->meid) < 0) {
		dbprintf(LOG_ERR,
			 "notify error, classid=%u, instance_num=%d, meid=%u\n",
			 me->classid, me->instance_num, me->meid);
	}

	dbprintf(LOG_NOTICE,
		 "classid=%u, instance_num=%d, meid=%u\n",
		 me->classid, me->instance_num, me->meid);

	return 0;
}

int
meinfo_me_release(struct me_t *me)
{
	struct meinfo_t *miptr;
	int i;

	if (me == NULL) {
		dbprintf(LOG_ERR, "%s, classid=%u\n", meinfo_util_strerror(MEINFO_ERROR_ME_PTR_NULL), me->classid);
		return (MEINFO_ERROR_ME_PTR_NULL);
	}

	miptr=meinfo_util_miptr(me->classid);
	if (miptr==NULL) {
		dbprintf(LOG_ERR, "%s, classid=%u, me=%p\n", meinfo_util_strerror(MEINFO_ERROR_CLASSID_UNDEF), me->classid, me);
		return MEINFO_ERROR_CLASSID_UNDEF;	//class_id out of range
	}

	// this should not happen
	if (me->refcount>0) {
		return MEINFO_ERROR_ME_STILL_IN_USE;
	}

	for (i = 1; i <= miptr->attr_total; i++) {
		//clear value
		meinfo_me_attr_release(me->classid, i, &me->attr[i].value);
		//clear value_current_data
		meinfo_me_attr_release(me->classid, i, &me->attr[i].value_current_data);
	}

	free_safe(me);

	return 0;
}

// delete me
static int
meinfo_me_delete_me_t(struct me_t *me)
{
	struct meinfo_t *miptr;

	if (me == NULL) {
		dbprintf(LOG_ERR, "%s\n", meinfo_util_strerror(MEINFO_ERROR_ME_PTR_NULL));
		return (MEINFO_ERROR_ME_PTR_NULL);
	}

	miptr=meinfo_util_miptr(me->classid);
	if (miptr==NULL) {
		dbprintf(LOG_ERR, "%s, classid=%u\n", meinfo_util_strerror(MEINFO_ERROR_CLASSID_UNDEF), me->classid);
		return MEINFO_ERROR_CLASSID_UNDEF;	//class_id out of range
	}

	//detele this instance
	if (me->refcount > 1)
		dbprintf(LOG_WARNING, "%s, classid=%u, instance_num=%d, meid=%u, me=%p, ref>1?\n",
			 meinfo_util_strerror(MEINFO_ERROR_ME_STILL_IN_USE), me->classid, me->instance_num, me->meid, me);

	if (!list_node_is_poison_or_null(&me->alarm_clone_node))
		dbprintf(LOG_WARNING, "%s, classid=%u, instance_num=%d, meid=%u, me=%p, alarm_clone?\n",
			 meinfo_util_strerror(MEINFO_ERROR_ME_STILL_IN_USE), me->classid, me->instance_num, me->meid, me);

	if (!list_node_is_poison_or_null(&me->mibupload_clone_node)) 
		dbprintf(LOG_WARNING, "%s, classid=%u, instance_num=%d, meid=%u, me=%p, mibupload_clone?\n",
			 meinfo_util_strerror(MEINFO_ERROR_ME_STILL_IN_USE), me->classid, me->instance_num, me->meid, me);

	// hw related release
	if (omci_env_g.mib_rw_mode == ENV_MIB_RW_MODE_HWHW || omci_env_g.mib_rw_mode == ENV_MIB_RW_MODE_SWHW ) 
	{
		unsigned char attr_mask[2]={0,0};
		// put attr_mask[] to avoid null ptr access, er group hw func should not refer to it anyway
		if (meinfo_me_issue_self_er_attr_group(me, ER_ATTR_GROUP_HW_OP_DEL, attr_mask) < 0)
		{
			dbprintf(LOG_ERR,
			 "issue self er_attr_group error, classid=%u, instance_num=%d, meid=%u\n",
			me->classid, me->instance_num, me->meid);
		}
	}

	if (miptr->callback.delete_cb && miptr->callback.delete_cb(me) < 0)
	{
		dbprintf(LOG_ERR,
			"hw init error, classid=%u, instance_num=%d, meid=%u, create_by=%d\n",
			me->classid, me->instance_num, me->meid);
	}

	// call notifychain issue() for event delete
	if (notify_chain_notify(NOTIFY_CHAIN_EVENT_ME_DELETE, me->classid, me->instance_num, me->meid) < 0) {
		dbprintf(LOG_ERR,
			 "notify error, classid=%u, instance_num=%d, meid=%u\n",
			 me->classid, me->instance_num, me->meid);
	}

	if( me->refcount > 0)
		me->refcount--; 
#if 0
	//list_del(&me->instance_node);
#endif
	if (me->refcount > 0)
		meinfo_util_gclist_collect_me(me, NULL);
	else
		meinfo_util_gclist_collect_me(me, meinfo_me_release);

	if (omci_env_g.mib_rw_mode == ENV_MIB_RW_MODE_HWHW || omci_env_g.mib_rw_mode == ENV_MIB_RW_MODE_SWHW ) 
	{
		//check me ready state related with this class
		if (meinfo_me_issue_related_er_attr_group(me->classid) < 0)
		{
			dbprintf(LOG_ERR,
			 "issue related er_attr_group error, classid=%u, instance_num=%d, meid=%u\n",
			me->classid, me->instance_num, me->meid);
		}
	}

#if 0
	if (me->refcount > 0)
		return 0;
	return meinfo_me_release(me);
#endif
	return 0;
}

// delete me by classid
int
meinfo_me_delete_by_classid(unsigned short classid)
{
	struct meinfo_t *miptr=meinfo_util_miptr(classid);
	struct me_t *me, *me_n;

	if (miptr==NULL) {
		dbprintf(LOG_ERR, "%s, classid=%u\n", meinfo_util_strerror(MEINFO_ERROR_CLASSID_UNDEF), classid);
		return MEINFO_ERROR_CLASSID_UNDEF;	//class_id out of range
	}

	list_for_each_entry_safe(me, me_n, &miptr->me_instance_list, instance_node) {
		meinfo_me_delete_me_t(me);
	}

	return 0;
}

// delete me by classid and me is_cbo (creat_by_olt)
int
meinfo_me_delete_by_classid_and_cbo(unsigned short classid, unsigned char is_cbo)
{
	struct meinfo_t *miptr=meinfo_util_miptr(classid);
	struct me_t *me, *me_n;

	if (miptr==NULL) {
		dbprintf(LOG_ERR, "%s, classid=%u\n", meinfo_util_strerror(MEINFO_ERROR_CLASSID_UNDEF), classid);
		return MEINFO_ERROR_CLASSID_UNDEF;	//class_id out of range
	}

	list_for_each_entry_safe(me, me_n, &miptr->me_instance_list, instance_node) {
		if (me->is_cbo == is_cbo)
		{
			meinfo_me_delete_me_t(me);
		}
	}

	return 0;
}


// delete me by instance number
int
meinfo_me_delete_by_instance_num(unsigned short classid, unsigned short instance_num)
{
	struct meinfo_t *miptr=meinfo_util_miptr(classid);
	struct me_t *me_pos_ptr, *me_n_ptr;

	if (miptr==NULL) {
		dbprintf(LOG_ERR, "%s, classid=%u, instance_num=%d\n",
			 meinfo_util_strerror(MEINFO_ERROR_CLASSID_UNDEF), classid, instance_num);
		return MEINFO_ERROR_CLASSID_UNDEF;	//class_id out of range
	}

	list_for_each_entry_safe(me_pos_ptr, me_n_ptr, &miptr->me_instance_list, instance_node) {
		//make sure which instance to delete
		if (me_pos_ptr->instance_num == instance_num) {
			meinfo_me_delete_me_t(me_pos_ptr);
			return 0;
		}
	}

	return MEINFO_ERROR_ME_NOT_FOUND;
}

struct me_t *
meinfo_me_copy(struct me_t *me)
{
	struct meinfo_t *miptr;
	struct me_t *me2;
	int i;

	miptr=meinfo_util_miptr(me->classid);
	if (miptr==NULL) {
		dbprintf(LOG_ERR, "%s, classid=%u\n", meinfo_util_strerror(MEINFO_ERROR_CLASSID_UNDEF), me->classid);
		return NULL;	//class_id out of range
	}

	me2 = meinfo_me_alloc(me->classid, me->instance_num);
	if (me2 == NULL)
		return NULL;

	me2->classid=me->classid;
	me2->instance_num=me->instance_num;
	me2->meid=me->meid;

	me2->is_private=me->is_private;
	me2->is_cbo=me->is_cbo;
	me2->is_ready=me->is_ready;

	memcpy(me2->alarm_result_mask, me->alarm_result_mask, 28);
	memcpy(me2->alarm_report_mask, me->alarm_report_mask, 28);
	memcpy(me2->avc_changed_mask, me->avc_changed_mask, 2);

	// copy attr
	for (i = 1; i <= miptr->attr_total; i++) {
		int format = meinfo_util_attr_get_format(me->classid, i);
		if (format == ATTR_FORMAT_TABLE) {
			if (meinfo_me_attr_copy_value(me->classid, i, &me2->attr[i].value, &me->attr[i].value)<0) {
				dbprintf(LOG_ERR,
					 "me table attr copy failed, classid=%u, meid=%u, instance_num=%d, attr=%d\n",
					 me->classid, me->meid, me->instance_num, i);
				meinfo_me_release(me2);
				return NULL;
			}
		} else {
			if (meinfo_me_attr_copy_value(me->classid, i, &me2->attr[i].value, &me->attr[i].value)<0 ||
			    meinfo_me_attr_copy_value(me->classid, i, &me2->attr[i].value_current_data, &me->attr[i].value_current_data)<0 ||
			    meinfo_me_attr_copy_value(me->classid, i, &me2->attr[i].value_hwprev, &me->attr[i].value_hwprev)<0) {
				dbprintf(LOG_ERR,
					 "me attr copy failed, classid=%u, meid=%u, instance_num=%d, attr=%d\n",
					 me->classid, me->meid, me->instance_num, i);
				meinfo_me_release(me2);
				return NULL;
			}
		}
	}

	return me2;
}

#if defined(USE_MEINFO_ME_COPY_ON_WRITE)
int
meinfo_me_copy_on_write(struct me_t *me, struct me_t **me_clone)
{
	struct meinfo_t *miptr;
	struct me_t *me2;

	if (me == NULL) {
		dbprintf(LOG_ERR, "%s\n", meinfo_util_strerror(MEINFO_ERROR_ME_PTR_NULL));
		return (MEINFO_ERROR_ME_PTR_NULL);
	}

	miptr=meinfo_util_miptr(me->classid);
	if (miptr==NULL) {
		dbprintf(LOG_ERR, "%s, classid=%u\n", meinfo_util_strerror(MEINFO_ERROR_CLASSID_UNDEF), me->classid);
		return MEINFO_ERROR_CLASSID_UNDEF;	//class_id out of range
	}

	if (me->refcount <= 1) {
		dbprintf(LOG_ERR, "COW error refcount<=1, classid=%u, meid=%u, instance_num=%d\n",
			 me->classid, me->meid, me->instance_num);
		return MEINFO_ERROR_ME_COPY_ON_WRITE;	// no need to clone
	}

	// alloc me & copy content
	me2 = meinfo_me_copy(me);
	if (me2==NULL) {
		dbprintf(LOG_ERR, "%s\n", meinfo_util_strerror(MEINFO_ERROR_ME_PTR_NULL));
		return (MEINFO_ERROR_ME_PTR_NULL);
	}

	// orig me, remove from instance list
	me->refcount--;
#if 0
	list_del(&me->instance_node);	// remove orig from instance list
#else
	meinfo_util_gclist_collect_me(me, NULL);
#endif

	// add clone me to instance list
	list_add_tail(&me2->instance_node, &miptr->me_instance_list);
	me2->refcount = 1;

	*me_clone = me2;
	return 0;
}
#endif

int omci_is_ready(void );	// prototyping from omcimain.h
struct me_t *
meinfo_me_get_by_meid(unsigned short classid, unsigned short meid)
{
	static unsigned int me256_exist = 0;
	static unsigned int me256_missing = 0;

	struct meinfo_t *miptr=meinfo_util_miptr(classid);
	struct me_t *me;

	if (miptr==NULL) {
		dbprintf(LOG_ERR, "%s, classid=%u, meid=%u\n",
			 meinfo_util_strerror(MEINFO_ERROR_CLASSID_UNDEF), classid, meid);
		return (NULL);	//class_id out of range
	}

	if (miptr->config.support == ME_SUPPORT_TYPE_NOT_SUPPORTED) {
		dbprintf(LOG_ERR, "%s, classid=%u, meid=%u\n",
			 meinfo_util_strerror(MEINFO_ERROR_CLASSID_NOT_SUPPORT), classid, meid);
		return (NULL);
	}

	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		if (me->meid == meid) {
			if (classid == 256) {
				me256_exist = 1;
				me256_missing = 0;
			}
			return me;
		}
	}

	if (omci_env_g.mib_aliasid_enable) {
		int meid2=meinfo_util_aliasid2meid(classid, meid);
		if (meid2 == 0xffff) {
			// this is a virtual classid, which means all attr of this classid are derived from runtime
			// classid of this type will have only one instance in mib,
			// no matter what meid is requested, the same instance is returned with meid modified
			list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
				me->meid = meid; // meid modified for this lookup
				return me;
			}
		} else if (meid2>=0) {	// real meid
			list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
				if (me->meid == meid2) {
					return me;
				}
			}
		}
	}
#if 0	// meid existance is a very basic condition checked by many function, so error msg should not be printed when existance is false
	dbprintf(LOG_ERR, "%s, classid=%u, meid=%u\n",
		 meinfo_util_strerror(MEINFO_ERROR_ME_NOT_FOUND), classid, meid);
#endif

	// UGLY HACK, to be fix
	if (omci_is_ready() && classid == 256 && me256_exist) {
		me256_missing++;
		if (me256_missing > 3) {
			dbprintf(LOG_ERR, "me256:0x%x missing %d, reboot...\n", meid, me256_missing);
			util_run_by_system("reboot");
		} else {
			dbprintf(LOG_ERR, "me256:0x%x missing %d\n", meid, me256_missing);
		}
	}

	return (NULL);
}

struct me_t *
meinfo_me_get_by_instance_num(unsigned short classid, unsigned short instance_num)
{
	static unsigned int me256_exist = 0;
	static unsigned int me256_missing = 0;

	struct meinfo_t *miptr=meinfo_util_miptr(classid);
	struct me_t *me = NULL;

	if (miptr==NULL) {
		dbprintf(LOG_ERR, "%s, classid=%u, instance_num=%d\n",
			  meinfo_util_strerror(MEINFO_ERROR_CLASSID_UNDEF), classid, instance_num);
		return (NULL);	//class_id out of range
	}

	if (miptr->config.support == ME_SUPPORT_TYPE_NOT_SUPPORTED) {
		dbprintf(LOG_ERR, "%s, classid=%u, instance_num=%d\n",
			  meinfo_util_strerror(MEINFO_ERROR_CLASSID_NOT_SUPPORT), classid, instance_num);
		return (NULL);
	}

	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		if (me->instance_num == instance_num) {
			if (classid == 256) {
				me256_exist = 1;
				me256_missing = 0;
			}
			return me;
		}
	}

	dbprintf(LOG_ERR, "%s, classid=%u, instance_num=%d\n",
		 meinfo_util_strerror(MEINFO_ERROR_ME_NOT_FOUND), classid, instance_num);

	// UGLY HACK, to be fix
	if (omci_is_ready() && classid == 256 && me256_exist) {
		me256_missing++;
		if (me256_missing > 3) {
			dbprintf(LOG_ERR, "me256:inst%d missing %d, reboot...\n", instance_num, me256_missing);
			util_run_by_system("reboot");
		} else {
			dbprintf(LOG_ERR, "me256:inst%d missing %d\n", instance_num, me256_missing);
		}
	}

	return (NULL);
}

int
meinfo_me_is_exist(unsigned short classid, unsigned short meid)
{
	return meinfo_me_get_by_meid(classid, meid)?1:0;
}

// check tca and set related alarm_result_mask
void
meinfo_me_determine_tca(unsigned short classid, unsigned short meid)
{
	int i;
	unsigned char thresh_data_index;
	struct meinfo_t *miptr=meinfo_util_miptr(classid);
	struct me_t *me, *me_td;

	if (miptr==NULL || (me = meinfo_me_get_by_meid(classid, meid)) == NULL ||
		meinfo_me_is_admin_locked(me))
	{
		return;
	}

	//threshhold data
        unsigned short threshold_meid = 0;
        if (classid == 425 || classid == 334)   // me 334/425 ueses 2.1     
                threshold_meid = htons(((unsigned short *) meinfo_util_me_attr_ptr(me, 2))[0]);
        else
                threshold_meid = me->attr[2].value.data;
                          
	if (threshold_meid != 0xFFFF)
	{
                
                // check global TCA bit
                unsigned short tca = 0;
                        
                if (classid == 425 || classid == 334)
                {
                        tca = htons(((unsigned short *) meinfo_util_me_attr_ptr(me, 2))[4]);
                        if (tca & 0x4000) return;       // global tca is turned off
                }                                   
       
                for (i = 3;i <= meinfo_util_get_attr_total(classid); i++)
		{
                        if (miptr->attrinfo[i].pm_tca_number > 0 && miptr->attrinfo[i].pm_tca_number < 15)
			{//need to check thresh value
				me_td = NULL;

                                if (classid == 425)
                                {
                                                if ((me_td = meinfo_me_get_by_meid(426, threshold_meid)) == NULL) {
                                                        dbprintf(LOG_INFO,
		 					"have no td3, classid=%u, meid=%u\n",
		 					classid, meid);
                                                }
                                }
                                else
                                {
                                        //determine for td1 or td2
                                        if (miptr->attrinfo[i].pm_tca_number <= MEINFO_ME_PM_TD_ATTR_NUM)
                                        {
                                                //td1
                                                if ((me_td = meinfo_me_get_by_meid(273, threshold_meid)) == NULL) {
                                                        dbprintf(LOG_INFO,
		 					"have no td1, classid=%u, meid=%u\n",
		 					classid, meid);
                                                }
                                        } else {
                                                //td2
                                                if ((me_td = meinfo_me_get_by_meid(274, threshold_meid)) == NULL) {
                                                        dbprintf(LOG_INFO,
		 					"have no td2, classid=%u, meid=%u\n",
		 					classid, meid);
                                                }
                                        }
                                }

				if (me_td)
				{                                         
                                        if (classid == 425 || classid == 334)
                                        {
                                                // test indivial attribute TCA
                                                if ((tca & 0x4000) && (tca & (1 << (i - 3)))) return;
                                        }
                                                                                
                                        thresh_data_index = (miptr->attrinfo[i].pm_tca_number <= MEINFO_ME_PM_TD_ATTR_NUM?
						miptr->attrinfo[i].pm_tca_number:
						miptr->attrinfo[i].pm_tca_number - MEINFO_ME_PM_TD_ATTR_NUM);
					if (me_td->attr[thresh_data_index].value.data && \
					me->attr[i].value_current_data.data >= me_td->attr[thresh_data_index].value.data)
					{
						util_alarm_mask_set_bit(me->alarm_result_mask, miptr->attrinfo[i].pm_alarm_number);
					} else {
						util_alarm_mask_clear_bit(me->alarm_result_mask, miptr->attrinfo[i].pm_alarm_number);
					}
				}
			}
		}
	}

	return;
}

int
meinfo_me_set_child_admin_lock(struct me_t *parent_me, unsigned short child_class, int lockval)
{
	struct me_t *me;
	struct meinfo_t *child_miptr;

	if (parent_me==NULL)
		return MEINFO_ERROR_ME_PTR_NULL;

	child_miptr=meinfo_util_miptr(child_class);
	if (child_miptr==NULL)
		return MEINFO_ERROR_CLASSID_UNDEF;

	if (child_miptr->callback.set_admin_lock_cb) {
		list_for_each_entry(me, &child_miptr->me_instance_list, instance_node) {
			if (me_related(me, parent_me))
				child_miptr->callback.set_admin_lock_cb(me, lockval);
		}
	}
	return 0;
}

int
meinfo_me_is_admin_locked(struct me_t *me)
{
	struct meinfo_t *miptr;
	if (me==NULL)
		return 0;

	// if ont-g is admin locked, we treat all me as admin locked
	if (me->classid!=256) {
		struct me_t *me_ont_g=meinfo_me_get_by_instance_num(256, 0);
		if (meinfo_me_is_admin_locked(me_ont_g))
			return 1;
	}
	miptr=meinfo_util_miptr(me->classid);
	if (miptr && miptr->callback.is_admin_locked_cb)
		return miptr->callback.is_admin_locked_cb(me);
	return 0;
}

int
meinfo_me_is_arc_enabled(struct me_t *me)
{
	struct meinfo_t *miptr;

	if (me==NULL)
		return 0;
	miptr=meinfo_util_miptr(me->classid);
	if (miptr && miptr->callback.is_arc_enabled_cb)
		return miptr->callback.is_arc_enabled_cb(me);

	return 0;
}

//1: ready, 0: not ready
int
meinfo_me_is_ready(struct me_t *me)
{
	struct meinfo_t *miptr;

	if (me == NULL)
	{
		return 0;
	}

	if ((miptr = meinfo_util_miptr(me->classid)) == NULL)
	{
		dbprintf(LOG_ERR, "class error, classid=%d\n", me->classid);
		return 0;
	}

	if (miptr->callback.is_ready_cb != NULL && miptr->hardware.is_ready_hw != NULL)
	{
		return (miptr->callback.is_ready_cb(me) && miptr->hardware.is_ready_hw(me));
	} else if (miptr->callback.is_ready_cb != NULL && miptr->hardware.is_ready_hw == NULL) {
		return miptr->callback.is_ready_cb(me);
	}else if (miptr->callback.is_ready_cb == NULL && miptr->hardware.is_ready_hw != NULL) {
		return miptr->hardware.is_ready_hw(me);
	} else {
		return 1; //default with no is_ready cb function
	}
}

int
meinfo_me_allocate_rare_meid(unsigned short classid, unsigned short *meid)
{
	int i;

	for (i = omci_env_g.mib_rare_meid_start; i < 0xffff; i++)
	{
		if (!meinfo_me_is_exist(classid, i))
		{
			*meid = i;
			return 0;
		}
	}

	return -1;
}

int
meinfo_me_allocate_rare2_meid(unsigned short classid, unsigned short *meid)
{
	int i;

	for (i = omci_env_g.mib_rare2_meid_start; i < 0xffff; i++)
	{
		if (!meinfo_me_is_exist(classid, i))
		{
			*meid = i;
			return 0;
		}
	}

	return -1;
}

int
meinfo_me_copy_from_large_string_me(struct me_t *me, unsigned char *buf, unsigned int len)
{
	int i;
	unsigned int offset;
	struct attr_value_t attr, attr_string;

	if (me == NULL || me->classid != 157)
	{
		return -1;
	}

	//get Number_of_parts
	meinfo_me_attr_get(me, 1, &attr);

	//copy
	offset = 0;
	for (i = 1; i <= attr.data && i <= 15; i++)
	{
		memset(&attr_string, 0x00, sizeof(attr_string));
		meinfo_me_attr_get(me, i + 1, &attr_string);

		if (offset + meinfo_util_attr_get_byte_size(157, i + 1) > len)
		{
			memcpy(buf + offset, attr_string.ptr, len - offset);
			meinfo_me_attr_release(157, i + 1, &attr_string);
			break;
		} else {
			memcpy(buf + offset, attr_string.ptr, meinfo_util_attr_get_byte_size(157, i + 1));
			meinfo_me_attr_release(157, i + 1, &attr_string);
		}
		offset += meinfo_util_attr_get_byte_size(157, i + 1);
	}

	return 0;
}

int
meinfo_me_copy_to_large_string_me(struct me_t *me, unsigned char *buf)
{
	int i, j;
	unsigned char uc;
	unsigned int offset;
	int len, part_size = meinfo_util_attr_get_byte_size(157, 2);
	struct attr_value_t attr, attr_string;

	if (me == NULL || me->classid != 157 || buf == NULL || 0 == (len=strlen(buf))) {
		return -1;
	}
	dbprintf(LOG_DEBUG, "len(%d) part_size(%d) [%s]\n", len, part_size, buf);

	uc = (unsigned char)meinfo_util_me_attr_data(me, 1);
	//set Number_of_parts
	attr.data = (unsigned char)(1 + ((len - 1) / part_size)); // if len != 0
	dbprintf(LOG_DEBUG, "attr.data(%d)\n", (unsigned char)attr.data);
	meinfo_me_attr_set(me, 1, &attr, 0);

	// prepare buff ofr attr string
	memset(&attr_string, 0x00, sizeof(attr_string));
	attr_string.ptr = malloc_safe(part_size+1);
	//copy
	offset = 0;
	for (i = 1; i <= attr.data && i <= 15; i++) {
		bzero(attr_string.ptr, part_size+1);
		if (offset + part_size >= len) {
			memcpy(attr_string.ptr,  buf + offset, len - offset);
			dbprintf(LOG_DEBUG, "%d offset(%d) attr_string.ptr[%s]\n", i, offset, attr_string.ptr);
			meinfo_me_attr_set(me, i + 1, &attr_string, 0);
			break;
		} else {
			memcpy(attr_string.ptr,  buf + offset, part_size);
			dbprintf(LOG_DEBUG, "%d offset(%d) attr_string.ptr[%s]\n", i, offset, attr_string.ptr);
			meinfo_me_attr_set(me, i + 1, &attr_string, 0);
		}
		offset += part_size;
	}
	free_safe(attr_string.ptr);
	dbprintf(LOG_DEBUG, "uc(%d) i(%d)\n", uc, i);
	struct meinfo_t *miptr;
	miptr = meinfo_util_miptr(me->classid);
	if (miptr != NULL) {
		for (j = i+1; j <= uc && j <= 15; j++) {
			if (me->attr[j+1].value.ptr != NULL) {
				dbprintf(LOG_DEBUG, "to bzero Part %d [%s]\n", j, me->attr[j+1].value.ptr);
				bzero(me->attr[j+1].value.ptr, miptr->attrinfo[j+1].byte_size);
			}
		}
	}
	return 0;
}

int
meinfo_me_refresh(struct me_t *me, unsigned char attr_mask[2])
{
	unsigned char attr_mask_get[2]={0, 0};
	struct meinfo_t *miptr;
	int sw_ret=0, hw_ret=0, ret=0, i;

	if (me == NULL) {
		dbprintf(LOG_ERR, "%s\n", meinfo_util_strerror(MEINFO_ERROR_ME_PTR_NULL));
		return (MEINFO_ERROR_ME_PTR_NULL);
	}
	miptr=meinfo_util_miptr(me->classid);
	if (miptr==NULL)
		return MEINFO_ERROR_CLASSID_UNDEF;

	if (omci_env_g.mib_rw_mode == ENV_MIB_RW_MODE_HWHW) {
		for (i=1; i<=miptr->attr_total; i++) {
			if (util_attr_mask_get_bit(attr_mask, i) == 1)
				util_attr_mask_set_bit(attr_mask_get, i);
		}
		if (miptr->hardware.get_hw && (hw_ret=miptr->hardware.get_hw(me, attr_mask_get)) < 0) {
			ret=hw_ret;
			dbprintf(LOG_ERR, "get_hw error!!, sw_ret=%d, classid=%u, meid=0x%x(%u), mask[0]=0x%x, mask[1]=0x%x\n",
				 hw_ret, me->classid, me->meid, attr_mask_get[0], attr_mask_get[1]);
			if (hw_ret == MEINFO_ERROR_ME_NOT_FOUND)
				return hw_ret;
		}
	}

	for (i=1; i<=miptr->attr_total; i++) {
		if (util_attr_mask_get_bit(attr_mask, i) == 1)
			util_attr_mask_set_bit(attr_mask_get, i);
	}
	if (miptr->callback.get_cb && (sw_ret = miptr->callback.get_cb(me, attr_mask_get)) < 0) {
		ret=sw_ret;
		if (sw_ret != MEINFO_ERROR_ATTR_UNDEF) {
			dbprintf(LOG_ERR, "get_cb error!!, sw_ret=%d, classid=%u, meid=0x%x(%u), mask[0]=0x%x, mask[1]=0x%x\n",
				 sw_ret, me->classid, me->meid, attr_mask_get[0], attr_mask_get[1]);
		}
		if (sw_ret == MEINFO_ERROR_ME_NOT_FOUND)
			return sw_ret;
	}

	return ret;
}


int
meinfo_me_flush(struct me_t *me, unsigned char attr_mask[2])
{
	unsigned char attr_mask_temp[2]={0, 0};
	struct meinfo_t *miptr;
	int ret=0, ret2=0, ret3=0;

	if (me == NULL) {
		dbprintf(LOG_ERR, "%s\n", meinfo_util_strerror(MEINFO_ERROR_ME_PTR_NULL));
		return (MEINFO_ERROR_ME_PTR_NULL);
	}
	miptr=meinfo_util_miptr(me->classid);
	if (miptr==NULL)
		return MEINFO_ERROR_CLASSID_UNDEF;

	attr_mask_temp[0] = attr_mask[0];
	attr_mask_temp[1] = attr_mask[1];
	if (miptr->callback.set_cb && (ret = miptr->callback.set_cb(me, attr_mask_temp)) < 0) {
		dbprintf(LOG_ERR, "ret=%d, classid=%u, meid=0x%x(%u), mask[0]=0x%2x, mask[1]=0x%2x\n",
			 ret, me->classid, me->meid, me->meid, attr_mask[0], attr_mask[1]);
	}

	if (omci_env_g.mib_rw_mode == ENV_MIB_RW_MODE_HWHW || 
	    omci_env_g.mib_rw_mode == ENV_MIB_RW_MODE_SWHW) {
		attr_mask_temp[0] = attr_mask[0];
		attr_mask_temp[1] = attr_mask[1];
		if ((ret2=meinfo_me_issue_self_er_attr_group(me, ER_ATTR_GROUP_HW_OP_UPDATE, attr_mask_temp)) < 0) {
			dbprintf(LOG_ERR,
			 "issue self er_attr_group error, ret=%d, classid=%u, instance_num=%d, meid=%u\n",
			ret2, me->classid, me->instance_num, me->meid);
		}
		//check me ready state related with this class
		if ((ret3=meinfo_me_issue_related_er_attr_group(me->classid)) < 0) {
			dbprintf(LOG_ERR,
			 "issue related er_attr_group error, ret=%d, classid=%u, instance_num=%d, meid=%u\n",
			ret3, me->classid, me->instance_num, me->meid);
		}
	}

	if (ret2<0)
		return ret2;
	if (ret3<0)
		return ret3;
	return ret;
}

int
meinfo_me_set_arc_timestamp(struct me_t *me, unsigned char attr_mask[2], unsigned char arc_attr_order, unsigned char arc_interval_attr_order)
{
	struct timeval curr_time;

	if (me == NULL || attr_mask == NULL)
	{
		return -1;
	}

	if(meinfo_util_me_attr_data(me, arc_attr_order) == 1 &&
		(util_attr_mask_get_bit(attr_mask, arc_attr_order) == 1 ||
		util_attr_mask_get_bit(attr_mask, arc_interval_attr_order) == 1))
	{
		if (util_get_uptime(&curr_time) < 0) {
			dbprintf(LOG_ERR, "get uptime error\n");
			return -1;
		}

		//set the arc stamp
		me->arc_timestamp = curr_time.tv_sec;
	}

	return 0;
}

//0:disable, 1:enable
int
meinfo_me_check_arc_interval(struct me_t *me, unsigned char arc_attr_order, unsigned char arc_interval_attr_order)
{
	unsigned int arc_interval;
	struct timeval curr_time;
	struct attr_value_t attr;

	if (me == NULL)
	{
		return 0;
	}

	//check arc and compare arc_timestamp with arc_interval
	if (meinfo_util_me_attr_data(me, arc_attr_order) == 1)
	{
		if ((arc_interval = meinfo_util_me_attr_data(me, arc_interval_attr_order)) == 255)
		{
			//never expire
			return 1;
		} else {
			if (util_get_uptime(&curr_time) < 0) {
				dbprintf(LOG_ERR, "get uptime error\n");
				return 1; //enable
			}

			//compare timestamp
			if ((curr_time.tv_sec - me->arc_timestamp) >= (arc_interval * 60))
			{
				//expire, set arc to 0
				attr.data = 0;
				meinfo_me_attr_set(me, arc_attr_order, &attr, 1);

				return 0; //disable
			} else {
				return 1; //enable
			}
		}
	} else {
		return 0; //disable
	}
}
