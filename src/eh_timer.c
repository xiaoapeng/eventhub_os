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
#include "eh_event.h"
#include "eh_timer.h"
#include "eh_interior.h"

#define timer_is_empty()            (eh_rb_root_is_empty(&timer_tree_root))
#define timer_get_first_expire()    (eh_rb_entry(eh_rb_first(&timer_tree_root), eh_timer_event_t, rb_node)->expire)

#define FIRST_TIMER_UPDATE      1
#define FIRST_TIMER_MAX_TIME    ((eh_sclock_t)(eh_msec_to_clock(1000*60)))

static struct eh_rbtree_root     timer_tree_root;                                       /* 系统时钟树 */
static        eh_clock_t         timer_now;
static int _eh_timer_rbtree_cmp(struct eh_rbtree_node *a, struct eh_rbtree_node *b){
    eh_sclock_t a_remaining_time = eh_remaining_time(timer_now, eh_rb_entry(a,eh_timer_event_t, rb_node));
    eh_sclock_t b_remaining_time = eh_remaining_time(timer_now, eh_rb_entry(b,eh_timer_event_t, rb_node));
    return a_remaining_time < b_remaining_time ? -1 : a_remaining_time > b_remaining_time ? 1 : 0;
}

static int _eh_timer_start_no_lock(eh_clock_t base, eh_timer_event_t *timer){
    int ret = EH_RET_OK;    
    if(!eh_rb_node_is_empty(&timer->rb_node)){
        ret = EH_RET_BUSY;
        goto out;
    }
    timer->expire = base + (eh_clock_t)timer->interval;
    
    /* 如果最紧急的定时器得到更新，那么应该通知调用者 */
    if(eh_rb_add(&timer->rb_node, &timer_tree_root))
        ret = FIRST_TIMER_UPDATE;
out:
    return ret; 
}

eh_sclock_t eh_timer_get_first_remaining_time_on_lock(void){
    eh_sclock_t min_remaining_time = 0;
    timer_now = eh_get_clock_monotonic_time();
    if(timer_is_empty())
        return FIRST_TIMER_MAX_TIME;
    min_remaining_time = eh_diff_time(timer_get_first_expire(), timer_now);
    if(min_remaining_time < 0)
        return 0;
    return min_remaining_time > FIRST_TIMER_MAX_TIME ? FIRST_TIMER_MAX_TIME : min_remaining_time;
}

void eh_timer_check(void){
    uint32_t state;
    eh_timer_event_t *first_timer;
    eh_clock_t base;
    
    eh_lock(&state);
    
    timer_now = eh_get_clock_monotonic_time();

    if(timer_is_empty())  goto out;

    for(first_timer = eh_rb_entry_safe(eh_rb_first(&timer_tree_root), eh_timer_event_t, rb_node); 
        first_timer && eh_remaining_time(timer_now, first_timer) <= 0 ; 
        first_timer = eh_rb_entry_safe(eh_rb_first(&timer_tree_root), eh_timer_event_t, rb_node)
    ){
        /* 定时器到期 */
        eh_event_notify(&first_timer->event);
        eh_rb_del(&first_timer->rb_node, &timer_tree_root);
        eh_rb_node_init(&first_timer->rb_node);

        if(!(first_timer->attrribute & EH_TIMER_ATTR_AUTO_CIRCULATION))
            continue;
        /* 重新启动定时器 */
        base = (first_timer->attrribute & EH_TIMER_ATTR_NOW_TIME_BASE) ? timer_now : 
            eh_diff_time(first_timer->expire + (eh_clock_t)first_timer->interval, timer_now) > 0 ? first_timer->expire : timer_now;
        _eh_timer_start_no_lock(base, first_timer);

    }

out:
    eh_unlock(state);
}



int eh_timer_start(eh_timer_event_t *timer){
    eh_t *eh = eh_get_global_handle();
    int ret;
    uint32_t state;
    eh_param_assert(eh);
    eh_param_assert(timer);
    eh_param_assert((eh_sclock_t)(timer->interval) > 0);

    eh_lock(&state);
    timer_now = eh_get_clock_monotonic_time();
    ret = _eh_timer_start_no_lock(timer_now, timer);
    if(ret == FIRST_TIMER_UPDATE)
        eh_idle_break();
    eh_unlock(state);
    return ret;
}

int eh_timer_stop(eh_timer_event_t *timer){
    uint32_t state;
    int ret = EH_RET_OK;
    eh_param_assert(timer);

    eh_lock(&state);
    if(eh_rb_node_is_empty(&timer->rb_node))
        goto out;
    
    eh_rb_del(&timer->rb_node, &timer_tree_root);
    eh_rb_node_init(&timer->rb_node);

out:
    eh_unlock(state);
    return ret;
}

int eh_timer_restart(eh_timer_event_t *timer){
    uint32_t state;
    int ret = EH_RET_OK;

    eh_param_assert(timer);
    eh_param_assert(timer->interval > 0);

    eh_lock(&state);
    timer_now = eh_get_clock_monotonic_time();

    /* 如果节点为空，说明不在工作，直接start */
    if(eh_rb_node_is_empty(&timer->rb_node)){
        ret = _eh_timer_start_no_lock(timer_now, timer);
        goto out;
    }
    /* 已经在工作，从树中删除后重新启动 */
    eh_rb_del(&timer->rb_node, &timer_tree_root);
    eh_rb_node_init(&timer->rb_node);
    ret = _eh_timer_start_no_lock(timer_now, timer);

out:
    if(ret == FIRST_TIMER_UPDATE)
        eh_idle_break();
    eh_unlock(state);
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
    eh_rb_node_init(&timer->rb_node);
    timer->expire = 0;
    timer->interval = 0;
    timer->attrribute = 0;
    return 0;
}


void eh_timer_clean(eh_timer_event_t *timer){
    eh_timer_stop(timer);
    eh_event_clean(&timer->event);
}

static int __init eh_timer_interior_init(void){
    eh_rb_root_init(&timer_tree_root, _eh_timer_rbtree_cmp);
    return 0;
}

static void __exit eh_timer_interior_exit(void){
}

eh_interior_module_export(eh_timer_interior_init, eh_timer_interior_exit);

