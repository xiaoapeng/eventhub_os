/**
 * @file event_hub.c
 * @brief 核心实现
 * @author simon.xiaoapeng (simon.xiaoapeng@gmail.com)
 * @version 1.0
 * @date 2024-04-14
 *  
 * @copyright Copyright (c) 2024  simon.xiaoapeng@gmail.com
 * 
 * @par 修改日志:
 */


#include "eh.h"

extern void _eh_timer_check(eh_t *eh);




int eh_event_init(eh_event_t *e, eh_event_type_t *type, void (*callback)(eh_event_t *e)){
    eh_param_assert(e);
    eh_param_assert(type);
    eh_param_assert(callback);

    INIT_EH_LIST_HEAD(&e->list_node);
    e->type = type;
    e->callback = callback;
    e->rc = 0;
    e->flag = 0;
    e->priority = type->default_priority;

    return EH_RET_OK;
}




eh_usec_t eh_schedule(eh_t *eh){
    eh_timer_check(eh);
    
    return 0;
}




int eh_global_init(eh_t *eh, eh_init_param_t *param){
    int i;
    eh_param_assert(eh);
    eh_param_assert(param);
    eh_param_assert(param->get_usec_monotonic_time);
    eh->event_bit_map = 0;
    for(i=0;i<EH_EVENT_FIFO_CNT;i++)
        INIT_EH_LIST_HEAD(&eh->event_fifo_head[i]);

    INIT_EH_LIST_HEAD(&eh->task_list_head);
    INIT_EH_LIST_HEAD(&eh->timer_list_head);
    eh->state = EH_SCHEDULER_STATE_INIT;
    eh->global_lock = param->global_lock;
    eh->global_unlock = param->global_unlock;
    eh->get_usec_monotonic_time = param->get_usec_monotonic_time;
    eh->user_data = param->user_data;
    return 0;
}





