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
 * File    : er_attr_group.c
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

static void 
er_attr_group_dbprintf_me_header(char *prefix, struct me_t *me)
{
	struct meinfo_t *miptr;

	if (omci_env_g.er_group_hw_debug_enable == 0)
	{
		return;
	}
	if (me == NULL || (miptr = meinfo_util_miptr(me->classid)) == NULL)
		return;
	er_attr_group_dbprintf(LOG_ERR, 0, "%s\n<<Caused me>> class=%s(%u): meid=0x%x\n", prefix, miptr->name, me->classid, me->meid);
}

static int 
er_attr_group_dbprintf_instance(int debug_level, struct er_attr_group_instance_t *attr_inst)
{
	struct er_attr_group_definition_t *attr_def;
	struct meinfo_t *miptr;
	int i;
	unsigned char attr_order;
	char buff[1024];
	unsigned short classid;

	if (omci_env_g.er_group_hw_debug_enable == 0 || debug_level > omci_env_g.debug_level)
	{
		return 0;
	}

	attr_def=attr_inst->er_attr_group_definition_ptr;

	//output format hw_name(classid:0xmeid@attr_order,....)
        er_attr_group_dbprintf(debug_level,0,"%s(", attr_def->name);
	for(i=0; i < ER_ATTR_GROUP_MEMBER_MAX; i++){
		if( attr_def->er_attr[i].classid==0)
			break;
		er_attr_group_dbprintf(debug_level,0, "%s%d:0x%x@%d",i==0?"":", ", 
			attr_def->er_attr[i].classid, 
			attr_inst->er_attr_instance[i].meid, 
			attr_def->er_attr[i].attr_order);
	}
	er_attr_group_dbprintf(debug_level,0, ")\n");

	for(i=0; i < ER_ATTR_GROUP_MEMBER_MAX; i++){
		if( attr_def->er_attr[i].classid==0)
			break;
		classid=attr_def->er_attr[i].classid;
		attr_order=attr_def->er_attr[i].attr_order;

		// print classid me_name@attr_order:attrname=attrvalue
		miptr = meinfo_util_miptr(classid);
		if( attr_order == 254 || attr_order == 255 || attr_order == 0){
			char *s="unknow?";
			if( attr_order == 254 )
				s="instid";
			else if( attr_order == 255)
				s="meid";
			er_attr_group_dbprintf(debug_level,0, "(%d)%d %s@%d:%s=%d\n", 
				i, classid, util_strptr_safe(miptr->name), attr_order, s, attr_inst->er_attr_instance[i].attr_value.data);
			continue;
		}
		er_attr_group_dbprintf(debug_level,0, "(%d)%d %s@%d:%s=", 
			i, classid, miptr->name ? miptr->name:"?",attr_order, miptr->attrinfo[attr_order].name);

		if (meinfo_util_attr_get_format(classid, attr_order) == ATTR_FORMAT_TABLE) {
			struct attr_table_header_t *tab_header = attr_inst->er_attr_instance[i].attr_value.ptr;
			struct attr_table_entry_t *tab_entry;
			int j=0;

			er_attr_group_dbprintf(debug_level,0, "\n(table)\n");
			list_for_each_entry(tab_entry, &tab_header->entry_list, entry_node) {
				if (tab_entry->entry_data_ptr != NULL) {
					struct attr_value_t attr;
					attr.data=0;
					attr.ptr=tab_entry->entry_data_ptr;// use attr to encapsulate entrydata for temporary use
					meinfo_conv_attr_to_string(miptr->classid, attr_order, &attr, buff, 1024);
					er_attr_group_dbprintf(debug_level,0, "\t%d: %s\n", j, buff);
					j++;
				}
			}
		} else {
			meinfo_conv_attr_to_string(classid, attr_order, &attr_inst->er_attr_instance[i].attr_value, buff, 1024);
			er_attr_group_dbprintf(debug_level,0, "%s\n", buff);
		}
	}
        er_attr_group_dbprintf(debug_level,0, "\n");
	return 0;
}


// find meid for attrdef by locating the index in er_me_group_definition, then locate me
static struct me_t *
er_attr_group_find_me_for_attrdef(struct er_me_group_definition_t *me_def, struct er_me_group_instance_t *me_inst,
					struct er_attr_group_definition_t *attr_def, int i)
{
	int index;
	for(index=0; index<ER_ATTR_GROUP_MEMBER_MAX; index++) {
		if( attr_def->er_attr[i].classid == me_def->classid[index])
			return meinfo_me_get_by_meid(me_def->classid[index], me_inst->meid[index]);
	}
	return NULL;
}
static int
er_attr_group_me_add_for_one_instance(struct er_me_group_definition_t *me_def, struct er_me_group_instance_t *me_inst, 
					struct er_attr_group_definition_t *attr_def, struct me_t *caused_me, unsigned char attr_mask[2])
{
	struct er_attr_group_instance_t *attr_inst=malloc_safe(sizeof(struct er_attr_group_instance_t));
	unsigned char attr_order;
	int i;
	unsigned char tmp_attr_mask[2];

	//create one er_attr_group_instance
	for(i=0; i<ER_ATTR_GROUP_MEMBER_MAX; i++) {
		struct me_t *me;
	
		if ( attr_def->er_attr[i].classid == 0 )	// end of er_attr in attr_def
			break;

		me=er_attr_group_find_me_for_attrdef(me_def, me_inst, attr_def, i);
		if (me==NULL) {
			free_safe(attr_inst);
			return -1; //error, normally never happen 
		}

		attr_order=attr_def->er_attr[i].attr_order;

		//copy value form er_me_group_instance to attr_group_instance according er_attr_group_definition
		//destination:attr_inst->er_attr_instance[i].attr_value, source:me->attr[attr_order].value
		attr_inst->er_attr_instance[i].meid=me->meid;
		if( attr_order == 254 ){
			attr_inst->er_attr_instance[i].attr_value.data=me->instance_num;
		} else if( attr_order == 255 || attr_order == 0 ){
			attr_inst->er_attr_instance[i].attr_value.data=me->meid;
		} else {
			if( meinfo_me_attr_copy_value(me->classid, attr_order, 
				&attr_inst->er_attr_instance[i].attr_value, &me->attr[attr_order].value)) {	
				dbprintf(LOG_ERR, "attr copy failed,classid=%u,order=%d\n",me->classid, attr_order);
				free_safe(attr_inst);
				return -1;
			}
		}
	}
	attr_inst->er_attr_group_definition_ptr=attr_def;
	list_add_tail(&attr_inst->next_er_attr_group_instance_node, &me_inst->er_attr_group_instance_list);

	// call hw routine
	er_attr_group_dbprintf_instance(LOG_ERR, attr_inst);
	//set attr_mask
	tmp_attr_mask[0] = attr_mask[0];
	tmp_attr_mask[1] = attr_mask[1];
	attr_inst->exec_result=er_attr_group_hw(ER_ATTR_GROUP_HW_OP_ADD, attr_inst, caused_me, tmp_attr_mask);	

	return 0;
}

int 
er_attr_group_me_add(struct me_t *me, unsigned char attr_mask[2])
{
	int is_me_header_printed = 0;
	int ret;
	struct meinfo_t *miptr;
	struct er_me_group_definition_ptr_node_t *node_pos;
	struct er_me_group_instance_find_context_t context;

	if (me == NULL || (miptr = meinfo_util_miptr(me->classid)) == NULL) {
		dbprintf(LOG_ERR, "err=%d, input error\n", ER_GROUP_ERROR_INPUT_ERROR);
		return ER_GROUP_ERROR_INPUT_ERROR;
	}

	//find er_me_group_definition for the classid
	list_for_each_entry(node_pos, &miptr->er_me_group_definition_ptr_list, node)
	{
		struct list_head er_me_group_instance_list;
		struct er_me_group_instance_t *er_me_group_instance_pos, *er_me_group_instance_n;
		struct er_attr_group_definition_t *er_attr_group_def;

		memset(&context, 0x00, sizeof(struct er_me_group_instance_find_context_t));
		INIT_LIST_HEAD(&er_me_group_instance_list);

		context.er_group_definition_ptr = node_pos->ptr;
		context.found_instance_list = &er_me_group_instance_list;
		context.must_have_me = me;
		if ((ret = er_me_group_instance_find(&context)) > 0)
		{
			//found
			if (!is_me_header_printed) {
				er_attr_group_dbprintf_me_header("##Add", me);
				is_me_header_printed = 1;
			}

			list_for_each_entry_safe(er_me_group_instance_pos, er_me_group_instance_n, context.found_instance_list, er_me_group_instance_node)
			{
				//move
				list_del_init(&er_me_group_instance_pos->er_me_group_instance_node);
				list_add_tail(&er_me_group_instance_pos->er_me_group_instance_node, &node_pos->ptr->er_me_group_instance_list);

				//create er_attr_group_instance based on er_attr_group_definition_list
				list_for_each_entry(er_attr_group_def, &node_pos->ptr->er_attr_group_definition_list, er_attr_group_definition_node) {
					if (er_attr_group_me_add_for_one_instance(node_pos->ptr, er_me_group_instance_pos, er_attr_group_def, me, attr_mask)<0) {
						dbprintf(LOG_ERR, "er_attr_group_instance_add error\n");
					}
				}
			}
		}
	}

	return 0;
}

static int
er_attr_group_me_delete_for_once_instance(struct er_attr_group_instance_t *instance, struct me_t *me)
{
	unsigned char attr_mask[2] = {0, 0};
	int j;

	if (instance->er_attr_group_definition_ptr == NULL) {
		dbprintf(LOG_ERR, "delete definition ptr error\n");
		return -1;
	}

	// call hw routine
	er_attr_group_dbprintf_instance(LOG_ERR, instance);
	instance->exec_result=er_attr_group_hw(ER_ATTR_GROUP_HW_OP_DEL, instance, me, attr_mask);

	//clear all attrs in this instance
	for (j = 0; j < ER_ATTR_GROUP_MEMBER_MAX && instance->er_attr_group_definition_ptr && instance->er_attr_group_definition_ptr->er_attr[j].classid != 0; j++)
	{
		struct er_attr_t *er_attr_definition = instance->er_attr_group_definition_ptr->er_attr;
		if (er_attr_definition[j].attr_order != 0 &&
		    er_attr_definition[j].attr_order != 255 &&
		    er_attr_definition[j].attr_order != 254) { //meid and instance num 
			meinfo_me_attr_release(er_attr_definition[j].classid, er_attr_definition[j].attr_order, 
						&instance->er_attr_instance[j].attr_value);
		}
	}

	//delete this er_attr_group_instance_pos
	list_del(&instance->next_er_attr_group_instance_node);
	free_safe(instance);
	
	return 0;
}
int
er_attr_group_me_delete(struct me_t *me)
{
	int i, is_me_header_printed = 0;
	struct meinfo_t *miptr;
	struct er_me_group_definition_ptr_node_t *er_me_group_definition_ptr_node;
	struct er_me_group_instance_t *er_me_group_instance_pos, *er_me_group_instance_pos_n;
	struct er_attr_group_instance_t *er_attr_group_instance_pos, *er_attr_group_instance_n;

	if (me == NULL || (miptr = meinfo_util_miptr(me->classid)) == NULL)
	{
		dbprintf(LOG_ERR, "err=%d, input error\n", ER_GROUP_ERROR_INPUT_ERROR);
		return ER_GROUP_ERROR_INPUT_ERROR;
	}

	//traverse all er_me_group_definition of this class
	list_for_each_entry(er_me_group_definition_ptr_node, &miptr->er_me_group_definition_ptr_list, node)
	{
		if (er_me_group_definition_ptr_node->ptr != NULL)
		{
			//find the corrresponding class sequence in the definition
			for (i = 0; i < ER_ME_GROUP_MEMBER_MAX && er_me_group_definition_ptr_node->ptr->classid[i] != 0; i++)
			{
				if (er_me_group_definition_ptr_node->ptr->classid[i] == me->classid)
					break;
			}

			//i, respresent the sequence of class in the definition
			if (i == ER_ME_GROUP_MEMBER_MAX || er_me_group_definition_ptr_node->ptr->classid[i] == 0)
			{
				dbprintf(LOG_ERR, "err=%d, input error\n", ER_GROUP_ERROR_INPUT_ERROR);
				continue;
			}
			
			//traverse all er_me_group_instance this definition
			list_for_each_entry_safe(er_me_group_instance_pos, er_me_group_instance_pos_n, &er_me_group_definition_ptr_node->ptr->er_me_group_instance_list, er_me_group_instance_node)
			{
				if (er_me_group_instance_pos->meid[i] != me->meid) //meid not match, go to next
					continue;

				// ensure er_attr_group_instance list head pointer is okay before release 
				// ps: ugly check: as er_me_group_instance_pos->er_attr_group_instance_list is a list head
				if (er_me_group_instance_pos->er_attr_group_instance_list.next != LIST_POISON1 &&
				    er_me_group_instance_pos->er_attr_group_instance_list.prev != LIST_POISON2) {

					//find all er_attr_group_instance, delete it
					list_for_each_entry_safe(er_attr_group_instance_pos, er_attr_group_instance_n, &er_me_group_instance_pos->er_attr_group_instance_list, next_er_attr_group_instance_node)
					{
						if (!is_me_header_printed) {
							er_attr_group_dbprintf_me_header("##Delete", me);
							is_me_header_printed = 1;
						}
						if (er_attr_group_me_delete_for_once_instance(er_attr_group_instance_pos, me)<0)
							continue;
					}
				}
	
				//delete this er_me_group_instance_pos
				list_del(&er_me_group_instance_pos->er_me_group_instance_node);
				free_safe(er_me_group_instance_pos);
			}
		}
	}

	return 0;
}

static int
er_attr_group_is_related_to_attrmask(struct er_attr_group_definition_t *er_attr_group_definition_ptr, unsigned short classid, unsigned char attr_mask[2])
{
	int j;
	for (j = 0; j < ER_ATTR_GROUP_MEMBER_MAX && er_attr_group_definition_ptr->er_attr[j].classid != 0; j++) {
		if (er_attr_group_definition_ptr->er_attr[j].classid == classid &&
			(util_attr_mask_get_bit(attr_mask, er_attr_group_definition_ptr->er_attr[j].attr_order) == 1))
			return 1;
	}
	return 0;
}
static int
er_attr_group_me_update_for_one_instance(struct er_attr_group_instance_t *instance, struct me_t *me, unsigned char attr_mask[2])
{
	int j;

	if (instance->er_attr_group_definition_ptr == NULL)
		return -1;
	
	// call hw routine
	er_attr_group_dbprintf_instance(LOG_ERR, instance);
	instance->exec_result=er_attr_group_hw(ER_ATTR_GROUP_HW_OP_UPDATE, instance, me, attr_mask);

	//update all attrs modified in this instance
	for (j = 0; j < ER_ATTR_GROUP_MEMBER_MAX && instance->er_attr_group_definition_ptr && instance->er_attr_group_definition_ptr->er_attr[j].classid != 0; j++)
	{
		struct er_attr_t *er_attr_definition = instance->er_attr_group_definition_ptr->er_attr;

		if (er_attr_definition[j].classid == me->classid &&
		    util_attr_mask_get_bit(attr_mask, er_attr_definition[j].attr_order)) {
			// update  value to into er_attr_group_instance
			switch(er_attr_definition[j].attr_order)
			{
			case 0:		// unknown, treated as meid
			case 255:	// meid
				instance->er_attr_instance[j].attr_value.data = me->meid;
				break;
			case 254:	// instance num
				instance->er_attr_instance[j].attr_value.data = me->instance_num;
				break;
			default:
				meinfo_me_attr_release(er_attr_definition[j].classid, er_attr_definition[j].attr_order,
							&instance->er_attr_instance[j].attr_value);
				meinfo_me_attr_copy_value(er_attr_definition[j].classid, er_attr_definition[j].attr_order,
							&instance->er_attr_instance[j].attr_value,
							&me->attr[er_attr_definition[j].attr_order].value);
			}
		}
	}
	return 0;
}
int
er_attr_group_me_update(struct me_t *me, unsigned char attr_mask[2])
{
	int i, is_me_header_printed = 0;
	struct meinfo_t *miptr;
	struct er_me_group_definition_ptr_node_t *er_me_group_definition_ptr_node;
	struct er_me_group_instance_t *er_me_group_instance_pos, *er_me_group_instance_pos_n;
	struct er_attr_group_instance_t *er_attr_group_instance_pos, *er_attr_group_instance_n;

	if (me == NULL || (miptr = meinfo_util_miptr(me->classid)) == NULL)
	{
		dbprintf(LOG_ERR, "err=%d, input error\n", ER_GROUP_ERROR_INPUT_ERROR);
		return ER_GROUP_ERROR_INPUT_ERROR;
	}
	dbprintf(LOG_DEBUG, "classid(%d) attr_mask[0](0x%02x) attr_mask[1](0x%02x)\n", me->classid ,attr_mask[0], attr_mask[1]);

	//check for relation attrs in the attr_mask, if any relation(pointer) attrs will be updated, then delete this me then add it again
	for (i = 1; i <= miptr->attr_total; i++)
	{
		if (!util_attr_mask_get_bit(attr_mask, i))
			continue;
		if (meinfo_util_attr_get_format(me->classid, i) == ATTR_FORMAT_POINTER ||
		    meinfo_util_attr_get_is_related_key(me->classid, i))
		{
			//detele this me 
			er_attr_group_me_delete(me);
			//add this me
			er_attr_group_me_add(me, attr_mask);
			return 0;
		}
	}

	//traverse all er_me_group_definition of this class
	list_for_each_entry(er_me_group_definition_ptr_node, &miptr->er_me_group_definition_ptr_list, node)
	{
		if (er_me_group_definition_ptr_node->ptr == NULL)
			continue;
		//find the corrresponding class sequence in the definition
		for (i = 0; i < ER_ME_GROUP_MEMBER_MAX && er_me_group_definition_ptr_node->ptr->classid[i] != 0; i++)
		{
			if (er_me_group_definition_ptr_node->ptr->classid[i] == me->classid)
				break;
		}

		//i, respresent the sequence of class in the definition
		if (i == ER_ME_GROUP_MEMBER_MAX || er_me_group_definition_ptr_node->ptr->classid[i] == 0)
		{
			dbprintf(LOG_ERR, "err=%d, input error\n", ER_GROUP_ERROR_INPUT_ERROR);
			continue;
		}

		//traverse all er_me_group_instance this definition
		list_for_each_entry_safe(er_me_group_instance_pos, er_me_group_instance_pos_n, &er_me_group_definition_ptr_node->ptr->er_me_group_instance_list, er_me_group_instance_node)
		{
			if (er_me_group_instance_pos->meid[i] != me->meid)
			{
				dbprintf(LOG_DEBUG, "er_me_group_instance update error \n");
				continue;
			}

			//find all er_attr_group_instance, update it
			list_for_each_entry_safe(er_attr_group_instance_pos, er_attr_group_instance_n, &er_me_group_instance_pos->er_attr_group_instance_list, next_er_attr_group_instance_node)
			{
				if (er_attr_group_instance_pos->er_attr_group_definition_ptr == NULL) {
					dbprintf(LOG_ERR, "update definition ptr error\n");
					continue;
				}					
				if (er_attr_group_is_related_to_attrmask(er_attr_group_instance_pos->er_attr_group_definition_ptr, me->classid, attr_mask)) {
					if (!is_me_header_printed) {
						er_attr_group_dbprintf_me_header("##Update", me);
						is_me_header_printed = 1;
					}						
					er_attr_group_me_update_for_one_instance(er_attr_group_instance_pos, me, attr_mask);
				}
			}
		}
	}

	return 0;
}
