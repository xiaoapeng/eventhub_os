/**
 * @file eh.h
 * @brief 主要的头文件实现
 * @author simon.xiaoapeng (simon.xiaoapeng@gmail.com)
 * @version 1.0
 * @date 2024-04-13
 * 
 * @copyright Copyright (c) 2024  simon.xiaoapeng@gmail.com
 * 
 * @par 修改日志:
 */


#ifndef _EVENT_HUB_H_
#define _EVENT_HUB_H_

#include <stdint.h>

#include "eh_config.h"
#include "eh_list.h"
#include "eh_error.h"
#include "eh_co.h"
#include "eh_module.h"

typedef struct eh                           eh_t;
typedef struct eh_platform_port_param       eh_platform_port_param_t;
typedef struct eh_event                     eh_event_t;
typedef struct eh_event_type                eh_event_type_t;
typedef uint64_t                            eh_usec_t;
typedef uint64_t                            eh_msec_t;
typedef uint64_t                            eh_clock_t;
typedef int64_t                             eh_sclock_t;
typedef struct eh_task                      eh_task_t;
typedef struct eh_event_receptor            eh_event_receptor_t;

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */

enum EH_TASK_STATE{
    EH_TASK_STATE_READY,                            /* 就绪状态 */
    EH_TASK_STATE_RUNING,                           /* 运行状态 */
    EH_TASK_STATE_WAIT,                             /* 等待状态 */
    EH_TASK_STATE_FINISH,                           /* 结束状态 */
};


struct eh_event_type{
    const char                          *name;
};

struct eh_event{
    struct eh_list_head                 receptor_list_head;    /* 事件产生时的受体链表 */
    const struct eh_event_type          *type;
};



struct eh_platform_port_param{
    void                                (*global_lock)(uint32_t *state);        /* 上锁， */
    void                                (*global_unlock)(uint32_t state);       /* 解锁 */
    eh_clock_t                          clocks_per_sec;
    eh_clock_t                          (*get_clock_monotonic_time)(void);      /* 系统单调时钟的微秒数 */
    void                                (*idle_or_extern_event_handler)(int is_wait_event);    /* 用户处理空闲和外部事件 */
    void                                (*expire_time_change)(int is_never_expire, eh_clock_t new_expire);     /* 定时器最近到期时间改变,当is_never_expire为1时，永不到期 */
    void*                               (*malloc)(size_t size);
    void                                (*free)(void* ptr);
};

#define EH_DEFINE_PLATFORM_PORT_PARAM(                                                  \
        _global_lock, _global_unlock,                                                   \
        _clocks_per_sec,                                                                \
        _get_clock_monotonic_time,                                                      \
        _idle_or_extern_event_handler,                                                  \
        expire_time_change,                                                             \
        _malloc,                                                                        \
        _free)                                                                          \
    eh_platform_port_param_t platform_port_param = {_global_lock,                       \
                                                    _global_unlock,                     \
                                                    _clocks_per_sec,                    \
                                                    _get_clock_monotonic_time,          \
                                                    _idle_or_extern_event_handler,      \
                                                    expire_time_change,                 \
                                                    _malloc,                            \
                                                    _free}

#define eh_get_clock_monotonic_time()   (_global_eh.get_clock_monotonic_time())

/**
 * @brief   微秒转换为时钟数
 * @param   _msec   毫秒数
 * return   eh_clock_t
 */
#define  eh_msec_to_clock(_msec) ({                                                     \
        eh_clock_t clock = ((eh_msec_t)(_msec) * _global_eh.clocks_per_sec)/1000;       \
        clock ? clock : 1;                                                              \
    })

/**
 * @brief   微秒转换为时钟数
 * @param   _usec   微秒数
 * return   eh_clock_t
 */
#define  eh_usec_to_clock(_usec) ({                                                     \
        eh_clock_t clock = ((eh_msec_t)(_usec) * _global_eh.clocks_per_sec)/1000000     \
        clock ? clock : 1;                                                              \
    })


/* @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ */
/* ########################################################## event 相关接口 ############################################################ */
/* @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ */


/**
 * @brief                           事件初始化
 * @param  e                        事件实例指针
 * @param  static_const_name        事件名称，可为NULL
 * @return int                      见eh_error.h
 */
extern int eh_event_init(eh_event_t *e, const eh_event_type_t* type);

/**
 * @brief                           唤醒所有监听该事件的任务
 *                                  并将其剔除等待队列，一般在释放event实例前进行调用
 * @param  e                        事件实例指针
 */
extern void eh_event_clean(eh_event_t *e);

/**
 * @brief                           事件通知,唤醒所有监听该事件的任务
 * @param  e                        事件实例指针
 * @return int 
 */
extern int eh_event_notify(eh_event_t *e);

/**
 * @brief                           事件等待
 * @param  e                        事件实例指针
 * @param  timeout                  超时时间,禁止为0,若想为0，请使用epoll,EH_TIMER_FOREVER为永不超时
 * @return int 
 */
int __async__ eh_event_wait_timeout(eh_event_t *e, eh_sclock_t timeout);

/* @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ */
/* EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE */


/* @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ */
/* ########################################################## task 相关接口 ############################################################ */
/* @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ */

/**
 * @brief                   使用静态方式创建一个协程任务
 * @param  name             任务名称
 * @param  stack            任务的静态栈
 * @param  stack_size       任务栈大小
 * @param  task_arg         任务参数
 * @param  task_function    任务执行函数
 * @return eh_task_t* 
 */
extern eh_task_t* eh_create_static_stack_task(const char *name, void *stack, uint32_t stack_size, void *task_arg, int (*task_function)(void*));
extern eh_task_t* eh_create_task(const char *name, uint32_t stack_size, void *task_arg, int (*task_function)(void*));
extern int        eh_task_exit(int ret);
extern eh_task_t* eh_self_task(void);
extern int        eh_task_join(eh_task_t *task, int *ret);


/* @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ */
/* EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE */

/**
 * @brief  初始化event_hub句柄
 *  1.考虑到配合系统多线程编程时在其他线程产生事件的操作，所以设计了互斥锁接口.
 *  2.给lock预留了state参数，考虑在裸机编程时，中断禁止时，需要记录获得锁前的中断状态，所以需要预留state参数。
 * @param  param                     初始化参数
 * @return int 
 */
extern int eh_global_init(void);

extern void eh_loop_run(void);


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#include "eh_timer.h"

#endif // _EVENT_HUB_H_