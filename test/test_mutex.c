/**
 * @file test_mutex.c
 * @brief 测试互斥锁
 * @author simon.xiaoapeng (simon.xiaoapeng@gmail.com)
 * @version 1.0
 * @date 2024-06-22
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
#include "eh_sleep.h" 
#include "eh_interior.h"
#include "eh_mutex.h"

eh_mutex_t sem;


void stdout_write(void *stream, const uint8_t *buf, size_t size){
    (void)stream;
    printf("%.*s", (int)size, (const char*)buf);
}

int task_test_2(void *arg){
    int ret;
    (void) arg;

    ret = __await__ eh_mutex_lock(sem, EH_TIME_FOREVER);
    eh_debugfl("ret = %d",ret);

    eh_debugfl("debug..");
    eh_usleep(1*1000*1000);
    eh_debugfl("debug..");
    eh_usleep(1*1000*1000);
    eh_debugfl("debug..");
    eh_usleep(1*1000*1000);
    eh_debugfl("debug..");
    eh_usleep(1*1000*1000);
    eh_debugfl("debug..");

    ret = eh_mutex_unlock(sem);
    eh_debugfl("ret = %d",ret);

    return 2;
}

int task_test_1(void *arg){
    int ret;
    (void) arg;

    ret = __await__ eh_mutex_lock(sem, EH_TIME_FOREVER);
    ret = __await__ eh_mutex_lock(sem, EH_TIME_FOREVER);
    eh_debugfl("ret = %d",ret);

    eh_debugfl("debug..");
    eh_usleep(1*1000*1000);
    eh_debugfl("debug..");
    eh_usleep(1*1000*1000);
    eh_debugfl("debug..");
    eh_usleep(1*1000*1000);
    eh_debugfl("debug..");
    eh_usleep(1*1000*1000);
    eh_debugfl("debug..");

    ret = eh_mutex_unlock(sem);
    ret = eh_mutex_unlock(sem);
    eh_debugfl("ret = %d",ret);

    return 1;
}


int task_app(void *arg){
    eh_task_t *test_1,*test_2;
    int app_ret;
    int ret;

    eh_debugfl("%s", arg);
    
    sem = eh_mutex_create(EH_MUTEX_TYPE_RECURSIVE);

    test_1 = eh_task_create("test_1", 12*1024, "1", task_test_1);
    test_2 = eh_task_create("test_2", 12*1024, "2", task_test_2);

    ret = __await__ eh_task_join(test_1, &app_ret, EH_TIME_FOREVER);
    eh_debugfl("test_1: ret=%d app_ret=%d", ret, app_ret);
    ret = __await__ eh_task_join(test_2, &app_ret, EH_TIME_FOREVER);
    eh_debugfl("test_2: ret=%d app_ret=%d", ret, app_ret);

    eh_mutex_destroy(sem);

    eh_loop_exit(0);
    return 0;
}

int main(void){
    int ret;

    eh_global_init();
    eh_task_create("task_app", 12*1024, "task_app", task_app);
    ret = eh_loop_run();
    eh_global_exit();

    return ret;
}

