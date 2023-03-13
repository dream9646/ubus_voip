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
 * File    : batchtab.h
 *
 ******************************************************************/

#ifndef __BATCHTAB_H__
#define __BATCHTAB_H__

#include <time.h>
#include <sys/time.h>

#include "fwk_mutex.h"

#define BATCHTAB_OMCI_UPDATE_NOAUTO	0
#define BATCHTAB_OMCI_UPDATE_AUTO	1

struct batchtab_t {
	char *name;
	unsigned int omci_update_auto;
	struct timeval omci_update_time;
	struct timeval table_gen_time;
	struct timeval hw_sync_time;
	struct timeval table_gen_accum_err_time;
	struct timeval hw_sync_accum_err_time;
	unsigned int o5_sequence;
	unsigned int omci_update_count;
	unsigned int tablegen_update_count;
	unsigned int crosstab_update_count;
	unsigned int table_gen_accum_err;
	unsigned int hw_sync_accum_err;
	
	unsigned int write_lock;		// whether batchtab is doing tablegen
	unsigned int ref_count;			// how many client are reading to tablegen
	unsigned int hw_sync_lock;		// whether batchtab is doing hwsync
	unsigned int enable;
	unsigned int exe_time_sw;
	unsigned int exe_time_hw;
	unsigned int exe_time_sw_max;		// the max that ever happend
	unsigned int exe_time_hw_max;		// the max that ever happend
	
	void *table_data;
	unsigned int omci_idle_timeout;
	int (*bat_init_funcptr)(void);		// called once on register
	int (*bat_finish_funcptr)(void);	// called once on unregister
	int (*table_generate_funcptr)(void **data);
	int (*table_release_funcptr)(void *data);	// called before tab gen is called
	int (*table_dump_funcptr)(int fd, void *data);
	int (*hw_sync_funcptr)(void *data);
	struct fwk_mutex_t mutex;
	struct list_head batchtab_node;
};

extern struct list_head batchtab_list;
extern struct timeval batchtab_extevent_time;

/* batchtab.c */
int batchtab_preinit(void);
struct batchtab_t *batchtab_find_by_name(char *name);
int batchtab_register(char *name, unsigned int omci_update_auto, unsigned int omci_idle_timeout, int (*bat_init_funcptr)(void), int (*bat_finish_funcptr)(void), int (*table_generate_funcptr)(void **data), int (*table_release_funcptr)(void *data), int (*table_dump_funcptr)(int fd, void *data), int (*hw_sync_funcptr)(void *data));
int batchtab_unregister(char *name);
int batchtab_unregister_all(void);
int batchtab_omci_update(char *name);
int batchtab_crosstab_update(char *name);
int batchtab_table_gen(char *name);
int batchtab_hw_sync(char *name);
int batchtab_o5_sequence_is_changed(char *name);
void *batchtab_table_data_get(char *name);
int batchtab_table_data_put(char *name);
int batchtab_table_data_release(char *name);
int batchtab_table_data_dump(int fd, char *name);
int batchtab_table_gen_hw_sync(char *name);
int batchtab_table_gen_hw_sync_all(int (*break_event_check)(void));
int batchtab_enable(char *name, int enable);
void batchtab_extevent_update(void);

#endif
