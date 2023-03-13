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
 * Module  : batchtab
 * File    : batchtab.c
 *
 ******************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <sys/time.h>

#include "list.h"
#include "util.h"
#include "conv.h"
#include "gpon_sw.h"
#include "coretask.h"
#include "batchtab.h"

struct list_head batchtab_list;
struct timeval batchtab_extevent_time;

int
batchtab_preinit(void)
{
	INIT_LIST_HEAD(&batchtab_list);
	util_get_uptime(&batchtab_extevent_time);
	return 0;
}

struct batchtab_t *
batchtab_find_by_name(char *name)
{
	struct batchtab_t *batptr;
	if (name) {
		list_for_each_entry(batptr, &batchtab_list, batchtab_node) {
			if (strcmp(batptr->name,name)==0)
				return batptr;
		}
	}
	return NULL;
}

///////////////////////////////////////////////////////////////////
// 0: ok, 1: unissued due to lock, -1: err
static int
batchtab_table_gen_do(struct batchtab_t *batptr)
{
	int ret;
	struct timeval start_time;

	if (batptr == NULL)
		return -1;
	if (batptr->write_lock || batptr->ref_count)
		return 1;

   	batptr->write_lock = 1;	// batch is doing tablegen

	fwk_mutex_lock(&batptr->mutex);
	batptr->ref_count++;
	fwk_mutex_unlock(&batptr->mutex);

	if(batptr->table_release_funcptr)
	{
   		batptr->table_release_funcptr(batptr->table_data);
	}

	util_get_uptime(&start_time);
	ret = batptr->table_generate_funcptr(&batptr->table_data);
	util_get_uptime(&batptr->table_gen_time);
	batptr->exe_time_sw = util_timeval_diff_usec(&batptr->table_gen_time, &start_time);

	if (batptr->exe_time_sw > batptr->exe_time_sw_max)
		batptr->exe_time_sw_max = batptr->exe_time_sw;
	if (batptr->exe_time_sw > 1000000) {
		dbprintf_bat(LOG_ERR, "batchtab %s table_gen, omci_update=%u, crosstab_update=%d, ret %d, exec %d usec > 1sec!\n",
			batptr->name, batptr->omci_update_count, batptr->crosstab_update_count, ret, batptr->exe_time_sw);
	} else {
		dbprintf_bat(LOG_WARNING, "batchtab %s table_gen, omci_update=%u, crosstab_update=%d, ret %d, exec %d usec\n",
			batptr->name, batptr->omci_update_count, batptr->crosstab_update_count, ret, batptr->exe_time_sw);
	}

	fwk_mutex_lock(&batptr->mutex);
	batptr->ref_count--;
	fwk_mutex_unlock(&batptr->mutex);

	batptr->write_lock = 0;

	if (ret < 0)
		return -1;
	return 0;
}

// 0: ok, 1: unissued due to idle not long enough or lock, -1: err
static int
batchtab_table_gen_check(struct batchtab_t *batptr)
{
	int ret;

	if (batptr == NULL)
		return -1;

	// check trigger from omci write
	if (batptr->omci_update_auto ||
	    batptr->omci_update_count > 0 ||
	    batptr->crosstab_update_count > 0) {
		struct timeval now_time;
		long long timediff;

		util_get_uptime(&now_time);

		if (batptr->omci_update_auto) {
			if (batptr->omci_update_count == 0) {
				batptr->omci_update_count = 1;	// fake an omci update for auto
				batptr->omci_update_time = now_time;
				return 1;	// do not issue until omci update has been idle for a while
			}
		}

		timediff = util_timeval_diff_usec(&now_time, &batchtab_extevent_time);
		if (timediff <= omci_env_g.batchtab_extevent_idletime *1000000)
			return 1;	// do not issue as external event idletime is not long enough

		timediff = util_timeval_diff_usec(&now_time, &batptr->omci_update_time);
		if (timediff < 0) {	// time wrap around? align referred timestamp to now
			batptr->omci_update_time = now_time;
			timediff = 0;
		}
		if (timediff <= batptr->omci_idle_timeout*1000000)
			return 1;	// do not issue until related omci update has been idle for a while

		timediff = util_timeval_diff_usec(&now_time, &batptr->table_gen_time);
		if (timediff < 0) {	// time wrap around? align referred timestamp to now
			batptr->table_gen_time = now_time;
			timediff = 0;
		}
		if (timediff <= 1000000)
			return 1;	// do not issue more than once within 1 second

		ret = batchtab_table_gen_do(batptr);
		if (ret == 0) {
			batptr->table_gen_accum_err = 0;
			batptr->omci_update_count = 0;
			batptr->crosstab_update_count = 0;
			batptr->tablegen_update_count++;
		} else {
			util_get_uptime(&now_time);
			if (batptr->table_gen_accum_err == 0)
				batptr->table_gen_accum_err_time = now_time;
			batptr->table_gen_accum_err++;

			if (omci_env_g.batchtab_retry_timeout > 0) {
				timediff = util_timeval_diff_usec(&now_time, &batptr->table_gen_accum_err_time);
				if (timediff < 0) {	// time wrap around? align referred timestamp to now
					batptr->table_gen_accum_err_time = now_time;
					timediff = 0;
				}
				if (timediff > omci_env_g.batchtab_retry_timeout*1000000) {
					dbprintf_bat(LOG_ERR, "batchtab %s table_gen retry timeout, abort! total retry %d\n",
						batptr->name, batptr->table_gen_accum_err);
					batptr->table_gen_accum_err = 0;
					batptr->omci_update_count = 0;
					batptr->crosstab_update_count = 0;
				}
			}
		}
		return ret;
	}

	return 0;
}

// 0: ok, 1: unissued due to lock, -1: err
static int
batchtab_hw_sync_do(struct batchtab_t *batptr)
{
	int ret;
	struct timeval start_time;

	if (batptr == NULL)
		return -1;
	if (batptr->write_lock)
		return 1;
	if (batptr->hw_sync_lock)
		return 1;

	batptr->hw_sync_lock = 1;	// batchtab is doing hwsync
	fwk_mutex_lock(&batptr->mutex);
	batptr->ref_count++;		// table is referred by hwsync
	fwk_mutex_unlock(&batptr->mutex);

	util_get_uptime(&start_time);
	ret = batptr->hw_sync_funcptr(batptr->table_data);
	util_get_uptime(&batptr->hw_sync_time);
	batptr->exe_time_hw = util_timeval_diff_usec(&batptr->hw_sync_time, &start_time);

	if (batptr->exe_time_hw > batptr->exe_time_hw_max)
		batptr->exe_time_hw_max = batptr->exe_time_hw;
	if ( batptr->exe_time_hw > 1000000) {
		dbprintf_bat(LOG_ERR, "batchtab %s hw_sync, table_gen_update=%u, ret %d, exec %d usec > 1sec!\n",
			batptr->name, batptr->tablegen_update_count, ret, batptr->exe_time_hw);
	} else {
		dbprintf_bat(LOG_WARNING, "batchtab %s hw_sync, table_gen_update=%u, ret %d, exec %d usec\n",
			batptr->name, batptr->tablegen_update_count, ret, batptr->exe_time_hw);
	}

	fwk_mutex_lock(&batptr->mutex);
	batptr->ref_count--;
	fwk_mutex_unlock(&batptr->mutex);

	batptr->hw_sync_lock = 0;

	if (ret < 0)
		return -1;
	return 0;
}

// 0: ok, 1: unissued due to lock, -1: err
static int
batchtab_hw_sync_check(struct batchtab_t *batptr)
{
	int ret;

	if (batptr == NULL)
		return -1;

	// check trigger from table_gen or other bachtab
	if (batptr->tablegen_update_count > 0 ||
	    batptr->o5_sequence != coretask_get_o5_sequence()) {
		struct timeval now_time;
		long long timediff;
		util_get_uptime(&now_time);

		timediff = util_timeval_diff_usec(&now_time, &batchtab_extevent_time);
		if (timediff <= omci_env_g.batchtab_extevent_idletime *1000000)
			return 1;	// do not issue as external event idletime is not long enough

		timediff = util_timeval_diff_usec(&now_time, &batptr->hw_sync_time);
		if (timediff < 0) {	// time wrap around? align referred timestamp to now
			batptr->hw_sync_time = now_time;
			timediff = 0;
		}
		if (timediff <= 1000000)
			return 1;	// do not issue more than once in 1 second

		ret = batchtab_hw_sync_do(batptr);
		if (ret == 0) {
			batptr->hw_sync_accum_err = 0;
			batptr->tablegen_update_count = 0;
			batptr->o5_sequence = coretask_get_o5_sequence();
		} else {
			util_get_uptime(&now_time);
			if (batptr->hw_sync_accum_err == 0)
				batptr->hw_sync_accum_err_time = now_time;
			batptr->hw_sync_accum_err++;
			if (omci_env_g.batchtab_retry_timeout > 0) {
				timediff = util_timeval_diff_usec(&now_time, &batptr->hw_sync_accum_err_time);
				if (timediff < 0) {	// time wrap around? align referred timestamp to now
					batptr->hw_sync_accum_err_time = now_time;
					timediff = 0;
				}
				if (timediff > omci_env_g.batchtab_retry_timeout*1000000) {
					dbprintf_bat(LOG_ERR, "batchtab %s hw_sync retry timeout, abort! total retry %d\n",
						batptr->name, batptr->hw_sync_accum_err);
					batptr->hw_sync_accum_err = 0;
					batptr->tablegen_update_count = 0;
					batptr->o5_sequence = coretask_get_o5_sequence();
				}
			}
		}
		return ret;
	}
	return 0;
}

////////////////////////////////////////////////////////////////////
int
batchtab_register(char *name,
	unsigned int omci_update_auto,
	unsigned int omci_idle_timeout,
	int (*bat_init_funcptr)(void),
	int (*bat_finish_funcptr)(void),
	int (*table_generate_funcptr)(void **data),
	int (*table_release_funcptr)(void *data),
	int (*table_dump_funcptr)(int fd, void *data),
	int (*hw_sync_funcptr)(void *data))
{
	struct batchtab_t *batptr;
	struct timeval now_time;

	if (name == NULL ||
	    table_generate_funcptr ==NULL ||
	    table_release_funcptr ==NULL ||
	    hw_sync_funcptr ==NULL) {
		dbprintf_bat(LOG_ERR, "batchtab has null parameter?\n", name);
		return -1;
	}

	batptr = batchtab_find_by_name(name);
	if (batptr) {
		dbprintf_bat(LOG_ERR, "batchtab %s is already registered\n", name);
		return -1;
	}

	batptr = (struct batchtab_t*)malloc_safe(sizeof(struct batchtab_t));
	batptr->name = malloc_safe(strlen(name)+1);
	strcpy(batptr->name, name);

	// prepare content
	batptr->omci_update_auto = omci_update_auto;
	batptr->omci_update_count = 0;
	batptr->crosstab_update_count = 0;
	batptr->tablegen_update_count = 0;

	batptr->write_lock = 0;
	batptr->ref_count = 0;
	batptr->enable = 1;
	batptr->omci_idle_timeout = omci_idle_timeout;
	util_get_uptime(&now_time);
	batptr->omci_update_time = batptr->table_gen_time = batptr->hw_sync_time = now_time;
	batptr->exe_time_sw = batptr->exe_time_hw = batptr->exe_time_sw_max = batptr->exe_time_hw_max = 0;

	batptr->table_data = NULL;
	batptr->bat_init_funcptr = bat_init_funcptr;
	batptr->bat_finish_funcptr = bat_finish_funcptr;
	batptr->table_generate_funcptr = table_generate_funcptr;
	batptr->table_release_funcptr = table_release_funcptr;
	batptr->table_dump_funcptr = table_dump_funcptr;
	batptr->hw_sync_funcptr = hw_sync_funcptr;

	// add to list after init(), or the batchtab might be called before init()
	batptr->bat_init_funcptr();

	fwk_mutex_create(&batptr->mutex);

	list_add_tail(&batptr->batchtab_node, &batchtab_list);

	return 0;
}

static int
_batchtab_unregister(struct batchtab_t *batptr)
{
	if (batptr) {
		// sync latest omci mib state (asuumed to be empty) into hw
		// ps: as unegister is called in omci_shutdown, all tasks are actually stoped, so we don't call tab_gen/hw_sync again
		//batchtab_table_gen_do(batptr);
		//batchtab_hw_sync_do(batptr);

		if (batptr->write_lock)
			dbprintf_bat(LOG_WARNING, "batchtab %s unreg with write_lock==1\n", batptr->name);
		if (batptr->ref_count>0)
			dbprintf_bat(LOG_WARNING, "batchtab %s unreg with ref_count=%d\n", batptr->name, batptr->ref_count);
		if (batptr->table_data)
			batptr->table_release_funcptr(batptr->table_data);

		fwk_mutex_destroy(&batptr->mutex);

		// del from list before finish(), or batchtab might be called after finish()
		list_del(&batptr->batchtab_node);
		batptr->bat_finish_funcptr();

		free_safe(batptr->name);
		free_safe(batptr);
	}
	return 0;
}
int
batchtab_unregister(char *name)
{
	struct batchtab_t *batptr = batchtab_find_by_name(name);

	if (batptr == NULL)
		return -1;
	return _batchtab_unregister(batptr);
}
int
batchtab_unregister_all(void)
{
	struct batchtab_t *batptr, *n;
	list_for_each_entry_safe(batptr, n, &batchtab_list, batchtab_node) {
		_batchtab_unregister(batptr);
	}
	return 0;
}

///////////////////////////////////////////////////////////////////

int
batchtab_omci_update(char *name)
{
	struct batchtab_t *batptr = batchtab_find_by_name(name);
	if (batptr == NULL)
		return -1;
	batptr->omci_update_count++;
	util_get_uptime(&batptr->omci_update_time);
	return 0;
}

int
batchtab_crosstab_update(char *name)
{
	struct batchtab_t *batptr = batchtab_find_by_name(name);
	if (batptr == NULL)
		return -1;
	batptr->crosstab_update_count++;
	return 0;
}

int
batchtab_table_gen(char *name)
{
	struct batchtab_t *batptr = batchtab_find_by_name(name);
	if (batptr == NULL)
		return -1;
	return batchtab_table_gen_do(batptr);
}

int
batchtab_hw_sync(char *name)
{
	struct batchtab_t *batptr = batchtab_find_by_name(name);
	if (batptr == NULL)
		return -1;
	return batchtab_hw_sync_do(batptr);
}

int
batchtab_o5_sequence_is_changed(char *name)
{
	struct batchtab_t *batptr = batchtab_find_by_name(name);
	if (batptr == NULL)
		return 0;
	if (batptr->o5_sequence == coretask_get_o5_sequence())
		return 0;
	return 1;
}

void *
batchtab_table_data_get(char *name)
{
	struct batchtab_t *batptr = batchtab_find_by_name(name);

	if (batptr == NULL)
		return NULL;
	if (batptr->write_lock) {
		dbprintf_bat(LOG_NOTICE, "batchtab %s get table data while write lock on?\n", batptr->name);
		return NULL;
	}

	batchtab_table_gen_check(batptr);
	batchtab_hw_sync_check(batptr);
	if (batptr->table_data)
	{
		fwk_mutex_lock(&batptr->mutex);
		batptr->ref_count++;
		fwk_mutex_unlock(&batptr->mutex);
	}
	return  batptr->table_data;
}

int
batchtab_table_data_put(char *name)
{
	struct batchtab_t *batptr = batchtab_find_by_name(name);

	if (batptr == NULL)
		return -1;
	if (batptr->write_lock)
		dbprintf_bat(LOG_ERR, "batchtab %s put table data while write lock on?\n", batptr->name);

	if (batptr->ref_count >0)
	{
		fwk_mutex_lock(&batptr->mutex);
		batptr->ref_count--;
		fwk_mutex_unlock(&batptr->mutex);
	}
	else
		dbprintf_bat(LOG_ERR, "batchtab %s put table data with ref count ==0?\n", batptr->name);
	return 0;
}

int
batchtab_table_data_release(char *name)
{
	struct batchtab_t *batptr = batchtab_find_by_name(name);

	if (batptr == NULL)
		return -1;
	if (batptr->ref_count!=0) {
		dbprintf_bat(LOG_ERR, "batchtab %s release table data with ref count==%d\n", batptr->name, batptr->ref_count);
		return -2;
	}
	if (batptr->write_lock) {
		dbprintf_bat(LOG_ERR, "batchtab %s release table data while write lock on?\n", batptr->name);
		return -3;
	}
	if (batptr->table_data != NULL) {
		batptr->write_lock = 1;
		fwk_mutex_lock(&batptr->mutex);
		batptr->ref_count++;
		fwk_mutex_unlock(&batptr->mutex);

		batptr->table_release_funcptr(batptr->table_data);
		batptr->table_data = NULL;

		fwk_mutex_lock(&batptr->mutex);
		batptr->ref_count--;
		fwk_mutex_unlock(&batptr->mutex);

		batptr->write_lock = 0;
	}
	return 0;
}

int
batchtab_table_data_dump(int fd, char *name)
{
	struct batchtab_t *batptr = batchtab_find_by_name(name);
	int ret;

	if (batptr == NULL)
		return -1;

	if (batptr->write_lock) {
		dbprintf_bat(LOG_NOTICE, "batchtab %s dump table data while write lock on?\n", batptr->name);
		return -1;
	}

	fwk_mutex_lock(&batptr->mutex);
	batptr->ref_count++;
	fwk_mutex_unlock(&batptr->mutex);

	batchtab_table_gen_check(batptr);
	batchtab_hw_sync_check(batptr);
	ret = batptr->table_dump_funcptr(fd, batptr->table_data);
	if (ret !=0)
		dbprintf_bat(LOG_NOTICE, "batchtab %s dump table data error %d?\n", batptr->name, ret);

	fwk_mutex_lock(&batptr->mutex);
	batptr->ref_count--;
	fwk_mutex_unlock(&batptr->mutex);

	return ret;
}

static int
_batchtab_table_gen_hw_sync(struct batchtab_t *batptr, int (*break_event_check)(void))
{
	int unissued_table_gen = 0;
	int unissued_hw_sync = 0;

	if (batptr == NULL)
		return -1;
	if (batptr->enable == 0)
		return 0;

	unissued_table_gen = batchtab_table_gen_check(batptr);

	// if some serious event comes(eg:omci msg), we exit this loop ASAP
	// and do the left batchtabs in this loop nexttime
	if (break_event_check && break_event_check()>0) {
		if (unissued_table_gen !=0) {
			dbprintf_bat(LOG_DEBUG, "batchtab %s table_gen %s\n", (unissued_table_gen>0)?"unissued":"error");
			return (unissued_table_gen > 0)? 1:-1;
		}
		return 1;	// hw sync skipped because of break event
	}

	unissued_hw_sync = batchtab_hw_sync_check(batptr);

	if (unissued_table_gen !=0) {
		dbprintf_bat(LOG_DEBUG, "batchtab %s table_gen %s\n", (unissued_table_gen>0)?"unissued":"error");
		return (unissued_table_gen > 0)? 1:-1;
	}
	if (unissued_hw_sync !=0) {
		dbprintf_bat(LOG_DEBUG, "batchtab %s hw_sync %s\n", (unissued_hw_sync>0)?"unissued":"error");
		return (unissued_hw_sync > 0)? 1:-1;
	}
	return 0;
}

int
batchtab_table_gen_hw_sync(char *name)
{
	struct batchtab_t *batptr = batchtab_find_by_name(name);
	if (batptr == NULL)
		return -1;
	return _batchtab_table_gen_hw_sync(batptr, NULL);
}

// return the number of unissued hw sync or error happened
int
batchtab_table_gen_hw_sync_all(int (*break_event_check)(void))
{
	struct batchtab_t *batptr;
	int not_issued = 0;
	int err_happened = 0;
	int ret;

	list_for_each_entry(batptr, &batchtab_list, batchtab_node) {
		ret = _batchtab_table_gen_hw_sync(batptr, break_event_check);
		if (ret == 1)
			not_issued++;
		else if (ret <0)
			err_happened++;
		// if some serious event comes(eg:omci msg), we exit this loop ASAP
		// and do the left batchtabs in this loop nexttime
		if (break_event_check && break_event_check()>0)
			break;
	}
	dbprintf_bat(LOG_DEBUG, "batchtab not issued = %d, err happened = %d\n", not_issued, err_happened);
	if (not_issued)
		return not_issued;
	if (err_happened)
		return -1*err_happened;
	return 0;
}

int
batchtab_enable(char *name, int enable)
{
	struct batchtab_t *batptr = batchtab_find_by_name(name);
	if (batptr == NULL)
		return -1;
	batptr->enable = enable?1:0;
	return 0;
}

void
batchtab_extevent_update(void)
{
	util_get_uptime(&batchtab_extevent_time);
}
