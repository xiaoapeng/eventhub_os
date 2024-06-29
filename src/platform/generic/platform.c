#include "eh.h"
#include "eh_event.h"
#include "eh_timer.h"
#include "eh_platform.h"

eh_clock_t  platform_get_clock_monotonic_time(void){
    return 0;
}
eh_save_state_t  platform_enter_critical(void){
    return 0;
}
void  platform_exit_critical(eh_save_state_t state){
    (void)state;
}
void* platform_malloc(size_t size){
    (void)size;
    return NULL;
}
void  platform_free(void* ptr){
    (void)ptr;
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

}

static void __exit generic_platform_deinit(void){
    
}

eh_core_module_export(generic_platform_init, generic_platform_deinit);