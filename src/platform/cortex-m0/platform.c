
#include <stdio.h>
#include <stdlib.h>
#include <eh.h>
#include <eh_debug.h>
#include <eh_event.h>
#include <eh_timer.h>
#include <eh_platform.h>


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





__attribute__((naked))  eh_save_state_t  platform_enter_critical(void){
    __asm__ volatile(
        "   .syntax unified                                             \n"
        "                                                               \n"
        "   mrs r0,     primask                                         \n"/* 保存中断状态在R0中 */
        "   cpsid       i                                               \n"/* 失能中断 */
        "   dsb                                                         \n"
        "   isb                                                         \n"
        "   bx          lr                                              \n"/* Return. */
        ::: "r0", "memory"
    );
}

__attribute__((naked)) void  platform_exit_critical( 
        __attribute__((unused)) eh_save_state_t state) {
    __asm__ volatile(
        "    .syntax unified                                            \n"
        "                                                               \n"
        "   msr         primask, r0                                     \n"/* 恢复中断状态 */
        "   dsb                                                         \n"
        "   isb                                                         \n"
        "   bx          lr                                              \n"/* Return. */
        ::: "memory"
    );
}

__attribute__((naked)) void HardFault_Handler(void){
    __asm__ volatile (
        "   .syntax unified                                             \n"

        "   movs        r1, #4                                          \n"  // 将立即数4加载到r1
        "   mov         r2, lr                                          \n"  // 将lr的值复制到r2
        "   ands        r2, r1                                          \n"  // r2 = lr & 4
        "   beq         1f                                              \n"  // 如果结果为0，跳转到标签1
        "   mrs         r0, psp                                         \n"  // 否则，将进程堆栈指针加载到r0
        "   b           2f                                              \n"  // 跳转到标签2
        "1:                                                             \n"
        "    mrs         r0, msp                                        \n"  // 如果结果为0，将主堆栈指针加载到r0
        "2:                                                             \n"
        
        "   subs        r0, r0, #32                                     \n" /* 预留其他寄存器空间 */
        "   mov         r1, r0                                          \n"
     
        "   stmia       r1!, {r4-r7}                                    \n" /* 保存 {r4 - r11} */
        "   mov         r4, r8                                          \n" 
        "   mov         r5, r9                                          \n"
        "   mov         r6, r10                                         \n"
        "   mov         r7, r11                                         \n"
        "   stmia       r1!, {r4-r7}                                    \n"

        "   mov         r1, lr                                          \n"/*   将 lr 的值移动到 r1*/
        "   mrs         r2, control                                     \n"/*   将 control 寄存器的值移动到 r2*/
        "   ldr         r3, hardfault_handler_c_address_const           \n"
        "   bx          r3                                              \n"
        "                                                               \n"
        "    .align 4                                                   \n"
        "hardfault_handler_c_address_const: .word hardfault_handler_c   \n"
        ::: "memory"
    );
}


void hardfault_handler_c(unsigned long sp, unsigned long lr , unsigned long control ){
    eh_task_sta_t                         sta;
    struct stack_state_context*           stack_state = (struct stack_state_context*)sp;
    eh_errln("");
    eh_errln("");
    eh_errln("######### hardfault_handler #########");
    eh_errln("SP:0x%08lx", sp);
    /* LR寄存器意义 Cortex-M0 Generic UG (r0p0, Issue A).pdf 章节：Table 2-12 Exception return behavior */
    eh_errln("LR:0x%08lx", lr);
    eh_errln("hardfault_handler  CONTROL:0x%08lx", control);


    /* 关于发生异常时，上下文堆栈情况 Cortex-M0 Generic UG (r0p0, Issue A).pdf 章节：2.3.6 Exception entry and return  */
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