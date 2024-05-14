
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include "eh.h"
#include "eh_module.h"
#include "eh_timer.h"



static struct {
    pthread_mutexattr_t attr;
    pthread_mutex_t     mutex;
    int                 is_never_expire;
    eh_clock_t          expire;
}linux_platform;
static void  linux_global_lock(uint32_t *state){
    (void) state;
    pthread_mutex_lock(&linux_platform.mutex);
}
static void  linux_global_unlock(uint32_t state){
    (void) state;
    pthread_mutex_unlock(&linux_platform.mutex);
}

static eh_clock_t linux_get_clock_monotonic_time(void){
    eh_clock_t microsecond;
        struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);

    microsecond = ((eh_clock_t)ts.tv_sec * 1000000) + ((eh_clock_t)ts.tv_nsec / 1000);
    return microsecond;
}

static void linux_idle_or_extern_event_handler(int is_wait_event){
    eh_sclock_t diff_time = eh_diff_time(linux_get_clock_monotonic_time(), linux_platform.expire);
    diff_time = linux_platform.is_never_expire ? 1000*1 : -1;
    if(is_wait_event && diff_time > 0){
        usleep((__useconds_t)diff_time);
    }
}
static void linux_expire_time_change(int is_never_expire, eh_clock_t new_expire){
    linux_platform.is_never_expire = is_never_expire;
    linux_platform.expire = new_expire;
}


EH_DEFINE_PLATFORM_PORT_PARAM(  
    linux_global_lock, linux_global_unlock, 1000000, 
    linux_get_clock_monotonic_time,
    linux_idle_or_extern_event_handler,
    linux_expire_time_change, 
    malloc, free);

static int __init linux_platform_init(void){
    pthread_mutexattr_init(&linux_platform.attr);
    pthread_mutexattr_settype(&linux_platform.attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&linux_platform.mutex, &linux_platform.attr);
    return 0;
}

static void __exit linux_platform_deinit(void){
    pthread_mutex_destroy(&linux_platform.mutex);
    pthread_mutexattr_destroy(&linux_platform.attr);
}

eh_core_module_export(linux_platform_init, linux_platform_deinit);