/******************************************************************
 *
 * Copyright (C) 2014 5V Technologies Ltd.
 * All Rights Reserved.
 *
 * This program is the proprietary software of 5V Technologies Ltd
 * and/or its licensors, and only be used, duplicated,
 * modified or distributed under the authorized license from 5VT.
 *
 * ANY USE OF THE SOFTWARE OTHER THAN AS AUTHORIZED UNDER
 * THIS LICENSE OR COPYRIGHT LAW IS PROHIBITED.
 *
 * Purpose : 5VT WWW Toolkit
 * Module  : libweb
 * File    : kv_config.h
 *
 ******************************************************************/

#ifndef __KV_CONFIG_H__
#define __KV_CONFIG_H__

// comment out the following line if mutex lock is not required
#define USE_MUTEX_LOCK

#ifdef USE_MUTEX_LOCK
#include "fwk_mutex.h"
#endif

#define KV_CONFIG_MAXLEN	65536
#define KV_CONFIG_HASHSIZE	1023

#define KV_CONFIG_UPDATE_ONLY	0
#define KV_CONFIG_UPDATE_ADD	1

#define KV_CONFIG_PRINT_URL_UNESCAPED	0
#define KV_CONFIG_PRINT_URL_ESCAPED	1
#define KV_CONFIG_PRINT_SHELL_ESCAPED	2

// customize dbprintf_kv_config to local debug printf function
#define dbprintf_kv_config(level, fmt, args...)	dbprintf_fwk(level, fmt, ##args)

#include "list.h"

struct kv_config_t {
	int total;
	struct list_head biglist;
	struct list_head *kv_hash;	// kv_hash[KV_CONFIG_HASHSIZE] is alloc by kv_config_init, freed by kv_config_release
#ifdef USE_MUTEX_LOCK
	struct fwk_mutex_t mutex;
#endif	
};

enum key_type_t {
	type_comment,
	type_normal,
	type_invalid
};

struct kv_entity_t {
	struct list_head biglist_node;
	struct list_head hash_node;
	char *key;
	char *value;
	enum key_type_t key_type;
};

struct template_info_t {
	char *head;
	char *tail;
	char *key;
	char *value;
	struct list_head node;
};

/* kv_config.c */
int kv_config_init(struct kv_config_t *kv_config);
int kv_config_is_empty(struct kv_config_t *kv_config);
struct kv_entity_t *kv_config_entity_locate(struct kv_config_t *kv_config, const char *key);
char *kv_config_get_value(struct kv_config_t *kv_config, const char *key, char is_safe);
int kv_config_file_get_value(const char *filename, const char *key, char *value);
int kv_config_entity_del(struct kv_config_t *kv_config, const char *key);
int kv_config_entity_add(struct kv_config_t *kv_config, const char *key, const char *value, char is_allow_overwrite);
int kv_config_entity_update(struct kv_config_t *kv_config, const char *key, const char *value);
int kv_config_file_load(struct kv_config_t *kv_config, const char *filename);
int kv_config_file_load_safe(struct kv_config_t *kv_config, const char *filename);
int kv_config_file_load_part(struct kv_config_t *kv_config, const char *filename, char *key1, char *key2);
int kv_config_file_load_group(struct kv_config_t *kv_config, char *filename, char *key_prefix);
int kv_config_file_save(struct kv_config_t *kv_config, const char *filename);
int kv_config_file_save_safe(struct kv_config_t *kv_config, const char *filename);
int kv_config_release(struct kv_config_t *kv_config);
int kv_config_dump(int fd, struct kv_config_t *kv_config, char *str);
int kv_config_dump_value(int fd, struct kv_config_t *kv_config, char *key_str, int has_value);
int kv_config_iterate(struct kv_config_t *kv_config, int (*iterate_func)(void *kv_config, const char *key, const char *value, void *data), void *data);
int kv_config_print(struct kv_config_t *kv_config, int print_mode);
int kv_config_print_by_key(struct kv_config_t *kv_config, int print_mode, const char *key);
int kv_config_merge(struct kv_config_t *kv_base, struct kv_config_t *kv_update, char is_add_new);
int kv_config_diff(struct kv_config_t *kv_old, struct kv_config_t *kv_new, struct kv_config_t *kv_diff, char is_add_new);
int kv_config_count(struct kv_config_t *kv_config);

#endif
