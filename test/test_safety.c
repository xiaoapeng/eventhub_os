/**
 * @file test_safety.c
 * @brief 线程安全测试
 * @author simon.xiaoapeng (simon.xiaoapeng@gmail.com)
 * @version 1.0
 * @date 2024-06-16
 * 
 * @copyright Copyright (c) 2024  simon.xiaoapeng@gmail.com
 * 
 * @par 修改日志:
 */

#include <stdio.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <unistd.h>

#include "eh.h"
#include "eh_debug.h"
#include "eh_event.h"
#include "eh_platform.h"
#include "eh_timer.h"

static bool gpio_status = false;


void stdout_write(void *stream, const uint8_t *buf, size_t size){
    (void)stream;
    printf("%.*s", (int)size, (const char*)buf);
}

void* thread_function(void* arg) {
    eh_timer_event_t *gpio_debounce_timer = (eh_timer_event_t *)arg;
    /* 模拟gpio 中断 线程 */
    while(1){
        /* 模拟抖动变高电平 */
        gpio_status = true;
        eh_timer_restart(gpio_debounce_timer);
        usleep(1000);
        gpio_status = false;
        eh_timer_restart(gpio_debounce_timer);
        usleep(1000);
        gpio_status = true;
        eh_timer_restart(gpio_debounce_timer);
        usleep(1000);
        gpio_status = false;
        eh_timer_restart(gpio_debounce_timer);
        usleep(30*1000);
        gpio_status = true;
        eh_timer_restart(gpio_debounce_timer);
        usleep(1000*1000);

        
        /* 模拟抖动变低电平 */
        gpio_status = false;
        eh_timer_restart(gpio_debounce_timer);
        usleep(3000);
        gpio_status = true;
        eh_timer_restart(gpio_debounce_timer);
        usleep(1000);
        gpio_status = false;
        eh_timer_restart(gpio_debounce_timer);
        usleep(1000);
        gpio_status = true;
        eh_timer_restart(gpio_debounce_timer);
        usleep(1000);
        gpio_status = false;
        eh_timer_restart(gpio_debounce_timer);
        usleep(30*1000);
        gpio_status = true;
        eh_timer_restart(gpio_debounce_timer);
        usleep(1000);
        gpio_status = false;
        eh_timer_restart(gpio_debounce_timer);
        usleep(1000*1000*3);
    }

    return NULL;
}

int task_app(void *arg){
    eh_timer_event_t gpio_debounce_timer;
    pthread_t thread_id;
    (void) arg;
    int ret;

    eh_timer_init(&gpio_debounce_timer);
    eh_timer_set_attr(&gpio_debounce_timer, 0);
    /* 假设是50ms的定时器 */
    eh_timer_config_interval(&gpio_debounce_timer, (eh_sclock_t)eh_msec_to_clock(50));

    if (pthread_create(&thread_id, NULL, thread_function, &gpio_debounce_timer) != 0) {
        eh_debugfl("pthread_create error!");
        eh_loop_exit(1);
    }
    for(;;){
        ret = __await__ eh_event_wait_timeout(eh_timer_to_event(&gpio_debounce_timer), EH_TIME_FOREVER);
        if(ret != EH_RET_OK)
            break;
        eh_debugfl("gpio_status:%s %llu", gpio_status ? "H":"L", eh_clock_to_msec(eh_get_clock_monotonic_time()));
    }

    return 0;
}


int main(void){

    eh_global_init();
    eh_task_create("task_app", 12*1024, "task_app", task_app);
    eh_loop_run();
    eh_global_exit();
    
    return 0;
}