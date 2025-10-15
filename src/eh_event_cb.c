/**
 * @file eh_event_cb.c
 * @brief 此模块是event的扩展，在event的基础上实现自动回调
 * @author simon.xiaoapeng (simon.xiaoapeng@gmail.com)
 * @date 2024-06-19
 * 
 * @copyright Copyright (c) 2024  simon.xiaoapeng@gmail.com
 * 
 */

#include <eh.h>
#include <eh_list.h>
#include <eh_event.h>
#include <eh_event_cb.h>
#include <eh_interior.h>
#include <eh_debug.h>

#define EH_EVENT_CB_EPOLL_SLOT_SIZE 8

static void task_system_data_destruct_function(eh_task_t *task){
    eh_epoll_t epoll = (eh_epoll_t)task->system_data;
    eh_epoll_del_event(epoll, &task->event);
    eh_epoll_del(epoll);
    task->system_data = NULL;
    task->system_data_destruct_function = NULL;
}

static eh_epoll_t task_get_epoll(eh_task_t *task){
    eh_epoll_t epoll = NULL;
    int ret;
    if(task->system_data){
        return (eh_epoll_t)task->system_data;
    }
    epoll = eh_epoll_new();
    if(eh_ptr_to_error(epoll) < 0){
        return epoll;
    }
    ret = eh_epoll_add_event(epoll, &task->event, NULL);
    if(ret < 0){
        eh_epoll_del(epoll);
        return eh_error_to_ptr(ret);
    }
    task->system_data = epoll;
    task->system_data_destruct_function = task_system_data_destruct_function;
    return epoll;
}



int eh_event_loop(void)
{
    int ret,i;
    unsigned int last_task_dispatch_cnt = eh_task_dispatch_cnt();
    unsigned int task_dispatch_cnt = last_task_dispatch_cnt;
    unsigned int continue_cnt0 = 0,continue_cnt1 = 0;
    eh_epoll_slot_t epool_slot[EH_EVENT_CB_EPOLL_SLOT_SIZE];
    eh_task_t *task = eh_task_self();
    eh_epoll_t epoll;

    if(eh_read_once(task->flags) & EH_TASK_FLAGS_INTERIOR_REQUEST_QUIT)
        return EH_RET_OK;

    epoll = task_get_epoll(task);
    if(eh_ptr_to_error(epoll) < 0)
        return eh_ptr_to_error(epoll);
    while(1){
        ret = eh_epoll_wait(epoll, epool_slot, EH_EVENT_CB_EPOLL_SLOT_SIZE, EH_TIME_FOREVER);
        if(ret < 0)
            return ret;
        for(i=0;i<ret;i++){
            eh_event_cb_trigger_t   *trigger = (eh_event_cb_trigger_t*)epool_slot[i].userdata;
            eh_event_cb_slot_t      *slot;
            struct eh_list_head     *node;
            struct eh_list_head     used_tmp_list;
            eh_event_t *e = epool_slot[i].event;

            if(epool_slot[i].affair == EH_EPOLL_AFFAIR_ERROR){
                eh_epoll_del_event(epoll, e);
                continue;
            }

            if(trigger == NULL)
                continue;

            /**
             *  为了避免在执行回调的时候，list被修改，这里将分离node到临时的list,
             */
            eh_list_head_init(&used_tmp_list);
            while(!eh_list_empty(&trigger->cb_head)){
                node = trigger->cb_head.next;
                eh_list_move_tail(node, &used_tmp_list);
                slot = eh_list_entry(node, eh_event_cb_slot_t, cb_node);
                if(slot->slot_function)
                    slot->slot_function(e, slot->slot_param);
            }

            /* 还原list */
            eh_list_splice(&used_tmp_list, &trigger->cb_head);

            continue_cnt1++;
            if(continue_cnt1%EH_CONFIG_EVENT_CB_DISPATCH_CNT_PER_CHECKTIMER == 0){
                eh_timer_check();
            }
        }

        if(eh_read_once(task->flags) & EH_TASK_FLAGS_INTERIOR_REQUEST_QUIT)
            break;

        task_dispatch_cnt = eh_task_dispatch_cnt();
        if(last_task_dispatch_cnt != task_dispatch_cnt){
            continue_cnt0 = 0;
            last_task_dispatch_cnt = task_dispatch_cnt;
        }else{
            continue_cnt0++;
            if(continue_cnt0 >= EH_CONFIG_EVENT_CB_DISPATCH_CNT_PER_YIELD){
                continue_cnt0 = 0;
                eh_task_yield();
            }
        }
    }
    return EH_RET_OK;
}

void eh_event_loop_request_quit(eh_task_t *task){
    eh_event_notify(&task->event);
    task->flags |= EH_TASK_FLAGS_INTERIOR_REQUEST_QUIT;
}

int eh_event_cb_register(eh_task_t *task, eh_event_t *e, eh_event_cb_trigger_t *trigger){
    eh_param_assert(task);
    eh_param_assert(trigger);
    eh_epoll_t epoll = task_get_epoll(task);
    if(eh_ptr_to_error(epoll) < 0)
        return eh_ptr_to_error(epoll);
    return eh_epoll_add_event(epoll, e, (void*)trigger);
}

int eh_event_cb_unregister(eh_task_t *task, eh_event_t *e){
    eh_epoll_t epoll = task_get_epoll(task);
    if(eh_ptr_to_error(epoll) < 0)
        return eh_ptr_to_error(epoll);
    return eh_epoll_del_event(epoll, e);
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


