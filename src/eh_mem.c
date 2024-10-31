/**
 * @file eh_mem.c
 * @brief 
 * @author simon.xiaoapeng (simon.xiaoapeng@gmail.com)
 * @version 1.0
 * @date 2024-07-03
 * 
 * @copyright Copyright (c) 2024  simon.xiaoapeng@gmail.com
 * 
 * @par 修改日志:
 */

#include <stdbool.h>
#include "eh.h"
#include "eh_debug.h"
#include "eh_error.h"
#include "eh_mem.h"
#include "eh_types.h"
#include "eh_user_config.h"
#include "eh_platform.h"

typedef unsigned long eh_size_t;

#if (defined(EH_CONFIG_USE_LIBC_MEM_MANAGE)) && (EH_CONFIG_USE_LIBC_MEM_MANAGE == 1)

#include <stdlib.h>
void* eh_malloc(size_t size){
    void *new;
    eh_save_state_t state;
    state = eh_enter_critical();
    new = malloc(size);
    eh_exit_critical(state);
    return new;
}
void  eh_free(void* ptr){
    eh_save_state_t state;
    state = eh_enter_critical();
    free(ptr);
    eh_exit_critical(state);
}

#else 


struct eh_mem_block {
    struct eh_mem_block*    next;
    eh_size_t               size;
};
#define EH_MEM_ALIGN_SIZE          ((eh_size_t)(EH_CONFIG_MEM_ALLOC_ALIGN))
#define EH_MEM_ALIGN_MASK           ((EH_MEM_ALIGN_SIZE) - 1)
#define EH_MEM_ALIGN_DOWN(addr)     (((eh_size_t)(addr)) & (~EH_MEM_ALIGN_MASK))
#define EH_MEM_ALIGN_UP(addr)       ((((eh_size_t)(addr)) + EH_MEM_ALIGN_MASK) & (~EH_MEM_ALIGN_MASK))
#define EH_MEM_BLOCK_HEAD_SIZE      EH_MEM_ALIGN_UP(sizeof(struct eh_mem_block))
#define EH_MEM_HEAP_ARRAY_NUM       (8U)



#if defined(EH_CONFIG_MEM_HEAP_SIZE) && (EH_CONFIG_MEM_HEAP_SIZE > 0)
eh_static_assert(EH_CONFIG_MEM_HEAP_SIZE > EH_MEM_BLOCK_HEAD_SIZE, "Please set EH_CONFIG_MEM_HEAP_SIZE to 0 or greater");

static uint8_t eh_aligned(EH_MEM_ALIGN_SIZE) mem_heap[EH_MEM_ALIGN_DOWN(EH_CONFIG_MEM_HEAP_SIZE)];
static struct eh_mem_heap mem_heap_array[EH_MEM_HEAP_ARRAY_NUM] = {
    {
        .heap_start = mem_heap,
        .heap_size = sizeof(mem_heap),
    },
};
static eh_size_t mem_heap_array_cnt = 1;
#else
static struct eh_mem_heap mem_heap_array[EH_MEM_HEAP_ARRAY_NUM];
static size_t mem_heap_array_cnt = 0;
#endif

/**
 *   此处使用结构体将内部变量放在结构体中，避免编译器优化时将 first_block 
 *   和 mem_heap_array优化为前后关系，这样会在插入时触发合并算法，
 *   会导致eh_malloc运行异常
 */
struct {
    struct eh_mem_block first_block;
    eh_size_t mem_total_size;
    eh_size_t mem_free_size;
    eh_size_t mem_min_ever_free_size_level;
}_eh_mem_run;

#define first_block                     (_eh_mem_run.first_block)
#define mem_total_size                  (_eh_mem_run.mem_total_size)
#define mem_free_size                   (_eh_mem_run.mem_free_size)
#define mem_min_ever_free_size_level    (_eh_mem_run.mem_min_ever_free_size_level)

static void eh_mem_insert(struct eh_mem_block *new_free_block){
    struct eh_mem_block *prev_block;
    struct eh_mem_block *pos_block;
    
    /* 循环寻找插入点 */
    for( prev_block = &first_block, pos_block = first_block.next; 
         pos_block && pos_block < new_free_block; 
         prev_block = pos_block, pos_block = pos_block->next ){
        
    }

    /* 加入链表 */
    prev_block->next = new_free_block;
    new_free_block->next = pos_block;
    mem_free_size += new_free_block->size;
    /* 
     * 尝试合并prev_block和new_free_block
     */
    if((uint8_t*)prev_block + prev_block->size + EH_MEM_BLOCK_HEAD_SIZE == (uint8_t*)new_free_block){
        prev_block->size += new_free_block->size + EH_MEM_BLOCK_HEAD_SIZE;
        prev_block->next = new_free_block->next;
        mem_free_size += EH_MEM_BLOCK_HEAD_SIZE;
        new_free_block = prev_block;
    }
    /*
     * 尝试合并new_free_block和pos_block
     */
    if((uint8_t*)new_free_block + new_free_block->size + EH_MEM_BLOCK_HEAD_SIZE == (uint8_t*)pos_block){
        new_free_block->size += pos_block->size + EH_MEM_BLOCK_HEAD_SIZE;
        new_free_block->next = pos_block->next;
        mem_free_size += EH_MEM_BLOCK_HEAD_SIZE;
    }
}

void eh_free_block_dump(void dump_func(void* start, size_t size)){
    eh_save_state_t state;
    struct eh_mem_block *pos_block;

    state = eh_enter_critical();
    for( pos_block = first_block.next; pos_block; pos_block = pos_block->next )
        dump_func(pos_block, pos_block->size + EH_MEM_BLOCK_HEAD_SIZE);
    eh_exit_critical(state);
}

void* eh_malloc(size_t _size){
    eh_save_state_t state;
    eh_size_t size = (eh_size_t)_size;
    eh_size_t align_size = EH_MEM_ALIGN_UP(size);

    struct eh_mem_block *new_block = NULL;
    struct eh_mem_block *new_free_block = NULL;
    eh_size_t new_free_block_size;
    void *new_mem = NULL;
    struct eh_mem_block *prev_block;
    struct eh_mem_block *pos_block;
    
    /* 如果size == 0, 或者向上对齐过的align_size比以前小，那么说明溢出了，要分配的内存太大了 */
    if(align_size == 0 || align_size < size) 
        return NULL;
    state = eh_enter_critical();
    /* 向上取整 */
    if(mem_free_size < align_size)
        goto out;
    /* 遍历查找合适的块 */
    for( prev_block = &first_block, pos_block = first_block.next; 
         pos_block && pos_block->size < align_size; 
         prev_block = pos_block, pos_block = pos_block->next ){
    }
    /* 没有找到，退出 */
    if(pos_block == NULL)
        goto out;
    new_block = pos_block;
    mem_free_size -= new_block->size;
    /* 是否具有能够拆分出一个空闲块 */
    new_free_block_size = new_block->size - align_size - EH_MEM_BLOCK_HEAD_SIZE;

    /* 
     * new_block->size > new_free_block_size 条件是防止向0溢出 
     */
    if(new_free_block_size > EH_MEM_ALIGN_SIZE && new_block->size > new_free_block_size){
        new_free_block = (struct eh_mem_block*)((uint8_t*)new_block + align_size + EH_MEM_BLOCK_HEAD_SIZE);
        new_free_block->size = new_free_block_size;
        new_free_block->next = new_block->next;
        new_block->size = align_size;
        new_block->next = new_free_block;
        
        mem_free_size += new_free_block_size;
    }
    if(mem_free_size < mem_min_ever_free_size_level)
        mem_min_ever_free_size_level = mem_free_size;
    new_mem = (void*)((uint8_t*)new_block + EH_MEM_BLOCK_HEAD_SIZE);
    prev_block->next = new_block->next;
out:
    eh_exit_critical(state);
    return new_mem;
}



void  eh_free(void* ptr){
    eh_save_state_t state;
    struct eh_mem_block *new_free_block = (struct eh_mem_block *)((uint8_t*)ptr - EH_MEM_BLOCK_HEAD_SIZE);
    if(ptr == NULL) return ;
    state = eh_enter_critical();
    eh_mem_insert(new_free_block);
    eh_exit_critical(state);
}


int eh_mem_heap_register(const struct eh_mem_heap *heap){
    eh_param_assert(heap);
    mem_heap_array[mem_heap_array_cnt].heap_start = heap->heap_start;
    mem_heap_array[mem_heap_array_cnt].heap_size = heap->heap_size;
    mem_heap_array_cnt++;
    return EH_RET_INVALID_STATE;
}


void eh_mem_get_heap_info(struct eh_mem_heap_info *heap_info){
    eh_save_state_t state;
    state = eh_enter_critical();
    heap_info->free_size = mem_free_size;
    heap_info->total_size = mem_total_size;
    heap_info->min_ever_free_size_level = mem_min_ever_free_size_level;
    eh_exit_critical(state);
}

static int __init eh_mem_init(void){
    struct eh_mem_block *block = NULL;
    uint8_t *end;
    first_block.next = NULL;
    first_block.size = 0;
    mem_total_size = 0;
    mem_free_size = 0;
    mem_min_ever_free_size_level = 0;
    eh_param_assert(mem_heap_array_cnt);
    for(eh_size_t i=0;i < mem_heap_array_cnt;i++){
        block = (struct eh_mem_block*)EH_MEM_ALIGN_UP(mem_heap_array[i].heap_start);
        end = (uint8_t*)EH_MEM_ALIGN_DOWN((uint8_t*)mem_heap_array[i].heap_start + mem_heap_array[i].heap_size);
        block->next = NULL;
        block->size = (eh_size_t)(end - (uint8_t*)block) - EH_MEM_BLOCK_HEAD_SIZE;
        eh_mem_insert(block);
    }
    mem_total_size = mem_free_size;
    mem_min_ever_free_size_level = mem_free_size;
    eh_infoln("Initializes the heap information:");
    eh_infoln("%11s\t%11s\t%11s\t%11s","total", "used" ,"free" ,"mefsl");
    eh_infoln("%11d\t%11d\t%11d\t%11d", mem_total_size, mem_total_size - mem_free_size, mem_free_size, mem_min_ever_free_size_level);
    return 0;
}

static void __exit eh_mem_exit(void){
    eh_infoln("Exits the heap information:");
    eh_infoln("%11s\t%11s\t%11s\t%11s","total", "used" ,"free" ,"mefsl");
    eh_infoln("%11d\t%11d\t%11d\t%11d", mem_total_size, mem_total_size - mem_free_size, mem_free_size, mem_min_ever_free_size_level);
    
}

eh_memory_module_export(eh_mem_init, eh_mem_exit);


#endif