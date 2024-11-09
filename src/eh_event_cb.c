/**
 * @file eh_event_cb.c
 * @brief 此模块是event的扩展，在event的基础上实现自动回调
 * @author simon.xiaoapeng (simon.xiaoapeng@gmail.com)
 * @date 2024-06-19
 * 
 * @copyright Copyright (c) 2024  simon.xiaoapeng@gmail.com
 * 
 */
#include "eh.h"
#include "eh_event.h"
#include "eh_event_cb.h"
#include "eh_interior.h"

#define EH_EVENT_CB_EPOLL_SLOT_SIZE 8
static eh_task_t *signal_dispose_task = NULL;
static eh_epoll_t signal_dispose_epoll = NULL;
static eh_epoll_slot_t epool_slot[EH_EVENT_CB_EPOLL_SLOT_SIZE];

/* 辅助自己退出 */
static eh_event_t event_task_quit;
static eh_event_cb_trigger_t trigger_task_quit;
static eh_event_cb_slot_t slot_task_quit;

static void task_quit_event_cb(eh_event_t *e, void *param){
    (void)e;
    (void)param;
    eh_task_exit(0);
}

static int task_signal_dispose(void *arg)
{
    int ret,i;
    (void) arg;
    while(1){
        ret = eh_epoll_wait(signal_dispose_epoll, epool_slot, EH_EVENT_CB_EPOLL_SLOT_SIZE, EH_TIME_FOREVER);
        if(ret < 0)
            return ret;
        for(i=0;i<ret;i++){
            eh_event_cb_trigger_t *trigger = (eh_event_cb_trigger_t*)epool_slot[i].userdata;
            eh_event_cb_slot_t *slot,*n;
            eh_event_t *e = epool_slot[i].event;
            if(epool_slot[i].affair == EH_EPOLL_AFFAIR_ERROR || trigger == NULL){
                eh_epoll_del_event(signal_dispose_epoll, e);
                continue;
            }

            eh_list_for_each_entry_safe(slot, n, &trigger->cb_head, cb_node){
                if(slot->slot_function)
                    slot->slot_function(e, slot->slot_param);
            }
        }
    }
}

int eh_event_cb_register(eh_event_t *e, eh_event_cb_trigger_t *trigger){
    eh_param_assert(trigger);
    return eh_epoll_add_event(signal_dispose_epoll, e, (void*)trigger);
}

int eh_event_cb_unregister(eh_event_t *e){
    return eh_epoll_del_event(signal_dispose_epoll, e);
}
int eh_event_cb_connect(eh_event_cb_trigger_t *trigger, eh_event_cb_slot_t *slot){
    eh_param_assert(trigger);
    eh_param_assert(slot);
    if(!eh_list_empty(&slot->cb_node))
        return EH_RET_BUSY;
    eh_list_add_tail(&slot->cb_node, &trigger->cb_head);
    return EH_RET_OK;
}

void eh_event_cb_disconnect(eh_event_cb_slot_t *slot){
    if(!slot) return ;
    eh_list_del_init(&slot->cb_node);
}

void eh_event_cb_trigger_clean(eh_event_cb_trigger_t *trigger){
    eh_event_cb_slot_t *slot,*n;
    if(trigger) return ;
    eh_list_for_each_entry_safe(slot, n, &trigger->cb_head, cb_node)
        eh_event_cb_disconnect(slot);
}
static int __init eh_event_cb_init(void)
{
    int ret = EH_RET_OK;
    signal_dispose_epoll = eh_epoll_new();
    ret = eh_ptr_to_error(signal_dispose_epoll);
    if(ret < 0)
        return ret;
    eh_event_init(&event_task_quit);
    eh_event_cb_slot_init(&slot_task_quit, task_quit_event_cb, NULL);
    eh_event_cb_trigger_init(&trigger_task_quit);
    
    eh_event_cb_connect(&trigger_task_quit, &slot_task_quit);
    eh_event_cb_register(&event_task_quit, &trigger_task_quit);
    
    signal_dispose_task = eh_task_create("event_cb", EH_TASK_FLAGS_SYSTEM_TASK, EH_CONFIG_EVENT_CALLBACK_FUNCTION_STACK_SIZE, NULL, task_signal_dispose);
    ret = eh_ptr_to_error(signal_dispose_task);
    if(ret < 0)
        goto eh_task_create_error;

    return ret;
eh_task_create_error:
    eh_event_cb_unregister(&event_task_quit);
    eh_event_cb_disconnect(&slot_task_quit);
    eh_task_destroy(signal_dispose_task);
    return ret;
}

static void __exit eh_event_cb_exit(void)
{
    eh_event_notify(&event_task_quit);
    
    eh_task_join(signal_dispose_task, NULL, EH_TIME_FOREVER);
    
    eh_event_cb_unregister(&event_task_quit);
    eh_event_cb_disconnect(&slot_task_quit);
    eh_epoll_del(signal_dispose_epoll);
}


eh_interior_module_export(eh_event_cb_init, eh_event_cb_exit);




