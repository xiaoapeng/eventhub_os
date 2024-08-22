/**
 * @file typedef.h
 * @brief 平台定义
 * @author simon.xiaoapeng (simon.xiaoapeng@gmail.com)
 * @version 1.0
 * @date 2023-04-01
 * 
 * @copyright Copyright (c) 2023  simon.xiaoapeng@gmail.com
 * 
 * @par 修改日志:
 */


#ifndef _TYPEDEF_H_
#define _TYPEDEF_H_


#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */

#include <stdint.h>
#include <stdlib.h>

#include <stdio.h>
#include <stdint.h>
#include <time.h>

static inline uint32_t get_milliseconds() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);

    uint32_t milliseconds = (ts.tv_sec * 1000) + (ts.tv_nsec / 1000000);
    return milliseconds;
}

#define MALLOC(__size) 
#define FREE(__ptr)
#define DELAY(__ms)
#define DELAY_US(__us)
#define GET_TICK()       get_milliseconds()



#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif // _TYPEDEF_H_
