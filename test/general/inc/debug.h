/**
 * @file debug.h
 * @brief debug用
 * @author simon.xiaoapeng (simon.xiaoapeng@gmail.com)
 * @version 1.0
 * @date 2021-04-11
 * 
 * @copyright Copyright (c) 2023  simon.xiaoapeng@gmail.com
 * 
 * @par 修改日志:
 */

#ifndef __DEBUG__H__
#define __DEBUG__H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */

#define LOG_INTERVAL_TIME 		20			        /* N分钟一个文件 */
#define LOG_MAX_FILE_COUNT		5			        /* 最多能存储M个文件 */
#define LOG_DIR_PATH			"/userdata/log"	    /* 日志路径 */

typedef enum _DBG_LEVEL
{
    DBG_ERR = 1,
    DBG_WARNING,
    DBG_SYS,
    DBG_INFO,
    DBG_DEBUG,
}DBG_LEVEL;

extern DBG_LEVEL dbg_level;

extern int debug_printf(int print_time,const char *format, ...);
extern void debug_phex(const void *buf,  int len);


extern int debug_init(void);
extern void debug_exit(void);


#define DBG_print(level,s, ...)        if(level<=dbg_level) debug_printf(1, s "\r\n", ##__VA_ARGS__)
#define DBG_print_fl(level,s, ...)     if(level<=dbg_level) debug_printf(1, "[%s, %d]: " s "\r\n", __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define DBG_printf(level,s, ...)       if(level<=dbg_level) debug_printf(1, s , ##__VA_ARGS__)
#define DBG_print_hex(level,buf,len) 	    if(level<=dbg_level) debug_phex((buf),(len))
#define DBG_printf_raw(level,s, ...) 	if(level<=dbg_level) debug_printf(0, s ,##__VA_ARGS__)
#define DBG_set_dbg_Level(_level) 		 (dbg_level = (_level))


/* ######################## 下面是简单写法 ####################### */
/* 带自动回车的版本 */
#define dbg_debugln(s, ...)        DBG_print(DBG_DEBUG, s, ##__VA_ARGS__)
#define dbg_infoln(s, ...)         DBG_print(DBG_INFO, s, ##__VA_ARGS__)
#define dbg_sysln(s, ...)          DBG_print(DBG_SYS, s, ##__VA_ARGS__)
#define dbg_warnln(s, ...)         DBG_print(DBG_WARNING, s, ##__VA_ARGS__)
#define dbg_errln(s, ...)          DBG_print(DBG_ERR, s, ##__VA_ARGS__)
/* 带自动回车，且带函数定位 */
#define dbg_debugfl(s, ...)        DBG_print_fl(DBG_DEBUG, s, ##__VA_ARGS__)
#define dbg_infofl(s, ...)         DBG_print_fl(DBG_INFO, s, ##__VA_ARGS__)
#define dbg_sysfl(s, ...)          DBG_print_fl(DBG_SYS, s, ##__VA_ARGS__)
#define dbg_warnfl(s, ...)         DBG_print_fl(DBG_WARNING, s, ##__VA_ARGS__)
#define dbg_errfl(s, ...)          DBG_print_fl(DBG_ERR, s, ##__VA_ARGS__)
/* 不带回车 */
#define dbg_debug(s, ...)          DBG_printf(DBG_DEBUG, s, ##__VA_ARGS__)
#define dbg_info(s, ...)           DBG_printf(DBG_INFO, s, ##__VA_ARGS__)
#define dbg_sys(s, ...)            DBG_printf(DBG_SYS, s, ##__VA_ARGS__)
#define dbg_warn(s, ...)           DBG_printf(DBG_WARNING, s, ##__VA_ARGS__)
#define dbg_err(s, ...)            DBG_printf(DBG_ERR, s, ##__VA_ARGS__)
/* 打印内核的hex值 */
#define dbg_debughex(buf,len)      DBG_print_hex(DBG_DEBUG, buf, len)
#define dbg_infohex(buf,len)       DBG_print_hex(DBG_INFO, buf, len)
#define dbg_syshex(buf,len)        DBG_print_hex(DBG_SYS, buf, len)
#define dbg_warnhex(buf,len)       DBG_print_hex(DBG_WARNING, buf, len)
#define dbg_errhex(buf,len)        DBG_print_hex(DBG_ERR, buf, len)
/* 打印原始数据,其他版本都带由时间显示（开启DEBUG_USE_TIME宏时） */
#define dbg_debugraw(s, ...)       DBG_printf_raw(DBG_DEBUG, s, ##__VA_ARGS__)
#define dbg_inforaw(s, ...)        DBG_printf_raw(DBG_INFO, s, ##__VA_ARGS__)
#define dbg_sysraw(s, ...)         DBG_printf_raw(DBG_SYS, s, ##__VA_ARGS__)
#define dbg_warnraw(s, ...)        DBG_printf_raw(DBG_WARNING, s, ##__VA_ARGS__)
#define dbg_errraw(s, ...)         DBG_printf_raw(DBG_ERR, s, ##__VA_ARGS__)



/**
 * @brief 打印错误原因，且执行 action 语句
 */
#define DBG_ERROR_EXEC(expression, label, action)  do{          \
    if(expression){                                             \
        DBG_print_fl(DBG_ERR,label ": ("#expression") execute {"#action"}"); \
        action;                                                 \
    }                                                           \
}while(0)

/**
 * @brief 打印错误原因，且执行 action 语句
 */
#define DBG_INIT_ERROR_EXEC(expression, action) DBG_ERROR_EXEC(expression, "init", action)


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* __DEBUG__H__ */



