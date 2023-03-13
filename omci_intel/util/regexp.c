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
 * File    : regexp.c
 *
 ******************************************************************/

#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <sys/types.h>

#include "regexp.h"

#define ARRAY_SIZE(arr) (sizeof((arr)) / sizeof((arr)[0]))


#if 1
int
regexp_prepare(struct regexp_context_t *context, char *regex)
{
	int reg_option = 0;
	memset(context, 0, sizeof (struct regexp_context_t));

	/* compile regular expression */
	reg_option = REG_EXTENDED | REG_NEWLINE;
	if (regcomp(&context->pattern_buffer, regex, reg_option))
		return -2;
	context->is_compiled=REGEXP_IS_COMPILED_COOKIE;
	return 0;
}	
#else
int
regexp_prepare(struct regexp_context_t *context, char *regex)
{
	memset(context, 0, sizeof (struct regexp_context_t));

	context->pattern_buffer.allocated = 0;
	context->pattern_buffer.buffer = 0;
	context->pattern_buffer.fastmap = 0;
	context->pattern_buffer.translate = 0;

	/* compile regular expression */
	re_syntax_options = RE_SYNTAX_EGREP | RE_DOT_NEWLINE | RE_INTERVALS | RE_BACKSLASH_ESCAPE_IN_LISTS;
	if (re_compile_pattern(regex, strlen(regex), &context->pattern_buffer)!=NULL)
		return -2;
	context->is_compiled=REGEXP_IS_COMPILED_COOKIE;
	return 0;
}	
#endif

#if 1
int
regexp_release(struct regexp_context_t *context)
{		
	/* free internal data related to previous compiled regex */
	if (context->is_compiled==REGEXP_IS_COMPILED_COOKIE) {
		regfree(&context->pattern_buffer);
		context->is_compiled = 0;
	}
	return 0;
}
#else
int
regexp_release(struct regexp_context_t *context)
{		
	/* free resulf buff of previous match */
	if (context->regs.start) free(context->regs.start);
	if (context->regs.end) free(context->regs.end);

	/* free internal data related to previous compiled regex */
	if (context->is_compiled==REGEXP_IS_COMPILED_COOKIE) {
		regfree(&context->pattern_buffer);
		context->is_compiled = 0;
	}
	return 0;
}
#endif

int
regexp_is_compiled(struct regexp_context_t *context)
{
	return context->is_compiled==REGEXP_IS_COMPILED_COOKIE;
}

/* 
int 
re_search (struct re_pattern_buffer *pattern_buffer, const char *string, const int size, 
	const int start, const int range, struct re_registers *regs)

   there is no support for \d \s \w in the C interface, we have to use named class  
   [:alnum:], [:alpha:], [:cntrl:], [:digit:], [:graph:],
   [:lower:], [:print:], [:punct:], [:space:], [:upper:], and [:xdigit:]

   re_search ret(): -1:match not found, -2:internal error, >=0:offset of match
   regs->start[0], regs->end[0] : start and end of whole pattern
   regs->start[1..9], regs->end[1..9] : start and end of submatch in the pattern 
*/

// ret 0: not found, 1: found, <0: internal err
#if 1
int
regexp_match(struct regexp_context_t *context, char *str)
{
	if (context->is_compiled!=REGEXP_IS_COMPILED_COOKIE)
		return -2;

	if (regexec(&context->pattern_buffer, str, ARRAY_SIZE(context->regs), context->regs, 0))
        	return 0;
	
	return 1;		// internal error
}

// ret 0: not found, 1: found, <0: internal err
int
regexp_nextmatch(struct regexp_context_t *context)
{
	if (context->is_compiled!=REGEXP_IS_COMPILED_COOKIE)
		return -2;

	context->dataptr += context->regs[0].rm_eo;

	// tricky!, regs should be 0 for re_search to work properly
	memset(&context->regs[0], 0, sizeof(regmatch_t));	

	if (regexec(&context->pattern_buffer, context->dataptr, ARRAY_SIZE(context->regs), context->regs, 0))
        	return 0;

	return 1;		// internal error
}

// ret 0: not found, 1: found, <0: buff too small
// n=0, cp whole match; n=1..9, cp submatch 
int
regexp_match_cpy(struct regexp_context_t *context, int n, char *buff, int buffsize)
{
	char *src;
	int match_len;

	if (context->regs[0].rm_so < 0)
		return 0;	// not found

	src = context->dataptr + context->regs[0].rm_so;
	match_len = context->regs[0].rm_eo - context->regs[0].rm_so;

	if (match_len >= buffsize) {
		strncpy(buff, src, buffsize - 1);
		buff[buffsize - 1] = 0x0;
		return -1;	// buffsize tool small
	}

	strncpy(buff, src, match_len);
	buff[match_len] = 0x0;
	return 1;		// match found
}

// wrappers for above wrapper function //////////////////////////////////////////////////////////////////////////////////////////               

// ret: null: match not found, else: ptr to match str
// n=0, cp whole match; n=1..9, cp submatch
char *
regexp_match_str(struct regexp_context_t *context, int n)
{
	static char buff[1024];
	if (regexp_match_cpy(context, n, buff, 1024) == 0)
		return NULL;
	return (buff);
}


#else
int
regexp_match(struct regexp_context_t *context, char *str)
{
	int ret, len;
	if (context->is_compiled!=REGEXP_IS_COMPILED_COOKIE)
		return -2;

	if (context->regs.start) free(context->regs.start);
	if (context->regs.end) free(context->regs.end);
	// tricky!, regs should be 0 for re_search to work properly
	memset(&context->regs, 0, sizeof (struct re_registers));	

	context->dataptr = str;
	len = strlen(context->dataptr);
	ret = re_search(&context->pattern_buffer, context->dataptr , len, 0, len, &context->regs);
	if (ret>=0)
		return 1;	// found
	if (ret==-1)
		return 0;	// not found
	return ret;		// internal error
}

// ret 0: not found, 1: found, <0: internal err
int
regexp_nextmatch(struct regexp_context_t *context)
{
	int ret, len;
	if (context->is_compiled!=REGEXP_IS_COMPILED_COOKIE)
		return -2;

	context->dataptr += context->regs.end[0];

	if (context->regs.start) free(context->regs.start);
	if (context->regs.end) free(context->regs.end);
	// tricky!, regs should be 0 for re_search to work properly
	memset(&context->regs, 0, sizeof (struct re_registers));	

	len = strlen(context->dataptr);
	ret = re_search(&context->pattern_buffer, context->dataptr , len, 0, len, &context->regs);
	if (ret>=0)
		return 1;	// found
	if (ret==-1)
		return 0;	// not found
	return ret;		// internal error
}

// ret 0: not found, 1: found, <0: buff too small
// n=0, cp whole match; n=1..9, cp submatch 
int
regexp_match_cpy(struct regexp_context_t *context, int n, char *buff, int buffsize)
{
	char *src;
	int match_len;

	if (context->regs.start[n] < 0)
		return 0;	// not found

	src = context->dataptr + context->regs.start[n];
	match_len = context->regs.end[n] - context->regs.start[n];

	if (match_len >= buffsize) {
		strncpy(buff, src, buffsize - 1);
		buff[buffsize - 1] = 0x0;
		return -1;	// buffsize tool small
	}

	strncpy(buff, src, match_len);
	buff[match_len] = 0x0;
	return 1;		// match found
}

// wrappers for above wrapper function //////////////////////////////////////////////////////////////////////////////////////////               

// ret: null: match not found, else: ptr to match str
// n=0, cp whole match; n=1..9, cp submatch
char *
regexp_match_str(struct regexp_context_t *context, int n)
{
	static char buff[1024];
	if (regexp_match_cpy(context, n, buff, 1024) == 0)
		return NULL;
	return (buff);
}

// ret 0: match not found, 1: match found & replaced, <0: buff too small
int
regexp_match_replace(struct regexp_context_t *context, char *replace, char *origdata, char *buff, int buff_len)
{
	int origdata_len = 0;
	int replace_len = 0;
	int offset = 0;
	int match_count = 0;

	if (origdata == NULL || replace == NULL || buff == NULL)
		return -2;

	//clear target buffer
	memset(buff, 0x0, buff_len);

	origdata_len = strlen(origdata);
	replace_len = strlen(replace);

	while (regexp_match(context, origdata + offset)) {
		int match_len = context->regs.end[0] - context->regs.start[0];
		
		/* copy the unmatched string before offset into target */
		if (strlen(buff) + context->regs.start[0]  >= buff_len)
			return -2;
		memcpy(buff + strlen(buff), origdata + offset, context->regs.start[0]);
			
		/* copy replace string into target */
		if (replace_len > match_len && strlen(buff) + (replace_len - match_len) >= buff_len)
			return -2;
		memcpy(buff + strlen(buff), replace, replace_len);

		/* seek to end of match string */
		offset += context->regs.end[0];
		
		match_count++;
	}

	/* copy the remain string of origdata into target */
	if (strlen(buff) + origdata_len - offset >= buff_len)
		return -2;
	memcpy(buff + strlen(buff), origdata + offset, origdata_len - offset);

	return match_count;
}
#endif

#ifdef TEST
int
main(int argc, char **argv)
{
	char buff[1024];
	char buff2[1024];
	struct regexp_context_t re_context;
	int i;

	strcpy(buff, "[this_is_a_test]");

	if (argc < 2)
		exit(1);
	for (i = 0;; i++) {
		printf("%d\n", i);
		
		regexp_prepare(&re_context, "\\[([^\\[\\]]+)\\]");		
		if (regexp_match(&re_context, argv[1])) {
			regexp_match_cpy(&re_context, 1, buff2, 1024);
			printf("hit %s\n", buff2);
		} else {
			printf("miss\n");
		}
		regexp_release(&re_context);		
		
		regexp_prepare(&re_context, "\\[([^\\[\\]]+)\\]");		
		if (regexp_match_replace(&re_context, "", argv[1], buff2, 1024) > 0) {
			printf("Replace pattern with empty , result = %s\n", buff2);
		} else {
			printf("Replace Error\n");
		}
		regexp_release(&re_context);		

		if (i % 10 == 0)
			sleep(1);
	}
	exit(0);
}
#endif
