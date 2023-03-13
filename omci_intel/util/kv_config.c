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
 * File    : kv_config.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>
#include <linux/limits.h>
#include <sys/stat.h>
#include <sys/types.h>              

#include "util.h"
#include "url.h"
#include "kv_config.h"

#ifdef USE_MUTEX_LOCK
static struct fwk_mutex_t merge_mutex_g;
static struct fwk_mutex_t diff_mutex_g;
static int is_global_init_g = 0;
#else
#define fwk_mutex_create(x)
#define fwk_mutex_lock(x)
#define fwk_mutex_unlock(x)
#define fwk_mutex_destroy(x)
#endif

// retry to cover 'del B before rename A to B' behavior in kv_config_file_save_safe()
static int
kv_config_fopen_wait(const char *filename, int count, int wait_ms, const char *mode, FILE **fptr)
{
	int i;
	if (filename == NULL || count <= 0 || wait_ms <= 0 || mode == NULL) {
		dbprintf_kv_config(LOG_ERR, "parm err filename[%s] count(%d) wait_ms(%d) mode[%s]\n", filename, count, wait_ms, mode);
		return -1;
	}
	for (i=0; i<count; i++) {
		if ((*fptr = fopen(filename, mode)) != NULL) {
			return 0;
		}
		usleep(wait_ms*1000);
	}
	dbprintf_kv_config(LOG_ERR, "fopen %s err\n", filename);
	return 1;
}

static unsigned int
strhash(const char *s)
{
	unsigned int hashval;

	for (hashval = 0; *s != '\0'; s++)
		hashval = *s + 31 * hashval;

	return hashval % KV_CONFIG_HASHSIZE;
}

int
kv_config_init(struct kv_config_t *kv_config)
{
	int i;
	
#ifdef USE_MUTEX_LOCK
	if (!is_global_init_g) {
		is_global_init_g = 1;
		fwk_mutex_create(&merge_mutex_g);
		fwk_mutex_create(&diff_mutex_g);
	}
#endif

	fwk_mutex_create(&kv_config->mutex);
	fwk_mutex_lock(&kv_config->mutex);
	
	kv_config->total = 0;
	INIT_LIST_HEAD(&kv_config->biglist);
	kv_config->kv_hash = malloc_safe(sizeof(struct list_head) * KV_CONFIG_HASHSIZE);
	for (i = 0; i < KV_CONFIG_HASHSIZE; i++) {
		INIT_LIST_HEAD(&kv_config->kv_hash[i]);
	}

	fwk_mutex_unlock(&kv_config->mutex);
	return 0;
}

int
kv_config_release(struct kv_config_t *kv_config)
{
	struct kv_entity_t *pos, *temp;

	fwk_mutex_lock(&kv_config->mutex);

	list_for_each_entry_safe(pos, temp, &kv_config->biglist, biglist_node) {
		list_del(&pos->biglist_node);
		if (pos->key_type != type_comment) {
			list_del(&pos->hash_node);
			free_safe(pos->key);
		}
		free_safe(pos->value);
		free_safe(pos);
	}
	free_safe(kv_config->kv_hash);	

	kv_config->total = 0;
	
	fwk_mutex_unlock(&kv_config->mutex);
	fwk_mutex_destroy(&kv_config->mutex);

	return 0;
}

int
kv_config_is_empty(struct kv_config_t *kv_config)
{
	int ret;
	fwk_mutex_lock(&kv_config->mutex);
	ret = list_empty(&kv_config->biglist);
	fwk_mutex_unlock(&kv_config->mutex);
	return ret;
}

struct kv_entity_t *
kv_config_entity_locate(struct kv_config_t *kv_config, const char *key)
{
	int hash = strhash(key);
	struct kv_entity_t *pos, *temp;

	dbprintf_kv_config(LOG_DEBUG, "hash=%d\n", hash);
	fwk_mutex_lock(&kv_config->mutex);
	list_for_each_entry_safe(pos, temp, &kv_config->kv_hash[hash], hash_node) {
		dbprintf_kv_config(LOG_DEBUG, "hash=%d, key=%s\n", hash, key);
		if (strcmp(pos->key, key) == 0) {
			fwk_mutex_unlock(&kv_config->mutex);
			return pos;
		}
	}
	fwk_mutex_unlock(&kv_config->mutex);
	return NULL;
}

char *
kv_config_get_value(struct kv_config_t *kv_config, const char *key, char is_safe)
{
	struct kv_entity_t *kv_entity = kv_config_entity_locate(kv_config, key);

	if (kv_entity) {
		return kv_entity->value;
	} else {
		return (is_safe) ? "" : NULL;	//safe: return string
	}
}

int
kv_config_entity_del(struct kv_config_t *kv_config, const char *key)
{
	struct kv_entity_t *pos;

	if ((pos = kv_config_entity_locate(kv_config, key)) == NULL) {
		dbprintf_kv_config(LOG_ERR, "can't found key=%s\n", key);
		return -1;
	} else {
		int hash = strhash(key);
		dbprintf_kv_config(LOG_DEBUG, "entry_del, hash=%d\n", hash);

		fwk_mutex_lock(&kv_config->mutex);
		list_del(&pos->biglist_node);
		list_del(&pos->hash_node);
		kv_config->total--;
		fwk_mutex_unlock(&kv_config->mutex);

		free_safe(pos->key);
		free_safe(pos->value);
		free_safe(pos);
	}

	return 0;
}

/* add node to hash by key*/
int
kv_config_entity_add(struct kv_config_t *kv_config, const char *key, const char *value, char is_allow_overwrite)
{
	struct kv_entity_t *kv_entity;
	int hash = strhash(key);
	int tmplen;
	struct kv_entity_t *pos;

	dbprintf_kv_config(LOG_DEBUG, "add key=%s, value=%s, hash=%d\n", key, value, hash);
	if ((pos = kv_config_entity_locate(kv_config, key)) != NULL) {
		//overwrite value
		if (is_allow_overwrite) {
			if ((tmplen = strlen(value)) >= KV_CONFIG_MAXLEN) {
				dbprintf_kv_config(LOG_ERR, "value len %d, should < %d\n", tmplen, KV_CONFIG_MAXLEN);
				return -1;
			}
			//release old
			free_safe(pos->value);
			//overwrite with new
			pos->value = malloc_safe(tmplen + 1);
			strcpy(pos->value, value);
			return 0;
		} else {
			dbprintf_kv_config(LOG_ERR, "duplicate key=%s\n", key);
			return -1;
		}
	}
	//add new entity(key, value)
	kv_entity = malloc_safe(sizeof(struct kv_entity_t));
	if ((tmplen = strlen(key)) >= KV_CONFIG_MAXLEN) {
		free_safe(kv_entity);
		dbprintf_kv_config(LOG_ERR, "key len %d, should < %d\n", tmplen, KV_CONFIG_MAXLEN);
		return -1;
	}

	kv_entity->key = malloc_safe(tmplen + 1);
	strcpy(kv_entity->key, key);

	if ((tmplen = strlen(value)) >= KV_CONFIG_MAXLEN) {
		free_safe(kv_entity->key);
		free_safe(kv_entity);
		dbprintf_kv_config(LOG_ERR, "value len %d, should < %d\n", tmplen, KV_CONFIG_MAXLEN);
		return -1;
	}

	kv_entity->value = malloc_safe(tmplen + 1);
	kv_entity->key_type = type_normal;
	strcpy(kv_entity->value, value);

	fwk_mutex_lock(&kv_config->mutex);
	list_add_tail(&kv_entity->biglist_node, &kv_config->biglist);
	list_add_tail(&kv_entity->hash_node, &kv_config->kv_hash[hash]);
	kv_config->total++;
	fwk_mutex_unlock(&kv_config->mutex);
	
	return 0;
}

// <0: error, 0: no update required, >0: entries updated
int
kv_config_entity_update(struct kv_config_t *kv_config, const char *key, const char *value)
{
	struct kv_entity_t *pos;
	int tmplen;

	dbprintf_kv_config(LOG_DEBUG, "update=%s\n", key);
	if ((pos = kv_config_entity_locate(kv_config, key)) == NULL) {
		dbprintf_kv_config(LOG_INFO, "key=%s not found\n", key);
		return -1;
	}

	if ((tmplen = strlen(value)) >= KV_CONFIG_MAXLEN) {
		dbprintf_kv_config(LOG_ERR, "value len %d, should < %d\n", tmplen, KV_CONFIG_MAXLEN);
		return -1;
	}
	// do nothing if value are equal
	if (strcmp(pos->value, value) == 0)
		return 0;

	free_safe(pos->value);
	pos->value = malloc_safe(tmplen + 1);
	strcpy(pos->value, value);
	return 1;
}

/// load & save from file /////////////////////////////////////////////////////////////////////////////////////////////

static int
kv_config_file_load2(struct kv_config_t *kv_config, const char *filename, int is_load_anyway, char *key_prefix)
{
	char *strbuf = NULL;
	char *ch, *key_start, *value_start, *key = NULL, *value = NULL;
	FILE *fptr;
	int line_num = 0, ret = 0, key_prefix_len = 0;

	if (key_prefix == NULL) {
		key_prefix_len = 0;
		dbprintf_kv_config(LOG_DEBUG, "fopen[%s] key_prefix[NULL]\n", filename);
	} else {
		key_prefix_len = strlen(key_prefix);
		dbprintf_kv_config(LOG_DEBUG, "fopen[%s] key_prefix[%s]\n", filename, key_prefix);
	}
	// retry to cover 'del B before rename A to B' behavior in kv_config_file_save_safe()
	// sleep 20ms at most 2sec to force context switch implicitly
	if (0 != kv_config_fopen_wait(filename, 100, 20, "r", &fptr)) {
		dbprintf_kv_config(LOG_ERR, "fopen %s err\n", filename);
		ret = -1;
		goto exit;
	}
	strbuf = malloc_safe(KV_CONFIG_MAXLEN);
	while (fgets(strbuf, KV_CONFIG_MAXLEN, fptr) != NULL) {
		line_num++;

		key_start = util_trim(strbuf);
		if (key_start[0] == 0)
			continue;

		if (key_start[0] == '#') {
			struct kv_entity_t *kv_entity = malloc_safe(sizeof(struct kv_entity_t));
			int len;

			dbprintf_kv_config(LOG_DEBUG, "comment %s\n", key_start);
			if ((len = strlen(key_start)) >= KV_CONFIG_MAXLEN) {
				dbprintf_kv_config(LOG_ERR, "%s:%d: value len %d, should < %d\n", filename, line_num, len,
						 KV_CONFIG_MAXLEN);
				ret = -1;
				goto exit;
			}
			kv_entity->value = malloc_safe(len + 1);
			strcpy(kv_entity->value, key_start);
			kv_entity->key_type = type_comment;

			fwk_mutex_lock(&kv_config->mutex);			
			list_add_tail(&kv_entity->biglist_node, &kv_config->biglist);
			fwk_mutex_unlock(&kv_config->mutex);			
		} else if (key_prefix_len && 0 != strncmp(key_start, key_prefix, key_prefix_len)) {
			continue;
		} else {
			//dbprintf_kv_config(LOG_DEBUG, "[%s]\n", key_start);
			if ((ch = strchr(key_start, '=')) == NULL) {
				if (strlen(key_start) > 32) {
					key_start[32] = 0;
					dbprintf_kv_config(LOG_ERR, "%s:%d: '=' missing on line '%s ...'\n", filename,
							 line_num, key_start);
				} else {
					dbprintf_kv_config(LOG_ERR, "%s:%d: '=' missing on line '%s'\n", filename, line_num,
							 key_start);
				}
				if (is_load_anyway) {
					continue;
				} else {
					ret = -1;
					goto exit;
				}
			} else {
				value_start = ch + 1;
				*ch = 0x0;
				util_trim(key_start);
				key = url_unescape(key_start, strlen(key_start));
				util_trim(value_start);
				value = url_unescape(value_start, strlen(value_start));
				if (key && value) {
					if (kv_config_entity_add(kv_config, key, value, is_load_anyway) != 0) {
						ret = -1;
						goto exit;
					}
				} else {
					dbprintf_kv_config(LOG_ERR, "can't allocate memory\n");
					ret = -1;
					goto exit;
				}
				if (key) {
					free_safe(key);
					key = NULL;
				}
				if (value) {
					free_safe(value);
					value = NULL;
				}
			}
		}
	}

      exit:
	if (fptr) {
		dbprintf_kv_config(LOG_DEBUG, "fclose %s\n", filename);
		fclose(fptr);
	}
      	if (strbuf)
		free_safe(strbuf);
	if (key)
		free_safe(key);
	if (value)
		free_safe(value);
	return ret;
}

int
kv_config_file_load(struct kv_config_t *kv_config, const char *filename)
{
	return kv_config_file_load2(kv_config, filename, 0, NULL);
}

//if key is the same, allow overwrite
int
kv_config_file_load_safe(struct kv_config_t *kv_config, const char *filename)
{
	return kv_config_file_load2(kv_config, filename, 1, NULL);
}

// this is for fast loading part of a large kv config file.
// 1. only keys between ke1, key2 will be loaded
// 2. assume all key are not url-escaped, only values are url-escaped.
int
kv_config_file_load_part(struct kv_config_t *kv_config, const char *filename, char *key1, char *key2)
{
	char *strbuf = NULL;
	char *ch, *key_start, *value_start, *value = NULL;
	FILE *fptr;
	int line_num = 0, ret = 0;

	int is_in_part = 0;
	char key1buf[128];
	int key1len;

	if (key1) {
		snprintf(key1buf, 127, "%s=", key1);
		key1len = strlen(key1buf);
	} else {
		is_in_part = 1;
	}

	dbprintf_kv_config(LOG_DEBUG, "fopen %s\n", filename);
	// retry to cover 'del B before rename A to B' behavior in kv_config_file_save_safe()
	// sleep 20ms at most 2sec to force context switch implicitly
	if (0 != kv_config_fopen_wait(filename, 100, 20, "r", &fptr)) {
		dbprintf_kv_config(LOG_ERR, "fopen %s err\n", filename);
		ret = -1;
		goto exit;
	}
	strbuf = malloc_safe(KV_CONFIG_MAXLEN);
	while (fgets(strbuf, KV_CONFIG_MAXLEN, fptr) != NULL) {
		line_num++;

		key_start = util_trim(strbuf);
		if (key_start[0] == 0)
			continue;
		if (key_start[0] == '#')
			continue;

		if (is_in_part == 0) {
			if (strncmp(key_start, key1buf, key1len) == 0) {
				is_in_part = 1;
			} else {
				continue;
			}
		}

		if (is_in_part) {
			//dbprintf_kv_config(LOG_DEBUG, "[%s]\n", key_start);
			if ((ch = strchr(key_start, '=')) == NULL) {
				dbprintf_kv_config(LOG_ERR, "%s:%d: '=' missing on line '%s'\n", filename, line_num, key_start);
				continue;
			} else {
				value_start = ch + 1;
				*ch = 0x0;
				value = url_unescape(value_start, strlen(value_start));
				if (value) {
					if (kv_config_entity_add(kv_config, key_start, value, 1) != 0) {
						ret = -1;
						goto exit;
					}
				} else {
					dbprintf_kv_config(LOG_ERR, "can't allocate memory\n");
					ret = -1;
					goto exit;
				}
				if (value) {
					free_safe(value);
					value = NULL;
				}
				if (key2 && strcmp(key_start, key2) == 0)
					break;
			}
		}
	}

      exit:
	if (fptr) {
		dbprintf_kv_config(LOG_DEBUG, "fclose %s\n", filename);
		fclose(fptr);
	}
      	if (strbuf)
		free_safe(strbuf);
	if (value)
		free_safe(value);
	return ret;
}

int
kv_config_file_load_group(struct kv_config_t *kv_config, char *filename, char *key_prefix)
{
	return kv_config_file_load2(kv_config, filename, 1, key_prefix);
}

int
kv_config_file_save(struct kv_config_t *kv_config, const char *filename)
{
	struct kv_entity_t *pos;
	FILE *fptr;
	char *key = NULL, *value = NULL;
	int prev_line_is_comment = 1;

	dbprintf_kv_config(LOG_INFO, "fopen %s\n", filename);
	// retry to cover 'del B before rename A to B' behavior in kv_config_file_save_safe()
	// sleep 20ms at most 2sec to force context switch implicitly
	if (0 != kv_config_fopen_wait(filename, 100, 20, "w+", &fptr)) {
		dbprintf_kv_config(LOG_ERR, "fopen %s err\n", filename);
		return -1;
	}
	fwk_mutex_lock(&kv_config->mutex);
	list_for_each_entry(pos, &kv_config->biglist, biglist_node) {
		switch (pos->key_type) {
		case type_comment:
			if (prev_line_is_comment) {
				if (fprintf(fptr, "%s\n", pos->value) < 0)
					goto fprintf_error;
			} else {
				if (fprintf(fptr, "\n%s\n", pos->value) < 0)
					goto fprintf_error;
			}
			prev_line_is_comment = 1;
			break;
		case type_normal:
			key = url_escape(pos->key, strlen(pos->key));
			value = url_escape(pos->value, strlen(pos->value));
			if (key && value) {
				if (fprintf(fptr, "%s=%s\n", key, value) < 0) {
					free_safe(key);
					free_safe(value);
					goto fprintf_error;
				}
				prev_line_is_comment = 0;
			} else {
				dbprintf_kv_config(LOG_ERR, "can't allocate memory\n");
			}
			if (key)
				free_safe(key);
			if (value)
				free_safe(value);
			break;
		default:
			dbprintf_kv_config(LOG_ERR, "invalid: key=%s, value=%s\n", pos->key, pos->value);
		}
	}
	fwk_mutex_unlock(&kv_config->mutex);
	dbprintf_kv_config(LOG_INFO, "fclose %s\n", filename);
	fclose(fptr);
	return 0;
      fprintf_error:
      	fwk_mutex_unlock(&kv_config->mutex);
	dbprintf_kv_config(LOG_ERR, "fprintf %s error %d(%s)\n", filename, errno, strerror(errno));
	fclose(fptr);
	unlink(filename);
	return -1;
}

// save kv into tempfile and rename to filename
// this is to guarentee the update to filename is atomic,
// so other access won't see incomplete kv in filename
int
kv_config_file_save_safe(struct kv_config_t *kv_config, const char *filename)
{
	char tmpfile[PATHNAME_MAXLEN];
	int ret;

	// open/create a tempfile name based on filename, so they are in same dir
	snprintf(tmpfile, PATHNAME_MAXLEN, "%s-XXXXXX", filename);
	if ((ret = mkstemp(tmpfile)) < 0) {
		dbprintf_kv_config(LOG_ERR, "mkstemp %s failed, err=%s\n", tmpfile, strerror(errno));
		return ret;
	}
	close(ret);

	if ((ret = kv_config_file_save(kv_config, tmpfile)) != 0) {
		dbprintf_kv_config(LOG_ERR, "kv_config_file_save %s failed?\n", tmpfile);
		unlink(tmpfile);
		fflush(0);		// user space buf -> kernel buf
		sync();			// kernel buf -> disk
		return ret;
	}
	
	// ugly hack to workaround the minifo fs out of space issue:
	// if file A is renamed to B, then original file B space wont be released until next reboot
	// so we need to 'del B before rename A to B'
	unlink(filename);
	
	if ((ret = rename(tmpfile, filename)) != 0) {
		dbprintf_kv_config(LOG_ERR, "reanme [%s] to [%s], errno=%s\n", tmpfile, filename, strerror(errno));
		unlink(tmpfile);
	}
	fflush(0);		// user space buf -> kernel buf
	sync();			// kernel buf -> disk
	return ret;
}

/// misc print/dump function which might hold the lock for a long time/////////////////////////////////////////////////
int
kv_config_dump(int fd, struct kv_config_t *kv_config, char *str)
{
	struct kv_entity_t *pos;

	fwk_mutex_lock(&kv_config->mutex);
	list_for_each_entry(pos, &kv_config->biglist, biglist_node) {
		switch (pos->key_type) {
		case type_comment:
			if (str == NULL)
				util_fdprintf(fd, "%s\n", pos->value);
			break;
		case type_normal:
			if (str == NULL || strcasestr(pos->key, str))
				util_fdprintf(fd, "%s=%s\n", pos->key, pos->value);
			break;
		default:
			if (str == NULL)
				util_fdprintf(fd, "err: invalid: key=%s, value=%s\n", pos->key, pos->value);
		}
	}
	fwk_mutex_unlock(&kv_config->mutex);
	return 0;
}

int
kv_config_dump_value(int fd, struct kv_config_t *kv_config, char *key_str, int has_value)
{
	struct kv_entity_t *pos;

	fwk_mutex_lock(&kv_config->mutex);
	list_for_each_entry(pos, &kv_config->biglist, biglist_node) {
		switch (pos->key_type) {
		case type_comment:
			if (key_str == NULL)
				util_fdprintf(fd, "%s\n", pos->value);
			break;
		case type_normal:
			if ((key_str == NULL || strcasestr(pos->key, key_str)) && has_value && pos->value && strlen(pos->value)>0)
				util_fdprintf(fd, "%s=%s\n", pos->key, pos->value);
			break;
		default:
			if (key_str == NULL)
				util_fdprintf(fd, "err: invalid: key=%s, value=%s\n", pos->key, pos->value);
		}
	}
	fwk_mutex_unlock(&kv_config->mutex);
	return 0;
}
// if iterate function ret <0, then break the iteration immediately,
// or the iterate function will be called for each node in kv_config
int
kv_config_iterate(struct kv_config_t *kv_config,
		  int (*iterate_func) (void * kv_config, const char *key, const char *value, void *data),
		  void *data)
{
	struct kv_entity_t *pos;
	fwk_mutex_lock(&kv_config->mutex);
	list_for_each_entry(pos, &kv_config->biglist, biglist_node) {
		if (pos->key_type == type_normal) {
			if (iterate_func((void*)kv_config, pos->key, pos->value, data) < 0) {
				fwk_mutex_unlock(&kv_config->mutex);
				return 1;
			}
		}
	}
	fwk_mutex_unlock(&kv_config->mutex);
	return 0;
}

int
kv_config_print(struct kv_config_t *kv_config, int print_mode)
{
	char *key = NULL, *value = NULL;
	struct kv_entity_t *pos;

	fwk_mutex_lock(&kv_config->mutex);
	list_for_each_entry(pos, &kv_config->biglist, biglist_node) {
		switch (pos->key_type) {
		case type_comment:
			printf("%s\n", pos->value);
			break;
		case type_normal:
			if (print_mode == KV_CONFIG_PRINT_URL_ESCAPED) {
				key = url_escape(pos->key, strlen(pos->key));
				value = url_escape(pos->value, strlen(pos->value));
				if (key && value) {
					printf("%s=%s\n", key, value);
				} else {
					dbprintf_kv_config(LOG_ERR, "err: can't allocate memory\n");
				}
				if (key)
					free_safe(key);
				if (value)
					free_safe(value);
			} else if (print_mode == KV_CONFIG_PRINT_SHELL_ESCAPED) {
				printf("%s=\"%s\"\n", pos->key, util_str_shell_escape(pos->value));
			} else {	// KV_CONFIG_PRINT_URL_UNESCAPED or others
				printf("%s=%s\n", pos->key, pos->value);
			}
			break;
		default:
			dbprintf_kv_config(LOG_ERR, "err: invalid: key=%s, value=%s\n",
				      pos->key, pos->value);
		}
	}
	fwk_mutex_unlock(&kv_config->mutex);
	return 0;
}

int
kv_config_print_by_key(struct kv_config_t *kv_config, int print_mode, const char *key)
{
	char *value = kv_config_get_value(kv_config, key, 1);

	if (print_mode == KV_CONFIG_PRINT_URL_ESCAPED) {
		char *escaped_key = url_escape(key, strlen(key));
		char *escaped_value = url_escape(value, strlen(value));
		if (escaped_key && escaped_value)
			printf("%s=%s\n", escaped_key, escaped_value);
		if (escaped_key)
			free_safe(escaped_key);
		if (escaped_value)
			free_safe(escaped_value);
	} else if (print_mode == KV_CONFIG_PRINT_SHELL_ESCAPED) {
		printf("%s=\"%s\"\n", key, util_str_shell_escape(value));
	} else {		// KV_CONFIG_PRINT_URL_UNESCAPED or others
		printf("%s=%s\n", key, value);
	}
	return 0;
}

/// merge & diff function, which operates multiple kv_config at the same time /////////////////////////////////////////////////
/// to avoid dead lock between different thread, we put lock for the merge/diff function itself ///////////////////////////////

// return value >= 0 means kv_base update/add count
int
kv_config_merge(struct kv_config_t *kv_base, struct kv_config_t *kv_update, char is_add_new)
{
	int ret, count = 0;
	struct kv_entity_t *pos, *temp;

	fwk_mutex_lock(&merge_mutex_g);
	fwk_mutex_lock(&kv_update->mutex);
	
	list_for_each_entry_safe(pos, temp, &kv_update->biglist, biglist_node) {

		if (pos->key_type != type_normal)
			continue;

		ret = kv_config_entity_update(kv_base, pos->key, pos->value);
		if (ret > 0) {	// update ok
			count++;
		} else if (ret == 0) {
			// no change, do nothing
		} else if (ret < 0 && is_add_new) {	//need to add new
			dbprintf_kv_config(LOG_INFO, "add new %s=%s\n", pos->key, pos->value);
			if (kv_config_entity_add(kv_base, pos->key, pos->value, 0) != 0) {
				dbprintf_kv_config(LOG_ERR, "kv_config_entity_add err %s=%s\n", pos->key, pos->value);
				count = -1;
				goto end_merge;
			}
			count++;
		}
	}

      end_merge:
	fwk_mutex_unlock(&kv_update->mutex);
	fwk_mutex_unlock(&merge_mutex_g);
	return count;
}

// return value >= 0 means kv_diff entry count
int
kv_config_diff(struct kv_config_t *kv_old, struct kv_config_t *kv_new, struct kv_config_t *kv_diff, char is_add_new)
{
	struct kv_entity_t *pos, *temp, *tmp_target;
	int count = 0;

	fwk_mutex_lock(&diff_mutex_g);
	fwk_mutex_lock(&kv_new->mutex);

	list_for_each_entry_safe(pos, temp, &kv_new->biglist, biglist_node) {

		if (pos->key_type != type_normal) {
			continue;
		}

		if ((tmp_target = kv_config_entity_locate(kv_old, pos->key)) == NULL) {	//can't locate
			dbprintf_kv_config(LOG_DEBUG, "key=%s not found\n", pos->key);
			//need to add new
			if (is_add_new) {
				dbprintf_kv_config(LOG_DEBUG, "add new %s=%s\n", pos->key, pos->value);
				if (kv_config_entity_add(kv_diff, pos->key, pos->value, 0) != 0) {
					dbprintf_kv_config(LOG_ERR, "kv_config_entity_add err %s=%s\n", pos->key, pos->value);
					count = -1;
					goto end_diff;
				}
				count++;
			}

		} else if (strcmp(tmp_target->value, pos->value) != 0) {	//different
			if (kv_config_entity_add(kv_diff, pos->key, pos->value, 1) != 0) {
				dbprintf_kv_config(LOG_ERR, "kv_config_entity_add err %s=%s\n", pos->key, pos->value);
				count = -1;
				goto end_diff;
			}
			count++;
		}
	}

      end_diff:
	fwk_mutex_unlock(&kv_new->mutex);
	fwk_mutex_unlock(&diff_mutex_g);

	return count;
}

int
kv_config_file_get_value(const char *filename, const char *key, char *value) {
	struct kv_config_t kv_config;
	kv_config_init(&kv_config);
	char *str;
	
	if(kv_config_file_load_safe(&kv_config, filename) !=0)
	{
		dbprintf(LOG_ERR, "load kv_config error\n");
		kv_config_release(&kv_config);
		return -1;
	}
	
	if(strlen(str = kv_config_get_value(&kv_config, key, 1)) > 0)
		memcpy(value, str, strlen(str)+1 );

	kv_config_release(&kv_config);
	return strlen(value);
}

int
kv_config_count(struct kv_config_t *kv_config)
{
	int ret=0;
	struct kv_entity_t *pos;
	fwk_mutex_lock(&kv_config->mutex);
	list_for_each_entry(pos, &kv_config->biglist, biglist_node) {
		switch (pos->key_type) {
		case type_normal:
			ret++;
			break;
		default:
			break;
		}
	}
	fwk_mutex_unlock(&kv_config->mutex);
	return ret;
}
