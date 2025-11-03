/**
 * @file eh_debug.h
 * @brief debug相关接口实现
 * @author simon.xiaoapeng (simon.xiaoapeng@gmail.com)
 * @date 2024-07-09
 * 
 * @copyright Copyright (c) 2024  simon.xiaoapeng@gmail.com
 * 
 */
#ifndef _EH_DEBUG_H_
#define _EH_DEBUG_H_

#include <stddef.h>
#include <stdarg.h>

#include <eh_config.h>
#include <eh_types.h>

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */

enum eh_dbg_level{
#define  EH_DBG_SUPPRESS                0
#define  EH_DBG_ERR                     1
#define  EH_DBG_WARNING                 2
#define  EH_DBG_SYS                     3
#define  EH_DBG_INFO                    4
#define  EH_DBG_DEBUG                   5
    _EH_DBG_SUPPRESS = EH_DBG_SUPPRESS,    /* 意味着不输出任何信息 */
    _EH_DBG_ERR = EH_DBG_ERR,
    _EH_DBG_WARNING = EH_DBG_WARNING,
    _EH_DBG_SYS = EH_DBG_SYS,
    _EH_DBG_INFO = EH_DBG_INFO,
    _EH_DBG_DEBUG = EH_DBG_DEBUG,
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
#else
#define EH_DEBUG_ENTER_SIGN "\r\n"
#endif

static inline int _eh_dbg_feign_return(int n){
    return n;
}

#define EH_MACRO_DEBUG_LEVEL(name) ((uint32_t)(EH_STRINGIFY(name)[0] - '0'))

#define eh_mprintfl(name, tag, level, fmt, ...) ({\
        int n = 0; \
        if( EH_MACRO_DEBUG_LEVEL(name) >= (uint32_t)level){ \
            n = eh_dbg_printfl(level, "[" #tag "] ", fmt, ##__VA_ARGS__); \
        } \
        _eh_dbg_feign_return(n); \
    })

#define eh_mprintln(name, tag, level, fmt, ...) ({\
        int n = 0; \
        if( EH_MACRO_DEBUG_LEVEL(name) >= level){ \
            n = eh_dbg_println(level, "[" #tag "] ", fmt, ##__VA_ARGS__); \
        } \
        _eh_dbg_feign_return(n); \
    })

#define eh_mprintraw(name, level, fmt, ...) ({\
        int n = 0; \
        if( EH_MACRO_DEBUG_LEVEL(name) >= level){ \
            n = eh_dbg_printraw(level, fmt, ##__VA_ARGS__); \
        } \
        _eh_dbg_feign_return(n); \
    })

#define eh_mhex(name, level, buf, len) ({\
        int n = 0; \
        if( EH_MACRO_DEBUG_LEVEL(name) >= level){ \
            n = eh_dbg_hex(level, EH_DBG_FLAGS, len, buf); \
        } \
        _eh_dbg_feign_return(n); \
    })

extern int eh_dbg_set_level(enum eh_dbg_level level);
extern int eh_dbg_raw(enum eh_dbg_level level, enum eh_dbg_flags flags, const char *fmt, ...);
extern int eh_vdbg_raw(enum eh_dbg_level level, enum eh_dbg_flags flags, const char *fmt, va_list args);
extern int eh_dbg_hex(enum eh_dbg_level level, enum eh_dbg_flags flags, size_t len, const void *buf);
#define eh_dbg_println(level, tag_str, fmt, ...)     eh_dbg_raw(level, EH_DBG_FLAGS, tag_str fmt EH_DEBUG_ENTER_SIGN , ##__VA_ARGS__)
#define eh_dbg_printfl(level, tag_str, fmt, ...)     eh_dbg_raw(level, EH_DBG_FLAGS, tag_str "[%s, %d]: " fmt EH_DEBUG_ENTER_SIGN , __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define eh_dbg_printraw(level, fmt, ...)    eh_dbg_raw(level, 0, fmt, ##__VA_ARGS__)

/* ######################## 下面是简单写法 ####################### */
/* 带自动回车的版本 */
#define eh_debugln(fmt, ...)                eh_dbg_println(EH_DBG_DEBUG, "", fmt, ##__VA_ARGS__)
#define eh_infoln(fmt, ...)                 eh_dbg_println(EH_DBG_INFO, "", fmt, ##__VA_ARGS__)
#define eh_sysln(fmt, ...)                  eh_dbg_println(EH_DBG_SYS, "", fmt, ##__VA_ARGS__)
#define eh_warnln(fmt, ...)                 eh_dbg_println(EH_DBG_WARNING, "", fmt, ##__VA_ARGS__)
#define eh_errln(fmt, ...)                  eh_dbg_println(EH_DBG_ERR, "", fmt, ##__VA_ARGS__)
/* 带自动回车，带函数定位 */
#define eh_debugfl(fmt, ...)                eh_dbg_printfl(EH_DBG_DEBUG, "", fmt, ##__VA_ARGS__)
#define eh_infofl(fmt, ...)                 eh_dbg_printfl(EH_DBG_INFO, "", fmt, ##__VA_ARGS__)
#define eh_sysfl(fmt, ...)                  eh_dbg_printfl(EH_DBG_SYS, "", fmt, ##__VA_ARGS__)
#define eh_warnfl(fmt, ...)                 eh_dbg_printfl(EH_DBG_WARNING, "", fmt, ##__VA_ARGS__)
#define eh_errfl(fmt, ...)                  eh_dbg_printfl(EH_DBG_ERR, "", fmt, ##__VA_ARGS__)
/* 原始数据版本 */
#define eh_debugraw(fmt, ...)               eh_dbg_printraw(EH_DBG_DEBUG, fmt, ##__VA_ARGS__)
#define eh_inforaw(fmt, ...)                eh_dbg_printraw(EH_DBG_INFO, fmt, ##__VA_ARGS__)
#define eh_sysraw(fmt, ...)                 eh_dbg_printraw(EH_DBG_SYS, fmt, ##__VA_ARGS__)
#define eh_warnraw(fmt, ...)                eh_dbg_printraw(EH_DBG_WARNING, fmt, ##__VA_ARGS__)
#define eh_errraw(fmt, ...)                 eh_dbg_printraw(EH_DBG_ERR, fmt, ##__VA_ARGS__)
/* 16进制数组打印 */
#define eh_debughex(buf,len)                eh_dbg_hex(EH_DBG_DEBUG, EH_DBG_FLAGS, len, buf)
#define eh_infohex(buf,len)                 eh_dbg_hex(EH_DBG_INFO, EH_DBG_FLAGS, len, buf)
#define eh_syshex(buf,len)                  eh_dbg_hex(EH_DBG_SYS, EH_DBG_FLAGS, len, buf)
#define eh_warnhex(buf,len)                 eh_dbg_hex(EH_DBG_WARNING, EH_DBG_FLAGS, len, buf)
#define eh_errhex(buf,len)                  eh_dbg_hex(EH_DBG_ERR, EH_DBG_FLAGS, len, buf)


/**
 * 可通过编译宏来精细的控制不同模块的打印等级
 * 假如有如下语句
 *   void func(void){
 *       eh_mdebugln(FUNC_A, "1 hello world");
 *       eh_minfoln(FUNC_A, "2 hello world");
 *       eh_merrln(FUNC_A, "3 hello world");
 *   }
 * 只有输出语句的打印等级同时满足全局打印等级和模块打印等级才会输出
 * 全局打印等级可以通过eh_dbg_set_level函数设置
 * 而模块打印等级则只能在编译器设置,设置宏为EH_DBG_MODEULE_LEVEL_<module_name>来设置
 * 例如：
 *   #define EH_DBG_MODEULE_LEVEL_FUNC_A  EH_DBG_DEBUG
 *   或者由编译脚本来控制 -DEH_DBG_MODEULE_LEVEL_FUNC_A=EH_DBG_DEBUG
 *
 */
/* 模块可进行宏控制打印是否输出*/
#define eh_mdebugln(name, fmt, ...)  eh_mprintln(EH_DBG_MODEULE_LEVEL_##name, name, EH_DBG_DEBUG, fmt, ##__VA_ARGS__)
#define eh_minfoln(name, fmt, ...)   eh_mprintln(EH_DBG_MODEULE_LEVEL_##name, name, EH_DBG_INFO, fmt, ##__VA_ARGS__)
#define eh_msysln(name, fmt, ...)    eh_mprintln(EH_DBG_MODEULE_LEVEL_##name, name, EH_DBG_SYS, fmt, ##__VA_ARGS__)
#define eh_mwarnln(name, fmt, ...)   eh_mprintln(EH_DBG_MODEULE_LEVEL_##name, name, EH_DBG_WARNING, fmt, ##__VA_ARGS__)
#define eh_merrln(name, fmt, ...)    eh_mprintln(EH_DBG_MODEULE_LEVEL_##name, name, EH_DBG_ERR, fmt, ##__VA_ARGS__)
/* 模块可进行宏控制打印是否输出(带行号和函数名称) */
#define eh_mdebugfl(name, fmt, ...)  eh_mprintfl(EH_DBG_MODEULE_LEVEL_##name, name, EH_DBG_DEBUG, fmt, ##__VA_ARGS__)
#define eh_minfofl(name, fmt, ...)   eh_mprintfl(EH_DBG_MODEULE_LEVEL_##name, name, EH_DBG_INFO, fmt, ##__VA_ARGS__)
#define eh_msysfl(name, fmt, ...)    eh_mprintfl(EH_DBG_MODEULE_LEVEL_##name, name, EH_DBG_SYS, fmt, ##__VA_ARGS__)
#define eh_mwarnfl(name, fmt, ...)   eh_mprintfl(EH_DBG_MODEULE_LEVEL_##name, name, EH_DBG_WARNING, fmt, ##__VA_ARGS__)
#define eh_merrfl(name, fmt, ...)    eh_mprintfl(EH_DBG_MODEULE_LEVEL_##name, name, EH_DBG_ERR, fmt, ##__VA_ARGS__)
/* 模块可进行宏控制打印是否输出(原始数据) */
#define eh_mdebugraw(name, fmt, ...)  eh_mprintraw(EH_DBG_MODEULE_LEVEL_##name, EH_DBG_DEBUG, fmt, ##__VA_ARGS__)
#define eh_minforaw(name, fmt, ...)   eh_mprintraw(EH_DBG_MODEULE_LEVEL_##name, EH_DBG_INFO, fmt, ##__VA_ARGS__)
#define eh_msysraw(name, fmt, ...)    eh_mprintraw(EH_DBG_MODEULE_LEVEL_##name, EH_DBG_SYS, fmt, ##__VA_ARGS__)
#define eh_mwarnraw(name, fmt, ...)   eh_mprintraw(EH_DBG_MODEULE_LEVEL_##name, EH_DBG_WARNING, fmt, ##__VA_ARGS__)
#define eh_merrraw(name, fmt, ...)    eh_mprintraw(EH_DBG_MODEULE_LEVEL_##name, EH_DBG_ERR, fmt, ##__VA_ARGS__)
/* 模块可进行宏控制打印是否输出(16进制) */
#define eh_mdebughex(name, buf, len)  eh_mhex(EH_DBG_MODEULE_LEVEL_##name, EH_DBG_DEBUG, buf, len)
#define eh_minfohex(name, buf, len)   eh_mhex(EH_DBG_MODEULE_LEVEL_##name, EH_DBG_INFO, buf, len)
#define eh_msyshex(name, buf, len)    eh_mhex(EH_DBG_MODEULE_LEVEL_##name, EH_DBG_SYS, buf, len)
#define eh_mwarnhex(name, buf, len)   eh_mhex(EH_DBG_MODEULE_LEVEL_##name, EH_DBG_WARNING, buf, len)
#define eh_merrhex(name, buf, len)    eh_mhex(EH_DBG_MODEULE_LEVEL_##name, EH_DBG_ERR, buf, len)

/**
 * @brief 打印错误原因，且执行 action 语句
 */
#define EH_DBG_ERROR_EXEC(expression, action)  do{                                 \
    if(expression){                                                                \
        eh_dbg_printfl(EH_DBG_ERR, "", "(%s) execute {%s}", #expression, #action); \
        action;                                                                    \
    }                                                                              \
}while(0)





#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif // _EH_DEBUG_H_