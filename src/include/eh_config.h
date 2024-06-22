/**
 * @file event_config.h
 * @brief 配置文件
 * @author simon.xiaoapeng (simon.xiaoapeng@gmail.com)
 * @version 1.0
 * @date 2024-04-14
 * 
 * @copyright Copyright (c) 2024  simon.xiaoapeng@gmail.com
 * 
 * @par 修改日志:
 */
#ifndef _EVENT_CONFIG_H_
#define _EVENT_CONFIG_H_

#include <stdlib.h>
#include "debug.h"


#define EH_EVENT_CALLBACK_FUNCTION_STACK_SIZE          (8*1024)
#define eh_debugfl(fmt, ...)                            dbg_debugfl(fmt, ##__VA_ARGS__)

#endif // _EVENT_CONFIG_H_