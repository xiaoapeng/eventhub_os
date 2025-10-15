/**
 * @file eh_signal.h
 * @brief 在eh_event 和 eh_event_cb 基础之上封装的信号和槽机制
 * @author simon.xiaoapeng (simon.xiaoapeng@gmail.com)
 * @date 2024-07-28
 * 
 * @copyright Copyright (c) 2024  simon.xiaoapeng@gmail.com
 * 
 */

#ifndef _EH_SIGNAL_H_
#define _EH_SIGNAL_H_

#include <eh_event.h>
#include <eh_event_cb.h>
#include <eh_list.h>

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */


typedef eh_event_cb_slot_t eh_signal_slot_t;

/* 泛型自定义信号 */
#define EH_STRUCT_CUSTOM_SIGNAL(event_type)                                             \
    struct {                                                                            \
        eh_event_cb_trigger_t               trigger;                                    \
        union{                                                                          \
            eh_event_t                      event;                                      \
            event_type                      custom_event;                               \
        };                                                                              \
    }


#define EH_SIGNAL_INIT_VAL_NONE             {0}


/**
 * @brief 定义并初步初始化一个局部自定义信号，在.c中使用，建议将自定义部分初始化后再进行注册
 *         .c中使用，此方式定义的信号无需init
 */
#define EH_DEFINE_LOCAL_CUSTOM_SIGNAL(signal_name, event_type, signal_init_val)         \
        EH_STRUCT_CUSTOM_SIGNAL(event_type)  signal_name = {                            \
            .trigger = EH_EVENT_CB_TRIGGER_INIT(signal_name.trigger),                   \
            .custom_event = signal_init_val,                                            \
        }


/**
 * @brief 定义并初步初始化一个局部（静态）自定义信号，在.c中使用，建议将自定义部分初始化后再进行注册
 *         .c中使用，此方式定义的信号无需init
 */
#define EH_DEFINE_STATIC_CUSTOM_SIGNAL(signal_name, event_type, signal_init_val)        \
        static EH_STRUCT_CUSTOM_SIGNAL(event_type)  signal_name = {                     \
            .trigger = EH_EVENT_CB_TRIGGER_INIT(signal_name.trigger),                   \
            .custom_event = signal_init_val,                                            \
        }


/**
 * @brief 定义并初步初始化一个全局自定义信号，在.c中使用，建议将自定义部分初始化后再进行注册,
 *         .c中使用，此方式定义的信号无需init
 */
#define EH_DEFINE_CUSTOM_SIGNAL(signal_name, event_type, signal_init_val)               \
        signal_name##_##event_type##_signal_type_t signal_name = {                      \
            .trigger = EH_EVENT_CB_TRIGGER_INIT(signal_name.trigger),                   \
            .custom_event = signal_init_val,                                            \
        }

/**
 * @brief 声明一个全局自定义信号，在头文件中使用此宏,外部模块可以使用此信号
 *         .h中使用
 */
#define EH_EXTERN_CUSTOM_SIGNAL(signal_name, event_type)                                \
    typedef EH_STRUCT_CUSTOM_SIGNAL(event_type)                                         \
            signal_name##_##event_type##_signal_type_t;                                 \
    extern signal_name##_##event_type##_signal_type_t signal_name;

/**
 * @brief 通用信号结构体，可用于定义通用信号，定义后需要init后方可使用
 */
typedef  EH_STRUCT_CUSTOM_SIGNAL(eh_event_t) eh_signal_base_t;

/**
 * @brief 声明一个私有的通用信号，在.c文件中定义后需注册使用，此方式定义的信号无需init
 *         .c中使用
 */
#define EH_STATIC_SIGNAL(signal_name)                                                   \
    EH_DEFINE_STATIC_CUSTOM_SIGNAL(signal_name, eh_event_t, EH_EVENT_INIT(signal_name.event))

/**
 * @brief 定义并初始化一个通用信号，在.c文件中定义后需注册使用，此方式定义的信号无需init
 *         .c中使用
 */
#define EH_DEFINE_SIGNAL(signal_name)                                                   \
    EH_DEFINE_CUSTOM_SIGNAL(signal_name, eh_event_t, EH_EVENT_INIT(signal_name.event))

/**
 * @brief 声明一个通用信号，在头文件中使用此宏,外部模块可以使用此信号
 */
#define EH_EXTERN_SIGNAL(signal_name)                                                   \
    EH_EXTERN_CUSTOM_SIGNAL(signal_name, eh_event_t)

/**
 * @brief 定义一个槽，并进行初始化（无需再调用 eh_signal_slot_init）
 */
#define EH_DEFINE_SLOT(slot_name, _slot_function, _slot_param)                          \
    eh_signal_slot_t slot_name = {                                                      \
        .slot_function = _slot_function,                                                \
        .slot_param = _slot_param,                                                      \
        .cb_node = EH_LIST_HEAD_INIT(slot_name.cb_node)                                 \
    }

/**
 * @brief 信号初始化，使用EH_DEFINE_SIGNAL/EH_STATIC_SIGNAL定义的信号无需调用本函数，
 *          若为自定义信号，调用该函数后需要再调用自定义的初始化
 */
#define eh_signal_init(signal)   do{                                                    \
        eh_event_cb_trigger_init(&(signal)->trigger);                                   \
        eh_event_init(&(signal)->event);                                                \
    }while(0)

/**
 * @brief 信号注册，只有注册的信号才能正常回调槽函数,注意信号拥有线程亲和性，
 *        该函数会将信号注册到当前任务，相关槽函数将在当前任务中执行。
 *        注册任务后必须要调用eh_signal_dispatch_loop开始循环处理信号
 * @param signal 信号实例指针
 * @return 0:成功，负数:失败
 */
#define eh_signal_register(signal)                                                      \
    eh_event_cb_register(eh_task_self(), (&(signal)->event), &(signal)->trigger)

/**
 * @brief 信号注销，注销后无法再回调槽函数,注意信号拥有线程亲和性，调用本函数时，
 *        必须从注册任务中注销，否则应该使用eh_signal_unregister_from_task函数
 * @param signal 信号实例指针
 */
#define eh_signal_unregister(signal)                                                    \
    eh_event_cb_unregister(eh_task_self(), (&(signal)->event))

/**
 * @brief  信号注册，注册信号到指定任务
 */
#define eh_signal_register_to_task(signal, task)                                        \
    eh_event_cb_register(task, (&(signal)->event), &(signal)->trigger)

/**
 * @brief  信号注销，注销信号从指定任务
 */
#define eh_signal_unregister_from_task(signal, task)                                    \
    eh_event_cb_unregister(task, (&(signal)->event))

/**
 * @brief 清除信号和槽函数的全部连接,清除等待该事件的接收器
 */
#define eh_signal_clean(signal)  do{                                                    \
        eh_event_cb_trigger_clean(&(signal)->trigger);                                  \
        eh_event_clean((&(signal)->event));                                             \
    }while(0)

/**
 * @brief 获取自定义信号中的自定义事件
 */
#define eh_signal_to_custom_event(signal)                                               \
    (&(signal)->custom_event)

/**
 * @brief 获取事件所对应的信号
 */
#define eh_signal_from_event(event_base_ptr)                                            \
    eh_container_of(event_base_ptr, eh_signal_base_t, event)

/**
 * @brief 槽初始化
 */
#define eh_signal_slot_init(slot, slot_function, slot_param)                            \
    eh_event_cb_slot_init(slot, slot_function, slot_param)

/**
 * @brief 连接信号和槽
 */
#define eh_signal_slot_connect(signal, slot)                                            \
    eh_event_cb_connect((&(signal)->trigger), slot)

/**
 * @brief 断开信号和槽
 */
#define eh_signal_slot_disconnect(slot)                                                 \
    eh_event_cb_disconnect(slot)

/**
 * @brief 触发信号
 */
#define eh_signal_notify(signal)                                                        \
    eh_event_notify((&(signal)->event))

/**
 * @brief 信号处理世界循环，调用 eh_signal_dispatch_loop_request_quit 退出循环
 */
#define eh_signal_dispatch_loop()    eh_event_loop()

/**
 * @brief 请求退出信号处理循环
 */
#define eh_signal_dispatch_loop_request_quit()    eh_event_loop_request_quit(eh_task_self())

/**
 * @brief 请求指定任务退出信号处理循环
 */
#define eh_signal_dispatch_loop_request_quit_form_task(task)    eh_event_loop_request_quit(task)


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif // _EH_SIGNAL_H_