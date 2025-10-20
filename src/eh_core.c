/**
 * @file event_hub.c
 * @brief 核心实现
 * @author simon.xiaoapeng (simon.xiaoapeng@gmail.com)
 * @date 2024-04-14
 *  
 * @copyright Copyright (c) 2024  simon.xiaoapeng@gmail.com
 * 
 */

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <eh.h>
#include <eh_debug.h>
#include <eh_mem.h>
#include <eh_event.h>
#include <eh_platform.h>
#include <eh_interior.h>
#include <eh_timer.h>

#define EH_STACK_PAD_BYTE               0xFF

eh_t _global_eh;

static struct  eh_task s_main_task;

static void _task_auto_destruct(void *arg);

static eh_loop_poll_task_t auto_destruct_task = {
    .poll_task = _task_auto_destruct,
    .arg = NULL,
    .list_node = EH_LIST_HEAD_INIT(auto_destruct_task.list_node),
};



static int _task_entry(void* arg){
    (void) arg;
    eh_task_t *current_task = eh_task_get_current();
    current_task->task_ret = current_task->task_function(current_task->task_arg);
    current_task->state = EH_TASK_STATE_FINISH;
    eh_event_notify(&current_task->event);
    __await eh_task_next();
    return current_task->task_ret;
}


static bool _task_is_finish(void *arg){
    eh_task_t *task = arg;
    return task->state == EH_TASK_STATE_FINISH;
}


static void _task_destroy(eh_task_t *task){
    eh_save_state_t state;
    if(task->system_data && task->system_data_destruct_function)
        task->system_data_destruct_function(task);
    eh_event_clean(&task->event);
    state = eh_enter_critical();
    eh_list_del(&task->task_list_node);
    eh_exit_critical(state);
    if(!task->is_static_stack)
        eh_free(task->stack);
    eh_free(task);
}

static void _task_auto_destruct(void *arg){
    (void) arg;
    eh_t *eh = eh_get_global_handle();
    eh_task_t *pos,*n;
    eh_list_for_each_entry_safe(pos, n, &eh->task_finish_auto_destruct_list_head, task_list_node)
        _task_destroy(pos);
    eh_loop_poll_task_del(&auto_destruct_task);
}

static void _clear(void){
    eh_t *eh = eh_get_global_handle();
    eh_task_t *pos,*n;

    eh_list_for_each_entry_safe(pos, n, &eh->task_finish_auto_destruct_list_head, task_list_node)
        _task_destroy(pos);
}

static inline void _eh_poll_run(void){
    eh_loop_poll_task_t *pos,*n;
    eh_t *eh = eh_get_global_handle();
    eh_list_for_each_prev_entry_safe(pos, n, &eh->loop_poll_task_head, list_node){
        if(pos->poll_task)
            pos->poll_task(pos->arg);
    }
}

static void eh_poll(void){
    _eh_poll_run();

    /* 调用用户外部处理函数 */
    eh_idle_or_extern_event_handler();
    
    /* 检查定时器是否超时，超时后进行相关事件通知 */
    eh_timer_check();
}


eh_clock_t eh_msec_to_clock(eh_msec_t msec){
    eh_clock_t clock = ((msec)/1000ULL) * (eh_clock_t)EH_CONFIG_CLOCKS_PER_SEC;
    clock += ((((msec)%1000ULL)) * (eh_clock_t)EH_CONFIG_CLOCKS_PER_SEC)/1000ULL;
    return clock ? clock : !!(msec);
}

eh_clock_t eh_usec_to_clock(eh_usec_t usec){
    eh_clock_t clock = ((usec)/1000000ULL) * (eh_clock_t)EH_CONFIG_CLOCKS_PER_SEC;
    clock += ((((usec)%1000000ULL)) * (eh_clock_t)EH_CONFIG_CLOCKS_PER_SEC)/1000000ULL;
    return clock ? clock : !!(usec);
}

eh_msec_t eh_clock_to_msec(eh_clock_t clock){
    eh_msec_t msec = ((eh_msec_t)((clock/(eh_clock_t)EH_CONFIG_CLOCKS_PER_SEC) * 1000ULL));
    msec += ((clock%(eh_clock_t)EH_CONFIG_CLOCKS_PER_SEC) * 1000ULL)/(eh_clock_t)EH_CONFIG_CLOCKS_PER_SEC;
    return msec ? msec : !!(clock);
}


eh_usec_t eh_clock_to_usec(eh_clock_t clock){
    eh_usec_t usec = ((eh_usec_t)((clock/(eh_clock_t)EH_CONFIG_CLOCKS_PER_SEC) * 1000000ULL));
    usec += ((clock%(eh_clock_t)EH_CONFIG_CLOCKS_PER_SEC) * 1000000ULL)/(eh_clock_t)EH_CONFIG_CLOCKS_PER_SEC;
    return usec ? usec : !!(clock);
}


void __async eh_task_next(void){
    eh_t *eh = eh_get_global_handle();
    eh_save_state_t state;
    eh_task_t *current_task = eh_task_get_current();
    eh_task_t *to;
    eh_clock_t idle_time = 0;
    
    if( eh->dispatch_cnt % EH_CONFIG_TASK_DISPATCH_CNT_PER_POLL == 0 ){
        eh_poll();
    }
    

    for(;;){
        state = eh_enter_critical();
        if(!eh_list_empty(&current_task->task_list_node))
        {
            if(idle_time)
                eh->idle_time += (eh_get_clock_monotonic_time() - idle_time);
            break;
        }
        eh_exit_critical(state);

        /* task list 为空，说明已经没有任务了*/
        eh_poll();
        if( current_task->state == EH_TASK_STATE_RUNING || 
            current_task->state == EH_TASK_STATE_READY ){
            current_task->state = EH_TASK_STATE_RUNING;
            if(idle_time)
                eh->idle_time += (eh_get_clock_monotonic_time() - idle_time);
            return ;
        }
        
        if(idle_time == 0)
            idle_time = eh_get_clock_monotonic_time();
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
            if(current_task->is_auto_destruct){
                eh_list_move_tail(&current_task->task_list_node, &eh->task_finish_auto_destruct_list_head);
                eh_loop_poll_task_add(&auto_destruct_task);
            }else{
                eh_list_move_tail(&current_task->task_list_node, &eh->task_finish_list_head);
            }
            break;
    }
    to->state = EH_TASK_STATE_RUNING;
    eh_exit_critical(state);
    co_context_swap(NULL, &current_task->context, &to->context);
    eh->dispatch_cnt++;

}

unsigned int eh_task_dispatch_cnt(void){
    return eh_read_once(eh_get_global_handle()->dispatch_cnt);
}

eh_clock_t eh_task_idle_time(void){
    return eh_read_once(eh_get_global_handle()->idle_time);
}

void eh_task_wake_up(eh_task_t *wakeup_task){
    eh_save_state_t state;
    state = eh_enter_critical();
    if(wakeup_task->state != EH_TASK_STATE_WAIT)
        goto out;
    eh_idle_break();
    wakeup_task->state = EH_TASK_STATE_READY;
    if(wakeup_task == eh_task_get_current())
        goto out;
    if(wakeup_task->is_system_task){
        eh_list_move(&wakeup_task->task_list_node, &eh_task_get_current()->task_list_node);
    }else{
        eh_list_move_tail(&wakeup_task->task_list_node, &eh_task_get_current()->task_list_node);
    }
out:
    eh_exit_critical(state);
}

static void _eh_task_struct_init(eh_task_t *task, int is_static_stack, uint32_t flags,
    void *stack, unsigned long stack_size, void *task_arg, int (*task_function)(void*)){
    eh_list_head_init(&task->task_list_node);
    task->task_function = task_function;
    task->task_arg = task_arg;
    task->stack = stack;
    task->stack_size = stack_size;
    if(task_function){
        task->context = co_context_make(stack, ((uint8_t*)stack) + stack_size, _task_entry);
    }else{
        task->context = NULL;
    }
    task->task_ret = 0;
    task->state = EH_TASK_STATE_WAIT;
    task->flags = flags & EH_TASK_FLAGS_MASK;
    task->is_static_stack = !!is_static_stack;
    task->system_data = NULL;
    task->system_data_destruct_function = NULL;
    eh_event_init(&task->event);
}

static eh_task_t* _eh_task_create_stack(const char *name,int is_static_stack, uint32_t flags,
            void *stack, unsigned long stack_size, void *task_arg, int (*task_function)(void*)){
    eh_task_t *task = (eh_task_t *)eh_malloc(sizeof(eh_task_t) + strlen(name) + 1);
    if(task == NULL) return eh_error_to_ptr(EH_RET_MALLOC_ERROR);
    memset(stack, EH_STACK_PAD_BYTE, stack_size);
    task->name = (char*)(task + 1);
    strcpy((char*)task->name, name);
    _eh_task_struct_init(task, is_static_stack, flags, stack, stack_size, task_arg, task_function);
    eh_task_wake_up(task);
    return task;
}




void __async eh_task_yield(void){
    eh_task_next();
}

eh_task_t* eh_task_static_stack_create(const char *name,uint32_t flags, void *stack, unsigned long stack_size, void *task_arg, int (*task_function)(void*)){
    return _eh_task_create_stack(name, 1, flags, stack, stack_size, task_arg, task_function);
}

eh_task_t* eh_task_create(const char *name, uint32_t flags,  unsigned long stack_size, void *task_arg, int (*task_function)(void*)){
    eh_task_t *task;
    void *stack = eh_malloc(stack_size);
    if(stack == NULL) return eh_error_to_ptr(EH_RET_MALLOC_ERROR);
    task = _eh_task_create_stack(name, 0, flags, stack, stack_size, task_arg, task_function);
    if(eh_ptr_to_error(task) < 0)
        eh_free(stack);
    return task;
}

int __async  eh_task_join(eh_task_t *task, int *ret, eh_sclock_t timeout){
    int wait_ret;
    eh_param_assert( task != NULL );

    wait_ret = __await eh_event_wait_condition_timeout(&task->event, task, _task_is_finish, timeout);
    if(wait_ret < 0)
        return wait_ret;

    if(ret)
        *ret = task->task_ret;
    _task_destroy(task);
    return EH_RET_OK;
}

void eh_task_destroy(eh_task_t *task){
    _task_destroy(task);
}

void eh_task_exit(int ret){
    eh_save_state_t state;
    eh_task_t *task = eh_task_get_current();
    state = eh_enter_critical();
    task->task_ret = ret;
    if(task != eh_get_global_handle()->main_task){
        task->state = EH_TASK_STATE_FINISH;
        eh_event_notify(&task->event);
    }
    eh_exit_critical(state);
    eh_task_next();
}

void eh_task_sta(const eh_task_t *task, eh_task_sta_t *sta){
    unsigned long i;
    sta->task_name = task->name;
    sta->state = task->state;
    sta->stack_size = task->stack_size;
    sta->stack_min_ever_free_size_level = 0;
    sta->stack = task->stack;
    //EH_STACK_PAD_BYTE
    for(i = 0; i < task->stack_size; i++){
        if(((uint8_t*)task->stack)[i] != (uint8_t)EH_STACK_PAD_BYTE)
            break;
    }
    sta->stack_min_ever_free_size_level = i;
}

eh_task_t* eh_task_self(void){
    return eh_task_get_current();
}


eh_task_t* eh_task_main(void){
    return eh_task_get_main();
}


const char* eh_task_name(const eh_task_t *task){
    return task->name;
}

eh_sclock_t eh_get_loop_idle_time(void){
    eh_save_state_t state;
    eh_sclock_t half_time;
    state = eh_enter_critical();
    half_time = (!eh_list_empty(&eh_task_get_current()->task_list_node)) || 
                 eh_task_get_current()->state <= EH_TASK_STATE_RUNING ?  0 
                    : eh_timer_get_first_remaining_time_on_lock();
    eh_exit_critical(state);
    return half_time;
}



void eh_loop_poll_task_add(eh_loop_poll_task_t *poll_task){
    eh_t *eh = eh_get_global_handle();
    if(eh_list_empty(&poll_task->list_node))
        eh_list_add_tail(&poll_task->list_node, &eh->loop_poll_task_head);
}

static int interior_init(void){
    eh_t *eh = eh_get_global_handle();
    bzero(eh, sizeof(eh_t));
    bzero(&s_main_task,  sizeof(struct  eh_task));
    
    eh_list_head_init(&eh->task_wait_list_head);
    eh_list_head_init(&eh->task_finish_list_head);
    eh_list_head_init(&eh->loop_poll_task_head);
    eh_list_head_init(&eh->task_finish_auto_destruct_list_head);

    eh->dispatch_cnt = 0;
    eh->idle_time = 0;
    eh->eh_init_fini_array = (struct eh_module*)eh_module_section_begin();
    eh->eh_init_fini_array_len = ((struct eh_module*)eh_module_section_end() - (struct eh_module*)eh_module_section_begin());
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
            if(ret < 0){
                eh_errfl("%0#p init failed ret = %d", eh_init_fini_array[i].init, ret);
                goto init_error;
            }
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
    return module_group_init();
}

void eh_global_exit(void){
    _clear();
    module_group_exit();
}

static __init int main_task_init(void){
    eh_t *eh = eh_get_global_handle();
    eh->main_task = &s_main_task;
    eh->current_task = &s_main_task;
    
    s_main_task.name = "main_task";
    _eh_task_struct_init(&s_main_task, 1, 0, NULL, 0, NULL, NULL);
    s_main_task.state = EH_TASK_STATE_RUNING;

    return 0;
}

static __exit void main_task_exit(void){
    /* 清理main 任务的一些资源 */
    if(s_main_task.system_data && s_main_task.system_data_destruct_function)
        s_main_task.system_data_destruct_function(&s_main_task);
}

eh_main_task_module_export(main_task_init, main_task_exit);



