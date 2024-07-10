/**
 * @file epoll_hub.c
 * @brief epool event集中处理中心，所有的系统描述符都将在这里被监听，然后根据注册情况调用相关回调
 * @author simon.xiaoapeng (simon.xiaoapeng@gmail.com)
 * @version 1.0
 * @date 2024-06-06
 * 
 * @copyright Copyright (c) 2024  simon.xiaoapeng@gmail.com
 * 
 * @par 修改日志:
 */


#include <unistd.h>
#include <sys/timerfd.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include "eh.h"
#include "eh_module.h"
#include "epoll_hub.h"

#define EPOLL_WAIT_MAX_EVENTS 1024

struct epoll_hub{
    int                         epoll_fd;
    int                         timeout_fd;             /* 使用定时器的方式实现ns级别的超时 */
    int                         wait_break_fd;
    struct epoll_fd_action      wait_break_fd_action;
    struct epoll_event          wait_events[1024];
}epoll_hub;


static void event_wait_break_callback(uint32_t events, void *arg){
    (void) events;
    (void) arg;
    epoll_hub_clean_wait_break_event();
}

void epoll_hub_clean_wait_break_event(void){
    eventfd_t value;
    eventfd_read(epoll_hub.wait_break_fd, &value);
}

void epoll_hub_set_wait_break_event(void){
    eventfd_write(epoll_hub.wait_break_fd, 1);
}


int epoll_hub_add_fd(int fd, uint32_t events, struct epoll_fd_action *action){
    struct epoll_event event = {0};

    event.events = events;
    event.data.ptr = action;
    return epoll_ctl(epoll_hub.epoll_fd, EPOLL_CTL_ADD, fd, &event);
}

int epoll_hub_del_fd(int fd){
    return epoll_ctl(epoll_hub.epoll_fd, EPOLL_CTL_DEL, fd, NULL);
}


int epoll_hub_poll(eh_usec_t usec_timeout){
    struct itimerspec timeout_spec = {0};
    int epoll_parameter_timeout;
    int ret;
    if(usec_timeout){
        timeout_spec.it_value.tv_sec = (__time_t)(usec_timeout / 1000000);
        timeout_spec.it_value.tv_nsec = (__syscall_slong_t)((usec_timeout % 1000000) * 1000);
        epoll_parameter_timeout = -1;        
    }else{
        timeout_spec.it_value.tv_sec = 0;
        timeout_spec.it_value.tv_nsec = 0;
        epoll_parameter_timeout = 0;
    }

    timerfd_settime(epoll_hub.timeout_fd, 0, &timeout_spec, NULL);
    ret = epoll_wait(epoll_hub.epoll_fd,  epoll_hub.wait_events, EPOLL_WAIT_MAX_EVENTS, epoll_parameter_timeout);
    if(ret <= 0) return  ret;
    for(int i = 0; i < ret; i++){
        struct epoll_event *event = &epoll_hub.wait_events[i];
        struct epoll_fd_action *action = event->data.ptr;
        if(action && action->callback)
            action->callback(event->events, action->arg);
    }
    

    return 0;
}



int __init epoll_hub_init(void){
    int ret;
    ret = epoll_create1(EPOLL_CLOEXEC);
    if(ret < 0)
        return -1;
    epoll_hub.epoll_fd = ret;
    
    ret = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    if(ret < 0)
        goto timerfd_create_error;
    epoll_hub.timeout_fd = ret;

    ret = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if(ret < 0)
        goto eventfd_error;
    epoll_hub.wait_break_fd = ret;

    epoll_hub.wait_break_fd_action.arg = NULL;
    epoll_hub.wait_break_fd_action.callback = event_wait_break_callback;

    ret = epoll_hub_add_fd(epoll_hub.wait_break_fd, EPOLLIN, &epoll_hub.wait_break_fd_action);
    if(ret < 0)
        goto epoll_hub_add_fd_error;

    ret = epoll_hub_add_fd(epoll_hub.timeout_fd, EPOLLIN, NULL);
    if(ret < 0)
        goto epoll_hub_add_fd_error;

    ret = 0;
    return ret;
epoll_hub_add_fd_error:
    close(epoll_hub.wait_break_fd);
eventfd_error:
    close(epoll_hub.timeout_fd);
timerfd_create_error:
    close(epoll_hub.epoll_fd);
    return ret;
}

void __exit epoll_hub_exit(void){
    close(epoll_hub.epoll_fd);
    close(epoll_hub.timeout_fd);
}