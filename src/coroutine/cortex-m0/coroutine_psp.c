/**
 * @file coroutine.c
 * @brief arm m0 架构协程相关代码,PSP模式下，非Pendsv中断方式切换任务
 *        比PendSV方式更加简洁，高效(无浮点现场的情况下)
 *        后面默认使用此方案
 *
 * @author simon.xiaoapeng (simon.xiaoapeng@gmail.com)
 * @date 2024-08-17
 * 
 * @copyright Copyright (c) 2024  simon.xiaoapeng@gmail.com
 * 
 */



#include <stdio.h>
#include <strings.h>
#include <eh.h>
#include "eh_co.h"
#include "eh_config.h"

#define SHPR2_REGISTER_ADDRESS (0xE000ED1CUL)
#define SHPR3_REGISTER_ADDRESS (0xE000ED20UL)



#ifndef __FPU_USED__
#define __FPU_USED__ 0
#endif


struct stack_init_context{
    unsigned long   r12;

    unsigned long   r8;
    unsigned long   r9;
    unsigned long   r10;
    unsigned long   r11;

    unsigned long   r4;
    unsigned long   r5;
    unsigned long   r6;
    unsigned long   r7;
    unsigned long   lr;
};


__attribute__((naked)) void * co_context_swap(
    __attribute__((unused)) void *arg, 
    __attribute__((unused)) context_t *from, 
    __attribute__((unused)) const context_t * const to){
    /*
     * r0: arg
     * r1: from
     * r2: to
     */
    __asm__ volatile(
        "   .syntax unified                                     \n"
        "   push        {r4-r7,lr}                              \n" /* 保存现场 {r12, r8-r11, r4-r7,lr} */
        "   mov         r4, r8                                  \n"
        "   mov         r5, r9                                  \n"
        "   mov         r6, r10                                 \n"
        "   mov         r7, r11                                 \n"
        "   push        {r4-r7}                                 \n"
        "   mov         r4, r12                                 \n"
        "   push        {r4}                                    \n"
        
        "   mov         r3, sp                                  \n"
        "   str         r3, [r1]                                \n"/* 保存当前栈指针到from */
/* ---------------------------------------- restore context ---------------------------------------- */
        "   ldr         r3, [r2]                                \n"/* 恢复栈指针 */
        "   mov         sp, r3                                  \n"

        "   pop         {r4}                                    \n" /* 恢复现场 {r12, r8-r11, r4-r7,lr} */
        "   mov         r12, r4                                 \n"
        "   pop         {r4-r7}                                 \n"
        "   mov         r8, r4                                  \n"
        "   mov         r9, r5                                  \n"
        "   mov         r10, r6                                 \n"
        "   mov         r11, r7                                 \n"
        "   pop         {r4-r7, pc}                             \n"
        ::: "memory"
    );
}

static __attribute__((naked)) void __start_task(void){
    __asm__ volatile(
        "   .syntax unified                                     \n"
        "   blx         r7                                      \n"/* r7存放着协程的入口函数 */
        "1: b           1b                                      \n"/* 若返回，则进入死循环 */
        :::
    );
}


context_t co_context_make( 
    __attribute__((unused)) void *stack_lim,
    __attribute__((unused)) void *stack_top, 
    __attribute__((unused)) int (*func)(void *arg)){
    uint32_t u32_stack_top = (uint32_t)(stack_top);
    struct stack_init_context *context_m0;
    u32_stack_top = u32_stack_top & (~7UL);               /* 向8对齐 */
    context_m0 = (struct stack_init_context *)(u32_stack_top - sizeof(struct stack_init_context));
    bzero(context_m0, sizeof(struct stack_init_context));
    context_m0->lr = (uint32_t)__start_task;
    context_m0->r7 = (uint32_t)func;
    return (context_t)context_m0;
}

extern void context_convert_to_psp(void);
extern void context_convert_to_msp(void);

static int  __init coroutine_init(void){
    /* 设置SVC优先级  Cortex-M0 Generic UG (r0p0, Issue A).pdf  2.3.1*/
    /* SVC 优先级设置为0 */
    *((volatile uint32_t*)SHPR2_REGISTER_ADDRESS) =  ((*((volatile uint32_t*)SHPR2_REGISTER_ADDRESS)) & (~0xFF000000U)) | ((0x00U) << 24);

    /* 触发svc中断，将当前的上下文任务化，MSP -> PSP，然后使用新MSP准备栈空间 */
    context_convert_to_psp();
    return 0;
}

static void __exit coroutine_deinit(void){
    context_convert_to_msp();
}

eh_core_module_export(coroutine_init, coroutine_deinit);