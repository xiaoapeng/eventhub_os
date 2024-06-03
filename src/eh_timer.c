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
#include "eh_co.h"
#include "eh_config.h"
#include "eh_interior.h"

#define _eh_timer_is_empty(eh)   ( eh_list_empty(&(eh)->timer_list_head) )
#define _eh_timer_get_expire(eh) ( container_of((eh)->timer_list_head.next, eh_timer_event_t, list_node)->expire )

static int _eh_timer_start_no_lock(eh_t *eh, eh_clock_t now, eh_timer_event_t *timer){
    eh_timer_event_t *tail, *head, *pos;
    eh_sclock_t tail_remaining_time;
    eh_sclock_t head_remaining_time;
    eh_sclock_t timer_remaining_time;
    int ret = EH_RET_OK;    
    if(!eh_list_empty(&timer->list_node)){
        ret = EH_RET_BUSY;
        goto out;
    }

    timer->expire = now + (eh_clock_t)timer->interval;
    if( _eh_timer_is_empty(eh)){
        eh_list_add(&timer->list_node, &eh->timer_list_head);
        goto out;
    }

    tail = eh_list_entry(eh->timer_list_head.prev, eh_timer_event_t, list_node);
    head = eh_list_entry(eh->timer_list_head.next, eh_timer_event_t, list_node);
    tail_remaining_time = eh_remaining_time(now, tail);
    head_remaining_time = eh_remaining_time(now, head);
    timer_remaining_time = timer->interval;

    if(timer_remaining_time < head_remaining_time){
        eh_list_add(&timer->list_node, &eh->timer_list_head);
        goto out;
    }
    if(timer_remaining_time >= tail_remaining_time){
        eh_list_add_tail(&timer->list_node, &eh->timer_list_head);
        goto out;
    }

    /* 判断目前的时间是离头近还是离尾近，离谁近就从哪开始遍历 */
    if( tail_remaining_time - timer_remaining_time  <= timer_remaining_time - head_remaining_time){
        /* 从尾开始遍历 */
        eh_list_for_each_prev_entry(pos, &eh->timer_list_head, list_node){
            if(eh_remaining_time(now, pos) <= timer_remaining_time){
                eh_list_add(&timer->list_node, &pos->list_node);
                goto out;
            }
        }
    }else{
        /* 从头开始遍历 */
        eh_list_for_each_entry(pos, &eh->timer_list_head, list_node){
            if(eh_remaining_time(now, pos) > timer_remaining_time){
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


void _eh_timer_check(void){
    uint32_t state;
    eh_t *eh = eh_get_global_handle();
    eh_timer_event_t *pos, *n;
    eh_clock_t now = eh_get_clock_monotonic_time();
    eh_clock_t base;
    eh_clock_t old_expire;
    eh_clock_t new_expire;

    _eh_lock(&state);
    
    if(_eh_timer_is_empty(eh))  goto out;

    old_expire =  _eh_timer_get_expire(eh);

    eh_list_for_each_entry_safe(pos, n, &eh->timer_list_head, list_node){
        if(eh_remaining_time(now, pos) > 0)
            break ;
        /* 定时器已经到期 */
        eh_event_notify(&pos->event);
        eh_list_del_init(&pos->list_node);
        if(!(pos->attrribute & EH_TIMER_ATTR_AUTO_CIRCULATION))
            continue;
        /* 重新启动定时器 */
        base = (pos->attrribute & EH_TIMER_ATTR_NOW_TIME_BASE) ? now : 
            eh_diff_time(pos->expire + (eh_clock_t)pos->interval, now) > 0 ? pos->expire : now;
        _eh_timer_start_no_lock(eh, base, pos);
    }

    if(_eh_timer_is_empty(eh)){
        eh->expire_time_change(1, 0);
    }else if(old_expire != (new_expire = _eh_timer_get_expire(eh))){
        eh->expire_time_change(0, new_expire);
    }

out:
    _eh_unlock(state);
}



int eh_timer_start(eh_timer_event_t *timer){
    eh_t *eh = eh_get_global_handle();
    int ret;
    uint32_t state;
    eh_clock_t now = eh_get_clock_monotonic_time();
    eh_clock_t old_expire = now - 0xFFFF; 
    eh_clock_t new_expire;
    eh_param_assert(eh);
    eh_param_assert(timer);
    eh_param_assert((eh_sclock_t)(timer->interval) > 0);

    _eh_lock(&state);

    old_expire = _eh_timer_is_empty(eh) ? old_expire : _eh_timer_get_expire(eh);
    ret = _eh_timer_start_no_lock(eh, now, timer);
    if(ret < 0) goto out;
    new_expire = _eh_timer_get_expire(eh);
    if(new_expire != old_expire)
        eh->expire_time_change(0, new_expire);
out:
    _eh_unlock(state);
    return ret;
}

int eh_timer_stop(eh_timer_event_t *timer){
    uint32_t state;
    eh_t *eh = eh_get_global_handle();
    int ret = EH_RET_OK;
    eh_clock_t old_expire; 
    eh_clock_t new_expire;
    eh_param_assert(timer);

    _eh_lock(&state);
    if(eh_list_empty(&timer->list_node))
        goto out;
    old_expire = _eh_timer_get_expire(eh);
    eh_list_del_init(&timer->list_node);
    if(_eh_timer_is_empty(eh)){
        eh->expire_time_change(1, 0);
    }else if(old_expire != (new_expire = _eh_timer_get_expire(eh))){
        eh->expire_time_change(0, new_expire);
    }
out:
    _eh_unlock(state);
    return ret;
}

int eh_time_restart(eh_timer_event_t *timer){
    eh_t *eh = eh_get_global_handle();
    uint32_t state;
    eh_clock_t now = eh_get_clock_monotonic_time();
    int ret = EH_RET_OK;
    eh_sclock_t old_remaining_time;
    eh_sclock_t new_remaining_time;
    eh_timer_event_t *pos;
    eh_clock_t old_expire = now - 0xFFFF; 
    eh_clock_t new_expire;

    eh_param_assert(timer);
    eh_param_assert(timer->interval > 0);

    _eh_lock(&state);
    old_expire = _eh_timer_is_empty(eh) ? old_expire : _eh_timer_get_expire(eh);

    /* 如果节点为空，说明不在工作，直接start */
    if(eh_list_empty(&timer->list_node)){
        ret = _eh_timer_start_no_lock(eh, now, timer);
        goto out;
    }
    /* 已经在工作，应该重新设置expire后再重排序 */
    old_remaining_time = eh_remaining_time(now, timer);
    new_remaining_time = timer->interval;
    if(old_remaining_time == new_remaining_time) goto out;
    timer->expire = now + (eh_clock_t)timer->interval;
    pos = timer;
    if(new_remaining_time > old_remaining_time){
        /* 向后移动 */
        eh_list_for_each_entry_continue(pos, &eh->timer_list_head, list_node){
            if(eh_remaining_time(now, pos) > new_remaining_time){
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
            if(eh_remaining_time(now, pos) <= new_remaining_time){
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
    if(ret == EH_RET_OK){
        new_expire = _eh_timer_get_expire(eh);
        if(new_expire != old_expire)
            eh->expire_time_change(0, new_expire);
    }
    _eh_unlock(state);
    return ret;
}

static const eh_event_type_t eh_timer_event_type = {
    .name = "timer_event",
};

int eh_timer_init(eh_timer_event_t *timer){
    int ret;
    eh_param_assert(timer);
    ret = eh_event_init(&timer->event, &eh_timer_event_type);
    if(ret < 0) return ret;
    eh_list_head_init(&timer->list_node);
    timer->expire = 0;
    timer->interval = 0;
    timer->attrribute = 0;
    return 0;
}

