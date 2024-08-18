/**
 * @file test_sem.c
 * @brief 信号量测试
 * @author simon.xiaoapeng (simon.xiaoapeng@gmail.com)
 * @version 1.0
 * @date 2024-06-23
 * 
 * @copyright Copyright (c) 2024  simon.xiaoapeng@gmail.com
 * 
 * @par 修改日志:
 */

#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include "eh.h"
#include "eh_debug.h"
#include "eh_co.h"
#include "eh_event.h"
#include "eh_platform.h"
#include "eh_timer.h"
#include "eh_sleep.h" 
#include "eh_interior.h"
#include "eh_sem.h"

eh_sem_t sem;


void stdout_write(void *stream, const uint8_t *buf, size_t size){
    (void)stream;
    printf("%.*s", (int)size, (const char*)buf);
}

int task_test(void *arg){
    int ret;
    for(;;){
        ret = __await eh_sem_wait(sem, EH_TIME_FOREVER);
        eh_debugfl("ret=%d %s",ret, arg);
        if(ret < 0)
            return -1;
    }
    return 0;
}

void* thread_function(void* arg) {
    (void) arg;
    for(int i=0;i<100;i++){
        usleep(1000*100);
        eh_sem_post(sem);
    }
    eh_sem_destroy(sem);
    return NULL;
}
int task_app(void *arg){
    eh_task_t *test_1,*test_2;
    eh_task_t *test_3,*test_4,*test_5;
    pthread_t thread_id;
    int app_ret;

    eh_debugfl("%s", arg);
    
    sem = eh_sem_create(0);

    if (pthread_create(&thread_id, NULL, thread_function, NULL) != 0) {
        eh_debugfl("pthread_create error!");
        return -1;
    }


    test_1 = eh_task_create("test_1", 0, 12*1024, "1", task_test);
    test_2 = eh_task_create("test_2", 0, 12*1024, "2", task_test);
    test_3 = eh_task_create("test_3", 0, 12*1024, "3", task_test);
    test_4 = eh_task_create("test_4", 0, 12*1024, "4", task_test);
    test_5 = eh_task_create("test_5", 0, 12*1024, "5", task_test);

    __await eh_task_join(test_1, &app_ret, EH_TIME_FOREVER);
    __await eh_task_join(test_2, &app_ret, EH_TIME_FOREVER);
    __await eh_task_join(test_3, &app_ret, EH_TIME_FOREVER);
    __await eh_task_join(test_4, &app_ret, EH_TIME_FOREVER);
    __await eh_task_join(test_5, &app_ret, EH_TIME_FOREVER);


    return 0;
}

int main(void){
    int ret;

    eh_global_init();
    ret = task_app("task_app");
    eh_global_exit();

    return ret;
}

