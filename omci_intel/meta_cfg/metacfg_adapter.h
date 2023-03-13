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

#ifndef __METACFG_ADAPTER_h__
#define __METACFG_ADAPTER_h__

#include "fwk_mutex.h"
#include "list.h"
#include "kv_config.h"

#ifdef OMCI_METAFILE_ENABLE

#if USE_UCI_CONFIG
#define METACFG_UCI_STRING_LEN 256
struct metacfg_t {
	void * uci_ctx;
	void * uci_pkg;
	int total; 
	int is_init;
	char * string_val;
	//struct list_head biglist;

	struct fwk_mutex_t mutex;
	
};
struct metacfg_entity_t {
	char *value;
};

#else



struct metacfg_t {
	int total;
	struct list_head biglist;
	struct list_head *kv_hash;	// kv_hash[KV_CONFIG_HASHSIZE] is alloc by kv_config_init, freed by kv_config_release
#ifdef USE_MUTEX_LOCK
	struct fwk_mutex_t mutex;
#endif	
};
struct metacfg_entity_t {
	struct list_head biglist_node;
	struct list_head hash_node;
	char *key;
	char *value;
	enum key_type_t key_type;
};

#endif
int
metacfg_adapter_config_init(struct metacfg_t *metacfg);
int metacfg_adapter_config_file_load_part(struct metacfg_t *metacfg, const char *filename, char *key1, char *key2);
int metacfg_adapter_config_file_load_safe(struct metacfg_t *metacfg, const char *filename);
char * metacfg_adapter_config_get_value(struct metacfg_t *metacfg, const char *key, char is_safe);
int
metacfg_adapter_config_release(struct metacfg_t *metacfg);
int
metacfg_adapter_config_merge(struct metacfg_t *metacfg, struct metacfg_t *metacfg_update, char is_add_new);
int
metacfg_adapter_config_entity_update(struct metacfg_t *metacfg, const char *key, const char *value);
int metacfg_adapter_config_diff(struct metacfg_t * kv_old, struct metacfg_t * kv_new, struct metacfg_t *kv_diff, char is_add_new);
int metacfg_adapter_config_dump(int fd, struct metacfg_t *kv_config, char *str);
int metacfg_adapter_config_file_save_safe(struct metacfg_t *kv_config, const char *filename);


int metacfg_adapter_config_file_save(struct metacfg_t *kv_config, const char *filename);

int metacfg_adapter_config_is_empty(struct metacfg_t *kv_config);


int metacfg_adapter_config_iterate (struct metacfg_t *kv_config,
		  int (*iterate_func) (void * kv_config, const char *key, const char *value, void *data),
		  void *data);


int metacfg_adapter_config_entity_add(struct metacfg_t *kv_config, const char *key, const char *value, char is_allow_overwrite);


struct metacfg_entity_t * metacfg_adapter_config_entity_locate(struct metacfg_t *kv_config, const char *key);

int metacfg_adapter_config_entity_del(struct metacfg_t *kv_config, const char *key);

int metacfg_adapter_config_count(struct metacfg_t *kv_config);

int metacfg_adapter_config_dump_value(int fd, struct metacfg_t *kv_config, char *key_str, int has_value);

int 
metacfg_adapter_config_file_load_group(struct metacfg_t *kv_config, char *filename, char *key_prefix);

int metacfg_adapter_config_file_get_value(const char *filename, const char *key, char *value);

#endif
#endif
