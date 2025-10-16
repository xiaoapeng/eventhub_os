
#ifndef    _EH_RBTERR_H_
#define    _EH_RBTERR_H_

#include <stddef.h>
#include <stdbool.h>
#include <eh_types.h>


typedef unsigned long parent_node_t;
struct eh_rbtree_node {
    parent_node_t  parent_and_color;
    struct eh_rbtree_node *rb_right;
    struct eh_rbtree_node *rb_left;
} eh_aligned(sizeof(long));
/* The alignment might seem pointless, but allegedly CRIS needs it */

struct eh_rbtree_root {
    struct eh_rbtree_node *rb_node;
    struct eh_rbtree_node *rb_leftmost;
    /**
     * @brief      内部使用的比较函数
     * @return  -1:a<b  0:a==b  1:a>b
     */
    int (*cmp)(struct eh_rbtree_node *a, struct eh_rbtree_node *b);
};

#define EH_RBTREE_ROOT_INIT(root, _cmp) {               \
        .rb_node = NULL,                                \
        .rb_leftmost = NULL,                            \
        .cmp = _cmp,                                    \
    }
#define EH_RBTREE_NODE_INIT(root) {                     \
        .parent_and_color = (parent_node_t)&root,       \
        .rb_left = NULL,                                \
        .rb_right = NULL,                               \
    }

#define    RB_RED        0
#define    RB_BLACK    1

#define __rb_parent(pc)    ((struct eh_rbtree_node *)(pc & ~((parent_node_t)3)))

#define __rb_color(pc)     ((pc) & ((parent_node_t)1))
#define __rb_is_black(pc)  __rb_color(pc)
#define __rb_is_red(pc)    (!__rb_color(pc))
#define rb_color(rb)       __rb_color((rb)->parent_and_color)
#define rb_is_red(rb)      __rb_is_red((rb)->parent_and_color)
#define rb_is_black(rb)    __rb_is_black((rb)->parent_and_color)


#define rb_parent(r)   ((struct eh_rbtree_node *)((r)->parent_and_color & ~((parent_node_t)3)))

#define eh_rb_entry(ptr, type, member)         eh_container_of(ptr, type, member)

#define eh_rb_entry_safe(ptr, type, member)    eh_container_of_safe(ptr, type, member)

#define eh_rb_root_is_empty(root)  ((root)->rb_node == NULL)

/* 'empty' nodes are nodes that are known not to be inserted in an rbtree */
#define eh_rb_node_is_empty(node)                               \
    ((node)->parent_and_color == (parent_node_t)(node))
#define eh_rb_node_clear(node)                                  \
    ((node)->parent_and_color = (parent_node_t)(node))

#define eh_rb_root_init(root, __cmp)                            \
    do{                                                         \
        (root)->rb_node = NULL;                                 \
        (root)->rb_leftmost = NULL;                             \
        (root)->cmp = (__cmp);                                  \
    }while(0)

#define eh_rb_node_init(node)                                   \
    do{                                                         \
        (node)->parent_and_color = (parent_node_t)(node);       \
        (node)->rb_left = NULL;                                 \
        (node)->rb_right = NULL;                                \
    }while(0)

/**
 * @brief                     删除某个节点
 * @return struct eh_rbtree_node* 
 */
extern struct eh_rbtree_node * eh_rb_del(struct eh_rbtree_node *, struct eh_rbtree_root *);

/**
 * @brief                     添加节点
 * @param  node             要添加的节点
 * @param  tree             rb树对象
 * @return                     正常返回NULL, 返回 node 说明 插入最左的节点(为定时器而考虑的逻辑)
 */
extern struct eh_rbtree_node * eh_rb_add(struct eh_rbtree_node *node, struct eh_rbtree_root *tree);

/**
 * @brief                     添加节点
 * @param  node             要添加的节点
 * @param  tree             rb树对象
 * @return                  返回找到的节点, 如果没有找到返回插入的节点
 */
extern struct eh_rbtree_node * eh_rb_find_add(struct eh_rbtree_node *node, struct eh_rbtree_root *tree);




/**
 * @brief                     找到一个与key匹配的节点, 如果没有找到, 则创建一个新节点
 * @param  key              用于cmp函数比较的第一个参数
 * @param  tree             rb树对象
 * @param  cmp              比较函数
 * @param  user_data        用于new_node函数的参数
 * @param  new_node         创建新节点的函数
 * @return struct eh_rbtree_node* 
 */
extern struct eh_rbtree_node *eh_rb_find_new_add(const void *key, struct eh_rbtree_root *tree,
          int (*cmp)(const void *key, const struct eh_rbtree_node *), void *user_data,
          struct eh_rbtree_node* (new_node)(void *user_data));

/**
 * @brief                     找到一个与key匹配的节点
 * @param  key              用于match函数比较的第一个参数
 * @param  tree             rb树对象
 * @param  match            匹配函数
 * @return struct eh_rbtree_node* 
 */
extern struct eh_rbtree_node * eh_rb_match_find(const void *key, struct eh_rbtree_root *tree, 
    int (*match)(const void *key, const struct eh_rbtree_node *));

/**
 * @brief                     下一个节点
 * @return struct eh_rbtree_node* 
 */
extern struct eh_rbtree_node *    eh_rb_next(const struct eh_rbtree_node *);

/**
 * @brief                     上一个节点
 * @return struct eh_rbtree_node* 
 */
extern struct eh_rbtree_node *    eh_rb_prev(const struct eh_rbtree_node *);

/**
 * @brief                     第一个节点
 */
#define                         eh_rb_first(root)    ((root)->rb_leftmost)

/**
 * @brief                     最后一个节点
 * @return struct eh_rbtree_node* 
 */
extern struct eh_rbtree_node *    eh_rb_last(const struct eh_rbtree_root *);

/**
 * @brief                     找到后序遍历的第一个
 * @param  root             rb树
 * @return struct eh_rbtree_node* 
 */
extern struct eh_rbtree_node *rb_first_postorder(const struct eh_rbtree_root *root);

/**
 * @brief                     返回后续遍历的下一个
 * @param  node             本次遍历的节点
 * @return struct eh_rbtree_node* 
 */
extern struct eh_rbtree_node *rb_next_postorder(const struct eh_rbtree_node *);

/**
 * @brief                     找到一个与key匹配的节点,最左边的
 * @param  key              用于match函数比较的第一个参数
 * @param  tree             rb树对象
 * @param  match            匹配函数
 * @return struct eh_rbtree_node* 
 */
extern struct eh_rbtree_node *eh_rb_find_first(const void *key, const struct eh_rbtree_root *tree,
          int (*match)(const void *key, const struct eh_rbtree_node *));

/**
 * @brief                     找到下一个能匹配key的节点
 * @param  key              用于match函数比较的第一个参数
 * @param  node             本次用于匹配的节点
 * @param  match            匹配函数
 * @return struct eh_rbtree_node* 
 */
extern struct eh_rbtree_node *eh_rb_next_match(const void *key, struct eh_rbtree_node *node,
          int (*match)(const void *key, const struct eh_rbtree_node *));

/*后序 左右根 */
#define eh_rb_postorder_for_each_entry_safe(pos, n, root, member)                   \
    for (pos = eh_rb_entry_safe(rb_first_postorder(root), typeof(*pos), member);    \
         pos && ({ n = eh_rb_entry_safe(rb_next_postorder(&pos->member),            \
            typeof(*pos), member); 1; });                                           \
         pos = n)

/* 递增遍历 */
#define eh_rb_next_for_each_entry(pos, root, member)                                \
    for (pos = eh_rb_entry_safe(eh_rb_first(root), typeof(*pos), member);           \
         pos ;                                                                      \
         pos = eh_rb_entry_safe(eh_rb_next(&pos->member), typeof(*pos), member))

/* 递减遍历 */
#define eh_rb_prev_for_each_entry(pos, root, member)                                \
    for (pos = eh_rb_entry_safe(eh_rb_last(root), typeof(*pos), member) ;           \
         pos ;                                                                      \
         pos = eh_rb_entry_safe(eh_rb_prev(&pos->member), typeof(*pos), member))

/* 条件遍历 */
#define eh_rb_for_entry_each(pos, tree, key, match, member)                         \
    for ((pos) = eh_rb_entry_safe(eh_rb_find_first((key), (tree),                   \
            (match)), typeof(*pos), member);                                        \
         (pos); (pos) = eh_rb_entry_safe( eh_rb_next_match((key), &((pos)->member), (match)), \
             typeof(*pos), member))



#endif    /* _EH_RBTERR_H_ */
