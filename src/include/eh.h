/**
 * @file eh.h
 * @brief 主要的头文件实现
 * @author simon.xiaoapeng (simon.xiaoapeng@gmail.com)
 * @date 2024-04-13
 * 
 * @copyright Copyright (c) 2024  simon.xiaoapeng@gmail.com
 * 
 */


#ifndef _EVENT_HUB_H_
#define _EVENT_HUB_H_

#include <stdint.h>

#include <eh_config.h>
#include <eh_types.h>
#include <eh_list.h>
#include <eh_rbtree.h>
#include <eh_error.h>
#include <eh_co.h>
#include <eh_module.h>

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */

typedef struct eh                           eh_t;
typedef uint64_t                            eh_usec_t;
typedef uint64_t                            eh_msec_t;
typedef uint64_t                            eh_clock_t;
typedef int64_t                             eh_sclock_t;
typedef struct eh_task                      eh_task_t;
typedef struct eh_loop_poll_task            eh_loop_poll_task_t;
typedef struct eh_task_sta                  eh_task_sta_t;

#define EH_TASK_FLAGS_SYSTEM_TASK          0x00000002
#define EH_TASK_FLAGS_DETACH               0x00000004   /* 自动分离，指定此参数在任务退出时自动释放 */

enum EH_TASK_STATE{
    /* 顺序很重要，不要轻易调整 */
    EH_TASK_STATE_READY,                            /* 就绪状态 */
    EH_TASK_STATE_RUNING,                           /* 运行状态 */
    EH_TASK_STATE_WAIT,                             /* 等待状态 */
    EH_TASK_STATE_FINISH,                           /* 结束状态 */
};

/* 永不到期 */
#define EH_TIME_FOREVER                 (-1)

struct eh_loop_poll_task{
    struct eh_list_head         list_node;
    void                        *arg;
    void                        (*poll_task)(void* arg);
};

struct eh_task_sta{
    enum EH_TASK_STATE           state;
    void*                        stack;
    unsigned long                stack_size;
    unsigned long                stack_min_ever_free_size_level;
    const char*                  task_name;
};


/**
 * @brief   微秒转换为时钟数
 * @param   _msec   毫秒数
 * return   eh_clock_t
 */
extern eh_clock_t eh_msec_to_clock(eh_msec_t msec);

/**
 * @brief   微秒转换为时钟数
 * @param   _usec   微秒数
 * return   eh_clock_t
 */
 extern eh_clock_t eh_usec_to_clock(eh_usec_t usec);

/**
 * @brief   时钟数转换为毫秒数
 * @param   clock  时钟数
 * return   eh_msec_t
 */
extern eh_msec_t eh_clock_to_msec(eh_clock_t clock);

/**
 * @brief   时钟数转换为微秒数
 * @param   clock  时钟数
 * return   eh_usec_t
 */
extern eh_usec_t eh_clock_to_usec(eh_clock_t clock);

/**
 * @brief 让出当前CPU
 * @return int 返回0
 */
extern void __async eh_task_yield(void);

/**
 * @brief                   使用静态方式创建一个协程任务
 * @param  name             任务名称
 * @param  flags            任务标志    设置为EH_TASK_FLAGS_SYSTEM_TASK后将在事件发生后具有优先调用的权利
 * @param  stack            任务的静态栈
 * @param  stack_size       任务栈大小
 * @param  task_arg         任务参数
 * @param  task_function    任务执行函数
 * @return eh_task_t* 
 */
extern eh_task_t* eh_task_static_stack_create(const char *name, uint32_t flags, void *stack, unsigned long stack_size, void *task_arg, int (*task_function)(void*));
/**
 * @brief                   使用动态方式创建一个协程任务
 * @param  name             任务名称
 * @param  flags            任务标志     设置为EH_TASK_FLAGS_SYSTEM_TASK后将在事件发生后具有优先调用的权利
 *                                      设置为EH_TASK_FLAGS_DETACH将自动释放任务
 * @param  stack_size       任务栈大小
 * @param  task_arg         任务参数
 * @param  task_function    任务执行函数
 * @return eh_task_t* 
 */
extern eh_task_t* eh_task_create(const char *name, uint32_t flags,  unsigned long stack_size, void *task_arg, int (*task_function)(void*));

/**
 * @brief                   退出任务
 * @param  ret              退出返回值
 */
extern void  eh_task_exit(int ret);

/**
 * @brief                   任务获取自己的任务句柄
 * @return eh_task_t*       返回当前的任务句柄
 */
extern eh_task_t* eh_task_self(void);

/**
 * @brief                   获取任务状态
 * @param  task             任务句柄
 * @param  sta              任务状态
 * @return int 
 */
extern void eh_task_sta(const eh_task_t *task, eh_task_sta_t *sta);

/**
 * @brief                   阻塞等待任务结束
 * @param  task             被等待的任务句柄,
 * @param  ret              成功返回0，成功时任务将会被 eh_task_destroy 释放掉
 * @return int 
 */
extern int __async eh_task_join(eh_task_t *task, int *ret, eh_sclock_t timeout);

/**
 * @brief                   无条件回收任务，回收前应确认任务相关资源已经全部销毁（不建议使用）
 */
extern void eh_task_destroy(eh_task_t *task);


/**
 * @brief                获取当前被调度的次数
 * @return  unsigned int 
 */
 extern unsigned int eh_task_dispatch_cnt(void);

/**
 * @brief                   添加一个轮询任务，轮询任务会在系统栈(系统任务中)内循环执行，
 *                          在世界循环每次都会被执行到，请勿在此循环中做任何超时服务，包括await
 *                          非必要，请不要使用，可使用timer event_cb代替
 * @param  poll_task             
 */
extern void eh_loop_poll_task_add(eh_loop_poll_task_t *poll_task);


/**
 * @brief                   删除一个轮询任务
 * @param  poll_task
 */
static inline void eh_loop_poll_task_del(eh_loop_poll_task_t *poll_task){
    eh_list_del_init(&poll_task->list_node);
}

/**
 * @brief  初始化event_hub
 * @return int 
 */
extern int eh_global_init(void);

/**
 * @brief 反初始化event_hub
 */
extern void eh_global_exit(void);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif // _EVENT_HUB_H_