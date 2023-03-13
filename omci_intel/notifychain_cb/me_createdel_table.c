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
 * Module  : notifychain_cb
 * File    : me_createdel_table.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <ctype.h>
#include <errno.h>
#include <assert.h>
 
#include "list.h"
#include "notify_chain.h"
#include "me_createdel_table.h"
#include "fwk_rwlock.h"
#include "util.h"
#include "conv.h"
#include "env.h"

#define STRING_LEN_MAX	1024
#define TOKEN_COUNT_MAX	512

#define EVENT_STATE	0
#define CLASSID_STATE	1
#define RELATIVE_STATE	2

struct fwk_rwlock_t me_createdel_table_lock;
struct list_head me_createdel_table_list;
static int me_createdel_init = 0;

int
isall_digit(char *str)
{
	char *cur = str;
	while (*cur != '\0') {
		if (!isdigit(*cur)) {
			dbprintf(LOG_DEBUG, "ch err=%c\n", *cur);
			return 0;
		}
		dbprintf(LOG_DEBUG, "ch=%c\n", *cur);
		cur++;
	}
	return 1;
}

/* type=> event:0, classid:1, relative:2*/
int
me_createdel_table_parse(unsigned char type, char *token, struct instance_type_t *inst_type)
{
	switch (type) {
	case EVENT_STATE:	/*0 */
		if (token == NULL) {
			dbprintf(LOG_ERR, "unknow event token\n");
			return -1;
		}

		if (strcasecmp("create", token) == 0) {
			return NOTIFY_CHAIN_EVENT_ME_CREATE;
		} else if (strcasecmp("delete", token) == 0) {
			return NOTIFY_CHAIN_EVENT_ME_DELETE;
		} else if (strcasecmp("update", token) == 0) {
			return NOTIFY_CHAIN_EVENT_ME_UPDATE;
		} else if (strcasecmp("boot", token) == 0) {
			return NOTIFY_CHAIN_EVENT_BOOT;
		} else if (strcasecmp("shutdown", token) == 0) {
			return NOTIFY_CHAIN_EVENT_SHUTDOWN;
		} else {
			dbprintf(LOG_ERR, "unknow event\n");
			return -1;
		}
		break;

	case CLASSID_STATE:	/*1 */
		if (token == NULL) {
			dbprintf(LOG_ERR, "unknow classid token\n");
			return -1;
		}
		return atoi(token);
		break;

	case RELATIVE_STATE:	/*2 */
		if (token == NULL) {
			dbprintf(LOG_ERR, "unknow classid token\n");
			return -1;
		}

		if (strcasecmp(".", token) == 0) {
			inst_type->state= INSTANCE_RELATIVITY;
			inst_type->start = 0;
			inst_type->end = 0;
			return 0;
		} else if (strcasecmp("*", token) == 0) {
			inst_type->state = INSTANCE_ALL;
			inst_type->start = 0;
			inst_type->end = 0;
			return 0;
		} else {
			inst_type->state = 0;

			if (isall_digit(token)) {
				dbprintf(LOG_DEBUG, "token=%s\n", token);
				inst_type->start = atoi(token);
				inst_type->end = 0;
				return 0;
			} else {
				char *cur = token;
				while (*cur != '\0') {
					if ((*cur) == '-') {
						*cur = '\0';
						dbprintf(LOG_DEBUG, "start token=%s\n", token);
						inst_type->start = atoi(token);
						cur++;
						break;	/*2-23=> 2\023\0 */
					}
					cur++;
				}
				dbprintf(LOG_DEBUG, "end token=%s\n", cur);
				inst_type->end = atoi(cur);
				return 0;
			}
		}
		break;
	}

	dbprintf(LOG_ERR, "error token\n");
	return -1;
}

int
me_createdel_table_load(char *filename)
{
	FILE *fptr;
	char strbuf[STRING_LEN_MAX], strtmp[STRING_LEN_MAX];
	char *strtoken[TOKEN_COUNT_MAX], event_token[TOKEN_COUNT_MAX], classid[TOKEN_COUNT_MAX], *ch, *next_start;
	int len, i, ret;
	struct me_createdel_src_t *me_createdel_src;
	struct me_createdel_dst_t *me_createdel_dst;
	struct instance_type_t inst_type;

	if ((fptr = fopen(filename, "r")) == NULL) {	/*which file you want to parser */
		dbprintf(LOG_ERR, "Can't open %s (%s)\n", filename, strerror(errno));
		return -1;
	}

	fwk_rwlock_create(&me_createdel_table_lock);

	if(!me_createdel_init) {
		INIT_LIST_HEAD(&me_createdel_table_list);
		me_createdel_init = 1;
	}

	fwk_rwlock_wrlock(&me_createdel_table_lock);	//atomic operator

	//while (fgets(strtmp, STRING_LEN_MAX, fptr) != NULL) {

	while (util_fgets(strtmp, STRING_LEN_MAX, fptr, 1, 1)!=NULL) {
		struct me_createdel_src_t *me_createdel_table_src;
		struct me_createdel_dst_t *me_createdel_table_dst;
		int me_createdel_hit = 0;
		
		strcpy(strbuf, util_trim(strtmp));
		if (strbuf[0] == '#' || strbuf[0] == 0)
			continue;

		me_createdel_src = malloc_safe(sizeof (struct me_createdel_src_t));
		dbprintf(LOG_DEBUG, "%s\n", strbuf);
		if ((ch = strchr(strbuf, '/')) == NULL) {
			dbprintf(LOG_ERR, "%s: event miss '/'\n", filename);
			me_createdel_table_free();
			fclose(fptr);
			return -1;
		} else {
			next_start = ch + 1;
			*ch = 0x0;
			strncpy(event_token, util_trim(strbuf), TOKEN_COUNT_MAX);
			if ((ret = me_createdel_table_parse(EVENT_STATE, event_token, &inst_type)) < 0){
				me_createdel_table_free();
				fclose(fptr);
				return -1;
			} else
				me_createdel_src->event = ret;

			memcpy(strbuf, next_start, sizeof(strbuf));
			dbprintf(LOG_DEBUG, "EVENT[%s]\n", event_token);
		}

		if ((ch = strchr(strbuf, '=')) == NULL) {
			dbprintf(LOG_ERR, "%s: event miss '='\n", filename);
			me_createdel_table_free();
			fclose(fptr);
			return -1;
		} else {
			next_start = ch + 1;
			*ch = 0x0;
			strncpy(classid, util_trim(strbuf), TOKEN_COUNT_MAX);
			if ((ret = me_createdel_table_parse(CLASSID_STATE, classid, &inst_type)) < 0){
				me_createdel_table_free();
				fclose(fptr);
				return -1;
			} else {
				dbprintf(LOG_DEBUG, "classid=[%d]\n", ret);
				me_createdel_src->classid = ret;
			}

			memcpy(strbuf, next_start, sizeof(strbuf));
			dbprintf(LOG_DEBUG, "[%s]", classid);
		}

		dbprintf(LOG_DEBUG, "=>[%s]\n", strbuf);
		len = conv_str2array(strbuf, ",", strtoken, TOKEN_COUNT_MAX);
		dbprintf(LOG_DEBUG, "token num=%d", len);
		INIT_LIST_HEAD(&me_createdel_src->list_frame_ptr_dst);
		for (i = 0; i < len; i++) {
			dbprintf(LOG_DEBUG, "[%s]\n", strtoken[i]);

			me_createdel_dst = malloc_safe(sizeof (struct me_createdel_dst_t));
			if ((ch = strchr(strtoken[i], ':')) == NULL) {
				dbprintf(LOG_ERR, "classid miss ':'\n");
				me_createdel_table_free();
				fclose(fptr);
				return -1;
			} else {
				next_start = ch + 1;
				*ch = 0x0;
				strncpy(event_token, util_trim(strtoken[i]), TOKEN_COUNT_MAX);
				if ((ret = me_createdel_table_parse(CLASSID_STATE, event_token, &inst_type)) < 0){
					me_createdel_table_free();
					fclose(fptr);
					return -1;
				} else
					me_createdel_dst->classid = ret;

				strcpy(strtoken[i], next_start);
				dbprintf(LOG_DEBUG, "[%d]\n", ret);
			}

			strncpy(event_token, util_trim(strtoken[i]), TOKEN_COUNT_MAX);
			if ((ret = me_createdel_table_parse(RELATIVE_STATE, event_token, &inst_type)) < 0){
				me_createdel_table_free();
				fclose(fptr);
				return -1;
			}

			me_createdel_dst->instance_type.state = inst_type.state;
			me_createdel_dst->instance_type.start = inst_type.start;
			me_createdel_dst->instance_type.end = inst_type.end;
			
			me_createdel_hit = 0;
			list_for_each_entry(me_createdel_table_src, &me_createdel_table_list, list_frame_ptr_src) {
				if((me_createdel_table_src->event == me_createdel_src->event) && (me_createdel_table_src->classid == me_createdel_src->classid)) {
					list_for_each_entry(me_createdel_table_dst, &me_createdel_table_src->list_frame_ptr_dst, list_frame_ptr_dst) {
						if(me_createdel_table_dst->classid == me_createdel_dst->classid) {
							me_createdel_table_dst->instance_type.state = me_createdel_dst->instance_type.state;
							me_createdel_table_dst->instance_type.start = me_createdel_dst->instance_type.start;
							me_createdel_table_dst->instance_type.end = me_createdel_dst->instance_type.end;
							free_safe(me_createdel_dst);
							me_createdel_hit = 1;
							break;
						}
					}
					if(!me_createdel_hit)
						list_add_tail(&me_createdel_dst->list_frame_ptr_dst, &me_createdel_table_src->list_frame_ptr_dst);
					me_createdel_hit = 1;
					break;
				}
			}
			if(!me_createdel_hit)
				list_add_tail(&me_createdel_dst->list_frame_ptr_dst, &me_createdel_src->list_frame_ptr_dst);
		}
		me_createdel_hit = 0;
		list_for_each_entry(me_createdel_table_src, &me_createdel_table_list, list_frame_ptr_src) {
			if((me_createdel_table_src->event == me_createdel_src->event) && (me_createdel_table_src->classid == me_createdel_src->classid)) {
				free_safe(me_createdel_src);
				me_createdel_hit = 1;
				break;
			}
		}
		if(!me_createdel_hit)
			list_add_tail(&me_createdel_src->list_frame_ptr_src, &me_createdel_table_list);
	}
	fwk_rwlock_unlock(&me_createdel_table_lock);	//atomic operator

	fclose(fptr);
	return 0;
}

int
me_createdel_table_dump(int fd)
{
	struct list_head *mlist, *nlist, *m, *n;

	fwk_rwlock_rdlock(&me_createdel_table_lock);	//atomic operator
	list_for_each_safe(mlist, m, &me_createdel_table_list) {
		struct me_createdel_src_t *p = list_entry(mlist, struct me_createdel_src_t, list_frame_ptr_src);
		util_fdprintf(fd, "%d/%d=", p->event, p->classid);

		list_for_each_safe(nlist, n, &(p->list_frame_ptr_dst)) {
			struct me_createdel_dst_t *q = list_entry(nlist, struct me_createdel_dst_t, list_frame_ptr_dst);
			if (q->instance_type.state != 0) {
				if( q->instance_type.state == INSTANCE_RELATIVITY )
					util_fdprintf(fd, "%d:. ", q->classid);
				else if ( q->instance_type.state == INSTANCE_ALL)
					util_fdprintf(fd, "%d:* ", q->classid);

			} else {
				if (q->instance_type.end == 0)
					util_fdprintf(fd, "%d:%d ", q->classid, q->instance_type.start); //never happen
				else
					util_fdprintf(fd, "%d:%d-%d ", q->classid, q->instance_type.start, q->instance_type.end);
			}
		}
		util_fdprintf(fd, "\n");
	}
	fwk_rwlock_unlock(&me_createdel_table_lock);	//atomic operator
	return 0;
}

/* should call this each retrun 1 ? */
int
me_createdel_table_free()
{
	struct list_head *mlist, *nlist, *m, *n;

	fwk_rwlock_wrlock(&me_createdel_table_lock);	//atomic operator
	list_for_each_safe(mlist, m, &me_createdel_table_list) {
		struct me_createdel_src_t *p = list_entry(mlist, struct me_createdel_src_t, list_frame_ptr_src);

		list_for_each_safe(nlist, n, &(p->list_frame_ptr_dst)) {
			struct me_createdel_dst_t *q = list_entry(nlist, struct me_createdel_dst_t, list_frame_ptr_dst);
			list_del(&q->list_frame_ptr_dst);
			free_safe(q);
		}
		list_del(&p->list_frame_ptr_src);
		free_safe(p);
	}
	fwk_rwlock_unlock(&me_createdel_table_lock);	//atomic operator
	return 0;
}

int
me_createdel_traverse_table(unsigned char event,
			    unsigned short classid, unsigned short instance_num, unsigned short meid, void *data_ptr)
{
	struct list_head *mlist, *nlist, *m, *n;

	fwk_rwlock_rdlock(&me_createdel_table_lock);	//atomic operator

	list_for_each_safe(mlist, m, &me_createdel_table_list) {
		struct me_createdel_src_t *p = list_entry(mlist, struct me_createdel_src_t, list_frame_ptr_src);

		if (p->event == event && p->classid == classid) {
			if (event == NOTIFY_CHAIN_EVENT_BOOT)
				event = NOTIFY_CHAIN_EVENT_ME_CREATE;
			list_for_each_safe(nlist, n, &(p->list_frame_ptr_dst)) {
				struct me_createdel_dst_t *q =
				    list_entry(nlist, struct me_createdel_dst_t, list_frame_ptr_dst);
				if (q->instance_type.end == 0)
					dbprintf(LOG_DEBUG, "call:classid %d, instance_type.start:%d ",
						 q->classid, q->instance_type.start);
				else
					dbprintf(LOG_DEBUG, "call:classid %d, instance_type:%d-%d ",
						 q->classid, q->instance_type.start, q->instance_type.end);
				notify_chain_notify(event, q->classid, instance_num, meid);
			}
		}
	}
	fwk_rwlock_unlock(&me_createdel_table_lock);	//atomic operator
	return 0;
}
