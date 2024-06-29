/**
 * eventhub_os 用户配置文件
 */

#ifndef _EH_USER_CONFIG_H_
#define _EH_USER_CONFIG_H_

#define EH_CONFIG_EVENT_CALLBACK_FUNCTION_STACK_SIZE            (8*1024U)
#define EH_CONFIG_CLOCKS_PER_SEC                                (1000000ULL)
#define eh_debugfl(fmt, ...)

#endif // _EH_USER_CONFIG_H_