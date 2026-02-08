/**
 * @file eh_atomic.h
 * @brief atomic operations
 * @author simon.xiaoapeng (simon.xiaoapeng@gmail.com)
 * @date 2026-02-06
 * 
 * @copyright Copyright (c) 2026  simon.xiaoapeng@gmail.com
 * 
 */

#ifndef _EH_ATOMIC_H_
#define _EH_ATOMIC_H_

#include <stdatomic.h>


typedef enum{
    eh_memory_order_relaxed = __ATOMIC_RELAXED,  /* 只保证操作本身是原子的 */
    eh_memory_order_consume = __ATOMIC_CONSUME,  /* 通过依赖关系来排序，确保在消费之前所有相关操作都完成 */
    eh_memory_order_acquire = __ATOMIC_ACQUIRE,  /* 资源获取时使用，确保之后的操作对其他线程可见 */
    eh_memory_order_release = __ATOMIC_RELEASE,  /* 资源释放时使用，确保之前的操作对其他线程可见 */
    eh_memory_order_acq_rel = __ATOMIC_ACQ_REL,  /* 资源获取时使用，确保之后的操作对其他线程可见，资源释放时使用，确保之前的操作对其他线程可见 */
    eh_memory_order_seq_cst = __ATOMIC_SEQ_CST   /* 最强的内存顺序，确保所有线程看到的操作顺序一致 */
}eh_memory_order;

/* 内存屏障 */
#define eh_compiler_barrier()                   __asm__ volatile("" : : : "memory")             /* 防止 编译器重新排 */
#define eh_memory_order_acquire_barrier()       atomic_thread_fence(memory_order_acquire)       /* 防止 loadload 重排和 loadstore 重排和 与相关load操作相关计算的重排 */
#define eh_memory_order_release_barrier()       atomic_thread_fence(memory_order_release)       /* 防止 loadstore 重排和 storestore 重排 */
#define eh_memory_order_acq_rel_barrier()       atomic_thread_fence(memory_order_acq_rel)       /* 防止 loadload、loadstore、storestore重排 */
#define eh_memory_order_seq_cst_barrier()       atomic_thread_fence(memory_order_seq_cst)       /* memory_order_acq_rel 的加强版本，插入缓冲同步指令，使所有CPU看到所有操作的顺序一致 */

/*原子变量读写*/
#define eh_atomic_init(PTR, VAL) atomic_init((PTR), (VAL))
#define eh_atomic_store_explicit(PTR, VAL, MO) atomic_store_explicit((PTR), (VAL), (MO))
#define eh_atomic_load_explicit(PTR, MO) atomic_load_explicit((PTR), (MO))
#define eh_atomic_exchange_explicit(PTR, VAL, MO) atomic_exchange_explicit((PTR), (VAL), (MO))
#define eh_atomic_compare_exchange_strong_explicit(PTR, VAL, DES, SUC, FAIL) \
    atomic_compare_exchange_strong_explicit((PTR), (VAL), (DES), (SUC), (FAIL))
#define eh_atomic_compare_exchange_strong(PTR, VAL, DES) \
    atomic_compare_exchange_strong((PTR), (VAL), (DES))
#define eh_atomic_compare_exchange_weak_explicit(PTR, VAL, DES, SUC, FAIL) \
    atomic_compare_exchange_weak_explicit((PTR), (VAL), (DES), (SUC), (FAIL))
#define eh_atomic_compare_exchange_weak(PTR, VAL, DES) \
    atomic_compare_exchange_weak((PTR), (VAL), (DES))
#define eh_atomic_fetch_add_explicit(PTR, VAL, MO) atomic_fetch_add_explicit((PTR), (VAL), (MO))
#define eh_atomic_fetch_sub_explicit(PTR, VAL, MO) atomic_fetch_sub_explicit((PTR), (VAL), (MO))
#define eh_atomic_fetch_or_explicit(PTR, VAL, MO) atomic_fetch_or_explicit((PTR), (VAL), (MO))
#define eh_atomic_fetch_xor_explicit(PTR, VAL, MO) atomic_fetch_xor_explicit((PTR), (VAL), (MO))
#define eh_atomic_fetch_and_explicit(PTR, VAL, MO) atomic_fetch_and_explicit((PTR), (VAL), (MO))





#endif // _EH_ATOMIC_H_