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
#ifdef USE_UCI_CONFIG
#include "metacfg_uci.h"
#endif

#ifdef OMCI_METAFILE_ENABLE
int
metacfg_adapter_config_init(struct metacfg_t *metacfg)
{
#ifdef USE_UCI_CONFIG
	if(!metacfg->is_init){
		memset(metacfg,0x0,sizeof(struct metacfg_t));
		fwk_mutex_create(&metacfg->mutex);
		fwk_mutex_lock(&metacfg->mutex);
		
		metacfg->total = 0;
		metacfg->is_init = 1;
		//INIT_LIST_HEAD(&metacfg->biglist);
		metacfg->string_val =  malloc_safe(METACFG_UCI_STRING_LEN);
		fwk_mutex_unlock(&metacfg->mutex);
	}
	return 0;
#else
	return kv_config_init((struct kv_config_t*)metacfg);
#endif
}

int metacfg_adapter_config_file_load_part(struct metacfg_t *metacfg, const char *filename, char *key1, char *key2)
{
#ifdef USE_UCI_CONFIG
	int ret=0;
	if( key1 ||key2){}
	
	fwk_mutex_lock(&metacfg->mutex);
	ret = metacfg_uci_config_file_load_safe(&metacfg->uci_ctx,&metacfg->uci_pkg,filename);
	fwk_mutex_unlock(&metacfg->mutex);
	return ret;
	return 0;
#else
	return kv_config_file_load_part((struct kv_config_t*)metacfg,filename,key1,key2);
#endif
}

int metacfg_adapter_config_file_load_safe(struct metacfg_t *metacfg, const char *filename)
{
#ifdef USE_UCI_CONFIG
	int ret=0;
	fwk_mutex_lock(&metacfg->mutex);
	ret = metacfg_uci_config_file_load_safe(&metacfg->uci_ctx,&metacfg->uci_pkg,filename);
	fwk_mutex_unlock(&metacfg->mutex);
	return ret;

#else
	return kv_config_file_load_safe((struct kv_config_t*)metacfg,filename);
#endif
}

char * metacfg_adapter_config_get_value(struct metacfg_t *metacfg, const char *key, char is_safe)
{
#ifdef USE_UCI_CONFIG
	int ret;
	fwk_mutex_lock(&metacfg->mutex);
	
	memset(metacfg->string_val,0x0,METACFG_UCI_STRING_LEN);
	ret = metacfg_uci_get_config_string_by_key(key,metacfg->string_val);

	fwk_mutex_unlock(&metacfg->mutex);

	if(ret < 0){
		return (is_safe) ? "" : NULL;
	}
	
	if(!strlen(metacfg->string_val))
		return (is_safe) ? "" : NULL;
	return metacfg->string_val;
#else
	return kv_config_get_value((struct kv_config_t*)metacfg,key,is_safe);
#endif

}

int metacfg_adapter_config_release(struct metacfg_t *metacfg)
{
#ifdef USE_UCI_CONFIG
	if(metacfg->is_init){
		fwk_mutex_lock(&metacfg->mutex);
		free_safe(metacfg->string_val);	
		
		if(metacfg->uci_ctx && metacfg->uci_pkg ){
			//metacfg_uci_config_save(metacfg->uci_ctx,metacfg->uci_pkg);
			metacfg_uci_release(metacfg->uci_ctx,metacfg->uci_pkg);
		}
		
		metacfg->total = 0;
		metacfg->is_init = 0;
		metacfg->uci_ctx = NULL;
		metacfg->uci_pkg = NULL;
		fwk_mutex_unlock(&metacfg->mutex);
		fwk_mutex_destroy(&metacfg->mutex);
	}
	return 0;
#else
	return kv_config_release((struct kv_config_t*)metacfg);
#endif
	
}

int
metacfg_adapter_config_merge(struct metacfg_t *metacfg, struct metacfg_t *metacfg_update, char is_add_new)
{
#ifdef USE_UCI_CONFIG
	int ret;
	if( is_add_new){}
	fwk_mutex_lock(&metacfg->mutex);
	if(metacfg->uci_pkg == NULL && metacfg->uci_ctx == NULL){
		if(metacfg_uci_init_by_cppkg(metacfg_update->uci_pkg,&metacfg->uci_ctx,&metacfg->uci_pkg) < 0){
			dbprintf(LOG_ERR, "metacfg_adapter_config_merge fail \n");
			fwk_mutex_unlock(&metacfg->mutex);
			return -1;
		}	
	}
	ret = metacfg_uci_config_merge(metacfg->uci_pkg,metacfg_update->uci_pkg);
	fwk_mutex_unlock(&metacfg->mutex);
	return ret;
#else /*use 5vt meta data*/
	return kv_config_merge((struct kv_config_t*)metacfg,(struct kv_config_t*)metacfg_update,is_add_new);	
#endif

}

int
metacfg_adapter_config_entity_update(struct metacfg_t *metacfg, const char *key, const char *value)
{
#ifdef USE_UCI_CONFIG
	fwk_mutex_lock(&metacfg->mutex);

	if(!metacfg->uci_ctx && !metacfg->uci_pkg)
	{
		if(metacfg_uci_init(key,&metacfg->uci_ctx,&metacfg->uci_pkg)<0){
			dbprintf(LOG_ERR, "metacfg_uci_init fail (%s)\n", key);
			return -1;
		}
		
	}
	if(metacfg_uci_set_config_string(metacfg->uci_ctx,key,value) < 0){
		dbprintf(LOG_ERR, "metacfg_uci_set_config_string fail (%s-%s)\n", key,value);
		return -1;
		
	}
	metacfg->total++;
	fwk_mutex_unlock(&metacfg->mutex);
	return 0;
#else /*use 5vt meta data*/
	return kv_config_entity_update((struct kv_config_t*)metacfg,key,value);	
#endif

}
int metacfg_adapter_config_diff(struct metacfg_t * kv_old, struct metacfg_t * kv_new, struct metacfg_t *kv_diff, char is_add_new)
{
#ifdef USE_UCI_CONFIG
	int ret = 0 ; 
	/*due to old kv is load frome metadata file*/
	if(kv_old || is_add_new) {}

	fwk_mutex_lock(&kv_diff->mutex);
	if(!kv_new->uci_pkg || !kv_new->uci_ctx){
		dbprintf(LOG_ERR, "metacfg_adapter_config_diff fail NULL\n");
		fwk_mutex_unlock(&kv_diff->mutex);
		return -1;
	}

	if(!kv_diff->uci_pkg && !kv_diff->uci_ctx)
		ret = metacfg_uci_init_by_cppkg(kv_new->uci_pkg,&kv_diff->uci_ctx,&kv_diff->uci_pkg);

	if(ret < 0){
		dbprintf(LOG_ERR, "metacfg_uci_init_by_cppkg fail \n");
		fwk_mutex_unlock(&kv_diff->mutex);
		return -1;
	}
	ret = metacfg_uci_list_configs_copy(kv_new->uci_pkg,kv_diff->uci_ctx);

	if(ret < 0){
		dbprintf(LOG_ERR, "metacfg_uci_list_configs_copy fail \n");
		fwk_mutex_unlock(&kv_diff->mutex);
		return -1;
	}
	fwk_mutex_unlock(&kv_diff->mutex);
	return ret;

#else
	return kv_config_diff((struct kv_config_t*)kv_old,(struct kv_config_t*)kv_new,(struct kv_config_t*)kv_diff,is_add_new);
#endif

}

int metacfg_adapter_config_dump(int fd, struct metacfg_t *kv_config, char *str)
{
#ifdef USE_UCI_CONFIG
	return 0;
#else
	return kv_config_dump(fd,(struct kv_config_t *)kv_config,str);
#endif
}

int metacfg_adapter_config_file_save_safe(struct metacfg_t *kv_config, const char *filename)
{
#ifdef USE_UCI_CONFIG
	int ret = 0;
	fwk_mutex_lock(&kv_config->mutex);
	if(kv_config->uci_ctx && kv_config->uci_pkg)
		 metacfg_uci_config_save(kv_config->uci_ctx,kv_config->uci_pkg);
	fwk_mutex_unlock(&kv_config->mutex);
	return ret;
#else
	return kv_config_file_save_safe((struct kv_config_t *)kv_config,filename);
#endif


}
int metacfg_adapter_config_file_save(struct metacfg_t *kv_config, const char *filename)
{
#ifdef USE_UCI_CONFIG
	int ret = 0;
	fwk_mutex_lock(&kv_config->mutex);
	if(kv_config->uci_ctx && kv_config->uci_pkg)
		metacfg_uci_config_save(kv_config->uci_ctx,kv_config->uci_pkg);
	fwk_mutex_unlock(&kv_config->mutex);
	return ret;
#else
	return kv_config_file_save((struct kv_config_t *)kv_config,filename);
#endif
}
int metacfg_adapter_config_is_empty(struct metacfg_t *kv_config)
{
	
#ifdef USE_UCI_CONFIG
	int ret;
	fwk_mutex_lock(&kv_config->mutex);
	ret =  metacfg_uci_config_is_empty(kv_config->uci_ctx,kv_config->uci_pkg);
	fwk_mutex_unlock(&kv_config->mutex);
	return ret;
#else
	return kv_config_is_empty((struct kv_config_t *)kv_config);
#endif
}

int metacfg_adapter_config_iterate (struct metacfg_t *kv_config,
		  int (*iterate_func) (void * kv_config, const char *key, const char *value, void *data),
		  void *data)
{
#ifdef USE_UCI_CONFIG
	int ret;
	fwk_mutex_lock(&kv_config->mutex);
	ret =  metacfg_uci_config_iterate(kv_config,kv_config->uci_pkg,iterate_func,data);
	fwk_mutex_unlock(&kv_config->mutex);
	return ret;
#else
	return kv_config_iterate((struct kv_config_t *)kv_config,iterate_func,data);
#endif
}

int metacfg_adapter_config_entity_add(struct metacfg_t *kv_config, const char *key, const char *value, char is_allow_overwrite)
{

#ifdef USE_UCI_CONFIG
	int ret;
	fwk_mutex_lock(&kv_config->mutex);
	ret =  metacfg_adapter_config_entity_update(kv_config,key,value);
	fwk_mutex_unlock(&kv_config->mutex);
	return ret;
#else
	return kv_config_entity_add((struct kv_config_t *)kv_config,key,value,is_allow_overwrite);
#endif
}

struct metacfg_entity_t * metacfg_adapter_config_entity_locate(struct metacfg_t *kv_config, const char *key)
{
#ifdef USE_UCI_CONFIG
	struct metacfg_entity_t* ret_enty;
	fwk_mutex_lock(&kv_config->mutex);
	ret_enty = (struct metacfg_entity_t*) metacfg_uci_config_entity_locate(kv_config->uci_pkg,key);
	fwk_mutex_unlock(&kv_config->mutex);
	return ret_enty;
#else
	return (struct metacfg_entity_t*)kv_config_entity_locate((struct kv_config_t *)kv_config,key);
#endif

}

int metacfg_adapter_config_entity_del(struct metacfg_t *kv_config, const char *key)
{
#ifdef USE_UCI_CONFIG
	int ret;
	fwk_mutex_lock(&kv_config->mutex);
	
	ret = metacfg_uci_config_entity_entity_del(kv_config->uci_pkg,key);
	
	fwk_mutex_unlock(&kv_config->mutex);
	return ret;
#else
	return kv_config_entity_del((struct kv_config_t *)kv_config,key);
#endif
}
int metacfg_adapter_config_count(struct metacfg_t *kv_config)
{
#ifdef USE_UCI_CONFIG
	int ret;
	fwk_mutex_lock(&kv_config->mutex);
	ret = metacfg_uci_config_count(kv_config->uci_pkg);
	fwk_mutex_unlock(&kv_config->mutex);
	return ret;
#else
	return kv_config_count((struct kv_config_t *)kv_config);
#endif
}
int metacfg_adapter_config_dump_value(int fd, struct metacfg_t *kv_config, char *key_str, int has_value)
{
#ifdef USE_UCI_CONFIG
	if(fd||kv_config||key_str||has_value){}
	return 0;
#else
	return kv_config_dump_value(fd,(struct kv_config_t *)kv_config,key_str,has_value);
#endif	

}
int 
metacfg_adapter_config_file_load_group(struct metacfg_t *kv_config, char *filename, char *key_prefix)
{
#ifdef USE_UCI_CONFIG
	int ret = 0 ;
	fwk_mutex_lock(&kv_config->mutex);
	if(!kv_config->uci_ctx && kv_config->uci_pkg){
		ret = metacfg_uci_init_by_prefixkey(&kv_config->uci_ctx,&kv_config->uci_pkg,key_prefix);
		if(ret <0){
			dbprintf(LOG_ERR, "metacfg_uci_init_by_prefixkey fail \n");
			fwk_mutex_unlock(&kv_config->mutex);
			return -1;
		}
	}
	ret = metacfg_uci_config_file_load_by_prefixkey(kv_config->uci_pkg,key_prefix);
	fwk_mutex_unlock(&kv_config->mutex);	
	return ret;
#else
	return kv_config_file_load_group((struct kv_config_t *)kv_config,filename,key_prefix);
#endif		

}
int metacfg_adapter_config_file_get_value(const char *filename, const char *key, char *value)
{
#ifdef USE_UCI_CONFIG
	int ret = 0 ;
	ret = metacfg_uci_get_config_string_by_key(key,value);
	if(ret < 0)
		return 0;
	return strlen(value);

#else
	return kv_config_file_get_value(filename,key,value);
#endif		
}
#endif

