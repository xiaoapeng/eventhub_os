/**
 * @file eh_formatio.h
 * @brief Implementation of standard io
 * @author simon.xiaoapeng (simon.xiaoapeng@gmail.com)
 * @date 2024-07-06
 * 
 * @copyright Copyright (c) 2024  simon.xiaoapeng@gmail.com
 * 
 */



#ifndef _EH_FORMATIO_H_
#define _EH_FORMATIO_H_

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <limits.h>

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */

enum stream_type{
    STREAM_TYPE_FUNCTION,
    STREAM_TYPE_FUNCTION_NO_CACHE,
    STREAM_TYPE_MEMORY,
};

struct stream_base{
    enum stream_type type;
};

struct stream_function{
    struct stream_base base;
    void (*write)(void *stream, const uint8_t *buf, size_t size);
    uint8_t *cache;
    uint8_t *pos;
    uint8_t *end;
};

struct stream_function_no_cache{
    struct stream_base base;
    void (*write)(void *stream, const uint8_t *buf, size_t size);
};

struct stream_memory{
    struct stream_base base;
    uint8_t *buf;
    uint8_t *pos;
    uint8_t *end;
};

extern struct stream_function _eh_stdout;

#define EH_STDOUT ((struct stream_base*)&_eh_stdout)


extern int eh_vprintf(const char *fmt, va_list args);
extern int eh_printf(const char *fmt, ...);

extern int eh_vsnprintf(char *buf, size_t size, const char *fmt, va_list args);
static inline int eh_vsprintf(char *buf, const char *fmt, va_list args){
    return eh_vsnprintf(buf, (size_t)((char *)(LONG_MAX) - buf), fmt, args);
}
extern int eh_snprintf(char *buf, size_t size, const char *fmt, ...);
extern int eh_sprintf(char *buf, const char *fmt, ...);

extern int eh_stream_vprintf(struct stream_base *stream, const char *fmt, va_list args);
extern int eh_stream_printf(struct stream_base *stream, const char *fmt, ...);
extern void eh_stream_putc(struct stream_base *stream, int c);
extern int eh_stream_puts(struct stream_base *stream, const char *s);
extern void eh_stream_finish(struct stream_base *stream);

static inline void eh_stream_function_init(struct stream_function *stream, 
    void (*write)(void *stream, const uint8_t *buf, size_t size), 
    uint8_t *cache, size_t cache_size){
    stream->base.type = STREAM_TYPE_FUNCTION;
    stream->write = write;
    stream->cache = cache;
    stream->pos = cache;
    stream->end = cache + cache_size;
}

static inline void eh_stream_function_no_cache_init(struct stream_function_no_cache *stream, 
    void (*write)(void *stream, const uint8_t *buf, size_t size)){
    stream->base.type = STREAM_TYPE_FUNCTION_NO_CACHE;
    stream->write = write;
}

static inline void eh_stream_memory_init(struct stream_memory *stream, uint8_t *buf, size_t size){
    stream->base.type = STREAM_TYPE_MEMORY;
    stream->buf = buf;
    stream->pos = buf;
    stream->end = buf + size;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif // _EH_FORMATIO_H_