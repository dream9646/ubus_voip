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


#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <string.h>
#include <errno.h>
#include <uci.h>

#include "util.h"
#include "util_uci.h"

static int __util_option_get(void *ctx, const char *value)
{
	char *out = ctx;

	if (!out || strlen(value) > UTIL_UCI_PARAM_STR_MAX_SIZE)
		return -1;

	strcpy(out, value);
	return 0;
}
static int __util_uci_tuple2ptr(void* uci_ctx,const char *path,
			 const char *sec,
			 const char *opt,
			 struct uci_ptr *ptr)
{
	char tuple[256];
	int len;

	if (!path || !sec || !ptr)
		return -1;

	memset(ptr, 0, sizeof(struct uci_ptr));

	if (opt)
		len = snprintf(tuple, sizeof(tuple), "%s.%s.%s",
			       path, sec, opt);
	else
		len = snprintf(tuple, sizeof(tuple), "%s.%s", path, sec);

	if (len > (int)sizeof(tuple))
		return -1;

	if (uci_lookup_ptr(uci_ctx, ptr, tuple, true) != UCI_OK)
		return -1;

	return 0;
}
			 
void util_uci_free(void * uci_ctx,void * uci_pkg)
{
	struct uci_context *tmp_uci_ctx = (struct uci_context *)uci_ctx;
	struct uci_package *tmp_net_pkg = (struct uci_package *)uci_pkg;
	
	if (tmp_net_pkg) {
		uci_unload(tmp_uci_ctx, tmp_net_pkg);
	}
	if (tmp_uci_ctx) {
		uci_free_context(tmp_uci_ctx);
	}
}

int  util_uci_init_by_cppkg(void * uci_pkg_ori, void ** uci_ctx,void ** uci_pkg)
{
	int ret = 0;	
	struct uci_context *tmp_uci_ctx = NULL;
	struct uci_package *tmp_net_pkg = NULL;
	struct uci_package *tmp_uci_pkg_ori =  (struct uci_package*) uci_pkg_ori;

	if(!tmp_uci_pkg_ori->e.name)
		return -1;
	
	tmp_uci_ctx = uci_alloc_context();
	
	if (!tmp_uci_ctx)
		return -1;
	if (!tmp_net_pkg)
		ret = uci_load(tmp_uci_ctx, tmp_uci_pkg_ori->e.name, &tmp_net_pkg);

	if (ret) {
		util_uci_free((void*)tmp_uci_ctx,(void*)tmp_net_pkg);
		return -1;
	}

	*uci_ctx = (void*)tmp_uci_ctx;
	*uci_pkg = (void*)tmp_uci_ctx;
	return 0;
}

int util_uci_init(const char *path, void ** uci_ctx,void ** uci_pkg)
{
	int ret = 0;	
	struct uci_context *tmp_uci_ctx = NULL;
	struct uci_package *tmp_net_pkg = NULL;
	
	tmp_uci_ctx = uci_alloc_context();
	
	if (!tmp_uci_ctx)
		return -1;
	if (!tmp_net_pkg)
		ret = uci_load(tmp_uci_ctx, path, &tmp_net_pkg);

	if (ret) {
		util_uci_free((void*)tmp_uci_ctx,(void*)tmp_net_pkg);
		return -1;
	}

	*uci_ctx = (void*)tmp_uci_ctx;
	*uci_pkg = (void*)tmp_net_pkg;
	return 0;	
}

int util_uci_config_get(const char *path, const char *sec, const char *opt,
			char *out)
{
	return util_uci_config_generic_get(path, sec, opt, out, __util_option_get);
}

int util_uci_config_generic_get(const char *path,
				const char *sec,
				const char *opt,
				void *ctx,
				int (*getter)(void *ctx, const char *value))
{
	struct uci_context *uci;
	char tuple[UTIL_UCI_TUPLE_STR_MAX_SIZE];
	struct uci_ptr ptr;
	struct uci_element *element;
	int len, ret = 0;

	memset(tuple, 0x0, sizeof(tuple));
	if (!getter)
		return -1;

	if (path && sec) {
		len = sprintf(tuple, "%s.%s", path, sec);
		if (len < 0)
			return -1;
	} else {
		return -1;
	}

	if (opt)
		if (sprintf(&tuple[len], ".%s", opt) < 0)
			return -1;

	uci = uci_alloc_context();
	if (!uci)
		return -1;

	if (uci_lookup_ptr(uci, &ptr, tuple, true) != UCI_OK) {
		ret = -1;
		goto on_exit;
	}

	if (!(ptr.flags & UCI_LOOKUP_COMPLETE)) {
		ret = -1;
		goto on_exit;
	}

	switch (ptr.last->type) {
	case UCI_TYPE_SECTION:
		ret = getter(ctx, ptr.s->type);
		break;
	case UCI_TYPE_OPTION:
		switch (ptr.o->type) {
		case UCI_TYPE_STRING:
			ret = getter(ctx, ptr.o->v.string);
			break;
		case UCI_TYPE_LIST:
			uci_foreach_element(&ptr.o->v.list, element) {
				ret = getter(ctx, element->name);
				if (ret != 0)
					break;
			}
			break;
		default:
			ret = -1;
			break;
		}
		break;
	default:
		ret = -1;
		break;
	}

on_exit:
	uci_free_context(uci);
	return ret;
}
				
int util_uci_config_set(void* uci_ctx,const char *path, const char *sec, const char *opt,
			   const char *val, int add_list)
{
	struct uci_ptr ptr;
	int ret;
	struct uci_context *tmp_uci_ctx = (struct uci_context *)uci_ctx;

	ret = __util_uci_tuple2ptr(uci_ctx,path, sec, opt, &ptr);
	if (ret)
		return ret;

	switch (ptr.last->type) {
	case UCI_TYPE_SECTION:
	case UCI_TYPE_OPTION:
		break;
	default:
		return -1;
	}

	ptr.package = path;
	ptr.section = sec;
	ptr.option = opt;
	ptr.value = val;

	if (add_list)
		return uci_add_list(tmp_uci_ctx, &ptr);
	else
		return uci_set(tmp_uci_ctx, &ptr);
}

void util_uci_config_save(void* uci_ctx,void* uci_pkg)
{
	struct uci_context *tmp_uci_ctx = (struct uci_context *)uci_ctx;
	struct uci_package *tmp_net_pkg = (struct uci_package *)uci_pkg;
	uci_save(tmp_uci_ctx, tmp_net_pkg);
}
int util_uci_config_is_empty(void* uci_ctx,void* uci_pkg)
{
	struct uci_element *e;
	struct uci_package *p = (struct uci_package*)uci_pkg;
	int count_idx=0;
	
	uci_foreach_element(&p->delta, e) {
		struct uci_delta *h = uci_to_delta(e);
		
		switch(h->cmd) {
			case UCI_CMD_ADD:
			case UCI_CMD_REMOVE:
			case UCI_CMD_CHANGE:
			case UCI_CMD_LIST_ADD:			
			case UCI_CMD_LIST_DEL:
			case UCI_CMD_RENAME:
			case UCI_CMD_REORDER:
				count_idx++;
				break;
			default:
				break;
		}
		
	}

	if(count_idx)
		return 0;

	return 1;
}

int util_uci_config_iterate(void* meta_cfg, void *uci_pkg,
		  	    int (*iterate_func) (void * kv_config, const char *key, const char *value, void *data),
		  	    void *data)
{
	struct uci_element *e,*tmp_e;
	struct uci_package *p = (struct uci_package*)uci_pkg;
	
	

	uci_foreach_element_safe(&p->delta,tmp_e, e) {

		struct uci_delta *h = uci_to_delta(e);

		switch(h->cmd) {
			case UCI_CMD_ADD:
			case UCI_CMD_CHANGE:
				if (!e->name){
					dbprintf(LOG_ERR, "uci copy no option name %s.%s\n", p->e.name, h->section);
					continue;
				}
				if (iterate_func((void*)meta_cfg, e->name, h->value, data) < 0){
					dbprintf(LOG_ERR, "iterate_func ERR %s.%s.%s=%s", p->e.name, h->section,e->name, h->value);
					continue;
				}
			break;
		
			default:
			break;
		}

	
	}
	return 0;

}
int util_uci_list_configs_copy(void* uci_pkg_old,void* uci_ctx_new)
{

	struct uci_element *e,*tmp_e;
	struct uci_package *p = (struct uci_package*)uci_pkg_old;
	int add_list = 0,need_cpy=0;
	int ret,error=0;
	
	uci_foreach_element_safe(&p->delta,tmp_e, e) {
		add_list = 0,need_cpy = 0;
		struct uci_delta *h = uci_to_delta(e);

		switch(h->cmd) {
			case UCI_CMD_ADD:
			case UCI_CMD_CHANGE:
				add_list = 0;
				need_cpy = 1;
				break;
			case UCI_CMD_LIST_ADD:
				add_list = 1;
				need_cpy = 1;
				break;
			
			default:
				break;
		}
		if(need_cpy){
			if (!e->name){
				dbprintf(LOG_ERR, "uci copy no option name %s.%s\n", p->e.name, h->section);
				continue;
			}
			
			ret = util_uci_config_set(uci_ctx_new,p->e.name,h->section,e->name,h->value,add_list);
			if(ret < 0){
				dbprintf(LOG_ERR, "uci copy ERR %s.%s.%s=%s", p->e.name, h->section,e->name, h->value);
				error = ret;
				continue;
			}
		}
		dbprintf(LOG_INFO, "uci copy %s.%s.%s=%s", p->e.name, h->section,e->name, h->value);		
	}
	return error;
}
void* util_uci_config_entity_locate(void* uci_pkg,const char *key)
{
	struct uci_element *e,*tmp_e;
	struct uci_package *p = (struct uci_package*)uci_pkg;	

	uci_foreach_element_safe(&p->delta,tmp_e, e) {

		struct uci_delta *h = uci_to_delta(e);

		switch(h->cmd) {
			case UCI_CMD_ADD:
			case UCI_CMD_CHANGE:
				if (!e->name){
					dbprintf(LOG_ERR, "uci copy no option name %s.%s\n", p->e.name, h->section);
					continue;
				}
				if (strcmp(e->name, key) == 0) {
					
					return(void*) e->name;
				}
				break;
		
			default:
				break;
		}

	
	}
	return NULL;
}

int util_uci_config_entity_del(void* uci_pkg,const char *key)
{
	struct uci_element *e,*tmp;
	struct uci_package *p = (struct uci_package*)uci_pkg;
	struct uci_ptr option_ptr;
	
	uci_foreach_element_safe(&p->delta,tmp, e) {
		struct uci_delta *h = uci_to_delta(e);
		
		switch(h->cmd) {
			case UCI_CMD_ADD:
			case UCI_CMD_CHANGE:
				if (!e->name){
					dbprintf(LOG_ERR, "uci copy no option name %s.%s\n", p->e.name, h->section);
					continue;
				}
				if (strcmp(e->name, key) == 0) {
					memset(&option_ptr,0x0,sizeof(struct uci_ptr));
					option_ptr.package = p->e.name;
					option_ptr.section = h->section; 
					option_ptr.option = e->name;  
					option_ptr.value = ""; 
					uci_delete(p->ctx,&option_ptr);
				}
				break;
			case UCI_CMD_LIST_ADD:			
			case UCI_CMD_LIST_DEL:
				if (!e->name){
					dbprintf(LOG_ERR, "uci copy no option name %s.%s\n", p->e.name, h->section);
					continue;
				}
				if (strcmp(e->name, key) == 0) {
					memset(&option_ptr,0x0,sizeof(struct uci_ptr));
					option_ptr.package = p->e.name;
					option_ptr.section = h->section; 
					option_ptr.option = e->name;  
					option_ptr.value = "";
					uci_del_list(p->ctx,&option_ptr);
				}
				break;
			default:
				break;
		}
		
	}

	return 0;
}
int util_uci_config_entity_merge(void* uci_base_pkg,void* uci_update_pkg)
{
	struct uci_element *e,*tmp,*be,*be_tmp;
	struct uci_package *p = (struct uci_package*)uci_update_pkg;
	struct uci_package *bp = (struct uci_package*)uci_base_pkg;
	struct uci_delta *h,*bh;
	struct uci_ptr option_ptr;
	int found =0,count = 0;

	uci_foreach_element_safe(&p->delta,tmp, e) {
		h = uci_to_delta(e);
		found = 0;
		if (!e->name){
			dbprintf(LOG_ERR, "merge null option name %s.%s\n", p->e.name, h->section);
			continue;
		}
		
		uci_foreach_element_safe(&bp->delta,be_tmp,be) {
			bh = uci_to_delta(be);
			switch(h->cmd) {
				case UCI_CMD_ADD:
				case UCI_CMD_CHANGE:
					if (!be->name){
						dbprintf(LOG_ERR, "uci copy no option name %s.%s\n", p->e.name, h->section);
						continue;
					}
					if (strcmp(e->name, be->name) == 0  && strcmp(h->value,bh->value) !=0)  {
						memset(&option_ptr,0x0,sizeof(struct uci_ptr));
						option_ptr.package = p->e.name;
						option_ptr.section = h->section; 
						option_ptr.option = e->name;  
						option_ptr.value = h->value;
						uci_set(bp->ctx,&option_ptr);
					}
					found =1;
					count++;
					break;
				case UCI_CMD_LIST_ADD:			
				case UCI_CMD_LIST_DEL:
					if (!e->name){
						dbprintf(LOG_ERR, "uci copy no option name %s.%s\n", p->e.name, h->section);
						continue;
					}
					if (strcmp(e->name, be->name) == 0 && strcmp(h->value,bh->value) !=0) {
						memset(&option_ptr,0x0,sizeof(struct uci_ptr));
						option_ptr.package = p->e.name;
						option_ptr.section = h->section; 
						option_ptr.option = e->name;  
						option_ptr.value = h->value;
						uci_add_list(bp->ctx,&option_ptr);
					}
					found =1;
					count++;
					break;
				default:
					break;
			}
			if(found == 1)
				break;
		
		}

		if(!found){
			memset(&option_ptr,0x0,sizeof(struct uci_ptr));
			option_ptr.package = p->e.name;
			option_ptr.section = h->section; 
			option_ptr.option = e->name;  
			option_ptr.value = h->value;
			uci_add_list(bp->ctx,&option_ptr);
			count++;
		}
		
	}

	return 0;
}
int util_uci_config_entity_count(void* uci_pkg)
{
	struct uci_element *e,*tmp;
	struct uci_package *p = (struct uci_package*)uci_pkg;
	int count_idx=0;
	
	uci_foreach_element_safe(&p->delta,tmp, e) {
		struct uci_delta *h = uci_to_delta(e);
		
		switch(h->cmd) {
			case UCI_CMD_ADD:
			case UCI_CMD_REMOVE:
			case UCI_CMD_CHANGE:
			case UCI_CMD_LIST_ADD:			
				count_idx++;
				break;
			default:
				break;
		}
		
	}

	return count_idx;

}
int util_uci_config_file_load_by_prefixkey(void* uci_pkg,char* prefix_key)
{
	struct uci_element *e,*tmp,*tmp_sec_e,*sec_e;
	struct uci_package *p = (struct uci_package*)uci_pkg;
	struct uci_ptr option_ptr;
	
	uci_foreach_element_safe(&p->sections,tmp, e) {
		struct uci_section *s = uci_to_section(e);
		struct uci_context* uci_ctx = p->ctx;
		uci_foreach_element_safe(&s->options,tmp_sec_e, sec_e) {
			struct uci_option *opt = uci_to_option(sec_e);			

			if(strncmp(opt->e.name,prefix_key,strlen(prefix_key)) == 0){
				if(opt->type == UCI_TYPE_STRING){				  
					option_ptr.section = s->e.name; 
					option_ptr.option = opt->e.name;  
					option_ptr.value = opt->v.string;
					uci_set(uci_ctx,&option_ptr);			
						
				}else{
					
					struct uci_element *list_e,*list_tmp;
					
					uci_foreach_element_safe(&opt->v.list, list_tmp,list_e)
					{
						option_ptr.section = s->e.name; 
						option_ptr.option = opt->e.name;  
						option_ptr.value = list_e->name;
						uci_add_list(uci_ctx,&option_ptr);
						
					}

				}
			}

		}
		
	}
	return 0;
}
