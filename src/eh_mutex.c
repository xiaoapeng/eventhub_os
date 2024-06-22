/**
 * @file eh_mutex.c
 * @brief 任务互斥锁的实现，虽然协程大部分情况下是不需要锁的，
 *    但是，在宏观资源面前，还是存在加锁的，比如，
 *    在遍历链表时调用 __await类型函数，在其他任务上就
 *    可能操作到该链表导致链表指针无效出现段错误，所以
 *    在非必要情况，是用不到本模块的。
 * @author simon.xiaoapeng (simon.xiaoapeng@gmail.com)
 * @version 1.0
 * @date 2024-06-22
 * 
 * @copyright Copyright (c) 2024  simon.xiaoapeng@gmail.com
 * 
 * @par 修改日志:
 */

#include <stdint.h>
#include "eh.h"
#include "eh_error.h"
#include "eh_event.h"
#include "eh_interior.h"
#include "eh_mutex.h"


struct eh_mutex {
    eh_event_t                  wakeup_event;
    uint32_t                    lock_cnt;
    enum eh_mutex_type          type;
    eh_task_t                   *lock_task;
};
const eh_event_type_t eh_mutex_type = {
    .name = "mutex_type"
};

static bool condition_mutex(void *arg){
    struct eh_mutex *mutex = (struct eh_mutex *)arg;
    return mutex->lock_cnt == 0 || (mutex->type == EH_MUTEX_TYPE_RECURSIVE && mutex->lock_task == eh_task_self());
}

eh_mutex_t eh_mutex_create(enum eh_mutex_type type){
    struct eh_mutex *new_mutex;

    if( type >= EH_MUTEX_TYPE_MAX)
        return eh_error_to_ptr(EH_RET_INVALID_PARAM);

    new_mutex = eh_malloc(sizeof(struct eh_mutex));
    if( new_mutex == NULL )
        return eh_error_to_ptr(EH_RET_MALLOC_ERROR);
    new_mutex->lock_cnt = 0;
    new_mutex->type = type;
    eh_event_init(&new_mutex->wakeup_event, &eh_mutex_type);
    return (eh_mutex_t)new_mutex;
}
void eh_mutex_destroy(eh_mutex_t _mutex){
    struct eh_mutex *mutex = (struct eh_mutex *)_mutex;
    eh_event_clean(&mutex->wakeup_event);
    eh_free(mutex);
}
int __async__ eh_mutex_lock(eh_mutex_t _mutex, eh_sclock_t timeout){
    struct eh_mutex *mutex = (struct eh_mutex *)_mutex;
    int ret;
    ret = __await__ eh_event_wait_condition_timeout(&mutex->wakeup_event, mutex, condition_mutex, timeout);
    if(ret < 0)
        return ret;
    mutex->lock_task = eh_task_self();
    mutex->lock_cnt++;
    return EH_RET_OK;
}
int eh_mutex_unlock(eh_mutex_t _mutex){
    struct eh_mutex *mutex = (struct eh_mutex *)_mutex;
    if(mutex->lock_cnt == 0)
        return EH_RET_OK;
    if(mutex->lock_task != eh_task_self())
        return EH_RET_INVALID_STATE;
    mutex->lock_cnt--;
    if(mutex->lock_cnt)
        return EH_RET_OK;
    return eh_event_notify(&mutex->wakeup_event);
}







