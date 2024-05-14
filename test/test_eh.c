/**
 * @file test_eh.c
 * @brief 
 * @author simon.xiaoapeng (simon.xiaoapeng@gmail.com)
 * @version 1.0
 * @date 2024-05-12
 * 
 * @copyright Copyright (c) 2024  simon.xiaoapeng@gmail.com
 * 
 * @par 修改日志:
 */

#include "debug.h"
#include "eh.h"
#include "eh_timer.h"



int task_test_2(void *arg){
    eh_timer_event_t timer;
    dbg_debugfl("%s", arg);
    eh_timer_init(&timer);
    eh_timer_set_attr(&timer, EH_TIMER_ATTR_AUTO_CIRCULATION);
    eh_timer_config_interval(&timer, 1000*1000);
    eh_timer_start(&timer);
    while(1){
        __await__ eh_event_wait_timeout(eh_timer_to_event(&timer), EH_TIMER_FOREVER);
        dbg_debugfl("hhh");
    }
}

int task_test_1(void *arg){
    eh_timer_event_t timer;
    dbg_debugfl("%s", arg);
    eh_timer_init(&timer);
    eh_timer_set_attr(&timer, EH_TIMER_ATTR_AUTO_CIRCULATION);
    eh_timer_config_interval(&timer, 500*1000);
    eh_timer_start(&timer);
    while(1){
        __await__ eh_event_wait_timeout(eh_timer_to_event(&timer), EH_TIMER_FOREVER);
        dbg_debugfl("hhhzz");
    }
    
}

int main(void){
    debug_init();

    eh_global_init();
    eh_create_task("test_1", 12*1024, "1", task_test_1);
    eh_create_task("test_1", 12*1024, "2", task_test_2);

    eh_loop_run();
    
    return 0;
}