/**
 * @file eh_sleep.c
 * @brief 阻塞延时实现
 * @author simon.xiaoapeng (simon.xiaoapeng@gmail.com)
 * @version 1.0
 * @date 2024-06-16
 * 
 * @copyright Copyright (c) 2024  simon.xiaoapeng@gmail.com
 * 
 * @par 修改日志:
 */

#include "eh.h"
#include "eh_event.h"
#include "eh_timer.h"
void __async__ eh_usleep(eh_usec_t usec){
    eh_timer_event_t sleep_timer;
    if(usec == 0) return ;
    eh_timer_init(&sleep_timer);
    eh_timer_set_attr(&sleep_timer, 0);
    eh_timer_config_interval(&sleep_timer, (eh_sclock_t)eh_usec_to_clock(usec));
    eh_timer_start(&sleep_timer);
    __await__ eh_event_wait_timeout(eh_timer_to_event(&sleep_timer), EH_TIME_FOREVER);
    eh_timer_clean(&sleep_timer);
}
