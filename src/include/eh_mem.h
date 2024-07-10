/**
 * @file eh_mem.h
 * @brief 内存分配实现
 * @author simon.xiaoapeng (simon.xiaoapeng@gmail.com)
 * @version 1.0
 * @date 2024-07-03
 * 
 * @copyright Copyright (c) 2024  simon.xiaoapeng@gmail.com
 * 
 * @par 修改日志:
 */
#ifndef _EH_MEM_H_
#define _EH_MEM_H_

#include <stddef.h>

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */

struct eh_mem_heap {
    void  *heap_start;
    size_t heap_size;
};

struct eh_mem_heap_info{
    size_t total_size;
    size_t free_size;
    size_t min_ever_free_size_level;
};

extern void* eh_malloc(size_t size);
extern void  eh_free(void* ptr);

/**
 * @brief 
 * @param  heap            注册堆空间用于内存分配
 * @return int             0: 成功, -1: 失败
 */
extern int eh_mem_heap_register(const struct eh_mem_heap *heap);

/**
 * @brief                   获取堆信息
 * @param  heap_info        
 */
extern void eh_mem_get_heap_info(struct eh_mem_heap_info *heap_info);


/**
 * @brief                   遍历可用内存块
 * @param  dump_func        dump_func内禁止任何形式的await
 */
void eh_free_block_dump(void dump_func(void* start, size_t size));

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif // _EH_MEM_H_