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

#ifndef __METACFG_UCI_h__
#define __METACFG_UCI_h__

int metacfg_uci_get_config_vlaue_by_key(char* option,int* value);


int metacfg_uci_get_config_string_by_key(const char* option,char* value);


int metacfg_uci_set_config_string(void * uci_ctx,const char* option,const char* value);


int metacfg_uci_release(void* uci_ctx,void* uci_pkg);

int metacfg_uci_init(const char *key,void** uci_ctx,void** uci_pkg);


int metacfg_uci_init_by_cppkg(void* uci_pkg_ori,void** uci_ctx,void** uci_pkg);

int metacfg_uci_init_by_prefixkey(void* uci_ctx,void** uci_pkg,char* prefix_key);

void metacfg_uci_config_save(void* uci_ctx,void* uci_pkg);

int metacfg_uci_list_configs_copy(void* uci_pkg_old, void* uci_ctx_new);

int metacfg_uci_config_is_empty(void* uci_ctx,void* uci_pkg);

int metacfg_uci_config_iterate(void *meta_cfg,void *uci_pkg,
		  int (*iterate_func) (void * kv_config, const char *key, const char *value, void *data),
		  void *data);

void*  metacfg_uci_config_entity_locate(void* uci_pkg,const char *key);

int metacfg_uci_config_entity_entity_del(void* uci_pkg,const char *key);

int metacfg_uci_config_merge(void* uci_base_pkg,void* uci_upd_pkg);

int metacfg_uci_config_count(void* uci_pkg);

int metacfg_uci_config_file_load_by_prefixkey(void* uci_pkg,char *key_prefix);
int metacfg_uci_config_file_load_safe(void** uci_ctx,void** uci_pkg ,const  char * filename);

#endif

