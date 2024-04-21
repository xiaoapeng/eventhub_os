/**
 * @file event_timer.c
 * @brief 定时器的相关实现
 * @author simon.xiaoapeng (simon.xiaoapeng@gmail.com)
 * @version 1.0
 * @date 2024-04-20
 * 
 * @copyright Copyright (c) 2024  simon.xiaoapeng@gmail.com
 * 
 * @par 修改日志:
 */

#include "eh.h"
#include "eh_list.h"
#include "eh_timer.h"

static eh_event_type_t eh_timer_event_type = {
    .name = "eh_timer",
    .destructor = NULL,
    .default_priority = EH_PRIORITY_SYSTEM,
};

static int _eh_timer_start_no_lock(eh_t *eh, eh_timer_event_t *timer){
    eh_usec_t now = _eh_get_usec_monotonic_time(eh);
    eh_timer_event_t *tail,*head;
    int64_t tail_timeout, head_timeout, timer_timeout;
    int ret = EH_RET_OK;    
    if(!eh_list_empty(&timer->list_node)){
        ret = EH_RET_BUSY;
        goto out;
    }

    timer->expire = now + timer->interval;
    if( eh_list_empty(&eh->timer_list_head)){
        eh_list_add_tail(&timer->list_node, &eh->timer_list_head);
        goto out;
    }

    tail = eh_list_entry(eh->timer_list_head.prev, eh_timer_event_t, list_node);
    head = eh_list_entry(eh->timer_list_head.next, eh_timer_event_t, list_node);
    head_timeout = (int64_t)(now - head->expire);
    timer_timeout = (int64_t)timer->interval;
    tail_timeout = (int64_t)(now - tail->expire);

    if(timer_timeout < head_timeout){
        eh_list_add(&timer->list_node, &eh->timer_list_head);
        goto out;
    }
    if(timer_timeout >= tail_timeout){
        eh_list_add_tail(&timer->list_node, &eh->timer_list_head);
        goto out;
    }

    /* 判断目前的时间是离头近还是离尾近，离谁近就从哪开始遍历 */
    if( tail_timeout - timer_timeout  <= timer_timeout - head_timeout){
        eh_timer_event_t *pos;
        int64_t pos_timeout;
        /* 从尾开始遍历 */
        eh_list_for_each_prev_entry(pos, &eh->timer_list_head, list_node){
            pos_timeout = (int64_t)(now - pos->expire);
            if(pos_timeout <= timer_timeout){
                eh_list_add(&timer->list_node, &pos->list_node);
                goto out;
            }
        }
    }else{
        eh_timer_event_t *pos;
        int64_t pos_timeout;
        /* 从头开始遍历 */
        eh_list_for_each_entry(pos, &eh->timer_list_head, list_node){
            pos_timeout = (int64_t)(now - pos->expire);
            if(pos_timeout > timer_timeout){
                eh_list_add(&timer->list_node, pos->list_node.prev);
                goto out;
            }
        }
    }
    /* 出现在这里说明出现灵异事件 */
    ret = EH_RET_FAULT;
out:
    return ret; 
}

int eh_timer_start(eh_t *eh, eh_timer_event_t *timer){
    int ret;
    uint32_t state;
    eh_param_assert(eh);
    eh_param_assert(timer);
    eh_param_assert((int64_t)(timer->interval) > 0);

    _eh_lock(eh, &state);
    ret = _eh_timer_start_no_lock(eh, timer);
    _eh_unlock(eh, state);
    return ret;
}

int eh_timer_stop(eh_t *eh, eh_timer_event_t *timer){
    uint32_t state;
    int ret = EH_RET_OK;
    eh_param_assert(eh);
    eh_param_assert(timer);

    _eh_lock(eh, &state);
    if(eh_list_empty(&timer->list_node))
        goto out;
    eh_list_del_init(&timer->list_node);
out:
    _eh_unlock(eh, state);
    return ret;
}

int eh_time_restart(eh_t *eh, eh_timer_event_t *timer){
    uint32_t state;
    eh_usec_t now = _eh_get_usec_monotonic_time(eh);
    int ret = EH_RET_OK;
    int64_t old_timeout, new_timeout, pos_timeout;
    eh_timer_event_t *pos;

    eh_param_assert(eh);
    eh_param_assert(timer);
    eh_param_assert((int64_t)(timer->interval) > 0);

    _eh_lock(eh, &state);
    /* 如何尾节点为空，说明不在工作，直接start */
    if(eh_list_empty(&timer->list_node)){
        ret = _eh_timer_start_no_lock(eh, timer);
        goto out;
    }
    /* 已经在工作，应该重新设置expire后再重排序 */
    old_timeout = (int64_t)(now - timer->expire);
    new_timeout = (int64_t)timer->interval;
    if(old_timeout == new_timeout) goto out;
    timer->expire = now + timer->interval;
    pos = timer;
    if(new_timeout > old_timeout){
        /* 向后移动 */
        eh_list_for_each_entry_continue(pos, &eh->timer_list_head, list_node){
            pos_timeout = (int64_t)(now - pos->expire);
            if(pos_timeout > new_timeout){
                if(&timer->list_node != pos->list_node.prev)
                    eh_list_move(&timer->list_node, pos->list_node.prev);
                goto out;
            }
        }
        /* 放到最后一个 */
        if(&timer->list_node != eh->timer_list_head.prev)
            eh_list_move(&timer->list_node, eh->timer_list_head.prev);
    }else{
        /* 向前移动 */
        eh_list_for_each_prev_entry_continue(pos, &eh->timer_list_head, list_node){
            pos_timeout = (int64_t)(now - pos->expire);
            if(pos_timeout <= new_timeout){
                if(&timer->list_node != pos->list_node.next)
                    eh_list_move(&timer->list_node, &pos->list_node);
                goto out;
            }
        }
        /* 放到第一个 */
        if(&timer->list_node != eh->timer_list_head.next)
            eh_list_move(&timer->list_node, &eh->timer_list_head);
    }

out:
    _eh_unlock(eh, state);
    return ret;
}

int eh_timer_init(eh_timer_event_t *timer, void (*callback)(eh_event_t *e)){
    int ret;
    eh_param_assert(timer);
    ret = eh_event_init(&timer->event, &eh_timer_event_type, callback);
    if(ret < 0) return ret;
    INIT_EH_LIST_HEAD(&timer->list_node);
    timer->expire = 0;
    timer->interval = 0;
    timer->flag = 0;
    return 0;
}

void _eh_timer_check(eh_t *eh){

}
