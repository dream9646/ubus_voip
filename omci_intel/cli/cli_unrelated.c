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
 * File    : cli_unrelated.c
 *
 ******************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <dirent.h>
#include <fcntl.h>

#include "util.h"
#include "meinfo.h"
#include "er_group.h"
#include "omciutil_vlan.h"
#include "cli.h"
#include "me_related.h"

struct me_t *me_map[1024];
int me_map_total;
int me_map_max;

static int
cli_unrelated_init_me_map(void)
{
	int index;

	me_map_total=0;
	for (index = 0; index < MEINFO_INDEX_TOTAL; index++) {
		unsigned short classid=meinfo_index2classid(index);
		struct meinfo_t *miptr = meinfo_util_miptr(classid); 
		if (miptr && !miptr->is_private) {
			struct me_t *me;
			list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
				if (!me->is_private) {
					me_map[me_map_total]=me;
					me_map_total++;
				}
			}
		}
	}
	me_map_max=me_map_total;
	return me_map_total;
}
	
static void
cli_unrelated_traverse_me_map_from_me(struct me_t *me_start)
{
	int i;
	
	if (me_start==NULL || me_map_total==0)
		return;

	for (i=0; i<me_map_max; i++) {
		if (me_map[i]) {
			if (me_start==me_map[i]) {
				me_map[i]=NULL; 
				me_map_total--;
			} else if (me_related(me_start, me_map[i]) || me_related(me_map[i], me_start)) {
				struct me_t *me_current=me_map[i];
				me_map[i]=NULL; 
				me_map_total--;
				cli_unrelated_traverse_me_map_from_me(me_current);
			}
		}
	}
}			
			
static int
cli_unrelated_traverse_me_map_from_classid(unsigned short classid)
{
	struct meinfo_t *miptr = meinfo_util_miptr(classid); 
	
	if (miptr) {
		struct me_t *me;
		list_for_each_entry(me, &miptr->me_instance_list, instance_node)
			cli_unrelated_traverse_me_map_from_me(me);
	}
	return 0;
}

int
cli_unrelated(int fd)
{
	int total=0, i, n;
	unsigned curr_classid=0;
	n=cli_unrelated_init_me_map();
	cli_unrelated_traverse_me_map_from_classid(45);
	cli_unrelated_traverse_me_map_from_classid(262);
	cli_unrelated_traverse_me_map_from_classid(53);
	
	for (i=0; i<n; i++) {
		if (me_map[i]) {
			total++;
			if (i==0 || me_map[i]->classid!=curr_classid) {
				if (curr_classid>0) 
					util_fdprintf(fd, "\n");
				curr_classid=me_map[i]->classid;
				util_fdprintf(fd, "%5u %s: 0x%x(%d)", me_map[i]->classid, 
					omcimsg_util_classid2name(me_map[i]->classid),
					me_map[i]->meid, me_map[i]->meid);
			} else {	
				util_fdprintf(fd, ",0x%x(%d)", 
					me_map[i]->meid, me_map[i]->meid);
			}
		}
	}
	if (total>0)
		util_fdprintf(fd, "\n");	
	util_fdprintf(fd, "unrelated me: %d\n", total);

	return CLI_OK;
}

// helper function used by cli.c /////////////////////////////////////////////////////////////////////////////////////////////

void
cli_unrelated_help(int fd)
{
}

int
cli_unrelated_cmdline(int fd, int argc, char *argv[])
{
	if (strcmp(argv[0], "unrelated")==0 || strcmp(argv[0], "un")==0) {
		return cli_unrelated(fd);
	} else {
		return CLI_ERROR_OTHER_CATEGORY;
	}	
}

