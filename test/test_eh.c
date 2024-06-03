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
#include "eh_types.h"
#include <stdlib.h>
#include <sys/epoll.h>
#include <valgrind/valgrind.h>
int task_test_2(void *arg){
    eh_timer_event_t timer;
    dbg_debugfl("%s", arg);
    eh_timer_init(&timer);
    eh_timer_set_attr(&timer, EH_TIMER_ATTR_AUTO_CIRCULATION);
    eh_timer_config_interval(&timer, (eh_sclock_t)eh_msec_to_clock(1000));
    eh_timer_start(&timer);
    for(int i=0; i<10; i++){
        __await__ eh_event_wait_timeout(eh_timer_to_event(&timer), EH_TIMER_FOREVER);
        dbg_debugfl("1000ms wakeup");
    }
    eh_timer_stop(&timer);
    dbg_debugfl("return");
    return 2;
}

int task_test_1(void *arg){
    eh_timer_event_t timer;
    dbg_debugfl("%s", arg);
    eh_timer_init(&timer);
    eh_timer_set_attr(&timer, EH_TIMER_ATTR_AUTO_CIRCULATION);
    eh_timer_config_interval(&timer, (eh_sclock_t)eh_msec_to_clock(500));
    eh_timer_start(&timer);
    for(int i=0;i<10;i++){
        __await__ eh_event_wait_timeout(eh_timer_to_event(&timer), EH_TIMER_FOREVER);
        dbg_debugfl("500ms wakeup");
    }
    eh_timer_stop(&timer);
    dbg_debugfl("return");
    return 1;
}

int task_app(void *arg){
    eh_task_t *test_1,*test_2;
    int app_ret;
    int ret;

    dbg_debugfl("%s", arg);
    test_1 = eh_task_create("test_1", 12*1024, "1", task_test_1);
    test_2 = eh_task_create("test_2", 12*1024, "2", task_test_2);
    ret = eh_task_join(test_1, &app_ret, EH_TIMER_FOREVER);
    dbg_debugfl("test_1: ret=%d app_ret=%d", ret, app_ret);
    ret = eh_task_join(test_2, &app_ret, EH_TIMER_FOREVER);
    dbg_debugfl("test_2: ret=%d app_ret=%d", ret, app_ret);
    eh_loop_exit(0);
    return 0;
}


int main(void){
    debug_init();

    eh_global_init();
    eh_task_create("task_app", 12*1024, "task_app", task_app);
    eh_loop_run();
    eh_global_exit();
    
    return 0;
}