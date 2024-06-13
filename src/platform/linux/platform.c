
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include "eh.h"
#include "eh_config.h"
#include "eh_module.h"
#include "eh_timer.h"
#include "eh_platform.h"


static struct {
    pthread_mutexattr_t attr;
    pthread_mutex_t     eh_use_mutex;
    eh_clock_t          expire;
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

static void linux_idle_or_extern_event_handler(void){
    
    usleep((__useconds_t)eh_clock_to_usec(eh_get_loop_idle_time()));

}
static void linux_idle_break( void ){
    
}

static eh_platform_port_param_t linux_platform_port_param = {
    .global_lock = linux_global_lock,
    .global_unlock = linux_global_unlock,
    .clocks_per_sec = 1000000,
    .get_clock_monotonic_time = linux_get_clock_monotonic_time,
    .idle_or_extern_event_handler = linux_idle_or_extern_event_handler,
    .idle_break = linux_idle_break,
    .malloc = malloc,
    .free = free,
};

static int  __init linux_platform_init(void){
    pthread_mutexattr_init(&linux_platform.attr);
    pthread_mutexattr_settype(&linux_platform.attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&linux_platform.eh_use_mutex, &linux_platform.attr);

    eh_platform_register_port_param(&linux_platform_port_param);
    return 0;
}

static void __exit linux_platform_deinit(void){
    pthread_mutex_destroy(&linux_platform.eh_use_mutex);
    pthread_mutexattr_destroy(&linux_platform.attr);
}

eh_core_module_export(linux_platform_init, linux_platform_deinit);