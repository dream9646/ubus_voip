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
 * Module  : util
 * File    : regexp.h
 *
 ******************************************************************/

#ifndef __REGEX_H__
#define __REGEX_H__

#if __BSD_VISIBLE
#include <gnu/regex.h>
#else
#include <regex.h>
#endif

#if 1
struct regexp_context_t {
	regex_t pattern_buffer;
	regmatch_t regs[1];
	char *dataptr;
	int is_compiled;
	int in_use;
};

#else
struct regexp_context_t {
	struct re_pattern_buffer pattern_buffer;
	struct re_registers regs;
	char *dataptr;
	int is_compiled;
	int in_use;
};
#endif

#define REGEXP_IS_COMPILED_COOKIE	0x27888118

/* regexp.c */
int regexp_prepare(struct regexp_context_t *context, char *regex);
int regexp_release(struct regexp_context_t *context);
int regexp_is_compiled(struct regexp_context_t *context);
int regexp_match(struct regexp_context_t *context, char *str);
int regexp_nextmatch(struct regexp_context_t *context);
int regexp_match_cpy(struct regexp_context_t *context, int n, char *buff, int buffsize);
char *regexp_match_str(struct regexp_context_t *context, int n);
int regexp_match_replace(struct regexp_context_t *context, char *replace, char *str, char *buff, int buff_len);

#endif
