#ifndef _EH_LLIST_H_
#define _EH_LLIST_H_

#include <stddef.h>
#include <stdbool.h>

#include <eh_types.h>
#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */


struct eh_llist_node{
    struct eh_llist_node *next;
};

struct eh_llist_head{
    struct eh_llist_node *first; /* 必须位于结构体第一位 */
    struct eh_llist_node *last;
};

eh_static_assert(eh_offsetof(struct eh_llist_head, first) == 0, "eh_llist_head must be the first member of struct");

#define EH_LLIST_HEAD_INIT(name)	{ NULL, NULL }
#define EH_LLIST_HEAD(name)	struct eh_llist_head name = EH_LLIST_HEAD_INIT(name)

/**
 * @brief                   初始化单链表头
 * @param  list             
 */
static inline void eh_llist_head_init(struct eh_llist_head *list)
{
	list->first = NULL;
    list->last = NULL;
}

/**
 * @brief                   由旧的链表头来初始化新的链表头，旧的链表头将被清空
 * @param  old_list         
 * @param  new_list         
 */
static inline void eh_llist_head_move_init(struct eh_llist_head *old_list, struct eh_llist_head *new_list){
    new_list->first = old_list->first;
    new_list->last = old_list->last;
    old_list->first = NULL;
    old_list->last = NULL;
}

/**
 * @brief                   初始化链表节点
 * @param  node             
 */
static inline void eh_llist_node_init(struct eh_llist_node *node)
{
	node->next = node;
}

/**
 * @brief                   判断链表节点是否在链表中
 * @param  node             My Param doc
 * @return true 
 * @return false 
 */
static inline bool eh_llist_on_list(const struct eh_llist_node *node)
{
	return node->next != node;
}

/**
 * @brief                   获取链表节点
 */
#define eh_llist_entry(ptr, type, member)		    \
	eh_container_of(ptr, type, member)

/**
 * @brief                   获取链表节点(安全)
 */
#define eh_llist_entry_safe(ptr, type, member)		\
	eh_container_of_safe(ptr, type, member)

/**
 * @brief                   遍历链表指针节点
 */
#define eh_llist_for_each(pos, head_or_prev)        \
	for ((pos) = ({                                                                                   \
                      eh_static_assert(                                                               \
                          eh_same_type( head_or_prev, struct eh_llist_head* )           ||            \
                          eh_same_type( head_or_prev, struct eh_llist_node* ),                        \
                          "pointer type mismatch in eh_llist_for_each_safe"                           \
                      );                                                                              \
                      head_or_prev ? ((struct eh_llist_node *)head_or_prev)->next : NULL;             \
                  });                                                                                 \
         (pos); (pos) = (pos)->next)


/**
 * @brief                   遍历链表成员项
 */
#define eh_llist_for_each_entry(pos, head_or_prev, member)				                                \
	for ((pos) = ({                                                                                     \
                      eh_static_assert(                                                                 \
                          eh_same_type( head_or_prev, struct eh_llist_head* )||                         \
                          eh_same_type( head_or_prev, struct eh_llist_node* ),                          \
                          "pointer type mismatch in eh_llist_for_each_safe"                             \
                      );                                                                                \
                      void *__tmp_ptr = (void *)(head_or_prev);                                         \
                      eh_llist_entry(                                                                   \
                            head_or_prev ? ((struct eh_llist_node *)__tmp_ptr)->next : NULL,            \
                            typeof(*(pos)), member);                                                    \
                  });                                                                                   \
	     eh_member_address_is_nonnull(pos, member);			                                            \
	     (pos) = eh_llist_entry((pos)->member.next, typeof(*(pos)), member))



/**
 * @brief                   遍历链表指针节点(安全)
 */ 
#define eh_llist_for_each_safe(prev, pos, _next, head_or_prev)		                                    \
for ( (prev) = ({                                                                                       \
                    eh_static_assert(                                                                   \
                        eh_same_type( head_or_prev, struct eh_llist_head* )||                           \
                        eh_same_type( head_or_prev, struct eh_llist_node* ),                            \
                        "pointer type mismatch in eh_llist_for_each_safe"                               \
                    );                                                                                  \
                    ((struct eh_llist_node *)head_or_prev);                                             \
                }),                                                                                     \
      (pos) = (prev) ? (prev)->next : NULL;                                                             \
      (pos) && ((_next) = (pos)->next, true);                                                           \
      (prev) = ((prev)->next == (_next) ? (prev) : (pos)),                                              \
      (pos) = (_next))


/**
 * @brief                   遍历链表指针节点(安全)
 */ 
#define eh_llist_for_each_safe_continue(prev, pos, _next, head_or_prev)		                            \
for ( (pos) = (prev) ? (prev)->next : NULL;                                                             \
      (pos) && ((_next) = (pos)->next, true);                                                           \
      (prev) = ((prev)->next == (_next) ? (prev) : (pos)),                                              \
      (pos) = (_next))


/**
 * @brief                   用于在遍历链表时安全地删除节点
 * @param  list             目标链表
 * @param  prev             要删除节点的上一个节点
 * @param  next             要删除节点的下一个节点
 */
static inline void eh_llist_del_node_in_for_each_safe(struct eh_llist_head *list, struct eh_llist_node *prev, struct eh_llist_node *next)
{
    prev->next = next;
    if(next == NULL)
        list->last = prev;
}

/**
 * @brief                   判断链表是否为空
 * @param  head
 * @return true 
 * @return false 
 */
static inline bool eh_llist_empty(const struct eh_llist_head *head)
{
	return head->first == NULL;
}

/**
 * @brief                   获取链表下一个节点
 * @param  node
 * @return struct eh_llist_node*
 */
static inline struct eh_llist_node *eh_llist_next(struct eh_llist_node *node)
{
	return node->next;
}

/**
 * @brief                   头插法，插入一批链表到链表头
 * @param  new_first        要插入链表的第一个节点，
 * @param  new_last         要插入链表的最后一个节点
 * @param  head             链表头
 * @return                  如果在添加此条目之前列表为空，则返回true
 */
extern bool eh_llist_add_batch(struct eh_llist_node *new_first, struct eh_llist_node *new_last, struct eh_llist_head *head);

/**
 * @brief                   尾插法，插入一批链表到链表尾
 * @param  new_first        要插入链表的第一个节点，
 * @param  new_last         要插入链表的最后一个节点
 * @param  head             链表头
 * @return                  如果在添加此条目之前列表为空，则返回true
 */
extern bool eh_llist_add_batch_tail(struct eh_llist_node *new_first, struct eh_llist_node *new_last, struct eh_llist_head *head);


/**
 * @brief                   从任意位置插入一个链表节点
 * @param  prev             要插入节点的前一个节点
 * @param  new              要插入的节点
 * @param  head             链表头
 * @return                  如果在添加此条目之前列表为空，则返回true
 */
static inline void eh_llist_insert(struct eh_llist_node *prev, struct eh_llist_node *new, struct eh_llist_head *head){
    new->next = prev->next;
    prev->next = new;
    if(new->next == NULL)
        head->last = new;
}

/**
 * @brief                   头插法，插入一个链表到链表头
 * @param  new              新节点
 * @param  head             链表头
 * @return                  如果在添加此条目之前列表为空，则返回true
 */
static inline bool eh_llist_add(struct eh_llist_node *new, struct eh_llist_head *head){
    return eh_llist_add_batch(new, new, head);
}

/**
 * @brief                   尾插法，插入一个链表到链表尾
 * @param  new              新节点
 * @param  head             链表头
 * @return                  如果在添加此条目之前列表为空，则返回true
 */
static inline bool eh_llist_add_tail(struct eh_llist_node *new, struct eh_llist_head *head){
    return eh_llist_add_batch_tail(new, new, head);
}

/**
 * @brief                   删除一个链表节点 (如果你不清楚你再干什么，请勿调用)
 * @param  prev_node        要填入想要删除节点的上一个节点
 */
static inline void _eh_llist_del(struct eh_llist_node *prev_node, struct eh_llist_node *del_node){
    prev_node->next = del_node->next;
}

/**
 * @brief                   删除一个链表节点，并初始化该节点 (如果你不清楚你再干什么，请勿调用)
 * @param  prev_node        要填入想要删除节点的上一个节点
 */
static inline void _eh_llist_del_init(struct eh_llist_node *prev_node, struct eh_llist_node *del_node){
    _eh_llist_del(prev_node, del_node);
    eh_llist_node_init(del_node);
}

/**
 * @brief                   删除链表头节点
 * @param  head             链表头
 */
static inline void eh_llist_del_first(struct eh_llist_head *head){
    _eh_llist_del((struct eh_llist_node *)&head->first, head->first);
}

/**
 * @brief                   删除链表头节点，并初始化该节点
 * @param  head             链表头
 */
static inline void eh_llist_del_first_init(struct eh_llist_head *head){
    _eh_llist_del_init((struct eh_llist_node *)&head->first, head->first);
}

/**
 * @brief                   栈：压入一个链表节点
 * @param  new              新节点
 * @param  head             链表头
 */
static inline void eh_llist_push(struct eh_llist_node *new, struct eh_llist_head *head){
    eh_llist_add(new, head);
}

/**
 * @brief                   栈：弹出一个链表节点
 * @param  head             链表头
 * @return struct eh_llist_node*   返回被弹出的节点，栈空时返回NULL
 */
static inline struct eh_llist_node *eh_llist_pop(struct eh_llist_head *head){
    struct eh_llist_node *node = head->first;
    if(node)
        _eh_llist_del_init((struct eh_llist_node *)&head->first, node);
    return node;
}

/**
 * @brief                   队列：入队一个链表节点
 * @param  new              新节点
 * @param  head             链表头
 */
static inline void eh_llist_enqueue(struct eh_llist_node *new, struct eh_llist_head *head){
    eh_llist_add_tail(new, head);
}

/**
 * @brief                   队列：出队一个链表节点
 * @param  head             链表头
 * @return struct eh_llist_node*   返回被出队的节点，队列空时返回NULL
 */
#define eh_llist_dequeue(head) eh_llist_pop(head)

/**
 * @brief                   返回首个节点
 * @param  head             链表头
 * @return struct eh_llist_node*   返回头节点，队列为空时返回NULL
 */
#define eh_llist_first(head) ((head)->first)

/**
 * @brief                   返回最后一个节点
 * @param  head             链表头
 * @return struct eh_llist_node*   返回尾节点，队列为空时返回NULL
 */
#define eh_llist_last(head)  ((head)->last)

/**
 * @brief                   队列/栈：查看偷看数据
 * @param  head             链表头
 * @return struct eh_llist_node*   返回队列头节点，队列为空时返回NULL
 */
#define eh_llist_peek(head) eh_llist_first(head)

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif // _EH_LLIST_H_