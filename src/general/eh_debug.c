/**
 * @file eh_debug.c
 * @brief debug实现
 * @author simon.xiaoapeng (simon.xiaoapeng@gmail.com)
 * @date 2024-07-09
 * 
 * @copyright Copyright (c) 2024  simon.xiaoapeng@gmail.com
 * 
 */


#include <stdarg.h>
#include <eh.h>
#include <eh_platform.h>
#include <eh_formatio.h>
#include <eh_debug.h>
#include <eh_config.h>

#if (defined(EH_CONFIG_DEFAULT_DEBUG_LEVEL))
static enum eh_dbg_level dbg_level = EH_CONFIG_DEFAULT_DEBUG_LEVEL;
#else
static enum eh_dbg_level dbg_level = EH_DBG_DEBUG;
#endif

const char *eh_dbg_level_str[] = {
    [0] = "UNDEF",
    [EH_DBG_ERR] = "ERR",
    [EH_DBG_WARNING] = "WARN",
    [EH_DBG_SYS] = "SYS",
    [EH_DBG_INFO] = "INFO",
    [EH_DBG_DEBUG] = "DBG",
};

int eh_dbg_set_level(enum eh_dbg_level level){
    dbg_level = level;
    return 0;
}


int eh_dbg_raw(enum eh_dbg_level level, 
    enum eh_dbg_flags flags, const char *fmt, ...){
    int n = 0;
    va_list args;
    eh_usec_t now_usec;
    if(level > dbg_level)
        return 0;
    now_usec = eh_clock_to_usec(eh_get_clock_monotonic_time());
    if(flags & EH_DBG_FLAGS_WALL_CLOCK){
        /* 打印墙上时间 */
    }
    if(flags & EH_DBG_FLAGS_MONOTONIC_CLOCK){
        n += eh_printf("[%5u.%06u] ", (unsigned int)(now_usec/1000000), 
            (unsigned int)(now_usec%1000000));
    }
    if(flags & EH_DBG_FLAGS_DEBUG_TAG && level >= EH_DBG_ERR && level <= EH_DBG_DEBUG){
        n += eh_printf("[%4s] ", eh_dbg_level_str[level]);
    }
    va_start(args, fmt);
    n += eh_vprintf(fmt, args);
    va_end(args);
    return n;
}

int eh_dbg_hex(enum eh_dbg_level level, 
    enum eh_dbg_flags flags, size_t len, const void *buf){
    const uint8_t *pos = buf;
    int n = 0;
    size_t y_n, x_n;
    if(level > dbg_level)
        return 0;
    y_n = len / 16;
    x_n = len % 16;
    n += eh_dbg_raw(level, flags, "______________________________________________________________" EH_DEBUG_ENTER_SIGN);
    n += eh_dbg_raw(level, flags, "            | 0| 1| 2| 3| 4| 5| 6| 7| 8| 9| A| B| C| D| E| F||" EH_DEBUG_ENTER_SIGN);
    n += eh_dbg_raw(level, flags, "--------------------------------------------------------------" EH_DEBUG_ENTER_SIGN);
    for(size_t i = 0; i < y_n; i++, pos += 16){
        n += eh_dbg_raw(level, flags, "|0x%08x| %-47.*hhq||" EH_DEBUG_ENTER_SIGN, 
            (unsigned int)(i*16), 16, pos);
    }
    if(x_n){
        n += eh_dbg_raw(level, flags, "|0x%08x| %-47.*hhq||" EH_DEBUG_ENTER_SIGN, 
                (unsigned int)(y_n*16), x_n, pos);
    }
    n += eh_dbg_raw(level, flags, "--------------------------------------------------------------" EH_DEBUG_ENTER_SIGN);
    return n;
}