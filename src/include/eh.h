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

#if ((EH_EVENT_FIFO_CNT != 2) && (EH_EVENT_FIFO_CNT != 8) && (EH_EVENT_FIFO_CNT != 16) && (EH_EVENT_FIFO_CNT != 32))
    #error "EH_EVENT_FIFO_CNT must be 2, 8, 16, 32"
#endif

// typedef struct eh_signal        eh_signal_t;
// typedef struct eh_slot          eh_slot_t;
// typedef struct eh_signal_type   eh_signal_type_t;
typedef struct eh               eh_t;
typedef struct eh_init_param    eh_init_param_t;
typedef struct eh_event         eh_event_t;
typedef struct eh_event_type    eh_event_type_t;
typedef uint64_t                eh_usec_t;
typedef uint64_t                eh_msec_t;

enum EH_TASK_STATE{
    EH_TASK_STATE_READY,        /* 就绪状态 */
    EH_TASK_STATE_RUNING,       /* 运行状态 */
    EH_TASK_STATE_WAIT,         /* 等待状态 */
    EH_TASK_STATE_FINISH,       /* 结束状态 */
};

enum EH_PRIORITY{
    EH_PRIORITY_SYSTEM = 0,           /* 用户不得使用 */
    EH_PRIORITY_1,
    EH_PRIORITY_2,
    EH_PRIORITY_3,
    EH_PRIORITY_4,
    EH_PRIORITY_5,
    EH_PRIORITY_6,
    EH_PRIORITY_7,
    EH_PRIORITY_8,
    EH_PRIORITY_9,
    EH_PRIORITY_10,
    EH_PRIORITY_11,
    EH_PRIORITY_12,
    EH_PRIORITY_13,
    EH_PRIORITY_14,
    EH_PRIORITY_15,
    EH_PRIORITY_16,
    EH_PRIORITY_17,
    EH_PRIORITY_18,
    EH_PRIORITY_19,
    EH_PRIORITY_20,
    EH_PRIORITY_21,
    EH_PRIORITY_22,
    EH_PRIORITY_23,
    EH_PRIORITY_24,
    EH_PRIORITY_25,
    EH_PRIORITY_26,
    EH_PRIORITY_27,
    EH_PRIORITY_28,
    EH_PRIORITY_29,
    EH_PRIORITY_30,
    EH_PRIORITY_LOWEST,
};

enum EH_SCHEDULER_STATE{
    EH_SCHEDULER_STATE_FAULT,               /* 错误状态 */
    EH_SCHEDULER_STATE_INIT,                /* 初始化状态,还没进入调度状态 */
    EH_SCHEDULER_STATE_RUNNING,             /* 调度状态 */
};

struct eh{
    struct      eh_list_head             event_fifo_head[EH_EVENT_FIFO_CNT];
    struct      eh_list_head             task_list_head;
    struct      eh_list_head             timer_list_head;
    enum        EH_SCHEDULER_STATE       state;
    uint32_t                             event_bit_map;                         /* 事件快速查询位图,只有相应位全部都处理后才清除对应bit */
    void                                (*global_lock)(eh_t *eh, uint32_t *state);
    void                                (*global_unlock)(eh_t *eh, uint32_t state);
    eh_usec_t                           (*get_usec_monotonic_time)(void);               /* 系统单调时钟的微秒数 */
    void                                 *user_data;
};

struct eh_event_type{
    const char              *name;                          /* 事件的类型名称 */
    void                    (*destructor)(eh_event_t *e);   /* 事件自动析构的释放函数 */
    enum EH_PRIORITY        default_priority;               /* 默认事件的优先级 */
};

struct eh_event{
    struct eh_list_head            list_node;               /* 事件队列链表 */
    eh_event_type_t                *type;                   /* 事件类型 */
    void (*callback)(eh_event_t *e);                        /* 事件处理时的回调函数 */
    int32_t                        rc;                      /* 被引用次数 */
    union{
        uint32_t                        flag;
        struct{
            enum EH_PRIORITY               priority:5;              /* 事件的优先级 */
            uint8_t                        is_ready:1;              /* 是否已经就绪,事件是否已经发生，事件是否需要处理 */
            uint8_t                        is_autodestruct:1;       /* 是否自动析构，当需要自动析构时，在引用次数变为0时将调用析构函数 */
            uint8_t                        reserve0:1;
            uint8_t                        reserve1[3];
        };
    };
    
};


struct eh_task{
    struct eh_list_head            list_node;               /* 任务链表 */
    struct eh_list_head            event_head;              /* 待处理事件的链表 */
    void (*task_function)(void);                            /* 任务函数 */
    enum EH_PRIORITY               priority:5;              /* 任务的优先级 */
    enum EH_TASK_STATE                state:3;              /* 任务运行状态*/
};

struct eh_init_param{
    void            (*global_lock)(eh_t *eh, uint32_t *state);      /* 上锁， */
    void            (*global_unlock)(eh_t *eh, uint32_t state);     /* 解锁 */
    eh_usec_t       (*get_usec_monotonic_time)(void);               /* 系统单调时钟的微秒数 */
    void*           (*malloc)(size_t size);
    void            (*free)(void* ptr);
    void            *user_data;
};




#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */

#define _eh_lock(eh, state)                         \
    do{                                             \
        if(eh->global_lock && eh->global_unlock)    \
            eh->global_lock(eh, state);             \
    }while(0)

#define _eh_unlock(eh, state)                       \
    do{                                             \
        if(eh->global_lock && eh->global_unlock)    \
            eh->global_unlock(eh, state);           \
    }while(0)

#define  _eh_get_usec_monotonic_time(eh)   (eh->get_usec_monotonic_time())

/* @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ */
/* ########################################################## event 相关接口 ############################################################ */
/* @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ */

/**/
/**
 * @brief                  事件初始化
 * @param  e               事件实例指针
 * @param  type            事件类型
 * @param  callback        事件发生时的回调函数
 * @return int             见eh_error.h
 */
extern int eh_event_init(eh_event_t *e, eh_event_type_t *type, void (*callback)(eh_event_t *e));

/**
 * @brief                  设置事件的优先级
 * @param  e               事件实例指针
 * @param  priority        事件优先级
 */
#define eh_event_config_priority(e, priority)   \
    do{                                         \
        e->priority = priority;                 \
    }while(0)

/**
 * @brief                   设置事件结束后自动析构
 * @param  e                事件实例指针
 */
#define eh_event_config_autodestruct(e)         \
    do{                                         \
        e->is_autodestruct = 1;                 \
    }while(0)

/* @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ */
/* EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE */

/**
 * @brief  初始化event_hub句柄
 *  1.考虑到配合系统多线程编程时在其他线程产生事件的操作，所以设计了互斥锁接口.
 *  2.给lock预留了state参数，考虑在裸机编程时，中断禁止时，需要记录获得锁前的中断状态，所以需要预留state参数。
 * @param  eh               event_hub指针
 * @param  global_lock      全局锁上锁/关中断， 要求是递归锁，也就是说获得锁的线程可以重复获得锁
 * @param  global_unlock    全局解锁/开中断
 * @return int 
 */
extern int eh_global_init(eh_t *eh, eh_init_param_t *param);


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif // _EVENT_HUB_H_