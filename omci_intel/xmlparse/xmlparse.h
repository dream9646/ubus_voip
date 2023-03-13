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
 * File    : xmlparse.h
 *
 ******************************************************************/

#ifndef __XMLPARSE_H__
#define __XMLPARSE_H__

#define MAXSIZE_URL		1024
#define MAXSIZE_TAGNAME		64
#define MAXSIZE_ATTRNAME	64
#define MAXSIZE_VALUE		512

struct xmlparse_env_t {
	char url[MAXSIZE_URL];
	int depth;
	int line;		// line num of the line in parsed xml file
	void *dataptr;		// data used by user callback

	// the following are callbacks used by xml parser
	int (*add_tag) (struct xmlparse_env_t * env, char *tag);
	int (*add_attr) (struct xmlparse_env_t * env, char *tag, char *attr, char *attrval);
	int (*finish_attrs) (struct xmlparse_env_t * env, char *tag);
	int (*add_content) (struct xmlparse_env_t * env, char *tag, char *s);
	int (*finish_tag) (struct xmlparse_env_t * env, char *tag);
};

/* xmlparse.c */
int xmlparse_parse(char *xml_filename, struct xmlparse_env_t *env);

#endif
