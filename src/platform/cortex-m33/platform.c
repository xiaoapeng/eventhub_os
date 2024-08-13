
#include <stdio.h>
#include <stdlib.h>
#include "eh.h"
#include "eh_debug.h"
#include "eh_event.h"
#include "eh_timer.h"
#include "eh_platform.h"
#include "core_cm33.h"



#define SVC_CALL_CONTEXT_CONVERT_TO_PSP 0
#define SVC_CALL_CONTEXT_CONVERT_TO_MSP 1


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

struct stack_state_context{
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

struct stack_state_float_context{
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
    unsigned long   s12;
    unsigned long   s13;
    unsigned long   s14;
    unsigned long   s15;
    unsigned long   fpscr;
};



static __attribute__((aligned(8))) uint8_t interrupt_stack[EH_CONFIG_INTERRUPT_STACK_SIZE];

/**
 *  将当前上下文转换为 PSP任务上下文
 */
__attribute__((naked))   void context_convert_to_psp(void){
    __asm__ volatile(
        "    .syntax unified                                    \n"
        "                                                       \n"
        "    cpsie       i                                      \n" /* Globally enable interrupts. */
        "    cpsie       f                                      \n"
        "    dsb                                                \n"
        "    isb                                                \n"
        "    svc         %0                                     \n" /* System call to start the first task. */
        "    bx          lr                                     \n"
        "   .align 4                                            \n"
        ::"i" ( SVC_CALL_CONTEXT_CONVERT_TO_PSP ) : "memory"
    );
}

/**
 *  将当前上下文转换为 MSP任务上下文
 */
__attribute__((naked))   void context_convert_to_msp(void){
    __asm__ volatile(
        "    .syntax unified                                    \n"
        "                                                       \n"
        "    cpsie       i                                      \n" /* Globally enable interrupts. */
        "    cpsie       f                                      \n"
        "    dsb                                                \n"
        "    isb                                                \n"
        "    svc         %0                                     \n" /* System call to start the first task. */
        "    bx          lr                                     \n"
        "   .align 4                                            \n"
        ::"i" ( SVC_CALL_CONTEXT_CONVERT_TO_MSP ) : "memory"
    );
}

void svc_handler_c(unsigned long *sp, uint8_t svc_number){
    struct stack_auto_push_context *context = (struct stack_auto_push_context *)sp;
    /* 目前无任何实现 */
    (void) context;
    (void) svc_number;
}

void context_convert(unsigned long *sp, uint8_t svc_number){
    /* sp-> svc_num psplim msplim psp msp lr */
    struct fsp{
        unsigned long svc_num;
        unsigned long psplim;
        unsigned long msplim;
        unsigned long psp;
        unsigned long msp;
        unsigned long lr;
    } *fsp = (void*)sp;
    switch (svc_number) {
        case SVC_CALL_CONTEXT_CONVERT_TO_PSP:
            fsp->lr |= (1UL<<2);
            fsp->psplim = fsp->msplim;
            fsp->psp = fsp->msp;
            fsp->msp = (unsigned long)(interrupt_stack + sizeof(interrupt_stack));
            fsp->msplim = (unsigned long)(interrupt_stack);
            break;
        case SVC_CALL_CONTEXT_CONVERT_TO_MSP:
            fsp->lr &= ~(1UL<<2);
            fsp->msp = fsp->psp;
            fsp->msplim = fsp->psplim;
            break;
    }
}

__attribute__((naked))  void  SVC_Handler( void )
{
    __asm__ volatile(
        "    .syntax unified                                            \n"
        "                                                               \n"
        "   tst         lr, #4                                          \n"
        "   ite         eq                                              \n"
        "   mrseq       r0, msp                                         \n"
        "   mrsne       r0, psp                                         \n"
        /* r0[6*4] -> retaddr  retaddr[-2]  获取svc服务编号              */
        "   ldr         r1, [r0, #24]                                   \n"
        "   ldrb        r1, [r1, #-2]                                   \n"
        "   cmp         r1, #1                                          \n"
        "   bgt         1f                                              \n"
        /*
         * 小于等于1,说明要进行堆栈转换的系统调用                 
         *  
         */
        "   mrs         r0, msp                                         \n"
        "   push        {r0,lr}                                         \n"/* msp lr */
        "   mov         r0, r1                                          \n"
        "	mrs         r1, psplim								        \n"
        "	mrs         r2, msplim								        \n"
        "   mrs         r3, psp                                         \n"
        "   push        {r0-r3}                                         \n"/* svc_num psplim msplim psp */
        "   mov         r1,r0                                           \n"
        "   mov         r0,sp                                           \n"/* 当前栈作为第一个参数给 context_convert_c_address_const */
        "   ldr         r2, context_convert_c_address_const             \n"
        "   blx         r2                                              \n"
        "   pop         {r0-r3}                                         \n"/* svc_num psplim msplim psp */
        "   msr         psplim, r1                                      \n"
        "   pop         {r0-r1}                                         \n"/* msp lr */
        "   msr         msp, r0                                         \n"
        "   msr         msplim, r2                                      \n"
        "   msr         psp, r3                                         \n"
        "   bx          r1                                              \n" /* 返回 */
        "1:                                                             \n"
        "   ldr         r3, svc_handler_c_address_const                 \n"
        "   bx          r3                                              \n"
        "                                                               \n"
        "    .align 4                                                   \n"
        "svc_handler_c_address_const: .word svc_handler_c               \n"
        "context_convert_c_address_const: .word context_convert         \n"
    );
}



__attribute__((naked))  eh_save_state_t  platform_enter_critical(void){
    __asm__ volatile(
        "    .syntax unified                                            \n"
        "                                                               \n"
        "    mrs r0, primask                                            \n"/* 保存中断状态在R0中 */
        "    cpsid   i                                                  \n"/* 失能中断 */
        "    dsb                                                        \n"
        "    isb                                                        \n"
        "    bx lr                                                      \n"/* Return. */
        ::: "r0", "memory"
    );
}

__attribute__((naked)) void  platform_exit_critical( 
        __attribute__((unused)) eh_save_state_t state) {
    __asm__ volatile(
        "    .syntax unified                                            \n"
        "                                                               \n"
        "    msr primask, r0                                            \n"/* 恢复中断状态 */
        "    dsb                                                        \n"
        "    isb                                                        \n"
        "    bx lr                                                      \n"/* Return. */
        ::: "memory"
    );
}

__attribute__((naked)) void HardFault_Handler(void){
    __asm__ volatile (
        "    .syntax unified                                            \n"
        "    tst         lr, #4                                         \n"
        "    ite         eq                                             \n"
        "    mrseq       r0, msp                                        \n"
        "    mrsne       r0, psp                                        \n"
        "	 stmdb       r0!, {r4-r11}								    \n"
        "    mov         r1, lr                                         \n"/*   将 lr 的值移动到 r1*/
        "    mrs         r2, control                                    \n"/*   将 control 寄存器的值移动到 r2*/
        "   ldr          r3, hardfault_handler_c_address_const          \n"
        "   bx           r3                                             \n"
        "                                                               \n"
        "    .align 4                                                   \n"
        "hardfault_handler_c_address_const: .word hardfault_handler_c   \n"
        ::: "memory"
    );
}


void hardfault_handler_c(unsigned long sp, unsigned long lr , unsigned long control ){
    eh_task_sta_t                         sta;
    struct stack_state_context*           stack_state = (struct stack_state_context*)sp;
    //struct StackStateFloatContext*      fpu_stack_state = (struct StackStateFloatContext*)(stack_state+1);
    eh_errln("");
    eh_errln("");
    eh_errln("######### hardfault_handler #########");
    eh_errln("SP:0x%08lx", sp);
    /* LR寄存器意义 DDI0553B_o_armv8m_arm.pdf 章节：D1.2.95*/
    eh_errln("LR:0x%08lx", lr);
    eh_errln("hardfault_handler  CONTROL:0x%08lx", control);
    eh_errln("FPU->FPCCR:%p", FPU->FPCCR);


    /* 关于发生异常时，上下文堆栈情况  DDI0553B_o_armv8m_arm.pdf 章节：  */
    eh_errln("dump sp:");
    eh_errln("r0:0x%08lx", stack_state->auto_push_context.r0);
    eh_errln("r1:0x%08lx", stack_state->auto_push_context.r1);
    eh_errln("r2:0x%08lx", stack_state->auto_push_context.r2);
    eh_errln("r3:0x%08lx", stack_state->auto_push_context.r3);
    eh_errln("r4:0x%08lx", stack_state->r4);
    eh_errln("r5:0x%08lx", stack_state->r5);
    eh_errln("r6:0x%08lx", stack_state->r6);
    eh_errln("r7:0x%08lx", stack_state->r7);
    eh_errln("r8:0x%08lx", stack_state->r8);
    eh_errln("r9:0x%08lx", stack_state->r9);
    eh_errln("r10:0x%08lx", stack_state->r10);
    eh_errln("r11:0x%08lx", stack_state->r11);
    eh_errln("r12:0x%08lx", stack_state->auto_push_context.r12);
    eh_errln("lr:0x%08lx", stack_state->auto_push_context.lr_r14);
    eh_errln("return_address:0x%08lx", stack_state->auto_push_context.return_address);
    eh_errln("xpsr:0x%08lx", stack_state->auto_push_context.xpsr);

    eh_task_sta(eh_task_self(), &sta);
    eh_errln("### eventhub os sta: ###");
    eh_errln("task:%s", sta.task_name);
    eh_errln("stack:0x%p", sta.stack);
    eh_errln("stack_size:%lu", sta.stack_size);
    eh_errln("stack_min_ever_free_size_level:%lu", sta.stack_min_ever_free_size_level);

    while(1){   }
}


static int  __init generic_platform_init(void){
    return 0;
}

static void __exit generic_platform_deinit(void){
}

eh_core_module_export(generic_platform_init, generic_platform_deinit);