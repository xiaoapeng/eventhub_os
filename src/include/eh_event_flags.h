/**
 * @file eh_event_flags.h
 * @brief 事件bit map 通知，可支持 unsigned long 位数大小的flag数
 * @author simon.xiaoapeng (simon.xiaoapeng@gmail.com)
 * @version 1.0
 * @date 2024-10-03
 * 
 * @copyright Copyright (c) 2024  simon.xiaoapeng@gmail.com
 * 
 * @par 修改日志:
 */

#ifndef _EH_EVENT_FLAGS_H_
#define _EH_EVENT_FLAGS_H_


#include "eh_types.h"

typedef struct eh_event_flags eh_event_flags_t;
typedef unsigned long eh_flags_t;

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */


struct eh_event_flags {
    /* 必须在第一位 */
    eh_event_t                  event;
    unsigned long               flags;
};



/**
 * @brief   获取事件对象
 */
#define eh_event_flags_get_event(ef) (&(ef)->event)

/**
 * @brief  初始化 eh_event_flags_t 结构
 * @return 
 */
extern __safety int eh_event_flags_init(eh_event_flags_t *ef);

/** 
 * @brief   清理 eh_event_flags_t 结构,与 eh_event_flags_init 相反
 */
static inline __safety void eh_event_flags_clean(eh_event_flags_t *ef){
    eh_event_clean(&ef->event);
}

/**
 * @brief   等待事件，并清除事件
 * @param  ef               
 * @param  wait_flags       要等待 flag
 * @param  claen_flags      如果等到了wait_flags,那么就按照claen_flags清除内部flags值
 * @param  reality_flags    返回实际等到的flag
 * @param  timeout          超时时间,EH_TIME_FOREVER为永不超时 
 * @return int 
 */
extern __async int eh_event_flags_wait(eh_event_flags_t *ef, eh_flags_t wait_flags, 
    eh_flags_t claen_flags, eh_flags_t *reality_flags, eh_sclock_t timeout);

/**
 * @brief   设置事件bit，
 * @param  ef
 * @param  flags            flags会与内部flags进行|操作
 * @return int
 */
extern __safety int eh_event_flags_set_bits(eh_event_flags_t *ef, eh_flags_t flags);

/**
 * @brief   设置flags
 * @param  ef
 * @param  flags            直接赋值给内部flags
 * @return __safety 
 */
extern __safety int eh_event_flags_set(eh_event_flags_t *ef, eh_flags_t flags);



#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif // _EH_EVENT_FLAGS_H_