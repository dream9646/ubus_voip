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
#include <pthread.h>

#include "voip_list.h"
#include "memdbg.h"
#include "memdbg_log.h"

struct memdbg_entry_t {
	struct voip_list_node nodeptr;
	void *addr;
	int size;
	char *filename;
	int linenum;
	char *funcname;
};

struct summary_entry_t {
	struct voip_list_node nodeptr;
	int count;
	unsigned int size;
	unsigned long total_size;
	char *filename;
	int linenum;
	char *funcname;
};

static struct voip_list_head memdbg_list[1024];
static struct voip_list_head summary_list[1024];
static int is_memdbg_inited=0;
static int is_memdbg_enable=0;

pthread_mutex_t memdbg_list_rwlock;

LOG_DEFINE_MODULE("MEMDBG");

static void
memdbg_init(void)
{
	int i;
	LOG_REGISTER_MODULE();
	for (i=0; i<1024; i++) {
		voip_list_head_init(&memdbg_list[i]);
		voip_list_head_init(&summary_list[i]);
	}
	if(pthread_mutex_init(&memdbg_list_rwlock, NULL) != 0) {
		LOGE("mutex init fail for memdbg");
		exit(0);
	}
}

void memdbg_enable(void) { is_memdbg_enable = 1; }
void memdbg_disable(void) { is_memdbg_enable = 0; }

static struct memdbg_entry_t *
memdbg_locate(void *addr)
{
	struct memdbg_entry_t *entry;
	struct voip_list_head *list=&memdbg_list[(((unsigned long)addr)>>4)%1024];

	pthread_mutex_lock (&memdbg_list_rwlock);
	voip_list_for_each(list, entry, nodeptr) {
		if (entry->addr==addr) {
			pthread_mutex_unlock(&memdbg_list_rwlock);
			return entry;
		}
	}
	pthread_mutex_unlock (&memdbg_list_rwlock);
	return NULL;
}

static int
memdbg_add(void *addr, int size, char *filename, int linenum, char *funcname)
{
	struct memdbg_entry_t *entry;
	struct voip_list_head *list=&memdbg_list[(((unsigned long)addr)>>4)%1024];

	if (!is_memdbg_inited) {
		is_memdbg_inited=1;
		memdbg_init();
	}

	entry=memdbg_locate(addr);
	if (entry) {
		LOGE("%s:%d %s(), repeated alloc on %p? previously by %s:%d %s()\n",
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

	pthread_mutex_lock (&memdbg_list_rwlock);
	voip_list_add(list, &entry->nodeptr);
	pthread_mutex_unlock (&memdbg_list_rwlock);
	return 0;
}

static int
memdbg_del(void *addr, char *filename, int linenum, char *funcname)
{
	struct memdbg_entry_t *entry;

	if (!is_memdbg_inited) {
		is_memdbg_inited=1;
		memdbg_init();
	}

	entry=memdbg_locate(addr);
	if (entry==NULL) {
		LOGE("%s:%d %s(), try to free non allocated addr %p?",
			filename, linenum, funcname, addr);
		return -1;
	}

	pthread_mutex_unlock(&memdbg_list_rwlock);
	voip_list_del(&entry->nodeptr);
	pthread_mutex_unlock(&memdbg_list_rwlock);
	free(entry);
	return 0;
}

////////////////////////////////////////////////////////////////

static struct summary_entry_t *
summary_locate(char *filename, int linenum)
{
	struct summary_entry_t *entry;
	struct voip_list_head *list=&summary_list[((unsigned long)filename+linenum)%1024];

	voip_list_for_each(list, entry, nodeptr) {
		if (entry->filename==filename && entry->linenum==linenum)
			return entry;
	}
	return NULL;
}

static int
summary_add(char *filename, int linenum, char *funcname, int size)
{
	struct summary_entry_t *entry;
	struct voip_list_head *list=&summary_list[((unsigned long)filename+linenum)%1024];

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
		voip_list_add(list, &entry->nodeptr);
	}
	return 0;
}

static int
summary_del_all(void)
{
	struct summary_entry_t *entry, *entry_n;
	int i;


	for (i=0; i<1024; i++) {
		struct voip_list_head *list=&summary_list[i];
		voip_list_for_each_safe(list, entry, entry_n, nodeptr) {
			voip_list_del(&entry->nodeptr);
			free(entry);
		}
	}
	return 0;
}

////////////////////////////////////////////////////////////////
int
memdbg_dump()
{
	int i;
	unsigned long long total_size=0;

	if (!is_memdbg_inited) {
		is_memdbg_inited=1;
		memdbg_init();
	}

	if (!is_memdbg_enable) {
		fprintf(stderr, "Please turn on memdbg at first\n");
		return 0;
	}

	for (i=0; i<1024; i++) {
		struct voip_list_head *list=&memdbg_list[i];
		struct memdbg_entry_t *entry;

		pthread_mutex_lock(&memdbg_list_rwlock);
		voip_list_for_each(list, entry, nodeptr) {
			summary_add(entry->filename, entry->linenum, entry->funcname, entry->size);
		}
		pthread_mutex_unlock(&memdbg_list_rwlock);
	}

	for (i=0; i<1024; i++) {
		struct voip_list_head *list=&summary_list[i];
		struct summary_entry_t *entry;
		voip_list_for_each(list, entry, nodeptr) {
			fprintf(stderr, "%4d %s:%d %s() size=%u (%lu)\n", entry->count,
				entry->filename, entry->linenum, entry->funcname, entry->size, entry->total_size);
			total_size+=entry->total_size;
		}
	}
        fprintf(stderr, "total size=%llu\n", total_size);
	summary_del_all();
	return 0;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void *
voip_util_malloc_safe(char *filename, int linenum, char *funcname, size_t size)
{
	void *ptr=malloc(size);

	if (ptr==NULL) {
		LOGE("%s:%d %s(), malloc(%d) return null?", filename, linenum, funcname, size);
		return NULL;
	}
	memset(ptr, 0x00, size);

	if (is_memdbg_enable) {
		memdbg_add(ptr, size, filename, linenum, funcname);
	}
	return ptr;
}

void *
voip_util_strdup_safe(char *filename, int linenum, char *funcname, const char *s)
{
	size_t size;
	void *ptr;

	if (s == NULL) {
		LOGE("%s:%d %s(), strdup() for null string", filename, linenum, funcname);
		return NULL;
	}

	size = strlen (s) + 1;
	ptr = malloc(size);

	if (ptr == NULL) {
		LOGE("%s:%d %s(), malloc(%d) return null?", filename, linenum, funcname, size);
		return NULL;
	}
	memcpy (ptr, s, size);

	if (is_memdbg_enable) {
		memdbg_add(ptr, size, filename, linenum, funcname);
	}
	return ptr;
}

void
voip_util_free_safe(char *filename, int linenum, char *funcname, void *ptr)
{
	if (ptr==NULL) {
		LOGE("%s:%d %s(), free null?", filename, linenum, funcname);
		return;
	}

	if (is_memdbg_enable) {
		memdbg_del(ptr, filename, linenum, funcname);
	}

	free(ptr);
}
