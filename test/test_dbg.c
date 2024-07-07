/**
 * @file test_dbg.c
 * @brief 
 * @author simon.xiaoapeng (simon.xiaoapeng@gmail.com)
 * @version 1.0
 * @date 2024-07-06
 * 
 * @copyright Copyright (c) 2024  simon.xiaoapeng@gmail.com
 * 
 * @par 修改日志:
 */

#include <stdint.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <stdio.h>
#include "debug.h"
#include "eh.h"
#include "eh_event.h"
#include "eh_sleep.h"
#include "eh_platform.h"
#include "eh_formatio.h"
#include "eh_timer.h" 
#include "eh_types.h"

void stdout_write(void *stream, const uint8_t *buf, size_t size){
    (void)stream;
    printf("%.*s", (int)size, (const char*)buf);
}

union double_union{
    double d;
    struct{
        uint64_t mantissa:52;
        uint64_t exponent:11;
        uint64_t sign:1;
    };
    struct{
        uint64_t v64;
    };
};


int task_app(void *arg){
    (void)arg;
    // double d = 123.5, ds;
    // int k;
    // union double_union du = {.d = d};
    // ds = (du.d - 1.5) * 0.289529654602168 + 0.1760912590558 + (du.exponent - 1023) * 0.301029995663981;
    // k = (int)ds;
    
    // if (ds < 0. && ds != k)
    //     k--;			/* want k = floor(ds) */
    // eh_printf("k:%d\n",k);
    // eh_printf("v64:%#.16lx\n", du.v64);
    // eh_printf("mantissa:%lu exponent:%lu sign:%lu \n", (uint64_t)du.mantissa, (uint64_t)du.exponent - 1023, (uint64_t)du.sign);
    // eh_printf("mantissa:%lb exponent:%lu sign:%lu \n", (uint64_t)du.mantissa, (uint64_t)du.exponent - 1023, (uint64_t)du.sign);


    eh_printf("test1 |x:%16x| |lx:%32lx| |llx:%32llx|\n", 0x1000, 0x1234UL, 0x34356ULL);
    eh_printf("test1 |x:%#16x| |lx:%#32lx| |llx:%#32llx|\n", 0x1000, 0x1234UL, 0x34356ULL);
    eh_printf("test1 |x:%#16.8x| |lx:%#32.16lx| |llx:%#32.16llx|\n", 0x1000, 0x1234UL, 0x34356ULL);
    eh_printf("test1 |x:%#16.8x| |lx:%#32.16lx| |llx:%#32.16llx|\n", 0x1000, 0x1234UL, 0x34356ULL);
    eh_printf("test1 |x:%-#16.8x| |lx:%-#32.16lx| |llx:%-#32.16llx|\n\n", 0x1000, 0x1234UL, 0x34356ULL);

    
    eh_printf("test2 |d:%16d| |ld:%32ld| |lld:%32lld|\n", 1000, 1234UL, 34356ULL);
    eh_printf("test2 |d:%16d| |ld:%32ld| |lld:%32lld|\n", 02300, 1234UL, 34356ULL);
    eh_printf("test2 |d:%16.8d| |ld:%32.16ld| |lld:%32.16lld|\n", 1000, 1234UL, 34356ULL);
    eh_printf("test2 |d:%16.8d| |ld:%32.16ld| |lld:%32.16lld|\n", 1000, 1234UL, 34356ULL);
    eh_printf("test2 |d:%-16.8d| |ld:%-32.16ld| |ld:%-32.16lld|\n\n", 1000, 1234UL, 34356ULL);

    
    eh_printf("test1 |o:%16o| |lo:%32lo| |llo:%32llo|\n", 01000, 01234UL, 04356ULL);
    eh_printf("test1 |o:%#16o| |lo:%#32lo| |llo:%#32llo|\n", 01000, 01234UL, 034356ULL);
    eh_printf("test1 |o:%#16.8o| |lo:%#32.16lo| |llo:%#32.16llo|\n", 01000, 01234UL, 034356ULL);
    eh_printf("test1 |o:%#16.8o| |lo:%#32.16lo| |llo:%#32.16llo|\n", 01000, 01234UL, 034356ULL);
    eh_printf("test1 |o:%-#16.8o| |lo:%-#32.16lo| |llo:%-#32.16llo|\n\n", 01000, 01234UL, 034356ULL);

    
    // eh_printf("test1 |b:%16b| |lb:%32lb| |llb:%32llb|\n", 01000, 01234UL, 04356ULL);
    // eh_printf("test1 |b:%#16b| |lb:%#32lb| |llb:%#32llb|\n", 01000, 01234UL, 034356ULL);
    // eh_printf("test1 |b:%#16.8b| |lb:%#32.16lb| |llb:%#32.16llb|\n", 01000, 01234UL, 034356ULL);
    // eh_printf("test1 |b:%#16.8b| |lb:%#32.16lb| |llb:%#32.16llb|\n", 01000, 01234UL, 034356ULL);
    // eh_printf("test1 |b:%-#16.8b| |lb:%-#32.16lb| |llb:%-#32.16llb|\n\n", 01000, 01234UL, 034356ULL);

    eh_printf("test2 |%s|\n","tesr hhh");
    eh_printf("test2 |%.*s|\n", 5, "tesr hhh");
    eh_printf("test2 |%8.*s|\n", 5, "tesr hhh");
    
    eh_printf("test2 |%zd|\n", sizeof(long double));

    eh_loop_exit(0);
    return 0;
}



int main(void){
    
    
    eh_global_init();
    
    eh_task_create("task_app", 12*1024, "task_app", task_app);

    eh_loop_run();
    eh_global_exit();
    return 0;
}

