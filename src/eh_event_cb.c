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
#include <eh_platform.h>
#include <eh_event.h>
#include <eh_event_cb.h>
#include <eh_interior.h>
#include <eh_debug.h>
#include <eh_mem.h>


#define EH_EVENT_CB_EPOLL_SLOT_SIZE 8


struct eh_event_cb_trigger{
    struct eh_list_head     cb_head;
};

#define EH_EVENT_CB_TRIGGER_INIT(trigger)   {               \
        .cb_head = EH_LIST_HEAD_INIT(trigger.cb_head),  \
    }


#define trigger_init(trigger) eh_list_head_init(&trigger->cb_head)


static void trigger_clean(struct eh_event_cb_trigger *trigger){
    eh_event_cb_slot_t *slot,*n;
    if(trigger) return ;
    eh_list_for_each_entry_safe(slot, n, &trigger->cb_head, cb_node)
        eh_list_del_init(&slot->cb_node);
}

static void epoll_userdata_free(void *node_handle){
    struct eh_event_cb_trigger *trigger = 
        (struct eh_event_cb_trigger *)eh_epoll_get_handle_userdata_no_lock(node_handle);
    if(trigger == NULL)
        return ;
    trigger_clean(trigger);
    eh_free(trigger);
}

static void task_system_data_destruct_function(eh_task_t *task){
    eh_epoll_t epoll = (eh_epoll_t)task->system_data;
    eh_epoll_del_event(epoll, &task->event);
    eh_epoll_del_advanced(epoll, epoll_userdata_free);
    task->system_data = NULL;
    task->system_data_destruct_function = NULL;
}

static eh_epoll_t task_get_epoll_try_new(eh_task_t *task){
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

static inline eh_epoll_t task_get_epoll(eh_task_t *task){
    return (eh_epoll_t)task->system_data;
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

    epoll = task_get_epoll_try_new(task);
    if(eh_ptr_to_error(epoll) < 0)
        return eh_ptr_to_error(epoll);
    while(1){
        ret = eh_epoll_wait(epoll, epool_slot, EH_EVENT_CB_EPOLL_SLOT_SIZE, EH_TIME_FOREVER);
        if(ret < 0)
            return ret;
        for(i=0;i<ret;i++){
            struct eh_event_cb_trigger   *trigger = (struct eh_event_cb_trigger*)epool_slot[i].userdata;
            eh_event_cb_slot_t      *slot;
            struct eh_list_head     *node;
            struct eh_list_head     used_tmp_list;
            eh_event_t *e = epool_slot[i].event;

            if(epool_slot[i].affair == EH_EPOLL_AFFAIR_ERROR){
                if(trigger){
                    trigger_clean(trigger);
                    eh_free(trigger);
                }
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

int eh_event_cb_connect(eh_event_t *e, eh_event_cb_slot_t *slot, eh_task_t *task){
    void **set_trigger_ptr = NULL;
    struct eh_event_cb_trigger *trigger = NULL;
    int ret;
    eh_epoll_t epoll;
    eh_param_assert(e);
    eh_param_assert(slot);
    eh_param_assert(task);
    
    if(!eh_list_empty(&slot->cb_node)){
        eh_mwarnfl(EVENT_CB, "slot@%#p---sig@%#p is already connected to task@%#p", slot, e, task);
        return EH_RET_BUSY;
    }
    if(e == &task->event){
        /* 禁止连接任务自身的事件 */
        eh_mwarnfl(EVENT_CB, "slot@%#p---sig@%#p Prohibit connecting events of the task itself. task@%#p", slot, e, task);
        return EH_RET_INVALID_PARAM;
    }
    epoll = task_get_epoll_try_new(task);
    if(eh_ptr_to_error(epoll) < 0)
        return eh_ptr_to_error(epoll);
    ret = eh_epoll_add_event_advanced(epoll, e, &set_trigger_ptr);
    if(ret != EH_RET_OK && ret != EH_RET_EXISTS )
        return ret;
    if(ret == EH_RET_EXISTS){
        /* 获取以前的trigger */
        trigger = (struct eh_event_cb_trigger *)(*set_trigger_ptr);
    }else{
        trigger = eh_malloc(sizeof(struct eh_event_cb_trigger));
        if(!trigger) 
            return EH_RET_MALLOC_ERROR;
        trigger_init(trigger);
        *set_trigger_ptr = trigger;
    }
    /* 连接slot到trigger */
    eh_list_add_tail(&slot->cb_node, &trigger->cb_head);
    return EH_RET_OK;
}


void eh_event_cb_disconnect(eh_event_t *e, eh_event_cb_slot_t *slot, eh_task_t *task){
    eh_save_state_t state;
    eh_epoll_t epoll = task_get_epoll(task);
    void *node_handle;
    struct eh_event_cb_trigger *trigger = NULL;

    if(!slot || !e || !task || !epoll) 
        return ;
    eh_list_del_init(&slot->cb_node);
    state = eh_enter_critical();
    node_handle = eh_epoll_get_node_handle_no_lock(epoll, e);
    if(eh_ptr_to_error(node_handle) < 0)
        goto out;
    trigger = eh_epoll_get_handle_userdata_no_lock(node_handle);
    if(!eh_list_empty(&trigger->cb_head))
        goto out;
    eh_free(trigger);
    eh_epoll_del_event_form_handle_no_lock(epoll, node_handle);
out:
    eh_exit_critical(state);
}


void eh_event_cb_clean(eh_event_t *e, eh_task_t *task){
    eh_save_state_t state;
    eh_epoll_t epoll = task_get_epoll(task);
    void *node_handle;
    struct eh_event_cb_trigger *trigger = NULL;

    if(!e || !task || !epoll) 
        return ;
    state = eh_enter_critical();
    node_handle = eh_epoll_get_node_handle_no_lock(epoll, e);
    if(eh_ptr_to_error(node_handle) < 0)
        goto out;
    trigger = eh_epoll_get_handle_userdata_no_lock(node_handle);
    if(!trigger)
        goto out;
    trigger_clean(trigger);
    eh_epoll_del_event_form_handle_no_lock(epoll, node_handle);
    eh_free(trigger);
out:
    eh_exit_critical(state);
}


