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
 * File    : cli_me.c
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
#include "me_related.h"
#include "hwresource.h"
#include "coretask.h"
#include "cli.h"

static int
me_is_ready(struct meinfo_t *miptr, struct me_t *me)
{
	if (miptr->callback.is_ready_cb && miptr->callback.is_ready_cb(me)==0)
		return 0;
	return 1;
}


static int 
cli_me_create(int fd, unsigned short classid, unsigned short meid)
{
	struct meinfo_t *miptr=meinfo_util_miptr(classid);
	short instance_num;
	struct me_t *me;

	if (miptr == NULL) {
		util_fdprintf(fd,"err=%d, classid=%u, miptr null?\n",
			CLI_ERROR_RANGE, classid);
		return CLI_ERROR_RANGE;
	}

	if (meinfo_me_is_exist(classid, meid) ) {
		util_fdprintf(fd,"err=%d, classid=%u, meid=0x%x(%u), me already exist?\n",
			CLI_ERROR_RANGE, classid, meid, meid);
		return CLI_ERROR_RANGE;
	}

	if ((instance_num = meinfo_instance_alloc_free_num(classid)) < 0) {
		util_fdprintf(fd,"err=%d, classid=%u, meid=0x%x(%u), too many instance?\n",
			CLI_ERROR_RANGE, classid, meid, meid);
		return CLI_ERROR_RANGE;
	}

	me = meinfo_me_alloc(classid, instance_num);
	if (!me) {
		util_fdprintf(fd,"err=%d, classid=%u, meid=0x%x(%u), me alloc err?\n",
			CLI_ERROR_INTERNAL, classid, meid, meid);
		return CLI_ERROR_INTERNAL;
	}

	// set default from config
	meinfo_me_load_config(me);

	// assign meid
	me->meid = meid;

	//create by olt
	me->is_cbo = 1;

	if (meinfo_me_create(me) < 0) {
		meinfo_me_release(me);
		util_fdprintf(fd,"err=%d, classid=%u, meid=0x%x(%u), me create err?\n",
			CLI_ERROR_INTERNAL, classid, meid, meid);
		return CLI_ERROR_INTERNAL;
	}

	return CLI_OK;
}

static int 
cli_me_delete(int fd, unsigned short classid, unsigned short meid)
{
	struct meinfo_t *miptr=meinfo_util_miptr(classid);
	struct me_t *me;

	if (miptr == NULL) {
		util_fdprintf(fd,"err=%d, classid=%u, miptr null?\n",
			CLI_ERROR_RANGE, classid);
		return CLI_ERROR_RANGE;
	}

	me=meinfo_me_get_by_meid(classid, meid);
	if (me==NULL ) {
		util_fdprintf(fd,"err=%d, classid=%u, meid=0x%x(%u), me not exist?\n",
			CLI_ERROR_RANGE, classid, meid, meid);
		return CLI_ERROR_RANGE;
	}

	if (meinfo_me_delete_by_instance_num(classid, me->instance_num)<0)
		return CLI_ERROR_INTERNAL;

	return CLI_OK;
}

static int
cli_me_print_pm_value(struct attrinfo_t *aiptr, struct attr_value_t *attr, char *str, int str_size)
{
	int ret=0;

	switch(aiptr->format) {
	case ATTR_FORMAT_UNSIGNED_INT:
		ret=snprintf(str, str_size, "%llu", attr->data);
		break;
	case ATTR_FORMAT_SIGNED_INT:
		if (aiptr->byte_size==1) {
			ret=snprintf(str, str_size, "%d", (char)attr->data);
		} else if (aiptr->byte_size==2) {
			ret=snprintf(str, str_size, "%d", (short)attr->data);
		} else if (aiptr->byte_size<=4) {
			ret=snprintf(str, str_size, "%d", (int)attr->data);
		} else {
			ret=snprintf(str, str_size, "%lld", attr->data);
		}
		break;
	default:
		ret=snprintf(str, str_size, "0x%llx(%lld)", (unsigned long long)attr->data, attr->data);
	}

	return ret;
}

int 
cli_me(int fd, unsigned short classid, unsigned short meid, unsigned char attr_order)
{
	unsigned char get_attr_mask[2] = {0xff, 0xff};
	char buff[1024];
	int total_found=0;
	int index;

	for (index = 0; index < MEINFO_INDEX_TOTAL; index++) {
		unsigned short classid2=meinfo_index2classid(index);
		struct meinfo_t *miptr = meinfo_util_miptr(classid2); 
		unsigned short i;
		struct me_t *me;
		int is_classid_printed=0;
		int is_classid_pm=0;
		int found=0;

		if (classid!=0 && classid!=classid2)
			continue;
		if (miptr==NULL)
			continue;

		if (miptr->name && strstr(miptr->name, "_PM")) {
			struct attrinfo_t *aiptr1=meinfo_util_aiptr(miptr->classid, 1);
			struct attrinfo_t *aiptr2=meinfo_util_aiptr(miptr->classid, 2);
			if (miptr->attr_total>=3 &&
			    strcmp(aiptr1->name, "Interval_end_time")==0 &&
			    strcmp(aiptr2->name, "Threshold_data_1/2_id")==0) { 
                            
			    	is_classid_pm=1;
			}

			if (miptr->attr_total>=3 &&
			    strcmp(aiptr1->name, "Interval_end_time")==0 &&
			    strcmp(aiptr2->name, "Control_Block")==0) {
                                if (aiptr2->attrinfo_bitfield_ptr != NULL)
                                {
                                        if (strcmp(aiptr2->attrinfo_bitfield_ptr->field[0].name, "Threshold_data_3_id")==0)
                                                is_classid_pm=1;
                                }
			}                        
                        
		}

		list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
			struct meinfo_instance_def_t *instance_def_ptr = NULL;
			int mapped_meid=meinfo_util_aliasid2meid(classid, meid);

			if (meid != 0xffff && meid!=me->meid && mapped_meid!=0xffff)
				continue;
			if (meid != 0xffff && mapped_meid ==0xffff)	// change meid of virtual classid
				me->meid= meid;

			// call hw and cb before mib read
			if (meinfo_me_refresh(me, get_attr_mask)==MEINFO_ERROR_ME_NOT_FOUND)
				continue;

			if (!is_classid_printed) {
				is_classid_printed=1;
				util_fdprintf(fd, "classid: 0x%x(%u) %s\n", 
					miptr->classid, miptr->classid, (miptr->name)?miptr->name:"?"); 
			}

			util_fdprintf(fd, "meid: 0x%x(%u)", me->meid, me->meid);

			// print aliasid of this meid
			if (omci_env_g.mib_aliasid_enable) {
				struct meinfo_aliasid2meid_t *aliasid2meid;
				int alias_found=0;
				list_for_each_entry(aliasid2meid, &miptr->config.meinfo_aliasid2meid_list, aliasid2meid_node) {
					if (aliasid2meid->meid==me->meid) {
						alias_found++;
						break;
					}
				}
				if (alias_found) {
					util_fdprintf(fd, ", alias:");
					list_for_each_entry(aliasid2meid, &miptr->config.meinfo_aliasid2meid_list, aliasid2meid_node) {
						if (aliasid2meid->meid==me->meid)
							util_fdprintf(fd, " 0x%x(%u)", aliasid2meid->aliasid, aliasid2meid->aliasid);
					}
				}
			}

			util_fdprintf(fd, ", inst %d", me->instance_num);
			if (me->is_private)
				util_fdprintf(fd, ", private");
			if (me_is_ready(miptr, me) == 0)
				util_fdprintf(fd, ", not_ready");

			instance_def_ptr = meinfo_instance_find_def(me->classid, me->instance_num);
			if (instance_def_ptr && strlen(instance_def_ptr->devname)>0)
				util_fdprintf(fd, ", devname %s", instance_def_ptr->devname);

			util_fdprintf(fd, "\n");

			{
				char *item_delim="";
				char *num_delim="";

				if (util_is_have_alarm_mask(miptr->alarm_mask)) {
					num_delim="";
					util_fdprintf(fd, "%salarm: ", item_delim);
					for (i=0; i<224; i++) {
						if (util_alarm_mask_get_bit(me->alarm_result_mask, i)) {
							util_fdprintf(fd, "%s%d", num_delim, i);
							num_delim=",";
						}
					}
					if (strlen(num_delim)==0)
						util_fdprintf(fd, "none");

					//show reported alarm mask
					num_delim="";
					util_fdprintf(fd, "(");
					for (i=0; i<224; i++) {
						if (util_alarm_mask_get_bit(me->alarm_report_mask, i)) {
							util_fdprintf(fd, "%s%d", num_delim, i);
							num_delim=",";
						}
					}
					if (strlen(num_delim)==0)
						util_fdprintf(fd, "none");
					util_fdprintf(fd, ")");

					item_delim=", ";
				}
				if (miptr->action_mask & MEINFO_ACTION_MASK_GET_CURRENT_DATA) {
					num_delim="";
					util_fdprintf(fd, "%stca: ", item_delim);
					for (i=1; i<=miptr->attr_total; i++) {
						struct attrinfo_t *aiptr=meinfo_util_aiptr(miptr->classid, i);
						if (aiptr->pm_alarm_number && 
						    util_alarm_mask_get_bit(me->alarm_result_mask, aiptr->pm_alarm_number)) {
							util_fdprintf(fd, "%s%d", num_delim, i);
							num_delim=",";
						}
					}
					if (strlen(num_delim)==0)
						util_fdprintf(fd, "none");
					item_delim=", ";
				}
				if (miptr->avc_mask[0] || miptr->avc_mask[1]) {
					num_delim="";
					util_fdprintf(fd, "%savc: ", item_delim);
					for (i=1; i<=miptr->attr_total; i++) {
						if (util_attr_mask_get_bit(me->avc_changed_mask, i)) {
							util_fdprintf(fd, "%s%d", num_delim, i);
							num_delim=",";
						}
					}
					if (strlen(num_delim)==0)
						util_fdprintf(fd, "none");
					item_delim=", ";
				}
				if (miptr->action_mask & MEINFO_ACTION_MASK_GET_CURRENT_DATA) {
					num_delim="";
					util_fdprintf(fd, "%saccum: ", item_delim);
					for (i=3; i<=miptr->attr_total; i++) {
						if (util_attr_mask_get_bit(me->pm_is_accumulated_mask, i)) {
							util_fdprintf(fd, "%s%d", num_delim, i);
							num_delim=",";
						}
					}
					if (strlen(num_delim)==0)
						util_fdprintf(fd, "none");
					item_delim=", ";
				}

				if (strlen(item_delim))		// add newline if any item is printed
					util_fdprintf(fd, "\n");
			}

			for (i=1; i<=miptr->attr_total; i++) {
				struct attrinfo_t *aiptr=meinfo_util_aiptr(miptr->classid, i);

				if (attr_order != 0 && i != attr_order)
					continue;

				util_fdprintf(fd, "%2u: %s: ", i, aiptr->name?aiptr->name:"?");

				if (aiptr->format == ATTR_FORMAT_TABLE) {
					struct attr_table_header_t *tab_header = me->attr[i].value.ptr;						       
					struct attr_table_entry_t *tab_entry;
					int j=0;

					util_fdprintf(fd, "(table)\n", buff);
					if (tab_header) {
						list_for_each_entry(tab_entry, &tab_header->entry_list, entry_node) {
							if (tab_entry->entry_data_ptr != NULL) {
								struct attr_value_t attr;
								attr.data=0;
								attr.ptr=tab_entry->entry_data_ptr;	// use attr to encapsulate entrydata for temporary use
								meinfo_conv_attr_to_string(miptr->classid, i, &attr, buff, 1024);
								util_fdprintf(fd, "\t%d: %s\n", j, buff);
								j++;
							}
						}
					}

				} else if (aiptr->format == ATTR_FORMAT_POINTER) {
					meinfo_conv_attr_to_string(miptr->classid, i, &me->attr[i].value, buff, 1024);
					util_fdprintf(fd, "%s [classid %u]\n", buff, meinfo_me_attr_pointer_classid(me, i));

				} else {
					if (is_classid_pm && i>=3) {
						cli_me_print_pm_value(aiptr,  &me->attr[i].value, buff, 1024);
						util_fdprintf(fd, "%s(his), ", buff);
						cli_me_print_pm_value(aiptr,  &me->attr[i].value_current_data, buff, 1024);
						util_fdprintf(fd, "%s(cur), ", buff);
						cli_me_print_pm_value(aiptr,  &me->attr[i].value_hwprev, buff, 1024);
						util_fdprintf(fd, "%s(hwprev)\n", buff);
					} else {
						meinfo_conv_attr_to_string(miptr->classid, i, &me->attr[i].value, buff, 1024);
						util_fdprintf(fd, "%s\n", buff);
					}
				}
			}

			found++;	// me found
		}
		if (found && classid==0)
			util_fdprintf(fd, "\n"); 

		total_found+=found;
	}
	if (!total_found)
		return CLI_ERROR_RANGE;

	return CLI_OK;
}

static int 
cli_me_summary(int fd)
{
	int index;
	int summary_class=0, summary_me=0;

	for (index = 0; index < MEINFO_INDEX_TOTAL; index++) {
		unsigned short classid=meinfo_index2classid(index);
		struct meinfo_t *miptr = meinfo_util_miptr(classid); 
		int instance_total=0;
		struct me_t *me, *me_start=NULL, *me_prev=NULL;

		if (miptr == NULL)
			continue;

		list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
			instance_total++;
		}

		if (instance_total>0) {
			int i=0;

			util_fdprintf(fd, "%5u %s[%d]: ", miptr->classid, miptr->name?miptr->name:"?", instance_total);
			list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
				/*int ready=1;
				if (!miptr->callback.is_ready_cb || miptr->callback.is_ready_cb(me)==1)
					ready=0;*/
				if (i == 0) {
					util_fdprintf(fd, "%s0x%x(%u)", me_is_ready(miptr, me)?"":"!", me->meid, me->meid);
					me_start = me;
				} else {
					if (me_prev && (me->meid - me_prev->meid != 1)) {
						if (me_start) {
							if (me_prev->meid - me_start->meid == 1) {
								util_fdprintf(fd, ", %s0x%x(%u)", me_is_ready(miptr, me_prev)?"":"!", me_prev->meid, me_prev->meid);
							} else if (me_prev->meid != me_start->meid) {
								util_fdprintf(fd, "-%s0x%x(%u)", me_is_ready(miptr, me_prev)?"":"!", me_prev->meid, me_prev->meid);
							}
						}
						util_fdprintf(fd, ", %s0x%x(%u)", me_is_ready(miptr, me)?"":"!", me->meid, me->meid);
						me_start = me;
					}
				}
				me_prev = me;
				i++;
			}
			if (me_prev->meid - me_start->meid == 1) {
				util_fdprintf(fd, ", %s0x%x(%u)", me_is_ready(miptr, me_prev)?"":"!", me_prev->meid, me_prev->meid);
			} else if (me_prev->meid != me_start->meid) {
				util_fdprintf(fd, "-%s0x%x(%u)", me_is_ready(miptr, me_prev)?"":"!", me_prev->meid, me_prev->meid);
			}
			util_fdprintf(fd, "\n");

			summary_class++;
			summary_me+=instance_total;
		}
	}
	util_fdprintf(fd, "total class: %d, total me: %d\n", summary_class, summary_me);
	return CLI_OK;
}

static int 
cli_me_find(int fd, char *keyword)
{
	int index;
	int class_found_total=0, me_found_total=0;

	for (index = 0; index < MEINFO_INDEX_TOTAL; index++) {
		int class_found=0;
		unsigned short classid=meinfo_index2classid(index);
		struct meinfo_t *miptr = meinfo_util_miptr(classid); 
		struct me_t *me;

		if (miptr == NULL)
			continue;

		list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
			int i, me_found=0;

			for (i=1; i<=miptr->attr_total; i++) {
				int found=0;
				struct attrinfo_t *aiptr=meinfo_util_aiptr(miptr->classid, i);
				if (aiptr->format == ATTR_FORMAT_TABLE) {
					struct attr_table_header_t *tab_header = me->attr[i].value.ptr;						       
					struct attr_table_entry_t *tab_entry;
					list_for_each_entry(tab_entry, &tab_header->entry_list, entry_node) {
						if (tab_entry->entry_data_ptr != NULL) {
							struct attr_value_t attr;
							attr.data=0;
							attr.ptr=tab_entry->entry_data_ptr;	// use attr to encapsulate entrydata for temporary use
							if (meinfo_conv_attr_find(miptr->classid, i, &attr, keyword)) {
								found=1;
								break;
							}
						}
					}
				} else {
					if (meinfo_conv_attr_find(classid, i, &me->attr[i].value, keyword))
						found=1;
				}
				if (found) {
					util_fdprintf(fd, "%5u %s: ", miptr->classid, miptr->name?miptr->name:"?");
					util_fdprintf(fd, "0x%x(%u)@%u(%s): ", me->meid, me->meid, i, aiptr->name);
					if (aiptr->format == ATTR_FORMAT_TABLE) {
						util_fdprintf(fd, "(table)");
					} else {
						char s[128];
						meinfo_conv_attr_to_string(miptr->classid, i, &me->attr[i].value, s, 128);
						util_fdprintf(fd, "%s", s);
					}
					util_fdprintf(fd, "\n");
					class_found=me_found=1;
				}
			}

			if (me_found)
				me_found_total++;
		}
		if (class_found)
			class_found_total++;
	}
	util_fdprintf(fd, "found class: %d, found me: %d\n", class_found_total, me_found_total);
	return CLI_OK;
}

static int 
cli_me_check(int fd, unsigned short classid)
{
	int index;
	int attr_is_valid=0;
	int attr_is_invalid=0;
	int attr_is_warning=0;
	int me_is_checked=0;

	for (index = 0; index < MEINFO_INDEX_TOTAL; index++) {
		unsigned short classid2=meinfo_index2classid(index);
		struct meinfo_t *miptr = meinfo_util_miptr(classid2); 
		unsigned short i;
		struct me_t *me;

		if (classid!=0 && classid!=classid2)
			continue;
		if (miptr==NULL)
			continue;

		list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
			for (i=1; i<=miptr->attr_total; i++) {
				struct attrinfo_t *aiptr=meinfo_util_aiptr(miptr->classid, i);
				int ret=meinfo_me_attr_is_valid(me, i, &me->attr[i].value);
				switch (ret) {
				case 1:		attr_is_valid++; break;
				case 0:		attr_is_invalid++; break;
				case -1:	attr_is_warning++; break;
				}
				if (ret<=0) {
					util_fdprintf(fd, "%5s: ", (ret==0)?"ERROR":"WARN");
					util_fdprintf(fd, "%5u %s: ", miptr->classid, miptr->name?miptr->name:"?");
					util_fdprintf(fd, "0x%x(%u)@%u(%s): ", me->meid, me->meid, i, aiptr->name);
					if (aiptr->format == ATTR_FORMAT_TABLE) {
						util_fdprintf(fd, "(table)");
					} else {
						char s[128];
						meinfo_conv_attr_to_string(miptr->classid, i, &me->attr[i].value, s, 128);
						util_fdprintf(fd, "%s", s);
					}
					util_fdprintf(fd, "\n");
				}
			}
			me_is_checked++;
		}
	}

	util_fdprintf(fd, "me chekced:%u, attr valid:%u, warning:%u, invalid:%u\n", me_is_checked, attr_is_valid, attr_is_warning, attr_is_invalid);

	return CLI_OK;
}

int
cli_me_related_classid(int fd, unsigned short classid)
{
	struct meinfo_t *miptr = meinfo_util_miptr(classid); 
	int index;

	if (miptr == NULL) {
		util_fdprintf(fd, "classid %d not supported?\n", classid);
		return CLI_ERROR_RANGE;
	}
	util_fdprintf(fd, "classid %d (%s) related class:\n", classid, miptr->name?miptr->name:"?");
	for (index = 0; index < MEINFO_INDEX_TOTAL; index++) {
		unsigned short classid2=meinfo_index2classid(index);
		struct meinfo_t *miptr2 =meinfo_util_miptr(classid2);

		if (miptr2 == NULL)	// classid2 not exist
			continue;
		if (me_related_find(classid, classid2) == NULL)	// relation classid -> classid2 not exist
			continue;
		util_fdprintf(fd, "%d (%s)\n", classid2, miptr2->name?miptr2->name:"?");
	}
	return CLI_OK;
}

int
cli_me_related(int fd, unsigned short classid, unsigned short classid2)
{
	struct meinfo_t *miptr = meinfo_util_miptr(classid); 
	struct meinfo_t *miptr2 =meinfo_util_miptr(classid2);
	struct me_t *me, *me2;
	char *me_devname, *me2_devname;
	char mestr[64], me2str[64];

	if (miptr == NULL) {
		util_fdprintf(fd, "classid %d not supported?\n", classid);
		return CLI_ERROR_RANGE;
	}
	if (miptr2 == NULL) {
		util_fdprintf(fd, "classid %d not supported?\n", classid2);
		return CLI_ERROR_RANGE;
	}

	if (me_related_find(classid, classid2) == NULL) {
		util_fdprintf(fd, "me_relation class %d -> %d doesn't exist\n", classid, classid2);
		return CLI_ERROR_RANGE;
	}

	list_for_each_entry(me2, &miptr2->me_instance_list, instance_node) {
		me2_devname=hwresource_devname(me2);
		if (me2_devname == NULL || strlen(me2_devname) == 0)
			me2_devname = meinfo_util_get_config_devname(me2);
		if (me2_devname && strlen(me2_devname)>0) {
			snprintf(me2str, 64, "%d:0x%x(%d) %s",
				classid2, me2->meid, me2->meid, me2_devname);
		} else {
			snprintf(me2str, 64, "%d:0x%x(%d)", 
				classid2, me2->meid, me2->meid);
		}

		list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
			if (me_related(me, me2)) {
				me_devname=hwresource_devname(me);
				if (me_devname == NULL || strlen(me_devname) == 0)
					me_devname = meinfo_util_get_config_devname(me);
				if (me_devname && strlen(me_devname)>0) {
					snprintf(mestr, 64, "%d:0x%x(%d) %s",
						classid, me->meid, me->meid, me_devname);
				} else {
					snprintf(mestr, 64, "%d:0x%x(%d)", 
						classid, me->meid, me->meid);
				}
				util_fdprintf(fd, "%s -> %s\n", mestr, me2str); 
			}
		}
	}
	return CLI_OK;
}

/* this is called before mibreset request is processed */
static void
cli_me_mibreset_dump_fd(int fd)
{
	if (omci_env_g.mibreset_dump_enable & 1) {
		util_fdprintf(fd, "------ mibreset dump er diag ------\n");
		cli_bridge(fd, 0xffff);
		cli_tcont(fd, 0xffff, 1);
		cli_pots(fd, 0xffff);
		util_fdprintf(fd, "\n");
		cli_unrelated(fd);
	}
	if (omci_env_g.mibreset_dump_enable & 2) {
		util_fdprintf(fd, "------ mibreset dump me summary ------\n");
		cli_me_summary(fd);
		util_fdprintf(fd, "------ mibreset dump me detail ------\n");
		cli_me(fd, 0, 0xffff, 0);
	}
	if (omci_env_g.mibreset_dump_enable)
		util_fdprintf(fd, "------ mibreset dump end ------\n");
}

void
cli_me_mibreset_dump(void)
{
	if (!omci_env_g.mibreset_dump_enable)
		return;
	if (omci_env_g.debug_log_type & ENV_DEBUG_LOG_TYPE_BITMASK_CONSOLE) {
		cli_me_mibreset_dump_fd(util_dbprintf_get_console_fd());
	}
	if ((omci_env_g.debug_log_type & ENV_DEBUG_LOG_TYPE_BITMASK_FILE) && omci_env_g.debug_log_file) {
		int fd=open(omci_env_g.debug_log_file, O_APPEND|O_CREAT);
		if (fd>=0) {
			cli_me_mibreset_dump_fd(fd);
			close(fd);
		}
	}
}

// helper function used by cli.c /////////////////////////////////////////////////////////////////////////////////////////////

void
cli_me_help(int fd)
{
	util_fdprintf(fd, 
		"me [classid] [meid] [attr_order]\n"
		"   create|delete [classid] [meid]\n"
		"   check [classid]\n"
		"   find [keyword]\n"
		"   related [classid]\n"
		"   related [classid] [classid2]\n"
		"   gclist [dump|aging]\n");
}

int
cli_me_cmdline(int fd, int argc, char *argv[])
{
	unsigned short classid=0, meid=0;
	unsigned char attr_order=0;

	if (strcmp(argv[0], "me")==0 && argc<=4) {
		if (argc==1) {
			return cli_me_summary(fd);

		} else if (argc==4 && strcmp(argv[1], "create")==0) {
			classid=util_atoi(argv[2]);
			meid=util_atoi(argv[3]);
			return cli_me_create(fd, classid, meid);

		} else if (argc==4 && strcmp(argv[1], "delete")==0) {
			classid=util_atoi(argv[2]);
			meid=util_atoi(argv[3]);
			return cli_me_delete(fd, classid, meid);

		} else if (argc>=2 && argc<=3 && strcmp(argv[1], "check")==0) {
			if (argc==2)
				return cli_me_check(fd, 0);
			else
				return cli_me_check(fd, util_atoi(argv[2]));

		} else if (argc==3 && strcmp(argv[1], "find")==0) {
			return cli_me_find(fd, argv[2]);

		} else if (argc>=3 && strcmp(argv[1], "related")==0) {
			classid=util_atoi(argv[2]);
			if (argc==3) {
				return cli_me_related_classid(fd, classid);
			} else {
				int classid2=util_atoi(argv[3]);
				return cli_me_related(fd, classid, classid2);
			}

		} else if (argc>=2 && strcmp(argv[1], "gclist")==0) {
			if (argc == 2) {
				meinfo_util_gclist_dump(fd, 0);
				return CLI_OK;
			} else if (argc ==3 && strcmp(argv[2], "dump")==0) {
				meinfo_util_gclist_dump(fd, 1);
				return CLI_OK;
			} else if (argc ==3 && strcmp(argv[2], "aging")==0) {
				meinfo_util_gclist_aging(CORETASK_GCLIST_AGING_INTERVAL);
				return CLI_OK;
			}

		} else {
			meid=0xffff;
			attr_order = 0;
			if (!util_string_is_number(argv[1]))
				return CLI_ERROR_OTHER_CATEGORY;
			classid=util_atoi(argv[1]);
			if (argc==3) {
				if (argv[2][0] == '*') {
					meid=0xffff;
				} else {
					meid=util_atoi(argv[2]);
				}
			}
			if (argc==4) {
				if (argv[2][0] == '*') {
					meid=0xffff;
				} else {
					meid=util_atoi(argv[2]);
				}
				attr_order=util_atoi(argv[3]);
			}
			return cli_me(fd, classid, meid, attr_order);
		}

	}

	return CLI_ERROR_OTHER_CATEGORY;
}

