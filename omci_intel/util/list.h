#ifndef _LINUX_LIST_H
#define _LINUX_LIST_H

#include <unistd.h>
#include <sys/types.h>
#include <signal.h>

#undef offsetof
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)

// rename container_of() as list_container_of for the redefined error.
#define list_container_of(ptr, type, member) ( { \
        const typeof( ((type *)0)->member ) *__mptr = (ptr); \
        (type *)( (char *)__mptr - offsetof(type,member) ); } )

static inline void
prefetch(const void *x)
{;
}
static inline void
prefetchw(const void *x)
{;
}

#define LIST_DBGMARK		0x27888118
#define LIST_DBGMARK2		0x27887366
#define LIST_POISON1  ((void *) 0x00100100)
#define LIST_POISON2  ((void *) 0x00200200)

#ifdef LIST_DBGMARK

#include <stdio.h>
#include <strings.h>
#include <stdarg.h>
static inline char *list_purename(char *path) 
{ 
	char *delim=rindex(path, '/'); 
	return delim?(delim+1):path; 
}
static inline int list_printf(char *fmt, ...)
{
	FILE *fp; va_list ap; int ret;

	va_start(ap, fmt); 

	if ((fp = fopen("/dev/console", "w")) != NULL) {
                ret = vfprintf(fp, fmt, ap); 
                fclose(fp);
        } else {
                ret = vfprintf(stderr, fmt, ap); 
        }

        // log msg to file for further debugging
	if ((fp = fopen("/storage/omci_list_debug.log", "a")) ||
	    (fp = fopen("/var/log/omci_list_debug.log", "a")) ||
	    (fp = fopen("/tmp/omci_list_debug.log", "a"))) {
                vfprintf(fp, fmt, ap); 
                fclose(fp);
        }        
        
	va_end(ap);
	return ret;
}

struct list_head {
	unsigned int dbgmark;
	unsigned char *funcname;
	unsigned int line;
	struct list_head *next, *prev;
};
#define LIST_HEAD_INIT(name) { LIST_DBGMARK, NULL, 0, &(name), &(name) }
#define INIT_LIST_HEAD(ptr) do { \
        (ptr)->dbgmark = LIST_DBGMARK; (ptr)->funcname=NULL; (ptr)->line=0; (ptr)->next = (ptr); (ptr)->prev = (ptr); \
} while (0)

#else

struct list_head {
	struct list_head *next, *prev;
};
#define LIST_HEAD_INIT(name) { &(name), &(name) }
#define INIT_LIST_HEAD(ptr) do { \
        (ptr)->next = (ptr); (ptr)->prev = (ptr); \
} while (0)

#endif

#define LIST_HEAD(name) \
        struct list_head name = LIST_HEAD_INIT(name)

#define INIT_LIST_NODE(ptr) do { \
        (ptr)->next = NULL; (ptr)->prev = NULL; \
} while (0)

/*
* Insert a new entry between two known consecutive entries.
*
* This is only for internal list manipulation where we know
* the prev/next entries already!
*/
static inline void
__list_add(struct list_head *new, struct list_head *prev, struct list_head *next)
{
	new->next = next;
	new->prev = prev;
#ifdef LIST_DBGMARK
	new->dbgmark = LIST_DBGMARK;
#endif
	next->prev = new;
	prev->next = new;
}

/**
* list_add - add a new entry
* @new: new entry to be added
* @head: list head to add it after
*
* Insert a new entry after the specified head.
* This is good for implementing stacks.
*/
static inline void
list_add_orig(struct list_head *new, struct list_head *head)
{
	__list_add(new, head, head->next);
}
#ifdef LIST_DBGMARK
#define list_add(new, head)	do { \
        if ((head)->dbgmark != LIST_DBGMARK) { \
                list_printf("%s:%d: %s(): list_add head %p MARK_MISMATCH!\n", list_purename(__FILE__), __LINE__, __FUNCTION__, (head)); \
                kill(getpid(), SIGABRT); \
        } \
	list_add_orig(new, head); \
	(new)->funcname=(unsigned char*)__FUNCTION__; (new)->line=__LINE__; } while (0);
#else
#define list_add(new, head)	list_add_orig(new, head)
#endif

/**
* list_add_tail - add a new entry
* @new: new entry to be added
* @head: list head to add it before
*
* Insert a new entry before the specified head.
* This is useful for implementing queues.
*/
static inline void
list_add_tail_orig(struct list_head *new, struct list_head *head)
{
	__list_add(new, head->prev, head);
}
#ifdef LIST_DBGMARK
#define list_add_tail(new, head)	do { \
        if ((head)->dbgmark != LIST_DBGMARK) { \
                list_printf("%s:%d: %s(): list_add_tail head %p MARK_MISMATCH!\n", list_purename(__FILE__), __LINE__, __FUNCTION__, (head)); \
                kill(getpid(), SIGABRT); \
        } \
	list_add_tail_orig(new, head); \
	(new)->funcname=(unsigned char*)__FUNCTION__; (new)->line=__LINE__; } while (0);
#else
#define list_add_tail(new, head)	list_add_tail_orig(new, head)
#endif


/**
 * list_replace - replace old entry by new one
 * @old : the element to be replaced
 * @new : the new element to insert
 *
 * If @old was empty, it will be overwritten.
 */
static inline void list_replace(struct list_head *old,
				struct list_head *new)
{
	new->next = old->next;
	new->next->prev = new;
	new->prev = old->prev;
	new->prev->next = new;
}

static inline void list_replace_init(struct list_head *old,
					struct list_head *new)
{
	list_replace(old, new);
	INIT_LIST_HEAD(old);
}
static inline void
__list_del(struct list_head *prev, struct list_head *next)
{
	next->prev = prev;
	prev->next = next;
}

static inline void
list_del_orig(struct list_head *entry)
{
	__list_del(entry->prev, entry->next);
#ifdef LIST_DBGMARK
	entry->dbgmark = LIST_DBGMARK2;
#endif
	entry->next = LIST_POISON1;
	entry->prev = LIST_POISON2;
}

#ifdef LIST_DBGMARK
#define list_del(x)	do { \
        if ((x)->dbgmark != LIST_DBGMARK) { \
                list_printf("%s:%d: %s(): list_del %p MARK_MISMATCH!\n", list_purename(__FILE__), __LINE__, __FUNCTION__, (x)); \
                kill(getpid(), SIGABRT); \
        } \
        list_del_orig(x); \
        (x)->funcname=(unsigned char*)__FUNCTION__; (x)->line=__LINE__; } while (0);
#else
#define list_del(x)	list_del_orig(x)
#endif

static inline void
list_del_init(struct list_head *entry)
{
	__list_del(entry->prev, entry->next);
	INIT_LIST_HEAD(entry);
}

static inline void
list_move(struct list_head *list, struct list_head *head)
{
	__list_del(list->prev, list->next);
	list_add(list, head);
}

static inline void
list_move_tail(struct list_head *list, struct list_head *head)
{
	__list_del(list->prev, list->next);
	list_add_tail(list, head);
}

static inline int 
list_is_last(const struct list_head *list, const struct list_head *head)
{
        return list->next == head;
}



static inline int
list_empty(const struct list_head *head)
{
	return head->next == head;
}

static inline int
list_empty_careful(const struct list_head *head)
{
	struct list_head *next = head->next;
	return (next == head) && (next == head->prev);
}

static inline void
__list_splice(struct list_head *list, struct list_head *head)
{
	struct list_head *first = list->next;
	struct list_head *last = list->prev;
	struct list_head *at = head->next;

	first->prev = head;
	head->next = first;

	last->next = at;
	at->prev = last;
}

/**
* list_splice - join two lists
* @list: the new list to add.
* @head: the place to add it in the first list.
*/
static inline void
list_splice(struct list_head *list, struct list_head *head)
{
	if (!list_empty(list))
		__list_splice(list, head);
}

/**
* list_splice_init - join two lists and reinitialise the emptied list.
* @list: the new list to add.
* @head: the place to add it in the first list.
*
* The list at @list is reinitialised
*/
static inline void
list_splice_init(struct list_head *list, struct list_head *head)
{
	if (!list_empty(list)) {
		__list_splice(list, head);
		INIT_LIST_HEAD(list);
	}
}

static inline int
list_head_is_poison(struct list_head *head)
{
	return (head->next == LIST_POISON1 || head->prev == LIST_POISON2);
}

static inline int
list_node_is_poison_or_null(struct list_head *node)
{
	return (node->next == LIST_POISON1 || node->prev == LIST_POISON2 || node->next == NULL || node->prev == NULL);
}


#define list_entry(ptr, type, member) list_container_of(ptr, type, member)
/*
 * list_first_entry - get the first element from a list
 * @ptr:        the list head to take the element from.
 * @type:       the type of the struct this is embedded in.
 * @member:     the name of the list_struct within the struct.
 *
 * Note, that list is expected to be not empty.
 */
#define list_first_entry(ptr, type, member) \
	list_entry((ptr)->next, type, member)


#define list_for_each(pos, head) \
        for (pos = (head)->next; prefetch(pos->next), list_head_is_poison(pos)==0 && pos != (head); \
                pos = pos->next)

#define __list_for_each(pos, head) \
        for (pos = (head)->next; list_head_is_poison(pos)==0 && pos != (head); pos = pos->next)

#define list_for_each_prev(pos, head) \
        for (pos = (head)->prev; prefetch(pos->prev), list_head_is_poison(pos)==0 && pos != (head); \
                pos = pos->prev)

#define list_for_each_safe(pos, n, head) \
        for (pos = (head)->next, n = pos->next; list_head_is_poison(pos)==0 && pos != (head); \
                pos = n, n = pos->next)

#define list_for_each_entry(pos, head, member)                                \
        for (pos = list_entry((head)->next, typeof(*pos), member);        \
             prefetch(pos->member.next), list_head_is_poison(&pos->member)==0 && &pos->member != (head);         \
             pos = list_entry(pos->member.next, typeof(*pos), member))

#define list_for_each_entry_reverse(pos, head, member)                        \
        for (pos = list_entry((head)->prev, typeof(*pos), member);        \
             prefetch(pos->member.prev), list_head_is_poison(&pos->member)==0 && &pos->member != (head);         \
             pos = list_entry(pos->member.prev, typeof(*pos), member))

#define list_prepare_entry(pos, head, member) \
        ((pos) ? : list_entry(head, typeof(*pos), member))

#define list_for_each_entry_continue(pos, head, member)                 \
        for (pos = list_entry(pos->member.next, typeof(*pos), member);        \
             prefetch(pos->member.next), list_head_is_poison(&pos->member)==0 && &pos->member != (head);        \
             pos = list_entry(pos->member.next, typeof(*pos), member))

#define list_for_each_entry_safe(pos, n, head, member)                        \
        for (pos = list_entry((head)->next, typeof(*pos), member),        \
                n = list_entry(pos->member.next, typeof(*pos), member);        \
             list_head_is_poison(&pos->member)==0 && &pos->member != (head);                                         \
             pos = n, n = list_entry(n->member.next, typeof(*n), member))

//HASH LIST
struct hlist_head {
	struct hlist_node *first;
};

struct hlist_node {
	struct hlist_node *next, **pprev;
};

#define HLIST_HEAD_INIT { .first = NULL }
#define HLIST_HEAD(name) struct hlist_head name = {  .first = NULL }
#define INIT_HLIST_HEAD(ptr) ((ptr)->first = NULL)
#define INIT_HLIST_NODE(ptr) ((ptr)->next = NULL, (ptr)->pprev = NULL)

static inline int
hlist_unhashed(const struct hlist_node *h)
{
	return !h->pprev;
}

static inline int
hlist_empty(const struct hlist_head *h)
{
	return !h->first;
}

static inline void
__hlist_del(struct hlist_node *n)
{
	struct hlist_node *next = n->next;
	struct hlist_node **pprev = n->pprev;
	*pprev = next;
	if (next)
		next->pprev = pprev;
}

static inline void
hlist_del(struct hlist_node *n)
{
	__hlist_del(n);
	n->next = LIST_POISON1;
	n->pprev = LIST_POISON2;
}

static inline void
hlist_del_init(struct hlist_node *n)
{
	if (n->pprev) {
		__hlist_del(n);
		INIT_HLIST_NODE(n);
	}
}

static inline void
hlist_add_head(struct hlist_node *n, struct hlist_head *h)
{
	struct hlist_node *first = h->first;
	n->next = first;
	if (first)
		first->pprev = &n->next;
	h->first = n;
	n->pprev = &h->first;
}

/* next must be != NULL */
static inline void
hlist_add_before(struct hlist_node *n, struct hlist_node *next)
{
	n->pprev = next->pprev;
	n->next = next;
	next->pprev = &n->next;
	*(n->pprev) = n;
}

static inline void
hlist_add_after(struct hlist_node *n, struct hlist_node *next)
{
	next->next = n->next;
	n->next = next;
	next->pprev = &n->next;

	if (next->next)
		next->next->pprev = &next->next;
}

#define hlist_entry(ptr, type, member) list_container_of(ptr,type,member)

#define hlist_for_each(pos, head) \
        for (pos = (head)->first; pos && ({ prefetch(pos->next); 1; }); \
             pos = pos->next)

#define hlist_for_each_safe(pos, n, head) \
        for (pos = (head)->first; pos && ({ n = pos->next; 1; }); \
             pos = n)

#define hlist_for_each_entry(tpos, pos, head, member)                         \
        for (pos = (head)->first;                                         \
             pos && ({ prefetch(pos->next); 1;}) &&                         \
                ({ tpos = hlist_entry(pos, typeof(*tpos), member); 1;}); \
             pos = pos->next)

#define hlist_for_each_entry_continue(tpos, pos, member)                 \
        for (pos = (pos)->next;                                                 \
             pos && ({ prefetch(pos->next); 1;}) &&                         \
                ({ tpos = hlist_entry(pos, typeof(*tpos), member); 1;}); \
             pos = pos->next)

#define hlist_for_each_entry_from(tpos, pos, member)                         \
        for (; pos && ({ prefetch(pos->next); 1;}) &&                         \
                ({ tpos = hlist_entry(pos, typeof(*tpos), member); 1;}); \
             pos = pos->next)

#define hlist_for_each_entry_safe(tpos, pos, n, head, member)                  \
        for (pos = (head)->first;                                         \
             pos && ({ n = pos->next; 1; }) &&                                  \
                ({ tpos = hlist_entry(pos, typeof(*tpos), member); 1;}); \
             pos = n)

#endif
