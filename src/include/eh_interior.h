/**
 * @file eh_interior.h
 * @brief  内部进行使用的定义
 * @author simon.xiaoapeng (simon.xiaoapeng@gmail.com)
 * @version 1.0
 * @date 2024-05-10
 * 
 * @copyright Copyright (c) 2024  simon.xiaoapeng@gmail.com
 * 
 * @par 修改日志:
 */

#ifndef _EH_INTERIOR_H_
#define _EH_INTERIOR_H_
#include <stdint.h>

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */

enum EH_SCHEDULER_STATE{
    EH_SCHEDULER_STATE_ON_INIT,                     /* 未初始化状态 */
    EH_SCHEDULER_STATE_INIT,
    EH_SCHEDULER_STATE_RUN,                         /* 初始化状态 */
    EH_SCHEDULER_STATE_IDLE_OR_EVENT_HANDLER,       /* 等待状态 */
    EH_SCHEDULER_STATE_ERROR,                       /* 错误状态 */
    EH_SCHEDULER_STATE_EXIT,                        /* 退出状态 */
};



struct eh{
    struct      eh_list_head             task_wait_list_head;                                   /* 等待中的任务列表 */
    struct      eh_list_head             task_finish_list_head;                                 /* 完成待销毁的任务列表 */
    struct      eh_list_head             timer_list_head;                                       /* 系统定时器列表 */
    enum        EH_SCHEDULER_STATE       state;
    struct      eh_task                  *current_task;                                         /* 当前被调度的任务 */
    struct      eh_task                  *main_task;                                            /* 系统栈任务 */
    eh_clock_t                           clocks_per_sec;
    void*                                (*malloc)(size_t size);
    void                                 (*free)(void* ptr);
    void                                 (*global_lock)(uint32_t *state);
    void                                 (*global_unlock)(uint32_t state);
    eh_usec_t                            (*get_clock_monotonic_time)(void);                     /* 系统单调时钟的次数 */
    void                                 (*idle_or_extern_event_handler)(int is_wait_event);    /* 用户处理空闲和外部事件 */
    void                                 (*expire_time_change)(int is_never_expire, eh_clock_t new_expire);     /* 定时器最近到期时间改变,当is_never_expire为1时，永不到期 */
    struct module_group                  module_group[EH_MODEULE_GROUP_MAX_CNT];
};

/* 事件接收器  */
struct eh_event_receptor{
    struct eh_list_head                 list_node;
    struct eh_task                      *wakeup_task;          /* 被唤醒的任务            */
    uint32_t                            notify_cnt;            /* 事件通告次数            */
};

struct eh_task{
    const char                          *name;               
    struct eh_list_head                 task_list_node;          /* 任务链表,可被挂载到就绪，等待，完成等链表上 */
    struct eh_list_head                 epoll_list_node;         /* epoll中相关句柄，保存下来供任务释放时一同释放 */
    int                                 (*task_function)(void*); /* 任务函数 */
    void                                *task_arg;               /* 任务相关参数 */
    void                                *stack_top;              /* 协程栈顶 */
    uint32_t                            stack_size;              /* 任务栈大小 */
    context_t                           context;                 /* 协程上下文 */
    int                                 task_ret;                /* 任务返回值 */
    enum EH_TASK_STATE                  state;                   /* 任务运行状态*/
    eh_event_t                          event;                   /* 任务相关事件，任务退出 */
    union{
        uint32_t                        flags;
        struct{
            uint32_t                    is_static_stack;          /* 是否是静态栈 */
        };
    };
    
};

struct eh_event_epoll_receptor{
    struct eh_list_head                 list_node;
    struct eh_event_receptor            receptor;
    eh_event_t                          *event;
}

struct eh_epoll{
    struct eh_list_head                 list_node;
    struct eh_list_head                 receptor_list_head;
}

extern eh_t _global_eh;

/* ######################################################################################################################## */

/**
 * @brief 定时器检查函数
 */
extern void _eh_timer_check(void);

/**
 * @brief               加锁
 * @param   state_ptr   保存状态的指针
 */
#define _eh_lock(state_ptr)                                         \
    do{                                                             \
        if(_global_eh.global_lock && _global_eh.global_unlock)      \
            _global_eh.global_lock(state_ptr);                      \
    }while(0)

/**
 * @brief               解锁
 * @param   state       加锁时保存的状态
 */
#define _eh_unlock(state)                                           \
    do{                                                             \
        if(_global_eh.global_lock && _global_eh.global_unlock)      \
            _global_eh.global_unlock(state);                        \
    }while(0)

/**
 * @brief               获取全局句柄
 * @return eh_t*        全局句柄
 */
#define eh_get_global_handle() (&_global_eh)

/**
 * @brief               动态内存分配
 * @return              void*
 */
#define eh_malloc(size)    (eh_get_global_handle()->malloc((size)))

/**
 * @brief               内存释放
 */
#define eh_free(ptr)       (eh_get_global_handle()->free((ptr)))

/**
 * @brief               初始化事件接收器
 * @param   receptor    事件接收器
 * @param   wakeup_task 事件接收时被唤醒的任务
 */
#define eh_event_receptor_init(receptor, _wakeup_task)                  \
    do{                                                                 \
        INIT_EH_LIST_HEAD(&(receptor)->list_node);                      \
        (receptor)->wakeup_task = _wakeup_task;                         \
        (receptor)->notify_cnt = 0;                                     \
    }while(0)

/**
 * @brief               给事件添加接收器
 * @param   e           事件
 * @param   receptor    事件接收器
 */
#define eh_event_add_receptor_no_lock(e, receptor)   eh_list_add_tail(&(receptor)->list_node, &(e)->receptor_list_head)

/**
 * @brief              移除事件接收器
 * @param   receptor    事件接收器
 */
#define eh_event_remove_receptor_no_lock(receptor)   eh_list_del_init(&(receptor)->list_node)

/**
 * @brief               判断事件接收器是否分离 调用 eh_event_remove_receptor_no_lock后将返回真
 * @param   receptor    事件接收器
 */
#define eh_event_receptors_is_isolate(receptor)      eh_list_empty(&(receptor)->list_node)

/**
 * @brief               返回当前任务
 */
#define eh_get_current_task()                (_global_eh.current_task)
/**
 * @brief               设置当前任务
 * @param _current_task 被设置的任务对象
 */
#define eh_set_current_task(_current_task)   do{_global_eh.current_task = (_current_task);}while(0)

/**
 * @brief               获取当前任务状态
 */
#define eh_get_current_task_state()          (_global_eh.current_task->state)

/**
 * @brief               设置当前任务状态
 */
#define eh_set_current_task_state(_state)     do{_global_eh.current_task->state = (_state);}while(0)

/**
 * @brief               进行下一个任务的调度，调度成功返回0，调度失败返回-1
 * @return int          -1:调度失败 0:调度成功
 */
extern int __async__ eh_task_next(void);

/**
 * @brief                进行任务唤醒，配置目标任务为唤醒状态，后续将加入调度环
 * @param  wakeup_task   被唤醒的任务
 */
extern void eh_task_wake_up(eh_task_t *wakeup_task);


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif // _EH_INTERIOR_H_