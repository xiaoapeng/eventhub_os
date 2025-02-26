/**
 * @file coroutine.c
 * @brief arm m33 架构协程相关代码,MSP模式下的协程切换代码，
 *    此代码准备遗弃，与pendsv悬挂中断相比，有如下缺点（但毕竟费了一番心血，不进行删除），
 *  1. 任务使用MSP栈空间，而中断也使用MSP栈空间，每个任务都要给中断预留栈，会导致栈空间浪费严重。
 *  2. 在使用FPU时，切换协程需要保存浮点寄存器，无法利用 Lazy FP技术 减少开销。
 *    优点：
 *  1. 代码简单很多。
 *  2. 协程切换时间确定，每次切换时长基本一致。
 *
 * @author simon.xiaoapeng (simon.xiaoapeng@gmail.com)
 * @date 2024-06-29
 * 
 * @copyright Copyright (c) 2024  simon.xiaoapeng@gmail.com
 * 
 */



#include <stdio.h>
#include <strings.h>
#include "eh_config.h"
#include "eh_co.h"


#ifndef __FPU_USED__
#define __FPU_USED__ 0
#endif

#ifndef CONTEXT_SWAP_DISABLEIRQ
#if (__FPU_USED__ == 1)

/*
 *  当没有协处理器时，没有必要禁止中断，
 *  因为节省不到栈空间
 */

/* 
 *  CONTEXT_SWAP_DISABLEIRQ 为1时
 *  在协程交换中会禁止中断，禁止中断会多几条汇编指令，
 *  会降低性能，但是可以节省几十到上百字节的堆栈空间，
 *  因为保存现场需要占用栈，中断后保存也需要栈，
 *  会导致重复保存现场
 */
#define CONTEXT_SWAP_DISABLEIRQ 1
#else
#define CONTEXT_SWAP_DISABLEIRQ 0
#endif /* __FPU_USED__ */

#endif /* CONTEXT_SWAP_DISABLEIRQ */


struct context_m33{
#if (__FPU_USED__ == 1)
    unsigned long       fpscr;
    unsigned long       s0;
    unsigned long       s1;
    unsigned long       s2;
    unsigned long       s3;
    unsigned long       s4;
    unsigned long       s5;
    unsigned long       s6;
    unsigned long       s7;
    unsigned long       s8;
    unsigned long       s9;
    unsigned long       s10;
    unsigned long       s11;
    unsigned long       s12;
    unsigned long       s13;
    unsigned long       s14;
    unsigned long       s15;
    unsigned long       s16;
    unsigned long       s17;
    unsigned long       s18;
    unsigned long       s19;
    unsigned long       s20;
    unsigned long       s21;
    unsigned long       s22;
    unsigned long       s23;
    unsigned long       s24;
    unsigned long       s25;
    unsigned long       s26;
    unsigned long       s27;
    unsigned long       s28;
    unsigned long       s29;
    unsigned long       s30;
    unsigned long       s31;
#endif
    unsigned long       psplim;
    unsigned long       r4;
    unsigned long       r5;
    unsigned long       r6;
    unsigned long       r7;
    unsigned long       r8;
    unsigned long       r9;
    unsigned long       r10;
    unsigned long       r11;
    unsigned long       r12;
    unsigned long       lr;
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
        "   .syntax unified                                 \n"
        "   mrs         r3, msplim                          \n"/* 获取psplim寄存器值到r3 */
        "   push        {r3-r12,lr}                         \n"/* 保存r3-r12和lr寄存器的值 */

        "   mov         r3, #0                              \n"
        "   msr         msplim, r3                          \n"/* 将msplim设置为0相当于禁用msplim */

#if     CONTEXT_SWAP_DISABLEIRQ == 1
        "   mrs r4, primask                                 \n"/* 保存中断状态在r4中 */
        "   cpsid   i                                       \n"/* 失能中断 */
        "   dsb                                             \n"
        "   isb                                             \n"
#endif

#if  (__FPU_USED__ == 1)
        "   vpush       {s0-s31}                            \n"/* 保存浮点寄存器的值 */
        "   vmrs        r3, fpscr                           \n"
        "   push        {r3}                                \n"
#endif
        "   str         sp, [r1]                            \n"/* 保存当前栈指针到from */
/* ---------------------------------------- restore context ---------------------------------------- */
        "   ldr         sp, [r2]                            \n"/* 恢复栈指针 */
        
        
#if  (__FPU_USED__ == 1)
        "   pop         {r3}                                \n"/* 恢复浮点寄存器的值 */
        "   vmsr        fpscr, r3                           \n"
        "   vpop        {s0-s31}                            \n"
#endif

#if     CONTEXT_SWAP_DISABLEIRQ == 1
        "   msr primask, r4                                 \n"/* 恢复中断状态 */
        "   dsb                                             \n"
        "   isb                                             \n"
#endif

        "   pop         {r3-r12,lr}                         \n"/* 恢复r3-r12和lr寄存器的值 */
        "   msr         msplim, r3                          \n"/* 从r3恢复psplim寄存器值 */
        "   bx lr                                           \n"/* Return. */
        ::: "memory"
    );
}

static void __exit(void){
    while(1){ }
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
    uint32_t u32_stack_lim = (uint32_t)(stack_lim);
    struct context_m33 *context_m33;
    u32_stack_top = u32_stack_top & (~7);               /* 向8对齐 */
    u32_stack_lim = (u32_stack_lim+7) & (~7);           /* 向8对齐 */
    context_m33 = (struct context_m33 *)(u32_stack_top - sizeof(struct context_m33));
    bzero(context_m33, sizeof(struct context_m33));
    context_m33->lr = (uint32_t)__start_task;
    context_m33->r7 = (uint32_t)func;
    context_m33->psplim = (uint32_t)u32_stack_lim;
    return (context_t)context_m33;
}
