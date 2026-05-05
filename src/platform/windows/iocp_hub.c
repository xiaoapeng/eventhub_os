/**
 * @file iocp_hub.c
 * @brief IOCP 事件集中处理中心，替代 Linux epoll_hub / macOS kqueue_hub
 * @author simon.xiaoapeng (simon.xiaoapeng@gmail.com)
 * @date 2026-05-04
 *
 * @copyright Copyright (c) 2026  simon.xiaoapeng@gmail.com
 *
 */

#include <stdbool.h>
#include <stdlib.h>
#include <windows.h>
#include <eh.h>
#include <eh_module.h>
#include <eh_debug.h>
#include <iocp_hub.h>

#define IOCP_MAX_DEQUEUE_EVENTS 1024

#ifndef EH_DBG_MODULE_LEVEL_IOCP_HUB
#define EH_DBG_MODULE_LEVEL_IOCP_HUB EH_DBG_WARNING
#endif

struct iocp_hub {
    HANDLE                      iocp_port;
    bool                        wakeup_pending;
    struct iocp_hub_fd_action   wakeup_action;
};

static struct iocp_hub iocp_hub;

static void wakeup_callback(ULONG_PTR completion_key, DWORD bytes_transferred, OVERLAPPED *overlapped, void *arg){
    (void)completion_key;
    (void)bytes_transferred;
    (void)overlapped;
    (void)arg;
    iocp_hub.wakeup_pending = false;
}

void iocp_hub_set_wait_break_event(void){
    if(!iocp_hub.wakeup_pending){
        iocp_hub.wakeup_pending = true;
        PostQueuedCompletionStatus(iocp_hub.iocp_port, 0, IOCP_HUB_WAKEUP_KEY, NULL);
    }
}

void iocp_hub_clean_wait_break_event(void){
    /* No-op: the posted packet is consumed automatically by GetQueuedCompletionStatusEx */
}

int iocp_hub_add_handle(HANDLE handle, ULONG_PTR completion_key, struct iocp_hub_fd_action *action){
    HANDLE ret = CreateIoCompletionPort(handle, iocp_hub.iocp_port, completion_key, 0);
    if(ret == NULL)
        return -1;
    (void)action;
    return 0;
}

int iocp_hub_del_handle(HANDLE handle){
    (void)handle;
    /* IOCP does not have an explicit dissociate; closing the handle removes it */
    return 0;
}

int iocp_hub_poll(eh_usec_t timeout_usec){
    OVERLAPPED_ENTRY entries[IOCP_MAX_DEQUEUE_EVENTS];
    ULONG num_removed = 0;
    DWORD ms_timeout;

    if(timeout_usec == 0)
        ms_timeout = 0;
    else if(timeout_usec == (eh_usec_t)-1)
        ms_timeout = INFINITE;
    else
        ms_timeout = (DWORD)(timeout_usec / 1000);

    BOOL ok = GetQueuedCompletionStatusEx(iocp_hub.iocp_port, entries,
        IOCP_MAX_DEQUEUE_EVENTS, &num_removed, ms_timeout, FALSE);

    if(!ok || num_removed == 0)
        return 0;

    for(ULONG i = 0; i < num_removed; i++){
        if(entries[i].lpCompletionKey == IOCP_HUB_WAKEUP_KEY){
            if(iocp_hub.wakeup_action.callback)
                iocp_hub.wakeup_action.callback(IOCP_HUB_WAKEUP_KEY, 0, entries[i].lpOverlapped, iocp_hub.wakeup_action.arg);
            continue;
        }
        /* Future: dispatch to registered action by completion_key */
    }

    return 0;
}

int __init iocp_hub_init(void){
    iocp_hub.iocp_port = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 1);
    if(iocp_hub.iocp_port == NULL)
        return -1;

    iocp_hub.wakeup_pending = false;
    iocp_hub.wakeup_action.callback = wakeup_callback;
    iocp_hub.wakeup_action.arg = NULL;

    return 0;
}

void __exit iocp_hub_exit(void){
    if(iocp_hub.iocp_port != NULL){
        CloseHandle(iocp_hub.iocp_port);
        iocp_hub.iocp_port = NULL;
    }
}