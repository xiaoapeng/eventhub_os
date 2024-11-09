/**
 * @file coroutine.c
 * @brief riscv32 coroutine 汇编代码，我们已知N200架构是rv32imac,
 *   所以去除了烦人的条件编译，一个架构一个汇编，当支持架构多了，再去优化
 *
 * @author simon.xiaoapeng (simon.xiaoapeng@gmail.com)
 * @date 2024-08-14
 * 
 * @copyright Copyright (c) 2024  simon.xiaoapeng@gmail.com
 * 
 */


#include "riscv_encoding.h"

#define REGBYTES        4


.global coroutine_asm_setup
.global co_context_swap

/**
 * @brife 保存用户态的上下文，这里的保存方式只适合没有任务切换的情况，rtos不能用这种方式
 * @param x 目标sp寄存器
 */
.macro SAVE_CONTEXT
    addi sp,  sp, -20*REGBYTES /* 分配 20*4字节的空间用来存储通用寄存器 + 状态寄存器 的上下文 */

    /* 存储通用寄存器 17个 */
    /* 
     * # 以下类型的寄存器无需保存：              共15个寄存器无需保存  32-15 = 17个需要保存
     *     Callee类型的寄存器：                 13个 
     *        根据C语言调用ABI规则 Callee类型的寄存器由被调用方保存，所以无需保存 
     *        x2(sp)  x8(s0/fp)  x9(s1/gp)  x18-x27(s2-s11)
     *     Zreo 类型： x0(zero)                 1个
     *     全局指针:   x3(gd)                   1个
     */
    sw   x1,  0*REGBYTES(sp)
    sw   x4,  1*REGBYTES(sp)
    sw   x5,  2*REGBYTES(sp)
    sw   x6,  3*REGBYTES(sp)
    sw   x7,  4*REGBYTES(sp)
    sw   x10, 5*REGBYTES(sp)
    sw   x11, 6*REGBYTES(sp)
    sw   x12, 7*REGBYTES(sp)
    sw   x13, 8*REGBYTES(sp)
    sw   x14, 9*REGBYTES(sp)
    sw   x15, 10*REGBYTES(sp)
    sw   x16, 11*REGBYTES(sp)
    sw   x17, 12*REGBYTES(sp)
    sw   x28, 13*REGBYTES(sp)
    sw   x29, 14*REGBYTES(sp)
    sw   x30, 15*REGBYTES(sp)
    sw   x31, 16*REGBYTES(sp)

    /* 存储状态寄存器 */
    // 实际上是使用cause作为操作数直接将其存储到sp的内存中 (这是N200的扩展实现)
    csrrwi  x0, CSR_PUSHMCAUSE, 17
    // 实际上是使用mepc作为操作数直接将其存储到sp的内存中 (这是N200的扩展实现)
    csrrwi  x0, CSR_PUSHMEPC, 18
    // 实际上是使用msubm作为操作数直接将其存储到sp的内存中 (这是N200的扩展实现)
    csrrwi  x0, CSR_PUSHMSUBM, 19
.endm

/**
 * @brife 恢复用户态的上下文
 * @param x 目标sp寄存器
 */
.macro RESTORE_CONTEXT
    
    /* 恢复状态寄存器 */
    lw   t0,  19*REGBYTES(sp)
    csrw CSR_MSUBM, t0
    lw   t0,  18*REGBYTES(sp)
    csrw CSR_MEPC, t0  
    lw   t0,  17*REGBYTES(sp)
    csrw CSR_MCAUSE, t0
    
    /* 恢复通用寄存器 */
    lw   x31, 16*REGBYTES(sp)
    lw   x30, 15*REGBYTES(sp)
    lw   x29, 14*REGBYTES(sp)
    lw   x28, 13*REGBYTES(sp)
    lw   x17, 12*REGBYTES(sp)
    lw   x16, 11*REGBYTES(sp)
    lw   x15, 10*REGBYTES(sp)
    lw   x14, 9*REGBYTES(sp)
    lw   x13, 8*REGBYTES(sp)
    lw   x12, 7*REGBYTES(sp)
    lw   x11, 6*REGBYTES(sp)
    lw   x10, 5*REGBYTES(sp)
    lw   x7,  4*REGBYTES(sp)
    lw   x6,  3*REGBYTES(sp)
    lw   x5,  2*REGBYTES(sp)
    lw   x4,  1*REGBYTES(sp)
    lw   x1,  0*REGBYTES(sp)
    
    addi sp,  sp, 20*REGBYTES
.endm


/**
 * @brife 陷阱门处理函数
 *      中断栈版本
 */
.section .text.coroutine_trap_entry
.align 6  // 64 byte alignment 
// 陷阱门处理函数
coroutine_trap_entry:
    /* 使用中断栈,查看手册 Nuclei_N200系列指令架构手册.pdf 7.4.24.*/
    csrrw sp, CSR_MSCRATCHCSWL, sp
    
    SAVE_CONTEXT
    
    /*
     * 先死循环后续再到C中进行错误处理 
     */
    // TODO
loop:
  	j loop

    RESTORE_CONTEXT
    
    /* 恢复用户栈 */
    csrrw sp, CSR_MSCRATCHCSWL, sp
    mret




/**
 * @brife 非向量模式下的中断处理入口
 *      中断栈版本
 */
.section .text.irq_entry
.align 2
// 中断门处理函数
coroutine_irq_entry:
    /* 使用中断栈,查看手册 Nuclei_N200系列指令架构手册.pdf 7.4.24.*/
    csrrw sp, CSR_MSCRATCHCSWL, sp
    
    
    SAVE_CONTEXT
    
    /*
     * 逐个处理非向量模式下的中断。
     * 为什么看起来这么怪？其实是为了实现咬尾 
     * 查看手册 Nuclei_N200系列指令架构手册.pdf 5.13.1.3
     */
int_loop:
  	csrrw ra, CSR_JALMNXTI, ra
    
    /* 处理完中断后会自动打开中断，所以这里关闭中断，然后再恢复寄存器 */
    csrc CSR_MSTATUS, MSTATUS_MIE

    RESTORE_CONTEXT
    
    /* 恢复用户栈 */
    csrrw sp, CSR_MSCRATCHCSWL, sp
    mret




.section .text.co_context_swap
.align 2
co_context_swap:
    /*
     * a0 = arg
     * a1 = from
     * a2 = to
     */
    
    addi sp, sp, -13*REGBYTES
    
    # 保存 s0-s11、ra 寄存器
    sw  s0,  0*REGBYTES(sp)
    sw  s1,  1*REGBYTES(sp)
    sw  s2,  2*REGBYTES(sp)
    sw  s3,  3*REGBYTES(sp)
    sw  s4,  4*REGBYTES(sp)
    sw  s5,  5*REGBYTES(sp)
    sw  s6,  6*REGBYTES(sp)
    sw  s7,  7*REGBYTES(sp)
    sw  s8,  8*REGBYTES(sp)
    sw  s9,  9*REGBYTES(sp)
    sw  s10, 10*REGBYTES(sp)
    sw  s11, 11*REGBYTES(sp)
    sw  ra,  12*REGBYTES(sp)
    
    sw  sp,  0*REGBYTES(a1)
    
    #  ------------------- 恢复现场 -------------------- #
    
    lw  sp,  0*REGBYTES(a2)
    
    # 恢复 s0-s11、ra 寄存器
    lw  ra,  12*REGBYTES(sp)
    lw  s11, 11*REGBYTES(sp)
    lw  s10, 10*REGBYTES(sp)
    lw  s9,  9*REGBYTES(sp)
    lw  s8,  8*REGBYTES(sp)
    lw  s7,  7*REGBYTES(sp)
    lw  s6,  6*REGBYTES(sp)
    lw  s5,  5*REGBYTES(sp)
    lw  s4,  4*REGBYTES(sp)
    lw  s3,  3*REGBYTES(sp)
    lw  s2,  2*REGBYTES(sp)
    lw  s1,  1*REGBYTES(sp)
    lw  s0,  0*REGBYTES(sp)
    
    addi sp, sp, 13*REGBYTES
    
    ret
    


/*
 * @brife 重新设置陷阱门和中断门处理函数
 *  设置中断栈
 */
.section .text.coroutine_asm_setup
.align 2
coroutine_asm_setup:
    csrc CSR_MSTATUS, MSTATUS_MIE       // 关闭中断
    csrw CSR_MSCRATCH, a0               // 设置中断栈，由C语言传参进来 用于在执行 csrrw sp, CSR_MSCRATCHCSWL, sp 时自动和该寄存器进行交换

    /* 重新非向量中断处理入口 */
    la t0, coroutine_irq_entry
    csrw CSR_MTVT2, t0
    csrs CSR_MTVT2, 0x1

    /* 重新设置陷阱处理入口 */
    la t0, coroutine_trap_entry
    csrw CSR_MTVEC, t0
    csrs CSR_MTVEC, 0x3                 // 配置到 ECLIC 中断模式

    csrs CSR_MSTATUS, MSTATUS_MIE       // 开启中断
    ret
