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
 * File    : er_attr_group_hw.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <string.h>
#include <sys/time.h>
#include "util.h"
#include "er_group.h"
#include "hwresource.h"

#define ATTR_GROUP_HASH_SIZE	312
#define ATTR_MAX_LINE	1024

struct er_attr_group_hw_entity_t {
	char *name;
	int (*er_attr_group_hw_func) (int action, struct er_attr_group_instance_t *er_attr_group_instance, struct me_t *me, unsigned char attr_mask[2]);
	struct list_head node;
};

static struct list_head er_attr_group_hw_hash[ATTR_GROUP_HASH_SIZE];

void 
er_attr_group_hw_preinit()
{
	int i;
	for (i = 0; i < ATTR_GROUP_HASH_SIZE; i++)
		INIT_LIST_HEAD(&er_attr_group_hw_hash[i]);
}

static inline unsigned int 
hash_position(char *name)
{
	//key generator; DJB Hash
	unsigned int hash=5381,i;
	for(i = 0; i < strlen(name); i++)
		hash = ((hash << 5) + hash) + name[i];
	return  (hash % ATTR_GROUP_HASH_SIZE);
}

static struct er_attr_group_hw_entity_t *
er_attr_group_hw_find(char *name)
{
	struct er_attr_group_hw_entity_t *er_attr_group_hw_entity;
	unsigned int position;

	position=hash_position(name);
	list_for_each_entry(er_attr_group_hw_entity, &er_attr_group_hw_hash[position], node) {
		if(strcmp(name,er_attr_group_hw_entity->name) == 0 )
			return er_attr_group_hw_entity;
	}

        dbprintf(LOG_INFO,"Can't find callback funcation name:%s\n",name);
	return 0;
}

int 
er_attr_group_hw_add(char *name, int (*er_attr_group_hw_func) (int action, struct er_attr_group_instance_t *er_attr_group_instance, struct me_t *me, unsigned char attr_mask[2]))
{
	struct er_attr_group_hw_entity_t *er_attr_group_hw_entity;
	char *tmp_name;
	unsigned int position=0;

        er_attr_group_hw_entity = malloc_safe(sizeof (struct er_attr_group_hw_entity_t));

	//find place to add
	if ( er_attr_group_hw_find(name) != 0 ) {
		dbprintf(LOG_ERR, "Error! er_attr_group_hw function duplicate(%s)\n", name);
                exit(-1);
	}

	tmp_name=malloc_safe(strlen(name) + 1);
	strcpy(tmp_name, name);
	er_attr_group_hw_entity->name = tmp_name;
	er_attr_group_hw_entity->er_attr_group_hw_func = er_attr_group_hw_func;

	position=hash_position(name);
	list_add_tail(&er_attr_group_hw_entity->node, &er_attr_group_hw_hash[position]);
	return 0;
}

int 
er_attr_group_hw_del(char *name)
{
	struct er_attr_group_hw_entity_t *er_attr_group_hw_entity, *tmp_entity;
	unsigned int position=0;

	position=hash_position(name);
	list_for_each_entry_safe(er_attr_group_hw_entity, tmp_entity,  &er_attr_group_hw_hash[position], node) {
		if(strcmp(name,er_attr_group_hw_entity->name) == 0 ) {
			if ( er_attr_group_hw_entity->name)
				free_safe(er_attr_group_hw_entity->name);
			list_del(&er_attr_group_hw_entity->node);
			free_safe(er_attr_group_hw_entity);
			return 0;
		}
	}
	return -1;
}

int 
er_attr_group_hw_dump(int fd)
{
	struct er_attr_group_hw_entity_t *er_attr_group_hw_entity;
	int i;

	for (i=0; i < ATTR_GROUP_HASH_SIZE; i++){
		util_fdprintf(fd, "\nHash key=%d ", i);
		list_for_each_entry(er_attr_group_hw_entity, &er_attr_group_hw_hash[i], node) {
			util_fdprintf(fd, "\tfunc_name=%s ",er_attr_group_hw_entity->name);
		}
	}
	util_fdprintf(fd, "\n");
	return 0;
}

static int 
er_attr_group_hw_default(int action, struct er_attr_group_instance_t *er_attr_group_instance, struct me_t *caused_me, unsigned char attr_mask[2])
{
	char *action_str;

	if(action == 1)
		action_str="ADD";
	else if (action == 2 )
		action_str="DEL";
	else if (action == 3 )
		action_str="UPDATE";
	else
		action_str="UNKNOW";
			
	dbprintf(LOG_ERR,"cb not found: op:%s(%d), name:%s\n",
		action_str, action, er_attr_group_instance->er_attr_group_definition_ptr->name);

	return 0;	//not related
}

int 
er_attr_group_hw(int action, struct er_attr_group_instance_t *er_attr_group_instance, struct me_t *caused_me, unsigned char attr_mask[2])
{
	int ret = 0;
	struct er_attr_group_hw_entity_t *er_attr_group_hw_entity;
	struct timeval tv0, tv1;	
	
	if ( omci_env_g.er_group_hw_time_enable ) {
		util_get_uptime(&tv0);	
	}
	if (er_attr_group_instance != NULL && er_attr_group_instance->er_attr_group_definition_ptr != NULL)
		er_attr_group_hw_entity=er_attr_group_hw_find(er_attr_group_instance->er_attr_group_definition_ptr->name);
	else
		return ER_ATTR_GROUP_HW_RESULT_UNREASONABLE;
	if (er_attr_group_hw_entity) 
		ret = er_attr_group_hw_entity->er_attr_group_hw_func(action, er_attr_group_instance, caused_me, attr_mask);
	else
		ret = er_attr_group_hw_default(action, er_attr_group_instance, caused_me, attr_mask);

	if ( omci_env_g.er_group_hw_time_enable ) {
		util_get_uptime(&tv1);
		if ( action == ER_ATTR_GROUP_HW_OP_ADD) 
			er_attr_group_instance->er_attr_group_definition_ptr->add_time_diff += util_timeval_diff_usec(&tv1, &tv0);
		if ( action == ER_ATTR_GROUP_HW_OP_DEL) 
			er_attr_group_instance->er_attr_group_definition_ptr->del_time_diff += util_timeval_diff_usec(&tv1, &tv0);
	}
	return ret;
}
