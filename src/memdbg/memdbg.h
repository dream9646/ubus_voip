#ifndef __MEMDBG_H__
#define __MEMDBG_H__
#include <stddef.h>

#define malloc_safe(size)		voip_util_malloc_safe(__FILE__, __LINE__, (char*)__FUNCTION__, size)
#define calloc_safe(num, __size)	voip_util_malloc_safe(__FILE__, __LINE__, (char*)__FUNCTION__, num*__size)
#define strdup_safe(s)			voip_util_strdup_safe(__FILE__, __LINE__, (char*)__FUNCTION__, s)
#define free_safe(addr)			voip_util_free_safe(__FILE__, __LINE__, (char*)__FUNCTION__, addr)

/* memdbg.c */
void memdbg_enable(void);
void memdbg_disable(void);
int memdbg_dump(void);

void *voip_util_malloc_safe(char *filename, int linenum, char *funcname, size_t size);
void *voip_util_strdup_safe(char *filename, int linenum, char *funcname, const char *s);
void voip_util_free_safe(char *filename, int linenum, char *funcname, void *ptr);
#endif
