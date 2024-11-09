/**
 * @file coroutine.c
 * @brief riscv32 coroutine
 *
 * @author simon.xiaoapeng (simon.xiaoapeng@gmail.com)
 * @date 2024-08-14
 * 
 * @copyright Copyright (c) 2024  simon.xiaoapeng@gmail.com
 * 
 */


#include <stdio.h>
#include <strings.h>
#include "eh.h"
#include "eh_co.h"
#include "eh_config.h"

void coroutine_asm_setup(void *interrupt_stack);

static eh_aligned(16) uint8_t interrupt_stack[EH_CONFIG_INTERRUPT_STACK_SIZE];

struct stack_init_context{
    unsigned long   s0;
    unsigned long   s1;
    unsigned long   s2;
    unsigned long   s3;
    unsigned long   s4;
    unsigned long   s5;
    unsigned long   s6;
    unsigned long   s7;
    unsigned long   s8;
    unsigned long   s9;
    unsigned long   s10;
    unsigned long   s11;
    unsigned long   ra;
};

static __attribute__((naked)) void __start_task(void){
    __asm__ volatile(
        "   jalr        s0                                  \n"/* s0存放着协程的入口函数 */
        "1: j           1b                                  \n"/* 若返回，则进入死循环 */
        :::
    );
}

context_t co_context_make( 
    __attribute__((unused)) void *_stack_lim,
    __attribute__((unused)) void *_stack_top, 
    __attribute__((unused)) int (*func)(void *arg)){
    unsigned long stack_top = (unsigned long)_stack_top;
    struct stack_init_context *ctx;
    stack_top = stack_top & (~0xfUL);               /* 向16对齐 */
    ctx = (struct stack_init_context *)(stack_top - sizeof(struct stack_init_context));
    bzero(ctx, sizeof(struct stack_init_context));
    ctx->ra = (unsigned long)__start_task;
    ctx->s0 = (unsigned long)func;
    return (context_t)ctx;
}






static int  __init coroutine_init(void){
    /* 设置中断栈, 安装陷阱门和中断门处理函数 */
    coroutine_asm_setup(((uint8_t*)interrupt_stack) + EH_CONFIG_INTERRUPT_STACK_SIZE);
    
    return 0;
}

static void __exit coroutine_deinit(void){

}

eh_core_module_export(coroutine_init, coroutine_deinit);