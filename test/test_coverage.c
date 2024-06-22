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
#include "eh_event.h"
#include "eh_timer.h" 
#include "eh_types.h"
#include <stdlib.h>
#include <sys/epoll.h>

int task_app(void *arg){
    eh_timer_event_t timer1, timer2, timer3, timer4;
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
    eh_epoll_add_event(epoll, eh_timer_to_event(&timer3), "timer3");
    eh_epoll_add_event(epoll, eh_timer_to_event(&timer4), "timer4");

    for(int i=0;i<40;i++){
        ret = eh_epoll_wait(epoll, epoll_slot, 3, (eh_sclock_t)eh_msec_to_clock(5000));
        dbg_debugfl("ret=%d",ret);
        for(int i=0; i < ret; i++){
            if(epoll_slot[i].affair == EH_EPOLL_AFFAIR_EVENT_TRIGGER){
                dbg_debugfl("%s timeout!! %lld", epoll_slot[i].userdata, eh_get_clock_monotonic_time());
            }else{
                dbg_debugfl("%s timeout!! ERROR! ", epoll_slot[i].userdata);
            }
            
            if(epoll_slot[i].event == eh_timer_to_event(&timer2)){
                /* restart test */
                dbg_debugfl("eh_time_restart(&timer1)");
                eh_timer_restart(&timer1);
            }
        }
    }
    
    eh_timer_clean(&timer4);
    eh_timer_clean(&timer3);
    eh_timer_clean(&timer2);
    eh_timer_clean(&timer1);

    eh_epoll_del(epoll);

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