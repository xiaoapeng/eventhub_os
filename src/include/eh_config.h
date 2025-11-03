/**
 * @file event_config.h
 * @brief 配置文件
 * @author simon.xiaoapeng (simon.xiaoapeng@gmail.com)
 * @date 2024-04-14
 * 
 * @copyright Copyright (c) 2024  simon.xiaoapeng@gmail.com
 * 
 */
#ifndef _EVENT_CONFIG_H_
#define _EVENT_CONFIG_H_

#if __has_include("eh_user_config.h")
#include "eh_user_config.h"
#endif

/**
 *  platform_get_clock_monotonic_time 函数获取到的时钟使用的时钟频率
 */
#ifndef EH_CONFIG_CLOCKS_PER_SEC
extern unsigned long platform_get_clock_freq(void);
#define EH_CONFIG_CLOCKS_PER_SEC                                (platform_get_clock_freq())
#endif /* EH_CONFIG_CLOCKS_PER_SEC */

#ifdef CONFIG_EH_CONFIG_CLOCKS_PER_SEC
#undef EH_CONFIG_CLOCKS_PER_SEC
#define EH_CONFIG_CLOCKS_PER_SEC                                CONFIG_EH_CONFIG_CLOCKS_PER_SEC
#endif

#if defined(EH_SYSTEM_IS_LINUX)
#undef EH_CONFIG_CLOCKS_PER_SEC
#define EH_CONFIG_CLOCKS_PER_SEC                                (1000000U)
#endif


/**
 *  EH_CONFIG_USE_LIBC_MEM_MANAGE为1时,将使用libc库的内存管理,未定义
 *   或者定义为0时使用eventhub_os的内存管理,使用libc时请务必实现：
 *    caddr_t _sbrk(int incr); 否则由于协程的特殊性(协程栈在堆上实现)，
 *    会使C库误以为发生堆栈碰撞而malloc失败。
 *  EH_CONFIG_MEM_ALLOC_ALIGN为分配空间的对齐粒度，为2的幂，这里默认为系统指针大小的2倍
 *  EH_CONFIG_MEM_HEAP_SIZE为堆内存大小，默认为20K
 */
#ifndef EH_CONFIG_USE_LIBC_MEM_MANAGE
#if defined(EH_SYSTEM_IS_LINUX)
#define EH_CONFIG_USE_LIBC_MEM_MANAGE                            1
#else
#define EH_CONFIG_USE_LIBC_MEM_MANAGE                            0
#endif
#endif /* EH_CONFIG_USE_LIBC_MEM_MANAGE */

#ifdef CONFIG_EH_CONFIG_USE_LIBC_MEM_MANAGE
#undef EH_CONFIG_USE_LIBC_MEM_MANAGE
#define EH_CONFIG_USE_LIBC_MEM_MANAGE                            CONFIG_EH_CONFIG_USE_LIBC_MEM_MANAGE
#endif

#if (!defined(EH_CONFIG_USE_LIBC_MEM_MANAGE)) || (EH_CONFIG_USE_LIBC_MEM_MANAGE == 0)

#  ifndef EH_CONFIG_MEM_ALLOC_ALIGN
#   define EH_CONFIG_MEM_ALLOC_ALIGN                             (sizeof(void*)*2)
#endif /* EH_CONFIG_MEM_ALLOC_ALIGN */

#  ifdef CONFIG_EH_CONFIG_MEM_ALLOC_ALIGN
#   undef EH_CONFIG_MEM_ALLOC_ALIGN
#   define EH_CONFIG_MEM_ALLOC_ALIGN                             CONFIG_EH_CONFIG_MEM_ALLOC_ALIGN
#  endif

#  ifndef EH_CONFIG_MEM_HEAP_SIZE
#   define EH_CONFIG_MEM_HEAP_SIZE                               (8*1024U)
#  endif /* EH_CONFIG_MEM_HEAP_SIZE */
#endif

#  ifdef CONFIG_EH_CONFIG_MEM_HEAP_SIZE
#   undef EH_CONFIG_MEM_HEAP_SIZE
#   define EH_CONFIG_MEM_HEAP_SIZE                               CONFIG_EH_CONFIG_MEM_HEAP_SIZE
#  endif

/**
 *  配置标准输出缓存大小,该缓冲为单次输出的最大字节数，并不限制eh_printf的输出字节数
 */
#ifndef EH_CONFIG_STDOUT_MEM_CACHE_SIZE
#define EH_CONFIG_STDOUT_MEM_CACHE_SIZE                          (32U)
#endif /* EH_CONFIG_STDOUT_MEM_CACHE_SIZE */

#ifdef CONFIG_EH_CONFIG_STDOUT_MEM_CACHE_SIZE
#undef EH_CONFIG_STDOUT_MEM_CACHE_SIZE
#define EH_CONFIG_STDOUT_MEM_CACHE_SIZE                          CONFIG_EH_CONFIG_STDOUT_MEM_CACHE_SIZE
#endif

/**
 *  配置系统默认的BUG等级
 */
#ifndef EH_CONFIG_DEFAULT_DEBUG_LEVEL
#define EH_CONFIG_DEFAULT_DEBUG_LEVEL                            EH_DBG_DEBUG
#endif  /* EH_CONFIG_DEFAULT_DEBUG_LEVEL */

#ifdef CONFIG_EH_CONFIG_DEFAULT_DEBUG_LEVEL
#undef EH_CONFIG_DEFAULT_DEBUG_LEVEL
#define EH_CONFIG_DEFAULT_DEBUG_LEVEL                            CONFIG_EH_CONFIG_DEFAULT_DEBUG_LEVEL
#endif

/** 
 *  DEBUG时的回车符号，当未定义时会根据系统类型自动判断
 */
#ifndef EH_CONFIG_DEBUG_ENTER_SIGN
#define EH_CONFIG_DEBUG_ENTER_SIGN                               "\n"
#endif /* EH_CONFIG_DEBUG_ENTER_SIGN */

#ifdef CONFIG_EH_CONFIG_DEBUG_ENTER_SIGN
#undef EH_CONFIG_DEBUG_ENTER_SIGN
#define EH_CONFIG_DEBUG_ENTER_SIGN                               CONFIG_EH_CONFIG_DEBUG_ENTER_SIGN
#endif

#if defined(EH_SYSTEM_IS_LINUX)
#undef EH_CONFIG_DEBUG_ENTER_SIGN
#define EH_CONFIG_DEBUG_ENTER_SIGN                               "\n"
#endif


/**
 *  DEBUG打印的TAG配置
 */
#ifndef EH_CONFIG_DEBUG_FLAGS
#define EH_CONFIG_DEBUG_FLAGS                                    (EH_DBG_FLAGS_DEBUG_TAG|EH_DBG_FLAGS_MONOTONIC_CLOCK)
#endif /* EH_CONFIG_DEBUG_FLAGS */

#ifdef CONFIG_EH_CONFIG_DEBUG_FLAGS
#undef EH_CONFIG_DEBUG_FLAGS
#define EH_CONFIG_DEBUG_FLAGS                                    CONFIG_EH_CONFIG_DEBUG_FLAGS
#endif

/**
 *  中断栈的大小
 *  该栈仅仅在单片机时才能使用到，默认使用1024即可
 */
#ifndef EH_CONFIG_INTERRUPT_STACK_SIZE
#define EH_CONFIG_INTERRUPT_STACK_SIZE                          1024
#endif /* EH_CONFIG_INTERRUPT_STACK_SIZE */

#ifdef CONFIG_EH_CONFIG_INTERRUPT_STACK_SIZE
#undef EH_CONFIG_INTERRUPT_STACK_SIZE
#define EH_CONFIG_INTERRUPT_STACK_SIZE                          CONFIG_EH_CONFIG_INTERRUPT_STACK_SIZE
#endif

/**
 *  配置任务调度多少次后进行一次轮询
 */
#ifndef EH_CONFIG_TASK_DISPATCH_CNT_PER_POLL
#define EH_CONFIG_TASK_DISPATCH_CNT_PER_POLL                    4
#endif /* EH_CONFIG_TASK_DISPATCH_CNT_PER_POLL */

/*
 *  哈希表初始表大小
 */
#ifndef EH_CONFIG_HASHTBL_MIN_SIZE
#define EH_CONFIG_HASHTBL_MIN_SIZE                              16
#endif /* EH_CONFIG_HASHTBL_MIN_SIZE */

#ifndef EH_CONFIG_EVENT_CB_DISPATCH_CNT_PER_YIELD
#define EH_CONFIG_EVENT_CB_DISPATCH_CNT_PER_YIELD               4
#endif

#ifndef EH_CONFIG_EVENT_CB_DISPATCH_CNT_PER_CHECKTIMER
#define EH_CONFIG_EVENT_CB_DISPATCH_CNT_PER_CHECKTIMER          4
#endif

#endif // _EVENT_CONFIG_H_