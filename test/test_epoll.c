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


#include <stdio.h>
#include "eh.h"
#include "eh_debug.h"
#include "eh_co.h"
#include "eh_event.h"
#include "eh_platform.h"
#include "eh_timer.h" 
#include "eh_types.h"
#include <stdlib.h>
#include <sys/epoll.h>


void stdout_write(void *stream, const uint8_t *buf, size_t size){
    (void)stream;
    printf("%.*s", (int)size, (const char*)buf);
}

int task_app(void *arg){
    eh_timer_event_t timer1, timer2, timer3;
    eh_epoll_t epoll;
    eh_epoll_slot_t epoll_slot;
    (void) arg;
    int ret;

    eh_timer_init(&timer1);
    eh_timer_set_attr(&timer1, EH_TIMER_ATTR_AUTO_CIRCULATION);
    eh_timer_config_interval(&timer1, (eh_sclock_t)eh_msec_to_clock(300));
    
    eh_timer_init(&timer2);
    eh_timer_set_attr(&timer2, EH_TIMER_ATTR_AUTO_CIRCULATION);
    eh_timer_config_interval(&timer2, (eh_sclock_t)eh_msec_to_clock(700));
    
    eh_timer_init(&timer3);
    eh_timer_set_attr(&timer3, EH_TIMER_ATTR_AUTO_CIRCULATION);
    eh_timer_config_interval(&timer3, (eh_sclock_t)eh_msec_to_clock(1100));


    eh_timer_start(&timer1);
    eh_timer_start(&timer2);
    eh_timer_start(&timer3);

    epoll = eh_epoll_new();

    eh_epoll_add_event(epoll, eh_timer_to_event(&timer1), "timer1");
    eh_epoll_add_event(epoll, eh_timer_to_event(&timer2), "timer2");
    eh_epoll_add_event(epoll, eh_timer_to_event(&timer3), "timer3");

    for(int i=0;i<40;i++){
        ret = __await__ eh_epoll_wait(epoll, &epoll_slot, 1, (eh_sclock_t)eh_msec_to_clock(5000));
        eh_debugfl("ret=%d",ret);
        if(ret > 0)
            eh_debugfl("%s timeout!! %lld", epoll_slot.userdata, eh_get_clock_monotonic_time());
    }
    
    eh_epoll_del(epoll);

    eh_timer_stop(&timer3);
    eh_timer_stop(&timer2);
    eh_timer_stop(&timer1);

    eh_loop_exit(0);
    return 0;
}


int main(void){

    eh_global_init();
    eh_task_create("task_app", 0, 12*1024, "task_app", task_app);
    eh_loop_run();
    eh_global_exit();
    
    return 0;
}