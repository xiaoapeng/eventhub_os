/**
 * @file iocp_hub.h
 * @brief IOCP 事件集中处理中心，替代 Linux epoll_hub / macOS kqueue_hub
 * @author simon.xiaoapeng (simon.xiaoapeng@gmail.com)
 * @date 2026-05-04
 *
 * @copyright Copyright (c) 2026  simon.xiaoapeng@gmail.com
 *
 */

#ifndef _IOCP_HUB_H_
#define _IOCP_HUB_H_

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

#define IOCP_HUB_WAKEUP_KEY  ((ULONG_PTR)0x1)

struct iocp_hub_fd_action {
    void (*callback)(ULONG_PTR completion_key, DWORD bytes_transferred, OVERLAPPED *overlapped, void *arg);
    void *arg;
};

void iocp_hub_set_wait_break_event(void);
void iocp_hub_clean_wait_break_event(void);
int  iocp_hub_add_handle(HANDLE handle, ULONG_PTR completion_key, struct iocp_hub_fd_action *action);
int  iocp_hub_del_handle(HANDLE handle);
int  iocp_hub_poll(eh_usec_t timeout_usec);
int  iocp_hub_init(void);
void iocp_hub_exit(void);

#ifdef __cplusplus
}
#endif

#endif /* _IOCP_HUB_H_ */
