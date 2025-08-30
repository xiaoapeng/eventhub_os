/**
 * @file eh_mem_pool.c
 * @brief 
 * @author simon.xiaoapeng (simon.xiaoapeng@gmail.com)
 * @date 2024-10-06
 * 
 * @copyright Copyright (c) 2024  simon.xiaoapeng@gmail.com
 * 
 */

#include <stddef.h>
#include <eh.h>
#include <eh_types.h>
#include <eh_platform.h>
#include <eh_mem.h>
#include <eh_error.h>
#include <eh_mem_pool.h>
#include <eh_debug.h>


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

void  eh_mem_pool_destroy(eh_mem_pool_t _pool){
    int i;
    void *ptr;
    struct eh_mem_pool *pool = (struct eh_mem_pool *)_pool;
    eh_mem_pool_for_each(i, pool, ptr){
        if(eh_mem_pool_idx_is_used(pool, i)){
            eh_mwarnfl(MEM_POOL, "pool mem is not freed pool=%p idx=%d", pool, i);
        }
    }
    eh_free(pool);
}

void* eh_mem_pool_alloc(eh_mem_pool_t _pool){
    void *new_mem = NULL;
    eh_save_state_t state;
    struct eh_mem_pool_list     *new_mem_node;
    
    struct eh_mem_pool *pool = (struct eh_mem_pool *)_pool;
    size_t index;
    
    state = eh_enter_critical();

    if(pool->free_list_head.next == NULL)
        goto quit;

    index = (size_t)(pool->free_list_head.next - pool->free_list);
    if(index >= pool->num){
        eh_mwarnfl(MEM_POOL, "pool index out of range pool=%p index=%d num=%d", pool, index, pool->num);
        goto quit;
    }
    new_mem_node = pool->free_list_head.next;
    new_mem = (char*)pool->base + index*pool->align_size;
    pool->free_list_head.next = new_mem_node->next;
    /* 如果节点的下一个节点指向自己，那么意味着该节点已经被分配出去 */
    new_mem_node->next = new_mem_node;
    eh_mdebugfl(MEM_POOL, "pool=%p index=%d new_mem=%p new_mem_node=%p", pool, index, new_mem, new_mem_node);
quit:
    eh_exit_critical(state);
    return new_mem;
}

void  eh_mem_pool_free(eh_mem_pool_t _pool, void* ptr){
    struct eh_mem_pool *pool = (struct eh_mem_pool *)_pool;
    size_t index;
    eh_save_state_t state;

    index = (size_t)((char*)ptr - (char*)pool->base)/pool->align_size;
    if(index >= pool->num){
        eh_mwarnfl(MEM_POOL, "pool index out of range pool=%p index=%d num=%d", pool, index, pool->num);
        return ;
    }
    state = eh_enter_critical();
    if(!eh_mem_pool_idx_is_used(_pool, index)){
        /* 释放一个没有被分配的pool mem */
        eh_mwarnfl(MEM_POOL, "Release an unallocated pool. mempool=%p index=%d num=%d", pool, index, pool->num);
        return ;
    }

    pool->free_list[index].next = pool->free_list_head.next;
    pool->free_list_head.next = pool->free_list + index;
    eh_exit_critical(state);
    
    eh_mdebugfl(MEM_POOL, "pool=%p index=%d ptr=%p", pool, index, ptr);
}

void eh_mem_pool_dump(eh_mem_pool_t _pool){
    struct eh_mem_pool *pool = (struct eh_mem_pool *)_pool;
    
    struct eh_mem_pool_list  *new_mem_node = pool->free_list_head.next;
    eh_minfoln(MEM_POOL, "pool=%0#p base=%0#p align_size=%d num=%d\n", pool, pool->base, pool->align_size, pool->num);
    eh_minforaw(MEM_POOL, "free list:");
    while(new_mem_node != NULL){
        eh_minforaw(MEM_POOL, " %d", new_mem_node - pool->free_list);
        new_mem_node = new_mem_node->next;
    }
    eh_minforaw(MEM_POOL, "\n");
}


int eh_mem_pool_ptr_to_idx(eh_mem_pool_t _pool, void* ptr){
    struct eh_mem_pool *pool = (struct eh_mem_pool *)_pool;
    size_t offset;
    if((char*)ptr < (char*)pool->base || (char*)ptr >= (char*)pool->base + pool->num*pool->align_size)
        return -1;
    offset = (size_t)((char*)ptr - (char*)pool->base);
    if(offset % pool->align_size != 0)
        return -1;
    return (int)(offset/pool->align_size);
}