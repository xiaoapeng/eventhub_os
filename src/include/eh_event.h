/**
 * @file eh_event.h
 * @brief 事件和事件epoll的实现
 * @author simon.xiaoapeng (simon.xiaoapeng@gmail.com)
 * @version 1.0
 * @date 2024-06-22
 * 
 * @copyright Copyright (c) 2024  simon.xiaoapeng@gmail.com
 * 
 * @par 修改日志:
 */
#ifndef _EH_EVENT_H_
#define _EH_EVENT_H_

#include "eh_types.h"

typedef struct eh_event                     eh_event_t;
typedef struct eh_event_type                eh_event_type_t;
typedef int*                                eh_epoll_t;
typedef struct eh_epoll_slot                eh_epoll_slot_t;

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */


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


/**
 * @brief                           事件初始化
 * @param  e                        事件实例指针
 * @param  static_const_name        事件名称，可为NULL
 * @return int                      见eh_error.h
 */
extern __safety int eh_event_init(eh_event_t *e, const eh_event_type_t* type);

/**
 * @brief                           唤醒所有监听该事件的任务
 *                                  并将其剔除等待队列，一般在释放event实例前进行调用
 * @param  e                        事件实例指针
 */
extern __safety void eh_event_clean(eh_event_t *e);

/**
 * @brief                           事件通知,唤醒所有监听该事件的任务
 * @param  e                        事件实例指针
 * @return int 
 */
extern __safety int eh_event_notify(eh_event_t *e);

/**
 * @brief                           事件等待,若事件e在此函数调用前发生，将无法捕获到事件(事件无队列)
 *                                  必须等待condition返回true才进行返回，若condition == NULL 那么就等待信号发生就直接返回
 * @param  e                        事件实例指针
 * @param  arv                      condition的参数
 * @param  condition                条件函数
 * @param  timeout                  超时时间,EH_TIME_FOREVER为永不超时 因为事件无队列，
 *                              当condition为NULL时，超时时间为0将毫无意义，若想使用0，请使用epoll监听事件
 * @return int 
 */
extern int __async__ eh_event_wait_condition_timeout(eh_event_t *e, void* arg, bool (*condition)(void* arg), eh_sclock_t timeout);

/**
 * @brief                           事件等待,若事件e在此函数调用前发生，将无法捕获到事件(事件无队列)
 * @param  e                        事件实例指针
 * @param  timeout                  超时时间,EH_TIME_FOREVER为永不超时 因为事件无队列，
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
extern __safety eh_epoll_t eh_epoll_new(void);


/**
 * @brief                   删除一个epoll句柄
 * @param  epoll            
 */
extern __safety void eh_epoll_del(eh_epoll_t epoll);

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
 * @param  timeout          超时时间，当0则立即返回,当EH_TIME_FOREVER永久等待，其他大于0的值则进行相应时间的等待
 * @return int              成功返回拿到event事件的个数，失败返回负数错误码
 */
extern int __async__ eh_epoll_wait(eh_epoll_t epoll,eh_epoll_slot_t *epool_slot, int slot_size, eh_sclock_t timeout);





#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif // _EH_EVENT_H_