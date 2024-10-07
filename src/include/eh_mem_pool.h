/**
 * @file eh_mem_pool.h
 * @brief 内存池功能实现
 * @author simon.xiaoapeng (simon.xiaoapeng@gmail.com)
 * @version 1.0
 * @date 2024-10-06
 * 
 * @copyright Copyright (c) 2024  simon.xiaoapeng@gmail.com
 * 
 * @par 修改日志:
 */

#ifndef _EH_MEM_POOL_H_
#define _EH_MEM_POOL_H_

#include <stddef.h>

#include "eh_types.h"


typedef int* eh_mem_pool_t;

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */

/**
 * @brief                   内存池创建
 * @param  align            内存对齐字节数
 * @param  size             每一项内存块的大小
 * @param  num              内存块数量
 * @return eh_mem_pool_t    应该使用eh_ptr_to_error来判断错误码，若成功应该为0，失败为负数
 */
extern  eh_mem_pool_t eh_mem_pool_create(size_t align, size_t size, size_t num);

/**
 * @brief                   销毁内存池
 * @param  pool             内存池句柄
 */
extern  void  eh_mem_pool_destroy(eh_mem_pool_t pool);

/**
 * @brief                   从内存池分配一项内存块
 * @param  pool             内存池句柄
 * @return void* 
 */
extern  void* eh_mem_pool_alloc(eh_mem_pool_t pool);

/**
 * @brief                   回收一块内存块
 * @param  pool             内存池句柄
 * @param  ptr              内存块指针
 */
extern  void  eh_mem_pool_free(eh_mem_pool_t pool, void* ptr);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif // _EH_MEM_POOL_H_