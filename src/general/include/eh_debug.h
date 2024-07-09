/**
 * @file eh_debug.h
 * @brief debug相关接口实现
 * @author simon.xiaoapeng (simon.xiaoapeng@gmail.com)
 * @version 1.0
 * @date 2024-07-09
 * 
 * @copyright Copyright (c) 2024  simon.xiaoapeng@gmail.com
 * 
 * @par 修改日志:
 */
#ifndef _EH_DEBUG_H_
#define _EH_DEBUG_H_

#include "eh_config.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */

enum eh_dbg_level{
    EH_DBG_ERR = 1,
    EH_DBG_WARNING,
    EH_DBG_SYS,
    EH_DBG_INFO,
    EH_DBG_DEBUG,
};

enum eh_dbg_flags {
    EH_DBG_FLAGS_WALL_CLOCK         = 0x01, /* [%Y/%m/%d %H:%M:%S] */
    EH_DBG_FLAGS_MONOTONIC_CLOCK    = 0x02, /* [xxxx.xxx] */
    EH_DBG_FLAGS_DEBUG_TAG          = 0x04, /* [DBG/ERR/WARN/SYS/INFO] */
};

#if defined(EH_CONFIG_DEBUG_FLAGS)
    #define EH_DBG_FLAGS                            (EH_CONFIG_DEBUG_FLAGS)
#else
    #define EH_DBG_FLAGS                            (EH_DBG_FLAGS_DEBUG_TAG|EH_DBG_FLAGS_MONOTONIC_CLOCK)
#endif

#if defined(EH_CONFIG_DEBUG_ENTER_SIGN)
#define EH_DEBUG_ENTER_SIGN EH_CONFIG_DEBUG_ENTER_SIGN
#elif (defined EH_CMAKE_CONFIG_DEBUG_ENTER_SIGN)
#define EH_DEBUG_ENTER_SIGN EH_CMAKE_CONFIG_DEBUG_ENTER_SIGN
#else
#define EH_DEBUG_ENTER_SIGN "\r\n"
#endif

extern int eh_dbg_raw(enum eh_dbg_level level, enum eh_dbg_flags flags, const char *fmt, ...);
extern int eh_dbg_hex(enum eh_dbg_level level, enum eh_dbg_flags flags, size_t len, const void *buf);
#define eh_dbg_println(level, fmt, ...)     eh_dbg_raw(level, EH_DBG_FLAGS, fmt EH_DEBUG_ENTER_SIGN , ##__VA_ARGS__)
#define eh_dbg_printfl(level, fmt, ...)     eh_dbg_raw(level, EH_DBG_FLAGS, "[%s, %d]: " fmt EH_DEBUG_ENTER_SIGN , __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define eh_dbg_printraw(level, fmt, ...)    eh_dbg_raw(level, 0, fmt, ##__VA_ARGS__)

/* ######################## 下面是简单写法 ####################### */
/* 带自动回车的版本 */
#define eh_debugln(fmt, ...)                eh_dbg_println(EH_DBG_DEBUG, fmt, ##__VA_ARGS__)
#define eh_infoln(fmt, ...)                 eh_dbg_println(EH_DBG_INFO, fmt, ##__VA_ARGS__)
#define eh_sysln(fmt, ...)                  eh_dbg_println(EH_DBG_SYS, fmt, ##__VA_ARGS__)
#define eh_warnln(fmt, ...)                 eh_dbg_println(EH_DBG_WARNING, fmt, ##__VA_ARGS__)
#define eh_errln(fmt, ...)                  eh_dbg_println(EH_DBG_ERR, fmt, ##__VA_ARGS__)
/* 带自动回车，带函数定位 */
#define eh_debugfl(fmt, ...)                eh_dbg_printfl(EH_DBG_DEBUG, fmt, ##__VA_ARGS__)
#define eh_infofl(fmt, ...)                 eh_dbg_printfl(EH_DBG_INFO, fmt, ##__VA_ARGS__)
#define eh_sysfl(fmt, ...)                  eh_dbg_printfl(EH_DBG_SYS, fmt, ##__VA_ARGS__)
#define eh_warnfl(fmt, ...)                 eh_dbg_printfl(EH_DBG_WARNING, fmt, ##__VA_ARGS__)
#define eh_errfl(fmt, ...)                  eh_dbg_printfl(EH_DBG_ERR, fmt, ##__VA_ARGS__)
/* 原始数据版本 */
#define eh_debugraw(fmt, ...)               eh_dbg_printraw(EH_DBG_DEBUG, fmt, ##__VA_ARGS__)
#define eh_inforaw(fmt, ...)                eh_dbg_printraw(EH_DBG_INFO, fmt, ##__VA_ARGS__)
#define eh_sysraw(fmt, ...)                 eh_dbg_printraw(EH_DBG_SYS, fmt, ##__VA_ARGS__)
#define eh_warnraw(fmt, ...)                eh_dbg_printraw(EH_DBG_WARNING, fmt, ##__VA_ARGS__)
#define eh_errraw(fmt, ...)                 eh_dbg_printraw(EH_DBG_ERR, fmt, ##__VA_ARGS__)
/* 二进制buf打印 */
#define eh_debughex(buf,len)                eh_dbg_hex(EH_DBG_DEBUG, EH_DBG_FLAGS, len, buf)
#define eh_infohex(buf,len)                 eh_dbg_hex(EH_DBG_INFO, EH_DBG_FLAGS, len, buf)
#define eh_syshex(buf,len)                  eh_dbg_hex(EH_DBG_SYS, EH_DBG_FLAGS, len, buf)
#define eh_warnhex(buf,len)                 eh_dbg_hex(EH_DBG_WARNING, EH_DBG_FLAGS, len, buf)
#define eh_errhex(buf,len)                  eh_dbg_hex(EH_DBG_ERR, EH_DBG_FLAGS, len, buf)


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif // _EH_DEBUG_H_