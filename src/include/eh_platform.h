/**
 * @file eh_platform.h
 * @brief 给底层平台提供的相关接口
 * @author simon.xiaoapeng (simon.xiaoapeng@gmail.com)
 * @date 2024-06-07
 * 
 * @copyright Copyright (c) 2024  simon.xiaoapeng@gmail.com
 * 
 */
#ifndef _EH_PLATFORM_H_
#define _EH_PLATFORM_H_

typedef unsigned long eh_save_state_t ;
#include "platform_port.h"
#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */

/**
 * @brief                  获取系统的单调时钟数,由平台方实现，精度越高越好
 */
#define eh_get_clock_monotonic_time()               platform_get_clock_monotonic_time()


/**
 * @brief               进入临界区
 */
#define eh_enter_critical()                         platform_enter_critical()

/**
 * @brief               解锁
 * @param   state       退出临界区
 */
#define eh_exit_critical(state)                     platform_exit_critical(state)

/**
 * @brief               打断空闲状态
 */
#define eh_idle_break()                              platform_idle_break()

/**
 * @brief               空闲或外部事件处理函数
 */
#define eh_idle_or_extern_event_handler()           platform_idle_or_extern_event_handler()


/**
 * @brief                  获取当前最大的空闲时钟数
 * @return eh_sclock_t     返回能空闲的时钟数
 */
extern eh_sclock_t eh_get_loop_idle_time(void);







#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif // _EH_PLATFORM_H_