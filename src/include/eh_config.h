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
/* 最大支持的任务数量 */
#define EH_CONFIG_MAX_TASK_NUM                  32

//#include "debug.h"
#define eh_debugfl(fmt, ...)                    //dbg_debugfl(fmt, ##__VA_ARGS__)

#endif // _EVENT_CONFIG_H_