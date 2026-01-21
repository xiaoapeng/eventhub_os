/**
 * @file eh_comp_timer.h
 * @brief eventhub timer signal component
 * @author simon.xiaoapeng (simon.xiaoapeng@gmail.com)
 * @date 2026-01-21
 * 
 * @copyright Copyright (c) 2026  simon.xiaoapeng@gmail.com
 * 
 */
#ifndef _EH_COMP_TIMER_H_
#define _EH_COMP_TIMER_H_

#include <eh.h>
#include <eh_signal.h>
#include <eh_timer.h>
#include <autoconf.h>

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */

#ifdef CONFIG_PACKAGE_EVENTHUB_GLOBAL_TIMER_SIGNAL_1S
EH_EXTERN_CUSTOM_SIGNAL(signal_eh_comp_timer_1s, eh_event_timer_t);
#endif

#ifdef CONFIG_PACKAGE_EVENTHUB_GLOBAL_TIMER_SIGNAL_500MS
EH_EXTERN_CUSTOM_SIGNAL(signal_eh_comp_timer_500ms, eh_event_timer_t);
#endif

#ifdef CONFIG_PACKAGE_EVENTHUB_GLOBAL_TIMER_SIGNAL_100MS
EH_EXTERN_CUSTOM_SIGNAL(signal_eh_comp_timer_100ms, eh_event_timer_t);
#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif // _EH_COMP_TIMER_H_