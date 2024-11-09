/**
 * @file eh_mutex.h
 * @brief 虽然协程大部分情况下是不需要锁的
 *  但是，在宏观资源面前，还是存在加锁的，比如,
 *  在遍历链表时调用__async类型函数，在其他任务上就
 *  可能操作到该链表,导致链表被两个任务同时访问到，所以
 *  在非必要情况，是用不到本模块的。
 *  此模块所有的函数都只能在协程上下文中使用，
 *  也就是说，不要在中断上下文，或者其他线程（posix线程，系统线程）上下文中调用本函数
 * @author simon.xiaoapeng (simon.xiaoapeng@gmail.com)
 * @date 2024-06-22
 * 
 * @copyright Copyright (c) 2024  simon.xiaoapeng@gmail.com
 * 
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

/**
 * @brief                   创建一个互斥锁
 * @param  type             锁的类型
 * @return eh_mutex_t       应该使用eh_ptr_to_error来判断错误码，若成功应该为0，失败为负数
 */
extern eh_mutex_t eh_mutex_create(enum eh_mutex_type type);

/**
 * @brief                   释放互斥锁
 * @param  mutex            锁句柄
 */
extern void eh_mutex_destroy(eh_mutex_t mutex);

/**
 * @brief                   加锁
 * @param  mutex            锁句柄
 * @param  timeout          最多等待时间，EH_TIME_FOREVER永不超时
 * @return int          
 */
extern int __async eh_mutex_lock(eh_mutex_t mutex, eh_sclock_t timeout);
/**
 * @brief                   解锁
 * @param  mutex            锁句柄
 * @return int 
 */
extern int eh_mutex_unlock(eh_mutex_t mutex);


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif // _EH_MUTEX_H_
