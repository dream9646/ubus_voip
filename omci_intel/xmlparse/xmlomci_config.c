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
 * File    : xmlomci_config.c
 *
 ******************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <syslog.h>

#include <arpa/inet.h>

#include "util.h"
#include "conv.h"
#include "env.h"
#include "xmlparse.h"
#include "xmlomci.h"
#include "meinfo.h"
#include "list.h"

struct xmlomci_config_context_t {
	unsigned short id;
	unsigned char attr_order;
	unsigned short table_row;
	unsigned char table_column;
	unsigned char field_number;
	unsigned short instance_num;
	struct meinfo_instance_def_t *instance_def_ptr;
	struct attr_table_header_t *table_header;
	struct attr_table_entry_t *table_entry;
};

// func used for config.xml ////////////////////////////////////////////////////////////////////

static int
xmlomci_config_add_tag(struct xmlparse_env_t *env, char *tag)
{
	struct xmlomci_config_context_t *x = (struct xmlomci_config_context_t *) env->dataptr;
	struct meinfo_t *miptr;
	struct attrinfo_t *a;
	struct attrinfo_table_t *tab;

	dbprintf_xml(LOG_DEBUG, "%d: url=%s, classid=%u, instance=%d, attr=%d\n",
		 env->line, env->url, x->id, x->instance_num, x->attr_order);

	if (!strstr(env->url, "/omci/class"))	// only care url containing /omci/class
		return 0;

	// init context
	if (strcmp(env->url, "/omci/class") == 0) {	// reset x
		memset((void *) x, 0, sizeof (struct xmlomci_config_context_t));
		return 0;
	}

	// check miptr, a, tab
	if ((miptr = meinfo_util_miptr(x->id))==NULL) {
		dbprintf_xml(LOG_ERR, "%d: url=%s, classid=%u, miptr null\n", env->line, env->url, x->id);
		return -1;
	}
	a = meinfo_util_aiptr(x->id, x->attr_order);
	tab = a->attrinfo_table_ptr;

	// del tags
	if (strcmp(env->url, "/omci/class/instance") == 0) {
		struct meinfo_instance_def_t *inst = malloc_safe(sizeof (struct meinfo_instance_def_t));
		list_add_tail(&inst->instance_node, &miptr->config.meinfo_instance_def_list);
		x->instance_def_ptr = inst;

	} else if (strcmp(env->url, "/omci/class/instance/attr/attr_table") == 0) {
		struct attr_value_t *v;
		struct attr_table_header_t *h;

		if (x->instance_def_ptr == NULL) {
			dbprintf_xml(LOG_ERR,
				 "%d: url=%s, classid=%u, instance=%d, attr=%d, instance ptr null\n",
				 env->line, env->url, x->id, x->instance_num, x->attr_order);
			return -1;
		}
		// alloc attr_table_header
		h = (struct attr_table_header_t *)malloc_safe(sizeof (struct attr_table_header_t));
		INIT_LIST_HEAD(&h->entry_list);

		// assign allocated table header to default value ptr
		v = &x->instance_def_ptr->custom_default_value[x->attr_order];
		v->ptr = h;
		x->table_header = h;

	} else if (strcmp(env->url, "/omci/class/instance/attr/attr_table/entry") == 0) {
		struct attr_table_entry_t *e;
		void *edata;

		if (x->instance_def_ptr == NULL) {
			dbprintf_xml(LOG_ERR,
				 "%d: url=%s, classid=%u, instance=%d, attr=%d, instance ptr null\n",
				 env->line, env->url, x->id, x->instance_num, x->attr_order);
			return -1;
		}
		if (tab == NULL) {
			dbprintf_xml(LOG_ERR,
				 "%d: url=%s, classid=%u, instance=%d, attr=%d, not a table?\n",
				 env->line, env->url, x->id, x->instance_num, x->attr_order);
			return -1;
		}
		if (x->table_header == NULL) {
			dbprintf_xml(LOG_ERR,
				 "%d: url=%s, classid=%u, instance=%d, attr=%d, table_header null\n",
				 env->line, env->url, x->id, x->instance_num, x->attr_order);
			return -1;
		}
		// alloc attr_table_entry
		e = (struct attr_table_entry_t *)malloc_safe(sizeof (struct attr_table_entry_t));
		edata = malloc_safe(tab->entry_byte_size);

		// init table_entry
		e->entry_data_ptr = edata;

		// put entry on table header list
		list_add_tail(&e->entry_node, &x->table_header->entry_list);

		x->table_entry = e;
		x->table_header->entry_count++;
	}
	return 0;
}

static int
xmlomci_config_add_attr(struct xmlparse_env_t *env, char *tag, char *attr, char *attrval)
{
	struct xmlomci_config_context_t *x = (struct xmlomci_config_context_t *) env->dataptr;
	struct meinfo_t *miptr = meinfo_util_miptr(x->id);

	dbprintf_xml(LOG_DEBUG, "%d: url=%s, classid=%u, instance=%d, attr=%d\n",
		 env->line, env->url, x->id, x->instance_num, x->attr_order);

	if (!strstr(env->url, "/omci/class"))	// only care url containing /omci/class
		return 0;

	// init classid, attr_order
	if (strcmp(env->url, "/omci/class") == 0) {
		if (strcmp(attr, "id") == 0) {
			x->id = util_atoi(attrval);
			miptr=meinfo_util_miptr(x->id);	// relocate meinfo_t entry
			if (miptr==NULL) {
				dbprintf_xml(LOG_ERR,
					 "%d: url=%s, classid=%u, null miptr error?\n",
					 env->line, env->url, x->id);
				return -1;
			}
			return 0;
		}
	} else if (strcmp(env->url, "/omci/class/instance/attr") == 0) {
		if (strcmp(attr, "i") == 0) {
			x->attr_order = util_atoi(attrval);
			return 0;
		}
	}

	// check miptr, a
	if (miptr==NULL) {
		dbprintf_xml(LOG_ERR, "%d: url=%s, classid=%u, miptr null\n", env->line, env->url, x->id);
		return -1;
	}              

	// for instance_def_ptr related
	if (strstr(env->url, "/omci/class/instance")) {
		if (x->instance_def_ptr == NULL) {
			dbprintf_xml(LOG_ERR, "%d: null instance_def_ptr for %s, classid=%u\n", env->line, env->url, x->id);
			return -1;
		}

		if (strcmp(env->url, "/omci/class/instance") == 0) {
			if (strcmp(attr, "i") == 0) {
				x->instance_num = util_atoi(attrval);
				x->instance_def_ptr->instance_num = x->instance_num;
			}

		} else if (strcmp(env->url, "/omci/class/instance/attr/attr_table/entry")
			   == 0) {
			if (strcmp(attr, "i") == 0) {
				x->table_row = util_atoi(attrval);
				// x->table_entry->entryid=x->table_row;
			}
		}
	}

	return 0;
}

static int
xmlomci_config_finish_attrs(struct xmlparse_env_t *env, char *tag)
{
	struct xmlomci_config_context_t *x = (struct xmlomci_config_context_t *) env->dataptr;

	dbprintf_xml(LOG_DEBUG, "%d: url=%s, classid=%u, instance=%d, attr=%d\n",
		 env->line, env->url, x->id, x->instance_num, x->attr_order);

	return 0;
}

static int
xmlomci_config_add_content(struct xmlparse_env_t *env, char *tag, char *s)
{
	struct xmlomci_config_context_t *x = (struct xmlomci_config_context_t *) env->dataptr;
	struct meinfo_t *miptr = meinfo_util_miptr(x->id);
	struct attrinfo_t *a;
	struct attrinfo_table_t *tab;

	dbprintf_xml(LOG_DEBUG, "%d: url=%s, classid=%u, instance=%d, attr=%d\n",
		 env->line, env->url, x->id, x->instance_num, x->attr_order);

	if (!strstr(env->url, "/omci/class"))	// only care url containing /omci/class
		return 0;

	// check miptr, a, b, tab	 
	if (miptr==NULL) {
		dbprintf_xml(LOG_ERR, "%d: url=%s, classid=%u, miptr null\n", env->line, env->url, x->id);
		return -1;
	}
	a = meinfo_util_aiptr(x->id, x->attr_order);
	tab = a->attrinfo_table_ptr;

	// content for me
	if (strcmp(env->url, "/omci/class/support") == 0) {
		miptr->config.support = util_atoi(s);

	} else if (strcmp(env->url, "/omci/class/instance_total_max") == 0) {
		miptr->config.instance_total_max = util_atoi(s);

		// content for me instance_def_ptr related
	} else if (strstr(env->url, "/omci/class/instance")) {
		if (x->instance_def_ptr == NULL) {
			dbprintf_xml(LOG_ERR, "%d: url=%s, classid=%u, instance_def_ptr null\n", env->line, env->url, x->id);
			return -1;
		}

		if (strcmp(env->url, "/omci/class/instance/devname") == 0) {
			strncpy(x->instance_def_ptr->devname, s, DEVICE_NAME_MAX_LEN);

		} else if (strcmp(env->url, "/omci/class/instance/default_meid") == 0) {
			x->instance_def_ptr->default_meid=util_atoi(s);

		} else if (strcmp(env->url, "/omci/class/instance/pm_is_accumulated_mask") == 0) {
			unsigned short mask=util_atoi(s);
			x->instance_def_ptr->pm_is_accumulated_mask[0] = mask >> 8;
			x->instance_def_ptr->pm_is_accumulated_mask[1] = mask & 0xff;

		} else if (strcmp(env->url, "/omci/class/instance/is_private") == 0) {
			x->instance_def_ptr->is_private=util_atoi(s)?1:0;

		} else if (strcmp(env->url, "/omci/class/instance/attr") == 0) {
			if (a->format!=ATTR_FORMAT_TABLE) {
				struct attr_value_t *v = &x->instance_def_ptr->custom_default_value[x->attr_order];
				int ret = meinfo_conv_string_to_attr(x->id, x->attr_order, v, s);
				if (ret<0) {
					dbprintf_xml(LOG_ERR,
						 "%d: url=%s, classid=%u, instance=%d, attr=%d, err=%d\n",
						 env->line, env->url, x->id, x->instance_num, x->attr_order, ret);
					return -1;
				}
			}
		} else if (strcmp(env->url, "/omci/class/instance/attr/attr_table/entry") == 0) {
			struct attr_value_t v;
			int ret;
			
			if (tab == NULL) {
				dbprintf_xml(LOG_ERR,
					 "%d: url=%s, classid=%u, instance=%d, attr=%d, not a table?\n",
					 env->line, env->url, x->id, x->instance_num, x->attr_order);
				return -1;
			}
			if (x->table_entry == NULL) {
				dbprintf_xml(LOG_ERR,
					 "%d: url=%s, classid=%u, instance=%d, attr=%d, table_entry null\n",
					 env->line, env->url, x->id, x->instance_num, x->attr_order);
				return -1;
			}

			v.data=0;
			v.ptr=x->table_entry->entry_data_ptr;	// encapsule table entry data in attr_value_t
			ret = meinfo_conv_string_to_attr(x->id, x->attr_order, &v, s);
			if (ret<0) {
				dbprintf_xml(LOG_ERR,
					 "%d: url=%s, classid=%u, instance=%d, attr=%d, err=%d\n",
					 env->line, env->url, x->id, x->instance_num, x->attr_order, ret);
				return -1;
			}
		}
	}

	return 0;
}

static int
xmlomci_config_finish_tag(struct xmlparse_env_t *env, char *tag)
{
	struct xmlomci_config_context_t *x = (struct xmlomci_config_context_t *) env->dataptr;
	struct meinfo_t *miptr = meinfo_util_miptr(x->id);

	dbprintf_xml(LOG_DEBUG, "%d: url=%s, classid=%u, instance=%d, attr=%d\n",
		 env->line, env->url, x->id, x->instance_num, x->attr_order);

	if (!strstr(env->url, "/omci/class"))	// only care url containing /omci/class
		return 0;

	if (miptr==NULL) {
		dbprintf_xml(LOG_ERR, "%d: url=%s, classid=%u, miptr null\n", env->line, env->url, x->id);
		return -1;
	}

	if (strcmp(env->url, "/omci/class") == 0) {	// reset x
		memset((void *) x, 0, sizeof (struct xmlomci_config_context_t));
		miptr->config.is_inited = 1;		// mark this meinfo_config as inited
	} else if (strcmp(env->url, "/omci/class/instance") == 0) {
		x->instance_def_ptr = NULL;
	} else if (strcmp(env->url, "/omci/class/instance/attr/attr_table") == 0) {
		x->table_header = NULL;
	} else if (strcmp(env->url, "/omci/class/instance/attr/attr_table/entry") == 0) {
		x->table_entry = NULL;
	}

	return 0;
}

int
xmlomci_config_init(char *config_xml_filename)
{
	struct xmlomci_config_context_t x;
	struct xmlparse_env_t env;

	env.dataptr = (void *) &x;
	env.add_tag = xmlomci_config_add_tag;
	env.add_attr = xmlomci_config_add_attr;
	env.finish_attrs = xmlomci_config_finish_attrs;
	env.add_content = xmlomci_config_add_content;
	env.finish_tag = xmlomci_config_finish_tag;

	memset((void *) &x, 0, sizeof (struct xmlomci_config_context_t));
	return xmlparse_parse(config_xml_filename, &env);
}
