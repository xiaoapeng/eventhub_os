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

#define  EH_EVENT_FIFO_CNT      8       /* 事件fifo的数量 必须选择 2 8 16 32， 数值越大理论性能越好*/

#define  eh_malloc(size)                malloc(size)
#define  eh_free(ptr)                   free(ptr)

#endif // _EVENT_CONFIG_H_