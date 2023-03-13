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
 * File    : er_me_group.c
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

// base for all er_me_group_definition
struct list_head er_me_group_definition_list;

int 
er_me_group_preinit(void)
{
	INIT_LIST_HEAD(&er_me_group_definition_list);
	return 0;
}

//-1: not found, >= 0: found
static unsigned int
er_me_group_instance_find_class(struct er_me_group_definition_t *er_group_definition_ptr, unsigned short classid)
{
	int i;

	if (er_group_definition_ptr == NULL)
	{
		return ER_GROUP_CLASS_NOT_FOUND;
	}

	for (i = 0; i < ER_ME_GROUP_MEMBER_MAX && er_group_definition_ptr->classid[i] != 0; i++)
	{
		if (classid == er_group_definition_ptr->classid[i])
		{
			return i;
		}
	}

	return ER_GROUP_CLASS_NOT_FOUND;
}

static void
er_me_group_instance_release_context_list(struct list_head *context_list)
{
	struct er_me_group_instance_context_node_t *context_node_curr, *context_node_curr_n;

	if (context_list == NULL)
	{
		return;
	}

	list_for_each_entry_safe(context_node_curr, context_node_curr_n, context_list, node)
	{
		list_del(&context_node_curr->node);
		free_safe(context_node_curr);
	}

	INIT_LIST_HEAD(context_list);

	return;
}

//0: have no contexts, >0: have contexts
static int
er_me_group_instance_find_join_list_from(struct er_me_group_definition_t *er_group_definition_ptr,
	struct list_head *context_list, struct meinfo_t *miptr, unsigned char seq_from, unsigned char seq_to)
{
	struct list_head context_list_new;
	struct er_me_group_instance_context_node_t *context_node_curr, *context_node_new;
	struct me_t *me;

	if (er_group_definition_ptr == NULL || context_list == NULL || miptr == NULL ||
		seq_from >= ER_ME_GROUP_MEMBER_MAX || seq_to >= ER_ME_GROUP_MEMBER_MAX ||
		list_empty(&miptr->me_instance_list) || list_empty(context_list))
	{
		return 0;
	}

	INIT_LIST_HEAD(&context_list_new);

	//check from
	if (miptr->classid == er_group_definition_ptr->classid[seq_from])
	{
		//get to
		list_for_each_entry(context_node_curr, context_list, node)
		{
			if (context_node_curr->context_me[seq_to] == NULL)
			{
				continue;
			}
			//get me for from
			list_for_each_entry(me, &miptr->me_instance_list, instance_node)
			{
				if (me->is_ready == 0)
				{
					continue;
				}
				if (me_related(me, context_node_curr->context_me[seq_to]))
				{
					//allocate a new one context node
					context_node_new = malloc_safe(sizeof(struct er_me_group_instance_context_node_t));
					INIT_LIST_NODE(&context_node_new->node);
					memcpy(context_node_new->context_me, context_node_curr->context_me, 4 * ER_ME_GROUP_MEMBER_MAX);
					context_node_new->context_me[seq_from] = me;
					list_add_tail(&context_node_new->node, &context_list_new);
				}
			}
		}

		//release old, replace new
		er_me_group_instance_release_context_list(context_list);

		list_replace_init(&context_list_new, context_list);

		return (!list_empty(context_list));
	}else {
		dbprintf(LOG_ERR, "classid=%u not match, er_group_definition id=%u, class seq=%u\n", miptr->classid, er_group_definition_ptr->id, seq_from);
		return 0;
	}	
}

//0: have no contexts, >0: have contexts
static int
er_me_group_instance_find_join_list_to(struct er_me_group_definition_t *er_group_definition_ptr,
	struct list_head *context_list, struct meinfo_t *miptr, unsigned char seq_from, unsigned char seq_to)
{
	struct list_head context_list_new;
	struct er_me_group_instance_context_node_t *context_node_curr, *context_node_new;
	struct me_t *me;

	if (er_group_definition_ptr == NULL || context_list == NULL || miptr == NULL ||
		seq_from >= ER_ME_GROUP_MEMBER_MAX || seq_to >= ER_ME_GROUP_MEMBER_MAX ||
		list_empty(&miptr->me_instance_list) || list_empty(context_list))
	{
		return 0;
	}

	INIT_LIST_HEAD(&context_list_new);

	//check to
	if (miptr->classid == er_group_definition_ptr->classid[seq_to]) 
	{
		//get from
		list_for_each_entry(context_node_curr, context_list, node)
		{
			if (context_node_curr->context_me[seq_from] == NULL)
			{
				continue;
			}
			//get me for to
			list_for_each_entry(me, &miptr->me_instance_list, instance_node)
			{
				if (me->is_ready == 0)
				{
					continue;
				}
				if (me_related(context_node_curr->context_me[seq_from], me))
				{
					//allocate a new one context node
					context_node_new = malloc_safe(sizeof(struct er_me_group_instance_context_node_t));
					INIT_LIST_NODE(&context_node_new->node);
					memcpy(context_node_new->context_me, context_node_curr->context_me, 4 * ER_ME_GROUP_MEMBER_MAX);
					context_node_new->context_me[seq_to] = me;
					list_add_tail(&context_node_new->node, &context_list_new);
				}
			}
		}

		//release old, replace new
		er_me_group_instance_release_context_list(context_list);

		list_replace_init(&context_list_new, context_list);

		return (!list_empty(context_list));
	} else {
		dbprintf(LOG_ERR, "classid=%u not match, er_group_definition id=%u, class seq=%u\n", miptr->classid, er_group_definition_ptr->id, seq_to);
		return 0;
	}

}

//0: have no contexts, >0: have contexts
static int
er_me_group_instance_find_join_me_from(struct er_me_group_definition_t *er_group_definition_ptr,
	struct list_head *context_list, struct me_t *must_have, unsigned char seq_from, unsigned char seq_to)
{
	struct list_head context_list_new;
	struct er_me_group_instance_context_node_t *context_node_curr, *context_node_new;

	if (er_group_definition_ptr == NULL || context_list == NULL || must_have == NULL ||
		seq_from >= ER_ME_GROUP_MEMBER_MAX || seq_to >= ER_ME_GROUP_MEMBER_MAX ||
		must_have->is_ready == 0 || list_empty(context_list))
	{
		return 0;
	}

	INIT_LIST_HEAD(&context_list_new);

	//check from
	if (must_have->classid == er_group_definition_ptr->classid[seq_from])
	{
		//get to
		list_for_each_entry(context_node_curr, context_list, node)
		{
			if (context_node_curr->context_me[seq_to] == NULL)
			{
				continue;
			}
			if (me_related(must_have, context_node_curr->context_me[seq_to]))
			{
				//allocate a new one context node
				context_node_new = malloc_safe(sizeof(struct er_me_group_instance_context_node_t));
				INIT_LIST_NODE(&context_node_new->node);
				memcpy(context_node_new->context_me, context_node_curr->context_me, 4 * ER_ME_GROUP_MEMBER_MAX);
				context_node_new->context_me[seq_from] = must_have;
				list_add_tail(&context_node_new->node, &context_list_new);
			}
		}

		//release old, replace new
		er_me_group_instance_release_context_list(context_list);

		list_replace_init(&context_list_new, context_list);

		return (!list_empty(context_list));
	} else {
		dbprintf(LOG_ERR, "classid=%u not match, er_group_definition id=%u, class seq=%u\n", must_have->classid, er_group_definition_ptr->id, seq_from);
		return 0;
	}
}

//0: have no contexts, >0: have contexts
static int
er_me_group_instance_find_join_me_to(struct er_me_group_definition_t *er_group_definition_ptr,
	struct list_head *context_list, struct me_t *must_have, unsigned char seq_from, unsigned char seq_to)
{
	struct list_head context_list_new;
	struct er_me_group_instance_context_node_t *context_node_curr, *context_node_new;

	if (er_group_definition_ptr == NULL || context_list == NULL || must_have == NULL ||
		seq_from >= ER_ME_GROUP_MEMBER_MAX || seq_to >= ER_ME_GROUP_MEMBER_MAX ||
		must_have->is_ready == 0 || list_empty(context_list))
	{
		return 0;
	}

	INIT_LIST_HEAD(&context_list_new);

	//check to
	if (must_have->classid == er_group_definition_ptr->classid[seq_to])
	{
		//get from
		list_for_each_entry(context_node_curr, context_list, node)
		{
			if (context_node_curr->context_me[seq_from] == NULL)
			{
				continue;
			}
			if (me_related(context_node_curr->context_me[seq_from], must_have))
			{
				//allocate a new one context node
				context_node_new = malloc_safe(sizeof(struct er_me_group_instance_context_node_t));
				INIT_LIST_NODE(&context_node_new->node);
				memcpy(context_node_new->context_me, context_node_curr->context_me, 4 * ER_ME_GROUP_MEMBER_MAX);
				context_node_new->context_me[seq_to] = must_have;
				list_add_tail(&context_node_new->node, &context_list_new);
			}
		}

		//release old, replace new
		er_me_group_instance_release_context_list(context_list);

		list_replace_init(&context_list_new, context_list);

		return (!list_empty(context_list));
	} else {
		dbprintf(LOG_ERR, "classid=%u not match, er_group_definition id=%u, class seq=%u\n", must_have->classid, er_group_definition_ptr->id, seq_to);
		return 0;
	}
}

//0: have no contexts, >0: have contexts
static int
er_me_group_instance_find_join_list(struct er_me_group_definition_t *er_group_definition_ptr,
	struct list_head *context_list, struct meinfo_t *miptr, unsigned char seq)
{
	struct list_head context_list_new;
	struct er_me_group_instance_context_node_t *context_node_curr, *context_node_new;
	struct me_t *me;

	if (er_group_definition_ptr == NULL || context_list == NULL || miptr == NULL ||
		seq >= ER_ME_GROUP_MEMBER_MAX || list_empty(&miptr->me_instance_list))
	{
		return 0;
	}

	INIT_LIST_HEAD(&context_list_new);

	if (miptr->classid == er_group_definition_ptr->classid[seq])
	{
		if (list_empty(context_list))
		{
			list_for_each_entry(me, &miptr->me_instance_list, instance_node)
			{
				if (me->is_ready == 0)
				{
					continue;
				}

				//allocate a new one context node
				context_node_new = malloc_safe(sizeof(struct er_me_group_instance_context_node_t));
				INIT_LIST_NODE(&context_node_new->node);
				context_node_new->context_me[seq] = me;
				list_add_tail(&context_node_new->node, &context_list_new);
			}
		} else {
			list_for_each_entry(context_node_curr, context_list, node)
			{
				list_for_each_entry(me, &miptr->me_instance_list, instance_node)
				{
					if (me->is_ready == 0)
					{
						continue;
					}

					//allocate a new one context node
					context_node_new = malloc_safe(sizeof(struct er_me_group_instance_context_node_t));
					INIT_LIST_NODE(&context_node_new->node);
					memcpy(context_node_new->context_me, context_node_curr->context_me, 4 * ER_ME_GROUP_MEMBER_MAX);
					context_node_new->context_me[seq] = me;
					list_add_tail(&context_node_new->node, &context_list_new);
				}
			}
		}

		//release old, replace new
		er_me_group_instance_release_context_list(context_list);

		list_replace_init(&context_list_new, context_list);

		return (!list_empty(context_list));
	} else {
		dbprintf(LOG_ERR, "classid=%u not match, er_group_definition id=%u, class seq=%u\n", miptr->classid, er_group_definition_ptr->id, seq);
		return 0;
	}

}

//0: have no contexts, >0: have contexts
static int
er_me_group_instance_find_join_me(struct er_me_group_definition_t *er_group_definition_ptr,
	struct list_head *context_list, struct me_t *must_have, unsigned char seq)
{
	struct list_head context_list_new;
	struct er_me_group_instance_context_node_t *context_node_curr, *context_node_new;

	if (er_group_definition_ptr == NULL || context_list == NULL || must_have == NULL ||
		seq >= ER_ME_GROUP_MEMBER_MAX || must_have->is_ready == 0)
	{
		return 0;
	}

	INIT_LIST_HEAD(&context_list_new);

	if (must_have->classid == er_group_definition_ptr->classid[seq])
	{
		if (list_empty(context_list))
		{
			//allocate a new one context node
			context_node_new = malloc_safe(sizeof(struct er_me_group_instance_context_node_t));
			INIT_LIST_NODE(&context_node_new->node);
			context_node_new->context_me[seq] = must_have;
			list_add_tail(&context_node_new->node, &context_list_new);
		} else {
			list_for_each_entry(context_node_curr, context_list, node)
			{
				//allocate a new one context node
				context_node_new = malloc_safe(sizeof(struct er_me_group_instance_context_node_t));
				INIT_LIST_NODE(&context_node_new->node);
				memcpy(context_node_new->context_me, context_node_curr->context_me, 4 * ER_ME_GROUP_MEMBER_MAX);
				context_node_new->context_me[seq] = must_have;
				list_add_tail(&context_node_new->node, &context_list_new);
			}
		}

		//release old, replace new
		er_me_group_instance_release_context_list(context_list);

		list_replace_init(&context_list_new, context_list);

		return (!list_empty(context_list));
	} else {
		dbprintf(LOG_ERR, "classid=%u not match, er_group_definition id=%u, class seq=%u\n", must_have->classid, er_group_definition_ptr->id, seq);
		return 0;
	}
}

//0: have no contexts, >0: have contexts
static int
er_me_group_instance_find_join_related(struct er_me_group_definition_t *er_group_definition_ptr,
	struct list_head *context_list, unsigned char seq_from, unsigned char seq_to)
{
	struct er_me_group_instance_context_node_t *context_node_curr, *context_node_n;

	if (er_group_definition_ptr == NULL || context_list == NULL ||
		seq_from >= ER_ME_GROUP_MEMBER_MAX || seq_to >= ER_ME_GROUP_MEMBER_MAX ||
		list_empty(context_list))
	{
		return 0;
	}

	// only do relation check
	list_for_each_entry_safe(context_node_curr, context_node_n, context_list, node)
	{
		if (!me_related(context_node_curr->context_me[seq_from], context_node_curr->context_me[seq_to]))
		{
			//delete and release
			list_del_init(&context_node_curr->node);
			free_safe(context_node_curr);
		}
	}

	return (!list_empty(context_list));
}

//0: not found, 1: found one at least
int
er_me_group_instance_find(struct er_me_group_instance_find_context_t *context)
{
	int i, j;
	int ret;
	unsigned char joined, error;
	unsigned char checked_flag[ER_ME_GROUP_MEMBER_MAX];
	unsigned char referred_checked_flag[ER_ME_GROUP_MEMBER_MAX];
	struct meinfo_t *miptr[ER_ME_GROUP_MEMBER_MAX];
	struct er_me_relation_t *er_me_relation;
	struct list_head context_list;
	struct er_me_group_instance_context_node_t *context_node_curr;
	struct er_me_group_instance_t *er_me_group_instance_new;

	if (context == NULL || context->er_group_definition_ptr == NULL || context->found_instance_list == NULL)
	{
		return 0;
	}

	memset(miptr, 0x00, 4 * ER_ME_GROUP_MEMBER_MAX);
	memset(checked_flag, 0x00, 1 * ER_ME_GROUP_MEMBER_MAX);
	INIT_LIST_HEAD(&context_list);

	//check each classes's instance list were not empty
	for (i = 0; i < ER_ME_GROUP_MEMBER_MAX && context->er_group_definition_ptr->classid[i] != 0; i++)
	{
		if ((miptr[i] = meinfo_util_miptr(context->er_group_definition_ptr->classid[i])) == NULL)
		{
			dbprintf(LOG_ERR, "definition err, classid=%d\n", context->er_group_definition_ptr->classid[i]);
			return 0;
		} else {
			if (list_empty(&miptr[i]->me_instance_list))
			{
				//could not found any instance
				dbprintf(LOG_INFO, "me instance list empty, classid=%d\n", context->er_group_definition_ptr->classid[i]);
				return 0;
			}
		}
	}

	//traverse all members in er_group_definition_ptr->classid, er_group_definition_ptr->er_me_relation_list has the same order
	i = 0;
	list_for_each_entry(er_me_relation, &context->er_group_definition_ptr->er_me_relation_list, er_me_relation_node)
	{
		unsigned char class_seq;

		if (checked_flag[i] == 0) // not add yet
		{
			memset(referred_checked_flag, 0x00, 1 * ER_ME_GROUP_MEMBER_MAX);

			joined = 0;
			//find referred class already checked in, use this one to join with new class
			for (j = 0; j < ER_ME_GROUP_MEMBER_MAX && er_me_relation->referred_classid[j] != 0; j++)
			{
				if ((class_seq = er_me_group_instance_find_class(context->er_group_definition_ptr, er_me_relation->referred_classid[j])) < ER_ME_GROUP_MEMBER_MAX &&
					checked_flag[class_seq])
				{
					//found, join and relation check with this class for the new class
					if (context->must_have_me != NULL && context->must_have_me->classid == miptr[i]->classid)
					{
						ret = er_me_group_instance_find_join_me_from(context->er_group_definition_ptr, &context_list, context->must_have_me, i, class_seq);
					} else {
						ret = er_me_group_instance_find_join_list_from(context->er_group_definition_ptr, &context_list, miptr[i], i, class_seq);
					}
					if (ret == 0)
					{
						er_me_group_instance_release_context_list(&context_list);
						return 0;
					}

					//mark referred_checked_flag
					referred_checked_flag[j] = 1;
					joined = 1;
					break;
				}
			}

			//have no referred class joined, just join self
			if (!joined)
			{
				//join new class only
				if (context->must_have_me != NULL && context->must_have_me->classid == miptr[i]->classid)
				{
					ret = er_me_group_instance_find_join_me(context->er_group_definition_ptr, &context_list, context->must_have_me, i);
				} else {
					ret = er_me_group_instance_find_join_list(context->er_group_definition_ptr, &context_list, miptr[i], i);
				}
				if (ret == 0)
				{
					er_me_group_instance_release_context_list(&context_list);
					return 0;
				}
			}

			//join other refer classes
			for (j = 0; j < ER_ME_GROUP_MEMBER_MAX && er_me_relation->referred_classid[j] != 0; j++)
			{
				if (!referred_checked_flag[j])
				{
					//if checked in, check the relation only, otherwise join the one
					if ((class_seq = er_me_group_instance_find_class(context->er_group_definition_ptr, er_me_relation->referred_classid[j])) < ER_ME_GROUP_MEMBER_MAX)
					{
						if (checked_flag[class_seq])
						{
							//check relation
							ret = er_me_group_instance_find_join_related(context->er_group_definition_ptr, &context_list, i, class_seq);
						} else {
							//join
							if (context->must_have_me != NULL && context->must_have_me->classid == miptr[class_seq]->classid)
							{
								ret = er_me_group_instance_find_join_me_to(context->er_group_definition_ptr, &context_list, context->must_have_me, i, class_seq);
							} else {
								ret = er_me_group_instance_find_join_list_to(context->er_group_definition_ptr, &context_list, miptr[class_seq], i, class_seq);
							}
							//mark checked flag
							checked_flag[class_seq] = 1;
						}
					} else {
						ret = 0;
					}
					if (ret == 0)
					{
						er_me_group_instance_release_context_list(&context_list);
						return 0;
					}
				}
			}

			//set checked
			checked_flag[i] = 1;
		} else { //already add
			for (j = 0; j < ER_ME_GROUP_MEMBER_MAX && er_me_relation->referred_classid[j] != 0; j++)
			{
				//if checked in, check the relation only, otherwise join the one
				if ((class_seq = er_me_group_instance_find_class(context->er_group_definition_ptr, er_me_relation->referred_classid[j])) < ER_ME_GROUP_MEMBER_MAX)
				{
					if (checked_flag[class_seq]) {
						//check relation
						ret = er_me_group_instance_find_join_related(context->er_group_definition_ptr, &context_list, i, class_seq);
					} else {
						//join
						if (context->must_have_me != NULL && context->must_have_me->classid == miptr[class_seq]->classid)
						{
							ret = er_me_group_instance_find_join_me_to(context->er_group_definition_ptr, &context_list, context->must_have_me, i, class_seq);
						} else {
							ret = er_me_group_instance_find_join_list_to(context->er_group_definition_ptr, &context_list, miptr[class_seq], i, class_seq);
						}
						//mark checked flag
						checked_flag[class_seq] = 1;
					}
				} else {
					ret = 0;
				}
				if (ret == 0)
				{
					er_me_group_instance_release_context_list(&context_list);
					return 0;
				}
			}
		}

		i++;
	}

	//translate context list to instance list
	list_for_each_entry(context_node_curr, &context_list, node)
	{
		//allocate a new er_group_instance node
		er_me_group_instance_new = malloc_safe(sizeof(struct er_me_group_instance_t));

		INIT_LIST_HEAD(&er_me_group_instance_new->er_attr_group_instance_list);

		error = 0;
		for (i = 0; i < ER_ME_GROUP_MEMBER_MAX && context->er_group_definition_ptr->classid[i] != 0; i++)
		{
			if (context_node_curr->context_me[i] != NULL)
			{
				er_me_group_instance_new->meid[i] = context_node_curr->context_me[i]->meid;
			} else {
				error = 1;
				break;	
			}
		}
		//add to instance list
		if (error)
		{
			free_safe(er_me_group_instance_new);
		} else {
			list_add_tail(&er_me_group_instance_new->er_me_group_instance_node, context->found_instance_list);
		}
	}

	//release context list
	er_me_group_instance_release_context_list(&context_list);

	return 1;
}

