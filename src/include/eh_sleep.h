/**
 * @file eh_sleep.h
 * @brief 睡眠函数实现
 * @author simon.xiaoapeng (simon.xiaoapeng@gmail.com)
 * @version 1.0
 * @date 2024-06-22
 * 
 * @copyright Copyright (c) 2024  simon.xiaoapeng@gmail.com
 * 
 * @par 修改日志:
 */
#ifndef _EH_SLEEP_H_
#define _EH_SLEEP_H_


#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */

/**
 * @brief                   定时器事件的封装，在在指定协程上睡眠 usec 微秒
 * @param  usec             微秒
 */
extern void __async__ eh_usleep(eh_usec_t usec);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif // _EH_SLEEP_H_