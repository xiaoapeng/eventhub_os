/**
 * @file test_yield.c
 * @brief 
 * @author simon.xiaoapeng (simon.xiaoapeng@gmail.com)
 * @version 1.0
 * @date 2024-06-18
 * 
 * @copyright Copyright (c) 2024  simon.xiaoapeng@gmail.com
 * 
 * @par 修改日志:
 */

#include <stdio.h>
#include "eh.h"
#include "eh_debug.h"
#include "eh_event.h"
#include "eh_platform.h"
#include "eh_timer.h" 
#include "eh_types.h"


void stdout_write(void *stream, const uint8_t *buf, size_t size){
    (void)stream;
    printf("%.*s", (int)size, (const char*)buf);
}

int task_test_2(void *arg){
    (void)arg;
    for(int i=0;i<1000000;i++){
        eh_infofl("debug! %d %llu",i,eh_get_clock_monotonic_time());
        __await eh_task_yield();
    }
    return 2;
}

int task_test_1(void *arg){
    (void)arg;
    for(int i=0;i<1000000;i++){
        eh_infofl("debug! %d %llu",i,eh_get_clock_monotonic_time());
        __await eh_task_yield();
    }
    return 1;
}

int task_app(void *arg){
    eh_task_t *test_1,*test_2;
    int app_ret;
    int ret;

    eh_debugfl("%s", arg);
    test_1 = eh_task_create("test_1", 0, 12*1024, "1", task_test_1);
    test_2 = eh_task_create("test_2", 0, 12*1024, "2", task_test_2);
    //eh_usleep(1000*1000);
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