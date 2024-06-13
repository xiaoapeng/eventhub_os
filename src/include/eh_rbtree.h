
#ifndef	_LINUX_RBTREE_H
#define	_LINUX_RBTREE_H

#include <stddef.h>
#include <stdbool.h>
#include <bits/types.h>
#include "eh_types.h"


typedef unsigned long parent_node_t;
struct eh_rbtree_node {
	parent_node_t  parent_and_color;
	struct eh_rbtree_node *rb_right;
	struct eh_rbtree_node *rb_left;
} __attribute__((aligned(sizeof(long))));
/* The alignment might seem pointless, but allegedly CRIS needs it */

struct eh_rbtree_root {
	struct eh_rbtree_node *rb_node;
	struct eh_rbtree_node *rb_leftmost;
	/**
	 * @brief  	内部使用的比较函数
	 * @return  -1:a<b  0:a==b  1:a>b
	 */
	int (*cmp)(struct eh_rbtree_node *a, struct eh_rbtree_node *b);
};

#define	RB_RED		0
#define	RB_BLACK	1

#define __rb_parent(pc)    ((struct eh_rbtree_node *)(pc & ~((parent_node_t)3)))

#define __rb_color(pc)     ((pc) & ((parent_node_t)1))
#define __rb_is_black(pc)  __rb_color(pc)
#define __rb_is_red(pc)    (!__rb_color(pc))
#define rb_color(rb)       __rb_color((rb)->parent_and_color)
#define rb_is_red(rb)      __rb_is_red((rb)->parent_and_color)
#define rb_is_black(rb)    __rb_is_black((rb)->parent_and_color)


#define rb_parent(r)   ((struct eh_rbtree_node *)((r)->parent_and_color & ~((parent_node_t)3)))

#define	eh_rb_entry(ptr, type, member) container_of(ptr, type, member)

#define eh_rb_entry_safe(ptr, type, member) \
	({ typeof(ptr) ____ptr = (ptr); \
	   ____ptr ? eh_rb_entry(____ptr, type, member) : NULL; \
	})

#define eh_rb_root_is_empty(root)  ((root)->rb_node == NULL)

/* 'empty' nodes are nodes that are known not to be inserted in an rbtree */
#define eh_rb_node_is_empty(node)  \
	((node)->parent_and_color == (parent_node_t)(node))
#define eh_rb_node_clear(node)  \
	((node)->parent_and_color = (parent_node_t)(node))

#define eh_rb_root_init(root, __cmp)	\
	do{									\
		(root)->rb_node = NULL;			\
		(root)->rb_leftmost = NULL;		\
		(root)->cmp = (__cmp);				\
	}while(0)

#define eh_rb_node_init(node)		\
	do{									\
		(node)->parent_and_color = (parent_node_t)(node);	\
		(node)->rb_left = NULL;			\
		(node)->rb_right = NULL;			\
	}while(0)

extern struct eh_rbtree_node * eh_rb_del(struct eh_rbtree_node *, struct eh_rbtree_root *);
extern struct eh_rbtree_node * eh_rb_add(struct eh_rbtree_node *node, struct eh_rbtree_root *tree);
extern struct eh_rbtree_node * eh_rb_find_add(struct eh_rbtree_node *node, struct eh_rbtree_root *tree);
extern struct eh_rbtree_node * eh_rb_match_find(const void *key, struct eh_rbtree_root *tree, 
	int (*match)(const void *key, const struct eh_rbtree_node *));
/* Find logical next and previous nodes in a tree */
extern struct eh_rbtree_node *	eh_rb_next(const struct eh_rbtree_node *);
extern struct eh_rbtree_node *	eh_rb_prev(const struct eh_rbtree_node *);
#define 						eh_rb_first(root)	((root)->rb_leftmost)
extern struct eh_rbtree_node *	eh_rb_last(const struct eh_rbtree_root *);
/* Postorder iteration - always visit the parent after its children */
extern struct eh_rbtree_node *rb_first_postorder(const struct eh_rbtree_root *root);
extern struct eh_rbtree_node *rb_next_postorder(const struct eh_rbtree_node *);

/*后序 左右根 */
#define eh_rb_postorder_for_each_entry_safe(pos, n, root, member) \
	for (pos = eh_rb_entry_safe(rb_first_postorder(root), typeof(*pos), member); \
	     pos && ({ n = eh_rb_entry_safe(rb_next_postorder(&pos->member), \
			typeof(*pos), member); 1; }); \
	     pos = n)

/* 递增遍历 */
#define eh_rb_next_for_each_entry(pos, root, member) \
    for (pos = eh_rb_entry_safe(eh_rb_first(root), typeof(*pos), member); \
         pos ; \
         pos = eh_rb_entry_safe(eh_rb_next(&pos->member), typeof(*pos), member))

/* 递减遍历 */
#define eh_rb_prev_for_each_entry(pos, root, member) \
    for (pos = eh_rb_entry_safe(eh_rb_last(root), typeof(*pos), member) ; \
         pos ; 	\
         pos = eh_rb_entry_safe(eh_rb_prev(&pos->member), typeof(*pos), member))


static __always_inline struct eh_rbtree_node *
eh_rb_find_first(const void *key, const struct eh_rbtree_root *tree,
	      int (*match)(const void *key, const struct eh_rbtree_node *))
{
	struct eh_rbtree_node *node = tree->rb_node;
	struct eh_rbtree_node *match_node = NULL;

	while (node) {
		int c = match(key, node);

		if (c <= 0) {
			if (!c)
				match_node = node;
			node = node->rb_left;
		} else if (c > 0) {
			node = node->rb_right;
		}
	}

	return match_node;
}

static __always_inline struct eh_rbtree_node *
eh_rb_next_match(const void *key, struct eh_rbtree_node *node,
	      int (*match)(const void *key, const struct eh_rbtree_node *))
{
	node = eh_rb_next(node);
	if (node && match(key, node))
		node = NULL;
	return node;
}

#define eh_rb_for_entry_each(pos, tree, key, match, member) \
	for ((pos) = eh_rb_entry_safe(eh_rb_find_first((key), (tree), \
			(match)), typeof(*pos), member); \
	     (pos); (pos) = eh_rb_entry_safe( eh_rb_next_match((key), &((pos)->member), (match)), \
		 	typeof(*pos), member))



#endif	/* _LINUX_RBTREE_H */
