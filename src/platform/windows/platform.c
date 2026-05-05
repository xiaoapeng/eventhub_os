/**
 * @file platform.c
 * @brief Windows 平台实现
 * @author simon.xiaoapeng (simon.xiaoapeng@gmail.com)
 * @date 2026-05-04
 *
 * @copyright Copyright (c) 2026  simon.xiaoapeng@gmail.com
 *
 */

#include <stdbool.h>
#include <windows.h>
#include <eh.h>
#include <eh_event.h>
#include <eh_timer.h>
#include <eh_platform.h>
#include <iocp_hub.h>

static struct {
    CRITICAL_SECTION    cs;
    bool                is_idle_state;
}win_platform;

static LARGE_INTEGER win_perf_freq;

eh_clock_t platform_get_clock_monotonic_time(void){
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    return (eh_clock_t)((counter.QuadPart * 1000000ULL) / win_perf_freq.QuadPart);
}

eh_save_state_t platform_enter_critical(void){
    EnterCriticalSection(&win_platform.cs);
    return 0;
}

void platform_exit_critical(eh_save_state_t state){
    (void)state;
    LeaveCriticalSection(&win_platform.cs);
}

void platform_idle_break(void){
    EnterCriticalSection(&win_platform.cs);
    if(win_platform.is_idle_state)
        iocp_hub_set_wait_break_event();
    LeaveCriticalSection(&win_platform.cs);
}

void platform_idle_or_extern_event_handler(void){
    eh_usec_t usec_timeout;

    EnterCriticalSection(&win_platform.cs);
    win_platform.is_idle_state = true;
    usec_timeout = eh_clock_to_usec((eh_clock_t)eh_get_loop_idle_time());
    iocp_hub_clean_wait_break_event();
    LeaveCriticalSection(&win_platform.cs);

    iocp_hub_poll(usec_timeout);

    win_platform.is_idle_state = false;
}

static int __init win_platform_init(void){
    int ret;
    ret = iocp_hub_init();
    if(ret < 0) return ret;
    win_platform.is_idle_state = false;
    QueryPerformanceFrequency(&win_perf_freq);
    InitializeCriticalSection(&win_platform.cs);
    return 0;
}

static void __exit win_platform_deinit(void){
    DeleteCriticalSection(&win_platform.cs);
    iocp_hub_exit();
}

eh_core_module_export(win_platform_init, win_platform_deinit);