/**
 * @file eh_event_flags.c
 * @brief 事件bit map 通知，可支持 unsigned long 位数大小的flag数
 * @author simon.xiaoapeng (simon.xiaoapeng@gmail.com)
 * @version 1.0
 * @date 2024-10-03
 * 
 * @copyright Copyright (c) 2024  simon.xiaoapeng@gmail.com
 * 
 * @par 修改日志:
 */

#include "eh.h"
#include "eh_event.h"
#include "eh_event_flags.h"
#include "eh_platform.h"

struct event_flags_condition_arg{
    eh_event_flags_t *ef;
    eh_flags_t wait_flags;
    eh_flags_t claen_flags;
    eh_flags_t *reality_flags;
};


static bool condition_event_flags(void *arg){
    eh_save_state_t state;
    struct event_flags_condition_arg *ef_condition_arg = (struct event_flags_condition_arg *)arg;
    bool ret = false;    
    state = eh_enter_critical();
    if(ef_condition_arg->ef->flags & ef_condition_arg->wait_flags){
        if(ef_condition_arg->reality_flags) 
            *ef_condition_arg->reality_flags = ef_condition_arg->ef->flags & ef_condition_arg->wait_flags;
        ef_condition_arg->ef->flags &= ~ef_condition_arg->claen_flags;
        ret = true;
    }
    eh_exit_critical(state);
    return ret;
}

__safety int eh_event_flags_init(eh_event_flags_t *ef){
    int ret;
    eh_param_assert(ef);
    ret = eh_event_init(&ef->event);
    if(ret < 0) return ret;
    ef->flags = 0;
    return 0;
}

__async int eh_event_flags_wait(eh_event_flags_t *ef, eh_flags_t wait_flags, 
    eh_flags_t claen_flags, eh_flags_t *reality_flags, eh_sclock_t timeout){
    int ret;
    struct event_flags_condition_arg arg = {
        .ef = ef,
        .wait_flags = wait_flags,
        .claen_flags = claen_flags,
        .reality_flags = reality_flags
    };
    ret = __await eh_event_wait_condition_timeout(&ef->event, &arg, condition_event_flags, timeout);
    return ret;
}


__safety int eh_event_flags_set_bits(eh_event_flags_t *ef, eh_flags_t flags){
    eh_save_state_t state;
    int ret;
    state = eh_enter_critical();
    ret = eh_event_notify(&ef->event);
    if(ret == 0)
        ef->flags |= flags;
    eh_exit_critical(state);
    return ret;
}

__safety int eh_event_flags_set(eh_event_flags_t *ef, eh_flags_t flags){
    eh_save_state_t state;
    int ret;
    state = eh_enter_critical();
    ret = eh_event_notify(&ef->event);
    if(ret == 0)
        ef->flags = flags;
    eh_exit_critical(state);
    return ret;
}