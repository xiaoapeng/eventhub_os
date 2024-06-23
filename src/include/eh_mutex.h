/**
 * @file eh_mutex.h
* @brief 任务互斥锁的实现，虽然协程大部分情况下是不需要锁的，
 *    但是，在宏观资源面前，还是存在加锁的，比如，
 *    在遍历链表时调用 __await类型函数，在其他任务上就
 *    可能操作到该链表导致链表指针无效出现段错误，所以
 *    在非必要情况，是用不到本模块的。
 *      使用限制: 此模块所有的函数都只能在协程上下文中使用，
 *      也就是说，不要在中断上下文，或者其他线程上下文中调用本函数
 * @author simon.xiaoapeng (simon.xiaoapeng@gmail.com)
 * @version 1.0
 * @date 2024-06-22
 * 
 * @copyright Copyright (c) 2024  simon.xiaoapeng@gmail.com
 * 
 * @par 修改日志:
 */


#ifndef _EH_MUTEX_H_
#define _EH_MUTEX_H_

typedef int* eh_mutex_t;

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */

enum eh_mutex_type {
    EH_MUTEX_TYPE_NORMAL,
    EH_MUTEX_TYPE_RECURSIVE,
    EH_MUTEX_TYPE_MAX
};


extern eh_mutex_t eh_mutex_create(enum eh_mutex_type type);
extern void eh_mutex_destroy(eh_mutex_t mutex);
extern int __async__ eh_mutex_lock(eh_mutex_t mutex, eh_sclock_t timeout);
extern int eh_mutex_unlock(eh_mutex_t mutex);


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif // _EH_MUTEX_H_
