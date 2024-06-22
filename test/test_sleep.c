/**
 * @file test_sleep.c
 * @brief 
 * @author simon.xiaoapeng (simon.xiaoapeng@gmail.com)
 * @version 1.0
 * @date 2024-05-12
 * 
 * @copyright Copyright (c) 2024  simon.xiaoapeng@gmail.com
 * 
 * @par 修改日志:
 */


#include <stdlib.h>
#include "eh.h"
#include "eh_event.h"
#include "eh_timer.h" 
#include "eh_sleep.h"

#include "debug.h"
int task_test_2(void *arg){
    (void) arg;
    for(int i=0; i < 20; i++){
        dbg_debugfl("debug!! %llu", eh_clock_to_usec(eh_get_clock_monotonic_time()));
        eh_usleep(1000*500);
    }
    return 2;
}

int task_test_1(void *arg){
    (void) arg;
    for(int i=0; i < 10; i++){
        dbg_debugfl("debug!! %llu", eh_clock_to_usec(eh_get_clock_monotonic_time()));
        eh_usleep(1000*1000);
    }
    return 1;
}

int task_app(void *arg){
    eh_task_t *test_1,*test_2;
    int app_ret;
    int ret;

    dbg_debugfl("%s", arg);
    test_1 = eh_task_create("test_1", 12*1024, "1", task_test_1);
    test_2 = eh_task_create("test_2", 12*1024, "2", task_test_2);
    ret = eh_task_join(test_1, &app_ret, EH_TIME_FOREVER);
    dbg_debugfl("test_1: ret=%d app_ret=%d", ret, app_ret);
    ret = eh_task_join(test_2, &app_ret, EH_TIME_FOREVER);
    dbg_debugfl("test_2: ret=%d app_ret=%d", ret, app_ret);
    eh_loop_exit(0);
    return 0;
}


int main(void){
    debug_init();
    dbg_debugfl("test_eh start!!");
    eh_global_init();
    eh_task_create("task_app", 12*1024, "task_app", task_app);
    eh_loop_run();
    eh_global_exit();

    return 0;
}