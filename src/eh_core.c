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
#include "eh_rbtree.h"
#include "eh_timer.h"
#include "eh_types.h"


eh_t _global_eh;
eh_clock_t   (*_get_clock_monotonic_time)(void);
eh_clock_t  _clocks_per_sec;


static const eh_event_type_t task_event_type = {
    .name = "task_event"
};

static struct  eh_task s_main_task;

static int _task_entry(void* arg){
    (void) arg;
    eh_task_t *current_task = eh_task_get_current();
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
    eh_lock(&state);
    eh_list_del(&task->task_list_node);
    eh_unlock(state);
    if(!task->is_static_stack)
        eh_free(task->stack);
    eh_free(task);
}

static void _clear(void){
    eh_t *eh = eh_get_global_handle();
    eh_task_t *pos,*n;

    /* 清理任务 */
    eh_list_for_each_entry_safe(pos, n, &eh->task_finish_list_head, task_list_node)
        _task_destroy(pos);
    eh_list_for_each_entry_safe(pos, n, &eh->current_task->task_list_node, task_list_node)
        _task_destroy(pos);
    eh_list_for_each_entry_safe(pos, n, &eh->task_wait_list_head, task_list_node)
        _task_destroy(pos);
    
}

/**
 * @brief             进行下一个任务的调度，调度成功返回0，调度失败返回-1
 * @return int        -1:调度失败 0:调度成功
 */
int __async__ eh_task_next(void){
    eh_t *eh = eh_get_global_handle();
    uint32_t state;
    eh_task_t *current_task = eh_task_get_current();
    eh_task_t *to;
    
    eh_lock(&state);
    if(eh_list_empty(&current_task->task_list_node)){
        current_task->state = EH_TASK_STATE_RUNING;
        eh_unlock(state);
        return EH_RET_SCHEDULING_ERROR;
    }
    to = eh_list_entry(current_task->task_list_node.next, eh_task_t, task_list_node);
    eh_task_set_current(to);
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
    eh_unlock(state);
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
    eh_lock(&state);
    if(wakeup_task->state != EH_TASK_STATE_WAIT)
        goto out;
    wakeup_task->state = EH_TASK_STATE_READY;
    eh_list_move_tail(&wakeup_task->task_list_node, &eh->current_task->task_list_node);
out:
    eh_unlock(state);
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
    eh_task_t *task = eh_task_get_current();
    if(task == eh_get_global_handle()->main_task)
        return ;
    eh_lock(&state);
    task->task_ret = ret;
    task->state = EH_TASK_STATE_FINISH;
    eh_unlock(state);
    eh_task_next();
}

/**
 * @brief                   任务获取自己的任务句柄
 * @return eh_task_t*       返回当前的任务句柄
 */
eh_task_t* eh_task_self(void){
    return eh_task_get_current();
}


void eh_loop_exit(int exit_code){
    eh_get_global_handle()->loop_stop_code = exit_code;
    eh_get_global_handle()->stop_flag = true;
    eh_task_exit(exit_code);
}

int eh_loop_run(void){
    eh_t *eh = eh_get_global_handle();
    eh->state = EH_SCHEDULER_STATE_RUN;
    eh->stop_flag = false;

    for(;;){

        /* 检查定时器是否超时，超时后进行相关事件通知 */
        eh_timer_check();
        
        /* 检查是否有信号需要处理 */
        
        /* 进行调度 */
        __await__ eh_task_next();

        if(unlikely(eh->stop_flag))
            break;

        /* 调用用户外部处理函数 */
        eh->state = EH_SCHEDULER_STATE_IDLE_OR_EVENT_HANDLER;
        eh->idle_or_extern_event_handler();
        eh->state = EH_SCHEDULER_STATE_RUN;
    }
    eh->state = EH_SCHEDULER_STATE_EXIT;
    return eh->loop_stop_code;
}

/**
 * @brief                   注册平台端口参数
 * @param  param            平台相关接口
 * @return int 
 */
int eh_platform_register_port_param(const eh_platform_port_param_t *param){
    eh_t *eh = eh_get_global_handle();
    eh_param_assert(param);
    eh_param_assert(param->get_clock_monotonic_time);
    eh_param_assert(param->idle_or_extern_event_handler);
    eh_param_assert(param->idle_break);
    eh_param_assert(param->clocks_per_sec >= 100);
    eh_param_assert(param->malloc);
    eh_param_assert(param->free);

    _get_clock_monotonic_time = param->get_clock_monotonic_time;
    _clocks_per_sec = param->clocks_per_sec;

    eh->global_lock = param->global_lock;
    eh->global_unlock = param->global_unlock;
    eh->get_clock_monotonic_time = param->get_clock_monotonic_time;
    eh->idle_or_extern_event_handler = param->idle_or_extern_event_handler;
    eh->idle_break = param->idle_break;
    eh->malloc = param->malloc;
    eh->free = param->free;
    return 0;
}

static int interior_init(void){
    eh_t *eh = eh_get_global_handle();
    bzero(eh, sizeof(eh_t));
    bzero(&s_main_task,  sizeof(struct  eh_task));
    
    eh_list_head_init(&eh->task_wait_list_head);
    eh_list_head_init(&eh->task_finish_list_head);

    eh->state = EH_SCHEDULER_STATE_INIT;

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

    eh->eh_init_fini_array = (struct eh_module*)eh_modeule_section_begin();
    eh->eh_init_fini_array_len = ((struct eh_module*)eh_modeule_section_end() - (struct eh_module*)eh_modeule_section_begin());
    return 0;
}

static int  module_group_init(void){
    eh_t *eh = eh_get_global_handle();
    struct eh_module  *eh_init_fini_array;
    long i,len;
    int ret = 0;
    len = eh->eh_init_fini_array_len;
    eh_init_fini_array = eh->eh_init_fini_array;
    for(i=0;i<len;i++){
        if(eh_init_fini_array[i].init){
            ret = eh_init_fini_array[i].init();
            if(ret < 0)
                goto init_error;
        }
    }

    return ret;
init_error:
    for(i=i-1;i>=0;i--){
        if(eh_init_fini_array[i].exit)
            eh_init_fini_array[i].exit();
    }
    return ret;
}

static void module_group_exit(void){
    eh_t *eh = eh_get_global_handle();
    struct eh_module  *eh_init_fini_array;
    long i;
    i = eh->eh_init_fini_array_len;
    eh_init_fini_array = eh->eh_init_fini_array;

    for(i=i-1;i>=0;i--){
        if(eh_init_fini_array[i].exit)
            eh_init_fini_array[i].exit();
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





