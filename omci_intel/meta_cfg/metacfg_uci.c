/******************************************************************
 *
 * Copyright (C) 2018 5V Technologies Ltd.
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
 * Module  : batchtab
 * File    : batchtab_cb_lldp.c
 *
 ******************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "util.h"
#include "list.h"
#include "util_uci.h"

#define ARRAY_SIZE(arr) (sizeof((arr)) / sizeof((arr)[0]))

struct metacfg_uci_table_t {
	char *path;
	char *section;
	char *option;
	int  is_list;
	int  pmatch;/*1 is patial match*/
	
};


struct metacfg_filename_t {
	char *filename;
	char *path;
	int  pmatch;/*1 is patial match*/
	
};

static struct metacfg_uci_table_t metacfg_table[]={
	{"fvt_metadata", "alu_option", "ont_mode",0,0},
	{"fvt_metadata", "brwan", "brwan",0,1},
	{"fvt_metadata", "device", "dev",0,1},
	{"fvt_metadata", "system", "system",0,1},
	{"fvt_metadata", "wan0", "wan0",0,1},
	{"fvt_metadata", "cwmp_acs", "cwmp",0,1},
	{"fvt_metadata", "calix_option", "calix_mgmt_mode",0,0},	
	{"fvt_metadata", "layer2_protocol_vlan", "l2_proto_vlan",0,1},
	/////////////////////////////////////////////////
	{"fvt_gpon", "gpon", "onu_serial",0,0},
	{"fvt_gpon", "gpon", "onu_password",0,0},
	//////////////////////////////////////////////
	{"fvt_version", "calix_option", "model",0,0},	
	{"fvt_version", "version", "l2_proto_vlan",0,0},
	////////////////////////////////////////////////////
	{"fvt_factory", "fvt_calix_fc", "calix_model",0,0},
	{"fvt_factory", "fvt_calix_fc", "calix_300_PN",0,0},
};
static struct metacfg_filename_t metacfg_file_table[]={
	{"metafile.dat", "fvt_metadata",1},
	{"gpon.dat", "fvt_gpon", 1},	
};


static int metacfg_match_uci_table_index(const char* option) 
{
	int i;
	for(i=0;i<ARRAY_SIZE(metacfg_table);i++)
	{
		if(metacfg_table[i].pmatch){
			if(strncmp(option,metacfg_table[i].option,strlen(metacfg_table[i].option)) == 0)
				return i;

		}else{
			if(strcmp(metacfg_table[i].option,option) == 0)
				return i;
		}
	}

	return -1;
}

static char* metacfg_search_uci_table_path(const char* option) 
{
	int i;
	for(i=0;i<ARRAY_SIZE(metacfg_table);i++)
	{
		if(metacfg_table[i].pmatch){
			if(strncmp(option,metacfg_table[i].option,strlen(metacfg_table[i].option)) == 0)
				return metacfg_table[i].path;

		}else{
			if(strcmp(metacfg_table[i].option,option) == 0)
				return metacfg_table[i].path;
		}
	}

	return NULL;
}

static char* metacfg_get_uci_table_path_by_filename(const char* filename) 
{
	int i;
	for(i=0;i<ARRAY_SIZE(metacfg_file_table);i++)
	{
		if(metacfg_file_table[i].pmatch){
			if(strstr(filename,metacfg_file_table[i].filename))
				return metacfg_file_table[i].path;

		}else{
			if(strcmp(filename,metacfg_file_table[i].filename) == 0)
				return metacfg_file_table[i].path;
		}
	}

	return NULL;
}

int metacfg_uci_get_config_vlaue_by_key(char* option,int* value)
{
	int idx;
	char result_char[UTIL_UCI_PARAM_STR_MAX_SIZE];
	
	idx=metacfg_match_uci_table_index(option);

	if(idx < 0){
		return -1;
	}

	memset(result_char,0x0,UTIL_UCI_PARAM_STR_MAX_SIZE);
	
	idx = util_uci_config_get(metacfg_table[idx].path
	                     ,metacfg_table[idx].section,metacfg_table[idx].option,&result_char[0]);

	if(idx < 0)
		return -1;

	idx = atoi(&result_char[0]);

	*value =idx;

	return 0;
	
}

int metacfg_uci_get_config_string_by_key(const char* option,char* value)
{
	int idx;
	char result_char[UTIL_UCI_PARAM_STR_MAX_SIZE];
	
	idx=metacfg_match_uci_table_index(option);

	if(idx < 0){
		return -1;
	}

	memset(result_char,0x0,UTIL_UCI_PARAM_STR_MAX_SIZE);
	
	idx = util_uci_config_get(metacfg_table[idx].path
	                     ,metacfg_table[idx].section,metacfg_table[idx].option,value);

	if(idx < 0)
		return -1;

	return 0;
	
}

int metacfg_uci_set_config_string(void * uci_ctx,const char* option,const char* value)
{
	int idx;
	//char result_char[UTIL_UCI_PARAM_STR_MAX_SIZE];

	if(uci_ctx == NULL)
		return -1;
	
	idx=metacfg_match_uci_table_index(option);

	if(idx < 0){
		return -1;
	}
	
	
	idx = util_uci_config_set(uci_ctx,metacfg_table[idx].path
	                     ,metacfg_table[idx].section,metacfg_table[idx].option,value,metacfg_table[idx].is_list);

	if(idx < 0)
		return -1;

	return 0;
	
}

int metacfg_uci_release(void* uci_ctx,void* uci_pkg)
{
	util_uci_free(uci_ctx,uci_pkg);
	return 0;
}

int metacfg_uci_init(const char *key,void** uci_ctx,void** uci_pkg)
{
	char * path = metacfg_search_uci_table_path(key);
	
	if(path ==NULL)
		return -1;
	
	if(util_uci_init(path,uci_ctx,uci_pkg)<0)
		return -1;

	return 0;
}

int metacfg_uci_init_by_cppkg(void* uci_pkg_ori,void** uci_ctx,void** uci_pkg)
{
	return util_uci_init_by_cppkg(uci_pkg_ori, uci_ctx, uci_pkg);

}


int metacfg_uci_init_by_prefixkey(void* uci_ctx,void** uci_pkg,char* prefix_key)
{
	char * path;
		
	if(uci_ctx == NULL)
		return -1;
	
	path = metacfg_search_uci_table_path(prefix_key);
	if(path == NULL)
		return -1;
	return util_uci_init(path,uci_ctx, uci_pkg);

}

void metacfg_uci_config_save(void* uci_ctx,void* uci_pkg)
{

	util_uci_config_save(uci_ctx,uci_pkg);
}

int metacfg_uci_list_configs_copy(void* uci_pkg_old, void* uci_ctx_new)
{
	return util_uci_list_configs_copy(uci_pkg_old,uci_ctx_new);
}
int metacfg_uci_config_is_empty(void* uci_ctx,void* uci_pkg)
{
	return util_uci_config_is_empty(uci_ctx,uci_pkg);

}
int metacfg_uci_config_iterate(void *meta_cfg,void *uci_pkg,
		  int (*iterate_func) (void * kv_config, const char *key, const char *value, void *data),
		  void *data)
{
	return util_uci_config_iterate(meta_cfg,uci_pkg,iterate_func,data);
}
void*  metacfg_uci_config_entity_locate(void* uci_pkg,const char *key)
{
	return util_uci_config_entity_locate(uci_pkg,key);
}
int metacfg_uci_config_entity_entity_del(void* uci_pkg,const char *key)
{
	return util_uci_config_entity_del(uci_pkg,key);
}
int metacfg_uci_config_merge(void* uci_base_pkg,void* uci_upd_pkg)
{
	return util_uci_config_entity_merge(uci_base_pkg,uci_upd_pkg);
}
int metacfg_uci_config_count(void* uci_pkg)
{
	return util_uci_config_entity_count(uci_pkg);
}
int metacfg_uci_config_file_load_by_prefixkey(void* uci_pkg,char *key_prefix)
{
	return util_uci_config_file_load_by_prefixkey(uci_pkg,key_prefix);
}
int metacfg_uci_config_file_load_safe(void** uci_ctx,void** uci_pkg , const char * filename)
{
	char *path;
	
	if(*uci_ctx || *uci_pkg){
		dbprintf(LOG_ERR, "metacfg load file fail(%s)\n", filename);
		return -1;
	}	

	path = metacfg_get_uci_table_path_by_filename(filename);
	if(path==NULL)
		return -1;
	if(util_uci_init(path,uci_ctx,uci_pkg)<0)
		return -1;
	return 0;
}
