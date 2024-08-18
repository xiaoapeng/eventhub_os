/**
 * @file eh_co.h
 * @brief 协程相关头文件
 * @author simon.xiaoapeng (simon.xiaoapeng@gmail.com)
 * @version 1.0
 * @date 2024-05-01
 * 
 * @copyright Copyright (c) 2024  simon.xiaoapeng@gmail.com
 * 
 * @par 修改日志:
 */

#ifndef _EH_CO_H_
#define _EH_CO_H_


#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */

#define __async
#define __await

typedef void* context_t;

extern void * co_context_swap(void *arg, context_t *from, const context_t * const to);
extern context_t co_context_make(void *stack_lim, void *stack_top, int (*func)(void *arg));

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif // _EH_CO_H_