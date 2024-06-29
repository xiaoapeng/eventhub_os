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

#include "eh.h"
#include <stdbool.h>
#include <stddef.h>
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

#define EH_EVENT_RECEPTOR_EPOLL                     0x00000001

struct eh{
    struct      eh_list_head             task_wait_list_head;                                   /* 等待中的任务列表 */
    struct      eh_list_head             task_finish_list_head;                                 /* 完成待销毁的任务列表 */
    enum        EH_SCHEDULER_STATE       state;
    struct      eh_task                  *current_task;                                         /* 当前被调度的任务 */
    struct      eh_task                  *main_task;                                            /* 系统栈任务 */
    struct eh_module                     *eh_init_fini_array;
    long                                 eh_init_fini_array_len;
    int                                  loop_stop_code;
    int                                  stop_flag;
};

/* 事件接收器  */
struct eh_event_receptor{
    struct eh_list_head                 list_node;
    struct eh_task                      *wakeup_task;           /* 被唤醒的任务            */
    union{
        uint32_t                            flags;
        struct{
            uint32_t                     trigger:1;
            uint32_t                     error:1;
        };
    };
    struct eh_epoll                     *epoll;
};

struct eh_task{
    const char                          *name;               
    struct eh_list_head                 task_list_node;          /* 任务链表,可被挂载到就绪，等待，完成等链表上 */
    int                                 (*task_function)(void*); /* 任务函数 */
    void                                *task_arg;               /* 任务相关参数 */
    void                                *stack;              /* 协程栈顶 */
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
    struct eh_rbtree_node               rb_node;
    struct eh_list_head                 pending_list_node;
    eh_event_t                          *event;
    void                                *userdata;
    struct eh_event_receptor            receptor;
};

struct eh_epoll{
    struct eh_list_head                 pending_list_head;
    struct eh_rbtree_root               all_receptor_tree;
    struct eh_task                      *wakeup_task;           /* 被唤醒的任务            */
};

extern eh_t _global_eh;

/* ######################################################################################################################## */

/**
 * @brief 定时器检查函数
 */
extern void eh_timer_check(void);

/**
 * @brief  获取第一个定时器剩余时间
 * @return eh_sclock_t 
 */
extern eh_sclock_t eh_timer_get_first_remaining_time_on_lock(void);

/**
 * @brief               获取全局句柄
 * @return eh_t*        全局句柄
 */
#define eh_get_global_handle() (&_global_eh)

#define __eh_event_receptor_init(receptor, _wakeup_task, _epoll)        \
    do{                                                                 \
        eh_list_head_init(&(receptor)->list_node);                      \
        (receptor)->wakeup_task = (_wakeup_task);                       \
        (receptor)->flags = 0;                                          \
        (receptor)->epoll = (_epoll);                                   \
    }while(0)

/**
 * @brief               初始化事件接收器
 * @param   receptor    事件接收器
 * @param   wakeup_task 事件接收时被唤醒的任务
 */
#define eh_event_receptor_init(receptor, _wakeup_task)                  \
    __eh_event_receptor_init((receptor), (_wakeup_task), NULL)

#define eh_event_receptor_epoll_init(receptor, _wakeup_task, epool)      \
    __eh_event_receptor_init((receptor), (_wakeup_task), epool)

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
#define eh_task_get_current()                (eh_get_global_handle()->current_task)
/**
 * @brief               设置当前任务
 * @param _current_task 被设置的任务对象
 */
#define eh_task_set_current(_current_task)   do{eh_get_global_handle()->current_task = (_current_task);}while(0)

/**
 * @brief               获取当前任务状态
 */
#define eh_task_get_current_state()          (eh_get_global_handle()->current_task->state)

/**
 * @brief               设置当前任务状态
 */
#define eh_task_set_current_state(_state)     do{eh_get_global_handle()->current_task->state = (_state);}while(0)

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