/**
 * @file context_convert.c
 * @brief 上下文转换，MSP <----> PSP互相转换
 * @author simon.xiaoapeng (simon.xiaoapeng@gmail.com)
 * @date 2024-09-16
 * 
 * @copyright Copyright (c) 2024  simon.xiaoapeng@gmail.com
 * 
 */

#include <eh.h>

#define SVC_CALL_CONTEXT_CONVERT_TO_PSP 0
#define SVC_CALL_CONTEXT_CONVERT_TO_MSP 1


static eh_aligned(8) uint8_t interrupt_stack[EH_CONFIG_INTERRUPT_STACK_SIZE];

/**
 *  将当前上下文转换为 PSP任务上下文
 */
__attribute__((naked))   void context_convert_to_psp(void){
    __asm__ volatile(
        "   .syntax unified                                             \n"
        "                                                               \n"
        "   cpsie       i                                               \n" /* Globally enable interrupts. */
        "   cpsie       f                                               \n"
        "   dsb                                                         \n"
        "   isb                                                         \n"
        "   svc         %0                                              \n" /* System call to start the first task. */
        "   bx          lr                                              \n"
        "   .align 4                                                    \n"
        ::"i" ( SVC_CALL_CONTEXT_CONVERT_TO_PSP ) : "memory"
    );
}

/**
 *  将当前上下文转换为 MSP任务上下文
 */
__attribute__((naked))   void context_convert_to_msp(void){
    __asm__ volatile(
        "   .syntax unified                                             \n"
        "                                                               \n"
        "   cpsie       i                                               \n" /* Globally enable interrupts. */
        "   cpsie       f                                               \n"
        "   dsb                                                         \n"
        "   isb                                                         \n"
        "   svc         %0                                              \n" /* System call to start the first task. */
        "   bx          lr                                              \n"
        "   .align 4                                                    \n"
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
        "   mrs         r1, psplim                                      \n"
        "   mrs         r2, msplim                                      \n"
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

