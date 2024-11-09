#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include <eh.h>
#include <eh_event.h>
#include <eh_timer.h>
#include <eh_platform.h>
#include <epoll_hub.h>

static struct {
    pthread_mutexattr_t attr;
    pthread_mutex_t     eh_use_mutex;
    eh_clock_t          expire;
    bool                is_idle_state;
}linux_platform;


eh_clock_t  platform_get_clock_monotonic_time(void){
    eh_clock_t microsecond;
        struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    microsecond = ((eh_clock_t)ts.tv_sec * 1000000) + ((eh_clock_t)ts.tv_nsec / 1000);
    return microsecond;
}
eh_save_state_t  platform_enter_critical(void){
    pthread_mutex_lock(&linux_platform.eh_use_mutex);
    return 0;
}
void  platform_exit_critical(eh_save_state_t state){
    (void)state;
    pthread_mutex_unlock(&linux_platform.eh_use_mutex);
}
void  platform_idle_break(void){
    pthread_mutex_lock(&linux_platform.eh_use_mutex);
    if(linux_platform.is_idle_state)
        epoll_hub_set_wait_break_event();
    pthread_mutex_unlock(&linux_platform.eh_use_mutex);
}

void  platform_idle_or_extern_event_handler(void){
    eh_usec_t usec_timeout;


    pthread_mutex_lock(&linux_platform.eh_use_mutex);
    linux_platform.is_idle_state = true;
    usec_timeout = eh_clock_to_usec((eh_clock_t)eh_get_loop_idle_time());
    epoll_hub_clean_wait_break_event();
    pthread_mutex_unlock(&linux_platform.eh_use_mutex);

    epoll_hub_poll(usec_timeout);

    linux_platform.is_idle_state = false;
}

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

    ret = 0;
    return ret;
pthread_mutex_init_eh_use_error:
pthread_mutexattr_settype_error:
    pthread_mutexattr_destroy(&linux_platform.attr);
mutex_attr_init_error:
    epoll_hub_exit();
    return ret;
}

static void __exit linux_platform_deinit(void){
    pthread_mutex_destroy(&linux_platform.eh_use_mutex);
    pthread_mutexattr_destroy(&linux_platform.attr);
    epoll_hub_exit();
}

eh_core_module_export(linux_platform_init, linux_platform_deinit);