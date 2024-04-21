#ifndef __EH_LIST_H__
#define __EH_LIST_H__

/**
 * container_of - cast a member of a structure out to the containing structure
 *
 * @ptr:	the pointer to the member.
 * @type:	the type of the container struct this is embedded in.
 * @member:	the name of the member within the struct.
 *typeof(type)是gcc的扩展，是得到type的数据类型，和我们比较熟悉的sizeof()比较类似。
 */
#ifndef offsetof
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#endif

#ifndef container_of
#define container_of(ptr, type, member) ({			\
        const typeof( ((type *)0)->member ) *__mptr = (ptr);	\
        (type *)( (char *)__mptr - offsetof(type,member) );})
#endif


struct eh_list_head {
	struct eh_list_head *next, *prev;
};

#define EH_LIST_HEAD_INIT(name) { &(name), &(name) }

#define EH_LIST_HEAD(name) \
struct eh_list_head name = EH_LIST_HEAD_INIT(name)

#define INIT_EH_LIST_HEAD(ptr) do { \
	(ptr)->next = (ptr); (ptr)->prev = (ptr); \
} while (0)

/*
* Insert a new entry between two known consecutive entries.
*
* This is only for internal list manipulation where we know
* the prev/next entries already!
*/
static inline void __eh_list_add(struct eh_list_head *new,
				struct eh_list_head *prev,
				struct eh_list_head *next)
{
	next->prev = new;
	new->next = next;
	new->prev = prev;
	prev->next = new;
}

/**
* eh_list_add – add a new entry
* @new: new entry to be added
* @head: eh_list head to add it after
*
* Insert a new entry after the specified head.
* This is good for implementing stacks.
*/
static inline void eh_list_add(struct eh_list_head *new, struct eh_list_head *head)
{
	__eh_list_add(new, head, head->next);
}

/**
* eh_list_add_tail – add a new entry
* @new: new entry to be added
* @head: eh_list head to add it before
*
* Insert a new entry before the specified head.
* This is useful for implementing queues.
*/
static inline void eh_list_add_tail(struct eh_list_head *new, struct eh_list_head *head)
{
	__eh_list_add(new, head->prev, head);
}

/*
* Delete a eh_list entry by making the prev/next entries
* point to each other.
*
* This is only for internal eh_list manipulation where we know
* the prev/next entries already!
*/
static inline void __eh_list_del(struct eh_list_head *prev, struct eh_list_head *next)
{
	next->prev = prev;
	prev->next = next;
}

/**
* eh_list_del – deletes entry from eh_list.
* @entry: the element to delete from the eh_list.
* Note: eh_list_empty on entry does not return true after this, the entry is in an undefined state.
*/
static inline void eh_list_del(struct eh_list_head *entry)
{
	__eh_list_del(entry->prev, entry->next);
	entry->next = (void *) 0;
	entry->prev = (void *) 0;
}

/**
* eh_list_del_init – deletes entry from eh_list and reinitialize it.
* @entry: the element to delete from the eh_list.
*/
static inline void eh_list_del_init(struct eh_list_head *entry)
{
	__eh_list_del(entry->prev, entry->next);
	INIT_EH_LIST_HEAD(entry);
}

/**
* eh_list_move – delete from one eh_list and add as another’s head
* @eh_list: the entry to move
* @head: the head that will precede our entry
*/
static inline void eh_list_move(struct eh_list_head *eh_list,
				struct eh_list_head *head)
{
	__eh_list_del(eh_list->prev, eh_list->next);
	eh_list_add(eh_list, head);
}

/**
* eh_list_move_tail – delete from one eh_list and add as another’s tail
* @eh_list: the entry to move
* @head: the head that will follow our entry
*/
static inline void eh_list_move_tail(struct eh_list_head *eh_list,
					struct eh_list_head *head)
{
	__eh_list_del(eh_list->prev, eh_list->next);
	eh_list_add_tail(eh_list, head);
}

/**
* eh_list_empty – tests whether a eh_list is empty
* @head: the eh_list to test.
*/
static inline int eh_list_empty(struct eh_list_head *head)
{
	return head->next == head;
}

static inline void __eh_list_splice(struct eh_list_head *eh_list,
					struct eh_list_head *head)
{
	struct eh_list_head *first = eh_list->next;
	struct eh_list_head *last = eh_list->prev;
	struct eh_list_head *at = head->next;

	first->prev = head;
	head->next = first;

	last->next = at;
	at->prev = last;
}

/**
* eh_list_splice – join two eh_lists
* @eh_list: the new eh_list to add.
* @head: the place to add it in the first eh_list.
*/
static inline void eh_list_splice(struct eh_list_head *eh_list, struct eh_list_head *head)
{
if (!eh_list_empty(eh_list))
__eh_list_splice(eh_list, head);
}

/**
* eh_list_splice_init – join two eh_lists and reinitialise the emptied eh_list.
* @eh_list: the new eh_list to add.
* @head: the place to add it in the first eh_list.
*
* The eh_list at @eh_list is reinitialised
*/
static inline void eh_list_splice_init(struct eh_list_head *eh_list,
struct eh_list_head *head)
{
if (!eh_list_empty(eh_list)) {
__eh_list_splice(eh_list, head);
INIT_EH_LIST_HEAD(eh_list);
}
}

/**
* eh_list_entry – get the struct for this entry
* @ptr:    the &struct eh_list_head pointer.
* @type:    the type of the struct this is embedded in.
* @member:    the name of the eh_list_struct within the struct.
*/
#define eh_list_entry(ptr, type, member) \
((type *)((char *)(ptr)-(unsigned long)(&((type *)0)->member)))

/**
* eh_list_for_each    -    iterate over a eh_list
* @pos:    the &struct eh_list_head to use as a loop counter.
* @head:    the head for your eh_list.
*/
#define eh_list_for_each(pos, head) \
for (pos = (head)->next; pos != (head); \
pos = pos->next)



/**
* eh_list_for_each_prev    -    iterate over a eh_list backwards
* @pos:    the &struct eh_list_head to use as a loop counter.
* @head:    the head for your eh_list.
*/
#define eh_list_for_each_prev(pos, head) \
for (pos = (head)->prev; pos != (head); \
pos = pos->prev)

/**
* eh_list_for_each_safe    -    iterate over a eh_list safe against removal of eh_list entry
* @pos:    the &struct eh_list_head to use as a loop counter.
* @n:        another &struct eh_list_head to use as temporary storage
* @head:    the head for your eh_list.
*/
#define eh_list_for_each_safe(pos, n, head) \
for (pos = (head)->next, n = pos->next; pos != (head); \
pos = n, n = pos->next)

/**
* eh_list_for_each_prev_safe    -    iterate over a eh_list safe against removal of eh_list entry
* @pos:    the &struct eh_list_head to use as a loop counter.
* @n:        another &struct eh_list_head to use as temporary storage
* @head:    the head for your eh_list.
*/
#define eh_list_for_each_prev_safe(pos, n, head) \
for (pos = (head)->prev, n = pos->prev; pos != (head); \
pos = n, n = pos->prev)

/**
* eh_list_for_each_entry    -    iterate over eh_list of given type
* @pos:    the type * to use as a loop counter.
* @head:    the head for your eh_list.
* @member:    the name of the eh_list_struct within the struct.
*/
#define eh_list_for_each_entry(pos, head, member)                \
for (pos = eh_list_entry((head)->next, typeof(*pos), member);    \
&pos->member != (head);                     \
pos = eh_list_entry(pos->member.next, typeof(*pos), member))

/**
* eh_list_for_each_prev_entry    -    iterate over eh_list of given type
* @pos:    the type * to use as a loop counter.
* @head:    the head for your eh_list.
* @member:    the name of the eh_list_struct within the struct.
*/
#define eh_list_for_each_prev_entry(pos, head, member)                \
for (pos = eh_list_entry((head)->prev, typeof(*pos), member);    \
&pos->member != (head);                     \
pos = eh_list_entry(pos->member.prev, typeof(*pos), member))

/**
 * eh_list_for_each_entry_continue - continue iteration over list of given type
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your list.
 * @member:	the name of the list_head within the struct.
 *
 * Continue to iterate over list of given type, continuing after
 * the current position.
 */
#define eh_list_for_each_entry_continue(pos, head, member) 							\
	for (pos = eh_list_entry((pos)->member.next, typeof(*(pos)), member);			\
	     &pos->member != (head);													\
	     pos = eh_list_entry((pos)->member.next, typeof(*(pos)), member))

/**
 * eh_list_for_each_prev_entry_continue - continue iteration over list of given type
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your list.
 * @member:	the name of the list_head within the struct.
 *
 * Continue to iterate over list of given type, continuing after
 * the current position.
 */
#define eh_list_for_each_prev_entry_continue(pos, head, member) 							\
	for (pos = eh_list_entry((pos)->member.prev, typeof(*(pos)), member);					\
	     &pos->member != (head);															\
	     pos = eh_list_entry((pos)->member.prev, typeof(*(pos)), member))

/**
* eh_list_for_each_entry_safe – iterate over eh_list of given type safe against removal of eh_list entry
* @pos:    the type * to use as a loop counter.
* @n:        another type * to use as temporary storage
* @head:    the head for your eh_list.
* @member:    the name of the eh_list_struct within the struct.
*/
#define eh_list_for_each_entry_safe(pos, n, head, member)            \
for (pos = eh_list_entry((head)->next, typeof(*pos), member),    \
n = eh_list_entry(pos->member.next, typeof(*pos), member);    \
&pos->member != (head);                     \
pos = n, n = eh_list_entry(n->member.next, typeof(*n), member))

/**
* eh_list_for_each_prev_entry_safe – iterate over eh_list of given type safe against removal of eh_list entry
* @pos:    the type * to use as a loop counter.
* @n:        another type * to use as temporary storage
* @head:    the head for your eh_list.
* @member:    the name of the eh_list_struct within the struct.
*/
#define eh_list_for_each_prev_entry_safe(pos, n, head, member)            \
for (pos = eh_list_entry((head)->prev, typeof(*pos), member),    \
n = eh_list_entry(pos->member.prev, typeof(*pos), member);    \
&pos->member != (head);                     \
pos = n, n = eh_list_entry(n->member.prev, typeof(*n), member))

#endif
