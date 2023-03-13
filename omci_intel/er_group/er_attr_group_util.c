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
 * File    : er_attr_group_util.c
 *
 ******************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>

#include "list.h"
#include "util.h"
#include "conv.h"
#include "meinfo.h"
#include "er_group.h"
#include "me_related.h"
#include "hwresource.h"

/* compare the entries of old/new table and find the entry been added,deleted or updated */
static int
_er_attr_group_util_table_op_detection(unsigned short classid, unsigned char attr_order,
	struct attr_table_header_t *tab_header_old, struct attr_table_header_t *tab_header_new, 
	struct attr_table_entry_t **result_entry_old, struct attr_table_entry_t **result_entry_new)
{

	int entry_byte_size=meinfo_util_attr_get_table_entry_byte_size(classid, attr_order);	
	struct list_head *ptr_old, *ptr_new;
	struct attr_table_entry_t *tab_entry_old, *tab_entry_new;
	int loop;

	*result_entry_old=NULL;
	*result_entry_new=NULL;
	
	if (entry_byte_size<0) {
		dbprintf(LOG_ERR, "classid=%u, attr_order=%u, can not get table entry_byte_size\n");
		return ER_ATTR_GROUP_HW_OP_ERROR;
	}
	
	ptr_old=&tab_header_old->entry_list;
	ptr_new=&tab_header_new->entry_list;	
	for (loop=0; loop<10000; loop++) {
		ptr_old=ptr_old->next;
		ptr_new=ptr_new->next;		

		if (ptr_old==&tab_header_old->entry_list) {
			if (ptr_new==&tab_header_new->entry_list) { // both table terminate, two table are the same
				return ER_ATTR_GROUP_HW_OP_NONE;
			} else { // old table terminates before new table => op=entry added in last
				*result_entry_new=list_entry(ptr_new, struct attr_table_entry_t, entry_node);
				return ER_ATTR_GROUP_HW_OP_ADD;
			}
		} else {
			if (ptr_new==&tab_header_new->entry_list) { // new table terminites before old table => entry del in last
				*result_entry_old=list_entry(ptr_old, struct attr_table_entry_t, entry_node);
				return ER_ATTR_GROUP_HW_OP_DEL;
			} // else: table entry comparson continues
		}
	
		tab_entry_old=list_entry(ptr_old, struct attr_table_entry_t, entry_node);
		tab_entry_new=list_entry(ptr_new, struct attr_table_entry_t, entry_node);

		if (tab_entry_old->entry_data_ptr==NULL ||
			tab_entry_new->entry_data_ptr==NULL) {
			dbprintf(LOG_ERR, "table entry has null data pointer\n");
			return ER_ATTR_GROUP_HW_OP_ERROR;
		}

		if (memcmp(tab_entry_old->entry_data_ptr, tab_entry_new->entry_data_ptr, entry_byte_size)!=0) {
			struct attr_table_entry_t *tab_entry_old_next=list_entry(ptr_old->next, struct attr_table_entry_t, entry_node);
			struct attr_table_entry_t *tab_entry_new_next=list_entry(ptr_new->next, struct attr_table_entry_t, entry_node);

			// both table terminate in next entry, this is an update on lasr entry
			if (ptr_old->next==&tab_header_old->entry_list &&
				ptr_new->next==&tab_header_new->entry_list) {
				*result_entry_old=tab_entry_old;
				*result_entry_new=tab_entry_new;
				return ER_ATTR_GROUP_HW_OP_UPDATE;
			}								

			// check next entry in old table
			if (ptr_old->next!=&tab_header_old->entry_list && tab_entry_old_next->entry_data_ptr) {
				if (memcmp(tab_entry_old_next->entry_data_ptr, tab_entry_new->entry_data_ptr, entry_byte_size)==0) {
					*result_entry_old=tab_entry_old;
					return ER_ATTR_GROUP_HW_OP_DEL;
				}
			}
			// check next entry in new table
			if (ptr_new->next!=&tab_header_new->entry_list && tab_entry_new_next->entry_data_ptr) {
				if (memcmp(tab_entry_old->entry_data_ptr, tab_entry_new_next->entry_data_ptr, entry_byte_size)==0) {
					*result_entry_new=tab_entry_new;
					return ER_ATTR_GROUP_HW_OP_ADD;
				}
			}
			
			// check next entry in both old/new table			
			if (ptr_old->next!=&tab_header_old->entry_list && tab_entry_old_next->entry_data_ptr &&
				ptr_new->next!=&tab_header_new->entry_list && tab_entry_new_next->entry_data_ptr) {
				if (memcmp(tab_entry_old_next->entry_data_ptr, tab_entry_new_next->entry_data_ptr, entry_byte_size)==0) {
					*result_entry_old=tab_entry_old;
					*result_entry_new=tab_entry_new;
					return ER_ATTR_GROUP_HW_OP_UPDATE;
				}
			}

			dbprintf(LOG_ERR, "table modified in more than one entry?\n");
			return ER_ATTR_GROUP_HW_OP_ERROR;	
		}
	}

	dbprintf(LOG_ERR, "endless loop happened in table comparison?\n");
	return ER_ATTR_GROUP_HW_OP_ERROR;	
}

int
er_attr_group_util_table_op_detection(struct er_attr_group_instance_t *attr_inst, int inst_order,
	struct attr_table_entry_t **result_entry_old, struct attr_table_entry_t **result_entry_new)
{
	struct me_t *me=er_attr_group_util_attrinst2me(attr_inst, inst_order);
	unsigned short classid;
	unsigned char attr_order;
	
	if (me==NULL) {
		dbprintf(LOG_ERR, "can not find the current me in mib\n");
		return ER_ATTR_GROUP_HW_OP_ERROR;
	}
	
	classid=attr_inst->er_attr_group_definition_ptr->er_attr[inst_order].classid;
	attr_order=attr_inst->er_attr_group_definition_ptr->er_attr[inst_order].attr_order;
	
	if (meinfo_util_attr_get_format(classid, attr_order)!=ATTR_FORMAT_TABLE) {
		if (meinfo_util_miptr(classid)) {
			dbprintf(LOG_ERR, "er_attr_group %s attrinst %u: classid%u %s attr%u %s is not a table\n", 
				attr_inst->er_attr_group_definition_ptr->name, inst_order, 
				classid, meinfo_util_get_name(classid), 
				attr_order, meinfo_util_attr_get_name(classid, attr_order));
		} else {
			dbprintf(LOG_ERR, "classid%u attr%u is not a table\n", classid, attr_order);
		}
	}
	
	return _er_attr_group_util_table_op_detection(classid, attr_order,
			(struct attr_table_header_t *)(attr_inst->er_attr_instance[inst_order].attr_value.ptr),
			(struct attr_table_header_t *)(meinfo_util_me_attr_ptr(me, attr_order)),
			result_entry_old, result_entry_new);
}


struct me_t *
er_attr_group_util_attrinst2me(struct er_attr_group_instance_t *attr_inst, int inst_order)
{
	unsigned short classid, meid;
	struct me_t* me;

	classid=attr_inst->er_attr_group_definition_ptr->er_attr[inst_order].classid;
	meid=attr_inst->er_attr_instance[inst_order].meid;
	me=meinfo_me_get_by_meid(classid, meid);
	
	if (me == NULL) {
                dbprintf(LOG_ERR, "er_attr_group_instance %u has no related me?\n", inst_order);
		return NULL;
	}
	return me;
}

struct me_t *
er_attr_group_util_attrinst2private_me(struct er_attr_group_instance_t *attr_inst, int inst_order)
{
	struct me_t* me=er_attr_group_util_attrinst2me(attr_inst, inst_order);
	struct me_t* private_me=NULL;
	
	if (me) {
		private_me=hwresource_public2private(me);
		if (private_me==NULL) {
	                dbprintf(LOG_ERR, "er_attr_group_instance %u has no related private me?\n", inst_order);
	                return NULL;
		}
	}
	return private_me;
}

int
er_attr_group_util_release_attrinst(struct er_attr_group_instance_t *attr_inst)
{
	int i;

	if (!attr_inst || !attr_inst->er_attr_group_definition_ptr)
	{
		return -1;
	}

	//clear all attr value
	for (i = 0; i < ER_ATTR_GROUP_MEMBER_MAX && attr_inst->er_attr_group_definition_ptr->er_attr[i].classid != 0; i++)
	{
		if (attr_inst->er_attr_group_definition_ptr->er_attr[i].attr_order != 0 &&
			attr_inst->er_attr_group_definition_ptr->er_attr[i].attr_order != 255 &&
			attr_inst->er_attr_group_definition_ptr->er_attr[i].attr_order != 254) //meid and instance num
		{
			meinfo_me_attr_release(attr_inst->er_attr_group_definition_ptr->er_attr[i].classid,
				attr_inst->er_attr_group_definition_ptr->er_attr[i].attr_order,
				&attr_inst->er_attr_instance[i].attr_value);
		}
		attr_inst->er_attr_instance[i].meid = 0;
	}

	free_safe(attr_inst);

	return 0;
}

struct er_attr_group_instance_t *
er_attr_group_util_gen_current_value_attrinst(struct er_attr_group_instance_t *old_attr_inst)
{
	int i;
	struct er_attr_group_instance_t *current_attr_inst;
	struct me_t *me;

	if (!old_attr_inst || !old_attr_inst->er_attr_group_definition_ptr)
	{
		return NULL;
	}

	current_attr_inst = malloc_safe(sizeof(struct er_attr_group_instance_t));
	current_attr_inst->er_attr_group_definition_ptr = old_attr_inst->er_attr_group_definition_ptr;

	for (i = 0; i < ER_ATTR_GROUP_MEMBER_MAX && old_attr_inst->er_attr_group_definition_ptr->er_attr[i].classid != 0; i++)
	{
		if ((me = er_attr_group_util_attrinst2me(old_attr_inst, i))==NULL)
		{
			er_attr_group_util_release_attrinst(current_attr_inst);
			return NULL;
		}

		//copy meid
		current_attr_inst->er_attr_instance[i].meid = old_attr_inst->er_attr_instance[i].meid;

		//copy attr_value
		switch(old_attr_inst->er_attr_group_definition_ptr->er_attr[i].attr_order)
		{
		case 0:
		case 255:
			current_attr_inst->er_attr_instance[i].attr_value.data = me->meid;
			break;
		case 254:
			current_attr_inst->er_attr_instance[i].attr_value.data = me->instance_num;
			break;
		default: 
			meinfo_me_attr_copy_value(old_attr_inst->er_attr_group_definition_ptr->er_attr[i].classid,
				old_attr_inst->er_attr_group_definition_ptr->er_attr[i].attr_order,
				&current_attr_inst->er_attr_instance[i].attr_value,
				&me->attr[old_attr_inst->er_attr_group_definition_ptr->er_attr[i].attr_order].value);
		}
	}

	return current_attr_inst;
}

struct me_t *
er_attr_group_util_find_related_me(struct me_t *me, unsigned short related_claissid)
{
	int i, self_found, related_found;
	unsigned char self_index, related_index;
	struct meinfo_t *miptr;
	struct er_me_group_definition_ptr_node_t *er_me_group_definition_ptr_node;
	struct er_me_group_instance_t *er_me_group_instance;
	struct me_t *result_me;

	if (me == NULL || (miptr = meinfo_util_miptr(me->classid)) == NULL) {
		dbprintf(LOG_ERR, "err=%d, input error\n", ER_GROUP_ERROR_INPUT_ERROR);
		return NULL;
	}

	list_for_each_entry(er_me_group_definition_ptr_node, &miptr->er_me_group_definition_ptr_list, node)
	{
		self_found = related_found = 0;
		//find me index in this er_group 
		for (i = 0; i < ER_ME_GROUP_MEMBER_MAX && er_me_group_definition_ptr_node->ptr->classid[i] != 0; i++)
		{
			if (er_me_group_definition_ptr_node->ptr->classid[i] == me->classid)
			{
				self_index = i;
				self_found = 1;
				break;
			}
		}
		//find related class index in this er_group 
		if (self_found)
		{
			for (i = 0; i < ER_ME_GROUP_MEMBER_MAX && er_me_group_definition_ptr_node->ptr->classid[i] != 0; i++)
			{
				if (er_me_group_definition_ptr_node->ptr->classid[i] == related_claissid)
				{
					related_index = i;
					related_found = 1;
					break;
				}
			}
		}
		if (self_found && related_found)
		{
			//traverse all er_group instance
			list_for_each_entry(er_me_group_instance, &er_me_group_definition_ptr_node->ptr->er_me_group_instance_list, er_me_group_instance_node)
			{
				if (er_me_group_instance->meid[self_index] == me->meid &&
					(result_me = meinfo_me_get_by_meid(related_claissid, er_me_group_instance->meid[related_index])) != NULL)
				{
					return result_me;
				}
			}
		}
	}

	return NULL;
}

// find the attr_mask for specific classid whose attrs are referred in the er_attr_group
// this is for the er_group_hw handle function to get what attr change should be assuemd for me of specific classid
// ps:
// a. for er_attr_group which contains only one classid, 
//    the parm me & attr_mask is the change mask of the spcific me, and the action could follow the attr_mask asis
// b. for er_attr_group which contains mes of more than one classid, 
//    the parm me & attr_mask in merely the change happened in the completion of er_attr_group
// c. so if the handler function want to know what change mask should be assumed for specific me in the er_attr_group
//    the attr_mask should be collected from the er_attr_group_definition
int
er_attr_group_util_get_attr_mask_by_classid(struct er_attr_group_instance_t *er_attr_group_instance, unsigned short classid, unsigned char attr_mask[2])
{
	struct er_attr_group_definition_t *attr_def = er_attr_group_instance->er_attr_group_definition_ptr;
	int total=0, i;
	
	attr_mask[0] = attr_mask[1] = 0;
	for (i=0; i< ER_ATTR_GROUP_MEMBER_MAX; i++) {
		if (attr_def->er_attr[i].classid == 0) {
			break;
		} else if (attr_def->er_attr[i].classid == classid) {
			util_attr_mask_set_bit(attr_mask, attr_def->er_attr[i].attr_order);
			total++;
		}
	}
	return total;
}
