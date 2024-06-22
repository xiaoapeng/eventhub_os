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
#include "eh_event.h"
#include "eh_interior.h"
#include "eh_list.h"
#include "eh_rbtree.h"
#include "eh_timer.h"
#include "eh_types.h"


static int __async__ _eh_event_wait(eh_event_t *e, void* arg, bool (*condition)(void* arg)){
    uint32_t state;
    int ret;
    struct eh_event_receptor receptor;

    if(condition && condition(arg))
        return EH_RET_OK;

    eh_event_receptor_init(&receptor, eh_task_get_current());

    eh_lock(&state);
    eh_event_add_receptor_no_lock(e, &receptor);
    eh_unlock(state);

    for(;;){
        eh_lock(&state);
        if(condition){
            if(condition(arg)){
                ret = EH_RET_OK;
                goto unlock_out;
            }
            if(receptor.error){
                ret = EH_RET_EVENT_ERROR;
                goto unlock_out;
            }
        }else if(receptor.flags) {
            ret = receptor.trigger ? EH_RET_OK : EH_RET_EVENT_ERROR;
            goto unlock_out;
        }
        eh_task_set_current_state(EH_TASK_STATE_WAIT);
        eh_unlock(state);

        ret = __await__ eh_task_next();
        if(ret < 0){
            eh_lock(&state);
            goto unlock_out;
        }
    }

unlock_out:
    eh_event_remove_receptor_no_lock(&receptor);
    eh_unlock(state);
    return ret;
}

static int __async__ _eh_event_wait_timeout(eh_event_t *e, void* arg, bool (*condition)(void* arg), eh_sclock_t timeout){
    uint32_t state;
    int ret;
    eh_timer_event_t timeout_timer;
    struct eh_event_receptor receptor,receptor_timer;
    
    if(condition && condition(arg))
        return EH_RET_OK;

    eh_event_receptor_init(&receptor, eh_task_get_current());
    eh_event_receptor_init(&receptor_timer, eh_task_get_current());

    eh_timer_init(&timeout_timer);
    eh_timer_config_interval(&timeout_timer, timeout);

    /* timer没有start前，可以无锁add */
    eh_event_add_receptor_no_lock(eh_timer_to_event(&timeout_timer), &receptor_timer);    
    eh_timer_start(&timeout_timer);

    /* 事件预激活过，必须有锁add */
    eh_lock(&state);
    eh_event_add_receptor_no_lock(e, &receptor);
    eh_unlock(state);

    for(;;){
        eh_lock(&state);
        if(condition){
            if(receptor.error){
                ret = EH_RET_EVENT_ERROR;
                goto unlock_out;
            }
            if(condition(arg)){
                ret = EH_RET_OK;
                goto unlock_out;
            }
        }else if(receptor.flags) {
            ret = receptor.trigger ? EH_RET_OK : EH_RET_EVENT_ERROR;
            goto unlock_out;
        }

        if(receptor_timer.flags){
            ret = receptor.trigger ? EH_RET_TIMEOUT : EH_RET_EVENT_ERROR;
            goto unlock_out;
        }
        eh_task_set_current_state(EH_TASK_STATE_WAIT);
        eh_unlock(state);

        ret = __await__ eh_task_next();
        if(ret < 0){
            eh_lock(&state);
            goto unlock_out;
        }
    }

unlock_out:
    eh_event_remove_receptor_no_lock(&receptor);
    eh_unlock(state);

    eh_timer_stop(&timeout_timer);
    eh_event_remove_receptor_no_lock(&receptor_timer);
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
    eh_list_head_init(&e->receptor_list_head);
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
    struct eh_event_epoll_receptor *epoll_receptor;
    
    if(!e) return ;

    eh_lock(&state);
    eh_list_for_each_entry_safe(pos, n, &e->receptor_list_head, list_node){
        pos->error = 1;
        if(pos->wakeup_task)
            eh_task_wake_up(pos->wakeup_task);
        if(pos->epoll){
            epoll_receptor = container_of(pos, struct eh_event_epoll_receptor, receptor);
            if(eh_list_empty(&epoll_receptor->pending_list_node))
                eh_list_add_tail(&epoll_receptor->pending_list_node, &pos->epoll->pending_list_head);
            if(pos->epoll->wakeup_task)
                eh_task_wake_up(pos->epoll->wakeup_task);
        }
        eh_event_remove_receptor_no_lock(pos);
    }
    eh_unlock(state);
}


/**
 * @brief                           事件通知,唤醒所有监听该事件的任务
 * @param  e                        事件实例指针
 * @return int 
 */
int eh_event_notify(eh_event_t *e){
    uint32_t state;
    struct eh_event_receptor *pos;
    struct eh_event_epoll_receptor *epoll_receptor;
    eh_param_assert(e);
    eh_lock(&state);
    eh_list_for_each_entry(pos, &e->receptor_list_head, list_node){
        pos->trigger = 1;
        if(pos->wakeup_task)
            eh_task_wake_up(pos->wakeup_task);
        if(pos->epoll){
            epoll_receptor = container_of(pos, struct eh_event_epoll_receptor, receptor);
            if(eh_list_empty(&epoll_receptor->pending_list_node))
                eh_list_add_tail(&epoll_receptor->pending_list_node, &pos->epoll->pending_list_head);
        if(pos->epoll->wakeup_task)
            eh_task_wake_up(pos->epoll->wakeup_task);
        }
    }
    eh_unlock(state);
    return EH_RET_OK;
}

int __async__ eh_event_wait_condition_timeout(eh_event_t *e, void* arg, bool (*condition)(void* arg), eh_sclock_t timeout){
    if(eh_time_is_forever(timeout)){
        return __await__ _eh_event_wait(e, arg, condition);
    }
    if(timeout == 0){
        if(condition && condition(arg))
            return EH_RET_OK;
        return EH_RET_TIMEOUT;
    }
    return __await__ _eh_event_wait_timeout(e, arg, condition, timeout);
}

static int __epoll_rbtree_cmp(struct eh_rbtree_node *a, struct eh_rbtree_node *b){
    eh_event_t *a_event = eh_rb_entry(a,struct eh_event_epoll_receptor, rb_node)->event;
    eh_event_t *b_event = eh_rb_entry(b,struct eh_event_epoll_receptor, rb_node)->event;
    return a_event < b_event ? -1 : a_event > b_event ? 1 : 0;
}

static int __epoll_rbtree_match(const void *key, const struct eh_rbtree_node *b){
    const eh_event_t *a_event = (const eh_event_t *)key;
    const eh_event_t *b_event = eh_rb_entry(b,struct eh_event_epoll_receptor, rb_node)->event;
    return a_event < b_event ? -1 : a_event > b_event ? 1 : 0;
}

eh_epoll_t eh_epoll_new(void){
    struct eh_epoll *epoll = eh_malloc(sizeof(struct eh_epoll));
    if( epoll == NULL )
        return eh_error_to_ptr(EH_RET_MALLOC_ERROR);
    eh_list_head_init(&epoll->pending_list_head);
    eh_rb_root_init(&epoll->all_receptor_tree, __epoll_rbtree_cmp);
    return (eh_epoll_t)epoll;
}

void eh_epoll_del(eh_epoll_t _epoll){
    struct eh_event_epoll_receptor *pos,*n;
    struct eh_epoll *epoll = (struct eh_epoll *)_epoll;
    uint32_t state;
    eh_rb_postorder_for_each_entry_safe(pos, n, &epoll->all_receptor_tree, rb_node){
        eh_lock(&state);
        eh_event_remove_receptor_no_lock(&pos->receptor);
        eh_unlock(state);
        eh_free(pos);
    }
    eh_free(epoll);
}

int eh_epoll_add_event(eh_epoll_t _epoll, eh_event_t *e, void *userdata){
    uint32_t state;
    int ret = EH_RET_OK;
    struct eh_rbtree_node  *ret_rb;
    struct eh_epoll *epoll = (struct eh_epoll *)_epoll;
    struct eh_event_epoll_receptor *receptor = 
        eh_malloc(sizeof(struct eh_event_epoll_receptor));
    eh_param_assert(_epoll);
    eh_param_assert(e);
    
    if( receptor == NULL )
        return EH_RET_MALLOC_ERROR;
    eh_event_receptor_epoll_init(&receptor->receptor, NULL, epoll);
    eh_list_head_init(&receptor->pending_list_node);
    eh_rb_node_init(&receptor->rb_node);
    receptor->event = e;
    receptor->userdata = userdata;
    eh_lock(&state);
    /* 添加到epoll树中 */
    ret_rb = eh_rb_find_add(&receptor->rb_node, &epoll->all_receptor_tree);
    if( ret_rb ){
        eh_free(receptor);
        ret = EH_RET_INVALID_PARAM;
        goto out;
    }
    /* 添加接收器到event中 */
    eh_event_add_receptor_no_lock(e, &receptor->receptor);
out:
    eh_unlock(state);
    return ret;
}

int eh_epoll_del_event(eh_epoll_t _epoll,eh_event_t *e){
    uint32_t state;
    struct eh_event_epoll_receptor *epoll_receptor;
    struct eh_epoll *epoll = (struct eh_epoll *)_epoll;

    eh_param_assert(_epoll);
    eh_param_assert(e);

    eh_lock(&state);
    
    epoll_receptor = eh_rb_entry_safe(
        eh_rb_match_find(e, &epoll->all_receptor_tree, __epoll_rbtree_match),
        struct eh_event_epoll_receptor, rb_node );
    if(epoll_receptor == NULL){
        eh_unlock(state);
        return EH_RET_INVALID_PARAM;
    }
    /* 从事件中删除接收器 */
    eh_event_remove_receptor_no_lock(&epoll_receptor->receptor);
    eh_list_del(&epoll_receptor->pending_list_node);
    eh_rb_del(&epoll_receptor->rb_node, &epoll->all_receptor_tree);
    eh_unlock(state);
    eh_free(epoll_receptor);
    return EH_RET_OK;
}

static int _eh_epoll_pending_read_on_lock(struct eh_epoll *epoll, eh_epoll_slot_t *epool_slot, int slot_size){
    struct eh_event_epoll_receptor *pos, *n;
    int slot_i=0;
    eh_list_for_each_entry_safe(pos, n, &epoll->pending_list_head, pending_list_node){
        if(pos->receptor.flags == 0){
            eh_list_del_init(&pos->pending_list_node);
            break;
        }
        epool_slot[slot_i].userdata = pos->userdata;
        epool_slot[slot_i].event = pos->event;
        epool_slot[slot_i].affair = pos->receptor.error ? EH_EPOLL_AFFAIR_ERROR : 
                                    pos->receptor.trigger ? EH_EPOLL_AFFAIR_EVENT_TRIGGER : EH_EPOLL_AFFAIR_ERROR;
        pos->receptor.flags = 0;
        eh_list_del_init(&pos->pending_list_node);
        if(++slot_i >= slot_size) break;
    }
    return slot_i;
}

static int __async__ _eh_epoll_wait(struct eh_epoll *epoll, eh_epoll_slot_t *epool_slot, int slot_size){
    uint32_t state;
    int ret;
    for(;;){
        eh_lock(&state);
        ret = _eh_epoll_pending_read_on_lock(epoll, epool_slot, slot_size);
        if(ret != 0)
            goto unlock_exit;
        eh_task_set_current_state(EH_TASK_STATE_WAIT);
        epoll->wakeup_task = eh_task_get_current();
        eh_unlock(state);

        ret = __await__ eh_task_next();
        if(ret < 0)
            return ret;
    }

unlock_exit:
    eh_unlock(state);
    return ret;
}

static int __async__ _eh_epoll_wait_timeout(struct eh_epoll *epoll, eh_epoll_slot_t *epool_slot, int slot_size, eh_sclock_t timeout){
    uint32_t state;
    eh_timer_event_t timeout_timer;
    struct eh_event_receptor receptor_timer;
    int ret;
    
    eh_event_receptor_init(&receptor_timer, eh_task_get_current());
    eh_timer_init(&timeout_timer);
    eh_timer_config_interval(&timeout_timer, timeout);

    /* timer没有start前，可以无锁add */
    eh_event_add_receptor_no_lock(eh_timer_to_event(&timeout_timer), &receptor_timer);    
    eh_timer_start(&timeout_timer);

    for(;;){
        eh_lock(&state);
        ret = _eh_epoll_pending_read_on_lock(epoll, epool_slot, slot_size);
        if(ret != 0){
            goto unlock_out;
        }
        if(receptor_timer.flags){
            ret = receptor_timer.trigger ? EH_RET_TIMEOUT : EH_RET_EVENT_ERROR;
            goto unlock_out;
        }
        eh_task_set_current_state(EH_TASK_STATE_WAIT);
        epoll->wakeup_task = eh_task_get_current();
        eh_unlock(state);

        ret = __await__ eh_task_next();
        if(ret < 0)
            goto out;
    }

unlock_out:
    eh_unlock(state);
out:
    eh_timer_stop(&timeout_timer);
    eh_event_remove_receptor_no_lock(&receptor_timer);
    return ret;
}
int __async__ eh_epoll_wait(eh_epoll_t _epoll,eh_epoll_slot_t *epool_slot, int slot_size, eh_sclock_t timeout){
    uint32_t state;
    int ret;
    struct eh_epoll *epoll = (struct eh_epoll *)_epoll;
    if(eh_time_is_forever(timeout))
        return __await__ _eh_epoll_wait(epoll, epool_slot, slot_size);
    if(timeout > 0)
        return __await__  _eh_epoll_wait_timeout(epoll, epool_slot, slot_size, timeout);
    /* timeout == 0 || timeout != EH_TIME_FOREVER 时，不进行任何等待 */
    eh_lock(&state);
    ret = _eh_epoll_pending_read_on_lock(epoll, epool_slot, slot_size);
    eh_unlock(state);
    return ret == 0 ? EH_RET_TIMEOUT : ret;
}

