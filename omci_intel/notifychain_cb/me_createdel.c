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
 * File    : me_createdel.c
 *
 ******************************************************************/


#include <stdio.h>
#include <syslog.h>
#include "list.h"
#include "notify_chain.h"
#include "me_createdel_table.h"
#include "meinfo.h"
#include "fwk_rwlock.h"
#include "util.h"

extern struct list_head me_createdel_table_list;
extern struct fwk_rwlock_t me_createdel_table_lock;

int
me_create_instance(unsigned short classid, unsigned short instance_num)
{
	int ret;
	struct me_t *me;

	dbprintf(LOG_INFO, "create %d:%d\n", classid, instance_num);
	me = meinfo_me_alloc(classid, instance_num);
	if (!me)
		return -1;
	meinfo_me_load_config(me);

	//create by ont
	me->is_cbo = 0;

	if ((ret = meinfo_me_create(me)) < 0)
	{
		//release
		meinfo_me_release(me);
		dbprintf(LOG_ERR, "me chain create fail, classid=%u, instance_num=%d, err=%d\n", classid, instance_num, ret);
	}
	/* chk if still need */
	//notify_chain_notify(NOTIFY_CHAIN_EVENT_ME_CREATE, classid, instance_num, 0);
	return 0;
}

int
me_delete_instance(unsigned short classid, struct instance_type_t instance_type, unsigned short instance_num)
{
	/* chk if instance not exist then return */
	if (instance_type.state == INSTANCE_ALL){
		meinfo_me_delete_by_classid(classid);
		dbprintf(LOG_NOTICE, "del classid %d\n", classid);
	} else {
		meinfo_me_delete_by_instance_num(classid, instance_num);
		dbprintf(LOG_NOTICE, "del %d:%d\n", classid, instance_num);
	}
	//notify_chain_notify(NOTIFY_CHAIN_EVENT_ME_DELETE, classid, instance_num, 0);
	return 0;
}

int
me_createdel_boot_handler(unsigned char event,
			  unsigned short classid, unsigned short instance_num, unsigned short meid, void *data_ptr)
{
	struct list_head *mlist, *nlist, *m, *n;

	fwk_rwlock_rdlock(&me_createdel_table_lock);	//atomic operator
	if (event == NOTIFY_CHAIN_EVENT_BOOT) {

		/* create me ONT-G classid 256 */
		dbprintf(LOG_INFO, "Boot call:classid %d", classid);

		list_for_each_safe(mlist, m, &me_createdel_table_list) {
			struct me_createdel_src_t *p = list_entry(mlist, struct me_createdel_src_t, list_frame_ptr_src);

			if (p->event == event && p->classid == classid) {

				list_for_each_safe(nlist, n, &(p->list_frame_ptr_dst)) {
					struct me_createdel_dst_t *q = list_entry(nlist, struct me_createdel_dst_t,
										  list_frame_ptr_dst);
					//for single or multipe instance
					if ( q->instance_type.state != 0 )
						dbprintf(LOG_DEBUG, "Not expect in boot!");

					if (q->instance_type.end == 0) {
						instance_num = q->instance_type.start;
						me_create_instance(q->classid, instance_num);
					} else {
						int i;
						for (i = q->instance_type.start; i <= q->instance_type.end; i++)
							me_create_instance(q->classid, i);
					}
				}
			}
		}
	}
	fwk_rwlock_unlock(&me_createdel_table_lock);	//atomic operator
	return 0;
}

int
me_createdel_shutdown_handler(unsigned char event,
			      unsigned short classid, unsigned short instance_num, unsigned short meid, void *data_ptr)
{
	struct list_head *mlist, *nlist, *m, *n;

	/* 256 is ONT-G classid */
	fwk_rwlock_rdlock(&me_createdel_table_lock);	//atomic operator
	if (event == NOTIFY_CHAIN_EVENT_SHUTDOWN) {

		dbprintf(LOG_INFO, "Shutdown call:classid %d", classid);

		list_for_each_safe(mlist, m, &me_createdel_table_list) {
			struct me_createdel_src_t *p = list_entry(mlist, struct me_createdel_src_t, list_frame_ptr_src);

			if (p->event == event && p->classid == classid) {

				list_for_each_safe(nlist, n, &(p->list_frame_ptr_dst)) {
					struct me_createdel_dst_t *q = list_entry(nlist, struct me_createdel_dst_t,
										  list_frame_ptr_dst);

					if (q->instance_type.state != INSTANCE_RELATIVITY)
						instance_num = q->instance_type.start;	// *, or start 

					me_delete_instance(q->classid, q->instance_type, instance_num);
				}
			}
		}
	}
	fwk_rwlock_unlock(&me_createdel_table_lock);	//atomic operator
	return 0;
}

int
me_createdel_create_handler(unsigned char event,
			    unsigned short classid, unsigned short instance_num, unsigned short meid, void *data_ptr)
{

	struct list_head *mlist, *nlist, *m, *n;

	if (event == NOTIFY_CHAIN_EVENT_ME_CREATE) {
		fwk_rwlock_rdlock(&me_createdel_table_lock);	//atomic operator
		list_for_each_safe(mlist, m, &me_createdel_table_list) {
			struct me_createdel_src_t *p = list_entry(mlist, struct me_createdel_src_t, list_frame_ptr_src);

			if (p->event == event && p->classid == classid) {
				list_for_each_safe(nlist, n, &(p->list_frame_ptr_dst)) {

					struct me_createdel_dst_t *q = list_entry(nlist, struct me_createdel_dst_t,
										  list_frame_ptr_dst);

					if (q->instance_type.state != INSTANCE_RELATIVITY)
						instance_num = q->instance_type.start;

					me_create_instance(q->classid, instance_num);
				}
			}
		}
		fwk_rwlock_unlock(&me_createdel_table_lock);	//atomic operator
	}
	return 0;
}

int
me_createdel_delete_handler(unsigned char event,
			    unsigned short classid, unsigned short instance_num, unsigned short meid, void *data_ptr)
{

	struct list_head *mlist, *nlist, *m, *n;

	fwk_rwlock_rdlock(&me_createdel_table_lock);	//atomic operator
	if (event == NOTIFY_CHAIN_EVENT_ME_DELETE) {
		list_for_each_safe(mlist, m, &me_createdel_table_list) {
			struct me_createdel_src_t *p = list_entry(mlist, struct me_createdel_src_t, list_frame_ptr_src);

			if (p->event == event && p->classid == classid) {
				list_for_each_safe(nlist, n, &(p->list_frame_ptr_dst)) {

					struct me_createdel_dst_t *q = list_entry(nlist, struct me_createdel_dst_t,
										  list_frame_ptr_dst);

					if (q->instance_type.state != INSTANCE_RELATIVITY)
						instance_num = q->instance_type.start;

					me_delete_instance(q->classid, q->instance_type, instance_num);
				}
			}
		}
	}
	fwk_rwlock_unlock(&me_createdel_table_lock);	//atomic operator
	return 0;
}
