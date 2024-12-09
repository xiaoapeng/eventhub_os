/**
 * @file test_eh.c
 * @brief 
 * @author simon.xiaoapeng (simon.xiaoapeng@gmail.com)
 * @date 2024-05-12
 * 
 * @copyright Copyright (c) 2024  simon.xiaoapeng@gmail.com
 * 
 */


#include <stdio.h>
#include <eh.h>
#include <eh_debug.h>
#include <eh_event.h>
#include <eh_platform.h>
#include <eh_timer.h> 
#include <eh_types.h>
#include <stdlib.h>
#include <sys/epoll.h>



void stdout_write(void *stream, const uint8_t *buf, size_t size){
    (void)stream;
    printf("%.*s", (int)size, (const char*)buf);
}


int task_test_2(void *arg){
    eh_event_timer_t timer;
    eh_debugfl("%s", arg);
    eh_timer_init(&timer);
    eh_timer_set_attr(&timer, EH_TIMER_ATTR_AUTO_CIRCULATION);
    eh_timer_config_interval(&timer, (eh_sclock_t)eh_msec_to_clock(1000));
    eh_timer_start(&timer);
    for(int i=0; i<10; i++){
        __await eh_event_wait_timeout(eh_timer_to_event(&timer), EH_TIME_FOREVER);
        eh_debugfl("1000ms wakeup %lld", eh_get_clock_monotonic_time());
    }
    eh_timer_stop(&timer);
    eh_debugfl("return");
    return 2;
}

int task_test_1(void *arg){
    eh_event_timer_t timer;
    eh_debugfl("%s", arg);
    eh_timer_init(&timer);
    eh_timer_set_attr(&timer, EH_TIMER_ATTR_AUTO_CIRCULATION);
    eh_timer_config_interval(&timer, (eh_sclock_t)eh_msec_to_clock(500));
    eh_timer_start(&timer);
    for(int i=0;i<10;i++){
        __await eh_event_wait_timeout(eh_timer_to_event(&timer), EH_TIME_FOREVER);
        eh_debugfl("500ms wakeup %lld", eh_get_clock_monotonic_time());
    }
    eh_timer_stop(&timer);
    eh_debugfl("return");
    return 1;
}

int task_app(void *arg){
    eh_task_t *test_1,*test_2;
    int app_ret;
    int ret;

    eh_debugfl("%s", arg);
    test_1 = eh_task_create("test_1", 0, 12*1024, "1", task_test_1);
    test_2 = eh_task_create("test_2", 0, 12*1024, "2", task_test_2);
    ret = __await eh_task_join(test_1, &app_ret, EH_TIME_FOREVER);
    eh_debugfl("test_1: ret=%d app_ret=%d", ret, app_ret);
    ret = __await eh_task_join(test_2, &app_ret, EH_TIME_FOREVER);
    eh_debugfl("test_2: ret=%d app_ret=%d", ret, app_ret);
    return 0;
}


int main(void){
    eh_debugfl("test_eh start!!");
    eh_global_init();
    task_app("task_app");
    eh_global_exit();
    return 0;
}