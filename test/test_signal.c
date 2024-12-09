/**
 * @file tesh_signal.c
 * @brief 
 * @author simon.xiaoapeng (simon.xiaoapeng@gmail.com)
 * @date 2024-07-28
 * 
 * @copyright Copyright (c) 2024  simon.xiaoapeng@gmail.com
 * 
 */


#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <eh.h>
#include <eh_error.h>
#include <eh_debug.h>
#include <eh_event.h>
#include <eh_sleep.h>
#include <eh_list.h>
#include <eh_platform.h>
#include <eh_timer.h> 
#include <eh_types.h>
#include <eh_signal.h>


void stdout_write(void *stream, const uint8_t *buf, size_t size){
    (void)stream;
    printf("%.*s", (int)size, (const char*)buf);
}

EH_DEFINE_STATIC_CUSTOM_SIGNAL(
    timer_1000ms_signal, 
    eh_event_timer_t, 
    EH_TIMER_INIT(timer_1000ms_signal.custom_event)
);

EH_STATIC_SIGNAL(test_signal);

static void slot_test_function(eh_event_t *e, void *slot_param);

EH_DEFINE_SLOT(
    test_slot0, 
    slot_test_function,
    "test_slot0"
);

EH_DEFINE_SLOT(
    test_slot1, 
    slot_test_function,
    "test_slot1"
);

EH_DEFINE_SLOT(
    timer_1000ms_slot0, 
    slot_test_function,
    "test1"
);

EH_DEFINE_SLOT(
    timer_1000ms_slot1, 
    slot_test_function,
    "test2"
);


EH_DEFINE_SLOT(
    timer_1000ms_slot2, 
    slot_test_function,
    "test3"
);

static void slot_test_function(eh_event_t *e, void *slot_param){
    (void) e;
    eh_debugfl("%s",slot_param);
    if(!strcmp(slot_param,"test3")){
        /* 触发其他信号 */
        eh_event_notify(eh_signal_to_custom_event(&test_signal));
    }
}

int task_app(void *arg){
    (void) arg;

    eh_timer_advanced_init(
        eh_signal_to_custom_event(&timer_1000ms_signal), 
        (eh_sclock_t)eh_msec_to_clock(1000), 
        EH_TIMER_ATTR_AUTO_CIRCULATION
    ); 
    eh_signal_register(&timer_1000ms_signal);
    eh_signal_slot_connect(&timer_1000ms_signal, &timer_1000ms_slot0);
    eh_signal_slot_connect(&timer_1000ms_signal, &timer_1000ms_slot1);
    eh_signal_slot_connect(&timer_1000ms_signal, &timer_1000ms_slot2);
    eh_timer_start(eh_signal_to_custom_event(&timer_1000ms_signal));
    
    eh_signal_register(&test_signal);
    eh_signal_slot_connect(&test_signal, &test_slot0);
    eh_signal_slot_connect(&test_signal, &test_slot1);

    eh_usleep(1000*1000*10);

    eh_signal_slot_disconnect(&test_slot1);
    eh_signal_slot_disconnect(&test_slot0);
    eh_signal_unregister(&test_signal);
    eh_signal_clean(&test_signal);


    eh_signal_slot_disconnect(&timer_1000ms_slot2);
    eh_signal_slot_disconnect(&timer_1000ms_slot1);
    eh_signal_slot_disconnect(&timer_1000ms_slot0);
    eh_timer_stop(eh_signal_to_custom_event(&timer_1000ms_signal));
    eh_signal_unregister(&timer_1000ms_signal);
    eh_signal_clean(&timer_1000ms_signal);

    eh_infofl("exit!!");
    return 0;
}


int main(void){
    eh_debugfl("tesh_signal start!!");
    eh_global_init();
    task_app("task_app");
    eh_global_exit();
    return 0;
}