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
 * File    : memdbg.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <ctype.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "list.h"
#include "util.h"
#include "fwk_rwlock.h"
#include "memdbg.h"

#if MEMDBG_ENABLE

struct memdbg_entry_t {
	struct list_head nodeptr;
	void *addr;
	int size;
	char *filename;
	int linenum;
	char *funcname;
};

struct summary_entry_t {
	struct list_head nodeptr;
	int count;
	unsigned int size;
	unsigned long total_size;
	char *filename;
	int linenum;
	char *funcname;
};

static struct list_head memdbg_list[1024];
static struct list_head summary_list[1024];
static int is_memdbg_inited=0;

struct fwk_rwlock_t memdbg_list_rwlock;

static void
memdbg_init(void)
{
	int i;
	for (i=0; i<1024; i++) {
		INIT_LIST_HEAD(&memdbg_list[i]);
		INIT_LIST_HEAD(&summary_list[i]);
	}
	fwk_rwlock_create(&memdbg_list_rwlock);
}

static struct memdbg_entry_t *
memdbg_locate(void *addr)
{
	struct memdbg_entry_t *entry;
	struct list_head *list=&memdbg_list[(((unsigned long)addr)>>4)%1024];
	
	fwk_rwlock_rdlock(&memdbg_list_rwlock);
	list_for_each_entry(entry, list, nodeptr) {
		if (entry->addr==addr) {
			fwk_rwlock_unlock(&memdbg_list_rwlock);
			return entry;
		}
	}
	fwk_rwlock_unlock(&memdbg_list_rwlock);
	return NULL;
}	

int
memdbg_add(void *addr, int size, char *filename, int linenum, char *funcname)
{
	struct memdbg_entry_t *entry;
	struct list_head *list=&memdbg_list[(((unsigned long)addr)>>4)%1024];

	if (!is_memdbg_inited) {
		is_memdbg_inited=1;
		memdbg_init();
	}		
	
	entry=memdbg_locate(addr);
	if (entry) {
		util_dbprintf(omci_env_g.debug_level, LOG_EMERG, 0, "%s:%d %s(), repeated alloc on %p? previously by %s:%d %s()\n", 
			filename, linenum, funcname, addr, entry->filename, entry->linenum, entry->funcname);
		return -1;
	}

	entry=malloc(sizeof(struct memdbg_entry_t));
	memset(entry, 0, sizeof(struct memdbg_entry_t));
	entry->addr=addr;
	entry->size=size;
	entry->filename=filename;
	entry->linenum=linenum;
	entry->funcname=funcname;

	fwk_rwlock_wrlock(&memdbg_list_rwlock);
	list_add(&entry->nodeptr, list);
	fwk_rwlock_unlock(&memdbg_list_rwlock);
	return 0;
}

int
memdbg_del(void *addr, char *filename, int linenum, char *funcname)
{
	struct memdbg_entry_t *entry;
	
	if (!is_memdbg_inited) {
		is_memdbg_inited=1;
		memdbg_init();
	}		
	
	entry=memdbg_locate(addr);
	if (entry==NULL) {
		util_dbprintf(omci_env_g.debug_level, LOG_EMERG, 0, "%s:%d %s(), try to free non allocated addr %p?\n", 
			filename, linenum, funcname, addr);
		return -1;
	}

	fwk_rwlock_wrlock(&memdbg_list_rwlock);
	list_del(&entry->nodeptr);
	fwk_rwlock_unlock(&memdbg_list_rwlock);
	free(entry);
	return 0;
}

////////////////////////////////////////////////////////////////

static struct summary_entry_t *
summary_locate(char *filename, int linenum)
{
	struct summary_entry_t *entry;
	struct list_head *list=&summary_list[((unsigned long)filename+linenum)%1024];
	
	list_for_each_entry(entry, list, nodeptr) {
		if (entry->filename==filename && entry->linenum==linenum)
			return entry;
	}
	return NULL;
}	

static int
summary_add(char *filename, int linenum, char *funcname, int size)
{
	struct summary_entry_t *entry;
	struct list_head *list=&summary_list[((unsigned long)filename+linenum)%1024];
	
	entry=summary_locate(filename, linenum);
	if (entry) {
		entry->count++;
		if (size>entry->size)
			entry->size=size;
		entry->total_size+=size;
	} else {
		entry=malloc(sizeof(struct summary_entry_t));
		memset(entry, 0, sizeof(struct summary_entry_t));
		entry->filename=filename;
		entry->linenum=linenum;
		entry->funcname=funcname;
		entry->size=size;
		entry->total_size=size;
		entry->count=1;
		list_add(&entry->nodeptr, list);
	}
	return 0;
}

static int
summary_del_all(void)
{
	struct summary_entry_t *entry, *entry_n;
	int i;
	
	for (i=0; i<1024; i++) {
		struct list_head *list=&summary_list[i];	
		list_for_each_entry_safe(entry, entry_n, list, nodeptr) {
			list_del(&entry->nodeptr);
			free(entry);
		}
	}
	return 0;
}

////////////////////////////////////////////////////////////////
int
memdbg_dump(int fd)
{
	int i;
	unsigned long total_size=0;

	for (i=0; i<1024; i++) {
		struct list_head *list=&memdbg_list[i];		
		struct memdbg_entry_t *entry;

		fwk_rwlock_rdlock(&memdbg_list_rwlock);
		list_for_each_entry(entry, list, nodeptr) {
			summary_add(entry->filename, entry->linenum, entry->funcname, entry->size);
		}
		fwk_rwlock_unlock(&memdbg_list_rwlock);
	}
	
	for (i=0; i<1024; i++) {
		struct list_head *list=&summary_list[i];
		struct summary_entry_t *entry;
		list_for_each_entry(entry, list, nodeptr) {
			util_fdprintf(fd, "%4d %s:%d %s() size=%u (%lu)\n", entry->count,
				entry->filename, entry->linenum, entry->funcname, entry->size, entry->total_size);
			total_size+=entry->total_size;
		}
	}
	util_fdprintf(fd, "total size=%lu\n", total_size);
	summary_del_all();
	return 0;
}

#else
int memdbg_add(void *addr, int size, char *filename, int linenum, char *funcname) { return 0; }
int memdbg_del(void *addr) { return 0; }
int memdbg_dump(int fd) { return 0; }
#endif

