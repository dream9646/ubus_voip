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
 * File    : xmlomci_spec.c
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

struct xmlomci_spec_context_t {
	unsigned short id;
	unsigned char attr_order;
	unsigned short table_row;
	unsigned char table_column;
	unsigned char field_number;
};

// data stru for spec.xml ////////////////////////////////////////////////////////////////////////////

struct conv_str2val_t map_me_required[] = {
	{"r", ME_REQUIRE_R},
	{"cr", ME_REQUIRE_CR},
	{"o", ME_REQUIRE_O},
	{NULL, 0}
};

struct conv_str2val_t map_me_access[] = {
	{"ont", ME_ACCESS_TYPE_CB_ONT},
	{"olt", ME_ACCESS_TYPE_CB_OLT},
	{"both", ME_ACCESS_TYPE_CB_BOTH},
	{NULL, 0}
};

struct conv_str2val_t map_me_action[] = {
	{"create", 0x00000008},
	{"create complete connection", 0x00000010},
	{"delete", 0x00000020},
	{"delete complete connection", 0x00000040},
	{"set", 0x00000080},
	{"get", 0x00000100},
	{"get complete connection", 0x00000200},
	{"get all alarms", 0x00000400},
	{"get all alarms next", 0x00000800},
	{"mib upload", 0x00001000},
	{"mib upload next", 0x00002000},
	{"mib reset", 0x00004000},
	{"alarm", 0x00008000},
	{"attribute value change", 0x00010000},
	{"test", 0x00020000},
	{"start software download", 0x00040000},
	{"download section", 0x00080000},
	{"end software download", 0x00100000},
	{"active software", 0x00200000},
	{"commit software", 0x00400000},
	{"synchronize time", 0x00800000},
	{"reboot", 0x01000000},
	{"get next", 0x02000000},
	{"test result", 0x04000000},
	{"get current data", 0x08000000},
	{NULL, 0}
};

struct conv_str2val_t map_me_attr_format[] = {
	{"pointer", ATTR_FORMAT_POINTER},
	{"bit field", ATTR_FORMAT_BIT_FIELD},
	{"signed integer", ATTR_FORMAT_SIGNED_INT},
	{"unsigned integer", ATTR_FORMAT_UNSIGNED_INT},
	{"string", ATTR_FORMAT_STRING},
	{"enumeration", ATTR_FORMAT_ENUMERATION},
	{"table", ATTR_FORMAT_TABLE},
	{NULL, 0}
};

struct conv_str2val_t map_me_attr_vformat[] = {
	{"pointer", ATTR_VFORMAT_POINTER},
	{"signed integer", ATTR_VFORMAT_SIGNED_INT},
	{"unsigned integer", ATTR_VFORMAT_UNSIGNED_INT},
	{"string", ATTR_VFORMAT_STRING},
	{"enumeration", ATTR_VFORMAT_ENUMERATION},
	{"mac", ATTR_VFORMAT_MAC},
	{"ipv4", ATTR_VFORMAT_IPV4},
	{"ipv6", ATTR_VFORMAT_IPV6},
	{NULL, 0}
};

struct conv_str2val_t map_me_attr_access_bitmap[] = {
	{"r", ATTRINFO_ACCESEE_BITMAP_READ},	// 0x1
	{"w", ATTRINFO_ACCESEE_BITMAP_WRITE},	// 0x2
	{"set_by_create", ATTRINFO_ACCESEE_BITMAP_SBC},	// 0x4
	{NULL, 0}
};

struct conv_str2val_t map_me_attr_necessity[] = {
	{"optional", ATTR_NECESSITY_OPTIONAL},	// 0
	{"mandatory", ATTR_NECESSITY_MANDATORY},	// 1
	{NULL, 0}
};

struct conv_str2val_t map_me_attr_entry_sort_method[] = {
	{"none", ATTR_ENTRY_SORT_METHOD_NONE},	// 0
	{"ascending", ATTR_ENTRY_SORT_METHOD_ASCENDING},	// 1
	{"dscending", ATTR_ENTRY_SORT_METHOD_DSCENDING},	// 2
	{NULL, 0}
};

// func used for spec.xml  ////////////////////////////////////////////////////////////////////
static int
xmlomci_spec_add_tag(struct xmlparse_env_t *env, char *tag)
{
	struct xmlomci_spec_context_t *x = (struct xmlomci_spec_context_t *) env->dataptr;
	struct meinfo_t *miptr;
	struct attrinfo_t *a;

	dbprintf_xml(LOG_DEBUG, "%d: url=%s, classid=%u, attr=%d\n", env->line, env->url, x->id, x->attr_order);

	if (!strstr(env->url, "/omci/class"))	// only care url containing /omci/class
		return 0;

	// init context
	if (strcmp(env->url, "/omci/class") == 0) {	// reset x
		memset((void *) x, 0, sizeof (struct xmlomci_spec_context_t));
		return 0;
	}
	
	// check miptr, a
	if ((miptr = meinfo_util_miptr(x->id))==NULL) {
		dbprintf_xml(LOG_ERR, "%d: url=%s, classid=%u, miptr null\n", env->line, env->url, x->id);
		return -1;
	}
	a = meinfo_util_aiptr(x->id, x->attr_order);

	// deal tags	
	if (strcmp(env->url, "/omci/class/attr/code_points_list") == 0) {
		INIT_LIST_HEAD(&a->code_points_list);

	} else if (strcmp(env->url, "/omci/class/attr/attrinfo_bitfield") == 0) {
		a->attrinfo_bitfield_ptr = (struct attrinfo_bitfield_t *) malloc_safe(sizeof (struct attrinfo_bitfield_t));

	} else if (strcmp(env->url, "/omci/class/attr/attrinfo_table") == 0) {
		a->attrinfo_table_ptr = (struct attrinfo_table_t *) malloc_safe(sizeof (struct attrinfo_table_t));
	}

	return 0;
}

static int
xmlomci_spec_add_attr(struct xmlparse_env_t *env, char *tag, char *attr, char *attrval)
{
	struct xmlomci_spec_context_t *x = (struct xmlomci_spec_context_t *) env->dataptr;
	struct meinfo_t *miptr;
	struct attrinfo_t *a;

	dbprintf_xml(LOG_DEBUG, "%d: url=%s, classid=%u, attr=%d\n", env->line, env->url, x->id, x->attr_order);

	if (!strstr(env->url, "/omci/class"))	// only care url containing /omci/class
		return 0;

	// init classid, attr_order
	if (strcmp(env->url, "/omci/class") == 0) {
		if (strcmp(attr, "id") == 0) {
			x->id = util_atoi(attrval);

			// alloc and init an meinfo_t entry
			miptr=meinfo_alloc_meinfo();
			if (miptr==NULL) {
				dbprintf_xml(LOG_ERR,
					 "%d: url=%s, classid=%u, alloc mem err\n",
					 env->line, env->url, x->id);
				return -1;
			}
			miptr->classid = x->id;
			
			// assign allocated meinfo_t to meinfo_array
			meinfo_util_miptr(x->id)=miptr;
			return 0;
		}
	} else if (strcmp(env->url, "/omci/class/attr") == 0) {
		if (strcmp(attr, "i") == 0) {
			x->attr_order = util_atoi(attrval);
			return 0;
		}
	}

	// check miptr, a
	if ((miptr = meinfo_util_miptr(x->id))==NULL) {
		dbprintf_xml(LOG_ERR, "%d: url=%s, classid=%u, miptr null\n", env->line, env->url, x->id);
		return -1;
	}
	a = meinfo_util_aiptr(x->id, x->attr_order);

	// deal attrs
	if (strcmp(env->url, "/omci/class") == 0) {
		if (strcmp(attr, "attr_total") == 0) {
			miptr->attr_total = util_atoi(attrval);
		} else if (strcmp(attr, "name") == 0) {
			if ((omci_env_g.xml_load_namestr || meinfo_util_is_proprietary(x->id)) && 
				xmlomci_loadstr(&miptr->name, xmlomci_abbrestr(attrval))<0) {
				dbprintf_xml(LOG_ERR, 
					 "%d: url=%s, classid=%u, loadstr err\n",
					 env->line, env->url, x->id);
				return -1;
			}
		} else if (strcmp(attr, "section") == 0) {
			if ((omci_env_g.xml_load_namestr || meinfo_util_is_proprietary(x->id)) && 
				xmlomci_loadstr(&miptr->section, attrval)<0) {
				dbprintf_xml(LOG_ERR,
					 "%d: url=%s, classid=%u, loadstr err\n",
					 env->line, env->url, x->id);
				return -1;
			}
		}
	} else if (strcmp(env->url, "/omci/class/attr") == 0) {
		if (strcmp(attr, "name") == 0) {			
			if ((omci_env_g.xml_load_namestr || meinfo_util_is_proprietary(x->id)) && 
				xmlomci_loadstr(&a->name, attrval)<0) {
				dbprintf_xml(LOG_ERR,
					 "%d: url=%s, classid=%u, attr=%d, loadstr err\n",
					 env->line, env->url, x->id, x->attr_order);
				return -1;
			}
		}
	} else if (strcmp(env->url, "/omci/class/attr/format") == 0) {
		if (strcmp(attr, "vformat") == 0) {
			a->vformat = conv_str2value(attrval, map_me_attr_vformat);	// 0 means not initialized
		}
		// for attrinfo_bitfield related
	} else if (strstr(env->url, "/omci/class/attr/attrinfo_bitfield")) {
		struct attrinfo_bitfield_t *b = a->attrinfo_bitfield_ptr;
		if (b == NULL) {
			dbprintf_xml(LOG_ERR,
				 "%d: url=%s, classid=%u, attr=%d, null attrinfo_bitfield ptr\n",
				 env->line, env->url, x->id, x->attr_order);
			return -1;
		}
		if (strcmp(env->url, "/omci/class/attr/attrinfo_bitfield") == 0) {
			if (strcmp(attr, "field_total") == 0) {
				b->field_total = util_atoi(attrval);
			}
		} else if (strcmp(env->url, "/omci/class/attr/attrinfo_bitfield/field_bit_width") == 0) {
			if (strcmp(attr, "i") == 0) {
				x->field_number = util_atoi(attrval);
			} else if (strcmp(attr, "vformat") == 0) {
				b->field[x->field_number].vformat = conv_str2value(attrval, map_me_attr_vformat);
			} else if (strcmp(attr, "name") == 0 ) {
				if (omci_env_g.xml_load_namestr && xmlomci_loadstr(&b->field[x->field_number].name, attrval)<0) {
					dbprintf_xml(LOG_ERR,
						 "%d: url=%s, classid=%u, attr=%d, field_number=%d, loadstr err\n",
						 env->line, env->url, x->id, x->attr_order, x->field_number);
					return -1;
				}
			}
		}
		// for attrinfo_table related
	} else if (strstr(env->url, "/omci/class/attr/attrinfo_table")) {
		struct attrinfo_table_t *t = a->attrinfo_table_ptr;
		if (t == NULL) {
			dbprintf_xml(LOG_ERR,
				 "%d: url=%s, classid=%u, attr=%d, null attrinfo_table ptr\n",
				 env->line, env->url, x->id, x->attr_order);
			return -1;
		}
		if (strcmp(env->url, "/omci/class/attr/attrinfo_table") == 0) {
			if (strcmp(attr, "field_total") == 0) {
				t->field_total = util_atoi(attrval);
			}
		} else if (strcmp(env->url, "/omci/class/attr/attrinfo_table/field_bit_width") == 0) {
			if (strcmp(attr, "i") == 0) {
				x->table_column = util_atoi(attrval);
			} else if (strcmp(attr, "vformat") == 0) {
				t->field[x->table_column].vformat = conv_str2value(attrval, map_me_attr_vformat);
			} else if (strcmp(attr, "pk") == 0) {
				t->field[x->table_column].is_primary_key = util_atoi(attrval);
			} else if (strcmp(attr, "name") == 0 ) {
				if (omci_env_g.xml_load_namestr && xmlomci_loadstr(&t->field[x->table_column].name, attrval)<0) {
					dbprintf_xml(LOG_ERR,
						 "%d: url=%s, classid=%u, attr=%d, table column=%d, loadstr err\n",
						 env->line, env->url, x->id, x->attr_order, x->table_column);
					return -1;
				}
			}
		}
	}

	return 0;
}

static int
xmlomci_spec_finish_attrs(struct xmlparse_env_t *env, char *tag)
{
	struct xmlomci_spec_context_t *x = (struct xmlomci_spec_context_t *) env->dataptr;

	dbprintf_xml(LOG_DEBUG, "%d: url=%s, classid=%u, attr=%d\n", env->line, env->url, x->id, x->attr_order);

	return 0;
}

static int
xmlomci_spec_add_content(struct xmlparse_env_t *env, char *tag, char *s)
{
	struct xmlomci_spec_context_t *x = (struct xmlomci_spec_context_t *) env->dataptr;
	struct meinfo_t *miptr;
	struct attrinfo_t *a;

	dbprintf_xml(LOG_DEBUG, "%d: url=%s, classid=%u, attr=%d\n", env->line, env->url, x->id, x->attr_order);

	if (!strstr(env->url, "/omci/class"))	// only care url containing /omci/class
		return 0;

	// check miptr, a
	if ((miptr = meinfo_util_miptr(x->id))==NULL) {
		dbprintf_xml(LOG_ERR, "%d: url=%s, classid=%u, miptr null\n", env->line, env->url, x->id);
		return -1;
	}
	a = meinfo_util_aiptr(x->id, x->attr_order);

	// deal content for me
	if (strcmp(env->url, "/omci/class/required") == 0) {
		miptr->required = conv_str2value(s, map_me_required);

	} else if (strcmp(env->url, "/omci/class/access") == 0) {
		miptr->access = conv_str2value(s, map_me_access);

	} else if (strcmp(env->url, "/omci/class/action") == 0) {
		miptr->action_mask = conv_strlist2mask(s, map_me_action);

	} else if (strcmp(env->url, "/omci/class/alarm_list") == 0) {
		unsigned char mask[28];
		memset(mask, 0x00, 28);
		if (conv_numlist2alarmmask(s, mask, 28) == 0)
			memcpy(miptr->alarm_mask, mask, 28);

	} else if (strcmp(env->url, "/omci/class/avc_list") == 0) {
		unsigned char mask[2];
		memset(mask, 0x00, 2);
		if (conv_numlist2attrmask(s, mask, 2) == 0)
			memcpy(miptr->avc_mask, mask, 2);

	} else if (strcmp(env->url, "/omci/class/is_private") == 0) {
		miptr->is_private = util_atoi(s);

	} else if (strcmp(env->url, "/omci/class/related_ready_class_list") == 0) {
		unsigned short array[32];
		unsigned int n, i;
		struct meinfo_related_ready_class_t *node;

		n = conv_numlist2array(s, array, sizeof(unsigned short), 32);

		for (i = 0; i < n; i++)
		{
			//allocate meinfo_related_ready_class_t
			node = malloc_safe(sizeof(struct meinfo_related_ready_class_t));
			node->classid = array[i];

			list_add_tail(&node->related_ready_class_node, &miptr->related_ready_class_list);
		}

	// deal content for me attr
	} else if (strcmp(env->url, "/omci/class/attr/format") == 0) {
		a->format = conv_str2value(s, map_me_attr_format);

	} else if (strcmp(env->url, "/omci/class/attr/default_value") == 0) {
		int ret;
		a->spec_default_value.data=0;
		a->spec_default_value.ptr=NULL;
		ret=meinfo_conv_string_to_attr(x->id, x->attr_order, &a->spec_default_value, s);
		if (ret<0) {
			dbprintf_xml(LOG_ERR,
				 "%d: url=%s, classid=%u, attr=%d, err=%d\n",
				 env->line, env->url, x->id, x->attr_order, ret);
			return -1;
		}

	} else if (strcmp(env->url, "/omci/class/attr/access_bitmap") == 0) {
		a->access_bitmap = conv_strlist2mask(s, map_me_attr_access_bitmap);

	} else if (strcmp(env->url, "/omci/class/attr/necessity") == 0) {
		a->necessity = conv_str2value(s, map_me_attr_necessity);

	} else if (strcmp(env->url, "/omci/class/attr/byte_size") == 0) {
		a->byte_size = util_atoi(s);

	} else if (strcmp(env->url, "/omci/class/attr/lower_limit") == 0) {
		int i = util_atoll(s);

		if (i != 0 && strlen(s) > 0) {	// limit defined and not null
			a->lower_limit = i;
			a->has_lower_limit = 1;
		}
	} else if (strcmp(env->url, "/omci/class/attr/is_related_key") == 0) {
		a->is_related_key = util_atoi(s);

	} else if (strcmp(env->url, "/omci/class/attr/upper_limit") == 0) {
		int i = util_atoll(s);

		if (i != 0 && strlen(s) > 0) {	// limit defined and not null
			a->upper_limit = i;
			a->has_upper_limit = 1;
		}
	} else if (strcmp(env->url, "/omci/class/attr/bit_field_mask") == 0) {
		a->bit_field_mask = util_atoll(s);	// may be not used

	} else if (strcmp(env->url, "/omci/class/attr/code_points_list") == 0) {
		xmlomci_strlist2codepoints(s, &a->code_points_list);

	} else if (strcmp(env->url, "/omci/class/attr/pointer_class_id") == 0) {
		a->pointer_classid = util_atoi(s);

	} else if (strcmp(env->url, "/omci/class/attr/pm_tca_number") == 0) {
		a->pm_tca_number = util_atoi(s);

	} else if (strcmp(env->url, "/omci/class/attr/pm_alarm_number") == 0) {
		a->pm_alarm_number = util_atoi(s);

		// for attrinfo_bitfield related
	} else if (strstr(env->url, "/omci/class/attr/attrinfo_bitfield")) {
		struct attrinfo_bitfield_t *b = a->attrinfo_bitfield_ptr;
		if (b == NULL) {
			dbprintf_xml(LOG_ERR,
				 "%d: url=%s, classid=%u, attr=%d, null attrinfo_bitfield ptr\n",
				 env->line, env->url, x->id, x->attr_order);
			return -1;

		} else if (strcmp(env->url, "/omci/class/attr/attrinfo_bitfield/field_bit_width") == 0) {
			b->field[x->field_number].bit_width = util_atoi(s);
		}
		// for attrinfo_table related
	} else if (strstr(env->url, "/omci/class/attr/attrinfo_table")) {
		struct attrinfo_table_t *t = a->attrinfo_table_ptr;
		if (t == NULL) {
			dbprintf_xml(LOG_ERR,
				 "%d: url=%s, classid=%u, attr=%d, null attrinfo_table ptr\n",
				 env->line, env->url, x->id, x->attr_order);
			return -1;
		}
		if (strcmp(env->url, "/omci/class/attr/attrinfo_table/entry_byte_size") == 0) {
			t->entry_byte_size = util_atoi(s);
		} else if (strcmp(env->url, "/omci/class/attr/attrinfo_table/entry_sort_method") == 0) {
			t->entry_sort_method = conv_str2value(s, map_me_attr_entry_sort_method);
		} else if (strcmp(env->url, "/omci/class/attr/attrinfo_table/field_bit_width") == 0) {
			t->field[x->table_column].bit_width = util_atoi(s);
		}
	}

	return 0;
}

static int
xmlomci_spec_finish_tag(struct xmlparse_env_t *env, char *tag)
{
	struct xmlomci_spec_context_t *x = (struct xmlomci_spec_context_t *) env->dataptr;

	dbprintf_xml(LOG_DEBUG, "%d: url=%s, classid=%u, attr=%d\n", env->line, env->url, x->id, x->attr_order);

	if (strcmp(env->url, "/omci/class") == 0) {	// reset x
		memset((void *) x, 0, sizeof (struct xmlomci_spec_context_t));
	}

	return 0;
}

int
xmlomci_spec_init(char *spec_xml_filename)
{
	struct xmlomci_spec_context_t x;
	struct xmlparse_env_t env;

	env.dataptr = (void *) &x;
	env.add_tag = xmlomci_spec_add_tag;
	env.add_attr = xmlomci_spec_add_attr;
	env.finish_attrs = xmlomci_spec_finish_attrs;
	env.add_content = xmlomci_spec_add_content;
	env.finish_tag = xmlomci_spec_finish_tag;

	memset((void *) &x, 0, sizeof (struct xmlomci_spec_context_t));
	
	return xmlparse_parse(spec_xml_filename, &env);
}
