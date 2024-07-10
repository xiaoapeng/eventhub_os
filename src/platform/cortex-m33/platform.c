
#include <stdio.h>
#include <stdlib.h>
#include "eh.h"
#include "eh_event.h"
#include "eh_timer.h"
#include "eh_platform.h"


__attribute__((naked))  eh_save_state_t  platform_enter_critical(void){
    __asm__ volatile(
        "	.syntax unified									\n"
        "													\n"
        "	mrs r0, primask									\n"/* 保存中断状态在R0中 */
        "	cpsid   i										\n"/* 失能中断 */
        "	dsb												\n"
        "	isb												\n"
        "	bx lr											\n"/* Return. */
        ::: "r0", "memory"
    );
}

__attribute__((naked)) void  platform_exit_critical(eh_save_state_t state){
    __asm__ volatile(
        "	.syntax unified									\n"
        "													\n"
        "	msr primask, r0									\n"/* 恢复中断状态 */
        "	dsb												\n"
        "	isb												\n"
        "	bx lr											\n"/* Return. */
        ::: "memory"
    );
}


extern char __HeapBase[]; // 链接器脚本中定义的堆的开始地址
extern char __heap_limit[]; // 链接器脚本中定义的堆的开始地址
static char *heap_end = __HeapBase;

caddr_t _sbrk(int incr) {
    char *prev_heap_end = heap_end;

    // 检查堆是否超出预定范围
    if (prev_heap_end + incr > __heap_limit) {
        // 如果超出范围，返回-1表示失败
        return (caddr_t) -1;
    }

    heap_end += incr;
    return (caddr_t) prev_heap_end;
}


void  platform_idle_break(void){

}

void  platform_idle_or_extern_event_handler(void){
    
}


static void generic_idle_or_extern_event_handler(void){
    //(void)eh_clock_to_usec(eh_get_loop_idle_time());
}
static void generic_idle_break( void ){

}


static int  __init generic_platform_init(void){
    return 0;
}

static void __exit generic_platform_deinit(void){
    
}

eh_core_module_export(generic_platform_init, generic_platform_deinit);