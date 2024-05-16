/**
 * @file eh_event.c
 * @brief 事件模型、poll模型的实现
 * @author simon.xiaoapeng (simon.xiaoapeng@gmail.com)
 * @version 1.0
 * @date 2024-05-15
 * 
 * @copyright Copyright (c) 2024  simon.xiaoapeng@gmail.com
 * 
 * @par 修改日志:
 */

#include <stdio.h>
#include <string.h>
#include "eh.h"
#include "eh_interior.h"


static int __async__ _eh_event_wait(eh_event_t *e){
    uint32_t state;
    int ret;
    eh_event_receptor_t receptor;
    eh_event_receptor_init(&receptor, eh_get_current_task());

    _eh_lock(&state);
    eh_event_add_receptor_no_lock(e, &receptor);

task_next:
    eh_set_current_task_state(EH_TASK_STATE_WAIT);
    _eh_unlock(state);

    ret = __await__ eh_task_next();
    if(ret < 0){
        _eh_lock(&state);
        eh_event_remove_receptor_no_lock(&receptor);
        _eh_unlock(state);
        return ret;
    }

    /* 被唤醒 */
    _eh_lock(&state);
    if(receptor.notify_cnt == 0){
        if(eh_event_receptors_is_isolate(&receptor)){
            /* 一般应该是被等待的事件被释放了 */
            ret = EH_RET_EVENT_ERROR;
            goto out;
        }
        goto task_next;

    }

    ret = EH_RET_OK;
out:
    eh_event_remove_receptor_no_lock(&receptor);
    _eh_unlock(state);
    return ret;
}

static int __async__ _eh_event_wait_timeout(eh_event_t *e, eh_sclock_t timeout){
    uint32_t state;
    int ret;
    eh_timer_event_t timeout_timer;
    eh_event_t *timeout_e = eh_timer_to_event(&timeout_timer);
    eh_event_receptor_t receptor,receptor_timer;
    
    eh_event_receptor_init(&receptor, eh_get_current_task());
    eh_event_receptor_init(&receptor_timer, eh_get_current_task());

    eh_timer_init(&timeout_timer);
    eh_timer_config_interval(&timeout_timer, timeout);

    _eh_lock(&state);
    eh_event_add_receptor_no_lock(e, &receptor);
    eh_event_add_receptor_no_lock(timeout_e, &receptor_timer);
    eh_timer_start(&timeout_timer);

task_next:
    eh_set_current_task_state(EH_TASK_STATE_WAIT);
    _eh_unlock(state);

    ret = __await__ eh_task_next();
    if(ret < 0){
        _eh_lock(&state);
        eh_event_remove_receptor_no_lock(&receptor);
        eh_event_remove_receptor_no_lock(&receptor_timer);
        eh_timer_stop(&timeout_timer);
        _eh_unlock(state);
        return ret;
    }
    
    /* 被唤醒 */

    _eh_lock(&state);
    if((receptor.notify_cnt == 0 && receptor_timer.notify_cnt == 0)){
        if( eh_event_receptors_is_isolate(&receptor)     ||
            eh_event_receptors_is_isolate(&receptor_timer)
         ){
            /* 一般应该是被等待的事件被释放了 */
            ret = EH_RET_EVENT_ERROR;
            goto out;
        }
        /* 莫名其妙被唤醒,重新调度 */
        goto task_next;
    }
    ret = receptor.notify_cnt > 0 ? EH_RET_OK : EH_RET_TIMEOUT;
out:
    eh_event_remove_receptor_no_lock(&receptor);
    eh_event_remove_receptor_no_lock(&receptor_timer);
    eh_timer_stop(&timeout_timer);
    _eh_unlock(state);
    return ret;
}


/**
 * @brief                           事件初始化
 * @param  e                        事件实例指针
 * @param  static_const_name        事件名称，可为NULL
 * @return int                      见eh_error.h
 */
int eh_event_init(eh_event_t *e, const eh_event_type_t* type){
    eh_param_assert(e);
    INIT_EH_LIST_HEAD(&e->receptor_list_head);
    e->type = type;
    return EH_RET_OK;
}


/**
 * @brief                           唤醒所有监听该事件的任务
 *                                  并将其剔除等待队列，一般在释放event实例前进行调用
 * @param  e                        事件实例指针
 */
void eh_event_clean(eh_event_t *e){
    uint32_t state;
    struct eh_event_receptor *pos ,*n;
    
    if(!e) return ;

    _eh_lock(&state);
    eh_list_for_each_entry_safe(pos, n, &e->receptor_list_head, list_node){
        if(pos->wakeup_task)
            eh_task_wake_up(pos->wakeup_task);
        eh_event_remove_receptor_no_lock(pos);
    }
    _eh_unlock(state);
}


/**
 * @brief                           事件通知,唤醒所有监听该事件的任务
 * @param  e                        事件实例指针
 * @return int 
 */
int eh_event_notify(eh_event_t *e){
    uint32_t state;
    struct eh_event_receptor *pos;
    eh_param_assert(e);
    _eh_lock(&state);
    eh_list_for_each_entry(pos, &e->receptor_list_head, list_node){
        pos->notify_cnt++;
        if(pos->wakeup_task)
            eh_task_wake_up(pos->wakeup_task);
    }
    _eh_unlock(state);
    return EH_RET_OK;
}

/**
 * @brief                           事件等待
 * @param  e                        事件实例指针
 * @param  timeout                  超时时间,禁止为0,若想为0，请使用epoll,EH_TIMER_FOREVER为永不超时
 * @return int 
 */
int __async__ eh_event_wait_timeout(eh_event_t *e, eh_sclock_t timeout){
    if(eh_time_is_forever(timeout)){
        return __await__ _eh_event_wait(e);
    }
    eh_param_assert(timeout > 0);
    return __await__ _eh_event_wait_timeout(e, timeout);
}


