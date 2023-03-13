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
 * Module  : switch_hw_prx126
 * File    : switch_hw_prx126_acl.c
 *
 ******************************************************************/

#ifndef __UTIL_UCI_h__
#define __UTIL_UCI_h__

#define UTIL_UCI_TUPLE_STR_MAX_SIZE	256
#define UTIL_UCI_PARAM_STR_MAX_SIZE	256
#define UTIL_UCI_MAX_LEN                200

void util_uci_free(void * uci_ctx,void * uci_pkg);

int  util_uci_init_by_cppkg(void * uci_pkg_ori, void ** uci_ctx,void ** uci_pkg);


int util_uci_init(const char *path, void ** uci_ctx,void ** uci_pkg);


int util_uci_config_get(const char *path, const char *sec, const char *opt,char *out);


int util_uci_config_generic_get(const char *path,
				const char *sec,
				const char *opt,
				void *ctx,
				int (*getter)(void *ctx, const char *value));

				
int util_uci_config_set(void* uci_ctx,const char *path, const char *sec, const char *opt,
			   const char *val, int add_list);

void util_uci_config_save(void* uci_ctx,void* uci_pkg);

int util_uci_config_is_empty(void* uci_ctx,void* uci_pkg);


int util_uci_config_iterate( void* meta_cfg,void *uci_pkg,
		  	    int (*iterate_func) (void * kv_config, const char *key, const char *value, void *data),
		  	    void *data);

int util_uci_list_configs_copy(void* uci_pkg_old,void* uci_ctx_new);

void* util_uci_config_entity_locate(void* uci_pkg,const char *key);

int util_uci_config_entity_del(void* uci_pkg,const char *key);

int util_uci_config_entity_merge(void* uci_base_pkg,void* uci_update_pkg);

int util_uci_config_entity_count(void* uci_pkg);

int util_uci_config_file_load_by_prefixkey(void* uci_pkg,char* prefix_key);

#endif
