/**
 * @file coroutine.c
 * @brief Windows x64 架构协程上下文切换
 * @author simon.xiaoapeng (simon.xiaoapeng@gmail.com)
 * @date 2026-05-04
 *
 * @copyright Copyright (c) 2026  simon.xiaoapeng@gmail.com
 *
 * Microsoft x64 calling convention: RCX, RDX, R8, R9
 * Callee-saved: RBX, RBP, RDI, RSI, R12-R15, XMM6-XMM15
 *
 ****************************************************************************************
 *                                                                                      *
 *  Context layout (0xF0 bytes total):                                                  *
 *                                                                                      *
 *  XMM6-XMM15 first for 16-byte alignment, then GPRs, then RIP                       *
 *                                                                                      *
 *  Offset  Content                                                                     *
 *  0x00    XMM6        (16 bytes)                                                      *
 *  0x10    XMM7        (16 bytes)                                                      *
 *  0x20    XMM8        (16 bytes)                                                      *
 *  0x30    XMM9        (16 bytes)                                                      *
 *  0x40    XMM10       (16 bytes)                                                      *
 *  0x50    XMM11       (16 bytes)                                                      *
 *  0x60    XMM12       (16 bytes)                                                      *
 *  0x70    XMM13       (16 bytes)                                                      *
 *  0x80    XMM14       (16 bytes)                                                      *
 *  0x90    XMM15       (16 bytes)                                                      *
 *  0xA0    fc_mxcsr    (4 bytes)                                                       *
 *  0xA4    fc_x87_cw   (4 bytes)                                                       *
 *  0xA8    R12         (8 bytes)                                                       *
 *  0xB0    R13         (8 bytes)                                                       *
 *  0xB8    R14         (8 bytes)                                                       *
 *  0xC0    R15         (8 bytes)                                                       *
 *  0xC8    RBX         (8 bytes)                                                       *
 *  0xD0    RBP         (8 bytes)                                                       *
 *  0xD8    RSI         (8 bytes)                                                       *
 *  0xE0    RDI         (8 bytes)                                                       *
 *  0xE8    RIP         (8 bytes)                                                       *
 *                                                                                      *
 ****************************************************************************************/

#include "eh_config.h"
#include "eh_co.h"


__attribute__((naked)) void * co_context_swap(
    __attribute__((unused)) void *arg,
    __attribute__((unused)) context_t *from,
    __attribute__((unused)) const context_t * const to){
    /*
     * RCX: arg
     * RDX: from
     * R8:  to
     */
    __asm__ volatile(
        "leaq  -0xE8(%rsp), %rsp \n"                    /* allocate context (0xF0 - 8 for return addr) */

        /* Save XMM6-XMM15 */
        "movaps  %xmm6,  0x00(%rsp) \n"
        "movaps  %xmm7,  0x10(%rsp) \n"
        "movaps  %xmm8,  0x20(%rsp) \n"
        "movaps  %xmm9,  0x30(%rsp) \n"
        "movaps  %xmm10, 0x40(%rsp) \n"
        "movaps  %xmm11, 0x50(%rsp) \n"
        "movaps  %xmm12, 0x60(%rsp) \n"
        "movaps  %xmm13, 0x70(%rsp) \n"
        "movaps  %xmm14, 0x80(%rsp) \n"
        "movaps  %xmm15, 0x90(%rsp) \n"

        /* Save x87/MMX state */
        "stmxcsr  0xA0(%rsp) \n"
        "fnstcw   0xA4(%rsp) \n"

        /* Save GPR callee-saves */
        "movq  %r12, 0xA8(%rsp) \n"
        "movq  %r13, 0xB0(%rsp) \n"
        "movq  %r14, 0xB8(%rsp) \n"
        "movq  %r15, 0xC0(%rsp) \n"
        "movq  %rbx, 0xC8(%rsp) \n"
        "movq  %rbp, 0xD0(%rsp) \n"
        "movq  %rsi, 0xD8(%rsp) \n"
        "movq  %rdi, 0xE0(%rsp) \n"

        /* Store RSP -> *from */
        "movq  %rsp, (%rdx) \n"

        /* Load *to -> RSP */
        "movq  (%r8), %rsp \n"

        /* Get return address */
        "movq  0xE8(%rsp), %r9 \n"

        /* Restore XMM6-XMM15 */
        "movaps  0x00(%rsp),  %xmm6 \n"
        "movaps  0x10(%rsp),  %xmm7 \n"
        "movaps  0x20(%rsp),  %xmm8 \n"
        "movaps  0x30(%rsp),  %xmm9 \n"
        "movaps  0x40(%rsp),  %xmm10 \n"
        "movaps  0x50(%rsp),  %xmm11 \n"
        "movaps  0x60(%rsp),  %xmm12 \n"
        "movaps  0x70(%rsp),  %xmm13 \n"
        "movaps  0x80(%rsp),  %xmm14 \n"
        "movaps  0x90(%rsp),  %xmm15 \n"

        /* Restore x87/MMX state */
        "ldmxcsr  0xA0(%rsp) \n"
        "fldcw   0xA4(%rsp) \n"

        /* Restore GPR callee-saves */
        "movq  0xA8(%rsp),  %r12 \n"
        "movq  0xB0(%rsp),  %r13 \n"
        "movq  0xB8(%rsp),  %r14 \n"
        "movq  0xC0(%rsp),  %r15 \n"
        "movq  0xC8(%rsp),  %rbx \n"
        "movq  0xD0(%rsp),  %rbp \n"
        "movq  0xD8(%rsp),  %rsi \n"
        "movq  0xE0(%rsp),  %rdi \n"

        /* Restore stack pointer */
        "leaq  0xF0(%rsp), %rsp \n"

        /* Return arg */
        "movq  %rcx, %rax \n"

        /* Jump to saved return address */
        "jmp *%r9 \n"
    );
}


__attribute__((naked)) context_t co_context_make(
    __attribute__((unused)) void *stack_lim,
    __attribute__((unused)) void *stack_top,
    __attribute__((unused)) int (*func)(void *arg)){
    /*
     * RCX: stack_lim
     * RDX: stack_top
     * R8:  func
     */
    __asm__ volatile(
        "movq  %rdx, %rax \n"                           /* stack_top */
        "andq  $-16, %rax \n"                           /* 16-byte align */
        "leaq  -0xF0(%rax), %rax \n"                    /* allocate full context */

        /* Zero out XMM area */
        "xorps %xmm0, %xmm0 \n"
        "movaps %xmm0, 0x00(%rax) \n"
        "movaps %xmm0, 0x10(%rax) \n"
        "movaps %xmm0, 0x20(%rax) \n"
        "movaps %xmm0, 0x30(%rax) \n"
        "movaps %xmm0, 0x40(%rax) \n"
        "movaps %xmm0, 0x50(%rax) \n"
        "movaps %xmm0, 0x60(%rax) \n"
        "movaps %xmm0, 0x70(%rax) \n"
        "movaps %xmm0, 0x80(%rax) \n"
        "movaps %xmm0, 0x90(%rax) \n"

        "stmxcsr  0xA0(%rax) \n"
        "fnstcw   0xA4(%rax) \n"

        "leaq  trampoline(%rip), %r9 \n"
        "movq  %r9, 0xE8(%rax) \n"                      /* RIP = trampoline */

        "leaq  finish(%rip), %r9 \n"
        "movq  %r9, 0xD0(%rax) \n"                      /* RBP = finish */

        "movq  %r8, 0xC8(%rax) \n"                      /* RBX = func */

        "ret \n"
    "trampoline: \n"
        "push %rbp \n"                                   /* RBP has finish address */
        "jmp *%rbx \n"                                   /* jump to func */
    "finish: \n"
        "movq  %rax, %rcx \n"                            /* first arg = exit code */
        "call  _exit \n"
        "ud2 \n"
    );
}