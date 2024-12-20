/**
 * @file main.c
 * @brief  test 
 * @author simon.xiaoapeng (simon.xiaoapeng@gmail.com)
 * @date 2024-04-13
 * 
 * @copyright Copyright (c) 2024  simon.xiaoapeng@gmail.com
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <eh.h>
#include <eh_debug.h>
#include "eh_co.h"

context_t co_context_task1;
context_t co_context_task2;
context_t co_context_sys_main;

static void init(void){
}

void stdout_write(void *stream, const uint8_t *buf, size_t size){
    (void)stream;
    printf("%.*s", (int)size, (const char*)buf);
}

int test_co_task_2(void *arg){
    while(1){
        eh_debugfl("arg=%p", arg);
        sleep(1);
        co_context_swap((void*)0x1, &co_context_task2, &co_context_task1);

        eh_debugfl("arg=%p", arg);
        sleep(1);
        co_context_swap((void*)0x1, &co_context_task2, &co_context_task1);
    }
    return 0;
}

int test_co_task_1(void *arg){
    for(int i=0;i < 2;i++){
        eh_debugfl("arg=%p", arg);
        sleep(1);
        co_context_swap((void*)0x2, &co_context_task1, &co_context_task2);
        
        eh_debugfl("arg=%p", arg);
        sleep(1);
        co_context_swap((void*)0x2, &co_context_task1, &co_context_task2);

    }
    co_context_swap((void*)0x3, &co_context_task1, &co_context_sys_main);
    eh_debugfl("test_co_task_1 return 4");
    return 4;
}


static uint8_t g_stack_main_task1[1024*32];
static uint8_t g_stack_main_task2[1024*32];




int main(void)
{
    init();
    eh_debugln("test app start !");
    void *arg;
    co_context_task1 = co_context_make(g_stack_main_task1, (g_stack_main_task1 + sizeof(g_stack_main_task1)), test_co_task_1);
    co_context_task2 = co_context_make(g_stack_main_task2, (g_stack_main_task2 + sizeof(g_stack_main_task2)), test_co_task_2);

    eh_debugfl("co_context_task1 = %p", co_context_task1);

    arg = co_context_swap((void*)0x1, &co_context_sys_main, &co_context_task1);

    eh_debugfl("co_context test ok! arg = %p", arg);
    arg = co_context_swap((void*)0x1, &co_context_sys_main, &co_context_task1);

    return 0;
}
