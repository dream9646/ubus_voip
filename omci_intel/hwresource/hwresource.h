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
 * Module  : hwresource
 * File    : hwresource.h
 *
 ******************************************************************/

#ifndef __HWRESOURCE_H__
#define __HWRESOURCE_H__

/* 
for hw resource me
1st attr is occupied_flag, 0=free, 1=occupied
2nd attr is tp pointer, it points to the meid of related public me 
*/

struct hwresource_relation_t {
	unsigned short public_classid;
	unsigned short private_classid;
	int (*match_funcptr)(struct me_t *public_me, struct me_t *private_me);		// 0: mismatch, 1: match
	int (*dismiss_funcptr)(struct me_t *public_me, struct me_t *private_me);	// 0:ok, <0 err
	struct list_head hwresource_relation_node;
};

extern struct list_head hwresource_relation_list;

/* hwresource.c */
int hwresource_preinit(void);
int hwresource_is_related(unsigned short classid);
struct me_t *hwresource_public2private(struct me_t *public_me);
struct me_t *hwresource_private2public(struct me_t *private_me);
int hwresource_register(unsigned short public_classid, char *private_classname, int (*match_funcptr)(struct me_t *public_me, struct me_t *private_me), int (*dismiss_funcptr)(struct me_t *public_me, struct me_t *private_me));
int hwresource_unregister(unsigned short public_classid);
struct me_t *hwresource_alloc(struct me_t *public_me);
int hwresource_free(struct me_t *public_me);
char *hwresource_devname(struct me_t *public_me);

/* hwresource_cb.c */
int hwresource_cb_init(void);

#endif

