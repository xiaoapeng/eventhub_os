/**
 * @file test_ringbuf.c
 * @brief 
 * @author simon.xiaoapeng (simon.xiaoapeng@gmail.com)
 * @date 2024-07-27
 * 
 * @copyright Copyright (c) 2024  simon.xiaoapeng@gmail.com
 * 
 */


#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <eh.h>
#include <eh_error.h>
#include <eh_debug.h>
#include <eh_event.h>
#include <eh_list.h>
#include <eh_platform.h>
#include <eh_timer.h> 
#include <eh_types.h>
#include "eh_ringbuf.h"

void stdout_write(void *stream, const uint8_t *buf, size_t size){
    (void)stream;
    printf("%.*s", (int)size, (const char*)buf);
}

#define TEST_BUF_SIZE 1024

void test_buf_init(uint8_t *buf, size_t size){
    for(size_t i=0; i < size; i++){
        buf[i] = (uint8_t)i;
    }
}
int test_buf_check(const uint8_t *buf, size_t size, uint8_t start_num){
    for(size_t i=0; i < size; i++){
        if(buf[i] != (uint8_t)(start_num+i))
            return -1;
    }
    return 0;
}

void test_buf_clean(uint8_t *buf, size_t size){
    memset(buf, 0, size);
}

struct random_wr{
    eh_ringbuf_t* ringbuf;
    eh_event_t    w_event;
    eh_event_t    exit_event;
    int           cnt;
    bool          exit;
};

void* thread_test_random_wr_function(void* arg){
    struct random_wr* rw = (struct random_wr*)arg;
    int r_len_sum = 0;
    uint8_t test_buf[(uint32_t)eh_ringbuf_total_size(rw->ringbuf)];
    for(int i=0; i < rw->cnt && !(volatile bool)rw->exit ; i++){
        int fsize = eh_ringbuf_free_size(rw->ringbuf);
        if(fsize == 0){
            usleep(10);
            continue;
        }
        int w_size = rand() % (fsize+1);
        for(int i=0;i<w_size; i++){
            test_buf[i] = (uint8_t)(i+r_len_sum);
        }
        r_len_sum += eh_ringbuf_write(rw->ringbuf, test_buf, w_size);
        eh_event_notify(&rw->w_event);
    }
    eh_event_notify(&rw->exit_event);
    return 0;
}

int test_random_wr(int32_t size, uint32_t cnt){
    struct random_wr rw;
    eh_epoll_slot_t  epoll_slot;
    eh_epoll_t       test_epoll;
    uint8_t test_buf[(uint32_t)size];
    pthread_t thread_id;
    int ret = 0;
    int r_len_sum = 0;
    rw.cnt = (int)cnt;
    rw.exit = false;
    srand((unsigned int)time(NULL));
    eh_event_init(&rw.exit_event);
    eh_event_init(&rw.w_event);
    test_epoll = eh_epoll_new();
    if(eh_ptr_to_error(test_epoll))
        return -1;
    EH_DBG_ERROR_EXEC((rw.ringbuf = eh_ringbuf_create(size, NULL)) == NULL, ret= -1; goto ringbuf_create_error );
    
    eh_epoll_add_event(test_epoll, &rw.w_event, NULL);
    eh_epoll_add_event(test_epoll, &rw.exit_event, NULL);

    if (pthread_create(&thread_id, NULL, thread_test_random_wr_function, &rw) != 0) {
        eh_debugfl("pthread_create error!");
        return -1;
    }

    while(1){
        ret = eh_epoll_wait(test_epoll, &epoll_slot, 1, EH_TIME_FOREVER);
        if(ret < 0){
            eh_errfl("epoll_wait error %d", ret);
            goto out;
        }
        
        if(epoll_slot.affair != EH_EPOLL_AFFAIR_EVENT_TRIGGER){
            ret = -1;
            goto out;
        }

        if(epoll_slot.event == &rw.exit_event){
            ret = 0;
            goto out;
        }
        
        int n = (rand()%5) + 1;
        int r_len;
        for(int i=0; i < n && eh_ringbuf_size(rw.ringbuf); i++){
            int r_size = rand()%(eh_ringbuf_size(rw.ringbuf)+1);
            if(eh_ringbuf_size(rw.ringbuf) == eh_ringbuf_total_size(rw.ringbuf)){
                r_size = eh_ringbuf_total_size(rw.ringbuf)/2;
            }
            r_len = eh_ringbuf_read(rw.ringbuf, test_buf, r_size);
            ret = test_buf_check(test_buf, (size_t)r_len, (uint8_t)r_len_sum);
            if(ret < 0)
                goto out;
            r_len_sum += r_len;
        }
        
    }

out:
    rw.exit = true;
    pthread_join(thread_id, NULL);
    eh_event_clean(&rw.exit_event);
    eh_event_clean(&rw.w_event);
    eh_ringbuf_destroy(rw.ringbuf);
ringbuf_create_error:
    eh_epoll_del(test_epoll);
    return ret;
}


int test_random_wr_peek(int32_t size, uint32_t cnt){
    struct random_wr rw;
    eh_epoll_slot_t  epoll_slot;
    eh_epoll_t       test_epoll;
    uint8_t test_buf[(uint32_t)size];
    pthread_t thread_id;
    int ret = 0;
    int r_len_sum = 0;
    rw.cnt = (int)cnt;
    rw.exit = false;
    srand((unsigned int)time(NULL));
    eh_event_init(&rw.exit_event);
    eh_event_init(&rw.w_event);
    test_epoll = eh_epoll_new();
    if(eh_ptr_to_error(test_epoll))
        return -1;
    EH_DBG_ERROR_EXEC((rw.ringbuf = eh_ringbuf_create(size, NULL)) == NULL, ret= -1; goto ringbuf_create_error );
    
    eh_epoll_add_event(test_epoll, &rw.w_event, NULL);
    eh_epoll_add_event(test_epoll, &rw.exit_event, NULL);

    if (pthread_create(&thread_id, NULL, thread_test_random_wr_function, &rw) != 0) {
        eh_debugfl("pthread_create error!");
        return -1;
    }

    while(1){
        ret = eh_epoll_wait(test_epoll, &epoll_slot, 1, EH_TIME_FOREVER);
        if(ret < 0){
            eh_errfl("epoll_wait error %d", ret);
            goto out;
        }
        
        if(epoll_slot.affair != EH_EPOLL_AFFAIR_EVENT_TRIGGER){
            ret = -1;
            goto out;
        }

        if(epoll_slot.event == &rw.exit_event){
            ret = 0;
            goto out;
        }
        
        int n = (rand()%5) + 1;
        const uint8_t *tmp_buf;
        for(int i=0; i < n && eh_ringbuf_size(rw.ringbuf); i++){
            //int r_size = rand()%(eh_ringbuf_size(rw.ringbuf)+1);
            //int offset = rand()%(r_size+1);
            int r_size = eh_ringbuf_size(rw.ringbuf);
            int offset = 0;
            r_size -= offset;
            if(eh_ringbuf_size(rw.ringbuf) == eh_ringbuf_total_size(rw.ringbuf)){
                r_size = eh_ringbuf_total_size(rw.ringbuf)/2;
                offset = 0;
            }
            tmp_buf = eh_ringbuf_peek(rw.ringbuf, offset, test_buf, &r_size);
            if(tmp_buf){
                ret = test_buf_check(tmp_buf, (size_t)r_size, (uint8_t)(r_len_sum + offset));
                if(ret < 0)
                    goto out;
                r_len_sum += r_size + offset;
                eh_ringbuf_read_skip(rw.ringbuf, r_size + offset);
            }
        }
        
    }
out:
    rw.exit = true;
    pthread_join(thread_id, NULL);
    eh_event_clean(&rw.exit_event);
    eh_event_clean(&rw.w_event);
    eh_ringbuf_destroy(rw.ringbuf);
ringbuf_create_error:
    eh_epoll_del(test_epoll);
    return ret;
}

/* 基础接口测试 */
int test_basics_interface(eh_ringbuf_t* ringbuf){
    uint8_t test_buf[TEST_BUF_SIZE*2] = {0};
    int len;

    /* 用例1 初始状态下的容量状态 */
    EH_DBG_ERROR_EXEC(eh_ringbuf_free_size(ringbuf) != TEST_BUF_SIZE, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_size(ringbuf) != 0, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_total_size(ringbuf) != TEST_BUF_SIZE, return -1);
    eh_ringbuf_reset(ringbuf);

    /* 用例2 满写满读测试*/
    test_buf_init(test_buf, sizeof(test_buf));
    EH_DBG_ERROR_EXEC(eh_ringbuf_write(ringbuf, test_buf, TEST_BUF_SIZE) != TEST_BUF_SIZE, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_free_size(ringbuf) != 0, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_size(ringbuf) != TEST_BUF_SIZE, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_read(ringbuf, test_buf, TEST_BUF_SIZE) != TEST_BUF_SIZE, return -1);
    EH_DBG_ERROR_EXEC(test_buf_check(test_buf, TEST_BUF_SIZE, 0), return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_free_size(ringbuf) != TEST_BUF_SIZE, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_size(ringbuf) != 0, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_total_size(ringbuf) != TEST_BUF_SIZE, return -1);
    eh_ringbuf_reset(ringbuf);

    /* 用例3 过量读写测试 */
    test_buf_init(test_buf, sizeof(test_buf));
    EH_DBG_ERROR_EXEC(eh_ringbuf_write(ringbuf, test_buf, TEST_BUF_SIZE+1) != TEST_BUF_SIZE, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_free_size(ringbuf) != 0, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_size(ringbuf) != TEST_BUF_SIZE, return -1);
    test_buf_clean(test_buf, sizeof(test_buf));
    EH_DBG_ERROR_EXEC(eh_ringbuf_read(ringbuf, test_buf, TEST_BUF_SIZE+1) != TEST_BUF_SIZE, return -1);
    EH_DBG_ERROR_EXEC(test_buf_check(test_buf, TEST_BUF_SIZE, 0), return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_free_size(ringbuf) != TEST_BUF_SIZE, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_size(ringbuf) != 0, return -1);
    eh_ringbuf_reset(ringbuf);

    /* 用例4 镜像区域满写满读测试 */
    /* 先满读满写进入镜像区域 */
    test_buf_init(test_buf, sizeof(test_buf));
    EH_DBG_ERROR_EXEC(eh_ringbuf_write(ringbuf, test_buf, TEST_BUF_SIZE) != TEST_BUF_SIZE, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_read(ringbuf, test_buf, TEST_BUF_SIZE) != TEST_BUF_SIZE, return -1);
    /* 再次满读满写 */
    test_buf_init(test_buf, sizeof(test_buf));
    EH_DBG_ERROR_EXEC(eh_ringbuf_write(ringbuf, test_buf, TEST_BUF_SIZE) != TEST_BUF_SIZE, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_free_size(ringbuf) != 0, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_size(ringbuf) != TEST_BUF_SIZE, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_read(ringbuf, test_buf, TEST_BUF_SIZE) != TEST_BUF_SIZE, return -1);
    EH_DBG_ERROR_EXEC(test_buf_check(test_buf, TEST_BUF_SIZE, 0), return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_free_size(ringbuf) != TEST_BUF_SIZE, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_size(ringbuf) != 0, return -1);
    eh_ringbuf_reset(ringbuf);

    /* 用例5 镜像区域半写半读测试 */
    /* 先满读满写进入镜像区域 */
    test_buf_init(test_buf, sizeof(test_buf));
    EH_DBG_ERROR_EXEC(eh_ringbuf_write(ringbuf, test_buf, TEST_BUF_SIZE) != TEST_BUF_SIZE, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_read(ringbuf, test_buf, TEST_BUF_SIZE) != TEST_BUF_SIZE, return -1);
    /* 再次半写半读 */
    test_buf_init(test_buf, sizeof(test_buf));
    EH_DBG_ERROR_EXEC(eh_ringbuf_write(ringbuf, test_buf, TEST_BUF_SIZE/2) != TEST_BUF_SIZE/2, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_free_size(ringbuf) != TEST_BUF_SIZE/2, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_size(ringbuf) != TEST_BUF_SIZE/2, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_read(ringbuf, test_buf, TEST_BUF_SIZE) != TEST_BUF_SIZE/2, return -1);
    EH_DBG_ERROR_EXEC(test_buf_check(test_buf, TEST_BUF_SIZE/2, 0), return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_free_size(ringbuf) != TEST_BUF_SIZE, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_size(ringbuf) != 0, return -1);
    eh_ringbuf_reset(ringbuf);

    /* 用例6 跨镜像边界线读写 */
    /* 先半读半写 */
    test_buf_init(test_buf, sizeof(test_buf));
    EH_DBG_ERROR_EXEC(eh_ringbuf_write(ringbuf, test_buf, TEST_BUF_SIZE/2) != TEST_BUF_SIZE/2, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_read(ringbuf, test_buf, TEST_BUF_SIZE/2) != TEST_BUF_SIZE/2, return -1);
    /* 再次跨镜像边界线读写 */
    test_buf_init(test_buf, sizeof(test_buf));
    EH_DBG_ERROR_EXEC(eh_ringbuf_write(ringbuf, test_buf, TEST_BUF_SIZE) != TEST_BUF_SIZE, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_free_size(ringbuf) != 0, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_size(ringbuf) != TEST_BUF_SIZE, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_read(ringbuf, test_buf, TEST_BUF_SIZE) != TEST_BUF_SIZE, return -1);
    EH_DBG_ERROR_EXEC(test_buf_check(test_buf, TEST_BUF_SIZE, 0), return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_free_size(ringbuf) != TEST_BUF_SIZE, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_size(ringbuf) != 0, return -1);
    eh_ringbuf_reset(ringbuf);

    /* 用例7 跨边界线读写 */
    /* 先半读半写 */
    test_buf_init(test_buf, sizeof(test_buf));
    EH_DBG_ERROR_EXEC(eh_ringbuf_write(ringbuf, test_buf, TEST_BUF_SIZE) != TEST_BUF_SIZE, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_read(ringbuf, test_buf, TEST_BUF_SIZE) != TEST_BUF_SIZE, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_write(ringbuf, test_buf, TEST_BUF_SIZE/3*2) != TEST_BUF_SIZE/3*2, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_read(ringbuf, test_buf, TEST_BUF_SIZE/3*2) != TEST_BUF_SIZE/3*2, return -1);
    /* 再次跨镜像边界线读写 */
    test_buf_init(test_buf, sizeof(test_buf));
    EH_DBG_ERROR_EXEC(eh_ringbuf_write(ringbuf, test_buf, TEST_BUF_SIZE) != TEST_BUF_SIZE, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_free_size(ringbuf) != 0, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_size(ringbuf) != TEST_BUF_SIZE, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_read(ringbuf, test_buf, TEST_BUF_SIZE) != TEST_BUF_SIZE, return -1);
    EH_DBG_ERROR_EXEC(test_buf_check(test_buf, TEST_BUF_SIZE, 0), return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_free_size(ringbuf) != TEST_BUF_SIZE, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_size(ringbuf) != 0, return -1);
    eh_ringbuf_reset(ringbuf);
    
    /* 用例8 多写单读 */
    test_buf_init(test_buf, sizeof(test_buf));
    EH_DBG_ERROR_EXEC(eh_ringbuf_write(ringbuf, test_buf, TEST_BUF_SIZE/3) != TEST_BUF_SIZE/3, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_write(ringbuf, test_buf + TEST_BUF_SIZE/3, TEST_BUF_SIZE/3) != TEST_BUF_SIZE/3, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_write(ringbuf, test_buf+ ((TEST_BUF_SIZE/3)*2), 
        TEST_BUF_SIZE - ((TEST_BUF_SIZE/3)*2)) != TEST_BUF_SIZE - ((TEST_BUF_SIZE/3)*2), return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_free_size(ringbuf) != 0, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_size(ringbuf) != TEST_BUF_SIZE, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_read(ringbuf, test_buf, TEST_BUF_SIZE) != TEST_BUF_SIZE, return -1);
    EH_DBG_ERROR_EXEC(test_buf_check(test_buf, TEST_BUF_SIZE, 0), return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_free_size(ringbuf) != TEST_BUF_SIZE, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_size(ringbuf) != 0, return -1);
    eh_ringbuf_reset(ringbuf);

    /* 用例9 镜像区域多写单读 */
    test_buf_init(test_buf, sizeof(test_buf));
    EH_DBG_ERROR_EXEC(eh_ringbuf_write(ringbuf, test_buf, TEST_BUF_SIZE) != TEST_BUF_SIZE, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_read(ringbuf, test_buf, TEST_BUF_SIZE) != TEST_BUF_SIZE, return -1);

    test_buf_init(test_buf, sizeof(test_buf));
    EH_DBG_ERROR_EXEC(eh_ringbuf_write(ringbuf, test_buf, TEST_BUF_SIZE/3) != TEST_BUF_SIZE/3, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_write(ringbuf, test_buf + TEST_BUF_SIZE/3, TEST_BUF_SIZE/3) != TEST_BUF_SIZE/3, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_write(ringbuf, test_buf+ ((TEST_BUF_SIZE/3)*2), 
        TEST_BUF_SIZE - ((TEST_BUF_SIZE/3)*2)) != TEST_BUF_SIZE - ((TEST_BUF_SIZE/3)*2), return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_free_size(ringbuf) != 0, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_size(ringbuf) != TEST_BUF_SIZE, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_read(ringbuf, test_buf, TEST_BUF_SIZE) != TEST_BUF_SIZE, return -1);
    EH_DBG_ERROR_EXEC(test_buf_check(test_buf, TEST_BUF_SIZE, 0), return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_free_size(ringbuf) != TEST_BUF_SIZE, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_size(ringbuf) != 0, return -1);
    eh_ringbuf_reset(ringbuf);

    /* 用例10 跨镜像区域多写单读 */
    test_buf_init(test_buf, sizeof(test_buf));
    EH_DBG_ERROR_EXEC(eh_ringbuf_write(ringbuf, test_buf, TEST_BUF_SIZE/2) != TEST_BUF_SIZE/2, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_read(ringbuf, test_buf, TEST_BUF_SIZE/2) != TEST_BUF_SIZE/2, return -1);
    
    test_buf_init(test_buf, sizeof(test_buf));
    EH_DBG_ERROR_EXEC(eh_ringbuf_write(ringbuf, test_buf, TEST_BUF_SIZE/3) != TEST_BUF_SIZE/3, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_write(ringbuf, test_buf + TEST_BUF_SIZE/3, TEST_BUF_SIZE/3) != TEST_BUF_SIZE/3, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_write(ringbuf, test_buf+ ((TEST_BUF_SIZE/3)*2), 
        TEST_BUF_SIZE - ((TEST_BUF_SIZE/3)*2)) != TEST_BUF_SIZE - ((TEST_BUF_SIZE/3)*2), return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_free_size(ringbuf) != 0, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_size(ringbuf) != TEST_BUF_SIZE, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_read(ringbuf, test_buf, TEST_BUF_SIZE) != TEST_BUF_SIZE, return -1);
    EH_DBG_ERROR_EXEC(test_buf_check(test_buf, TEST_BUF_SIZE, 0), return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_free_size(ringbuf) != TEST_BUF_SIZE, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_size(ringbuf) != 0, return -1);
    eh_ringbuf_reset(ringbuf);

    /* 用例11 跨区域多写单读 */
    test_buf_init(test_buf, sizeof(test_buf));
    EH_DBG_ERROR_EXEC(eh_ringbuf_write(ringbuf, test_buf, TEST_BUF_SIZE) != TEST_BUF_SIZE, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_read(ringbuf, test_buf, TEST_BUF_SIZE) != TEST_BUF_SIZE, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_write(ringbuf, test_buf, TEST_BUF_SIZE/3*2) != TEST_BUF_SIZE/3*2, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_read(ringbuf, test_buf, TEST_BUF_SIZE/3*2) != TEST_BUF_SIZE/3*2, return -1);
    
    test_buf_init(test_buf, sizeof(test_buf));
    EH_DBG_ERROR_EXEC(eh_ringbuf_write(ringbuf, test_buf, TEST_BUF_SIZE/3) != TEST_BUF_SIZE/3, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_write(ringbuf, test_buf + TEST_BUF_SIZE/3, TEST_BUF_SIZE/3) != TEST_BUF_SIZE/3, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_write(ringbuf, test_buf+ ((TEST_BUF_SIZE/3)*2), 
        TEST_BUF_SIZE - ((TEST_BUF_SIZE/3)*2)) != TEST_BUF_SIZE - ((TEST_BUF_SIZE/3)*2), return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_free_size(ringbuf) != 0, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_size(ringbuf) != TEST_BUF_SIZE, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_read(ringbuf, test_buf, TEST_BUF_SIZE) != TEST_BUF_SIZE, return -1);
    EH_DBG_ERROR_EXEC(test_buf_check(test_buf, TEST_BUF_SIZE, 0), return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_free_size(ringbuf) != TEST_BUF_SIZE, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_size(ringbuf) != 0, return -1);
    eh_ringbuf_reset(ringbuf);

    /* 用例12 多写多读 */
    test_buf_init(test_buf, sizeof(test_buf));
    EH_DBG_ERROR_EXEC(eh_ringbuf_write(ringbuf, test_buf, TEST_BUF_SIZE/3) != TEST_BUF_SIZE/3, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_write(ringbuf, test_buf + TEST_BUF_SIZE/3, TEST_BUF_SIZE/3) != TEST_BUF_SIZE/3, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_write(ringbuf, test_buf+ ((TEST_BUF_SIZE/3)*2), 
        TEST_BUF_SIZE - ((TEST_BUF_SIZE/3)*2)) != TEST_BUF_SIZE - ((TEST_BUF_SIZE/3)*2), return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_free_size(ringbuf) != 0, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_size(ringbuf) != TEST_BUF_SIZE, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_read(ringbuf, test_buf, TEST_BUF_SIZE/3) != TEST_BUF_SIZE/3, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_read(ringbuf, test_buf + TEST_BUF_SIZE/3, TEST_BUF_SIZE/3) != TEST_BUF_SIZE/3, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_read(ringbuf, test_buf+ ((TEST_BUF_SIZE/3)*2), 
        TEST_BUF_SIZE - ((TEST_BUF_SIZE/3)*2)) != TEST_BUF_SIZE - ((TEST_BUF_SIZE/3)*2), return -1);
    EH_DBG_ERROR_EXEC(test_buf_check(test_buf, TEST_BUF_SIZE, 0), return -1);
    eh_ringbuf_reset(ringbuf);

    /* 用例13 镜像区域多写多读*/
    test_buf_init(test_buf, sizeof(test_buf));
    EH_DBG_ERROR_EXEC(eh_ringbuf_write(ringbuf, test_buf, TEST_BUF_SIZE) != TEST_BUF_SIZE, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_read(ringbuf, test_buf, TEST_BUF_SIZE) != TEST_BUF_SIZE, return -1);
    
    test_buf_init(test_buf, sizeof(test_buf));
    EH_DBG_ERROR_EXEC(eh_ringbuf_write(ringbuf, test_buf, TEST_BUF_SIZE/3) != TEST_BUF_SIZE/3, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_write(ringbuf, test_buf + TEST_BUF_SIZE/3, TEST_BUF_SIZE/3) != TEST_BUF_SIZE/3, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_write(ringbuf, test_buf+ ((TEST_BUF_SIZE/3)*2), 
        TEST_BUF_SIZE - ((TEST_BUF_SIZE/3)*2)) != TEST_BUF_SIZE - ((TEST_BUF_SIZE/3)*2), return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_free_size(ringbuf) != 0, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_size(ringbuf) != TEST_BUF_SIZE, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_read(ringbuf, test_buf, TEST_BUF_SIZE/3) != TEST_BUF_SIZE/3, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_read(ringbuf, test_buf + TEST_BUF_SIZE/3, TEST_BUF_SIZE/3) != TEST_BUF_SIZE/3, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_read(ringbuf, test_buf+ ((TEST_BUF_SIZE/3)*2), 
        TEST_BUF_SIZE - ((TEST_BUF_SIZE/3)*2)) != TEST_BUF_SIZE - ((TEST_BUF_SIZE/3)*2), return -1);
    EH_DBG_ERROR_EXEC(test_buf_check(test_buf, TEST_BUF_SIZE, 0), return -1);
    eh_ringbuf_reset(ringbuf);

    /* 用例14 跨镜像区域多写多读 */
    test_buf_init(test_buf, sizeof(test_buf));
    EH_DBG_ERROR_EXEC(eh_ringbuf_write(ringbuf, test_buf, TEST_BUF_SIZE/2) != TEST_BUF_SIZE/2, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_read(ringbuf, test_buf, TEST_BUF_SIZE/2) != TEST_BUF_SIZE/2, return -1);
    
    test_buf_init(test_buf, sizeof(test_buf));
    EH_DBG_ERROR_EXEC(eh_ringbuf_write(ringbuf, test_buf, TEST_BUF_SIZE/3) != TEST_BUF_SIZE/3, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_write(ringbuf, test_buf + TEST_BUF_SIZE/3, TEST_BUF_SIZE/3) != TEST_BUF_SIZE/3, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_write(ringbuf, test_buf+ ((TEST_BUF_SIZE/3)*2), 
        TEST_BUF_SIZE - ((TEST_BUF_SIZE/3)*2)) != TEST_BUF_SIZE - ((TEST_BUF_SIZE/3)*2), return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_free_size(ringbuf) != 0, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_size(ringbuf) != TEST_BUF_SIZE, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_read(ringbuf, test_buf, TEST_BUF_SIZE/3) != TEST_BUF_SIZE/3, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_read(ringbuf, test_buf + TEST_BUF_SIZE/3, TEST_BUF_SIZE/3) != TEST_BUF_SIZE/3, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_read(ringbuf, test_buf+ ((TEST_BUF_SIZE/3)*2), 
        TEST_BUF_SIZE - ((TEST_BUF_SIZE/3)*2)) != TEST_BUF_SIZE - ((TEST_BUF_SIZE/3)*2), return -1);
    EH_DBG_ERROR_EXEC(test_buf_check(test_buf, TEST_BUF_SIZE, 0), return -1);
    eh_ringbuf_reset(ringbuf);

    /* 用例15 跨区域多写多读 */
    test_buf_init(test_buf, sizeof(test_buf));
    EH_DBG_ERROR_EXEC(eh_ringbuf_write(ringbuf, test_buf, TEST_BUF_SIZE) != TEST_BUF_SIZE, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_read(ringbuf, test_buf, TEST_BUF_SIZE) != TEST_BUF_SIZE, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_write(ringbuf, test_buf, TEST_BUF_SIZE/3*2) != TEST_BUF_SIZE/3*2, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_read(ringbuf, test_buf, TEST_BUF_SIZE/3*2) != TEST_BUF_SIZE/3*2, return -1);
    
    test_buf_init(test_buf, sizeof(test_buf));
    EH_DBG_ERROR_EXEC(eh_ringbuf_write(ringbuf, test_buf, TEST_BUF_SIZE/3) != TEST_BUF_SIZE/3, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_write(ringbuf, test_buf + TEST_BUF_SIZE/3, TEST_BUF_SIZE/3) != TEST_BUF_SIZE/3, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_write(ringbuf, test_buf+ ((TEST_BUF_SIZE/3)*2), 
        TEST_BUF_SIZE - ((TEST_BUF_SIZE/3)*2)) != TEST_BUF_SIZE - ((TEST_BUF_SIZE/3)*2), return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_free_size(ringbuf) != 0, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_size(ringbuf) != TEST_BUF_SIZE, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_read(ringbuf, test_buf, TEST_BUF_SIZE/3) != TEST_BUF_SIZE/3, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_read(ringbuf, test_buf + TEST_BUF_SIZE/3, TEST_BUF_SIZE/3) != TEST_BUF_SIZE/3, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_read(ringbuf, test_buf+ ((TEST_BUF_SIZE/3)*2), 
        TEST_BUF_SIZE - ((TEST_BUF_SIZE/3)*2)) != TEST_BUF_SIZE - ((TEST_BUF_SIZE/3)*2), return -1);
    EH_DBG_ERROR_EXEC(test_buf_check(test_buf, TEST_BUF_SIZE, 0), return -1);
    eh_ringbuf_reset(ringbuf);

    
    /* 用例12 单写多读 */
    test_buf_init(test_buf, sizeof(test_buf));
    EH_DBG_ERROR_EXEC(eh_ringbuf_write(ringbuf, test_buf, TEST_BUF_SIZE) != TEST_BUF_SIZE, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_free_size(ringbuf) != 0, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_size(ringbuf) != TEST_BUF_SIZE, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_read(ringbuf, test_buf, TEST_BUF_SIZE/3) != TEST_BUF_SIZE/3, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_read(ringbuf, test_buf + TEST_BUF_SIZE/3, TEST_BUF_SIZE/3) != TEST_BUF_SIZE/3, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_read(ringbuf, test_buf+ ((TEST_BUF_SIZE/3)*2), 
        TEST_BUF_SIZE - ((TEST_BUF_SIZE/3)*2)) != TEST_BUF_SIZE - ((TEST_BUF_SIZE/3)*2), return -1);
    EH_DBG_ERROR_EXEC(test_buf_check(test_buf, TEST_BUF_SIZE, 0), return -1);
    eh_ringbuf_reset(ringbuf);

    
    /* 用例12 镜像区域单写多读 */
    test_buf_init(test_buf, sizeof(test_buf));
    EH_DBG_ERROR_EXEC(eh_ringbuf_write(ringbuf, test_buf, TEST_BUF_SIZE) != TEST_BUF_SIZE, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_read(ringbuf, test_buf, TEST_BUF_SIZE) != TEST_BUF_SIZE, return -1);

    test_buf_init(test_buf, sizeof(test_buf));
    EH_DBG_ERROR_EXEC(eh_ringbuf_write(ringbuf, test_buf, TEST_BUF_SIZE) != TEST_BUF_SIZE, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_free_size(ringbuf) != 0, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_size(ringbuf) != TEST_BUF_SIZE, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_read(ringbuf, test_buf, TEST_BUF_SIZE/3) != TEST_BUF_SIZE/3, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_read(ringbuf, test_buf + TEST_BUF_SIZE/3, TEST_BUF_SIZE/3) != TEST_BUF_SIZE/3, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_read(ringbuf, test_buf+ ((TEST_BUF_SIZE/3)*2), 
        TEST_BUF_SIZE - ((TEST_BUF_SIZE/3)*2)) != TEST_BUF_SIZE - ((TEST_BUF_SIZE/3)*2), return -1);
    EH_DBG_ERROR_EXEC(test_buf_check(test_buf, TEST_BUF_SIZE, 0), return -1);
    eh_ringbuf_reset(ringbuf);

    
    /* 用例13 跨镜像区域单写多读 */
    test_buf_init(test_buf, sizeof(test_buf));
    EH_DBG_ERROR_EXEC(eh_ringbuf_write(ringbuf, test_buf, TEST_BUF_SIZE/2) != TEST_BUF_SIZE/2, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_read(ringbuf, test_buf, TEST_BUF_SIZE/2) != TEST_BUF_SIZE/2, return -1);
    
    test_buf_init(test_buf, sizeof(test_buf));
    EH_DBG_ERROR_EXEC(eh_ringbuf_write(ringbuf, test_buf, TEST_BUF_SIZE) != TEST_BUF_SIZE, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_free_size(ringbuf) != 0, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_size(ringbuf) != TEST_BUF_SIZE, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_read(ringbuf, test_buf, TEST_BUF_SIZE/3) != TEST_BUF_SIZE/3, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_read(ringbuf, test_buf + TEST_BUF_SIZE/3, TEST_BUF_SIZE/3) != TEST_BUF_SIZE/3, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_read(ringbuf, test_buf+ ((TEST_BUF_SIZE/3)*2), 
        TEST_BUF_SIZE - ((TEST_BUF_SIZE/3)*2)) != TEST_BUF_SIZE - ((TEST_BUF_SIZE/3)*2), return -1);
    EH_DBG_ERROR_EXEC(test_buf_check(test_buf, TEST_BUF_SIZE, 0), return -1);
    eh_ringbuf_reset(ringbuf);

    
    /* 用例14 跨区域单写多读 */
    test_buf_init(test_buf, sizeof(test_buf));
    EH_DBG_ERROR_EXEC(eh_ringbuf_write(ringbuf, test_buf, TEST_BUF_SIZE) != TEST_BUF_SIZE, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_read(ringbuf, test_buf, TEST_BUF_SIZE) != TEST_BUF_SIZE, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_write(ringbuf, test_buf, TEST_BUF_SIZE/3*2) != TEST_BUF_SIZE/3*2, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_read(ringbuf, test_buf, TEST_BUF_SIZE/3*2) != TEST_BUF_SIZE/3*2, return -1);
    
    test_buf_init(test_buf, sizeof(test_buf));
    EH_DBG_ERROR_EXEC(eh_ringbuf_write(ringbuf, test_buf, TEST_BUF_SIZE) != TEST_BUF_SIZE, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_free_size(ringbuf) != 0, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_size(ringbuf) != TEST_BUF_SIZE, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_read(ringbuf, test_buf, TEST_BUF_SIZE/3) != TEST_BUF_SIZE/3, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_read(ringbuf, test_buf + TEST_BUF_SIZE/3, TEST_BUF_SIZE/3) != TEST_BUF_SIZE/3, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_read(ringbuf, test_buf+ ((TEST_BUF_SIZE/3)*2), 
        TEST_BUF_SIZE - ((TEST_BUF_SIZE/3)*2)) != TEST_BUF_SIZE - ((TEST_BUF_SIZE/3)*2), return -1);
    EH_DBG_ERROR_EXEC(test_buf_check(test_buf, TEST_BUF_SIZE, 0), return -1);
    eh_ringbuf_reset(ringbuf);

    /* 用例15 skip测试*/
    test_buf_init(test_buf, sizeof(test_buf));
    EH_DBG_ERROR_EXEC(eh_ringbuf_write(ringbuf, test_buf, TEST_BUF_SIZE) != TEST_BUF_SIZE, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_read_skip(ringbuf, TEST_BUF_SIZE) != TEST_BUF_SIZE, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_free_size(ringbuf) != TEST_BUF_SIZE, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_size(ringbuf) != 0, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_free_size(ringbuf) != TEST_BUF_SIZE, return -1);

    /* 用例16 peek测试 */
    test_buf_init(test_buf, sizeof(test_buf));
    EH_DBG_ERROR_EXEC(eh_ringbuf_write(ringbuf, test_buf, TEST_BUF_SIZE) != TEST_BUF_SIZE, return -1);
    len = TEST_BUF_SIZE;
    EH_DBG_ERROR_EXEC(test_buf_check(eh_ringbuf_peek(ringbuf, 0, test_buf, &len), (size_t)len, 0), return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_read_skip(ringbuf, len) != TEST_BUF_SIZE, return -1);
    eh_ringbuf_reset(ringbuf);

    /* 用例17 peek过量测试 */
    test_buf_init(test_buf, sizeof(test_buf));
    EH_DBG_ERROR_EXEC(eh_ringbuf_write(ringbuf, test_buf, TEST_BUF_SIZE/2) != TEST_BUF_SIZE/2, return -1);
    len = TEST_BUF_SIZE;
    EH_DBG_ERROR_EXEC(eh_ringbuf_peek(ringbuf, 0, test_buf, &len), return -1);
    eh_ringbuf_reset(ringbuf);

    /** 用例18 跨区域peek */
    test_buf_init(test_buf, sizeof(test_buf));
    EH_DBG_ERROR_EXEC(eh_ringbuf_write(ringbuf, test_buf, TEST_BUF_SIZE/2) != TEST_BUF_SIZE/2, return -1);
    eh_ringbuf_clear(ringbuf);
    
    test_buf_init(test_buf, sizeof(test_buf));
    EH_DBG_ERROR_EXEC(eh_ringbuf_write(ringbuf, test_buf, TEST_BUF_SIZE) != TEST_BUF_SIZE, return -1);
    len = TEST_BUF_SIZE;
    EH_DBG_ERROR_EXEC(test_buf_check(eh_ringbuf_peek(ringbuf, 0, test_buf, &len), (size_t)len, 0), return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_read_skip(ringbuf, len) != TEST_BUF_SIZE, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_free_size(ringbuf) != TEST_BUF_SIZE, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_size(ringbuf) != 0, return -1);
    eh_ringbuf_reset(ringbuf);

    /* 用例19 少量非跨区域peek */
    test_buf_init(test_buf, sizeof(test_buf));
    EH_DBG_ERROR_EXEC(eh_ringbuf_write(ringbuf, test_buf, TEST_BUF_SIZE) != TEST_BUF_SIZE, return -1);
    len = TEST_BUF_SIZE/2;
    EH_DBG_ERROR_EXEC(test_buf_check(eh_ringbuf_peek(ringbuf, 0, test_buf, &len), (size_t)len, 0), return -1);
    EH_DBG_ERROR_EXEC(len != TEST_BUF_SIZE,  return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_read_skip(ringbuf, len) != TEST_BUF_SIZE, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_free_size(ringbuf) != TEST_BUF_SIZE, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_size(ringbuf) != 0, return -1);
    eh_ringbuf_reset(ringbuf);

    /* 用例20 少量跨区域peek */
    test_buf_init(test_buf, sizeof(test_buf));
    EH_DBG_ERROR_EXEC(eh_ringbuf_write(ringbuf, test_buf, TEST_BUF_SIZE/2) != TEST_BUF_SIZE/2, return -1);
    eh_ringbuf_clear(ringbuf);

    test_buf_init(test_buf, sizeof(test_buf));
    EH_DBG_ERROR_EXEC(eh_ringbuf_write(ringbuf, test_buf, TEST_BUF_SIZE) != TEST_BUF_SIZE, return -1);
    len = TEST_BUF_SIZE/2;
    EH_DBG_ERROR_EXEC(test_buf_check(eh_ringbuf_peek(ringbuf, 0, test_buf, &len), (size_t)len, 0), return -1);
    EH_DBG_ERROR_EXEC(len != TEST_BUF_SIZE/2,  return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_read_skip(ringbuf, len) != len, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_free_size(ringbuf) != TEST_BUF_SIZE/2, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_size(ringbuf) != TEST_BUF_SIZE/2, return -1);
    eh_ringbuf_reset(ringbuf);

    /* 用例21 少量非跨区区域peek */
    test_buf_init(test_buf, sizeof(test_buf));
    EH_DBG_ERROR_EXEC(eh_ringbuf_write(ringbuf, test_buf, 1000) != 1000, return -1);
    EH_DBG_ERROR_EXEC(eh_ringbuf_read_skip(ringbuf, 200) != 200, return -1);
    len = 200;
    EH_DBG_ERROR_EXEC(test_buf_check(eh_ringbuf_peek(ringbuf, 0, test_buf, &len), (size_t)len, 200), return -1);
    eh_ringbuf_reset(ringbuf);
    

    /* 用例21 并行随机读写测试 */
    EH_DBG_ERROR_EXEC(test_random_wr(100049, 10000)!=0, return -1);

    /* 用例22 并行随机偷看写测试 */
    EH_DBG_ERROR_EXEC(test_random_wr_peek(1024*200, 10000)!=0, return -1);
    

    return 0;

}


int task_app(void *arg){
    (void) arg;
    eh_ringbuf_t* ringbuf;
    int ret;

    
    ringbuf = eh_ringbuf_create(TEST_BUF_SIZE, NULL);
    if(eh_ptr_to_error(ringbuf)){
        eh_errfl("test_eh error: %d", eh_ptr_to_error(ringbuf));
        return -1;
    }

    ret = test_basics_interface(ringbuf);
    if(ret){
        eh_errfl("test_basics_interface Fail");
        return -1;
    }
    eh_ringbuf_destroy(ringbuf);
    eh_debugfl("test_basics_interface Pass");

    return 0;
}


int main(void){
    eh_debugfl("test_ringbuf start!!");
    eh_global_init();
    task_app("task_app");
    eh_global_exit();
    return 0;
}