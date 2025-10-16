/**
 * @file    eh_event_cb.h
 * @brief   此模块是event的扩展，在event的基础上实现自动回调
 * @author simon.xiaoapeng (simon.xiaoapeng@gmail.com)
 * @date 2024-06-19
 * 
 * @copyright Copyright (c) 2024  simon.xiaoapeng@gmail.com
 * 
 */


#ifndef _EH_EVENT_CB_H_
#define _EH_EVENT_CB_H_

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */

typedef struct eh_event_cb_slot eh_event_cb_slot_t;

struct eh_event_cb_slot{
    void                    (*slot_function)(eh_event_t *e, void *slot_param);
    void                    *slot_param;
    struct eh_list_head     cb_node;
};


static inline void eh_event_cb_slot_init(eh_event_cb_slot_t *slot, 
    void (*slot_function)(eh_event_t *e, void *slot_param), void *slot_param){
   slot->slot_function = slot_function;
   slot->slot_param = slot_param;
   eh_list_head_init(&slot->cb_node);
}

/**
 * @brief                   连接一个触发器与槽函数
 * @param  e                事件
 * @param  slot             槽函数
 * @param  task             执行触发回调的主体任务
 * @return int
 */
extern int eh_event_cb_connect(eh_event_t *e, eh_event_cb_slot_t *slot, eh_task_t *task);

/**
 * @brief                   断开一个触发器与槽函数的连接
 * @param  e                事件
 * @param  slot             槽函数
 * @param  task             执行触发回调的主体任务
 */
extern void eh_event_cb_disconnect(eh_event_t *e, eh_event_cb_slot_t *slot, eh_task_t *task);

/**
 * @brief                   清理一个任务下的所有触发器与槽函数的连接
 * @param  e                事件
 * @param  task             执行触发回调的主体任务
 */
extern void eh_event_cb_clean(eh_event_t *e, eh_task_t *task);

/**
 * @brief                   执行事件处理
 * @return int 
 */
extern int eh_event_loop(void);

/**
 * @brief                   请求退出事件循环
 * @param  task             任务
 */
extern void eh_event_loop_request_quit(eh_task_t *task);



#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif // _EH_EVENT_CB_H_