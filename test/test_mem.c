/**
 * @file test_mem.c
 * @brief  malloc测试
 * @author simon.xiaoapeng (simon.xiaoapeng@gmail.com)
 * @date 2024-05-12
 * 
 * @copyright Copyright (c) 2024  simon.xiaoapeng@gmail.com
 * 
 */

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <eh.h>
#include "eh_config.h"
#include <eh_debug.h>
#include <eh_event.h>
#include <eh_internal.h>
#include <eh_mem.h>
#include <eh_platform.h>
#include <eh_timer.h> 
#include <eh_types.h>

#if  defined(EH_CONFIG_USE_LIBC_MEM_MANAGE) && EH_CONFIG_USE_LIBC_MEM_MANAGE == 0

#define TEST_MEM_MALLOC_MAX_SIZE (3000)
#define TEST_MEM_HU_TIME         (1000*30)
#define TEST_PTR_NUM             (3000)
#define TEST_MEM_CYCLE           (1000)
#define TEST_MEM_CYCLE_COUNT     (5)


void *p[TEST_PTR_NUM];

typedef struct {
    eh_event_timer_t timer_event;
    int inode;
}test_data_t;
static test_data_t *test_data[TEST_PTR_NUM] = {NULL};
static size_t max_block = 0;


void stdout_write(void *stream, const uint8_t *buf, size_t size){
    (void)stream;
    printf("%.*s", (int)size, (const char*)buf);
}

void dump_func(void* start, size_t size){
    (void)start;
    (void)size;
    //eh_infoln("[0x%p  0x%p %-8x]",start, (uint8_t*)start + size, (uint32_t)size);
}

void dump_find_max_block_func(void* start, size_t size){
    (void)start;
    if(size > max_block)
        max_block = size;
}


void dump_max_block(void){
    max_block = 0;
    eh_free_block_dump(dump_find_max_block_func);
    eh_infoln("max_block %llu",max_block);
}

void test_rand_malloc_add_epoll(eh_epoll_t epoll){
    int i,malloc_i = 0;
    struct eh_mem_heap_info info;
    size_t malloc_size;
    void* placeholder = NULL;
    for(i=0; i<TEST_PTR_NUM; i++){
        if(test_data[i]) continue;
        if(placeholder == NULL){
            placeholder = eh_malloc(0x100);
        }
        if(placeholder == NULL){
            eh_errfl("cnt %d malloc failed!! malloc_size %d", malloc_i, 0x100);
            dump_max_block();
            eh_mem_get_heap_info(&info);
            eh_infoln("%llu %llu %llu ",info.free_size, info.total_size, info.min_ever_free_size_level);   
            break;
        }
        eh_mem_get_heap_info(&info);
        if(info.free_size < TEST_MEM_MALLOC_MAX_SIZE){
            malloc_size = info.free_size;
        }else{
            malloc_size = (size_t)(rand() % TEST_MEM_MALLOC_MAX_SIZE) + sizeof(test_data_t);
        }
        for(; malloc_size >= sizeof(test_data_t); malloc_size /= 2){
            test_data[i] = eh_malloc(malloc_size);
            if(test_data[i]) break;
        }

        if(test_data[i] == NULL){
            eh_errfl("cnt %d malloc failed!! malloc_size %d", malloc_i, malloc_size);
            dump_max_block();
            eh_mem_get_heap_info(&info);
            eh_infoln("%llu %llu %llu ",info.free_size, info.total_size, info.min_ever_free_size_level);   
            eh_free_block_dump(dump_func);
            break;
        }
        malloc_i++;
        test_data[i]->inode = i;
        if(placeholder){
            eh_free(placeholder);
            placeholder = NULL;
        }
        eh_timer_init(&test_data[i]->timer_event);
        eh_timer_config_interval(&test_data[i]->timer_event, eh_msec_to_clock( (eh_msec_t)((rand() % TEST_MEM_HU_TIME)+1) ) );
        eh_epoll_add_event(epoll, eh_timer_to_event(&test_data[i]->timer_event), test_data[i]);
        eh_timer_start(&test_data[i]->timer_event);
    }
    if(placeholder){
        eh_free(placeholder);
        placeholder = NULL;
    }
}

int task_app(void *arg){
    (void)arg;
    struct eh_mem_heap_info info;
    // 初始化随机数种子
    srand((unsigned int)eh_get_clock_monotonic_time());

    
    eh_mem_get_heap_info(&info);
    eh_infoln("%llu %llu %llu ",info.free_size, info.total_size, info.min_ever_free_size_level);
    eh_infofl("基础测试:");
    eh_free_block_dump(dump_func);


    p[0] = eh_malloc(1);
    p[1] = eh_malloc(3);
    p[2] = eh_malloc(7);
    p[3] = eh_malloc(5);
    p[4] = eh_malloc(8);
    p[5] = eh_malloc(34);
    p[6] = eh_malloc(53);
    p[7] = eh_malloc(2);
    p[8] = eh_malloc(6);

    eh_infofl("基础测试:");
    eh_free_block_dump(dump_func);

    eh_free(p[0]);
    eh_free(p[2]);
    eh_free(p[4]);
    eh_free(p[6]);
    eh_free(p[8]);

    eh_infofl("基础测试:");
    eh_free_block_dump(dump_func);

    p[0] = eh_malloc(1);
    eh_infofl("基础测试:");
    eh_free_block_dump(dump_func);
    p[2] = eh_malloc(7);
    p[4] = eh_malloc(8);
    p[6] = eh_malloc(53);
    p[8] = eh_malloc(6);

    eh_infofl("基础测试:");
    eh_free_block_dump(dump_func);

    eh_free(p[0]);
    eh_free(p[1]);
    eh_free(p[2]);
    eh_free(p[3]);
    eh_free(p[4]);
    eh_free(p[5]);
    eh_free(p[6]);
    eh_free(p[7]);
    eh_free(p[8]);
    eh_infofl("基础测试:");
    eh_free_block_dump(dump_func);


    {
        eh_epoll_slot_t epoll_slot[12];
        eh_epoll_t epoll = eh_epoll_new();
        eh_event_timer_t timer_test; /* 测试过程中打印和结束定时器 */
        int timer_timeout_cnt = 0;
        memset(test_data, 0, sizeof(test_data));
        /* 随机碎片化测试 */
        eh_timer_init(&timer_test);
        eh_timer_config_interval(&timer_test, eh_msec_to_clock(TEST_MEM_CYCLE));
        eh_timer_set_attr(&timer_test, EH_TIMER_ATTR_AUTO_CIRCULATION);
        eh_epoll_add_event(epoll, eh_timer_to_event(&timer_test), NULL);
        eh_timer_start(&timer_test);
        test_rand_malloc_add_epoll(epoll);
        while(1){
            int ret;
            ret = eh_epoll_wait(epoll, epoll_slot, 12, EH_TIME_FOREVER);
            if(ret <= 0){
                eh_errfl("eh_epoll_wait failed!!");
                goto test_rand_exit;
            }
            for(int i=0; i<ret; i++){
                if(epoll_slot[i].event == eh_timer_to_event(&timer_test)){
                    eh_infofl("timer_test timeout!! %d %llu ", timer_timeout_cnt, eh_clock_to_msec(eh_get_clock_monotonic_time()));
                    test_rand_malloc_add_epoll(epoll);
                    eh_free_block_dump(dump_func);
                    timer_timeout_cnt++;
                    if(timer_timeout_cnt > TEST_MEM_CYCLE_COUNT){
                        eh_infofl("随机测试结束...");
                        goto test_rand_exit;
                    }
                }else{
                    test_data_t *test_data_tmp = (test_data_t*)epoll_slot[i].userdata;
                    eh_epoll_del_event(epoll, epoll_slot[i].event);
                    eh_timer_clean(&test_data_tmp->timer_event);
                    test_data[test_data_tmp->inode] = NULL;
                    eh_free(test_data_tmp);
                }
            }
        }
    test_rand_exit:
        eh_epoll_del_event(epoll, eh_timer_to_event(&timer_test));
        eh_timer_clean(&timer_test);
        for(int i=0; i<TEST_PTR_NUM; i++){
            if(test_data[i] != NULL){
                eh_epoll_del_event(epoll, eh_timer_to_event(&test_data[i]->timer_event));
                eh_timer_clean(&test_data[i]->timer_event);
                eh_free(test_data[i]);
                test_data[i] = NULL;
            }
        }
        eh_epoll_del(epoll);
        eh_infofl("碎片化测试:");
        eh_free_block_dump(dump_func);
    }
    eh_mem_get_heap_info(&info);
    eh_infoln("%llu %llu %llu ",info.free_size, info.total_size, info.min_ever_free_size_level);

    return 0;
}

static uint8_t heap0_array[1024*1024];
static const struct eh_mem_heap heap0 = {
    .heap_start = heap0_array,
    .heap_size = sizeof(heap0_array)
};

static uint8_t heap1_array[1024*1024];
static const struct eh_mem_heap heap1 = {
    .heap_start = heap1_array,
    .heap_size = sizeof(heap1_array)
};

int main(void){
    eh_debugfl("test_eh start!!");

    eh_mem_heap_register(&heap0);
    eh_mem_heap_register(&heap1);

    eh_global_init();
    task_app("task_app");
    eh_global_exit();
    return 0;
}

#else

int main(void){
    eh_debugfl("Do not test the c library.");
    return 0;
}
#endif