/**
 * @file epoll_hub.h
 * @brief linux epoll处理中心
 * @author simon.xiaoapeng (simon.xiaoapeng@gmail.com)
 * @version 1.0
 * @date 2024-06-13
 * 
 * @copyright Copyright (c) 2024  simon.xiaoapeng@gmail.com
 * 
 * @par 修改日志:
 */

#ifndef _EPOLL_HUB_H_
#define _EPOLL_HUB_H_


#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */

struct epoll_fd_action{
    void (*callback)(uint32_t events, void *arg);
    void *arg;
};

extern int  epoll_hub_add_fd(int fd, uint32_t events, struct epoll_fd_action *action);
extern int  epoll_hub_del_fd(int fd);
extern int  epoll_hub_poll(eh_usec_t timeout);
extern int  epoll_hub_init(void);
extern void epoll_hub_exit(void);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif // _EPOLL_HUB_H_

