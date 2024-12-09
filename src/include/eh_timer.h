/**
 * @file eh_timer.h
 * @brief eh 定时器的实现
 * @author simon.xiaoapeng (simon.xiaoapeng@gmail.com)
 * @date 2024-04-20
 * 
 * @copyright Copyright (c) 2024  simon.xiaoapeng@gmail.com
 * 
 */


#ifndef _EH_TIMER_H_
#define _EH_TIMER_H_

#include <eh_types.h>
#include <eh_rbtree.h>

typedef struct eh_event_timer eh_event_timer_t;

#define EH_TIMER_ATTR_AUTO_CIRCULATION  0x00000001              /* 自动重复，重运行 */
#define EH_TIMER_ATTR_NOW_TIME_BASE     0x00000002              /* 当EH_TIMER_ATTR_AUTO_CIRCULATION有效时,装载时以当前时间为基准 */

struct eh_event_timer {
    eh_event_t                      event;
    struct eh_rbtree_node           rb_node;                    /* 定时器链，挂在在eh->timer_tree_root */
    eh_clock_t                      expire;                     /* 定时器到期时间 */
    eh_sclock_t                     interval;                   /* 定时器间隔时间 */
    uint32_t                        attrribute;
};

#define EH_TIMER_INIT(timer)    {                                               \
        .event = EH_EVENT_INIT(timer.event),                                    \
        .rb_node = EH_RBTREE_NODE_INIT(timer.rb_node),                          \
        .expire = 0,                                                            \
        .interval = 0,                                                          \
        .attrribute = 0,                                                        \
    }

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */

/**
 * @brief 计算时间差
 * @param  time_a           时间a
 * @param  time_b           时间b
 */
#define eh_diff_time(time_a, time_b)    (eh_sclock_t)(((time_a) - (time_b)))

/**
 * @brief   计算定时器剩余时钟数
 * @param  now_time         当前时间
 * @param  timer            定时器句柄指针
 * return                  剩余时间,为正数时表示还有时间，为负数时表示已经到期
 */
#define eh_remaining_time(now_time, timer_ptr)  eh_diff_time((timer_ptr)->expire, (now_time))

/**
 * @brief                   定时器启动
 * @param  timer            定时器实例指针
 * @return int 
 */
extern __safety int eh_timer_start(eh_event_timer_t *timer);

/**
 * @brief                   定时器停止
 * @param  timer            定时器实例指针
 * @return int 
 */
extern __safety int eh_timer_stop(eh_event_timer_t *timer);

/**
 * @brief                   定时器重启,若定时器没有运行，则调用本函数运行，若定时器正在运行，则重新加载定时器
 * @param  timer            定时器实例指针
 * @return int 
 */
extern __safety int eh_timer_restart(eh_event_timer_t *timer);

/**
 * @brief                   定时器高级初始化
 * @param  timer            定时器实例指针
 * @param  clock_interval   定时器间隔
 * @param  attr             定时器属性
 * @return __safety 
 */
extern __safety int eh_timer_advanced_init(eh_event_timer_t *timer, eh_sclock_t clock_interval, uint32_t attr);

/**
 * @brief                   定时器初始化
 * @param  timer            实例指针
 * @return int              见eh_error.h
 */
static __safety inline int eh_timer_init(eh_event_timer_t *timer){
    return eh_timer_advanced_init(timer, 0, 0);
}

/**
 * @brief                   判断定时器是否在运行
 * @param  timer            实例指针
 * @return 
 */
extern __safety bool eh_timer_is_running(eh_event_timer_t *timer);

/**
 * @brief                   清除占用资源
 * @param  timer            实例指针
 */
extern __safety void eh_timer_clean(eh_event_timer_t *timer);


/**
 * @brief                           配置定时器的超时时间,若定时器在运行中，则下次重新运行时使用此值
 * @param  timer                    实例指针
 * @param  clock_interval           超时时间
 * @return int 
 */
#define  eh_timer_config_interval(timer, clock_interval)     \
    do{                                                     \
        (timer)->interval = (eh_sclock_t)(clock_interval);                 \
    }while(0)

/**
 * @brief                           配置定时器属性
 * @param  timer                    实例指针
 * @param  attr                     定时器属性 EH_TIMER_ATTR_XXX | EH_TIMER_ATTR_XXXX
 */
#define eh_timer_set_attr(timer, attr)                      \
    do{                                                     \
        (timer)->attrribute = attr;                         \
    }while(0)

/**
 * @brief                           获取定时器的事件实例指针
 */
#define eh_timer_to_event(timer)            (&(timer)->event)

#define eh_time_is_forever(time_clock)      ((eh_sclock_t)(time_clock) == EH_TIME_FOREVER)

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif // _EH_TIMER_H_


