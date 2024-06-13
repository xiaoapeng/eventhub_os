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
#include <sys/epoll.h>
#include "eh_module.h"

struct epoll_hub{
    int epoll_fd;
    int timeout_fd;
    int wait_break_fd;
}epoll_hub;

int __init epoll_hub_init(void){
    epoll_hub.epoll_fd = epoll_create1(EPOLL_CLOEXEC);
    return 0;
}
