#include <unistd.h>
#include <sys/time.h>
#include <sys/event.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <eh.h>
#include <eh_module.h>
#include <eh_debug.h>
#include <kqueue_hub.h>

#define KEVENT_WAIT_MAX_EVENTS 1024

#ifndef EH_DBG_MODEULE_LEVEL_EPOL_HUB
#define EH_DBG_MODEULE_LEVEL_EPOL_HUB EH_DBG_WARNING
#endif

struct kqueue_hub {
    int                         kqueue_fd;
    int                         wait_break_fd;
    int                         wake_up_fd;
    struct kqueue_event_action      wait_break_fd_action;
    struct kevent               wait_events[KEVENT_WAIT_MAX_EVENTS];
} kqueue_hub;

static void kqueue_event_wait_break_callback(int fd, int16_t filter, void *arg) {
    (void) fd;
    (void) filter;
    (void) arg;
    kqueue_hub_clean_wait_break_event();
}

static int kqueue_event_set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        return -1;
    }
    
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void kqueue_hub_clean_wait_break_event(void) {
    // Read from the pipe to clear the interrupt event
    static char buffer[1024];
    while(read(kqueue_hub.wait_break_fd, &buffer, sizeof(buffer)) == sizeof(buffer)){
        /* ignore */
    }
}

void kqueue_hub_set_wait_break_event(void) {
    // Write to the pipe to simulate the interrupt (waking up the waiting thread)
    char buffer = '1';
    write(kqueue_hub.wait_break_fd, &buffer, sizeof(buffer));
}

int kqueue_hub_add_fd(int fd, int16_t filter, uint16_t flags, struct kqueue_event_action *action) {
    struct kevent event;
    EV_SET(&event, fd, filter, EV_ADD | EV_ENABLE | flags, 0, 0, action);
    return kevent(kqueue_hub.kqueue_fd, &event, 1, NULL, 0, NULL);
}

int kqueue_hub_del_fd(int fd, int16_t filter) {
    struct kevent event;
    EV_SET(&event, fd, filter, EV_DELETE, 0, 0, NULL);
    return kevent(kqueue_hub.kqueue_fd, &event, 1, NULL, 0, NULL);
}

int kqueue_hub_poll(eh_usec_t timeout) {
    struct timespec timeout_spec;
    int ret;
    if(timeout){
        timeout_spec.tv_sec = timeout / 1000000;
        timeout_spec.tv_nsec = (timeout % 1000000) * 1000;    
    }else{
        timeout_spec.tv_sec = 0;
        timeout_spec.tv_nsec = 0;
    }
    ret = kevent(kqueue_hub.kqueue_fd, NULL, 0, kqueue_hub.wait_events, KEVENT_WAIT_MAX_EVENTS, &timeout_spec);
    if (ret <= 0) {
        return ret;
    }

    for (int i = 0; i < ret; i++) {
        struct kevent *event = &kqueue_hub.wait_events[i];
        struct kqueue_event_action *action = (struct kqueue_event_action *)event->udata;
        if (action && action->callback) {
            action->callback((int)event->ident, event->filter, action->arg);
        }
    }

    return 0;
}

int kqueue_hub_init(void) {
    int ret;

    // Create kqueue
    ret = kqueue();
    if (ret < 0)
        return -1;
    kqueue_hub.kqueue_fd = ret;

    // Create a pipe to simulate eventfd functionality for interrupt mechanism
    int pipefd[2];
    ret = pipe(pipefd);
    if (ret < 0) {
        close(kqueue_hub.kqueue_fd);
        return -1;
    }
    kqueue_hub.wait_break_fd = pipefd[0];  // The read end of the pipe
    kqueue_hub.wake_up_fd = pipefd[1];  // The write end of the pipe
    ret = kqueue_event_set_nonblocking(kqueue_hub.wait_break_fd);
    if (ret < 0) {
        close(pipefd[0]);
        close(pipefd[1]);
        close(kqueue_hub.kqueue_fd);
        return ret;
    }

    // Set up the break event action for the pipe (interrupt mechanism)
    kqueue_hub.wait_break_fd_action.arg = NULL;
    kqueue_hub.wait_break_fd_action.callback = kqueue_event_wait_break_callback;

    // Add the read end of the pipe to the kqueue
    ret = kqueue_hub_add_fd(kqueue_hub.wait_break_fd, EVFILT_READ, 0, &kqueue_hub.wait_break_fd_action);
    if (ret < 0) {
        close(pipefd[0]);
        close(pipefd[1]);
        close(kqueue_hub.kqueue_fd);
        return ret;
    }

    ret = 0;
    return ret;
}

void kqueue_hub_exit(void) {
    // Delete the break event from the kqueue
    kqueue_hub_del_fd(kqueue_hub.wait_break_fd, EVFILT_READ);
    close(kqueue_hub.wait_break_fd);
    close(kqueue_hub.wake_up_fd);
    close(kqueue_hub.kqueue_fd);
}
