/**
 * @file test_event_cb.c
 * @brief 事件的回调函数测试
 * @author simon.xiaoapeng (simon.xiaoapeng@gmail.com)
 * @version 1.0
 * @date 2024-06-22
 * 
 * @copyright Copyright (c) 2024  simon.xiaoapeng@gmail.com
 * 
 * @par 修改日志:
 */

#include "debug.h"
#include "eh.h"
#include "eh_event.h"
#include "eh_timer.h" 
#include "eh_sleep.h"
#include "eh_event_cb.h"
#include "eh_types.h"

static eh_timer_event_t timer1;
static eh_event_cb_trigger_t trigger;
static eh_event_cb_slot_t slot1;
static eh_event_cb_slot_t slot2;
static void slot_function(eh_event_t *e, void *p){
    (void)e;
    eh_debugfl("slot_function %s %llu", p, eh_clock_to_usec(eh_get_clock_monotonic_time()));
}

int task_app(void *arg){
    (void) arg;
    eh_timer_init(&timer1);
    eh_timer_config_interval(&timer1, eh_msec_to_clock(1000));
    eh_timer_set_attr(&timer1, EH_TIMER_ATTR_AUTO_CIRCULATION);
    
    eh_event_cb_trigger_init(&trigger);
    eh_event_cb_slot_init(&slot1, slot_function, "slot1");
    eh_event_cb_slot_init(&slot2, slot_function, "slot2");

    eh_event_cb_register(eh_timer_to_event(&timer1), &trigger);

    eh_event_cb_connect(&trigger, &slot1);
    eh_event_cb_connect(&trigger, &slot2);
    
    eh_timer_start(&timer1);

    eh_usleep(1000*1000*10);

    eh_timer_stop(&timer1);

    eh_event_cb_disconnect(&slot1);
    eh_event_cb_disconnect(&slot2);

    eh_event_cb_unregister(eh_timer_to_event(&timer1));

    eh_timer_clean(&timer1);

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