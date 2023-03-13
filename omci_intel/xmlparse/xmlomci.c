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
 * File    : xmlomci.c
 *
 ******************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <syslog.h>
#include <dirent.h>

#include <arpa/inet.h>

#include "util.h"
#include "conv.h"
#include "meinfo.h"
#include "list.h"
#include "xmlparse.h"
#include "xmlomci.h"
#include "fencrypt.h"
#include "switch.h"
#include "voip.h"
#include "omci_config.h"

#define OMCI_ENV_FILE_PATH1 "./omcienv.xml"
#define OMCI_ENV_FILE_PATH2 "/etc/omci/omcienv.xml"
#define	TMPFILE	"/var/tmp/.xtmp"

// routine shared by spec.xml and config.xml ///////////////////////////////////////////////////////

int
xmlomci_strlist2codepoints(char *str, struct list_head *code_points_list_ptr)
{
	char buff[1024];
	char *a[128];
	int n, i;

	strncpy(buff, str, 1024);
	buff[1023] = 0;
	n = conv_str2array(buff, ",", a, 128);

	INIT_LIST_HEAD(code_points_list_ptr);

	for (i = 0; i < n; i++) {
		struct attr_code_point_t *c = malloc_safe(sizeof (struct attr_code_point_t));
		c->code = util_atoi(a[i]);
		list_add_tail(&c->code_points_list, code_points_list_ptr);
	}
	return 0;
}

int
xmlomci_loadstr(char **new_ptr, char *old_ptr)
{
	*new_ptr=malloc_safe(strlen(old_ptr)+1);
	strcpy(*new_ptr, old_ptr);
	return 0;
}

static char *abbrev_table[]={
	"performance_monitoring_history_data", "PM",
	"Physical_path_termination_point", "Pptp",
	"interworking_termination_point", "IWTP",
	"operation_configuration_data", "operation",
	"line_inventory_and_status_data_part", "line_status",
	NULL
}; 	

char *
xmlomci_abbrestr(char *str)
{
	static char buff[256]={0};
	int i;

	for (i=0; abbrev_table[i*2]!=NULL; i++) {
 		char *found=strstr(str, abbrev_table[i*2]);
 		if (found) {
 			char *found2=found+strlen(abbrev_table[i*2]);
 			int len1=found-str;
 			int len2=strlen(abbrev_table[i*2+1]); 		
 			memcpy(buff, str, len1);
 			memcpy(buff+len1, abbrev_table[i*2+1], len2);
 			strcpy(buff+len1+len2, found2);
 			return buff;
		}
	}			
	return str;
}

// this function load all xml files into mem data stru, 
// note:
// a. data preinit should be done before this func
// b. this function should be issued for only one
int
xmlomci_init(char *env_file_path, char *spec_passwd, char *config_passwd, char *er_group_passwd)
{
	{ // load omcienv.xml
		char *env_file = NULL;
		struct stat state;
		DIR *d;
		
		env_set_to_default();
		if (env_file_path != NULL && stat(env_file_path, &state) == 0) 
			env_file = env_file_path;
		if (env_file == NULL && stat(OMCI_ENV_FILE_PATH1, &state) == 0)
			env_file = OMCI_ENV_FILE_PATH1;
		if (env_file == NULL && stat(OMCI_ENV_FILE_PATH2, &state) == 0)
			env_file = OMCI_ENV_FILE_PATH2;
		if (env_file) {
			if (xmlomci_env_init(env_file)<0)
				return -1;
		} else {
			fprintf(stderr, "Have no env file, use default value\n");
		}

		omci_config_load_config();

		// ensure etc_omci_path is available
		d=opendir(omci_env_g.etc_omci_path);
		if (d==NULL) {
			dbprintf_xml(LOG_ERR, "etc_omci_path %s open failed\n", omci_env_g.etc_omci_path);
			return -1;
		}
		closedir(d);
	}

	//update auto detect voip enable state
	voip_update_voip_enable_auto();

	//update system hw interface lan vid starting value
	switch_get_system_hw_intf_lan_vid();

	{ // load spec_09_nn.xml & config_09_nn.xml
		char filename[512];
		int i, ret;
		
		for (i = 0; omci_env_g.xml_section[i]!=0; i++) {
			// load spec
			if (util_omcipath_filename(filename, 512, "spec/spec_9_%02d.xml", omci_env_g.xml_section[i])>=0) {
				ret=xmlomci_spec_init(filename);
			} else {
				util_omcipath_filename(filename, 512, "spec/spec_9_%02d.xdb", omci_env_g.xml_section[i]);
				fencrypt_decrypt(filename, TMPFILE, spec_passwd);
				ret=xmlomci_spec_init(TMPFILE);
				unlink(TMPFILE);
			}
			if (ret<0) {
				dbprintf_xml(LOG_ERR, "spec/spec_9_%02d.* load err\n", omci_env_g.xml_section[i]);
				return -1;
			}

			// load config
			if (util_omcipath_filename(filename, 512, "config/config_9_%02d.xml", omci_env_g.xml_section[i])>=0) {
				ret=xmlomci_config_init(filename);
			} else if (util_omcipath_filename(filename, 512, "config/config_9_%02d.xdb", omci_env_g.xml_section[i]) >= 0) {
				fencrypt_decrypt(filename, TMPFILE, config_passwd);
				ret=xmlomci_config_init(TMPFILE);
				unlink(TMPFILE);
			}
			if (ret<0) {
				dbprintf_xml(LOG_ERR, "config/config_9_%02d.* load err\n", omci_env_g.xml_section[i]);
				return -1;
			}
		}
	}

	{ // load er_group.xml
		char filename[512];
		int ret;
		
		if (util_omcipath_filename(filename, 512, "er_group/er_group.xml")>=0) {
			ret=xmlomci_er_group_init(filename);
		} else {
			util_omcipath_filename(filename, 512, "er_group/er_group.xdb");
			fencrypt_decrypt(filename, TMPFILE, er_group_passwd);
			ret=xmlomci_er_group_init(TMPFILE);
			unlink(TMPFILE);
		}
		if (ret <0) {
			dbprintf_xml(LOG_ERR, "er_group/er_group.* load err\n");
			return -1;
		}
		
		// optionally load er_group_99.xml/xdb
		if (util_omcipath_filename(filename, 512, "er_group/er_group_99.xml") > 0) {
			ret=xmlomci_er_group_init(filename);
		} else if (util_omcipath_filename(filename, 512, "er_group/er_group_99.xdb") > 0) {
			fencrypt_decrypt(filename, TMPFILE, er_group_passwd);
			ret=xmlomci_er_group_init(TMPFILE);
			unlink(TMPFILE);
		}
		if (ret <0) {
			dbprintf_xml(LOG_ERR, "er_group/er_group_99.* load err\n");
			// as this file is optional, we don't ret even load err
		}
	}

	return 0;
}
