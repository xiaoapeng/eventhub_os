/**
 * @file eh_comp_timer.c
 * @brief eventhub timer signal component
 * @author simon.xiaoapeng (simon.xiaoapeng@gmail.com)
 * @date 2026-01-21
 * 
 * @copyright Copyright (c) 2026  simon.xiaoapeng@gmail.com
 * 
 */

#include <eh_module.h>
#include <eh_comp_timer.h>

#ifdef CONFIG_PACKAGE_EVENTHUB_GLOBAL_TIMER_SIGNAL

#ifdef CONFIG_PACKAGE_EVENTHUB_GLOBAL_TIMER_SIGNAL_1S
EH_DEFINE_CUSTOM_SIGNAL(signal_eh_comp_timer_1s, eh_event_timer_t,  EH_TIMER_INIT(signal_eh_comp_timer_1s.custom_event));
#endif

#ifdef CONFIG_PACKAGE_EVENTHUB_GLOBAL_TIMER_SIGNAL_500MS
EH_DEFINE_CUSTOM_SIGNAL(signal_eh_comp_timer_500ms, eh_event_timer_t,  EH_TIMER_INIT(signal_eh_comp_timer_500ms.custom_event));
#endif

#ifdef CONFIG_PACKAGE_EVENTHUB_GLOBAL_TIMER_SIGNAL_100MS
EH_DEFINE_CUSTOM_SIGNAL(signal_eh_comp_timer_100ms, eh_event_timer_t,  EH_TIMER_INIT(signal_eh_comp_timer_100ms.custom_event));
#endif

static int __init ehip_comp_timer_init(void){

#ifdef CONFIG_PACKAGE_EVENTHUB_GLOBAL_TIMER_SIGNAL_1S
    eh_timer_advanced_init(
        eh_signal_to_custom_event(&signal_eh_comp_timer_1s), 
        (eh_sclock_t)eh_msec_to_clock(1000*1), 
        EH_TIMER_ATTR_AUTO_CIRCULATION);
    eh_timer_start(eh_signal_to_custom_event(&signal_eh_comp_timer_1s));
#endif

#ifdef CONFIG_PACKAGE_EVENTHUB_GLOBAL_TIMER_SIGNAL_500MS
    eh_timer_advanced_init(
        eh_signal_to_custom_event(&signal_eh_comp_timer_500ms), 
        (eh_sclock_t)eh_msec_to_clock(100*5), 
        EH_TIMER_ATTR_AUTO_CIRCULATION);
    eh_timer_start(eh_signal_to_custom_event(&signal_eh_comp_timer_500ms));
#endif

#ifdef CONFIG_PACKAGE_EVENTHUB_GLOBAL_TIMER_SIGNAL_100MS
    eh_timer_advanced_init(
        eh_signal_to_custom_event(&signal_eh_comp_timer_100ms), 
        (eh_sclock_t)eh_msec_to_clock(100*1), 
        EH_TIMER_ATTR_AUTO_CIRCULATION);
    eh_timer_start(eh_signal_to_custom_event(&signal_eh_comp_timer_100ms));
#endif

    return 0;
}

static void __exit ehip_comp_timer_exit(void){
    
#ifdef CONFIG_PACKAGE_EVENTHUB_GLOBAL_TIMER_SIGNAL_100MS
    eh_timer_stop(eh_signal_to_custom_event(&signal_eh_comp_timer_100ms));
#endif

#ifdef CONFIG_PACKAGE_EVENTHUB_GLOBAL_TIMER_SIGNAL_500MS
    eh_timer_stop(eh_signal_to_custom_event(&signal_eh_comp_timer_500ms));
#endif

#ifdef CONFIG_PACKAGE_EVENTHUB_GLOBAL_TIMER_SIGNAL_1S
    eh_timer_stop(eh_signal_to_custom_event(&signal_eh_comp_timer_1s));
#endif
}

eh_comp_module_export(ehip_comp_timer_init, ehip_comp_timer_exit);

#endif
