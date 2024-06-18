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
#include "eh_rbtree.h"
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
typedef void*                               eh_epoll_t;
typedef struct eh_epoll_slot                eh_epoll_slot_t;
#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */

extern eh_clock_t  (*_get_clock_monotonic_time)(void);
extern eh_clock_t  _clocks_per_sec;

enum EH_TASK_STATE{
    EH_TASK_STATE_READY,                            /* 就绪状态 */
    EH_TASK_STATE_RUNING,                           /* 运行状态 */
    EH_TASK_STATE_WAIT,                             /* 等待状态 */
    EH_TASK_STATE_FINISH,                           /* 结束状态 */
};

enum EH_EPOLL_AFFAIR{
    EH_EPOLL_AFFAIR_EVENT_TRIGGER,
    EH_EPOLL_AFFAIR_ERROR,
};

struct eh_event_type{
    const char                          *name;
};

struct eh_event{
    struct eh_list_head                 receptor_list_head;    /* 事件产生时的受体链表 */
    const struct eh_event_type          *type;
};

struct eh_epoll_slot{
    eh_event_t                          *event;
    void                                *userdata;
    enum EH_EPOLL_AFFAIR                affair;
};


struct eh_platform_port_param{
    void                                (*global_lock)(uint32_t *state);                            /* 上锁， */
    void                                (*global_unlock)(uint32_t state);                           /* 解锁 */
    eh_clock_t                          clocks_per_sec;
    eh_clock_t                          (*get_clock_monotonic_time)(void);                          /* 系统单调时钟的微秒数 */
    void                                (*idle_or_extern_event_handler);                            /* 用户处理空闲和外部事件 */
    void                                (*idle_break)(void);                                        /* 调用此函数通知"platform"从idle_or_extern_event_handler返回 */
    void*                               (*malloc)(size_t size);
    void                                (*free)(void* ptr);
};

#define eh_get_clock_monotonic_time()   (_get_clock_monotonic_time())

/**
 * @brief   微秒转换为时钟数
 * @param   _msec   毫秒数
 * return   eh_clock_t
 */
#define  eh_msec_to_clock(_msec) ({                                                                 \
        eh_clock_t clock = ((eh_msec_t)(_msec)/1000) * _clocks_per_sec;                             \
        clock += (((_msec)%1000) * _clocks_per_sec)/1000;                                           \
        clock ? clock : !!(_msec);                                                                  \
    })

/**
 * @brief   微秒转换为时钟数
 * @param   _usec   微秒数
 * return   eh_clock_t
 */
#define  eh_usec_to_clock(_usec) ({                                                                 \
        eh_clock_t clock = ((eh_usec_t)((_usec)/1000000) * _clocks_per_sec);                        \
        clock += (((_usec)%1000000) * _clocks_per_sec)/1000000;                                     \
        clock ? clock : !!(_usec);                                                                  \
    })

/**
 * @brief   时钟数转换为毫秒数
 * @param   _clock  时钟数
 * return   eh_msec_t
 */
#define  eh_clock_to_msec(_clock) ({                                                                \
        eh_msec_t msec = ((eh_msec_t)(((eh_clock_t)(_clock)/_clocks_per_sec) * 1000));              \
        msec += (((eh_clock_t)(_clock)%_clocks_per_sec) * 1000)/_clocks_per_sec;                    \
        msec ? msec : !!(_clock);                                                                   \
    })

/**
 * @brief   时钟数转换为微秒数
 * @param   _clock  时钟数
 * return   eh_usec_t
 */
#define  eh_clock_to_usec(_clock) ({                                                                \
        eh_msec_t msec = ((eh_msec_t)(((eh_clock_t)(_clock)/_clocks_per_sec) * 1000000));           \
        msec += (((eh_clock_t)(_clock)%_clocks_per_sec) * 1000000)/_clocks_per_sec;                 \
        msec ? msec : !!(_clock);                                                                   \
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
 * @brief                           事件等待,若事件e在此函数调用前发生，将无法捕获到事件(事件无队列)
 *                                  必须等待condition返回true才进行返回，若condition == NULL 那么就等待信号发生就直接返回
 * @param  e                        事件实例指针
 * @param  arv                      condition的参数
 * @param  condition                条件函数
 * @param  timeout                  超时时间,EH_TIMER_FOREVER为永不超时 因为事件无队列，
 *                              当condition为NULL时，超时时间为0将毫无意义，若想使用0，请使用epoll监听事件
 * @return int 
 */
extern int __async__ eh_event_wait_condition_timeout(eh_event_t *e, void* arg, bool (*condition)(void* arg), eh_sclock_t timeout);

/**
 * @brief                           事件等待,若事件e在此函数调用前发生，将无法捕获到事件(事件无队列)
 * @param  e                        事件实例指针
 * @param  timeout                  超时时间,EH_TIMER_FOREVER为永不超时 因为事件无队列，
                                所以超时时间为0将毫无意义，若想使用0，请使用epoll监听事件
 * @return int 
 */
static inline int __async__ eh_event_wait_timeout(eh_event_t *e, eh_sclock_t timeout){
    return __await__ eh_event_wait_condition_timeout(e, NULL, NULL, timeout);
}

/**
 * @brief                   创建一个epoll句柄
 * @return eh_epoll_t 
 */
extern eh_epoll_t eh_epoll_new(void);


/**
 * @brief                   删除一个epoll句柄
 * @param  epoll            
 */
extern void eh_epoll_del(eh_epoll_t epoll);

/**
 * @brief                   为epoll添加一个被监视事件
 * @param  epoll            epoll句柄
 * @param  e                事件句柄
 * @param  userdata         当事件发生时，可将userdata通过wait传递出来
 * @return int
 */
extern int eh_epoll_add_event(eh_epoll_t epoll, eh_event_t *e, void *userdata);

/**
 * @brief                   为epoll删除一个被监视事件
 * @param  epoll            epoll句柄
 * @param  e                事件句柄
 * @return int 
 */
extern int eh_epoll_del_event(eh_epoll_t epoll,eh_event_t *e);

/**
 * @brief                   epoll事件等待
 * @param  epoll            epoll句柄
 * @param  epool_slot       epoll事件等待槽
 * @param  slot_size        epoll事件等待槽大小
 * @param  timeout          超时时间，当0则立即返回,当EH_TIMER_FOREVER永久等待，其他大于0的值则进行相应时间的等待
 * @return int              成功返回拿到event事件的个数，失败返回负数错误码
 */
extern int __async__ eh_epoll_wait(eh_epoll_t epoll,eh_epoll_slot_t *epool_slot, int slot_size, eh_sclock_t timeout);


/**
 * @brief                   定时器事件的封装，在在指定协程上睡眠 usec 微秒
 * @param  usec             微秒
 */
extern void __async__ eh_usleep(eh_usec_t usec);



/* @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ */
/* EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE */


/* @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ */
/* ########################################################## task 相关接口 ############################################################ */
/* @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ */

/**
 * @brief 让出当前任务
 * @return int 返回0
 */
extern void __async__ eh_task_yield(void);

/**
 * @brief                   使用静态方式创建一个协程任务
 * @param  name             任务名称
 * @param  stack            任务的静态栈
 * @param  stack_size       任务栈大小
 * @param  task_arg         任务参数
 * @param  task_function    任务执行函数
 * @return eh_task_t* 
 */
extern eh_task_t* eh_task_static_stack_create(const char *name, void *stack, uint32_t stack_size, void *task_arg, int (*task_function)(void*));

/**
 * @brief                   使用动态方式创建一个协程任务
 * @param  name             任务名称
 * @param  stack_size       任务栈大小
 * @param  task_arg         任务参数
 * @param  task_function    任务执行函数
 * @return eh_task_t* 
 */
extern eh_task_t* eh_task_create(const char *name, uint32_t stack_size, void *task_arg, int (*task_function)(void*));

/**
 * @brief                   退出任务
 * @param  ret              退出返回值
 */
extern void       eh_task_exit(int ret);

/**
 * @brief                   任务获取自己的任务句柄
 * @return eh_task_t*       返回当前的任务句柄
 */
extern eh_task_t* eh_task_self(void);

/**
 * @brief                   阻塞等待任务结束
 * @param  task             被等待的任务句柄
 * @param  ret              成功返回0
 * @return int 
 */
extern int __async__ eh_task_join(eh_task_t *task, int *ret, eh_sclock_t timeout);

/* @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ */
/* EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE */

/**
 * @brief  初始化event_hub
 *  1.考虑到配合系统多线程编程时在其他线程产生事件的操作，所以设计了互斥锁接口.
 *  2.给lock预留了state参数，考虑在裸机编程时，中断禁止时，需要记录获得锁前的中断状态，所以需要预留state参数。
 * @param  param                     初始化参数
 * @return int 
 */
extern int eh_global_init(void);

/**
 * @brief 反初始化event_hub
 */
extern void eh_global_exit(void);

/**
 * @brief 世界循环，调用后将开始进行协程的调度
 */
extern int eh_loop_run(void);

/**
 * @brief                   退出任务
 * @param  exit_code        退出返回值
 */
extern void eh_loop_exit(int exit_code);


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#include "eh_timer.h"

#endif // _EVENT_HUB_H_