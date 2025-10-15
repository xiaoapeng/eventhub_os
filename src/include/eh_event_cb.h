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
typedef struct eh_event_cb_trigger eh_event_cb_trigger_t;

struct eh_event_cb_slot{
    void                    (*slot_function)(eh_event_t *e, void *slot_param);
    void                    *slot_param;
    struct eh_list_head     cb_node;
};

struct eh_event_cb_trigger{
    struct eh_list_head     cb_head;
};

#define EH_EVENT_CB_TRIGGER_INIT(trigger)   {               \
        .cb_head = EH_LIST_HEAD_INIT(trigger.cb_head),  \
    }

static inline void eh_event_cb_slot_init(eh_event_cb_slot_t *slot, 
    void (*slot_function)(eh_event_t *e, void *slot_param), void *slot_param){
   slot->slot_function = slot_function;
   slot->slot_param = slot_param;
   eh_list_head_init(&slot->cb_node);
}

static inline void eh_event_cb_trigger_init(eh_event_cb_trigger_t *trigger){
    eh_list_head_init(&trigger->cb_head);
}


/**
 * @brief                   注册一个事件触发器
 *                           只有注册的的触发器才能进行槽函数的连接
 *                           注册事件触发器并不会影响事件触发器与槽函数的连接
 * @param  task             执行触发回调的主体任务
 * @param  e                事件
 * @param  trigger          触发器
 * @return int 
 */
extern int eh_event_cb_register(eh_task_t *task, eh_event_t *e, eh_event_cb_trigger_t *trigger);

/**
 * @brief                   注销一个事件触发器
 *                           注销事件触发器并不会影响事件触发器与槽函数的连接
 * @param  task             执行触发回调的主体任务
 * @param  e                注册事件触发器时的事件
 * @return int 
 */
extern int eh_event_cb_unregister(eh_task_t *task, eh_event_t *e);


/**
 * @brief                   连接一个触发器与槽函数
 * @param  trigger          触发器
 * @param  slot             槽函数
 * @return int
 */
extern int eh_event_cb_connect(eh_event_cb_trigger_t *trigger, eh_event_cb_slot_t *slot);

/**
 * @brief                   断开一个触发器与槽函数的连接
 * @param  slot             槽函数
 */
extern void eh_event_cb_disconnect(eh_event_cb_slot_t *slot);

/**
 * @brief                   清除此触发器和槽函数的全部连接
 * @param  trigger          触发器
 */
extern void eh_event_cb_trigger_clean(eh_event_cb_trigger_t *trigger);


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