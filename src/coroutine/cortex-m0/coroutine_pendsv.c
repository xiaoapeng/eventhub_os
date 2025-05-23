/**
 * @file coroutine.c
 * @brief arm m0 架构协程相关代码,PSP模式下PendSV的协程切换代码，
 *
 * @author simon.xiaoapeng (simon.xiaoapeng@gmail.com)
 * @date 2024-08-10
 * 
 * @copyright Copyright (c) 2024  simon.xiaoapeng@gmail.com
 * 
 */



#include <stdio.h>
#include <strings.h>
#include <eh.h>
#include "eh_co.h"
#include "eh_config.h"


#define ICSR_REGISTER_ADDRESS  (0xE000ED04UL)
#define SHPR2_REGISTER_ADDRESS (0xE000ED1CUL)
#define SHPR3_REGISTER_ADDRESS (0xE000ED20UL)


#define ICSR_PENDSVSET_BIT    ( 1UL << 28UL)


#ifndef __FPU_USED__
#define __FPU_USED__ 0
#endif

struct stack_auto_push_context{
    unsigned long   r0;
    unsigned long   r1;
    unsigned long   r2;
    unsigned long   r3;
    unsigned long   r12;
    unsigned long   lr_r14;
    unsigned long   return_address;
    unsigned long   xpsr;
};

struct stack_init_context{
    /* M0内核没有FPU, 无需存储LR */
    //unsigned long   exc_return_lr;
    unsigned long   r4;
    unsigned long   r5;
    unsigned long   r6;
    unsigned long   r7;
    unsigned long   r8;
    unsigned long   r9;
    unsigned long   r10;
    unsigned long   r11;
    struct stack_auto_push_context auto_push_context;
};



__attribute__((naked)) void PendSV_Handler( void ){
    /*
     *       arg  from  to
     *      | R0 | R1 | R2 | R3 | R12 | LR | Return Address | XPSR |
     * PSP-----^
     *  r1-----^
     */

    __asm__ volatile(
        "   .syntax unified                                         \n"
        "                                                           \n"
        "   mrs         r0, psp                                     \n"
        "   mov         r1, r0                                      \n" /* r1作为栈帧寄存器稍后访问 arg from to*/

        "   ldr         r2, [r1, #0x04]                             \n"/* 读取 from */

        "   subs        r0, r0, #32                                 \n" /* Make space for the remaining low registers. */
        "   str         r0, [r2]                                    \n"/* 保存现场到from */


        "   stmia       r0!, {r4-r7}                                \n" /* 保存 {r4 - r11} */
        "   mov         r4, r8                                      \n" 
        "   mov         r5, r9                                      \n"
        "   mov         r6, r10                                     \n"
        "   mov         r7, r11                                     \n"
        "   stmia       r0!, {r4-r7}                                \n"

        "   ldr         r0, [r1, #0x08]                             \n"/* 读取 to */
        "   ldr         r1, [r1, #0x00]                             \n"/* 读取 arg */
    /* ------------------------------------------------------- restore context ------------------------------------------------------- */
        "   ldr         r0, [r0, #0x00]                             \n"/* to中读取被恢复任务的psp */

        "   adds        r0, r0, #16                                 \n"
        "   ldmia       r0!, {r4-r7}                                \n"/* 恢复 {r4 - r11} */
        "   mov         r8, r4                                      \n"
        "   mov         r9, r5                                      \n"
        "   mov         r10, r6                                     \n"
        "   mov         r11, r7                                     \n"
        
        "   str         r1, [r0, #0x00]                             \n"/* 设置 arg */
        "   msr         psp, r0                                     \n"/* Remember the new top of stack for the task. */

        "   subs        r0, r0, #32                                 \n"
        "   ldmia       r0!, {r4-r7}                                \n"
    
        "   bx          lr                                          \n"
    );
}



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
        "   .syntax unified                                 \n"
        "   push   {r4,r5}                                  \n"
        "   ldr    r4, =%[val]                              \n"
        "   ldr    r5, =%[addr]                             \n"
        "   str    r4, [r5]                                 \n" /* 触发PENDSV */
        "   dsb                                             \n"
        "   isb                                             \n"
        "   pop    {r4,r5}                                  \n"
        "   bx lr                                           \n"
        "                                                   \n"
        "   .align 4                                        \n"
        : 
        : [addr] "i" (ICSR_REGISTER_ADDRESS), [val] "i" (ICSR_PENDSVSET_BIT)
        : "memory"
    );
}

static __attribute__((naked)) void __start_task(void){
    __asm__ volatile(
        "   .syntax unified                                 \n"
        "   blx         r7                                  \n"/* r7存放着协程的入口函数 */
        "1: b           1b                                  \n"/* 若返回，则进入死循环 */
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
    context_m0->auto_push_context.return_address = (uint32_t)(__start_task);
    context_m0->auto_push_context.xpsr = 0x01000000;
    context_m0->r7 = (uint32_t)func;
    return (context_t)context_m0;
}

extern void context_convert_to_psp(void);
extern void context_convert_to_msp(void);

static int  __init coroutine_init(void){
    /* 设置SVC/PendCV优先级  Cortex-M0 Generic UG (r0p0, Issue A).pdf  2.3.1*/
    /* SVC 优先级设置为0 */
    *((volatile uint32_t*)SHPR2_REGISTER_ADDRESS) =  ((*((volatile uint32_t*)SHPR2_REGISTER_ADDRESS)) & (~0xFF000000U)) | ((0x00U) << 24);
    /* PendSV 优先级设置为0xFF */
    *((volatile uint32_t*)SHPR3_REGISTER_ADDRESS) =  ((*((volatile uint32_t*)SHPR3_REGISTER_ADDRESS)) & (~0x00FF0000U)) | ((0xFFU) << 16);


    /* 触发svc中断，将当前的上下文任务化，MSP -> PSP，然后使用新MSP准备栈空间 */
    context_convert_to_psp();
    return 0;
}

static void __exit coroutine_deinit(void){
    context_convert_to_msp();
}

eh_core_module_export(coroutine_init, coroutine_deinit);