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
 * File    : cli_config.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "util.h"
#include "meinfo.h"
#include "cli.h"

static int
cli_config_classid_instance(int fd, unsigned short classid, unsigned short instance)
{
	int i, j;
	struct meinfo_t *miptr;
	struct meinfo_instance_def_t *instance_def_ptr;
	struct attr_value_t attr;
	char ostr[128];

	miptr = meinfo_util_miptr(classid); 

	if (miptr != NULL && miptr->config.is_inited && miptr->config.support != ME_SUPPORT_TYPE_NOT_SUPPORTED)
	{
		attr.data=0;
		attr.ptr = malloc_safe(128);

		//show header
		util_fdprintf(fd, "id: %d\n", miptr->classid);
		util_fdprintf(fd, "name: %s\n", miptr->name?miptr->name:"?");
		//util_fdprintf(fd, "supported: %s\n", (str = conv_value2str(miptr->config.support, map_me_supported_config)) != NULL ? str : CLI_NONE_STRING);
		util_fdprintf(fd, "total instance max: %d\n", miptr->config.instance_total_max);

		list_for_each_entry(instance_def_ptr, &miptr->config.meinfo_instance_def_list, instance_node)
		{
			if (instance == 0xFFFF || instance == instance_def_ptr->instance_num)
			{
				util_fdprintf(fd, "instance:%d, meid:0x%.4x(%d)", 
					instance_def_ptr->instance_num,
					instance_def_ptr->default_meid,
					instance_def_ptr->default_meid);
				if (strlen(instance_def_ptr->devname)>0)
					util_fdprintf(fd, ", devname %s", instance_def_ptr->devname);
				if (instance_def_ptr->is_private)
					util_fdprintf(fd, ", private");
				util_fdprintf(fd, "\n");

				for (i = 1; i <= miptr->attr_total; i++)
				{
					memset(ostr, 0x00, sizeof(ostr));

					if (miptr->attrinfo[i].format == ATTR_FORMAT_TABLE) 
					{
						struct attr_table_entry_t *table_entry_node = NULL;
                				struct attr_table_header_t *table_header;

						util_fdprintf(fd,"%2d: %s:\n", i, miptr->attrinfo[i].name);

                				if ((table_header = instance_def_ptr->custom_default_value[i].ptr) != NULL)
						{
							j = 0;							
                					list_for_each_entry(table_entry_node, &table_header->entry_list, entry_node) {
                        					if (table_entry_node->entry_data_ptr != NULL) {
                                					memcpy(attr.ptr, table_entry_node->entry_data_ptr, miptr->attrinfo[i].attrinfo_table_ptr->entry_byte_size);
                                					meinfo_conv_attr_to_string(classid, i, &attr, ostr, 128);
                                					util_fdprintf(fd,"	%d: %s\n", j++, ostr);
                        					}
                					}
						} else {
							util_fdprintf(fd,"	table header null\n");
						}
					} else {
						meinfo_conv_attr_to_string(classid, i, &instance_def_ptr->custom_default_value[i], ostr, 128);
						util_fdprintf(fd,"%2d: %s: %s\n", i, miptr->attrinfo[i].name, ostr);
					}
				}
			}		
		}

		free_safe(attr.ptr);
	} else {
		util_fdprintf(fd, "No config for classid=%u\n", classid);
	}

	util_fdprintf(fd, "\n");

	return 0;
}

static int
cli_config(int fd, unsigned short classid, unsigned short instance)
{
	int i;
	struct meinfo_t *miptr;
	struct meinfo_instance_def_t *instance_def_ptr;

	if (classid != 0)
	{
		cli_config_classid_instance(fd, classid, instance);
	} else if (classid == 0 && instance != 0xFFFF) {
		for (i = 0; i < MEINFO_INDEX_TOTAL; i++)
		{
			miptr = meinfo_util_miptr(meinfo_index2classid(i)); 

			if (miptr == NULL)
				continue;

			if (miptr->config.is_inited && miptr->config.support != ME_SUPPORT_TYPE_NOT_SUPPORTED)
			{
				//find instance exist
				list_for_each_entry(instance_def_ptr, &miptr->config.meinfo_instance_def_list, instance_node)
				{
					if (instance_def_ptr->instance_num == instance)
					{
						cli_config_classid_instance(fd, meinfo_index2classid(i), instance);
						break;
					}
				}
			}
		}
	} else {
		for (i = 0; i < MEINFO_INDEX_TOTAL; i++)
		{
			miptr = meinfo_util_miptr(meinfo_index2classid(i)); 
			if (miptr == NULL)
				continue;

			if (miptr->config.is_inited && miptr->config.support != ME_SUPPORT_TYPE_NOT_SUPPORTED)
			{
				cli_config_classid_instance(fd, meinfo_index2classid(i), instance);
			}
		}
	}

	return 0;
}

static int
cli_config_summary(int fd)
{
	int i;
	int first;
	struct meinfo_t *miptr;
	struct meinfo_instance_def_t *instance_def_ptr;
	
	for (i = 0; i < MEINFO_INDEX_TOTAL; i++)
	{
		miptr = meinfo_util_miptr(meinfo_index2classid(i)); 

		if (miptr == NULL)
			continue;

		if (miptr->config.is_inited && miptr->config.support != ME_SUPPORT_TYPE_NOT_SUPPORTED)
		{
			util_fdprintf(fd, "%3d %s[%d]: ", miptr->classid, miptr->name?miptr->name:"?", miptr->config.instance_total_max);

			first = 1;
			list_for_each_entry(instance_def_ptr, &miptr->config.meinfo_instance_def_list, instance_node)
			{
				if (first)
				{
					util_fdprintf(fd, "0x%.4x(%d)", instance_def_ptr->default_meid, instance_def_ptr->default_meid);
					first = 0;
				} else {
					util_fdprintf(fd, ",0x%.4x(%d)", instance_def_ptr->default_meid, instance_def_ptr->default_meid);
				}
			}
			util_fdprintf(fd, "\n");
		}
	}

	return 0;
}


// helper function used by cli.c /////////////////////////////////////////////////////////////////////////////////////////////

void
cli_config_help(int fd)
{
	util_fdprintf(fd, 
		"config [classid]\n");
}

int
cli_config_cmdline(int fd, int argc, char *argv[])
{
	unsigned short classid=0, instance=0;

	if (strcmp(argv[0], "config")==0 && argc<=3) {
		if (argc==1) {
			return cli_config_summary(fd);
		} else {
			instance=0xffff;
			classid=util_atoi(argv[1]);
			if (argc==3)
				instance=util_atoi(argv[2]);
			return cli_config(fd, classid, instance);
		}
	} else {
		return CLI_ERROR_OTHER_CATEGORY;
	}	
}
