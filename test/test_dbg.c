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

#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <stdio.h>
#include <unistd.h>
#include "eh.h"
#include "eh_debug.h"
#include "eh_event.h"
#include "eh_sleep.h"
#include "eh_platform.h"
#include "eh_formatio.h"
#include "eh_timer.h" 
#include "eh_types.h"
#include "eh_debug.h"

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

static char test_buf[1024];

int task_app(void *arg){
    (void)arg;
    int n;
    

    n = eh_printf("test1 x:|%16x| lx:|%32lx| llx:|%32llx|\n", 0x1000, 0x1234UL, 0x34356ULL);
    n = eh_printf("n=%d\n",n);
    n = eh_printf("test1 x:|%#16x| lx:|%#32lx| llx:|%#32llx|\n", 0x1000, 0x1234UL, 0x34356ULL);
    n = eh_printf("n=%d\n",n);
    n = eh_printf("test1 x:|%#16.8x| lx:|%#32.16lx| llx:|%#32.16llx|\n", 0x1000, 0x1234UL, 0x34356ULL);
    n = eh_printf("n=%d\n",n);
    n = eh_printf("test1 x:|%#16.8x| lx:|%#32.16lx| llx:|%#32.16llx|\n", 0x1000, 0x1234UL, 0x34356ULL);
    n = eh_printf("n=%d\n",n);
    n = eh_printf("test1 x:|%-#16.8x| lx:|%-#32.16lx| llx:|%-#32.16llx|\n\n", 0x1000, 0x1234UL, 0x34356ULL);
    n = eh_printf("n=%d\n",n);

    
    n = eh_printf("test2 d:|%16d| ld:|%32ld| lld:|%32lld|\n", 1000, 1234UL, 34356ULL);
    n = eh_printf("n=%d\n",n);
    n = eh_printf("test2 d:|%16d| ld:|%32ld| lld:|%32lld|\n", 02300, 1234UL, 34356ULL);
    n = eh_printf("n=%d\n",n);
    n = eh_printf("test2 d:|%16.8d| ld:|%32.16ld| lld:|%32.16lld|\n", 1000, 1234UL, 34356ULL);
    n = eh_printf("n=%d\n",n);
    n = eh_printf("test2 d:|%16.8d| ld:|%32.16ld| lld:|%32.16lld|\n", 1000, 1234UL, 34356ULL);
    n = eh_printf("n=%d\n",n);
    n = eh_printf("test2 d:|%-16.8d| ld:|%-32.16ld| lld:|%-32.16lld|\n\n", 1000, 1234UL, 34356ULL);
    n = eh_printf("n=%d\n",n);

    
    n = eh_printf("test1 o:|%16o| lo:|%32lo| llo:|%32llo|\n", 01000, 01234UL, 04356ULL);
    n = eh_printf("n=%d\n",n);
    n = eh_printf("test1 o:|%#16o| lo:|%#32lo| llo:|%#32llo|\n", 01000, 01234UL, 034356ULL);
    n = eh_printf("n=%d\n",n);
    n = eh_printf("test1 o:|%#16.8o| lo:|%#32.16lo| llo:|%#32.16llo|\n", 01000, 01234UL, 034356ULL);
    n = eh_printf("n=%d\n",n);
    n = eh_printf("test1 o:|%#16.8o| lo:|%#32.16lo| llo:|%#32.16llo|\n", 01000, 01234UL, 034356ULL);
    n = eh_printf("n=%d\n",n);
    n = eh_printf("test1 o:|%-#16.8o| lo:|%-#32.16lo| llo:|%-#32.16llo|\n\n", 01000, 01234UL, 034356ULL);
    n = eh_printf("n=%d\n",n);

    /* 扩展语法 */
    // eh_printf("test1 b:|%16b| lb:|%32lb| llb:|%32llb|\n", 01000, 01234UL, 04356ULL);
    // eh_printf("test1 b:|%#16b| lb:|%#32lb| llb:|%#32llb|\n", 01000, 01234UL, 034356ULL);
    // eh_printf("test1 b:|%#16.8b| lb:|%#32.16lb| llb:|%#32.16llb|\n", 01000, 01234UL, 034356ULL);
    // eh_printf("test1 b:|%#16.8b| lb:|%#32.16lb| llb:|%#32.16llb|\n", 01000, 01234UL, 034356ULL);
    // eh_printf("test1 b:|%-#16.8b| lb:|%-#32.16lb| llb:|%-#32.16llb|\n\n", 01000, 01234UL, 034356ULL);

    n = eh_printf("test1   |%16.8f| xx:|%32.16f| xxx:|%-32.16f|\n", 1.23456789, 1.23456789, 1.23456789);
    n = eh_printf("n=%d\n",n);
  
    n = eh_printf("test2   |%32s|\n","test hhh");
    n = eh_printf("n=%d\n",n);
    n = eh_printf("test2   |%32.*s|\n", 5, "test hhh");
    n = eh_printf("n=%d\n",n);
    n = eh_printf("test2   |%-32.*s|\n", 1000000000, "test hhh");
    n = eh_printf("n=%d\n",n);
    
    n = eh_printf("test2   |%zd|\n", sizeof(long double));
    n = eh_printf("n=%d\n",n);

    n = eh_snprintf(test_buf, sizeof(test_buf), "12345678123456781234567812345678123456781234567812345678123456781234567812345678");
    n = eh_printf("test_buf:|%s|  n=%d\n", test_buf, n);

    static uint8_t test_buf[] = {
        0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
        0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,
        0x55
    };
    n = eh_printf("|%-.*Q|\n", (int)sizeof(test_buf), test_buf);
    n = eh_printf("n=%d\n",n);

    eh_debugfl("TEST %d",1234);
    eh_infofl("TEST %d",1234);
    eh_sysfl("TEST %d",1234);
    eh_warnfl("TEST %d",1234);
    eh_errfl("TEST %d\n",1234);

    eh_dbg_set_level(EH_DBG_WARNING);

    eh_debugln("TEST %d",1234);
    eh_infoln("TEST %d",1234);
    eh_sysln("TEST %d",1234);
    eh_warnfl("TEST %d",1234);
    eh_errln("TEST %d",1234);

    
    eh_debugln("|%.*hhq|", (int)sizeof(test_buf), test_buf);
    eh_debughex(test_buf, sizeof(test_buf));
    eh_errhex(test_buf, sizeof(test_buf));
    eh_loop_exit(0);
    return 0;
}



int main(void){
    
    
    eh_global_init();
    
    eh_task_create("task_app", 0, 12*1024, "task_app", task_app);

    eh_loop_run();
    eh_global_exit();
    return 0;
}

