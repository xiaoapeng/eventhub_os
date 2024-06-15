/**
 * @file eh_platform.h
 * @brief 给底层平台提供的相关接口
 * @author simon.xiaoapeng (simon.xiaoapeng@gmail.com)
 * @version 1.0
 * @date 2024-06-07
 * 
 * @copyright Copyright (c) 2024  simon.xiaoapeng@gmail.com
 * 
 * @par 修改日志:
 */
#ifndef _EH_PLATFORM_H_
#define _EH_PLATFORM_H_


#include "eh_interior.h"
#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */


/**
 * @brief                  获取当前最大的空闲时钟数
 * @return eh_sclock_t     返回能空闲的时钟数
 */
static inline eh_sclock_t eh_get_loop_idle_time(void){
    uint32_t state;
    eh_sclock_t half_time;
    eh_lock(&state);
    half_time = !eh_list_empty(&eh_task_get_current()->task_list_node) ? 
        0 : eh_timer_get_first_remaining_time_on_lock();
    eh_unlock(state);
    return half_time;
}

/**
 * @brief                   注册平台端口参数
 * @param  param            平台相关接口
 * @return int 
 */
extern int eh_platform_register_port_param(const eh_platform_port_param_t *param);





#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif // _EH_PLATFORM_H_