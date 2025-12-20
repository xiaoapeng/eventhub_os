/**
 * @file epoll_hub.h
 * @brief linux epoll处理中心
 * @author simon.xiaoapeng (simon.xiaoapeng@gmail.com)
 * @date 2024-06-13
 * 
 * @copyright Copyright (c) 2024  simon.xiaoapeng@gmail.com
 * 
 */

#ifndef _EPOLL_HUB_H_
#define _EPOLL_HUB_H_

#include <sys/event.h>

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */

struct kqueue_event_action{
    void (*callback)(struct kevent *event, void *arg);
    void *arg;
};

extern void kqueue_hub_set_wait_break_event(void);
extern void kqueue_hub_clean_wait_break_event(void);

/**
 * @brief 添加一个文件描述符到kqueue事件中心
 * @param  fd               文件描述符
 * @param  filter           事件过滤器      EVFILT_XX
 * @param  flags            事件标志        EV_XX
 * @param  action           事件回调
 * @return int              0 成功 -1 失败
 */
extern int  kqueue_hub_add_fd(int fd, int16_t filter, uint16_t flags, struct kqueue_event_action *action);

/**
 * @brief 删除一个文件描述符从kqueue事件中心
 * @param  fd               文件描述符
 * @param  filter           事件过滤器      EVFILT_XX
 * @return int              0 成功 -1 失败
 */
extern int  kqueue_hub_del_fd(int fd, int16_t filter);
extern int  kqueue_hub_poll(eh_usec_t timeout);
extern int  kqueue_hub_init(void);
extern void kqueue_hub_exit(void);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif // _EPOLL_HUB_H_

