/**
 * @file eh_hashtbl.c
 * @brief 哈希表实现
 * @author simon.xiaoapeng (simon.xiaoapeng@gmail.com)
 * @date 2025-02-26
 * 
 * @copyright Copyright (c) 2025  simon.xiaoapeng@gmail.com
 * 
 */

#include <string.h>
#include <eh.h>
#include <eh_types.h>
#include <eh_list.h>
#include <eh_mem.h>
#include <eh_hashtbl.h>

eh_static_assert(
    (EH_HASHTBL_MIN_SIZE >= 8) && ((EH_HASHTBL_MIN_SIZE & (EH_HASHTBL_MIN_SIZE - 1)) == 0),
    "EH_HASHTBL_MIN_SIZE must be a power of 2 and at least 8"
);


#define eh_hash_val(key, key_len)           fnv1a(key, key_len)
#define eh_hash_str_val(str, out_len_ptr)   fnv1a_str(str, out_len_ptr)




#define FNV_OFFSET_BASIS_32 2166136261U 
#define FNV_PRIME_32 16777619U 
 
static eh_hash_val_t fnv1a(const char *data, eh_hashtbl_kv_len_t len) {
    const char *bp = (const char *)data;
    const char *be = bp + len;
    eh_hash_val_t hash = FNV_OFFSET_BASIS_32;
    while(bp < be) {
        hash ^= (eh_hash_val_t)*(unsigned char*)bp++;
        hash *= FNV_PRIME_32; 
    }
    return hash;
}

static eh_hash_val_t fnv1a_str(const char *str, eh_hashtbl_kv_len_t *out_len) {
    const char *bp = str;
    eh_hash_val_t hash = FNV_OFFSET_BASIS_32;
    while(*bp) {
        hash ^= (eh_hash_val_t)*(unsigned char*)bp++;
        hash *= FNV_PRIME_32; 
    }
    if(out_len)
        *out_len = (eh_hashtbl_kv_len_t)(bp - str);
    return hash;
}

static int eh_hashtbl_resize(struct eh_hashtbl *hashtbl){
    /* 扩容 */
    unsigned int old_size = hashtbl->mask + 1;
    unsigned int new_mask = (hashtbl->mask << 1) + 1;
    unsigned int i;
    struct eh_list_head *new_table = eh_malloc(sizeof(struct eh_list_head) * (new_mask + 1));
    if(new_table == NULL)
        return EH_RET_MALLOC_ERROR;
    
    /* 新的后一半表项应该设置为待重建，所以设置为0 */
    memset(new_table + old_size, 0, sizeof(struct eh_list_head) * old_size);

    /* 前一半应该继承以前的表项 */
    for(i = 0; i < old_size; i++){
        /* 以前重建的依旧拷贝为重建 */
        if(eh_hash_table_node_is_need_remake(hashtbl, i)){
            new_table[i].next = NULL;
            continue;
        }
        eh_list_head_init(&new_table[i]);
        eh_list_splice(&hashtbl->table[i], &new_table[i]);
    }


    eh_free(hashtbl->table);
    hashtbl->table = new_table;
    hashtbl->mask = new_mask;
    hashtbl->threshold = hashtbl->threshold << 1;
    return EH_RET_OK;
}

static void eh_hashtbl_try_remake(struct eh_hashtbl *hashtbl, unsigned int idx){
    /* 哈希渐进式重建 */
    unsigned int old_idx;
    struct eh_list_head *pos, *n;
    if(eh_hash_table_node_is_need_remake(hashtbl, idx)){
        unsigned int mask_tmp = hashtbl->mask >> 1;
        for( mask_tmp = hashtbl->mask >> 1; 
             mask_tmp && eh_hash_table_node_is_need_remake(hashtbl, (idx & mask_tmp));
             mask_tmp = mask_tmp >> 1){
            /* find ... */
        }
        old_idx = idx & mask_tmp;
        /* 用旧的 table node 进行重建 */
        eh_list_for_each_safe(pos, n, &hashtbl->table[old_idx]){
            struct eh_hashtbl_node *node = eh_list_entry(pos, struct eh_hashtbl_node, node);
            unsigned int new_idx = node->hash_val & hashtbl->mask;
            if(new_idx != old_idx){
                eh_list_del(pos);
                if(eh_hash_table_node_is_need_remake(hashtbl, new_idx))
                    eh_list_head_init(hashtbl->table+new_idx);
                eh_list_add(pos, hashtbl->table+new_idx);
            }
        }
        if(eh_hash_table_node_is_need_remake(hashtbl, idx))
            eh_list_head_init(hashtbl->table+idx);
    }
}

struct eh_hashtbl_node* eh_hashtbl_node_new(eh_hashtbl_t hashtbl, 
        const void *key, eh_hashtbl_kv_len_t key_len, eh_hashtbl_kv_len_t value_len){
    (void)hashtbl;
    struct eh_hashtbl_node *node = eh_malloc(sizeof(struct eh_hashtbl_node) + 
        eh_align_up(key_len, EH_HASHTBL_KV_ALIGN) +  eh_align_up(value_len, EH_HASHTBL_KV_ALIGN));
    if(node == NULL)
        return NULL;
    memcpy(node->kv, key, key_len);
    node->value_len = value_len;
    node->key_len = key_len;
    node->hash_val = eh_hash_val(key, key_len);
    eh_list_head_init(&node->node);
    return node;
}


struct eh_hashtbl_node* eh_hashtbl_node_new_with_string(eh_hashtbl_t hashtbl, 
    const char *key, eh_hashtbl_kv_len_t value_len){
    (void)hashtbl;
    eh_hashtbl_kv_len_t key_len;
    eh_hash_val_t hash_val = eh_hash_str_val(key, &key_len);
    struct eh_hashtbl_node *node = eh_malloc(sizeof(struct eh_hashtbl_node) + 
        eh_align_up(key_len, EH_HASHTBL_KV_ALIGN) +  eh_align_up(value_len, EH_HASHTBL_KV_ALIGN));
    if(node == NULL)
        return NULL;
    memcpy(node->kv, key, key_len);
    node->value_len = value_len;
    node->key_len = key_len;
    node->hash_val = hash_val;
    eh_list_head_init(&node->node);
    return node;
}


struct eh_hashtbl_node* eh_hashtbl_node_renew(eh_hashtbl_t hashtbl, 
    struct eh_hashtbl_node* old_node, eh_hashtbl_kv_len_t value_len){
    (void)hashtbl;
    struct eh_hashtbl_node *node;
    if(old_node->value_len == value_len)
        return old_node;
    node = eh_malloc(sizeof(struct eh_hashtbl_node) + 
        eh_align_up(old_node->key_len, EH_HASHTBL_KV_ALIGN) + eh_align_up(value_len, EH_HASHTBL_KV_ALIGN));
    if(node == NULL)
        return NULL;
    node->value_len = value_len;
    node->key_len = old_node->key_len;
    node->hash_val = old_node->hash_val;
    memcpy(node->kv, old_node->kv, old_node->key_len);
    /* 检查旧节点是否挂在哈希表上 */
    if(!eh_list_empty(&old_node->node)){
        eh_list_add(&node->node, &old_node->node);
        eh_list_del(&old_node->node);
    }else{
        eh_list_head_init(&node->node);
    }
    eh_free(old_node);
    return node;
}


void eh_hashtbl_node_delete(eh_hashtbl_t _hashtbl, struct eh_hashtbl_node *node){
    struct eh_hashtbl *hashtbl = (struct eh_hashtbl *)_hashtbl;
    if(!eh_list_empty(&node->node)){
        eh_list_del(&node->node);
        hashtbl->count--;
    }
    eh_free(node);
}


int eh_hashtbl_insert(eh_hashtbl_t _hashtbl, struct eh_hashtbl_node *node){
    struct eh_hashtbl *hashtbl = (struct eh_hashtbl *)_hashtbl;
    int ret;
    unsigned int idx;
    if(!eh_list_empty(&node->node))
        return EH_RET_EXISTS;
    if(hashtbl->count + 1 >= hashtbl->threshold && hashtbl->mask != UINT32_MAX){
        ret = eh_hashtbl_resize(hashtbl);
        if(ret != EH_RET_OK)
            return ret;
    }
    idx = node->hash_val & hashtbl->mask;

    /* 尝试进行重建 */
    eh_hashtbl_try_remake(hashtbl, idx);

    eh_list_add(&node->node, hashtbl->table + idx);
    hashtbl->count++;
    return EH_RET_OK;
}

struct eh_list_head *_eh_hashtbl_find_list_head(eh_hashtbl_t _hashtbl, const void *key, eh_hashtbl_kv_len_t key_len){
    struct eh_hashtbl *hashtbl = (struct eh_hashtbl *)_hashtbl;
    unsigned int idx = eh_hash_val(key, key_len) & hashtbl->mask;
    /* 尝试进行重建 */
    eh_hashtbl_try_remake(hashtbl, idx);
    return &hashtbl->table[idx];

}

struct eh_list_head *_eh_hashtbl_find_list_head_with_string(eh_hashtbl_t _hashtbl, const char *key_str){
    struct eh_hashtbl *hashtbl = (struct eh_hashtbl *)_hashtbl;
    eh_hashtbl_kv_len_t key_len;
    unsigned int idx = eh_hash_str_val(key_str, &key_len) & hashtbl->mask;
    /* 尝试进行重建 */
    eh_hashtbl_try_remake(hashtbl, idx);
    return &hashtbl->table[idx];
}

int eh_hashtbl_find(eh_hashtbl_t _hashtbl, const void *key, eh_hashtbl_kv_len_t key_len, struct eh_hashtbl_node **out_node){
    struct eh_hashtbl *hashtbl = (struct eh_hashtbl *)_hashtbl;
    unsigned int idx = eh_hash_val(key, key_len) & hashtbl->mask;
    struct eh_list_head *pos;

    /* 尝试进行重建 */
    eh_hashtbl_try_remake(hashtbl, idx);

    eh_list_for_each(pos, &hashtbl->table[idx]){
        struct eh_hashtbl_node *node = eh_list_entry(pos, struct eh_hashtbl_node, node);
        if(node->key_len == key_len && memcmp(node->kv, key, key_len) == 0){
            *out_node = node;
            return EH_RET_OK;
        }
    }
    return EH_RET_NOT_EXISTS;
}


int eh_hashtbl_find_with_string(eh_hashtbl_t _hashtbl, const char *key_str, struct eh_hashtbl_node **out_node){
    struct eh_hashtbl *hashtbl = (struct eh_hashtbl *)_hashtbl;
    eh_hashtbl_kv_len_t key_len;
    unsigned int idx = eh_hash_str_val(key_str, &key_len) & hashtbl->mask;
    struct eh_list_head *pos;

    /* 尝试进行重建 */
    eh_hashtbl_try_remake(hashtbl, idx);

    eh_list_for_each(pos, &hashtbl->table[idx]){
        struct eh_hashtbl_node *node = eh_list_entry(pos, struct eh_hashtbl_node, node);
        if(node->key_len == key_len && memcmp(node->kv, key_str, key_len) == 0){
            *out_node = node;
            return EH_RET_OK;
        }
    }
    return EH_RET_NOT_EXISTS;
}


eh_hashtbl_t eh_hashtbl_create(float load_factor){
    struct eh_hashtbl *hashtbl;
    eh_hashtbl_t ret;
    if(load_factor <= 0.0f)
        return eh_error_to_ptr(EH_RET_INVALID_PARAM);

    hashtbl = eh_malloc(sizeof(struct eh_hashtbl));
    if(hashtbl == NULL){
        return eh_error_to_ptr(EH_RET_MALLOC_ERROR);
    }
    hashtbl->mask = EH_HASHTBL_MIN_SIZE - 1;
    hashtbl->threshold = (unsigned int)(EH_HASHTBL_MIN_SIZE * load_factor);
    hashtbl->count = 0;
    hashtbl->table = eh_malloc(sizeof(struct eh_list_head) * EH_HASHTBL_MIN_SIZE);
    if(hashtbl->table == NULL){
        ret = eh_error_to_ptr(EH_RET_MALLOC_ERROR);
        goto table_malloc_error;
    }
    for(int i = 0; i < EH_HASHTBL_MIN_SIZE; i++){
        eh_list_head_init(&hashtbl->table[i]);
    }
    return (eh_hashtbl_t)hashtbl;
table_malloc_error:
    eh_free(hashtbl);
    return ret;
}

extern void eh_hashtbl_destroy(eh_hashtbl_t _hashtbl){
    struct eh_hashtbl *hashtbl = (struct eh_hashtbl *)_hashtbl;
    for(unsigned int i = 0; i <= hashtbl->mask; i++){
        struct eh_list_head *pos, *n;
        if(eh_hash_table_node_is_need_remake(hashtbl, i))
            continue;
        eh_list_for_each_safe(pos, n, &hashtbl->table[i]){
            struct eh_hashtbl_node *node = eh_list_entry(pos, struct eh_hashtbl_node, node);
            eh_list_del(pos);
            eh_free(node);
        }
    }
    eh_free(hashtbl->table);
    eh_free(hashtbl);
}






