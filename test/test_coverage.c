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

int task_app(void *arg){
    eh_event_timer_t timer1, timer2, timer3, timer4;
    eh_epoll_t epoll;
    eh_epoll_slot_t epoll_slot[3];
    (void) arg;
    int ret;

    eh_timer_init(&timer1);
    eh_timer_set_attr(&timer1, EH_TIMER_ATTR_AUTO_CIRCULATION);
    eh_timer_config_interval(&timer1, (eh_sclock_t)eh_msec_to_clock(300));
    
    eh_timer_init(&timer2);
    eh_timer_set_attr(&timer2, EH_TIMER_ATTR_AUTO_CIRCULATION);
    eh_timer_config_interval(&timer2, (eh_sclock_t)eh_msec_to_clock(700));
    
    eh_timer_init(&timer3);
    eh_timer_set_attr(&timer3, 0);
    eh_timer_config_interval(&timer3, (eh_sclock_t)eh_msec_to_clock(1100));
    
    eh_timer_init(&timer4);
    eh_timer_set_attr(&timer4, EH_TIMER_ATTR_AUTO_CIRCULATION);
    eh_timer_config_interval(&timer4, (eh_sclock_t)eh_msec_to_clock(600));


    eh_timer_start(&timer1);
    eh_timer_start(&timer2);
    eh_timer_start(&timer3);
    eh_timer_restart(&timer4);

    epoll = eh_epoll_new();

    eh_epoll_add_event(epoll, eh_timer_to_event(&timer1), "timer1");
    eh_epoll_add_event(epoll, eh_timer_to_event(&timer2), "timer2");
    eh_epoll_add_event(epoll, eh_timer_to_event(&timer3), "timer3 once");
    eh_epoll_add_event(epoll, eh_timer_to_event(&timer4), "timer4");

    for(int i=0;i<40;i++){
        ret = __await eh_epoll_wait(epoll, epoll_slot, 3, (eh_sclock_t)eh_msec_to_clock(5000));
        eh_debugfl("ret=%d",ret);
        for(int i=0; i < ret; i++){
            if(epoll_slot[i].affair == EH_EPOLL_AFFAIR_EVENT_TRIGGER){
                eh_debugfl("%s timeout!! %lld", epoll_slot[i].userdata, eh_get_clock_monotonic_time());
            }else{
                eh_debugfl("%s timeout!! ERROR! ", epoll_slot[i].userdata);
            }
            
            if(epoll_slot[i].event == eh_timer_to_event(&timer2)){
                /* restart test */
                eh_debugfl("eh_time_restart(&timer1)");
                eh_timer_restart(&timer1);
            }
        }
    }
    
    eh_timer_clean(&timer4);
    eh_timer_clean(&timer3);
    eh_timer_clean(&timer2);
    eh_timer_clean(&timer1);

    eh_epoll_del(epoll);

    return 0;
}


int main(void){

    eh_global_init();
    task_app("task_app");
    eh_global_exit();
    
    return 0;
}