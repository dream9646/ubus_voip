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
 * File    : meinfo.c
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
#include "fwk_mutex.h"
#include "meinfo.h"
#include "notify_chain.h"
#include "meinfo_cb.h"

struct meinfo_t *meinfo_array[MEINFO_INDEX_TOTAL];
static struct list_head me_gclist;
static struct fwk_mutex_t me_gclist_mutex;

char *
meinfo_util_strerror(int errnum)
{
	switch(errnum) {
		case MEINFO_ERROR_CLASSID_UNDEF:	return "class undefined";
		case MEINFO_ERROR_CLASSID_NOT_SUPPORT:	return "class not supported";

		case MEINFO_ERROR_ME_NOT_FOUND:		return "me not found";
		case MEINFO_ERROR_ME_PTR_NULL:		return "me ptr null";
		case MEINFO_ERROR_ME_STILL_IN_USE:	return "me still in use";
		case MEINFO_ERROR_ME_INSTANCE_FULL:	return "me instance full";
		case MEINFO_ERROR_ME_COPY_ON_WRITE:	return "me copy on write";

		case MEINFO_ERROR_ATTR_UNDEF:		return "attr undefined";
		case MEINFO_ERROR_ATTR_PTR_NULL:	return "attr ptr null";
		case MEINFO_ERROR_ATTR_READONLY:	return "attr readonly";
		case MEINFO_ERROR_ATTR_VALUE_INVALID:	return "attr value invalid";
		case MEINFO_ERROR_ATTR_FORMAT_UNKNOWN:	return "attr format unknow";
		case MEINFO_ERROR_ATTR_FORMAT_CONVERT:	return "attr format convert err";
		case MEINFO_ERROR_ATTR_INVALID_ACTION_ON_TABLE:		return "attr invalid action for table";
		case MEINFO_ERROR_ATTR_INVALID_ACTION_ON_NON_PM_ME:	return "attr invalid action for pm me";

		case MEINFO_ERROR_CB_ME_GET:		return "cb get err";
		case MEINFO_ERROR_CB_ME_SET:		return "cb set err";
		case MEINFO_ERROR_CB_ME_CREATE:		return "cb create err";
		case MEINFO_ERROR_CB_ME_DELETE:		return "cb delete err";
		case MEINFO_ERROR_CB_ME_ALARM:		return "cb alarm err";

		case MEINFO_ERROR_HW_ME_GET:		return "hw get err";
		case MEINFO_ERROR_HW_ME_SET:		return "hw set err";
		case MEINFO_ERROR_HW_ME_CREATE:		return "hw create err";
		case MEINFO_ERROR_HW_ME_DELETE:		return "hw delete err";
		case MEINFO_ERROR_HW_ME_ALARM:		return "hw alarm err";
		default:				return "unknown err";
	}
	return "?";
}
unsigned short 
meinfo_index2classid(unsigned short index)
{
	if (index < MEINFO_INDEX_TOTAL_OFFICIAL) {
		return index+MEINFO_CLASSID_OFFICIAL_START;
	} else if (	index >= MEINFO_INDEX_TOTAL_OFFICIAL && 
			index < MEINFO_INDEX_TOTAL ) {
		return index-MEINFO_INDEX_TOTAL_OFFICIAL+MEINFO_CLASSID_PROPRIETARY_START;
	} else {
		return 0;	// classid 0 is invalid
	}
}

unsigned short
meinfo_classid2index(unsigned short classid)
{
#if 0
	if (	classid>=MEINFO_CLASSID_OFFICIAL_START &&
		classid<=MEINFO_CLASSID_OFFICIAL_END) {
		return classid-MEINFO_CLASSID_OFFICIAL_START;
	} else if (	classid >= MEINFO_CLASSID_PROPRIETARY_START && 
			classid <= MEINFO_CLASSID_PROPRIETARY_END) {
		return classid - MEINFO_CLASSID_PROPRIETARY_START + MEINFO_INDEX_TOTAL_OFFICIAL;
	}
#endif
	if (classid<=MEINFO_CLASSID_OFFICIAL_END) {
		return classid-MEINFO_CLASSID_OFFICIAL_START;
	} else if (classid >= MEINFO_CLASSID_PROPRIETARY_START) {
		return classid - MEINFO_CLASSID_PROPRIETARY_START + MEINFO_INDEX_TOTAL_OFFICIAL;
	} else {
		return 0;	// out of range, wrap to 0, meinfo_array[0] is a null ptr
	}
}

struct meinfo_t *
meinfo_alloc_meinfo(void) 
{
	struct meinfo_t *miptr=malloc_safe(sizeof(struct meinfo_t));
	int i;
	
	// init list
	INIT_LIST_HEAD(&miptr->me_pointer_update_list);
	INIT_LIST_HEAD(&miptr->me_instance_list);
	INIT_LIST_HEAD(&miptr->me_mibupload_clone_list);
	INIT_LIST_HEAD(&miptr->me_alarm_clone_list);

	// init er_me_group_definition_ptr_list
	INIT_LIST_HEAD(&miptr->er_me_group_definition_ptr_list);	

	// init miptr->attrinfo[j]
	for (i = 0; i < MEINFO_ATTR_ORDER_MAX; i++) {
		struct attrinfo_t *a = &miptr->attrinfo[i];
		INIT_LIST_HEAD(&a->code_points_list);
	}

	// init meinfo_config
	INIT_LIST_HEAD(&miptr->config.meinfo_instance_def_list);
	INIT_LIST_HEAD(&miptr->config.meinfo_aliasid2meid_list);

	// init related ready class list
	INIT_LIST_HEAD(&miptr->related_ready_class_list);
	
	return miptr;
}

void
meinfo_preinit(void)
{
	int i;
	for (i = 0; i < MEINFO_INDEX_TOTAL; i++)
		meinfo_array[i]=NULL;
	INIT_LIST_HEAD(&me_gclist);
	fwk_mutex_create(&me_gclist_mutex);
}

short
meinfo_util_attr_get_table_entry_byte_size(unsigned short classid, unsigned short attr_order)
{
	struct meinfo_t *miptr=meinfo_util_miptr(classid);
	struct attrinfo_table_t *tab;

	if (miptr==NULL) {
		dbprintf(LOG_ERR, "%s, classid=%u\n", meinfo_util_strerror(MEINFO_ERROR_CLASSID_UNDEF), classid);
		return MEINFO_ERROR_CLASSID_UNDEF;	//class_id out of range
	}

	if (miptr->config.support == ME_SUPPORT_TYPE_NOT_SUPPORTED) {
		dbprintf(LOG_ERR, "%s, classid=%u\n",
			 meinfo_util_strerror(MEINFO_ERROR_CLASSID_NOT_SUPPORT), classid, attr_order);
		return MEINFO_ERROR_CLASSID_NOT_SUPPORT;
	}

	if (attr_order > miptr->attr_total) {
		dbprintf(LOG_ERR, "%s, classid=%u, attr_order=%u\n",
			 meinfo_util_strerror(MEINFO_ERROR_ATTR_UNDEF), classid, attr_order);
		return MEINFO_ERROR_ATTR_UNDEF;
	}

	tab = miptr->attrinfo[attr_order].attrinfo_table_ptr;
	if (tab == NULL) {	// attr is not a table  
		dbprintf(LOG_ERR, "%s, classid=%u, attr_order=%u\n",
			 meinfo_util_strerror(MEINFO_ERROR_ATTR_PTR_NULL), classid, attr_order);
		return MEINFO_ERROR_ATTR_PTR_NULL;
	}
	return (tab->entry_byte_size);
}

int
meinfo_util_caculate_cmds_num(unsigned short classid, unsigned char capacity)
{
	struct meinfo_t *miptr=meinfo_util_miptr(classid);
	int cmds_num = 0;
	int byte_accum = 0;
	int i;

	if (miptr==NULL) {
		dbprintf(LOG_ERR, "%s, classid=%u\n", meinfo_util_strerror(MEINFO_ERROR_CLASSID_UNDEF), classid);
		return 0;	//class_id out of range
	}

	for (i = 1; i <= miptr->attr_total; i++) {
		if (miptr->attrinfo[i].format != ATTR_FORMAT_TABLE
		    && miptr->attrinfo[i].byte_size <= capacity) {
			byte_accum += miptr->attrinfo[i].byte_size;
			if (byte_accum > capacity) {
				byte_accum = miptr->attrinfo[i].byte_size;
				cmds_num++;
			}
		}
	}

	//increase, even byte_accum == 0, all attributes were table
	cmds_num++;

	return cmds_num;
}

static struct timeval datasync_update_time;
int
meinfo_util_increase_datasync(void)
{
	struct me_t *me = meinfo_me_get_by_meid(2, 0);

	if (me == NULL) {
		dbprintf(LOG_ERR, "ME ONT-Data not found!!\n");
		return MEINFO_ERROR_ME_NOT_FOUND;
	}

	if (me->attr[1].value.data >= 255) {
		me->attr[1].value.data = 1;
	} else {
		me->attr[1].value.data++;
	}

	util_get_uptime(&datasync_update_time);

	return 0;
}
unsigned char
meinfo_util_get_data_sync(void)
{
	struct me_t *me = meinfo_me_get_by_meid(2, 0);

	if (me == NULL) {
		dbprintf(LOG_ERR, "ME ONT-Data not found!!\n");
		return 0;
	}
	return (unsigned char)(me->attr[1].value.data);
}
struct timeval *
meinfo_util_get_data_sync_update_time(void)
{
	return &datasync_update_time;
}

int
meinfo_util_load_alias_file(char *filename)
{
	FILE *fp=fopen(filename, "r");
	char *linebuff;
	int line=0;
	char *s0, *s1, *s2;
	
	if (fp==NULL) {
		dbprintf(LOG_ERR, "alias file %s open failed (%s)\n",
			filename, strerror(errno));
		return -1;
	}

	linebuff = malloc_safe(1024);
	while (fgets(linebuff, 1024, fp)) {
		struct meinfo_t *miptr;
		unsigned short classid;
		
		line++;

		s0=strstr(linebuff, "#");
		if (s0)
			*s0=0;	// terminate linebuff at #
		s0=util_trim(linebuff);		
		if (strlen(s0)<=1)
			continue;		
			
		s1=strstr(linebuff, "/");
		if (s1==NULL) {
			dbprintf(LOG_ERR, "parse err in line %d, lack of '/'?\n", line);
			continue;
		}
		*s1=0; s1++;
		
		s2=strstr(s1, "=");
		if (s2==NULL) {
			dbprintf(LOG_ERR, "parse err in line %d, lack of '='?\n", line);
			continue;
		}
		*s2=0; s2++;
		
		classid=util_atoi(s0);
		miptr=meinfo_util_miptr(classid);
		if (miptr) {
			unsigned short meid=util_atoi(util_trim(s1));
			unsigned short alias[128];
			int n, i;

			n=conv_numlist2array(s2, alias, sizeof(unsigned short), 128);
			for (i=0; i<n; i++) {
				struct meinfo_aliasid2meid_t *aliasid2meid = malloc_safe(sizeof (struct meinfo_aliasid2meid_t));
				aliasid2meid->aliasid=alias[i];
				aliasid2meid->meid=meid;
				list_add_tail(&aliasid2meid->aliasid2meid_node, &miptr->config.meinfo_aliasid2meid_list);
				dbprintf(LOG_DEBUG, "line %d: classid=%u, alias %u -> meid %u\n", 
					line, classid, aliasid2meid->aliasid, aliasid2meid->meid);
			}
		}
	}
	free_safe(linebuff);
	fclose(fp);
	return 0;
}

int
meinfo_util_aliasid2meid(unsigned short classid, unsigned short aliasid)
{
	struct meinfo_t *miptr=meinfo_util_miptr(classid);
	struct meinfo_aliasid2meid_t *aliasid2meid;
	if (miptr==NULL) {
		dbprintf(LOG_ERR, "%s, classid=%u\n", classid, meinfo_util_strerror(MEINFO_ERROR_CLASSID_UNDEF), classid);
		return -1;
	}
	
	list_for_each_entry(aliasid2meid, &miptr->config.meinfo_aliasid2meid_list, aliasid2meid_node) {
		if (aliasid2meid->aliasid==aliasid || aliasid2meid->aliasid==0xFFFF) { //65535 is wildcard
			dbprintf(LOG_WARNING, "classid=%u, aliasid 0x%x(%u) -> meid 0x%x(%u) found\n", 
				classid, 
				aliasid2meid->aliasid, aliasid2meid->aliasid, 
				aliasid2meid->meid, aliasid2meid->meid);
			return aliasid2meid->meid;
		}
	}
	return -1;
}

// move me from miptr instance list to gclist
int
meinfo_util_gclist_collect_me(struct me_t *me, int (*postfunc)(struct me_t *me))
{
	struct meinfo_gc_t *gc=malloc_safe(sizeof(struct meinfo_gc_t));

	if (me->instance_node.next == LIST_POISON1 ||
	    me->instance_node.prev == LIST_POISON2 ||
	    me->instance_node.next == NULL ||
	    me->instance_node.prev == NULL) {
	    	dbprintf(LOG_ERR, "me %u:0x%x(%u), is not on miptr instance list?\n", me->classid, me->meid, me->meid);
	} else {	
		__list_del(me->instance_node.prev, me->instance_node.next);
	}
	gc->me=me;
	gc->postfunc=postfunc;
	gc->time=util_get_uptime_sec();
	
	// put the later added node in the last
	fwk_mutex_lock(&me_gclist_mutex);
	list_add_tail(&gc->gc_node, &me_gclist);
	fwk_mutex_unlock(&me_gclist_mutex);
	dbprintf(LOG_INFO, "me %u:0x%x(%u) is moved to gclist\n", me->classid, me->meid, me->meid);
	
	return 0;
}

static long
meinfo_util_gclist_get_oldest_taskts()
{
	long oldest_taskts = 0, current_ts;
	int i;

	current_ts = util_get_uptime_sec();
	
	for (i = 0; i < ENV_SIZE_TASK; i++)
	{
		if (omci_env_g.taskstate[i] == 1 &&
			omci_env_g.taskts[i] > 0 &&
			(oldest_taskts == 0 ||
			omci_env_g.taskts[i] < oldest_taskts))
		{
			oldest_taskts = omci_env_g.taskts[i];
		}					
	}

	if (oldest_taskts != 0)
	{
		return oldest_taskts;
	} else {
		return current_ts;	
	}
}
	
int
meinfo_util_gclist_aging(int aging_interval)
{
	time_t aging_time=util_get_uptime_sec()-aging_interval;
	time_t oldest_taskts=meinfo_util_gclist_get_oldest_taskts();
	struct meinfo_gc_t *gc, *gc_n;
	int total=0;
	
	//dbprintf(LOG_INFO, "aging interval %d\n", aging_interval);
	fwk_mutex_lock(&me_gclist_mutex);
	list_for_each_entry_safe(gc, gc_n, &me_gclist, gc_node) {
		if (gc->time<=aging_time && gc->time<oldest_taskts) { //make sure all tasks not in this me node
		    	dbprintf(LOG_INFO, "me %u:0x%x(%u) is cleared from gclist\n", 
		    		gc->me->classid, gc->me->meid, gc->me->meid);
			list_del(&gc->gc_node);
			
			// UGLY check (for old time that mutex was not used)
			// we ecountered seg fault here that gc->me == LIST_POISON1 in very few times, 
			// but we didn't find the root cause, so use if block to workaround it temporarily
			if (gc->me==NULL || gc->me==LIST_POISON1 || gc->me==LIST_POISON2) {
				// this should not happen after mutext is used
			    	dbprintf(LOG_ERR, "Invalid me ptr 0x in gc%x\n", gc->me);
			} else {
				gc->me->instance_node.next = LIST_POISON1;
				gc->me->instance_node.prev = LIST_POISON2;
				if (gc->postfunc)
					gc->postfunc(gc->me);
			}

			free_safe(gc);
			total++;
		} else {
			// as the node in gc are sorted by time, 
			// so we can break the loop once we met any younger node
			break;
		}
	}
	fwk_mutex_unlock(&me_gclist_mutex);
	return total;
}

int
meinfo_util_gclist_dump(int fd, int is_detail)
{
	struct meinfo_gc_t *gc;
	time_t now = util_get_uptime_sec();
	int with_postfunc = 0;
	int without_postfunc = 0;
	int is_invalid = 0;
	
	fwk_mutex_lock(&me_gclist_mutex);
	list_for_each_entry(gc, &me_gclist, gc_node) {
		if (gc->me==NULL || gc->me==LIST_POISON1 || gc->me==LIST_POISON2) {
			if (is_detail)
			    	dbprintf(LOG_ERR, "Invalid me ptr 0x in gc%x\n", gc->me);
		    	is_invalid++;
		} else {
			if (is_detail)
				 util_fdprintf(fd, "me %u:0x%x(%u), postfunc=%p, age=%dsec\n", 
				 	gc->me->classid, gc->me->meid, gc->me->meid, 
				 	gc->postfunc, now - gc->time);
			if (gc->postfunc)
				with_postfunc++;
			else
				without_postfunc++;	
		}
	}	
	fwk_mutex_unlock(&me_gclist_mutex);

	util_fdprintf(fd, "total: %d (with_postfunc: %d, without_postfunc:%d, invalid:%d)\n",
		with_postfunc + without_postfunc + is_invalid, with_postfunc, without_postfunc, is_invalid);

	return is_invalid;
}

char *
meinfo_util_get_config_devname(struct me_t *me)
{
	struct meinfo_instance_def_t *instance_def_ptr;
	if (!me)
		return NULL;	
	instance_def_ptr = meinfo_instance_find_def(me->classid, me->instance_num);	
	if (instance_def_ptr == NULL || strlen(instance_def_ptr->devname) == 0) {
		dbprintf(LOG_NOTICE, "can not find devname, classid=%u, meid=0x%x\n", me->classid, me->meid);
		return NULL;
	}
	return instance_def_ptr->devname;
}

