/**
 * @file eh_timer.h
 * @brief eh 定时器的实现
 * @author simon.xiaoapeng (simon.xiaoapeng@gmail.com)
 * @version 1.0
 * @date 2024-04-20
 * 
 * @copyright Copyright (c) 2024  simon.xiaoapeng@gmail.com
 * 
 * @par 修改日志:
 */


#ifndef _EH_TIMER_H_
#define _EH_TIMER_H_

#include <stdint.h>
typedef struct eh_timer_event eh_timer_event_t;

struct eh_timer_event {
    eh_event_t                      event;
    struct eh_list_head             list_node;               /* 定时器链，链在eh->timer_list_head上 */
    eh_usec_t                       expire;                  /* 定时器到期时间 */
    eh_usec_t                       interval;                /* 定时器间隔时间 */
    union{
        uint32_t                        flag;
        struct{
            uint32_t                     is_auto_circulation:1;
            uint32_t                     is_start:1;
        };
    };
    
};





#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */


/**
 * @brief                   定时器启动
 * @param  eh               even_hub实例指针
 * @param  timer            定时器实例指针
 * @return int 
 */
extern int eh_timer_start(eh_t *eh, eh_timer_event_t *timer);

/**
 * @brief                   定时器停止
 * @param  eh               even_hub实例指针
 * @param  timer            定时器实例指针
 * @return int 
 */
extern int eh_timer_stop(eh_t *eh, eh_timer_event_t *timer);

/**
 * @brief                   定时器重启,若定时器没有运行，则调用本函数运行，若定时器正在运行，则重新加载定时器
 * @param  eh               even_hub实例指针
 * @param  timer            定时器实例指针
 * @return int 
 */
extern int eh_time_restart(eh_t *eh, eh_timer_event_t *timer);

/**
 * @brief                   定时器初始化
 * @param  timer            实例指针
 * @param  callback         事件触发时的回调函数
 * @return int              见eh_error.h
 */
extern int eh_timer_init(eh_timer_event_t *timer, void (*callback)(eh_event_t *e));

/**
 * @brief                   配置定时器的超时时间,若定时器在运行中，则下次重新运行时使用此值
 * @param  timer            实例指针
 * @param  interval         超时时间
 * @return int 
 */
#define eh_timer_config_interval(timer, interval)   \
    do{                                             \
        timer->interval = interval;                 \
    }while(0)


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif // _EH_TIMER_H_


