/**
 * @file eh_hashtbl.h
 * @brief  哈希表实现
 * @author simon.xiaoapeng (simon.xiaoapeng@gmail.com)
 * @date 2025-02-25
 * 
 * @copyright Copyright (c) 2025  simon.xiaoapeng@gmail.com
 * 
 */

#ifndef _EH_HASHTBL_H_
#define _EH_HASHTBL_H_

#include <stdint.h>
#include <eh_list.h>
#include <eh_error.h>
#include <eh_types.h>
#include <eh_list.h>

typedef uint16_t eh_hashtbl_kv_len_t;
typedef int*    eh_hashtbl_t;
typedef uint32_t eh_hash_val_t;
#define EH_HASHTBL_DEFAULT_LOADFACTOR   0.75

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */

#define EH_HASHTBL_KV_ALIGN             sizeof(unsigned long)
struct eh_hashtbl_node{
    struct eh_list_head                         node;
    eh_hash_val_t                               hash_val;
    eh_hashtbl_kv_len_t                         value_len;
    eh_hashtbl_kv_len_t                         key_len;
    uint8_t   eh_aligned(EH_HASHTBL_KV_ALIGN)   kv[0];
};

struct eh_hashtbl{
    struct eh_list_head                         *table;                 /* 散列表 */
    unsigned int                                mask;                   /* 散列表大小减1,散列表大小永远为2的次幂 */
    unsigned int                                threshold;              /* 阈值,达到该值时自动扩容 */
    unsigned int                                count;                  /* 元素个数 */
};



extern struct eh_list_head *_eh_hashtbl_find_list_head(eh_hashtbl_t hashtbl, const void *key, eh_hashtbl_kv_len_t key_len);
extern struct eh_list_head *_eh_hashtbl_find_list_head_with_string(eh_hashtbl_t hashtbl, const char *key_str);

/* 判断哈希表节点是否需要重建*/
#define eh_hash_table_node_is_need_remake(hashtbl, idx)  ((hashtbl)->table[idx].next == NULL)

/**
 * @brief                   创建哈希表
 * @param  load_factor       负载因子
 * @return eh_hashtbl_t     成功返回哈希表句柄，失败需使用eh_ptr_to_error转换为错误码
 */
extern eh_hashtbl_t eh_hashtbl_create(float load_factor);

/**
 * @brief                   销毁哈希表
 * @param  hashtbl          哈希表句柄
 */
extern void eh_hashtbl_destroy(eh_hashtbl_t hashtbl);

/**
 * @brief                   创建哈希表节点, 调用该函数后需要手动赋值key，
 *                          然后调用 eh_hashtbl_node_key_refresh 进行刷新，最后在插入哈希表
 * @param  key_len          键长度
 * @param  value_len        值长度
 * @return struct eh_hashtbl_node* 
 */
extern struct eh_hashtbl_node* eh_hashtbl_node_new(eh_hashtbl_kv_len_t key_len, eh_hashtbl_kv_len_t value_len);

/**
 * @brief                   创建哈希表节点,且自动刷新内部哈希值，节点不挂在任何哈希表上，若要进行插入需要显式调用
 * @param  hashtbl          哈希表句柄
 * @param  key              键
 * @param  key_len          键长度
 * @param  value_len        值长度
 * @return struct eh_hashtbl_node*      成功返回哈希表节点句柄，失败返回NULL
 */
extern struct eh_hashtbl_node* eh_hashtbl_node_new_refresh(eh_hashtbl_t hashtbl, const void *key, eh_hashtbl_kv_len_t key_len, eh_hashtbl_kv_len_t value_len);

/**
 * @brief                   创建哈希表节点,且自动刷新内部哈希值，节点不挂在任何哈希表上，若要进行插入需要显式调用
 * @param  hashtbl          哈希表句柄
 * @param  key              键
 * @param  value_len        值长度
 * @return struct eh_hashtbl_node*      成功返回哈希表节点句柄，失败返回NULL
 */
extern struct eh_hashtbl_node* eh_hashtbl_node_new_with_string_refresh(eh_hashtbl_t hashtbl, const char *key, eh_hashtbl_kv_len_t value_len);

/**
 * @brief                   重建哈希表节点，如果新值长度与旧值不一样，会重新分配内存，如何该节点被挂在哈希表上，则会自动更新
 * @param  hashtbl          哈希表句柄
 * @param  old_node         旧节点，该节点将会自动删除，无需用户手动释放
 * @param  new_value_len    新“值”长度
 * @return struct eh_hashtbl_node*      成功返回哈希表节点句柄，失败返回NULL
 */
extern struct eh_hashtbl_node* eh_hashtbl_node_renew(eh_hashtbl_t hashtbl, struct eh_hashtbl_node* old_node, eh_hashtbl_kv_len_t value_len);

/**
 * @brief                   刷新哈希表节点的键,在强制修改键值后必须调用
 * @param  hashtbl          哈希表句柄
 * @param  node             哈希表节点句柄
 */
extern void eh_hashtbl_node_key_refresh(eh_hashtbl_t hashtbl, struct eh_hashtbl_node* node);

/**
 * @brief                   删除哈希表节点,如果该节点已经被挂在哈希表上，则会自动从哈希表上删除
 * @param  hashtbl          哈希表句柄
 * @param  node             哈希表节点句柄
 */
extern void eh_hashtbl_node_delete(eh_hashtbl_t hashtbl, struct eh_hashtbl_node *node);

/** 
 * @brief                  获取哈希表节点的键 
 * @param  node            哈希表节点句柄
 * @return const void*     返回 “键”
 */
#define eh_hashtbl_node_const_key(node)    ((const void*)((node)->kv))

/**
 * @brief                  获取哈希表节点的键,如果修改key,请立即调用 eh_hashtbl_node_key_refresh
 * @param  node            哈希表节点句柄
 * @return void*           返回 “键”
 */
#define eh_hashtbl_node_key(node)         ((void*)((node)->kv))

/** 
 * @brief                  获取哈希表节点的值 
 * @param  node            哈希表节点句柄
 * @return void*           返回 “值”
 */
#define eh_hashtbl_node_value(node)        ((void*)((node)->kv + eh_align_up((node)->key_len, EH_HASHTBL_KV_ALIGN)))

/**
 * @brief                   获取哈希表节点的键长度
 * @param  node             哈希表节点句柄
 * @return eh_hashtbl_kv_len_t      返回 “键长度”
 */
#define eh_hashtbl_node_key_len(node)      ((node)->key_len)

/**
 * @brief                   获取哈希表节点的值长度
 * @param  node             哈希表节点句柄
 * @return eh_hashtbl_kv_len_t      返回 “值长度”
 */
#define eh_hashtbl_node_value_len(node)    ((node)->value_len)

/**
 * @brief                   获取哈希表节点是否已经插入
 * @param  node             哈希表节点句柄
 * @return bool             插入状态
 */
#define eh_hashtbl_node_is_insert(_node)     (!eh_list_empty(&(_node)->node))

/**
 * @brief                   向哈希表插入节点
 * @param  hashtbl          哈希表句柄
 * @param  node             哈希表节点句柄
 * @return int              成功返回0，失败返回错误码
 */
extern int eh_hashtbl_insert(eh_hashtbl_t hashtbl, struct eh_hashtbl_node *node);

/**
 * @brief                   把某节点从哈希表中移除
 * @param  hashtbl          哈希表句柄
 * @param  node             哈希表节点句柄
 */
extern void eh_hashtbl_remove(eh_hashtbl_t hashtbl, struct eh_hashtbl_node *node);

/**
 * @brief                   从哈希表中寻找节点
 * @param  hashtbl          哈希表句柄
 * @param  key              键
 * @param  key_len          键长度
 * @param  out_node         输出节点
 * @return int              成功返回0，失败返回错误码
 */
extern int eh_hashtbl_find(eh_hashtbl_t hashtbl, const void *key, eh_hashtbl_kv_len_t key_len, struct eh_hashtbl_node **out_node);

/**
 * @brief                   从哈希表中寻找节点,键为字符串
 * @param  hashtbl          哈希表句柄
 * @param  key_str          键字符串
 * @param  out_node         输出节点
 * @return int              成功返回0，失败返回错误码
 */
extern int eh_hashtbl_find_with_string(eh_hashtbl_t hashtbl, const char *key_str, struct eh_hashtbl_node **out_node);

/**
 * @brief                   遍历哈希表
 * @param  hashtbl          哈希表句柄
 * @param  node_pos         节点位置
 * @param  node_tmp_n       临时变量
 * @param  tmp_uint_i       临时变量

 */
#define eh_hashtbl_for_each_safe(hashtbl, node_pos, node_tmp_n, tmp_uint_i)                                     \
    for(({                                                                                                      \
            eh_static_assert(                                                                                   \
                eh_same_type(hashtbl, eh_hashtbl_t),                                                            \
                "hashtbl must be eh_hashtbl_t"                                                                  \
            );                                                                                                  \
            tmp_uint_i = 0;                                                                                     \
        });                                                                                                     \
        tmp_uint_i <= ((struct eh_hashtbl*)(hashtbl))->mask;                                                    \
        tmp_uint_i++)                                                                                           \
        if(!eh_hash_table_node_is_need_remake(((struct eh_hashtbl*)(hashtbl)), tmp_uint_i))                     \
            eh_list_for_each_entry_safe(node_pos, node_tmp_n,                                                   \
                ((struct eh_hashtbl*)(hashtbl))->table + tmp_uint_i, node)

/**
 * @brief                   遍历哈希表,键为字符串
 * @param  hashtbl          哈希表句柄
 * @param  string           字符串
 * @param  node_pos         节点位置
 * @param  node_tmp_n       临时变量
 * @param  list_tmp_head    临时变量
 */
#define eh_hashtbl_for_each_with_string_safe(hashtbl, string, node_pos, node_tmp_n, list_tmp_head)              \
    for (list_tmp_head = _eh_hashtbl_find_list_head_with_string(hashtbl, string),                               \
        node_pos = eh_list_entry((list_tmp_head)->next, typeof(*node_pos), node),                               \
        node_tmp_n = eh_list_entry(node_pos->node.next, typeof(*node_pos), node);                               \
        &node_pos->node != (list_tmp_head);                                                                     \
        node_pos = node_tmp_n, node_tmp_n = eh_list_entry(node_tmp_n->node.next, typeof(*node_tmp_n), node))    \
        if(strncmp((const char*)eh_hashtbl_node_const_key(node_pos), string, eh_hashtbl_node_key_len(node_pos)) == 0)

/**
 * @brief                   遍历哈希表,键为二进制
 * @param  hashtbl          哈希表句柄
 * @param  key              键
 * @param  len              键长度
 * @param  node_pos         节点位置
 * @param  node_tmp_n       临时变量
 * @param  list_tmp_head    临时变量 eh_list_head 类型
 */
#define eh_hashtbl_for_each_with_key_safe(hashtbl, key, len, node_pos, node_tmp_n, list_tmp_head)               \
    for (list_tmp_head = _eh_hashtbl_find_list_head(hashtbl, key, len),                                         \
        node_pos = eh_list_entry((list_tmp_head)->next, typeof(*node_pos), node),                               \
        node_tmp_n = eh_list_entry(node_pos->node.next, typeof(*node_pos), node);                               \
        &node_pos->node != (list_tmp_head);                                                                     \
        node_pos = node_tmp_n, node_tmp_n = eh_list_entry(node_tmp_n->node.next, typeof(*node_tmp_n), node))    \
        if(len == eh_hashtbl_node_key_len(node_pos) && memcmp(eh_hashtbl_node_const_key(node_pos), key, len) == 0)

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif // _EH_HASHTBL_H_