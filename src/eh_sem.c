/**
 * @file eh_sem.c
 * @brief 任务信号量的实现，虽然事件也提供了类似与休眠唤醒的功能，
 *   但是事件只能保证多次set必有一次触发，无法保证多次触发，
 *   而信号量的实现能增加可靠性，当然信号量也是基于event实现的，
 *   也能使用event相关特性，比如epoll。
 * @author simon.xiaoapeng (simon.xiaoapeng@gmail.com)
 * @version 1.0
 * @date 2024-06-23
 * 
 * @copyright Copyright (c) 2024  simon.xiaoapeng@gmail.com
 * 
 * @par 修改日志:
 */


#include "eh.h"
#include "eh_event.h"
#include "eh_interior.h"
#include "eh_sem.h"
#include <stdbool.h>

struct eh_sem {
    eh_event_t                  wakeup_event;
    /**
     * @brief 从线程上考虑，读者（P操作）永远只有一个，就是协程上下文，
     *   写者(V操作)可能有多个，那么只需对V操作进行加锁，将PV变量分开，
     *   可以得到更好的性能
     */
    uint32_t                    sem_num_p; /* wait */
    uint32_t                    sem_num_v; /* post */
};

static const eh_event_type_t eh_sem_type = {
    .name = "sem_type"
};

static bool condition_sem(void *arg){
    struct eh_sem *sem = (struct eh_sem *)arg;
    return !(sem->sem_num_p == sem->sem_num_v) ;
}

eh_sem_t eh_sem_create(uint32_t value){
    struct eh_sem *new_sem;
    new_sem = (struct eh_sem *)eh_malloc(sizeof(struct eh_sem));
    if( new_sem == NULL )
        return eh_error_to_ptr(EH_RET_MALLOC_ERROR);
    new_sem->sem_num_v = value;
    new_sem->sem_num_p = 0;
    eh_event_init(&new_sem->wakeup_event, &eh_sem_type);
    return (eh_sem_t)new_sem;
}

void eh_sem_destroy(eh_sem_t _sem){
    struct eh_sem *sem = (struct eh_sem *)_sem;
    eh_event_clean(&sem->wakeup_event);
    eh_free(sem);
}


int eh_sem_wait(eh_sem_t _sem, eh_sclock_t timeout){
    struct eh_sem *sem = (struct eh_sem *)_sem;
    int ret;
    ret = __await__ eh_event_wait_condition_timeout(&sem->wakeup_event, sem, condition_sem, timeout);
    if(ret < 0)
        return ret;
    sem->sem_num_p++;
    return EH_RET_OK;
}

int eh_sem_post(eh_sem_t _sem){
    struct eh_sem *sem = (struct eh_sem *)_sem;
    int ret = EH_RET_OK;
    uint32_t state;
    eh_lock(&state);
    if(sem->sem_num_v + 1 == sem->sem_num_p){
        ret = EH_RET_BUSY;
        goto out;
    }
    sem->sem_num_v++;
    ret = eh_event_notify(&sem->wakeup_event);
out:
    eh_unlock(state);
    return ret;
}





