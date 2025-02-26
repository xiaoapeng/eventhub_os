/**
 * @file eh_types.h
 * @brief 
 * @author simon.xiaoapeng (simon.xiaoapeng@gmail.com)
 * @date 2024-05-12
 * 
 * @copyright Copyright (c) 2024  simon.xiaoapeng@gmail.com
 * 
 */

#ifndef _EH_TYPES_H_
#define _EH_TYPES_H_

#include <stdatomic.h>

#ifndef EH_SECTION
    #if defined(__CC_ARM) || defined(__CLANG_ARM)
        #define EH_SECTION(__section__)                __attribute__((section(__section__)))
    #elif defined (__IAR_SYSTEMS_ICC__)
        #define EH_SECTION(__section__)                @ __section__
    #elif defined(__GNUC__)
        #define EH_SECTION(__section__)                __attribute__((section(__section__)))
    #else
        #define EH_SECTION(__section__)
    #endif
#endif

#ifndef EH_USED
    #if defined(__CC_ARM) || defined(__CLANG_ARM)
        #define EH_USED                                 __attribute__((used))
    #elif defined (__IAR_SYSTEMS_ICC__)
        #define EH_USED                                 __root
    #elif defined(__GNUC__)
        #define EH_USED                                 __attribute__((used))
    #else
        #define EH_USED
    #endif
#endif


#define EH_STRINGIFY(x) #x

#define EH_ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

/* 内存屏障 */
#define eh_compiler_barrier()                   __asm__ volatile("" : : : "memory")             /* 防止 编译器重新排 */
#define eh_memory_order_consume_barrier()       atomic_thread_fence(memory_order_consume)       /* 防止 loadstore重排和loadstore重排 */
#define eh_memory_order_acquire_barrier()       atomic_thread_fence(memory_order_acquire)       /* 防止 loadstore重排和loadstore重排和 与相关load操作相关计算的重排 */
#define eh_memory_order_release_barrier()       atomic_thread_fence(memory_order_release)       /* 防止 loadstore重排和storestore重排 */
#define eh_memory_order_acq_rel_barrier()       atomic_thread_fence(memory_order_acq_rel)       /* 防止 loadload、loadstore、storestore重排 */
#define eh_memory_order_seq_cst_barrier()       atomic_thread_fence(memory_order_seq_cst)       /* 防止 loadload、loadstore、storestore重排 与相关load、store操作相关计算的重排 和缓存同步 */


#define eh_offsetof(TYPE, MEMBER)               __builtin_offsetof(TYPE, MEMBER)
#define eh_same_type(a, b)                      __builtin_types_compatible_p(typeof(a), typeof(b))
#define eh_static_assert(expr, msg)             _Static_assert(expr, msg)
#define eh_likely(x)                            __builtin_expect(!!(x), 1)
#define eh_unlikely(x)                          __builtin_expect(!!(x), 0)
#define eh_container_of(ptr, type, member) ({                                       \
    void *__mptr = (void *)(ptr);                                                   \
    eh_static_assert(eh_same_type(*(ptr), ((type *)0)->member) ||                   \
              eh_same_type(*(ptr), void),                                           \
              "pointer type mismatch in eh_container_of()");                        \
    ((type *)(__mptr - eh_offsetof(type, member))); })

#define eh_container_of_const(ptr, type, member)                                    \
    _Generic(ptr,                                                                   \
        const typeof(*(ptr)) *: ((const type *)eh_container_of(ptr, type, member)), \
        default: ((type *)eh_container_of(ptr, type, member))                       \
    )

#define eh_container_of_safe(ptr, type, member)                                     \
    ({ typeof(ptr) ____ptr = (ptr);                                                 \
       ____ptr ? eh_container_of(____ptr, type, member) : NULL;                     \
    })

#define eh_member_address_is_nonnull(ptr, member)	                                \
	((uintptr_t)(ptr) + offsetof(typeof(*(ptr)), member) != 0)


#define eh_aligned(x)                           __attribute__((aligned(x)))
#define eh_align_up(x, align)                   (((x) + ((align) - 1)) & (~((align) - 1)))
#define eh_align_down(x, align)                 ((x) & (~((align) - 1)))
#define eh_read_once(x)                         (*(const volatile typeof(x) *)&(x))

#define __weak                                  __attribute__((weak))
#define __safety                                /* 被此宏标记的函数，可在中断和其他线程中进行调用 */
#define __noreturn                              __attribute__((noreturn))

#ifndef __packed
#define __packed                                __attribute__((packed))
#endif

#define __function_const                        __attribute__((__const__))

#endif // _EH_TYPES_H_