#ifndef __EH_LIST_H__
#define __EH_LIST_H__

#include "eh_types.h"

struct eh_list_head {
	struct eh_list_head *next, *prev;
};


/**
 * @brief 定义时使用,初始化一个双向链表头
 *        Defined when used to initialize a bidirectional list header
 * @param name 双向链表头变量名
 *        Bidirectional list header variable name
 */
#define EH_LIST_HEAD_INIT(name) { &(name), &(name) }


/**
 * @brief 定义并初始化一个双向链表头
 *        Define and initialize a double linked list head
 * @param name 双向链表头变量名
 *        Double linked list head variable name
 */
#define EH_LIST_HEAD(name) \
	struct eh_list_head name = EH_LIST_HEAD_INIT(name)

/**
 * @brief 初始化给定的双向链表头
 *        Initialize the given double linked list head
 * @param ptr 指向双向链表头的指针
 *        Pointer to the double linked list head
 */
#define eh_list_head_init(ptr) do { \
	(ptr)->next = (ptr); (ptr)->prev = (ptr); \
} while (0)

/**
 * @brief 在两个已知相邻节点之间插入新节点
 *        Insert a new node between two known adjacent nodes
 * @param new 新节点
 *        New node
 * @param prev 已知前驱节点
 *        Known previous node
 * @param next 已知后继节点
 *        Known next node
 * Internal use only.
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
 * @brief 将新节点添加到链表头部之后
 *        Add a new node after the list head
 * @param new 新节点
 *        New node
 * @param head 链表头
 *        List head
 */
static inline void eh_list_add(struct eh_list_head *new, struct eh_list_head *head)
{
	__eh_list_add(new, head, head->next);
}

/**
 * @brief 将新节点添加到链表头部之前
 *        Add a new node before the list head
 * @param new 新节点
 *        New node
 * @param head 链表头
 *        List head
 */
static inline void eh_list_add_tail(struct eh_list_head *new, struct eh_list_head *head)
{
	__eh_list_add(new, head->prev, head);
}

/**
 * @brief 从双向链表中删除节点
 *        Remove a node from the double linked list
 * @param prev 节点的前驱节点指针
 *        Pointer to the previous node
 * @param next 节点的后继节点指针
 *        Pointer to the next node
 */
static inline void __eh_list_del(struct eh_list_head *prev, struct eh_list_head *next)
{
	next->prev = prev;
	prev->next = next;
}

/**
 * @brief 从双向链表中删除指定节点
 *        Remove the specified node from the double linked list
 * @param entry 待删除的节点
 *        Node to be removed
 */
static inline void eh_list_del(struct eh_list_head *entry)
{
	__eh_list_del(entry->prev, entry->next);
	entry->next = (void *) 0;
	entry->prev = (void *) 0;
}

/**
 * @brief 删除节点并重新初始化
 *        Remove the node and reinitialize it
 * @param entry 待删除并初始化的节点
 *        Node to be removed and initialized
 */
static inline void eh_list_del_init(struct eh_list_head *entry)
{
	__eh_list_del(entry->prev, entry->next);
	eh_list_head_init(entry);
}

/**
 * @brief 将节点从一个链表移动到另一个链表的头部
 *        Move a node from one list to the head of another list
 * @param eh_list 要移动的节点
 *        Node to be moved
 * @param head 目标链表的头部
 *        Head of the target list
 */
static inline void eh_list_move(struct eh_list_head *eh_list,
				struct eh_list_head *head)
{
	__eh_list_del(eh_list->prev, eh_list->next);
	eh_list_add(eh_list, head);
}

/**
 * @brief 将节点从一个链表移动到另一个链表的尾部
 *        Move a node from one list to the tail of another list
 * @param eh_list 要移动的节点
 *        Node to be moved
 * @param head 目标链表的头部
 *        Head of the target list
 */
static inline void eh_list_move_tail(struct eh_list_head *eh_list,
					struct eh_list_head *head)
{
	__eh_list_del(eh_list->prev, eh_list->next);
	eh_list_add_tail(eh_list, head);
}

/**
 * @brief 判断双向链表是否为空
 *        Check if the double linked list is empty
 * @param head 双向链表头
 *        Double linked list head
 * @retval true 如果链表为空
 *         true if the list is empty
 * @retval false 如果链表非空
 *         false if the list is not empty
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
 * @brief 合并两个双向链表
 *        Merge two double linked lists
 * @param eh_list 要添加的新链表
 *        New list to be added
 * @param head 目标链表的头部
 *        Head of the target list
 */
static inline void eh_list_splice(struct eh_list_head *eh_list, struct eh_list_head *head)
{
	if (!eh_list_empty(eh_list))
	__eh_list_splice(eh_list, head);
}

/**
 * @brief 合并两个双向链表并重置源链表
 *        Merge two double linked lists and reset the source list
 * @param eh_list 要添加并重置的新链表
 *        New list to be added and reset
 * @param head 目标链表的头部
 *        Head of the target list
 */
static inline void eh_list_splice_init(struct eh_list_head *eh_list,
struct eh_list_head *head)
{
	if (!eh_list_empty(eh_list)) {
		__eh_list_splice(eh_list, head);
		eh_list_head_init(eh_list);
	}
}

/**
 * @brief 从双向链表节点获取容器结构体指针
 *        Get the container structure pointer from the double linked list node
 * @param ptr 指向list_head的指针
 *        Pointer to list_head
 * @param type 容器结构体类型
 *        Container structure type
 * @param member 容器内list_head成员名称
 *        Member name of list_head in the container
 */
#define eh_list_entry(ptr, type, member) \
	((type *)((char *)(ptr)-(unsigned long)(&((type *)0)->member)))

/**
 * @brief 遍历双向链表
 *        Iterate over the double linked list
 * @param pos 用于循环计数的list_head指针
 *        list_head pointer used for loop counting
 * @param head 双向链表头
 *        Double linked list head
 */
#define eh_list_for_each(pos, head) \
	for (pos = (head)->next; pos != (head); pos = pos->next)



/**
 * @brief 反向遍历双向链表
 *        Iterate over the double linked list in reverse
 * @param pos 用于循环计数的list_head指针
 *        list_head pointer used for loop counting
 * @param head 双向链表头
 *        Double linked list head
 */
#define eh_list_for_each_prev(pos, head) \
	for (pos = (head)->prev; pos != (head); pos = pos->prev)

/**
 * @brief 安全地遍历双向链表，防止遍历过程中删除元素导致的问题
 *        Safely iterate over the double linked list to prevent issues caused by deleting elements during iteration
 * @param pos 用于循环计数的list_head指针
 *        list_head pointer used for loop counting
 * @param n 辅助指针，用于存储下一个元素
 *        Auxiliary pointer used to store the next element
 * @param head 双向链表头
 *        Double linked list head
 */
#define eh_list_for_each_safe(pos, n, head) \
	for (pos = (head)->next, n = pos->next; pos != (head); pos = n, n = pos->next)

/**
 * @brief 安全地反向遍历双向链表，防止遍历过程中删除元素导致的问题
 *        Safely iterate over the double linked list in reverse to prevent issues caused by deleting elements during iteration
 * @param pos 用于循环计数的list_head指针
 *        list_head pointer used for loop counting
 * @param n 辅助指针，用于存储上一个元素
 *        Auxiliary pointer used to store the previous element
 * @param head 双向链表头
 *        Double linked list head
 */
#define eh_list_for_each_prev_safe(pos, n, head) \
	for (pos = (head)->prev, n = pos->prev; pos != (head); pos = n, n = pos->prev)

/**
 * @brief 遍历特定类型元素构成的双向链表
 *        Iterate over the double linked list composed of specific type elements
 * @param pos 类型指针，作为循环变量
 *        Type pointer used as a loop variable
 * @param head 双向链表头
 *        Double linked list head
 * @param member 结构体中list_head成员的名称
 *        Member name of list_head in the structure
 */
#define eh_list_for_each_entry(pos, head, member) \
	for (pos = eh_list_entry((head)->next, typeof(*pos), member); \
		&pos->member != (head); \
		pos = eh_list_entry(pos->member.next, typeof(*pos), member))

/**
 * @brief 反向遍历特定类型元素构成的双向链表
 *        Iterate over the double linked list composed of specific type elements in reverse
 * @param pos 类型指针，作为循环变量
 *        Type pointer used as a loop variable
 * @param head 双向链表头
 *        Double linked list head
 * @param member 结构体中list_head成员的名称
 *        Member name of list_head in the structure
 */
#define eh_list_for_each_prev_entry(pos, head, member) \
	for (pos = eh_list_entry((head)->prev, typeof(*pos), member); \
		&pos->member != (head); \
		pos = eh_list_entry(pos->member.prev, typeof(*pos), member))

/**
 * @brief 继续遍历特定类型元素构成的双向链表，从当前位置开始
 *        Continue iterating over the double linked list composed of specific type elements from the current position
 * @param pos 当前迭代位置的类型指针
 *        Type pointer of the current iteration position
 * @param head 双向链表头
 *        Double linked list head
 * @param member 结构体中list_head成员的名称
 *        Member name of list_head in the structure
 */
#define eh_list_for_each_entry_continue(pos, head, member) \
	for (pos = eh_list_entry((pos)->member.next, typeof(*(pos)), member); \
	     &pos->member != (head); \
	     pos = eh_list_entry((pos)->member.next, typeof(*(pos)), member))

/**
 * @brief 继续反向遍历特定类型元素构成的双向链表，从当前位置开始
 *        Continue iterating over the double linked list composed of specific type elements in reverse from the current position
 * @param pos 当前迭代位置的类型指针
 *        Type pointer of the current iteration position
 * @param head 双向链表头
 *        Double linked list head
 * @param member 结构体中list_head成员的名称
 *        Member name of list_head in the structure
 */
#define eh_list_for_each_prev_entry_continue(pos, head, member) \
	for (pos = eh_list_entry((pos)->member.prev, typeof(*(pos)), member); \
	     &pos->member != (head); \
	     pos = eh_list_entry((pos)->member.prev, typeof(*(pos)), member))

/**
 * @brief 安全地遍历特定类型元素构成的双向链表，防止遍历中删除元素
 *        Safely iterate over the double linked list composed of specific type elements to prevent issues caused by deleting elements during iteration
 * @param pos 类型指针，作为循环变量
 *        Type pointer used as a loop variable
 * @param n 辅助指针，用于存储下一个元素
 *        Auxiliary pointer used to store the next element
 * @param head 双向链表头
 *        Double linked list head
 * @param member 结构体中list_head成员的名称
 *        Member name of list_head in the structure
 */
#define eh_list_for_each_entry_safe(pos, n, head, member) \
	for (pos = eh_list_entry((head)->next, typeof(*pos), member), \
		n = eh_list_entry(pos->member.next, typeof(*pos), member); \
		&pos->member != (head); \
		pos = n, n = eh_list_entry(n->member.next, typeof(*n), member))

/**
 * @brief 安全地反向遍历特定类型元素构成的双向链表，防止遍历中删除元素
 *        Safely iterate over the double linked list composed of specific type elements in reverse to prevent issues caused by deleting elements during iteration
 * @param pos 类型指针，作为循环变量
 *        Type pointer used as a loop variable
 * @param n 辅助指针，用于存储上一个元素
 *        Auxiliary pointer used to store the previous element
 * @param head 双向链表头
 *        Double linked list head
 * @param member 结构体中list_head成员的名称
 *        Member name of list_head in the structure
 */
#define eh_list_for_each_prev_entry_safe(pos, n, head, member) \
	for (pos = eh_list_entry((head)->prev, typeof(*pos), member), \
		n = eh_list_entry(pos->member.prev, typeof(*pos), member); \
		&pos->member != (head); \
		pos = n, n = eh_list_entry(n->member.prev, typeof(*n), member))

#endif
