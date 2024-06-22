#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include "debug.h"
#include "eh.h"
#include "eh_event.h"
#include "eh_timer.h"
#include "eh_platform.h"
#include "epoll_hub.h"

static struct {
    pthread_mutexattr_t attr;
    pthread_mutex_t     eh_use_mutex;
    pthread_mutex_t     idle_break_mutex;
    eh_clock_t          expire;
    bool                is_idle_state;
}linux_platform;
static void  linux_global_lock(uint32_t *state){
    (void) state;
    pthread_mutex_lock(&linux_platform.eh_use_mutex);
}
static void  linux_global_unlock(uint32_t state){
    (void) state;
    pthread_mutex_unlock(&linux_platform.eh_use_mutex);
}

static eh_clock_t linux_get_clock_monotonic_time(void){
    eh_clock_t microsecond;
        struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);

    microsecond = ((eh_clock_t)ts.tv_sec * 1000000) + ((eh_clock_t)ts.tv_nsec / 1000);
    return microsecond;
}

// static uint64_t linux_get_clock_monotonic_time(void) {
//     uint32_t low, high;
//     asm volatile (
//         "rdtsc"           // 执行RDTSC指令
//         : "=a"(low), "=d"(high) // 输出到%eax（low）和%edx（high）
//         :
//         : "memory" // 确保内存副作用正确处理
//     );
    
//     return ((uint64_t)high << 32) | low; // 结合高低32位为一个64位值
// }


static void linux_idle_or_extern_event_handler(void){
    eh_usec_t usec_timeout;


    pthread_mutex_lock(&linux_platform.idle_break_mutex);
    linux_platform.is_idle_state = true;
    usec_timeout = eh_clock_to_usec(eh_get_loop_idle_time());
    epoll_hub_clean_wait_break_event();
    pthread_mutex_unlock(&linux_platform.idle_break_mutex);

    epoll_hub_poll(usec_timeout);
    
    linux_platform.is_idle_state = false;
}
static void linux_idle_break( void ){
    pthread_mutex_lock(&linux_platform.idle_break_mutex);
    if(linux_platform.is_idle_state)
        epoll_hub_set_wait_break_event();
    pthread_mutex_unlock(&linux_platform.idle_break_mutex);
}

static eh_platform_port_param_t linux_platform_port_param = {
    .global_lock = linux_global_lock,
    .global_unlock = linux_global_unlock,
    .clocks_per_sec = 1000000,
    //.clocks_per_sec = 2903998000,
    .get_clock_monotonic_time = linux_get_clock_monotonic_time,
    .idle_or_extern_event_handler = linux_idle_or_extern_event_handler,
    .idle_break = linux_idle_break,
    .malloc = malloc,
    .free = free,
};

static int  __init linux_platform_init(void){
    int ret;
    ret = epoll_hub_init();
    if(ret < 0) return ret;
    linux_platform.is_idle_state = false;
    ret = pthread_mutexattr_init(&linux_platform.attr);
    if(ret < 0)
        goto mutex_attr_init_error;
    ret = pthread_mutexattr_settype(&linux_platform.attr, PTHREAD_MUTEX_RECURSIVE);
    if(ret < 0)
        goto pthread_mutexattr_settype_error;
    ret = pthread_mutex_init(&linux_platform.eh_use_mutex, &linux_platform.attr);
    if(ret < 0)
        goto pthread_mutex_init_eh_use_error;
        
    ret = pthread_mutex_init(&linux_platform.idle_break_mutex, NULL);
    if(ret < 0)
        goto pthread_mutex_idle_break_mutex_use_error;

    ret = eh_platform_register_port_param(&linux_platform_port_param);
    if(ret < 0)
        goto eh_platform_register_port_param_error;
    ret = 0;
    return ret;
eh_platform_register_port_param_error:
    pthread_mutex_destroy(&linux_platform.idle_break_mutex);
pthread_mutex_idle_break_mutex_use_error:
    pthread_mutex_destroy(&linux_platform.eh_use_mutex);
pthread_mutex_init_eh_use_error:
pthread_mutexattr_settype_error:
    pthread_mutexattr_destroy(&linux_platform.attr);
mutex_attr_init_error:
    epoll_hub_exit();
    return ret;
}

static void __exit linux_platform_deinit(void){
    pthread_mutex_destroy(&linux_platform.idle_break_mutex);
    pthread_mutex_destroy(&linux_platform.eh_use_mutex);
    pthread_mutexattr_destroy(&linux_platform.attr);
    epoll_hub_exit();
}

eh_core_module_export(linux_platform_init, linux_platform_deinit);