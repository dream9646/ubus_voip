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
 * File    : cli_attr.c
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
#include "er_group.h"

static int 
cli_attrget(int fd, unsigned short classid, unsigned short meid, unsigned char attr_order)
{
	struct meinfo_t *miptr;
	struct attrinfo_t *aiptr;
	struct attr_value_t attr;
	struct me_t *me_ptr;
	char input[MAX_CLI_LINE];
	unsigned char get_attr_mask[2] = {0, 0};

	if ((miptr=meinfo_util_miptr(classid))==NULL) {
		util_fdprintf(fd, "classid=%u, not supported\n", classid);
		return CLI_ERROR_RANGE;
	}

	if ((me_ptr = meinfo_me_get_by_meid(classid, meid)) == NULL) {
		util_fdprintf(fd,
			"err=%d, classid=%u, meid=0x%x(%u), attr_order=%u\n",
			MEINFO_ERROR_ME_NOT_FOUND, classid, meid, meid, attr_order);
		return CLI_ERROR_RANGE;
	}

	// call hw and cb before mib read
	util_attr_mask_set_bit(get_attr_mask, attr_order);
	meinfo_me_refresh(me_ptr, get_attr_mask);

	attr.data=0;
	attr.ptr = malloc_safe(MAX_CLI_LINE);      // max value GET_ATTRS_SIZE possible in get

	meinfo_me_attr_get(me_ptr, attr_order, &attr);
	aiptr = meinfo_util_aiptr(classid, attr_order);

	if (aiptr->format == ATTR_FORMAT_TABLE) {
		struct attr_table_entry_t *table_entry_node = NULL;
		struct attr_table_header_t *attr_node_head;	     

		attr_node_head = me_ptr->attr[attr_order].value.ptr;
						       
		list_for_each_entry(table_entry_node, &attr_node_head->entry_list, entry_node) {
			if (table_entry_node->entry_data_ptr != NULL) {
				memcpy(attr.ptr, table_entry_node->entry_data_ptr, MAX_CLI_LINE);
				meinfo_conv_attr_to_string(classid, attr_order, &attr, input, MAX_CLI_LINE);
				util_fdprintf(fd,"%s\n",input);
			}
		}
	} else {
		meinfo_conv_attr_to_string(classid, attr_order, &attr, input, MAX_CLI_LINE);
		if (aiptr->format == ATTR_FORMAT_STRING) {
			int len, i;
			len=(aiptr->byte_size < MAX_CLI_LINE)?aiptr->byte_size:MAX_CLI_LINE;
			util_fdprintf(fd,"%s (hex:", input);
			for (i=0; i<len; i++)
				util_fdprintf(fd," %02x", input[i]);
			util_fdprintf(fd,")\n", input[i]);
		} else {
			util_fdprintf(fd,"%s\n", input);
		}
	}		       

	if ( attr.ptr )
		free_safe(attr.ptr);
	return CLI_OK;
}

static int 
cli_attrset(int fd, unsigned short classid, unsigned short meid, unsigned char attr_order, char *str)
{
	struct meinfo_t *miptr;
	struct attr_value_t attr;
	struct me_t *me;
	unsigned char attr_mask[2]={0, 0};
	int ret;

	if ((miptr=meinfo_util_miptr(classid))==NULL) {
		util_fdprintf(fd, "classid=%u, not supported\n", classid);
		return CLI_ERROR_RANGE;
	}

	if ((me=meinfo_me_get_by_meid(classid, meid))==NULL) {
		util_fdprintf(fd, "classid=%u, meid=0x%x(%u), not found\n", classid, meid, meid);
		return CLI_ERROR_INTERNAL;
	}

	attr.ptr=NULL;
	attr.data=0;
	if ((ret=meinfo_conv_string_to_attr(classid, attr_order, &attr, str)) <0) {
		util_fdprintf(fd, "classid=%u, meid=0x%x(%u), attr_order=%u, err=%d\n", 
				classid, meid, meid, attr_order, ret);
		return CLI_ERROR_INTERNAL;
	}

	if (meinfo_me_attr_is_valid(me, attr_order, &attr)) {
		if (meinfo_util_attr_get_format(classid, attr_order) == ATTR_FORMAT_TABLE )
			ret=meinfo_me_attr_set_table_entry(me, attr_order, attr.ptr, 1);
		else
			ret=meinfo_me_attr_set(me, attr_order, &attr, 1);
	} else {
		ret = MEINFO_ERROR_ATTR_VALUE_INVALID;
	}
	if (attr.ptr)
		free_safe(attr.ptr);
	if (ret<0)
		return CLI_ERROR_INTERNAL;

	util_attr_mask_set_bit(attr_mask, attr_order);

	// call cb and hw after mib write
	meinfo_me_flush(me, attr_mask);

	return CLI_OK;
}

// attrset multiple 
static int 
cli_attrsetm(int fd, unsigned short classid, unsigned short meid, unsigned char attr_mask[], char *str_array[])
{
	struct meinfo_t *miptr;
	struct attr_value_t attr;
	struct me_t *me;
	int i, ret;
	unsigned short attr_order;

	if ((miptr=meinfo_util_miptr(classid))==NULL) {
		util_fdprintf(fd, "classid=%u, not supported\n", classid);
		return CLI_ERROR_RANGE;
	}

	if ((me=meinfo_me_get_by_meid(classid, meid))==NULL) {
		util_fdprintf(fd, "classid=%u, meid=0x%x(%u), not found\n", classid, meid, meid);
		return CLI_ERROR_INTERNAL;
	}

	// check if any null pointer in str_array[]
	for (attr_order=1, i=0; attr_order<=miptr->attr_total; attr_order++) {
		if (util_attr_mask_get_bit(attr_mask, attr_order)) {
			if (str_array[i]==NULL)
				return CLI_ERROR_SYNTAX;
			i++;
		}
	}

	for (attr_order=1, i=0; attr_order<=miptr->attr_total; attr_order++) {
		if (util_attr_mask_get_bit(attr_mask, attr_order)) {
			attr.ptr=NULL;
			attr.data=0;
			if ((ret=meinfo_conv_string_to_attr(classid, attr_order, &attr, str_array[i])) <0) {
				util_fdprintf(fd, "classid=%u, meid=0x%x(%u), attr_order=%u, str conv, err=%d\n", 
						classid, meid, meid, attr_order, ret);
				i++;
				continue;
			}

			if (!meinfo_me_attr_is_valid(me, attr_order, &attr)) {
    				util_fdprintf(fd, "classid=%u, meid=0x%x(%u), attr_order=%u, is_valid, err=%d\n", 
	      				classid, meid, meid, attr_order, MEINFO_ERROR_ATTR_VALUE_INVALID);
				i++;
				continue;
			}

			if (meinfo_util_attr_get_format(classid, attr_order) == ATTR_FORMAT_TABLE ) {
				ret=meinfo_me_attr_set_table_entry(me, attr_order, attr.ptr, 1);
			} else {
				ret=meinfo_me_attr_set(me, attr_order, &attr, 1);
			}
			if (ret<0) {
				util_fdprintf(fd, "classid=%u, meid=0x%x(%u), attr_order=%u, attr set, err=%d\n", 
					classid, meid, meid, attr_order, ret);
			}

			if (attr.ptr)
				free_safe(attr.ptr);

			i++;
		}
	}

	ret = 0;

	if (miptr->callback.set_cb && (ret = miptr->callback.set_cb(me, attr_mask)) < 0)
	{
		dbprintf(LOG_ERR,
			 "err=%d, classid=%u, meid=0x%x(%u), ret=%d, mask[0]=0x%2x, mask[1]=0x%2x\n",
			 OMCIMSG_ERROR_SW_ME_SET, classid, meid, meid, ret, attr_mask[0], attr_mask[1]);
	}

	if (omci_env_g.mib_rw_mode == ENV_MIB_RW_MODE_HWHW || omci_env_g.mib_rw_mode == ENV_MIB_RW_MODE_SWHW)
	{
		if (meinfo_me_issue_self_er_attr_group(me, ER_ATTR_GROUP_HW_OP_UPDATE, attr_mask) < 0)
		{
			dbprintf(LOG_ERR,
			 "issue self er_attr_group error, classid=%u, instance_num=%d, meid=%u\n",
			me->classid, me->instance_num, me->meid);
		}

		//check me ready state related with this class
		if (meinfo_me_issue_related_er_attr_group(me->classid) < 0)
		{
			dbprintf(LOG_ERR,
			 "issue related er_attr_group error, classid=%u, instance_num=%d, meid=%u\n",
			me->classid, me->instance_num, me->meid);
		}
	}

	return CLI_OK;
}

// helper function used by cli.c /////////////////////////////////////////////////////////////////////////////////////////////

void
cli_attr_help(int fd)
{
	util_fdprintf(fd, 
		"attr get [classid] [meid] [attr_order]\n"
		"     set [classid] [meid] [attr_order] [string]\n"
		"     setm [classid] [meid] [attrmask] [string] ...\n");
}

int
cli_attr_cmdline(int fd, int argc, char *argv[])
{
	unsigned short classid=0, meid=0;
	unsigned char attr_order=0;

	if (strcmp(argv[0], "attr")==0) {

		if (argc==1) {
			cli_attr_help(fd);
			return 0;

		} else if (argc==5 && strcmp(argv[1], "get")==0) {
			classid=util_atoi(argv[2]);
			meid=util_atoi(argv[3]);
			attr_order=util_atoi(argv[4]);
			//printf("[%d,%d,%d]\n", classid, meid, attr_order);
			return cli_attrget(fd, classid, meid, attr_order);

		} else if (argc>=6 && strcmp(argv[1], "set")==0) {
			char str[1024];
			int i;

			classid=util_atoi(argv[2]);
			meid=util_atoi(argv[3]);
			attr_order=util_atoi(argv[4]);

			str[0]=0;
			for (i=5; i<argc; i++) {
				if (i>5) strcat(str, " ");
				strcat(str, argv[i]);
			}
			//printf("[%d,%d,%d,%s]\n", classid, meid, attr_order, str);
			return cli_attrset(fd, classid, meid, attr_order, str);

		} else if (argc>=6 && strcmp(argv[1], "setm")==0) {
			unsigned char attr_mask[2];
			char *str_array[16];
			int i, bit_total=0;

			classid=util_atoi(argv[2]);
			meid=util_atoi(argv[3]);
			attr_mask[0]=(util_atoi(argv[4])>>8) & 0xff;
			attr_mask[1]=util_atoi(argv[4]) & 0xff;

			for (i=1; i<=16; i++) {
				if (util_attr_mask_get_bit(attr_mask, i))
					bit_total++;
			}
			if (bit_total!=argc-5) {
				util_fdprintf(fd, "%d bit set in mask 0x%02x%02x, but %d values\n", 
					bit_total, attr_mask[0], attr_mask[1], argc-5);
				return CLI_ERROR_SYNTAX;
			}
			memset(str_array, 0, sizeof(str_array));
			for (i=0; i<argc-5; i++)
				str_array[i]=argv[i+5];
			return cli_attrsetm(fd, classid, meid, attr_mask, str_array);
		}

	}
	return CLI_ERROR_OTHER_CATEGORY;
}

