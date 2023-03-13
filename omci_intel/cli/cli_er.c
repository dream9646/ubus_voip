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
 * File    : cli_er.c
 *
 ******************************************************************/

#include <stdio.h>
#include <string.h>

#include "util.h"
#include "conv.h"
#include "xmlomci.h"
#include "meinfo.h"
#include "cli.h"
#include "er_group.h"

extern struct conv_str2val_t map_me_attr_format[];
extern struct conv_str2val_t map_me_attr_vformat[];

static int
cli_er_id(int fd, unsigned short id)
{
	int j, flag = 0;
	struct er_me_group_definition_t *er_me_group_definition;
	struct er_me_relation_t *er_me_relation;
	struct er_attr_group_definition_t *er_attr_group_definition;

	list_for_each_entry(er_me_group_definition, &er_me_group_definition_list, er_me_group_definition_node)
	{
		if (id == 0 || er_me_group_definition->id == id)
		{
			//show this definition
			util_fdprintf(fd, "id: %u\n", er_me_group_definition->id);
			util_fdprintf(fd, "name: %s\n", er_me_group_definition->name ? er_me_group_definition->name : "?");
			util_fdprintf(fd, "relation class:\n");
			list_for_each_entry(er_me_relation, &er_me_group_definition->er_me_relation_list, er_me_relation_node)
			{
				util_fdprintf(fd, " %d", er_me_relation->classid);
				for (j = 0; j < ER_ME_GROUP_MEMBER_MAX && er_me_relation->referred_classid[j] != 0; j++)
				{
					util_fdprintf(fd, "%s%u", (j>0)?",":"->", er_me_relation->referred_classid[j]);
				}
				util_fdprintf(fd, "\n");
			}
			util_fdprintf(fd, "attr group:\n");
			list_for_each_entry(er_attr_group_definition, &er_me_group_definition->er_attr_group_definition_list, er_attr_group_definition_node)
			{
				util_fdprintf(fd, "%2u: %s: ", er_attr_group_definition->id, er_attr_group_definition->name);
				for (j = 0; j < ER_ATTR_GROUP_MEMBER_MAX && er_attr_group_definition->er_attr[j].classid != 0; j++)
				{
					util_fdprintf(fd, "%s%u@%u", (j>0)?",":"",
						er_attr_group_definition->er_attr[j].classid, er_attr_group_definition->er_attr[j].attr_order);
				}
				util_fdprintf(fd, "\n");
				
				for (j = 0; j < ER_ATTR_GROUP_MEMBER_MAX && er_attr_group_definition->er_attr[j].classid != 0; j++)
				{
					struct attrinfo_t *aiptr;
					char *str;

					switch(er_attr_group_definition->er_attr[j].attr_order) {
					case 0:
					case 255:
						//meid
						util_fdprintf(fd, "         %u@%u(meid): 2byte,unsigned integer\n", 
							er_attr_group_definition->er_attr[j].classid,
							er_attr_group_definition->er_attr[j].attr_order);
						break;
					case 254:
						//instance num
						util_fdprintf(fd, "         %u@%u(instance_num): 2byte,unsigned integer\n", 
							er_attr_group_definition->er_attr[j].classid,
							er_attr_group_definition->er_attr[j].attr_order);
						break;
					default:
						aiptr=meinfo_util_aiptr(er_attr_group_definition->er_attr[j].classid, er_attr_group_definition->er_attr[j].attr_order);
						util_fdprintf(fd, "         %u@%u(%s): ", er_attr_group_definition->er_attr[j].classid,
								er_attr_group_definition->er_attr[j].attr_order,
								aiptr->name);
						if (aiptr->format != ATTR_FORMAT_TABLE || aiptr->attrinfo_table_ptr == NULL) {
							util_fdprintf(fd, "%dbyte", aiptr->byte_size);
						} else {
							util_fdprintf(fd, "%dbyte", aiptr->attrinfo_table_ptr->entry_byte_size);
						}			
						util_fdprintf(fd, ",%s", (str = conv_value2str(aiptr->format, map_me_attr_format)) != NULL ? str : "?");
						if (aiptr->vformat != 0) {
							util_fdprintf(fd, "-%s", (str = conv_value2str(aiptr->vformat, map_me_attr_vformat)) != NULL ? str : "?");
						}
						util_fdprintf(fd, "\n");
					}
				}
				
			}

			util_fdprintf(fd, "\n");

			flag = 1;

			if (id != 0)
			{
				break;
			}
		}
	}

	if (!flag)
	{
		util_fdprintf(fd, "No definition for id=%u\n", id);
	}

	return CLI_OK;
}

static int 
cli_er_summary(int fd, char *str)
{
	struct er_me_group_definition_t *er_me_group_definition;

	list_for_each_entry(er_me_group_definition, &er_me_group_definition_list, er_me_group_definition_node)
	{
		if (str == NULL || strstr(er_me_group_definition->name, str) != NULL)
		{
			util_fdprintf(fd, "%5u %s\n", er_me_group_definition->id, er_me_group_definition->name ? er_me_group_definition->name : "?");
		}
	}

	return CLI_OK;
}

static int
cli_eri_id(int fd, unsigned short id)
{
	int i = 0, j, flag = 0;
	struct er_me_group_definition_t *er_me_group_definition;
	struct er_me_group_instance_t *er_me_group_instance;

	list_for_each_entry(er_me_group_definition, &er_me_group_definition_list, er_me_group_definition_node)
	{
		if (id == 0 || er_me_group_definition->id == id)
		{
			util_fdprintf(fd, "id: %u\n", er_me_group_definition->id);
			util_fdprintf(fd, "name: %s\n", er_me_group_definition->name ? er_me_group_definition->name : "?");
			util_fdprintf(fd, "instance:\n");
			list_for_each_entry(er_me_group_instance, &er_me_group_definition->er_me_group_instance_list, er_me_group_instance_node)
			{
				//show this definition
				util_fdprintf(fd, " %u: ", i++);
				for (j = 0; j < ER_ME_GROUP_MEMBER_MAX && er_me_group_definition->classid[j] != 0; j++)
				{
					util_fdprintf(fd, "%s%u:0x%x", (j>0)?",":"",
						er_me_group_definition->classid[j], er_me_group_instance->meid[j]);
				}
				util_fdprintf(fd, "\n");
			}

			util_fdprintf(fd, "\n");

			flag = 1;

			if (id != 0)
			{
				break;
			}
		}
	}

	if (!flag)
	{
		util_fdprintf(fd, "No definition and instance for id=%u\n", id);
	}

	return CLI_OK;
}

static int 
cli_eri_summary(int fd, char *str)
{
	int count;

	struct er_me_group_definition_t *er_me_group_definition;
	struct list_head *er_me_group_instance_node;

	list_for_each_entry(er_me_group_definition, &er_me_group_definition_list, er_me_group_definition_node)
	{
		if (str == NULL || strstr(er_me_group_definition->name, str) != NULL)
		{
			count = 0;

			//count the instance number
			list_for_each(er_me_group_instance_node, &er_me_group_definition->er_me_group_instance_list)
			{
				count++;
			}

			util_fdprintf(fd, "%5u %s[%u]\n", er_me_group_definition->id, er_me_group_definition->name ? er_me_group_definition->name : "?", count);
		}
	}

	return CLI_OK;
}

static int 
cli_erattri_id(int fd, unsigned short id, unsigned short attrid, unsigned short iid)
{
	int i, j, flag = 0;

	struct er_me_group_definition_t *er_me_group_definition;
	struct er_me_group_instance_t *er_me_group_instance;
	struct er_attr_group_definition_t *er_attr_group_definition;
	struct er_attr_group_instance_t *er_attr_group_instance;

	list_for_each_entry(er_me_group_definition, &er_me_group_definition_list, er_me_group_definition_node)
	{
		if (id == 0 || er_me_group_definition->id == id)
		{
			util_fdprintf(fd, "id: %u\n", er_me_group_definition->id);
			util_fdprintf(fd, "name: %s\n", er_me_group_definition->name ? er_me_group_definition->name : "?");
			util_fdprintf(fd, "er attr group instance:\n");

			//count the instance number
			list_for_each_entry(er_attr_group_definition, &er_me_group_definition->er_attr_group_definition_list, er_attr_group_definition_node)
			{
				if (attrid == 0xFFFF || er_attr_group_definition->id == attrid)
				{
					util_fdprintf(fd, "%5d %s:\n", er_attr_group_definition->id, er_attr_group_definition->name ? er_attr_group_definition->name : "?");
					i = 0;
					list_for_each_entry(er_me_group_instance, &er_me_group_definition->er_me_group_instance_list, er_me_group_instance_node)
					{
						list_for_each_entry(er_attr_group_instance, &er_me_group_instance->er_attr_group_instance_list, next_er_attr_group_instance_node)
						{
							//check er_attr_group_definition_ptr
							if (er_attr_group_instance->er_attr_group_definition_ptr == er_attr_group_definition)
							{
								if (iid == 0xFFFF)
								{
									util_fdprintf(fd, "%7d ", i++);
									for (j = 0; j < ER_ATTR_GROUP_MEMBER_MAX && er_attr_group_definition->er_attr[j].classid != 0; j++)
									{
										util_fdprintf(fd, "%s%u:0x%x@%u", (j>0)?",":"",
											er_attr_group_definition->er_attr[j].classid,
											er_attr_group_instance->er_attr_instance[j].meid,
											er_attr_group_definition->er_attr[j].attr_order);
									}
									if (er_attr_group_instance->exec_result==0)
									{
										util_fdprintf(fd, " %c\n", '+');
									} else if (er_attr_group_instance->exec_result==ER_ATTR_GROUP_HW_RESULT_UNREASONABLE) {
										util_fdprintf(fd, " %c\n", '?');
									} else {
										util_fdprintf(fd, " %c\n", '-');
									}
								} else if (iid == i) {
									//show detail one
									util_fdprintf(fd, "       detail attr instance %d:\n", iid);
									for (j = 0; j < ER_ATTR_GROUP_MEMBER_MAX && er_attr_group_definition->er_attr[j].classid != 0; j++)
									{
										char buff[1024];
										struct attrinfo_t *aiptr;

										switch(er_attr_group_definition->er_attr[j].attr_order)
										{
										case 0:
										case 255:
											//meid
											util_fdprintf(fd, "         %u:0x%x@%u(meid): 0x%x\n", 
												er_attr_group_definition->er_attr[j].classid,
												er_attr_group_instance->er_attr_instance[j].meid,
												er_attr_group_definition->er_attr[j].attr_order,
												er_attr_group_instance->er_attr_instance[j].meid);
											break;
										case 254:
											//instance num
											util_fdprintf(fd, "         %u:0x%x@%u(instance_num): %u\n", 
												er_attr_group_definition->er_attr[j].classid,
												er_attr_group_instance->er_attr_instance[j].meid,
												er_attr_group_definition->er_attr[j].attr_order,
												meinfo_instance_meid_to_num(er_attr_group_definition->er_attr[j].classid, er_attr_group_instance->er_attr_instance[j].meid));
											break;
										default:
											aiptr=meinfo_util_aiptr(er_attr_group_definition->er_attr[j].classid, er_attr_group_definition->er_attr[j].attr_order);
											util_fdprintf(fd, "         %u:0x%x@%u(%s): ", er_attr_group_definition->er_attr[j].classid,
													er_attr_group_instance->er_attr_instance[j].meid,
													er_attr_group_definition->er_attr[j].attr_order,
													aiptr->name);

											if (aiptr->format == ATTR_FORMAT_TABLE) {
												struct attr_table_header_t *tab_header = er_attr_group_instance->er_attr_instance[j].attr_value.ptr;						       
												struct attr_table_entry_t *tab_entry;
												int k=0;

												util_fdprintf(fd, "(table)\n", buff);
												list_for_each_entry(tab_entry, &tab_header->entry_list, entry_node) {
													if (tab_entry->entry_data_ptr != NULL) {
														struct attr_value_t attr;
														attr.data=0;
														attr.ptr=tab_entry->entry_data_ptr;	// use attr to encapsulate entrydata for temporary use
														meinfo_conv_attr_to_string(er_attr_group_definition->er_attr[j].classid, er_attr_group_definition->er_attr[j].attr_order, &attr, buff, 1024);
														util_fdprintf(fd, "\t%d: %s\n", k, buff);
														k++;
													}
												}
											} else {
												meinfo_conv_attr_to_string(er_attr_group_definition->er_attr[j].classid, er_attr_group_definition->er_attr[j].attr_order, &er_attr_group_instance->er_attr_instance[j].attr_value, buff, 1024);
												util_fdprintf(fd, "%s\n", buff);
											}
										}
									}
									util_fdprintf(fd, "\n");
									i++;
								} else {
									i++;
								}
							}
						}
					}
				}
			}

			flag = 1;

			if (id != 0)
			{
				break;
			}
		}
	}

	if (!flag)
	{
		util_fdprintf(fd, "No definition and instance for id=%u\n", id);
	}

	return CLI_OK;
}

static int
cli_erattri_time_reset(int fd)
{
	struct er_me_group_definition_t *er_me_group_definition;
	struct er_attr_group_definition_t *er_attr_group_definition;

	list_for_each_entry(er_me_group_definition, &er_me_group_definition_list, er_me_group_definition_node)
	{
		//count the instance number
		list_for_each_entry(er_attr_group_definition, &er_me_group_definition->er_attr_group_definition_list, er_attr_group_definition_node)
		{
			er_attr_group_definition->add_time_diff = 0;
			er_attr_group_definition->del_time_diff = 0;
		}
	}
	return 0;
}

static int 
cli_erattri_summary(int fd, char *str)
{
	int success_count, fail_count, unreasonable_count;

	struct er_me_group_definition_t *er_me_group_definition;
	struct er_me_group_instance_t *er_me_group_instance;
	struct er_attr_group_definition_t *er_attr_group_definition;
	struct er_attr_group_instance_t *er_attr_group_instance;
	unsigned int total_add_time = 0, total_del_time = 0;

	list_for_each_entry(er_me_group_definition, &er_me_group_definition_list, er_me_group_definition_node)
	{
		if (str == NULL || strstr(er_me_group_definition->name, str) != NULL)
		{
			util_fdprintf(fd, "%5u %s:\n", er_me_group_definition->id, er_me_group_definition->name ? er_me_group_definition->name : "?");

			//count the instance number
			list_for_each_entry(er_attr_group_definition, &er_me_group_definition->er_attr_group_definition_list, er_attr_group_definition_node)
			{
				success_count= 0;
				fail_count= 0;
				unreasonable_count= 0;
				list_for_each_entry(er_me_group_instance, &er_me_group_definition->er_me_group_instance_list, er_me_group_instance_node)
				{
					list_for_each_entry(er_attr_group_instance, &er_me_group_instance->er_attr_group_instance_list, next_er_attr_group_instance_node)
					{
						//check er_attr_group_definition_ptr
						if (er_attr_group_instance->er_attr_group_definition_ptr == er_attr_group_definition)
						{
							if (er_attr_group_instance->exec_result==0)
							{
								success_count++;
							} else if (er_attr_group_instance->exec_result==ER_ATTR_GROUP_HW_RESULT_UNREASONABLE) {
								unreasonable_count++;
							} else {
								fail_count++;
							}
						}
					}
				}
				util_fdprintf(fd, "	[%d+", success_count);
				if( fail_count > 0 )
				{
					util_fdprintf(fd, ", %d-", fail_count);
				}
				if( unreasonable_count > 0 )
				{
					util_fdprintf(fd, ", %d?", unreasonable_count);
				}
				util_fdprintf(fd, "] %d %s", er_attr_group_definition->id, er_attr_group_definition->name ? er_attr_group_definition->name : "?");
				if (er_attr_group_definition->add_time_diff>0 || er_attr_group_definition->del_time_diff>0) {
					util_fdprintf(fd, " (%u, %u)\n", er_attr_group_definition->add_time_diff , er_attr_group_definition->del_time_diff );
					total_add_time += er_attr_group_definition->add_time_diff;
					total_del_time += er_attr_group_definition->del_time_diff;
				} else {
					util_fdprintf(fd, "\n");
				}
			}
		}
	}
	if (total_add_time || total_del_time)
		util_fdprintf(fd, "total time: %u us (add:%u, del:%u)\n", total_add_time+total_del_time, total_add_time, total_del_time );

	return CLI_OK;
}

// helper function used by cli.c /////////////////////////////////////////////////////////////////////////////////////////////

void
cli_er_help(int fd)
{
	util_fdprintf(fd, 
		"er|eri [erid]\n"
		"erattri [erid] [erattrid]\n"
		"erattri_time_reset\n");
}

int
cli_er_cmdline(int fd, int argc, char *argv[])
{
	unsigned short classid=0, instance=0, meid=0;

	if (strcmp(argv[0], "er")==0 && argc<=2) {
		if (argc==1) {
			return cli_er_summary(fd, NULL);
		}else {
			classid=util_atoi(argv[1]);
			return cli_er_id(fd, classid);
		}

	} else if (strcmp(argv[0], "er_find")==0 && argc<=2) {
		if (argc==1) {
			return cli_er_summary(fd, NULL);
		} else {
			return cli_er_summary(fd, argv[1]);
		}

	} else if (strcmp(argv[0], "eri")==0 && argc<=2) {
		if (argc==1) {
			return cli_eri_summary(fd, NULL);
		} else {
			classid=util_atoi(argv[1]);
			return cli_eri_id(fd, classid);
		}

	} else if (strcmp(argv[0], "eri_find")==0 && argc<=2) {
		if (argc==1) {
			return cli_eri_summary(fd, NULL);
		} else {
			return cli_eri_summary(fd, argv[1]);
		}

	} else if (strcmp(argv[0], "erattri")==0 && argc<=4) {
		if (argc==1) {
			return cli_erattri_summary(fd, NULL);
		} else {
			meid=0xffff;
			instance=0xffff;
			if (argc==2) {
				classid=util_atoi(argv[1]);
			}
			if (argc==3) {
				classid=util_atoi(argv[1]);
				if (argv[2][0] != '*') {
					meid=util_atoi(argv[2]);
				}
			}
			if (argc==4) {
				classid=util_atoi(argv[1]);
				if (argv[2][0] != '*') {
					meid=util_atoi(argv[2]);
				}
				if (argv[3][0] != '*') {
					instance=util_atoi(argv[3]);
				}
			}
			return cli_erattri_id(fd, classid, meid, instance);
		}
	} else if (strcmp(argv[0], "erattri_find")==0 && argc<=2) {
		if (argc==1) {
			return cli_erattri_summary(fd, NULL);
		} else {
			return cli_erattri_summary(fd, argv[1]);
		}

	} else if (strcmp(argv[0], "erattri_time_reset") == 0) {
			return cli_erattri_time_reset(fd);
	} else {
		return CLI_ERROR_OTHER_CATEGORY;
	}
}

