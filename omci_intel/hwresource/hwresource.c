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
 * Module  : hwresource
 * File    : hwresource.c
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
#include "notify_chain.h"
#include "meinfo_cb.h"
#include "hwresource.h"

struct list_head hwresource_relation_list;

int 
hwresource_preinit(void)
{
	INIT_LIST_HEAD(&hwresource_relation_list);
	return 0;
}

static struct hwresource_relation_t *
hwresource_find_by_public(unsigned short public_classid)
{
	struct hwresource_relation_t *relptr;
	list_for_each_entry(relptr, &hwresource_relation_list, hwresource_relation_node) {
		if (relptr->public_classid==public_classid)
			return relptr;
	}
	return NULL;
}

static struct hwresource_relation_t *
hwresource_find_by_private(unsigned short private_classid)
{
	struct hwresource_relation_t *relptr;
	list_for_each_entry(relptr, &hwresource_relation_list, hwresource_relation_node) {
		if (relptr->private_classid==private_classid)
			return relptr;
	}
	return NULL;
}

int
hwresource_is_related(unsigned short classid)
{
	struct hwresource_relation_t *relptr=hwresource_find_by_public(classid);
	if (relptr)
		return (relptr->private_classid);
	else
		return 0;	// no related private classid found
}

struct me_t *
hwresource_public2private(struct me_t *public_me)
{
	struct hwresource_relation_t *relptr;
	struct meinfo_t *miptr;
	struct me_t *me;
	struct attr_value_t attr1, attr2;

	if (public_me == NULL)
		return NULL;
	// find hwresource relation
	if ((relptr=hwresource_find_by_public(public_me->classid))==NULL) {
		dbprintf(LOG_DEBUG, // slin - LOG_ERR, 
			"public me (classid=%u, meid=%u) has no related private class\n", 
			public_me->classid, public_me->meid);
		return NULL;
	}

	miptr=meinfo_util_miptr(relptr->private_classid);
	// find private me	
	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		meinfo_me_attr_get(me, 1, &attr1);	// occupied
		meinfo_me_attr_get(me, 2, &attr2);	// tp pointer to public me
		if (attr1.data && (unsigned short)(attr2.data)==public_me->meid)
			return me;
	}
	
	dbprintf(LOG_INFO, "public me %u:0x%x(%u) has no related private me\n", 
		public_me->classid, public_me->meid, public_me->meid);	
	return NULL;
}	

struct me_t *
hwresource_private2public(struct me_t *private_me)
{
	struct hwresource_relation_t *relptr;
	struct attr_value_t attr;

	if (private_me == NULL)
		return NULL;
	// find hwresource relation
	if ((relptr=hwresource_find_by_private(private_me->classid))==NULL) {
		dbprintf(LOG_ERR, "private me %u:0x%x(%u) has no related public class\n", 
			private_me->classid, private_me->meid, private_me->meid);
		return NULL;
	}

	meinfo_me_attr_get(private_me, 2, &attr);	// tp pointer to public me

	return meinfo_me_get_by_meid(relptr->public_classid, (unsigned short)(attr.data));
}	

int 
hwresource_register(unsigned short public_classid, char *private_classname, 
	int (*match_funcptr)(struct me_t *public_me, struct me_t *private_me),
	int (*dismiss_funcptr)(struct me_t *public_me, struct me_t *private_me))
{
	struct hwresource_relation_t *relptr=hwresource_find_by_public(public_classid);
	int private_classid=0, i;

	if (relptr) {
		dbprintf(LOG_ERR, "classid %u already has hwresource relation to private classid %u\n", 
			public_classid, relptr->private_classid);
		return -1;
	}

	for (i = 0; i < MEINFO_INDEX_TOTAL; i++) {
		struct meinfo_t *miptr = meinfo_util_miptr(meinfo_index2classid(i)); 
		if (miptr && miptr->config.is_inited && miptr->config.support != ME_SUPPORT_TYPE_NOT_SUPPORTED) {
			if (strcmp(miptr->name, private_classname)==0) {
				private_classid=miptr->classid;
				break;
			}
		}
	}
	if (private_classid==0) {
		dbprintf(LOG_ERR, "private classname %s has no meinfo definition\n", private_classname);
		return -2;
	}
		
	relptr=(struct hwresource_relation_t*)malloc_safe(sizeof(struct hwresource_relation_t));

	// prepare content
	relptr->public_classid=public_classid;
	relptr->private_classid=private_classid;
	relptr->match_funcptr=match_funcptr;
	relptr->dismiss_funcptr=dismiss_funcptr;
	
	// add to list
	list_add_tail(&relptr->hwresource_relation_node, &hwresource_relation_list);
	return 0;
}
	
int
hwresource_unregister(unsigned short public_classid)
{ 
	struct hwresource_relation_t *relptr=hwresource_find_by_public(public_classid);
	if (relptr) {
		list_del(&relptr->hwresource_relation_node);
		free_safe(relptr);
	}
	return 0;
}
	

struct me_t*
hwresource_alloc(struct me_t *public_me)
{
	struct hwresource_relation_t *relptr;
	struct meinfo_t *miptr;
	struct me_t *me;
	struct attr_value_t attr;
	
	if (public_me==NULL) {
		dbprintf(LOG_ERR, "null public me ptr?\n");
		return NULL;
	}
	if ((relptr=hwresource_find_by_public(public_me->classid))==NULL) {
		dbprintf(LOG_ERR, "public me %u:0x%x(%u) has no related private class\n", 
			public_me->classid, public_me->meid, public_me->meid);
		return NULL;
	}

	miptr=meinfo_util_miptr(relptr->private_classid);

	// check if me already allocates private me	
	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		if (meinfo_util_me_attr_data(me, 1) &&
		    (unsigned short)meinfo_util_me_attr_data(me, 2)==public_me->meid) {
			dbprintf(LOG_WARNING, "public me %u:0x%x(%u) already has alloc hwresource me %u:0x%x(%u)\n", 
				public_me->classid, public_me->meid, public_me->meid, me->classid, me->meid, me->meid);	
			return me;
		}
	}
	
	// find unoccupied private me	
	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		// check attr1(occupied flag) for unoccupied private me
		if (meinfo_util_me_attr_data(me, 1)==0)	{// occupied flag
			unsigned short default_associated_meid=meinfo_util_me_attr_data(me, 3);
			int match=0;			
			if (default_associated_meid==0xffff) {
				if (relptr->match_funcptr)
				    	match=relptr->match_funcptr(public_me, me);
				else
					match=1;
			} else if (default_associated_meid==public_me->meid) {
				if (relptr->match_funcptr) {
				    	match=relptr->match_funcptr(public_me, me);
				    	if (!match)
						dbprintf(LOG_ERR, "hwresource me %u:0x%x(%u) has wrong default asociated meid?\n", 
							me->classid, me->meid, me->meid);
				} else {
					match=1;
				}
			}			    	
			if (match) {
				// set attr1(occupied_flag) to 1
				attr.data=1;
				meinfo_me_attr_set(me, 1, &attr, 0);
				// set attr2(associated pointer) to public meid
				attr.data=public_me->meid;
				meinfo_me_attr_set(me, 2, &attr, 0);
				dbprintf(LOG_INFO, "public me %u:0x%x(%u) alloc hwresource me %u:0x%x(%u)\n", 
					public_me->classid, public_me->meid, public_me->meid,
					me->classid, me->meid, me->meid);	
				return me;
			}
		}
	}
	
	dbprintf(LOG_ERR, "public me %u:0x%x(%u) can't alloc hwresource in class %u\n", 
		public_me->classid, public_me->meid, public_me->meid, relptr->private_classid);	
	return NULL;
}
	
int
hwresource_free(struct me_t *public_me)
{
	struct me_t *private_me;
	struct attr_value_t attr;		
	struct hwresource_relation_t *relptr;

	if (public_me==NULL) {
		dbprintf(LOG_ERR, "null public me ptr?\n");
		return -1;
	}
	if ((relptr=hwresource_find_by_public(public_me->classid))==NULL) {
		dbprintf(LOG_ERR, "public me %u:0x%x(%u) has no related private class\n", 
			public_me->classid, public_me->meid, public_me->meid);
		return -1;
	}

	private_me=hwresource_public2private(public_me);
	if (private_me==NULL)
		return -1;

	// clear hw state or attr in private_me
	if (relptr->dismiss_funcptr) {
		int ret=relptr->dismiss_funcptr(public_me, private_me);
		if (ret<=0)
			dbprintf(LOG_INFO, "hwresource me %u:0x%x(%u) dismiss err %d\n", 
				private_me->classid, private_me->meid, private_me->meid, ret);
	}

	// clear attr1(occupied_flag) to 0
	attr.data=0;
	meinfo_me_attr_set(private_me, 1, &attr, 0);

	// clear attr2(tppointer) to 0xffff
	attr.data=0xffff;
	meinfo_me_attr_set(private_me, 2, &attr, 0);

	dbprintf(LOG_INFO, "public me %u:0x%x(%u) free hwresource me %u:0x%x(%u)\n", 
		public_me->classid, public_me->meid, public_me->meid, 
		private_me->classid, private_me->meid, private_me->meid);

	return 0;
}

// return devname from hwresource private me related config
char *
hwresource_devname(struct me_t *public_me)
{	
	if (public_me) {
		// try private me if available
		struct me_t *private_me = hwresource_public2private(public_me);
		char *devname = meinfo_util_get_config_devname(private_me?private_me:public_me);
		return devname?devname:"";
	}
	return "";
}
