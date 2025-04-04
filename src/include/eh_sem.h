/**
 * @file eh_sem.h
 * @brief 任务信号量的实现，虽然事件也提供了类似与休眠唤醒的功能，
 *  但是事件只能保证多次set必有一次触发，无法保证多次触发，
 *  而信号量的实现能增加可靠性, 信号量的实现继承了event的相关特性，
 *  也能使用event相关特性，比如epoll
 * @author simon.xiaoapeng (simon.xiaoapeng@gmail.com)
 * @date 2024-06-23
 * 
 * @copyright Copyright (c) 2024  simon.xiaoapeng@gmail.com
 * 
 */

#ifndef _EH_SEM_H_
#define _EH_SEM_H_

#include <eh_types.h>

typedef int* eh_sem_t;

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */

/**
 * @brief 获取信号量的事件句柄 
 * @param sem 
 * @return  eh_event_t*
 */
#define eh_sem_get_event(sem)   ((eh_event_t*)sem)

/**
 * @brief                   创建一个信号量
 * @param  value            信号量原生信号数
 * @return eh_sem_t         返回信号量句柄
 */
extern __safety eh_sem_t eh_sem_create(uint32_t value);

/**
 * @brief                   销毁信号量
 * @param  sem              信号量句柄
 * @return 
 */
extern __safety void eh_sem_destroy(eh_sem_t sem);

/**
 * @brief                   等待信号量
 * @param  sem              信号量句柄
 * @param  timeout          最多等待时间，EH_TIME_FOREVER永不超时
 * @return int              等待结果，0表示成功，其他表示失败
 */
extern  __async int eh_sem_wait(eh_sem_t sem, eh_sclock_t timeout);

/**
 * @brief                   唤醒信号量
 * @param  sem              信号量句柄
 * @return int              唤醒结果，0表示成功，其他表示失败
 */
extern __safety int eh_sem_post(eh_sem_t sem);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif // _EH_SEM_H_