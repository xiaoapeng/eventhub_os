/**
 * @file coroutine.c
 * @brief x64架构协程相关代码
 * @author simon.xiaoapeng (simon.xiaoapeng@gmail.com)
 * @version 1.0
 * @date 2024-05-01
 * 
 * @copyright Copyright (c) 2024  simon.xiaoapeng@gmail.com
 * 
 * @par 修改日志:
 */



#include <stdio.h>
#include <strings.h>
#include "eh_config.h"
#include "eh_co.h"

#define __FPU_USED_     1

struct context_m33{
#if (__FPU_USED_ == 1)
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
}__attribute__((aligned(sizeof(long))));





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
        "	.syntax unified									\n"
        "   mrs         r3, msplim                          \n"/* 获取psplim寄存器值到r3 */
        "   push        {r3-r12,lr}                         \n"/* 保存r3-r12和lr寄存器的值 */
#if  (__FPU_USED_ == 1)
        "   vpush       {s0-s31}                            \n"/* 保存浮点寄存器的值 */
        "   vmrs        r3, fpscr                           \n"
        "   push        {r3}                                \n"
#endif
        "   str         sp, [r1]                            \n"/* 保存当前栈指针到from */
/* ---------------------------------------- restore context ---------------------------------------- */
        "   ldr         sp, [r2]                            \n"/* 恢复栈指针 */
        
        "   sub         r3, sp, #0x100                      \n"
        "   msr         msplim, r3                          \n"/* 临时的psplim寄存器值 */
        
#if  (__FPU_USED_ == 1)
        "   pop         {r3}                                \n"/* 恢复浮点寄存器的值 */
        "   vmsr        fpscr, r3                           \n"
        "   vpop        {s0-s31}                            \n"
#endif
        "   pop         {r3-r12,lr}                         \n"/* 恢复r3-r12和lr寄存器的值 */
        "   msr         msplim, r3                          \n"/* 从r3恢复psplim寄存器值 */
        "	bx lr											\n"/* Return. */
        ::: "memory"
    );
}

static void __exit(void){
    while(1){ }
}


static __attribute__((naked)) void __start_task(void){
    __asm__ volatile(
        "	.syntax unified									\n"
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
