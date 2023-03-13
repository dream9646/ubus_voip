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
 * Module  : xmlparse
 * File    : xmlomci_er_group.c
 *
 ******************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <syslog.h>
#include <assert.h>

#include <arpa/inet.h>

#include "util.h"
#include "conv.h"
#include "env.h"
#include "xmlparse.h"
#include "xmlomci.h"
#include "meinfo.h"
#include "list.h"
#include "er_group.h"

struct xmlomci_er_group_context_t {
	unsigned short id;
	struct er_me_group_definition_t *er_me_group_definition;
	unsigned int er_me_relation_total;
	unsigned int er_me_relation_classid;
	struct er_attr_group_definition_t *er_attr_group_definition;
	unsigned int er_attr_total;
	unsigned int er_attr_classid;
};

// func used for config.xml ////////////////////////////////////////////////////////////////////

static int
xmlomci_er_group_add_tag(struct xmlparse_env_t *env, char *tag)
{
	struct xmlomci_er_group_context_t *x = (struct xmlomci_er_group_context_t *) env->dataptr;

	dbprintf_xml(LOG_DEBUG, "%d: url=%s\n", env->line, env->url);

	if (!strstr(env->url, "/omci/er_group"))	// only care url containing /omci/er_group
		return 0;

	// init context
	if (strcmp(env->url, "/omci/er_group") == 0) {	// reset x
		memset((void *) x, 0, sizeof (struct xmlomci_er_group_context_t));
		return 0;
	}

	if (strcmp(env->url, "/omci/er_group/er_me_group") == 0) {
		struct er_me_group_definition_t *megrp = malloc_safe(sizeof (struct er_me_group_definition_t));
		struct er_me_group_definition_t *megrp_pos, *megrp_n;
		struct list_head *insert_point;

		// init er_me_group_definition
		INIT_LIST_HEAD(&megrp->er_me_relation_list);
		INIT_LIST_HEAD(&megrp->er_me_group_instance_list);
		INIT_LIST_HEAD(&megrp->er_attr_group_definition_list);
		
		megrp->id = x->id;

		insert_point = NULL;
		//insert by id sequence
		list_for_each_entry_safe(megrp_pos, megrp_n, &er_me_group_definition_list, er_me_group_definition_node)
		{
			if (megrp_pos->id == megrp->id)
			{
				//remove and free, set insert pointer
				insert_point = megrp_pos->er_me_group_definition_node.prev;
				list_del(&megrp_pos->er_me_group_definition_node);
				free_safe(megrp_pos);
				break;
			} else if (megrp_pos->id > megrp->id) {
				//insert to the previous pointer
				insert_point = megrp_pos->er_me_group_definition_node.prev;
				break;
			} // else , continue
		}

		if (insert_point != NULL)
		{
			list_add(&megrp->er_me_group_definition_node, insert_point);
		} else {
			list_add_tail(&megrp->er_me_group_definition_node, &er_me_group_definition_list);
		}

		x->er_me_group_definition=megrp;
	} else if (strcmp(env->url, "/omci/er_group/er_attr_group") == 0) {
		struct er_attr_group_definition_t *agrp = malloc_safe(sizeof (struct er_attr_group_definition_t));

		// init er_attr_group_definition
		list_add_tail(&agrp->er_attr_group_definition_node, &x->er_me_group_definition->er_attr_group_definition_list);
		x->er_attr_group_definition=agrp;
		x->er_attr_total = 0;
	}
	return 0;
}

static int
xmlomci_er_group_add_attr(struct xmlparse_env_t *env, char *tag, char *attr, char *attrval)
{
	struct xmlomci_er_group_context_t *x = (struct xmlomci_er_group_context_t *) env->dataptr;

	dbprintf_xml(LOG_DEBUG, "%d: url=%s\n", env->line, env->url);

	if (!strstr(env->url, "/omci/er_group"))	// only care url containing /omci/er_group
		return 0;

	if (strcmp(env->url, "/omci/er_group") == 0) {
		if (strcmp(attr, "id") == 0) {
			x->id = util_atoi(attrval);
			return 0;
		}
	}

	// init er_me_group
	if (strcmp(env->url, "/omci/er_group/er_me_group") == 0) {
		if (strcmp(attr, "name") == 0) {
			xmlomci_loadstr(&x->er_me_group_definition->name, attrval);
			return 0;
		}
	} else if (strcmp(env->url, "/omci/er_group/er_me_group/er_me_relation") == 0) {
		if (strcmp(attr, "classid") == 0) {
			x->er_me_relation_classid = util_atoi(attrval);
			return 0;
		}

	} else  if (strcmp(env->url, "/omci/er_group/er_attr_group") == 0) {
		if (strcmp(attr, "name") == 0) {
			xmlomci_loadstr(&x->er_attr_group_definition->name, attrval);
			return 0;
		}
		if (strcmp(attr, "id") == 0) {
			x->er_attr_group_definition->id = util_atoi(attrval);
			return 0;
		}
	} else  if (strcmp(env->url, "/omci/er_group/er_attr_group/er_attr_list") == 0) {
		if (strcmp(attr, "classid") == 0) {
			x->er_attr_classid = util_atoi(attrval);
			return 0;
		}
	}

	return 0;
}

static int
xmlomci_er_group_finish_attrs(struct xmlparse_env_t *env, char *tag)
{
	//struct xmlomci_er_group_context_t *x = (struct xmlomci_er_group_context_t *) env->dataptr;

	//dbprintf_xml(LOG_DEBUG, "%d: url=%s\n", env->line, env->url);

	return 0;
}

static int
xmlomci_er_group_str2attrlist(char *str, unsigned char *attr_order_array, int array_size)
{
	char buff[1024];
	char *a[128];
	int n, i;
	
	strncpy(buff, str, 1024);
	buff[1023] = 0;
	n = conv_str2array(buff, ",", a, 128);
	if (n>array_size) 
		n=array_size;
		
	for (i = 0; i < n; i++) {
		attr_order_array[i]=(unsigned char)util_atoi(a[i]);
		if (attr_order_array[i]==0) {	// attr order should not be 0, meid or inst?
			if (strcmp(a[i], "meid")==0) {
				attr_order_array[i]=255;
			} else if (strcmp(a[i], "inst")==0) {
				attr_order_array[i]=254;
			} else {	// token not recognized?
				n = i;
				break;
			}
		}
	}
	return n;
}
static int
xmlomci_er_group_add_content(struct xmlparse_env_t *env, char *tag, char *s)
{
	struct xmlomci_er_group_context_t *x = (struct xmlomci_er_group_context_t *) env->dataptr;

	dbprintf_xml(LOG_DEBUG, "%d: url=%s\n", env->line, env->url);

	if (!strstr(env->url, "/omci/er_group"))	// only care url containing /omci/er_group
		return 0;

	if (strcmp(env->url, "/omci/er_group/er_me_group/er_me_relation") == 0) {
		struct er_me_relation_t *relptr = malloc_safe(sizeof (struct er_me_relation_t));
		struct meinfo_t *miptr;
		struct er_me_group_definition_ptr_node_t *ptrnode;

		// prepare the content of the er_me_relation
		list_add_tail(&relptr->er_me_relation_node, &x->er_me_group_definition->er_me_relation_list);
		relptr->classid=x->er_me_relation_classid;
		conv_numlist2array(s, relptr->referred_classid, sizeof(unsigned short), ER_ME_GROUP_MEMBER_MAX);
		
		// put this classid to classid[] in er_group_definition
		x->er_me_group_definition->classid[x->er_me_relation_total]=x->er_me_relation_classid;
		x->er_me_relation_total++;

		// have the er_me_group_definition linked by relation classid meinfo 
		if ((miptr = meinfo_util_miptr(x->er_me_relation_classid))==NULL) {
			dbprintf_xml(LOG_ERR, "%d: url=%s, megrp=%s, classid=%u, miptr null, class not found?\n", 
				env->line, env->url, util_strptr_safe(x->er_me_group_definition->name), x->er_me_relation_classid);
			return -1;
		}		
		ptrnode=malloc_safe(sizeof(struct er_me_group_definition_ptr_node_t));
		ptrnode->ptr=x->er_me_group_definition;
		list_add_tail(&ptrnode->node, &miptr->er_me_group_definition_ptr_list);

	} else if (strcmp(env->url, "/omci/er_group/er_attr_group/er_attr_list") == 0) {
		unsigned char attr_order_list[ER_ATTR_GROUP_MEMBER_MAX];
		int attr_total=xmlomci_er_group_str2attrlist(s, attr_order_list, ER_ATTR_GROUP_MEMBER_MAX);
		int i;
		
		for (i=0; i<attr_total; i++) {			
			if (x->er_attr_total<ER_ATTR_GROUP_MEMBER_MAX) {
				struct er_attr_t *era=&x->er_attr_group_definition->er_attr[x->er_attr_total];
				era->classid=x->er_attr_classid;
				era->attr_order=attr_order_list[i];
				x->er_attr_total++;			
			} else {
				dbprintf_xml(LOG_ERR, "%d: url=%s, megrp=%s, agrp=%s, attrlist full, [classid=%u, attr_order=%u] skipped\n", 
					env->line, env->url, 
					util_strptr_safe(x->er_me_group_definition->name), 
					util_strptr_safe(x->er_attr_group_definition->name), 
					x->er_attr_classid, attr_order_list[i]);
				return -1;
			}
		}	
	}
	return 0;
}

static int
xmlomci_er_group_finish_tag(struct xmlparse_env_t *env, char *tag)
{
	struct xmlomci_er_group_context_t *x = (struct xmlomci_er_group_context_t *) env->dataptr;

	dbprintf_xml(LOG_DEBUG, "%d: url=%s\n", env->line, env->url);

	if (!strstr(env->url, "/omci/er_group"))	// only care url containing /omci/er_group
		return 0;

	if (strcmp(env->url, "/omci/er_group") == 0) {	// reset x
		memset((void *) x, 0, sizeof (struct xmlomci_er_group_context_t));
	}

	return 0;
}

int
xmlomci_er_group_init(char *er_group_xml_filename)
{
	struct xmlomci_er_group_context_t x;
	struct xmlparse_env_t env;

	env.dataptr = (void *) &x;
	env.add_tag = xmlomci_er_group_add_tag;
	env.add_attr = xmlomci_er_group_add_attr;
	env.finish_attrs = xmlomci_er_group_finish_attrs;
	env.add_content = xmlomci_er_group_add_content;
	env.finish_tag = xmlomci_er_group_finish_tag;

	memset((void *) &x, 0, sizeof (struct xmlomci_er_group_context_t));
	return xmlparse_parse(er_group_xml_filename, &env);
}
