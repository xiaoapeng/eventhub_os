/**
 * @file platform_port.h
 * @brief Windows 平台端口实现
 * @author simon.xiaoapeng (simon.xiaoapeng@gmail.com)
 * @date 2026-05-04
 *
 * @copyright Copyright (c) 2026  simon.xiaoapeng@gmail.com
 *
 */

#ifndef _PLATFORM_PORT_H_
#define _PLATFORM_PORT_H_


#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */


extern eh_clock_t  platform_get_clock_monotonic_time(void);
extern eh_save_state_t  platform_enter_critical(void);
extern void  platform_exit_critical(eh_save_state_t state);
extern void  platform_idle_break(void);
extern void  platform_idle_or_extern_event_handler(void);


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif // _PLATFORM_PORT_H_
