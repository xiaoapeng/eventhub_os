/**
 * @file coroutine.c
 * @brief arm64架构协程相关代码
 * @author simon.xiaoapeng (simon.xiaoapeng@gmail.com)
 * @date 2024-05-01
 * 
 * @copyright Copyright (c) 2024  simon.xiaoapeng@gmail.com
 * 
 */

#include <eh_co.h>
#include "eh_config.h"

struct stack_init_context{
    unsigned long   d8;
    unsigned long   d9;
    unsigned long   d10;
    unsigned long   d11;
    unsigned long   d12;
    unsigned long   d13;
    unsigned long   d14;
    unsigned long   d15;
    unsigned long   x19;
    unsigned long   x20;
    unsigned long   x21;
    unsigned long   x22;
    unsigned long   x23;
    unsigned long   x24;
    unsigned long   x25;
    unsigned long   x26;
    unsigned long   x27;
    unsigned long   x28;
    unsigned long   fp;
    unsigned long   lr;
};

/*******************************************************
 *                                                     *
 *  -------------------------------------------------  *
 *  |  0  |  1  |  2  |  3  |  4  |  5  |  6  |  7  |  *
 *  -------------------------------------------------  *
 *  | 0x0 | 0x4 | 0x8 | 0xc | 0x10| 0x14| 0x18| 0x1c|  *
 *  -------------------------------------------------  *
 *  |    d8     |    d9     |    d10    |    d11    |  *
 *  -------------------------------------------------  *
 *  -------------------------------------------------  *
 *  |  8  |  9  |  10 |  11 |  12 |  13 |  14 |  15 |  *
 *  -------------------------------------------------  *
 *  | 0x20| 0x24| 0x28| 0x2c| 0x30| 0x34| 0x38| 0x3c|  *
 *  -------------------------------------------------  *
 *  |    d12    |    d13    |    d14    |    d15    |  *
 *  -------------------------------------------------  *
 *  -------------------------------------------------  *
 *  |  16 |  17 |  18 |  19 |  20 |  21 |  22 |  23 |  *
 *  -------------------------------------------------  *
 *  | 0x40| 0x44| 0x48| 0x4c| 0x50| 0x54| 0x58| 0x5c|  *
 *  -------------------------------------------------  *
 *  |    x19    |    x20    |    x21    |    x22    |  *
 *  -------------------------------------------------  *
 *  -------------------------------------------------  *
 *  |  24 |  25 |  26 |  27 |  28 |  29 |  30 |  31 |  *
 *  -------------------------------------------------  *
 *  | 0x60| 0x64| 0x68| 0x6c| 0x70| 0x74| 0x78| 0x7c|  *
 *  -------------------------------------------------  *
 *  |    x23    |    x24    |    x25    |    x26    |  *
 *  -------------------------------------------------  *
 *  -------------------------------------------------  *
 *  |  32 |  33 |  34 |  35 |  36 |  37 |  38 |  39 |  *
 *  -------------------------------------------------  *
 *  | 0x80| 0x84| 0x88| 0x8c| 0x90| 0x94| 0x98| 0x9c|  *
 *  -------------------------------------------------  *
 *  |    x27    |    x28    |    FP     |     LR    |  *
 *  -------------------------------------------------  *
 *                                                     *
 *******************************************************/

__attribute__((naked))  void * co_context_swap(
    __attribute__((unused)) void *arg, 
    __attribute__((unused)) context_t *from, 
    __attribute__((unused)) const context_t * const to){
    /*
     * x0: arg
     * x1: from
     * x2: to
     */

    __asm__ volatile(
        // 栈上分配空间保存现场
        "sub  sp, sp, #0xa0 \n"

        // 保存 d8 - d15
        "stp  d8,  d9,  [sp, #0x00] \n"
        "stp  d10, d11, [sp, #0x10] \n"
        "stp  d12, d13, [sp, #0x20] \n"
        "stp  d14, d15, [sp, #0x30] \n"

        // 保存 x19 - x30
        "stp  x19, x20, [sp, #0x40] \n"
        "stp  x21, x22, [sp, #0x50] \n"
        "stp  x23, x24, [sp, #0x60] \n"
        "stp  x25, x26, [sp, #0x70] \n"
        "stp  x27, x28, [sp, #0x80] \n"
        "stp  fp,  lr,  [sp, #0x90] \n"

        // 存储sp -> *x1
        "mov  x3, sp                \n"
        "str  x3, [x1, #0x00]       \n"
/* ---------------------------------------- restore context ---------------------------------------- */
        // 加载 *x2 -> sp
        "ldr  x3, [x2, #0x00]       \n"
        "mov  sp, x3                \n"

        // 恢复 d8 - d15
        "ldp  d8,  d9,  [sp, #0x00] \n"
        "ldp  d10, d11, [sp, #0x10] \n"
        "ldp  d12, d13, [sp, #0x20] \n"
        "ldp  d14, d15, [sp, #0x30] \n"

        // 恢复 x19 - x30
        "ldp  x19, x20, [sp, #0x40] \n"
        "ldp  x21, x22, [sp, #0x50] \n"
        "ldp  x23, x24, [sp, #0x60] \n"
        "ldp  x25, x26, [sp, #0x70] \n"
        "ldp  x27, x28, [sp, #0x80] \n"
        "ldp  fp,  lr,  [sp, #0x90] \n"

        // 恢复sp 
        "add  sp, sp, #0xa0         \n"

        // 跳转回PC
        "ret   lr \n"
    );
}

static __attribute__((naked)) void __start_task(void){
    __asm__ volatile(
        "   blr         x19                                 \n" /* x19存放着协程的入口函数 */
        "1: b           1b                                  \n"
        :::
    );
}

context_t co_context_make( 
    __attribute__((unused)) void *stack_lim,
    __attribute__((unused)) void *stack_top, 
    __attribute__((unused)) int (*func)(void *arg)){
    unsigned long ul_stack_top = (unsigned long)stack_top;
    struct stack_init_context  *context;
    ul_stack_top = ul_stack_top & (~0x0fUL);    /* 栈顶地址对齐到16字节 */
    context = (struct stack_init_context *)(ul_stack_top - sizeof(struct stack_init_context));
    context->x19 = (unsigned long)func;
    context->lr =  (unsigned long)__start_task;
    return (context_t)context;
}
