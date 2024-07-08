/**
 * @file eh_formatio.h
 * @brief Implementation of standard io
 * @author simon.xiaoapeng (simon.xiaoapeng@gmail.com)
 * @version 1.0
 * @date 2024-07-06
 * 
 * @copyright Copyright (c) 2024  simon.xiaoapeng@gmail.com
 * 
 * @par 修改日志:
 */

#include <stdarg.h>
#include <stddef.h>
#ifndef _EH_FORMATIO_H_
#define _EH_FORMATIO_H_


#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */

extern int eh_printf(const char *fmt, ...)  __attribute__((format(printf, 1, 2)));
extern int eh_snprintf(char *buf, size_t size, const char *fmt, ...) __attribute__((format(printf, 3, 4)));
extern int eh_sprintf(char *buf, const char *fmt, ...) __attribute__((format(printf, 2, 3)));

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif // _EH_FORMATIO_H_