/* Licensed under BSD-MIT - see LICENSE file for details */
#ifndef CCAN_LIST_H
#define CCAN_LIST_H
#include <stdbool.h>
#include <assert.h>
#include <ccan/container_of/voip_container_of.h>
#include <ccan/check_type/voip_check_type.h>

/**
 * struct voip_list_node - an entry in a doubly-linked list
 * @next: next entry (self if empty)
 * @prev: previous entry (self if empty)
 *
 * This is used as an entry in a linked list.
 * Example:
 *	struct child {
 *		const char *name;
 *		// Linked list of all us children.
 *		struct voip_list_node list;
 *	};
 */
struct voip_list_node
{
	struct voip_list_node *next, *prev;
};

/**
 * struct voip_list_head - the head of a doubly-linked list
 * @h: the voip_list_head (containing next and prev pointers)
 *
 * This is used as the head of a linked list.
 * Example:
 *	struct parent {
 *		const char *name;
 *		struct voip_list_head children;
 *		unsigned int num_children;
 *	};
 */
struct voip_list_head
{
	struct voip_list_node n;
};

/**
 * voip_list_check - check head of a list for consistency
 * @h: the voip_list_head
 * @abortstr: the location to print on aborting, or NULL.
 *
 * Because list_nodes have redundant information, consistency checking between
 * the back and forward links can be done.  This is useful as a debugging check.
 * If @abortstr is non-NULL, that will be printed in a diagnostic if the list
 * is inconsistent, and the function will abort.
 *
 * Returns the list head if the list is consistent, NULL if not (it
 * can never return NULL if @abortstr is set).
 *
 * See also: list_check_node()
 *
 * Example:
 *	static void dump_parent(struct parent *p)
 *	{
 *		struct child *c;
 *
 *		printf("%s (%u children):\n", p->name, p->num_children);
 *		voip_list_check(&p->children, "bad child list");
 *		voip_list_for_each(&p->children, c, list)
 *			printf(" -> %s\n", c->name);
 *	}
 */
struct voip_list_head *voip_list_check(const struct voip_list_head *h, const char *abortstr);

/**
 * voip_list_check_node - check node of a list for consistency
 * @n: the voip_list_node
 * @abortstr: the location to print on aborting, or NULL.
 *
 * Check consistency of the list node is in (it must be in one).
 *
 * See also: voip_list_check()
 *
 * Example:
 *	static void dump_child(const struct child *c)
 *	{
 *		voip_list_check_node(&c->list, "bad child list");
 *		printf("%s\n", c->name);
 *	}
 */
struct voip_list_node *voip_list_check_node(const struct voip_list_node *n,
				  const char *abortstr);

#ifdef CCAN_LIST_DEBUG
#define voip_list_debug(h) list_check((h), __func__)
#define voip_list_debug_node(n) list_check_node((n), __func__)
#else
#define voip_list_debug(h) (h)
#define voip_list_debug_node(n) (n)
#endif

/**
 * VOIP_LIST_HEAD_INIT - initializer for an empty list_head
 * @name: the name of the list.
 *
 * Explicit initializer for an empty list.
 *
 * See also:
 *	LIST_HEAD, list_head_init()
 *
 * Example:
 *	static struct voip_list_head my_list = VOIP_LIST_HEAD_INIT(my_list);
 */
#define VOIP_LIST_HEAD_INIT(name) { { &name.n, &name.n } }

/**
 * VOIP_LIST_HEAD - define and initialize an empty voip_list_head
 * @name: the name of the list.
 *
 * The VOIP_LIST_HEAD macro defines a voip_list_head and initializes it to an empty
 * list.  It can be prepended by "static" to define a static voip_list_head.
 *
 * See also:
 *	VOIP_LIST_HEAD_INIT, voip_list_head_init()
 *
 * Example:
 *	static VOIP_LIST_HEAD(my_global_list);
 */
#define VOIP_LIST_HEAD(name) \
	struct voip_list_head name = VOIP_LIST_HEAD_INIT(name)

/**
 * voip_list_head_init - initialize a voip_list_head
 * @h: the voip_list_head to set to the empty list
 *
 * Example:
 *	...
 *	struct parent *parent = malloc(sizeof(*parent));
 *
 *	voip_list_head_init(&parent->children);
 *	parent->num_children = 0;
 */
static inline void voip_list_head_init(struct voip_list_head *h)
{
	h->n.next = h->n.prev = &h->n;
}

/**
 * voip_list_add - add an entry at the start of a linked list.
 * @h: the voip_list_head to add the node to
 * @n: the voip_list_node to add to the list.
 *
 * The voip_list_node does not need to be initialized; it will be overwritten.
 * Example:
 *	struct child *child = malloc(sizeof(*child));
 *
 *	child->name = "marvin";
 *	voip_list_add(&parent->children, &child->list);
 *	parent->num_children++;
 */
static inline void voip_list_add(struct voip_list_head *h, struct voip_list_node *n)
{
	n->next = h->n.next;
	n->prev = &h->n;
	h->n.next->prev = n;
	h->n.next = n;
	(void)voip_list_debug(h);
}

/**
 * voip_list_add_tail - add an entry at the end of a linked list.
 * @h: the voip_list_head to add the node to
 * @n: the voip_list_node to add to the list.
 *
 * The voip_list_node does not need to be initialized; it will be overwritten.
 * Example:
 *	voip_list_add_tail(&parent->children, &child->list);
 *	parent->num_children++;
 */
static inline void voip_list_add_tail(struct voip_list_head *h, struct voip_list_node *n)
{
	n->next = &h->n;
	n->prev = h->n.prev;
	h->n.prev->next = n;
	h->n.prev = n;
	(void)voip_list_debug(h);
}

/**
 * voip_list_empty - is a list empty?
 * @h: the voip_list_head
 *
 * If the list is empty, returns true.
 *
 * Example:
 *	assert(voip_list_empty(&parent->children) == (parent->num_children == 0));
 */
static inline bool voip_list_empty(const struct voip_list_head *h)
{
	(void)voip_list_debug(h);
	return h->n.next == &h->n;
}

/**
 * voip_list_del - delete an entry from an (unknown) linked list.
 * @n: the voip_list_node to delete from the list.
 *
 * Note that this leaves @n in an undefined state; it can be added to
 * another list, but not deleted again.
 *
 * See also:
 *	voip_list_del_from()
 *
 * Example:
 *	voip_list_del(&child->list);
 *	parent->num_children--;
 */
static inline void voip_list_del(struct voip_list_node *n)
{
	(void)voip_list_debug_node(n);
	n->next->prev = n->prev;
	n->prev->next = n->next;
#ifdef CCAN_LIST_DEBUG
	/* Catch use-after-del. */
	n->next = n->prev = NULL;
#endif
}

/**
 * voip_list_del_from - delete an entry from a known linked list.
 * @h: the voip_list_head the node is in.
 * @n: the voip_list_node to delete from the list.
 *
 * This explicitly indicates which list a node is expected to be in,
 * which is better documentation and can catch more bugs.
 *
 * See also: voip_list_del()
 *
 * Example:
 *	voip_list_del_from(&parent->children, &child->list);
 *	parent->num_children--;
 */
static inline void voip_list_del_from(struct voip_list_head *h, struct voip_list_node *n)
{
#ifdef CCAN_LIST_DEBUG
	{
		/* Thorough check: make sure it was in list! */
		struct voip_list_node *i;
		for (i = h->n.next; i != n; i = i->next)
			assert(i != &h->n);
	}
#endif /* CCAN_LIST_DEBUG */

	/* Quick test that catches a surprising number of bugs. */
	assert(!voip_list_empty(h));
	voip_list_del(n);
}

/**
 * 5vt extension
 * voip_list_del_init - del a known linked list and init the linked list.
 * @h: the list_head to delete.
 *
 * note: it is dangerous. Maybe lose head link to other nodes.
 */
static inline void voip_list_del_init(struct voip_list_head *entry)
{
	voip_list_del(&entry->n);
	voip_list_head_init(entry);
}

/**
 * 5vt extension
 * voip_list_move - move an entry to a known linked list.
 * @n: the voip_list_node to move to the list.
 *
 * See also:
 *	voip_list_move_tail()
 *
 * Example:
 *	voip_list_move(&new_parent->children, &child->list);
 *	parent->num_children--;
 *	new_parent->num_children++;
 */
static inline void voip_list_move(struct voip_list_head *h, struct voip_list_node *n)
{
	voip_list_del(n);
	voip_list_add(h, n);
}

static inline void voip_list_move_tail(struct voip_list_head *h, struct voip_list_node *n)
{
	voip_list_del(n);
	voip_list_add_tail(h, n);
}

/**
 * voip_list_entry - convert a list_node back into the structure containing it.
 * @n: the voip_list_node
 * @type: the type of the entry
 * @member: the voip_list_node member of the type
 *
 * Example:
 *	// First list entry is children.next; convert back to child.
 *	child = voip_list_entry(parent->children.n.next, struct child, list);
 *
 * See Also:
 *	voip_list_top(), voip_list_for_each()
 */
#define voip_list_entry(n, type, member) voip_container_of(n, type, member)

/**
 * voip_list_top - get the first entry in a list
 * @h: the voip_list_head
 * @type: the type of the entry
 * @member: the voip_list_node member of the type
 *
 * If the list is empty, returns NULL.
 *
 * Example:
 *	struct child *first;
 *	first = voip_list_top(&parent->children, struct child, list);
 *	if (!first)
 *		printf("Empty list!\n");
 */
#define voip_list_top(h, type, member)					\
	((type *)voip_list_top_((h), voip_list_off_(type, member)))

static inline const void *voip_list_top_(const struct voip_list_head *h, size_t off)
{
	if (voip_list_empty(h))
		return NULL;
	return (const char *)h->n.next - off;
}

/**
 * voip_list_pop - remove the first entry in a list
 * @h: the voip_list_head
 * @type: the type of the entry
 * @member: the voip_list_node member of the type
 *
 * If the list is empty, returns NULL.
 *
 * Example:
 *	struct child *one;
 *	one = voip_list_pop(&parent->children, struct child, list);
 *	if (!one)
 *		printf("Empty list!\n");
 */
#define voip_list_pop(h, type, member)					\
	((type *)voip_list_pop_((h), voip_list_off_(type, member)))

static inline const void *voip_list_pop_(const struct voip_list_head *h, size_t off)
{
	struct voip_list_node *n;

	if (voip_list_empty(h))
		return NULL;
	n = h->n.next;
	voip_list_del(n);
	return (const char *)n - off;
}

/**
 * voip_list_tail - get the last entry in a list
 * @h: the voip_list_head
 * @type: the type of the entry
 * @member: the voip_list_node member of the type
 *
 * If the list is empty, returns NULL.
 *
 * Example:
 *	struct child *last;
 *	last = voip_list_tail(&parent->children, struct child, list);
 *	if (!last)
 *		printf("Empty list!\n");
 */
#define voip_list_tail(h, type, member) \
	((type *)voip_list_tail_((h), voip_list_off_(type, member)))

static inline const void *voip_list_tail_(const struct voip_list_head *h, size_t off)
{
	if (voip_list_empty(h))
		return NULL;
	return (const char *)h->n.prev - off;
}

/**
 * voip_list_for_each - iterate through a list.
 * @h: the voip_list_head (warning: evaluated multiple times!)
 * @i: the structure containing the voip_list_node
 * @member: the voip_list_node member of the structure
 *
 * This is a convenient wrapper to iterate @i over the entire list.  It's
 * a for loop, so you can break and continue as normal.
 *
 * Example:
 *	voip_list_for_each(&parent->children, child, list)
 *		printf("Name: %s\n", child->name);
 */
#define voip_list_for_each(h, i, member)					\
	voip_list_for_each_off(h, i, voip_list_off_var_(i, member))

/**
 * voip_list_for_each_rev - iterate through a list backwards.
 * @h: the voip_list_head
 * @i: the structure containing the voip_list_node
 * @member: the voip_list_node member of the structure
 *
 * This is a convenient wrapper to iterate @i over the entire list.  It's
 * a for loop, so you can break and continue as normal.
 *
 * Example:
 *	voip_list_for_each_rev(&parent->children, child, list)
 *		printf("Name: %s\n", child->name);
 */
#define voip_list_for_each_rev(h, i, member)					\
	for (i = voip_container_of_var(list_debug(h)->n.prev, i, member);	\
	     &i->member != &(h)->n;					\
	     i = voip_container_of_var(i->member.prev, i, member))

/**
 * voip_list_for_each_safe - iterate through a list, maybe during deletion
 * @h: the voip_list_head
 * @i: the structure containing the voip_list_node
 * @nxt: the structure containing the voip_list_node
 * @member: the voip_list_node member of the structure
 *
 * This is a convenient wrapper to iterate @i over the entire list.  It's
 * a for loop, so you can break and continue as normal.  The extra variable
 * @nxt is used to hold the next element, so you can delete @i from the list.
 *
 * Example:
 *	struct child *next;
 *	voip_list_for_each_safe(&parent->children, child, next, list) {
 *		voip_list_del(&child->list);
 *		parent->num_children--;
 *	}
 */
#define voip_list_for_each_safe(h, i, nxt, member)				\
	voip_list_for_each_safe_off(h, i, nxt, voip_list_off_var_(i, member))

/**
 * voip_list_next - get the next entry in a list
 * @h: the voip_list_head
 * @i: a pointer to an entry in the list.
 * @member: the voip_list_node member of the structure
 *
 * If @i was the last entry in the list, returns NULL.
 *
 * Example:
 *	struct child *second;
 *	second = voip_list_next(&parent->children, first, list);
 *	if (!second)
 *		printf("No second child!\n");
 */
#define voip_list_next(h, i, member)						\
	((list_typeof(i))voip_list_entry_or_null(voip_list_debug(h),		\
					    (i)->member.next,		\
					    voip_list_off_var_((i), member)))

/**
 * voip_list_prev - get the previous entry in a list
 * @h: the voip_list_head
 * @i: a pointer to an entry in the list.
 * @member: the voip_list_node member of the structure
 *
 * If @i was the first entry in the list, returns NULL.
 *
 * Example:
 *	first = voip_list_prev(&parent->children, second, list);
 *	if (!first)
 *		printf("Can't go back to first child?!\n");
 */
#define voip_list_prev(h, i, member)						\
	((list_typeof(i))voip_list_entry_or_null(voip_list_debug(h),		\
					    (i)->member.prev,		\
					    voip_list_off_var_((i), member)))

/**
 * voip_list_append_list - empty one list onto the end of another.
 * @to: the list to append into
 * @from: the list to empty.
 *
 * This takes the entire contents of @from and moves it to the end of
 * @to.  After this @from will be empty.
 *
 * Example:
 *	struct voip_list_head adopter;
 *
 *	voip_list_append_list(&adopter, &parent->children);
 *	assert(voip_list_empty(&parent->children));
 *	parent->num_children = 0;
 */
static inline void voip_list_append_list(struct voip_list_head *to,
				    struct voip_list_head *from)
{
	struct voip_list_node *from_tail = voip_list_debug(from)->n.prev;
	struct voip_list_node *to_tail = voip_list_debug(to)->n.prev;

	/* Sew in head and entire list. */
	to->n.prev = from_tail;
	from_tail->next = &to->n;
	to_tail->next = &from->n;
	from->n.prev = to_tail;

	/* Now remove head. */
	voip_list_del(&from->n);
	voip_list_head_init(from);
}

/**
 * voip_list_prepend_list - empty one list into the start of another.
 * @to: the list to prepend into
 * @from: the list to empty.
 *
 * This takes the entire contents of @from and moves it to the start
 * of @to.  After this @from will be empty.
 *
 * Example:
 *	voip_list_prepend_list(&adopter, &parent->children);
 *	assert(voip_list_empty(&parent->children));
 *	parent->num_children = 0;
 */
static inline void voip_list_prepend_list(struct voip_list_head *to,
				     struct voip_list_head *from)
{
	struct voip_list_node *from_tail = voip_list_debug(from)->n.prev;
	struct voip_list_node *to_head = voip_list_debug(to)->n.next;

	/* Sew in head and entire list. */
	to->n.next = &from->n;
	from->n.prev = &to->n;
	to_head->prev = from_tail;
	from_tail->next = to_head;

	/* Now remove head. */
	voip_list_del(&from->n);
	voip_list_head_init(from);
}

/**
 * voip_list_for_each_off - iterate through a list of memory regions.
 * @h: the voip_list_head
 * @i: the pointer to a memory region wich contains list node data.
 * @off: offset(relative to @i) at which list node data resides.
 *
 * This is a low-level wrapper to iterate @i over the entire list, used to
 * implement all oher, more high-level, for-each constructs. It's a for loop,
 * so you can break and continue as normal.
 *
 * WARNING! Being the low-level macro that it is, this wrapper doesn't know
 * nor care about the type of @i. The only assumtion made is that @i points
 * to a chunk of memory that at some @offset, relative to @i, contains a
 * properly filled `struct node_list' which in turn contains pointers to
 * memory chunks and it's turtles all the way down. Whith all that in mind
 * remember that given the wrong pointer/offset couple this macro will
 * happilly churn all you memory untill SEGFAULT stops it, in other words
 * caveat emptor.
 *
 * It is worth mentioning that one of legitimate use-cases for that wrapper
 * is operation on opaque types with known offset for `struct list_node'
 * member(preferably 0), because it allows you not to disclose the type of
 * @i.
 *
 * Example:
 *	voip_list_for_each_off(&parent->children, child,
 *				offsetof(struct child, list))
 *		printf("Name: %s\n", child->name);
 */
#define voip_list_for_each_off(h, i, off)                                    \
  for (i = voip_list_node_to_off_(voip_list_debug(h)->n.next, (off));             \
       voip_list_node_from_off_((void *)i, (off)) != &(h)->n;                \
       i = voip_list_node_to_off_(voip_list_node_from_off_((void *)i, (off))->next, \
                             (off)))

/**
 * voip_list_for_each_safe_off - iterate through a list of memory regions, maybe
 * during deletion
 * @h: the voip_list_head
 * @i: the pointer to a memory region wich contains list node data.
 * @nxt: the structure containing the voip_list_node
 * @off: offset(relative to @i) at which list node data resides.
 *
 * For details see `voip_list_for_each_off' and `voip_list_for_each_safe'
 * descriptions.
 *
 * Example:
 *	voip_list_for_each_safe_off(&parent->children, child,
 *		next, offsetof(struct child, list))
 *		printf("Name: %s\n", child->name);
 */
#define voip_list_for_each_safe_off(h, i, nxt, off)                          \
  for (i = voip_list_node_to_off_(voip_list_debug(h)->n.next, (off)),             \
         nxt = voip_list_node_to_off_(voip_list_node_from_off_(i, (off))->next,   \
                                 (off));                                \
       voip_list_node_from_off_(i, (off)) != &(h)->n;                        \
       i = nxt,                                                         \
         nxt = voip_list_node_to_off_(voip_list_node_from_off_(i, (off))->next,   \
                                 (off)))


/* Other -off variants. */
#define voip_list_entry_off(n, type, off)		\
	((type *)voip_list_node_from_off_((n), (off)))

#define voip_list_head_off(h, type, off)		\
	((type *)voip_list_head_off((h), (off)))

#define voip_list_tail_off(h, type, off)		\
	((type *)voip_list_tail_((h), (off)))

#define voip_list_add_off(h, n, off)                 \
	voip_list_add((h), voip_list_node_from_off_((n), (off)))

#define voip_list_del_off(n, off)                    \
	voip_list_del(voip_list_node_from_off_((n), (off)))

#define voip_list_del_from_off(h, n, off)			\
	voip_list_del_from(h, voip_list_node_from_off_((n), (off)))

/* Offset helper functions so we only single-evaluate. */
static inline void *voip_list_node_to_off_(struct voip_list_node *node, size_t off)
{
	return (void *)((char *)node - off);
}
static inline struct voip_list_node *voip_list_node_from_off_(void *ptr, size_t off)
{
	return (struct voip_list_node *)((char *)ptr + off);
}

/* Get the offset of the member, but make sure it's a list_node. */
#define voip_list_off_(type, member)					\
	(voip_container_off(type, member) +				\
	 voip_check_type(((type *)0)->member, struct voip_list_node))

#define voip_list_off_var_(var, member)			\
	(voip_container_off_var(var, member) +		\
	 voip_check_type(var->member, struct voip_list_node))

#if HAVE_TYPEOF
#define voip_list_typeof(var) typeof(var)
#else
#define voip_list_typeof(var) void *
#endif

/* Returns member, or NULL if at end of list. */
static inline void *voip_list_entry_or_null(struct voip_list_head *h,
				       struct voip_list_node *n,
				       size_t off)
{
	if (n == &h->n)
		return NULL;
	return (char *)n - off;
}
#endif /* CCAN_LIST_H */
