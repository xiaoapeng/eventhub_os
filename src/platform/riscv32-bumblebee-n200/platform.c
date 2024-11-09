
#include <stdio.h>
#include <stdlib.h>
#include <eh.h>
#include <eh_debug.h>
#include <eh_event.h>
#include <eh_timer.h>
#include <eh_platform.h>


__attribute__((naked))  eh_save_state_t  platform_enter_critical(void){
    __asm__ volatile(
        "   csrr        a0, mstatus                            \n"
        "   csrci       mstatus, 8                             \n"
        "   andi        a0, a0, 0x8                            \n"
        "   ret                                                \n"
        ::: "a0", "memory"
    );
}

__attribute__((naked)) void  platform_exit_critical( 
        __attribute__((unused)) eh_save_state_t state) {
    __asm__ volatile(
        "   csrr       t1, mstatus                             \n"
        "   or         t1, t1, a0                              \n"
        "   csrs       mstatus, t1                             \n"
        "   ret                                                \n"
        ::: "t1", "memory"
    );
}
