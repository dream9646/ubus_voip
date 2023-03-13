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
 * File    : cli_spec.c
 *
 ******************************************************************/

#include <stdio.h>
#include <string.h>

#include "util.h"
#include "conv.h"
#include "xmlomci.h"
#include "meinfo.h"
#include "cli.h"

#define CLI_NONE_STRING "?"

extern struct conv_str2val_t map_me_required[];
extern struct conv_str2val_t map_me_access[];
extern struct conv_str2val_t map_me_action[];
extern struct conv_str2val_t map_me_attr_format[];
extern struct conv_str2val_t map_me_attr_vformat[];
extern struct conv_str2val_t map_me_attr_access_bitmap[];
extern struct conv_str2val_t map_me_attr_necessity[];

struct conv_str2val_t map_me_attr_access_bitmap_short[] = {
	{"R", ATTRINFO_ACCESEE_BITMAP_READ},	// 0x1
	{"W", ATTRINFO_ACCESEE_BITMAP_WRITE},	// 0x2
	{"S", ATTRINFO_ACCESEE_BITMAP_SBC},	// 0x4
	{NULL, 0}
};

struct conv_str2val_t map_me_supported_config[] = {
	{"Fully Supported", ME_SUPPORT_TYPE_SUPPORTED},
	{"Unsupported", ME_SUPPORT_TYPE_NOT_SUPPORTED},
	{"Partially Supported", ME_SUPPORT_TYPE_PARTIAL_SUPPORTED},
	{"Ignored", ME_SUPPORT_TYPE_IGNORED},
	{NULL, 0}
};

static int 
cli_spec_classid(int fd, unsigned short classid)
{
	char *str;
	char buf[256];
	char temp_buf[16];
	int i, j;
	int first;
	struct meinfo_t *miptr;

	miptr = meinfo_util_miptr(classid); 

	if (miptr != NULL)
	{
		util_fdprintf(fd, "id: %d %s\n", miptr->classid, miptr->is_private?"(private)":"");
		util_fdprintf(fd, "name: %s\n", miptr->name ? miptr->name:CLI_NONE_STRING);
		util_fdprintf(fd, "section: %s\n", miptr->section ? miptr->section : CLI_NONE_STRING);
		util_fdprintf(fd, "required: %s\n", (str = conv_value2str(miptr->required,map_me_required)) != NULL ? str : CLI_NONE_STRING);
		util_fdprintf(fd, "attr total: %d\n", miptr->attr_total);
		conv_mask2strarray(miptr->action_mask, map_me_action, ",", buf, 256);
		util_fdprintf(fd, "action: %s\n", buf);
		conv_mask2strnum(miptr->alarm_mask, 28, ",", buf, 256, 0);
		util_fdprintf(fd, "alarm num: %s\n", buf);
		conv_mask2strnum((unsigned char *) &miptr->avc_mask, 2, ",", buf, 256, 1);
		util_fdprintf(fd, "avc num: %s\n", buf);
		first = 1;
		buf[0]=0;
		for (i = 1; i <= miptr->attr_total; i++)
		{
			if (miptr->attrinfo[i].pm_tca_number != 0)
			{
				if (first)
				{
					sprintf(buf, "%d", i);
					first = 0;
				} else {
					sprintf(temp_buf, ",%d", i);
					strcat(buf, temp_buf);
				}
			}
		}
		util_fdprintf(fd, "tca num: %s\n", buf);
		conv_mask2strnum((unsigned char *) &miptr->test_mask, 1, ",", buf, 256, 0);
		util_fdprintf(fd, "test num: %s\n", buf);
		util_fdprintf(fd, "access: %s\n", (str = conv_value2str(miptr->access, map_me_access)) != NULL ? str : CLI_NONE_STRING);

		util_fdprintf(fd, "attributes:\n");
		for (i = 1; i <= miptr->attr_total; i++)
		{
			util_fdprintf(fd, "%2d: %s", i, miptr->attrinfo[i].name);
			util_fdprintf(fd, " ("); 
			if (miptr->attrinfo[i].format != ATTR_FORMAT_TABLE || miptr->attrinfo[i].attrinfo_table_ptr == NULL)
			{
				util_fdprintf(fd, "%dbyte", miptr->attrinfo[i].byte_size);
			} else {
				util_fdprintf(fd, "%dbyte", miptr->attrinfo[i].attrinfo_table_ptr->entry_byte_size);
			}
			
			util_fdprintf(fd, ",%s", (str = conv_value2str(miptr->attrinfo[i].format, map_me_attr_format)) != NULL ? str : CLI_NONE_STRING);
			if (miptr->attrinfo[i].vformat != 0)
			{
				util_fdprintf(fd, "-%s", (str = conv_value2str(miptr->attrinfo[i].vformat, map_me_attr_vformat)) != NULL ? str : CLI_NONE_STRING);
			}
			conv_mask2strarray(miptr->attrinfo[i].access_bitmap, map_me_attr_access_bitmap_short, "", buf, 256);
			util_fdprintf(fd, ",%s", buf);
			util_fdprintf(fd, ")");

			if (meinfo_util_attr_has_lower_limit(classid, i) || meinfo_util_attr_has_upper_limit(classid, i)) {
				util_fdprintf(fd, " [");
				if (meinfo_util_attr_has_lower_limit(classid, i))
					util_fdprintf(fd, "%lld", meinfo_util_attr_get_lower_limit(classid, i));
				else
					util_fdprintf(fd, ".");
				util_fdprintf(fd, " ~ ");
				if (meinfo_util_attr_has_upper_limit(classid, i))
					util_fdprintf(fd, "%lld", meinfo_util_attr_get_upper_limit(classid, i));
				else
					util_fdprintf(fd, ".");
				util_fdprintf(fd, "]");
			}
			
			if (miptr->attrinfo[i].format == ATTR_FORMAT_POINTER) {
				util_fdprintf(fd, " [classid %u]", meinfo_util_attr_get_pointer_classid(classid, i));
			}

			util_fdprintf(fd, "\n");

			if (miptr->attrinfo[i].format == ATTR_FORMAT_BIT_FIELD && miptr->attrinfo[i].attrinfo_bitfield_ptr != NULL)
			{
				for (j = 0; j < miptr->attrinfo[i].attrinfo_bitfield_ptr->field_total; j++)
				{
					util_fdprintf(fd, "%2d.%d: %s (%dbit,%s)\n", 
						i, j,
						miptr->attrinfo[i].attrinfo_bitfield_ptr->field[j].name,
						miptr->attrinfo[i].attrinfo_bitfield_ptr->field[j].bit_width,
						(str = conv_value2str(miptr->attrinfo[i].attrinfo_bitfield_ptr->field[j].vformat, map_me_attr_vformat)) != NULL ? str : CLI_NONE_STRING);
				}
			}
			if (miptr->attrinfo[i].format == ATTR_FORMAT_TABLE && miptr->attrinfo[i].attrinfo_table_ptr != NULL)
			{
				for (j = 0; j < miptr->attrinfo[i].attrinfo_table_ptr->field_total; j++)
				{
					util_fdprintf(fd, "%2d.%d: %s %s (%dbit,%s)\n", 
						i, j,
						miptr->attrinfo[i].attrinfo_table_ptr->field[j].is_primary_key ? "P" : " ",
						miptr->attrinfo[i].attrinfo_table_ptr->field[j].name,
						miptr->attrinfo[i].attrinfo_table_ptr->field[j].bit_width,
						(str = conv_value2str(miptr->attrinfo[i].attrinfo_table_ptr->field[j].vformat, map_me_attr_vformat)) != NULL ? str : CLI_NONE_STRING);
				}
			}

		}
	} else {
		util_fdprintf(fd, "No spec for classid=%u\n", classid);
		return CLI_ERROR_RANGE;
	}

	util_fdprintf(fd, "\n");

	return 0;
}

static int 
cli_spec(int fd, unsigned short classid)
{
	int i;
	struct meinfo_t *miptr;

	if (classid != 0)
	{
		cli_spec_classid(fd, classid);
	} else {
		for (i = 0; i < MEINFO_INDEX_TOTAL; i++)
		{
			miptr = meinfo_util_miptr(meinfo_index2classid(i)); 

			if (miptr == NULL)
				continue;

			if (miptr->config.is_inited && miptr->config.support != ME_SUPPORT_TYPE_NOT_SUPPORTED)
			{
				cli_spec_classid(fd, meinfo_index2classid(i));
			}
		}
	}
	return CLI_OK;
}

static int 
cli_spec_section(int fd, char *section_str)
{
	char secname[16];
	int found=0, i;
	struct meinfo_t *miptr;

	if (section_str) {
		int len;
		int is_exact_match=1;
	
		strncpy(secname, section_str, 16);
		secname[15]=0;
		len=strlen(secname);
		if (secname[len-1] == '*') {
			secname[len-1]=0;
			is_exact_match=0;
		}
		
		for (i = 0; i < MEINFO_INDEX_TOTAL; i++)
		{
			miptr = meinfo_util_miptr(meinfo_index2classid(i)); 

			if (miptr == NULL)
				continue;

			if (miptr->config.is_inited && miptr->config.support != ME_SUPPORT_TYPE_NOT_SUPPORTED)
			{
				if (miptr->section && strncmp(miptr->section, secname, strlen(secname))==0) {
					cli_spec_classid(fd, meinfo_index2classid(i));
					found++;
					if (is_exact_match)
						break;
				}
			}
		}
		if (!found)
			util_fdprintf(fd, "section '%s' not found\n", section_str);
	}
	
	return CLI_OK;
}

static int 
cli_spec_summary(int fd)
{
	int i;
	char *str;
	unsigned short classid;
	struct meinfo_t *miptr;

	for (i = 0; i < MEINFO_INDEX_TOTAL; i++)
	{
		classid = meinfo_index2classid(i);
		miptr = meinfo_util_miptr(classid); 

		if (miptr == NULL)
			continue;

		if (miptr->config.is_inited && miptr->config.support != ME_SUPPORT_TYPE_NOT_SUPPORTED)
		{
			util_fdprintf(fd, "%3d %s [%s, %s]\n", miptr->classid, 
				miptr->name?miptr->name:CLI_NONE_STRING, 
				miptr->section ? miptr->section : CLI_NONE_STRING,
				(str = conv_value2str(miptr->config.support, map_me_supported_config)) != NULL ? str : CLI_NONE_STRING);
		}
	}
	return 0;
}

static int 
cli_spec_find_classid(int fd, unsigned short classid, char *keyword)
{
	struct meinfo_t *miptr = meinfo_util_miptr(classid); 
	int attr_order, j;

	if (miptr == NULL)
		return 0;

	if (miptr->name && util_strcasestr(miptr->name, keyword))
		return 1;

	for (attr_order = 1; attr_order <= miptr->attr_total; attr_order++) {
		struct attrinfo_t *aiptr=meinfo_util_aiptr(classid, attr_order);
		struct attrinfo_bitfield_t *b=aiptr->attrinfo_bitfield_ptr;
		struct attrinfo_table_t *t=aiptr->attrinfo_table_ptr;
			
		if (aiptr->name && util_strcasestr(aiptr->name, keyword))
			return 1;

		if (aiptr->format == ATTR_FORMAT_BIT_FIELD && b != NULL) {
			for (j = 0; j < b->field_total; j++) {
				if (b->field[j].name && util_strcasestr(b->field[j].name, keyword))
					return 1;
			}
		}

		if (aiptr->format == ATTR_FORMAT_TABLE && t != NULL) {
			for (j = 0; j < t->field_total; j++) {
				if (t->field[j].name && util_strcasestr(t->field[j].name, keyword))
					return 1;
			}
		}

	}
	return 0;
}

static int 
cli_spec_find(int fd, char *keyword)
{
	int i, found=0;;
	char *str;

	for (i = 0; i < MEINFO_INDEX_TOTAL; i++) {
		unsigned short classid = meinfo_index2classid(i);
		if (cli_spec_find_classid(fd, classid, keyword)) {
			struct meinfo_t *miptr=meinfo_util_miptr(classid);
			util_fdprintf(fd, "%3d %s [%s, %s]\n", miptr->classid, 
				miptr->name?miptr->name:CLI_NONE_STRING, 
				miptr->section ? miptr->section : CLI_NONE_STRING,
				(str = conv_value2str(miptr->config.support, map_me_supported_config)) != NULL ? str : CLI_NONE_STRING);
			found++;
		}
	}
	if (!found)
		util_fdprintf(fd, "keyword '%s' not found\n", keyword);
	return CLI_OK;
}

static int 
cli_spec_alarm(int fd, unsigned short classid, char *alarmlist)
{
	struct meinfo_t *miptr=meinfo_util_miptr(classid);
	unsigned char mask[28];

	if (miptr==NULL)
		return CLI_ERROR_RANGE;

	memset(mask, 0x00, 28);
	if (strlen(alarmlist)==0 || conv_numlist2alarmmask(alarmlist, mask, 28) == 0) {	
		memcpy(miptr->alarm_mask, mask, 28);
	} else {		
		util_fdprintf(fd, "alarm list '%s' parse err\n", alarmlist);
		return CLI_ERROR_SYNTAX;
	}
	return CLI_OK;
}

static int 
cli_spec_avc(int fd, unsigned short classid, char *avclist)
{
	struct meinfo_t *miptr=meinfo_util_miptr(classid);
	unsigned char mask[2];

	if (miptr==NULL)
		return CLI_ERROR_RANGE;	

	memset(mask, 0x00, 2);
	if (strlen(avclist)==0 || conv_numlist2attrmask(avclist, mask, 2) == 0) {
		memcpy(miptr->avc_mask, mask, 2);
	} else {
		util_fdprintf(fd, "avc list '%s' parse err\n", avclist);
		return CLI_ERROR_SYNTAX;
	}
	return CLI_OK;
}

// helper function used by cli.c /////////////////////////////////////////////////////////////////////////////////////////////

void
cli_spec_help(int fd)
{
	util_fdprintf(fd, 
		"spec [classid|section]\n"
		"     alarm|avc [classid] [n1,n2,n3...]\n"
		"     find [keyword]\n");
}

int
cli_spec_cmdline(int fd, int argc, char *argv[])
{
	unsigned short classid=0;

	if (strcmp(argv[0], "spec")==0) {
		if (argc==1) {
			return cli_spec_summary(fd);
		} else if (argc==2 && strstr(argv[1], ".")) {
			return cli_spec_section(fd, argv[1]);
		} else if (argc==2) {
			classid=util_atoi(argv[1]);
			return cli_spec(fd, classid);
		} else if (argc==4 && strcmp(argv[1], "avc")==0) {
			classid=util_atoi(argv[2]);
			return cli_spec_avc(fd, classid, argv[3]);
		} else if (argc==4 && strcmp(argv[1], "alarm")==0) {
			classid=util_atoi(argv[2]);
			return cli_spec_alarm(fd, classid, argv[3]);
		} else if (argc==3 && strcmp(argv[1], "find")==0) {
			return cli_spec_find(fd, argv[2]);
		}
	}
	return CLI_ERROR_OTHER_CATEGORY;
}
