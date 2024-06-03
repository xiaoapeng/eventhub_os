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

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include "eh.h"
#include "eh_co.h"
#include "eh_config.h"
#include "eh_error.h"
#include "eh_interior.h"
#include "eh_list.h"
#include "eh_module.h"
#include "eh_timer.h"
#include "eh_types.h"


eh_t _global_eh;
eh_clock_t   (*_get_clock_monotonic_time)(void);
eh_clock_t  _clocks_per_sec;

eh_module_section_define();

static const eh_event_type_t task_event_type = {
    .name = "task_event"
};

static struct  eh_task s_main_task;

static int _task_entry(void* arg){
    (void) arg;
    eh_task_t *current_task = eh_get_current_task();
    current_task->task_ret = current_task->task_function(current_task->task_arg);
    current_task->state = EH_TASK_STATE_FINISH;
    eh_event_notify(&current_task->event);
    __await__ eh_task_next();
    return current_task->task_ret;
}

static bool _task_is_finish(void *arg){
    eh_task_t *task = arg;
    return task->state == EH_TASK_STATE_FINISH;
}

static void _task_destroy(eh_task_t *task){
    uint32_t state;
    eh_event_clean(&task->event);
    _eh_lock(&state);
    eh_list_del(&task->task_list_node);
    _eh_unlock(state);
    if(!task->is_static_stack)
        eh_free(task->stack);
    eh_free(task);
}

static void _clear(void){
    eh_t *eh = eh_get_global_handle();
    
    {
        eh_timer_event_t *pos,*n;
        /* 停止定时器 */
        eh_list_for_each_entry_safe(pos, n, &eh->timer_list_head, list_node)
            eh_timer_stop(pos);

    }
    {
        eh_task_t *pos,*n;
        /* 清理任务 */
        eh_list_for_each_entry_safe(pos, n, &eh->task_finish_list_head, task_list_node)
            _task_destroy(pos);
        eh_list_for_each_entry_safe(pos, n, &eh->current_task->task_list_node, task_list_node)
            _task_destroy(pos);
        eh_list_for_each_entry_safe(pos, n, &eh->task_wait_list_head, task_list_node)
            _task_destroy(pos);
    }
}

/**
 * @brief             进行下一个任务的调度，调度成功返回0，调度失败返回-1
 * @return int        -1:调度失败 0:调度成功
 */
int __async__ eh_task_next(void){
    eh_t *eh = eh_get_global_handle();
    uint32_t state;
    eh_task_t *current_task = eh_get_current_task();
    eh_task_t *to;
    
    _eh_lock(&state);
    if(eh_list_empty(&current_task->task_list_node)){
        current_task->state = EH_TASK_STATE_RUNING;
        _eh_unlock(state);
        return EH_RET_SCHEDULING_ERROR;
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

/**
 * @brief                   进行任务唤醒，配置目标任务为唤醒状态，后续将加入调度环
 * @param  wakeup_task      被唤醒的任务
 */
void eh_task_wake_up(eh_task_t *wakeup_task){
    eh_t *eh = eh_get_global_handle();
    uint32_t state;
    _eh_lock(&state);
    if(wakeup_task->state != EH_TASK_STATE_WAIT)
        goto out;
    wakeup_task->state = EH_TASK_STATE_READY;
    eh_list_move_tail(&wakeup_task->task_list_node, &eh->current_task->task_list_node);
out:
    _eh_unlock(state);
}


static eh_task_t* _eh_task_create_stack(const char *name,int is_static_stack, 
            void *stack, uint32_t stack_size, void *task_arg, int (*task_function)(void*)){
    eh_task_t *task = (eh_task_t *)eh_malloc(sizeof(eh_task_t) + strlen(name) + 1);
    if(task == NULL) return eh_error_to_ptr(EH_RET_MALLOC_ERROR);
    task->name = (char*)(task + 1);
    strcpy((char*)task->name, name);
    bzero(stack,stack_size);
    eh_list_head_init(&task->task_list_node);
    task->task_function = task_function;
    task->task_arg = task_arg;
    task->stack = stack;
    task->stack_size = stack_size;
    task->context = co_context_make(((uint8_t*)stack) + stack_size, _task_entry);
    task->task_ret = 0;
    task->state = EH_TASK_STATE_WAIT;
    task->is_static_stack = is_static_stack & 0x01;
    eh_event_init(&task->event, &task_event_type);
    eh_task_wake_up(task);
    return task;
}

/**
 * @brief                   使用静态方式创建一个协程任务
 * @param  name             任务名称
 * @param  stack            任务的静态栈
 * @param  stack_size       任务栈大小
 * @param  task_arg         任务参数
 * @param  task_function    任务执行函数
 * @return eh_task_t* 
 */
eh_task_t* eh_task_static_stack_create(const char *name, void *stack, uint32_t stack_size, void *task_arg, int (*task_function)(void*)){
    return _eh_task_create_stack(name, 1, stack, stack_size, task_arg, task_function);
}

/**
 * @brief                   使用动态方式创建一个协程任务
 * @param  name             任务名称
 * @param  stack_size       任务栈大小
 * @param  task_arg         任务参数
 * @param  task_function    任务执行函数
 * @return eh_task_t* 
 */
eh_task_t* eh_task_create(const char *name, uint32_t stack_size, void *task_arg, int (*task_function)(void*)){
    eh_task_t *task;
    void *stack = eh_malloc(stack_size);
    if(stack == NULL) return eh_error_to_ptr(EH_RET_MALLOC_ERROR);
    task = _eh_task_create_stack(name, 0, stack, stack_size, task_arg, task_function);
    if(eh_ptr_to_error(task) < 0)
        eh_free(stack);
    return task;
}

/**
 * @brief                   任务合并函数
 * @param  task             任务句柄
 * @param  ret              任务返回值
 * @param  timeout          合并等待时间
 * @return int              成功返回0
 */
int __async__  eh_task_join(eh_task_t *task, int *ret, eh_sclock_t timeout){
    int wait_ret;
    eh_param_assert( task != NULL );

    wait_ret = __await__ eh_event_wait_condition_timeout(&task->event, task, _task_is_finish, timeout);
    if(wait_ret < 0)
        return wait_ret;

    if(ret)
        *ret = task->task_ret;
    _task_destroy(task);
    return EH_RET_OK;
}
/**
 * @brief                   退出任务
 * @param  ret              退出返回值
 */
void  eh_task_exit(int ret){
    uint32_t state;
    eh_task_t *task = eh_get_current_task();
    if(task == eh_get_global_handle()->main_task)
        return ;
    _eh_lock(&state);
    task->task_ret = ret;
    task->state = EH_TASK_STATE_FINISH;
    _eh_unlock(state);
    eh_task_next();
}

/**
 * @brief                   任务获取自己的任务句柄
 * @return eh_task_t*       返回当前的任务句柄
 */
eh_task_t* eh_task_self(void){
    return eh_get_current_task();
}


void eh_loop_exit(int exit_code){
    eh_get_global_handle()->loop_stop_code = exit_code;
    eh_get_global_handle()->stop_flag = true;
    eh_task_exit(exit_code);
}

int eh_loop_run(void){
    int ret;
    eh_t *eh = eh_get_global_handle();
    eh->state = EH_SCHEDULER_STATE_RUN;
    eh->stop_flag = false;

    while(!eh->stop_flag){

        /* 检查定时器是否超时，超时后进行相关事件通知 */
        _eh_timer_check();
        
        /* 检查是否有信号需要处理 */
        
        /* 进行调度 */
        ret = __await__ eh_task_next();
        /* 调用用户外部处理函数 */
        eh->state = EH_SCHEDULER_STATE_IDLE_OR_EVENT_HANDLER;
        eh->idle_or_extern_event_handler(ret == 0 ? 0 : 1);
        eh->state = EH_SCHEDULER_STATE_RUN;

    }
    return eh->loop_stop_code;
}

static int interior_init(void){
    extern eh_platform_port_param_t platform_port_param;
    eh_t *eh = eh_get_global_handle();
    eh_param_assert(platform_port_param.get_clock_monotonic_time);
    eh_param_assert(platform_port_param.idle_or_extern_event_handler);
    eh_param_assert(platform_port_param.expire_time_change);
    eh_param_assert(platform_port_param.clocks_per_sec >= 100);
    bzero(eh, sizeof(eh_t));
    bzero(&s_main_task,  sizeof(struct  eh_task));

    _get_clock_monotonic_time = platform_port_param.get_clock_monotonic_time;
    _clocks_per_sec = platform_port_param.clocks_per_sec;

    eh->clocks_per_sec = platform_port_param.clocks_per_sec;
    
    eh_list_head_init(&eh->task_wait_list_head);
    eh_list_head_init(&eh->task_finish_list_head);
    eh_list_head_init(&eh->timer_list_head);

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
    eh_list_head_init( &s_main_task.task_list_node );
    s_main_task.task_function = NULL;
    s_main_task.task_arg = NULL;
    s_main_task.context = NULL;
    s_main_task.stack = NULL;
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

static void module_group_exit(void){
    struct module_group *start_group = eh_get_global_handle()->module_group;
    long i,j;
    for(i=EH_MODEULE_GROUP_MAX_CNT-1;i>=0;i--){
        for(j=start_group[i].module_cnt-1;j>=0;j--){
            if(start_group[i].module_array[j].exit){
                start_group[i].module_array[j].exit();
            }
        }
    }


}

int eh_global_init( void ){
    interior_init();
    module_group_init();
    return 0;
}

void eh_global_exit(void){
    _clear();
    module_group_exit();
}





