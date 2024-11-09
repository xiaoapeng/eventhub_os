/**
 * @file eh_mem_pool.c
 * @brief 
 * @author simon.xiaoapeng (simon.xiaoapeng@gmail.com)
 * @date 2024-10-06
 * 
 * @copyright Copyright (c) 2024  simon.xiaoapeng@gmail.com
 * 
 */

#include "eh_types.h"
#include "eh_mem.h"
#include "eh_error.h"
#include "eh_mem_pool.h"
#include "eh_debug.h"


#include <stddef.h>

struct eh_mem_pool_list{
    struct eh_mem_pool_list     *next;
};

struct eh_mem_pool{
    void                        *base;
    size_t                      align_size;
    size_t                      num;
    struct eh_mem_pool_list     free_list_head;
    struct eh_mem_pool_list     free_list[0];
};

eh_mem_pool_t eh_mem_pool_create(size_t align, size_t size, size_t num){
    size_t allocation_size = sizeof(struct eh_mem_pool) + 
        num * sizeof(struct eh_mem_pool_list) + 
        (eh_align_up(size, align) * num + (align-1));
    size_t i;
    struct eh_mem_pool *pool = eh_malloc(allocation_size);
    if( pool == NULL )
        return eh_error_to_ptr(EH_RET_MALLOC_ERROR);
    
    pool->base = (void*)eh_align_up((unsigned long)(pool->free_list + num), align);
    pool->align_size = eh_align_up(size, align);
    pool->num = num;
    pool->free_list_head.next = pool->free_list;
    for(i = 0; i < (num - 1); i++){
        pool->free_list[i].next = pool->free_list + i + 1;
    }
    pool->free_list[i].next = NULL;
    
    return (eh_mem_pool_t)pool;
}

void  eh_mem_pool_destroy(eh_mem_pool_t pool){
    eh_free(pool);
}

void* eh_mem_pool_alloc(eh_mem_pool_t _pool){
    void *new_mem;
    struct eh_mem_pool_list     *new_mem_node;
    
    struct eh_mem_pool *pool = (struct eh_mem_pool *)_pool;
    size_t index;
    
    if(pool->free_list_head.next == NULL)
        return NULL;

    index = (size_t)(pool->free_list_head.next - pool->free_list);
    if(index >= pool->num){
        eh_warnfl("pool index out of range pool=%p index=%d num=%d", pool, index, pool->num);
        return NULL;
    }
    new_mem_node = pool->free_list_head.next;
    new_mem = (char*)pool->base + index*pool->align_size;
    pool->free_list_head.next = new_mem_node->next;
    /* 如果节点的下一个节点指向自己，那么意味着该节点已经被分配出去 */
    new_mem_node->next = new_mem_node;
    return new_mem;
}

void  eh_mem_pool_free(eh_mem_pool_t _pool, void* ptr){
    struct eh_mem_pool *pool = (struct eh_mem_pool *)_pool;
    size_t index;
    index = (size_t)((char*)ptr - (char*)pool->base)/pool->align_size;
    if(index >= pool->num){
        eh_warnfl("pool index out of range pool=%p index=%d num=%d", pool, index, pool->num);
        return ;
    }
    if(pool->free_list[index].next != &pool->free_list[index]){
        /* 释放一个没有被分配的pool mem */
        eh_warnfl("Release an unallocated pool mem.");
        return ;
    }
    pool->free_list[index].next = pool->free_list_head.next;
    pool->free_list_head.next = pool->free_list + index;
}


int eh_mem_pool_is_from_this(eh_mem_pool_t _pool, void* ptr){
    struct eh_mem_pool *pool = (struct eh_mem_pool *)_pool;
    return  ((char*)ptr >= (char*)pool->base && 
            (char*)ptr < (char*)pool->base + pool->num*pool->align_size) && 
            ((size_t)((char*)ptr - (char*)pool->base) % pool->align_size == 0);
}