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

#include <stdio.h>
#include <string.h>
#include "debug.h"
#include "eh.h"
#include "eh_co.h"
#include "eh_error.h"
#include "eh_interior.h"
#include "eh_list.h"
#include "eh_module.h"
#include "eh_types.h"


eh_t _global_eh;

eh_module_section_define();

static const eh_event_type_t task_event_type = {
    .name = "task_event"
};

static struct  eh_task s_main_task;

/**
 * @brief             进行下一个任务的调度，调度成功返回0，调度失败返回-1
 * @return int        -1:调度失败 0:调度成功
 */
static  int __async__ eh_task_next(void){
    eh_t *eh = &_global_eh;
    uint32_t state;
    eh_task_t *current_task = eh_get_current_task();
    eh_task_t *to;
    
    _eh_lock(&state);
    if(eh_list_empty(&current_task->task_list_node)){
        _eh_unlock(state);
        return -1;
    }
    to = eh_list_entry(current_task->task_list_node.next, eh_task_t, task_list_node);
    eh_set_current_task(to);
    switch (current_task->state) {
        case EH_TASK_STATE_READY:
        case EH_TASK_STATE_RUNING:
            current_task->state = EH_TASK_STATE_READY;
            break;
        case EH_TASK_STATE_WAIT:
            eh_list_move_tail(&current_task->task_list_node, &eh->task_wait_list_head);
            break;
        case EH_TASK_STATE_FINISH:
            eh_list_move_tail(&current_task->task_list_node, &eh->task_finish_list_head);
            break;
    }
    to->state = EH_TASK_STATE_RUNING;
    _eh_unlock(state);
    co_context_swap(NULL, &current_task->context, &to->context);
    return 0;
}

static int _task_entry(void* arg){
    (void) arg;
    eh_task_t *current_task = eh_get_current_task();
    current_task->task_ret = current_task->task_function(current_task->task_arg);
    current_task->state = EH_TASK_STATE_FINISH;
    eh_event_notify(&current_task->event);
    __await__ eh_task_next();
    return current_task->task_ret;
}


static void eh_task_wake_up(eh_task_t *wakeup_task){
    eh_t *eh = &_global_eh;
    uint32_t state;
    _eh_lock(&state);
    if(wakeup_task->state != EH_TASK_STATE_WAIT)
        goto out;
    wakeup_task->state = EH_TASK_STATE_READY;
    eh_list_move_tail(&wakeup_task->task_list_node, &eh->current_task->task_list_node);
out:
    _eh_unlock(state);
}


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

int eh_event_init(eh_event_t *e, const eh_event_type_t* type){
    eh_param_assert(e);
    INIT_EH_LIST_HEAD(&e->receptor_list_head);
    e->type = type;
    return EH_RET_OK;
}

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
int __async__ eh_event_wait_timeout(eh_event_t *e, eh_sclock_t timeout){
    if(eh_time_is_forever(timeout)){
        return __await__ _eh_event_wait(e);
    }
    eh_param_assert(timeout > 0);
    return __await__ _eh_event_wait_timeout(e, timeout);
}

static eh_task_t* _eh_create_static_stack_task(const char *name,int is_static_stack, 
            void *stack, uint32_t stack_size, void *task_arg, int (*task_function)(void*)){
    eh_task_t *task = (eh_task_t *)eh_malloc(sizeof(eh_task_t) + strlen(name) + 1);
    if(task == NULL) return eh_error_to_ptr(EH_RET_MALLOC_ERROR);
    task->name = (char*)(task + 1);
    strcpy((char*)task->name, name);
    INIT_EH_LIST_HEAD(&task->task_list_node);
    INIT_EH_LIST_HEAD(&task->epoll_list_node);
    task->task_function = task_function;
    task->task_arg = task_arg;
    task->stack_top = stack + stack_size;
    task->stack_size = stack_size;
    task->context = co_context_make(stack + stack_size, _task_entry);
    task->task_ret = 0;
    task->state = EH_TASK_STATE_WAIT;
    task->is_static_stack = is_static_stack & 0x01;
    eh_event_init(&task->event, &task_event_type);
    eh_task_wake_up(task);
    return task;
}

eh_task_t* eh_create_static_stack_task(const char *name, void *stack, uint32_t stack_size, void *task_arg, int (*task_function)(void*)){
    return _eh_create_static_stack_task(name, 1, stack, stack_size, task_arg, task_function);
}

eh_task_t* eh_create_task(const char *name, uint32_t stack_size, void *task_arg, int (*task_function)(void*)){
    eh_task_t *task;
    void *stack = eh_malloc(stack_size);
    if(stack == NULL) return eh_error_to_ptr(EH_RET_MALLOC_ERROR);
    task = eh_create_static_stack_task(name, stack, stack_size, task_arg, task_function);
    if(eh_ptr_to_error(task) < 0)
        eh_free(stack);
    return task;
}

void eh_loop_run(void){
    int ret;
    _global_eh.state = EH_SCHEDULER_STATE_RUN;
    for(;;){

        /* 检查定时器是否超时，超时后进行相关事件通知 */
        _eh_timer_check();
        
        /* 检查是否有信号需要处理 */
        
        /* 进行调度 */
        ret = __await__ eh_task_next();

        /* 调用用户外部处理函数 */
        _global_eh.state = EH_SCHEDULER_STATE_IDLE_OR_EVENT_HANDLER;
        _global_eh.idle_or_extern_event_handler(ret == 0 ? 0 : 1);
        _global_eh.state = EH_SCHEDULER_STATE_RUN;

    }
}



static int interior_init(void){
    extern eh_platform_port_param_t platform_port_param;
    eh_t *eh = eh_get_global_handle();
    eh_param_assert(platform_port_param.get_clock_monotonic_time);
    eh_param_assert(platform_port_param.idle_or_extern_event_handler);
    eh_param_assert(platform_port_param.expire_time_change);
    eh->clocks_per_sec = platform_port_param.clocks_per_sec;
    
    INIT_EH_LIST_HEAD(&eh->task_wait_list_head);
    INIT_EH_LIST_HEAD(&eh->task_finish_list_head);
    INIT_EH_LIST_HEAD(&eh->timer_list_head);

    eh->state = EH_SCHEDULER_STATE_INIT;
    eh->global_lock = platform_port_param.global_lock;
    eh->global_unlock = platform_port_param.global_unlock;
    eh->get_clock_monotonic_time = platform_port_param.get_clock_monotonic_time;
    eh->idle_or_extern_event_handler = platform_port_param.idle_or_extern_event_handler;
    eh->expire_time_change = platform_port_param.expire_time_change;
    eh->malloc = platform_port_param.malloc;
    eh->free = platform_port_param.free;

    eh->main_task = &s_main_task;
    eh->current_task = &s_main_task;
    s_main_task.task_arg = NULL;
    s_main_task.name = "main_task";
    INIT_EH_LIST_HEAD( &s_main_task.task_list_node );
    INIT_EH_LIST_HEAD( &s_main_task.epoll_list_node );
    s_main_task.task_function = NULL;
    s_main_task.task_arg = NULL;
    s_main_task.context = NULL;
    s_main_task.stack_top = NULL;
    s_main_task.stack_size = 0;
    s_main_task.state = EH_TASK_STATE_RUNING;

    eh->module_group[0].module_array = (struct eh_module*)eh_modeule_section_begin(0);
    eh->module_group[0].module_cnt = (struct eh_module*)eh_modeule_section_end(0) - (struct eh_module*)eh_modeule_section_begin(0);

    eh->module_group[1].module_array = (struct eh_module*)eh_modeule_section_begin(1);
    eh->module_group[1].module_cnt = (struct eh_module*)eh_modeule_section_end(1) - (struct eh_module*)eh_modeule_section_begin(1);

    eh->module_group[2].module_array = (struct eh_module*)eh_modeule_section_begin(2);
    eh->module_group[2].module_cnt = (struct eh_module*)eh_modeule_section_end(2) - (struct eh_module*)eh_modeule_section_begin(2);

    eh->module_group[3].module_array = (struct eh_module*)eh_modeule_section_begin(3);
    eh->module_group[3].module_cnt = (struct eh_module*)eh_modeule_section_end(3) - (struct eh_module*)eh_modeule_section_begin(3);

    eh->module_group[4].module_array = (struct eh_module*)eh_modeule_section_begin(4);
    eh->module_group[4].module_cnt = (struct eh_module*)eh_modeule_section_end(4) - (struct eh_module*)eh_modeule_section_begin(4);

    eh->module_group[5].module_array = (struct eh_module*)eh_modeule_section_begin(5);
    eh->module_group[5].module_cnt = (struct eh_module*)eh_modeule_section_end(5) - (struct eh_module*)eh_modeule_section_begin(5);

    eh->module_group[6].module_array = (struct eh_module*)eh_modeule_section_begin(6);
    eh->module_group[6].module_cnt = (struct eh_module*)eh_modeule_section_end(6) - (struct eh_module*)eh_modeule_section_begin(6);

    eh->module_group[7].module_array = (struct eh_module*)eh_modeule_section_begin(7);
    eh->module_group[7].module_cnt = (struct eh_module*)eh_modeule_section_end(7) - (struct eh_module*)eh_modeule_section_begin(7);
    
    return 0;
}

static int  module_group_init (void){
    long i,j;
    int ret;
    struct module_group *start_group = eh_get_global_handle()->module_group;
    for(i=0;i<EH_MODEULE_GROUP_MAX_CNT;i++){
        for(j=0;j<start_group[i].module_cnt;j++){
            if(start_group[i].module_array[j].init){
                ret = start_group[i].module_array[j].init();
                if(ret < 0) goto init_error;
            }
        }
    }

    return ret;
init_error:
    for(j=j-1;j>=0;j--){
        if(start_group[i].module_array[j].exit)
            start_group[i].module_array[j].exit();
    }
    for(i=i-1;i>=0;i--){
        j = start_group[i].module_cnt;
        for(j=j-1;j>=0;j--){
            if(start_group[i].module_array[j].exit)
                start_group[i].module_array[j].exit();
        }
    }
    return ret;
}

int eh_global_init( void ){
    interior_init();
    module_group_init();
    return 0;
}





