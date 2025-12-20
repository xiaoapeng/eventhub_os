/**
 * @file coroutine.c
 * @brief arm64架构协程相关代码
 * @author simon.xiaoapeng (simon.xiaoapeng@gmail.com)
 * @date 2024-05-01
 * 
 * @copyright Copyright (c) 2024  simon.xiaoapeng@gmail.com
 * 
 */


#include "eh_config.h"
#include "eh_co.h"


__attribute__((naked))  void * co_context_swap(
    __attribute__((unused)) void *arg, 
    __attribute__((unused)) context_t *from, 
    __attribute__((unused)) const context_t * const to){
    /*
     * rdi: arg
     * rsi: from
     * rdx: to
     */
//     __asm__ volatile(
//         "leaq  -0x38(%rsp), %rsp \n"                    /* 分配0x38字节存储上下文状态，不分配0x40是因为call指令已经自动存储了返回地址 */
// #if !defined(X86_X64_USE_TSX)
//         "stmxcsr  (%rsp) \n"                            /* save MMX control- and status-word */
//         "fnstcw   0x4(%rsp) \n"                         /* save x87 control-word */
// #endif
//         "movq  %r12, 0x8(%rsp)  \n"                     /* save R12 */
//         "movq  %r13, 0x10(%rsp) \n"                     /* save R13 */
//         "movq  %r14, 0x18(%rsp) \n"                     /* save R14 */
//         "movq  %r15, 0x20(%rsp) \n"                     /* save R15 */
//         "movq  %rbx, 0x28(%rsp) \n"                     /* save rbx */
//         "movq  %rbp, 0x30(%rsp) \n"                     /* save rbp */
        
//         "movq  %rsp, (%rsi) \n"                         /* 存储RSP -> *from */
        
//         "movq  (%rdx), %rsp \n"                         /* 加载*to -> RAX */
//         //"movq  %rax, %rsp \n"                         /* 加载RAX -> RSP */

//         "movq 0x38(%rsp), %r8 \n"                       /* 获取返回地址 */

// #if !defined(X86_X64_USE_TSX)
//         "ldmxcsr  (%rsp) \n"                            /* restore MMX control- and status-word */
//         "fldcw   0x4(%rsp) \n"                          /* restore x87 control-word */
// #endif
//         "movq  0x8(%rsp),  %r12 \n"                     /* restore R12 */
//         "movq  0x10(%rsp), %r13 \n"                     /* restore R13 */
//         "movq  0x18(%rsp), %r14 \n"                     /* restore R14 */
//         "movq  0x20(%rsp), %r15 \n"                     /* restore R15 */
//         "movq  0x28(%rsp), %rbx \n"                     /* restore RBX */
//         "movq  0x30(%rsp), %rbp \n"                     /* restore RBP */

//         "leaq  0x40(%rsp), %rsp \n"                     /* 恢复栈指针 */

//         "movq  %rdi, %rax \n"                           /* 将设置的arg进行return */

//         "jmp *%r8 \n"                                   /* 恢复到原现场 */
//     );
}



__attribute__((naked))  context_t co_context_make( 
    __attribute__((unused)) void *stack_lim,
    __attribute__((unused)) void *stack_top, 
    __attribute__((unused)) int (*func)(void *arg)){
    /*
     * rdi: stack_lim
     * rsi: stack_top
     * rdx: func
     */
    // __asm__ volatile(
    //     "movq  %rsi, %rax \n"                           /*  获得栈顶指针 */
    //     "andq  $-16, %rax \n"                           /*  设置为16字节对齐 */
    //     "leaq  -0x40(%rax), %rax \n"                    /*  制作context的空间 0x40个字节*/
    //     "stmxcsr  (%rax) \n"                            /* fc_mxcsr:  save MMX control- and status-word */
    //     "fnstcw   0x4(%rax) \n"                         /* fc_x87_cw: save x87 control-word */
    //     "leaq  trampoline(%rip), %rcx \n"               /*  获得trampoline的绝对地址 */
    //     "movq  %rcx, 0x38(%rax) \n"                     /* RIP:       将trampoline的地址放到RIP上 */
    //     "leaq  finish(%rip), %rcx \n"                   /*  获得finish的绝对地址 */
    //     "movq  %rcx, 0x30(%rax) \n"                     /* RBP:       将finish的地址放到RBP的位置 */
    //     "movq  %rdx, 0x28(%rax) \n"                     /* RBX:       将func地址放到RBX上 */
    //     "ret \n"
    // "trampoline: \n"
    //     "push %rbp \n"                                  /* 将RBP中存储的是finish的地址 */
    //     "jmp *%rbx \n"                                  /* 跳转到func */
    // "finish: \n"
    //     "movq  %rax, %rdi \n"                           /*  设置exit参数 */
    //     "call  _exit@PLT \n"                            /*  exit(0) */
    //     "hlt \n"
        
    // );
}
