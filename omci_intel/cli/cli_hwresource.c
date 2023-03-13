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
 * Module  : cli
 * File    : cli_hwresource.c
 *
 ******************************************************************/

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "stdio.h"
#include "list.h"
#include "util.h"
#include "cli.h"
#include "meinfo.h"
#include "hwresource.h"

static int 
cli_hwresource_summary(int fd)
{
	struct hwresource_relation_t *relptr;

	list_for_each_entry(relptr, &hwresource_relation_list, hwresource_relation_node) {
		struct meinfo_t *public_miptr=meinfo_util_miptr(relptr->public_classid);
		struct meinfo_t *private_miptr=meinfo_util_miptr(relptr->private_classid);
		struct me_t *me;
		int i;

		if (public_miptr==NULL || private_miptr==NULL) {
			util_fdprintf(fd,
				"error hwresource entry, public_classid=%u, private_class=%u\n",
				relptr->public_classid, relptr->private_classid);
			return CLI_ERROR_INTERNAL;
		}
		util_fdprintf(fd, "%5u %s -> %5u %s\n", 
			relptr->public_classid, util_strptr_safe(public_miptr->name),
			relptr->private_classid, util_strptr_safe(private_miptr->name));
		
		// list occupied bitmap
		i=0;
		util_fdprintf(fd, "      alloc bitmap: ");
		list_for_each_entry(me, &private_miptr->me_instance_list, instance_node) {
			struct attr_value_t attr1;
			meinfo_me_attr_get(me, 1, &attr1);	// occupied
			util_fdprintf(fd, "%d", (attr1.data)?1:0);
			i++;
			if (i==8) {
				util_fdprintf(fd, " ");
				i=0;
			}
				
		}
		util_fdprintf(fd, "\n");			
	}
	return CLI_OK;	
}

static int 
cli_hwresource(int fd, unsigned short classid, unsigned short meid, int show_detail)
{
	struct hwresource_relation_t *relptr;

	list_for_each_entry(relptr, &hwresource_relation_list, hwresource_relation_node) {
		struct meinfo_t *public_miptr, *private_miptr;
		struct me_t *me;
		
		if (classid!=0 && 
			classid!=relptr->public_classid &&
			classid!=relptr->private_classid)
			continue;

		public_miptr=meinfo_util_miptr(relptr->public_classid);
		private_miptr=meinfo_util_miptr(relptr->private_classid);
		if (public_miptr==NULL || private_miptr==NULL) {
			util_fdprintf(fd,
				"error hwresource entry, public_classid=%u, private_class=%u\n",
				relptr->public_classid, relptr->private_classid);
			return CLI_ERROR_INTERNAL;
		}

/*
		util_fdprintf(fd, "%u %s -> %u %s\n", 
			relptr->public_classid, util_strptr_safe(public_miptr->name),
			relptr->private_classid, util_strptr_safe(private_miptr->name));
*/
		
		// list hw mapping for this class
		list_for_each_entry(me, &private_miptr->me_instance_list, instance_node) {		
			struct attr_value_t attr1;
			struct meinfo_instance_def_t *instance_def_ptr;
			char *devname="";

			if (meid && (classid==relptr->private_classid && meid!=me->meid))
				continue;

			instance_def_ptr = meinfo_instance_find_def(me->classid, me->instance_num);
			if (instance_def_ptr && strlen(instance_def_ptr->devname)>0)
				devname=instance_def_ptr->devname;

			meinfo_me_attr_get(me, 1, &attr1);	// occupied			
			if (attr1.data) {
				struct attr_value_t attr2;
				meinfo_me_attr_get(me, 2, &attr2);	// public meid
				if (meid && (classid==relptr->public_classid && meid!=attr2.data))
					continue;
				util_fdprintf(fd, "%u:0x%x(%u) -> %u:0x%x(%u) %s\n", 
					relptr->public_classid, (unsigned short)(attr2.data), (unsigned short)(attr2.data), 
					me->classid, me->meid, me->meid, devname);
				if (show_detail) {
					cli_me(fd, relptr->public_classid, (unsigned short)(attr2.data), 0);
					cli_me(fd, me->classid, me->meid, 0);
					util_fdprintf(fd, "\n"); 
				}					
			} else {
				if (meid && classid==relptr->public_classid)
					continue;
				util_fdprintf(fd, "free -> %u:0x%x(%u) %s\n", me->classid, me->meid, me->meid, devname);
				if (show_detail) {
					cli_me(fd, me->classid, me->meid, 0);
					util_fdprintf(fd, "\n"); 
				}
			}
		}
	}
	return CLI_OK;	
}

// helper function used by cli.c /////////////////////////////////////////////////////////////////////////////////////////////

void
cli_hwresource_help(int fd)
{
	util_fdprintf(fd, 
		"hwresource|hw [classid] [meid]\n");
}

int
cli_hwresource_cmdline(int fd, int argc, char *argv[])
{
	unsigned short classid=0, meid=0;
	
	if ((strcmp(argv[0], "hwresource")==0 || strcmp(argv[0], "hw")==0) && argc<=3) {
		if (argc==1) {
			return cli_hwresource_summary(fd);
		} else if (argc==2 && strcmp(argv[1], "help")==0) {
			cli_hwresource_help(fd);
			return 0;
		} else if (argc==2) {
			classid=util_atoi(argv[1]);
			return cli_hwresource(fd, classid, 0, 0);
		} else {
			classid=util_atoi(argv[1]);
			meid=util_atoi(argv[2]);
			return cli_hwresource(fd, classid, meid, 1);
		}

	} else {
		return CLI_ERROR_OTHER_CATEGORY;
	}	
}

