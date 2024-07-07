/**
 * @file eh_formatio.c
 * @brief  Implementation of standard io
 * @author simon.xiaoapeng (simon.xiaoapeng@gmail.com)
 * @version 1.0
 * @date 2024-07-06
 * 
 * @copyright Copyright (c) 2024  simon.xiaoapeng@gmail.com
 * 
 * @par 修改日志:
 */


#include <stdint.h>
#include <stdarg.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "eh_mem.h"
#include "eh_types.h"
#include "eh_formatio.h"
#include "eh_config.h"

#define FORMAT_LEFT         0x00000001          /* 输出结果左对齐 */
#define FORMAT_PLUS         0x00000002          /* 输出结果若为正数则添加正号 */
#define FORMAT_SPACE        0x00000004          /* 输出结果若为正数则添加空格 */
#define FORMAT_SPECIAL      0x00000008          /* 特殊标志位，用于添加一些特定符号，比如%#x自动添加0x */
#define FORMAT_ZEROPAD      0x00000010          /* 当宽度大于输出结果长度时，则用0填充 */
#define FORMAT_LARGE        0x00000020          /* 使用大写输出 */
#define FORMAT_SIGNED       0x00000040          /* 有符号输出 */
#define FORMAT_FLOAT_E      0x00000040          /* 浮点数输出  E格式*/
#define FORMAT_FLOAT_F      0x00000040          /* 浮点数输出  F格式*/
#define FORMAT_FLOAT_G      0x00000040          /* 浮点数输出  F格式*/

enum stream_type{
    STREAM_TYPE_FUNCTION,
    STREAM_TYPE_MEMORY,
};

enum format_qualifier{
    FORMAT_QUALIFIER_NONE,
    FORMAT_QUALIFIER_LONG,
    FORMAT_QUALIFIER_LONG_LONG,
    FORMAT_QUALIFIER_SHORT,
    FORMAT_QUALIFIER_CHAR,
    FORMAT_QUALIFIER_SIZE_T,
};

enum base_type{
    BASE_TYPE_BIN = 2, 
    BASE_TYPE_OCT = 8, 
    BASE_TYPE_DEC = 10, 
    BASE_TYPE_HEX = 16
}base;

struct stream_out{
    enum stream_type type;
    union{
        struct{
            void (*write)(void *stream, const uint8_t *buf, size_t size);
            uint8_t *cache;
            uint8_t *pos;
            uint8_t *end;
        }f;
        struct{
            uint8_t *buf;
            uint8_t *pos;
            uint8_t *end;
        }m;
    };
};


static const char small_digits[] = "0123456789abcdef";
static const char large_digits[] = "0123456789ABCDEF";
static uint8_t _stdout_cache[EH_CONFIG_STDOUT_MEM_CACHE_SIZE];

__weak void stdout_write(void *stream, const uint8_t *buf, size_t size){
    (void)stream;
    (void)buf;
    (void)size;
}

static struct stream_out _stdout = {
    .type = STREAM_TYPE_FUNCTION,
    .f = {
        .write = stdout_write,
        .cache = _stdout_cache,
        .pos = _stdout_cache,
        .end = _stdout_cache + EH_CONFIG_STDOUT_MEM_CACHE_SIZE,
    },
};



static int num_rsh(unsigned long long *number, int base){
    int res;
    res = (int)((*number) % (unsigned long long)base);
    *number = (*number) / (unsigned long long)base;
    return res;
}

/* 获取数字在任意进制下的位数 */
static int num_bit_count(unsigned long long number, int base){
    int res = 0;
    do{
        number = number / (unsigned long long)base;
        res++;
    }while(number);
    return res;
}

static inline void streamout_in_byte(struct stream_out *stream, char ch){
    switch(stream->type){
        case STREAM_TYPE_FUNCTION:
            if(stream->f.pos < stream->f.end){
                *stream->f.pos = (uint8_t)ch;
                stream->f.pos++;
            }
            if(stream->f.pos >= stream->f.end){
                stream->f.write(&stream, stream->f.cache,  (size_t)(stream->f.end - stream->f.cache));
                stream->f.pos = stream->f.cache;
            }
            break;
        case STREAM_TYPE_MEMORY:
            if(stream->m.pos < stream->m.end){
                *stream->m.pos = (uint8_t)ch;
                stream->m.pos++;
            }
            break;
    }
}

static inline void streamout_finish(struct stream_out *stream){
    switch(stream->type){
        case STREAM_TYPE_FUNCTION:
            if(stream->f.pos > stream->f.cache){
                stream->f.write(&stream, stream->f.cache,  (size_t)(stream->f.pos - stream->f.cache));
                stream->f.pos = stream->f.cache;
            }
            break;
        case STREAM_TYPE_MEMORY:
            if(stream->m.pos < stream->m.end){
                *stream->m.pos = '\0';
            }else{
                *(stream->m.end - 1) = '\0';
            }
    }
}

static inline int skip_atoi(const char **s)
{
    int i = 0;
    while (isdigit(**s))
        i = i * 10 + (*((*s)++) - '0');
    return i;
}

static inline int vprintf_char(struct stream_out *stream, char ch, int field_width, int flags){
    int n = 0;
    n = field_width <= 1 ? 1 : field_width;
    if(flags & FORMAT_LEFT)
        streamout_in_byte(stream, ch);
    while(field_width-- > 1)
        streamout_in_byte(stream, ' ');
    if(!(flags & FORMAT_LEFT))
        streamout_in_byte(stream, ch);
    return n;
}

static inline int vprintf_string(struct stream_out *stream, char *s, int field_width, int precision, int flags){
    int n = 0;
    int len;
    int diff;
    if( s == NULL )
        s= "(null)";
    if(field_width < 0){
        for(len = 0; (len != precision) && (s[len]); len++,n++)
            streamout_in_byte(stream, s[len]);
        return n;
    }

    for (len = 0; (len != precision) && (s[len]); len++) { }

    if(field_width <= len){
        for(int i=0; i<len; i++,n++){
            streamout_in_byte(stream, s[i]);
        }
        return n;
    }
    diff = field_width - len;
    if(!(flags & FORMAT_LEFT)){
        /* 右对齐 */
        for(int i=0; i<diff; i++,n++){
            streamout_in_byte(stream, ' ');
        }
    }
    for(int i=0; i<len; i++,n++){
        streamout_in_byte(stream, s[i]);
    }
    
    if(flags & FORMAT_LEFT){
        /* 左对齐 */
        for(int i=0; i<diff; i++,n++){
            streamout_in_byte(stream, ' ');
        }
    }
    return n;
}

static inline int vprintf_float(struct stream_out *stream, double num, int field_width, int precision, int flags){
    (void) stream;
    (void) num;
    (void) field_width;
    (void) precision;
    (void) flags;
    int n=0;
    
    return n;
}
static inline int vprintf_number(struct stream_out *stream, unsigned long long num, int field_width, int precision, int flags, enum base_type base){
    char *number_buf;
    char _number_buf[16];
    char sign = 0;
    char *special = NULL;
    const char *digits = small_digits;
    int bit_count;
    int n = 0;
    int special_count = 0;
    int reality_count = 0;
    int zeropad_count = 0;
    int spacepad_count = 0;
    
    if(base > BASE_TYPE_HEX) return 0;

    if(flags & FORMAT_LARGE)
        digits = large_digits;
    /* 正数化 */
    if(flags & FORMAT_SIGNED){
        if((long long)num < 0){
            sign = '-';
            num = -num;
        }
    }
    if(sign != '-'){
        if(flags & FORMAT_PLUS){
            sign = '+';
        }else if(flags & FORMAT_SPACE){
            sign = ' ';
        }
    }
    
    bit_count = num_bit_count(num, (int)base);
    
    /* 不要轻易去malloc */
    if(bit_count > (int)sizeof(_number_buf)){
        number_buf = eh_malloc((size_t)bit_count);
        if(number_buf == NULL)  return 0;
    }else{
        number_buf = _number_buf;
    }
    for(int i=(int)(bit_count-1); i>=0; i--){
        number_buf[i] = digits[num_rsh(&num, (int)base)];
    }

    if(flags&FORMAT_SPECIAL){
        if(base == BASE_TYPE_OCT){
            special = "0";
            special_count = 1;
        }else if(base == BASE_TYPE_HEX){
            special = flags & FORMAT_LARGE ? "0X":"0x";
            special_count = 2;
        }else if(base == BASE_TYPE_BIN){
            special = flags & FORMAT_LARGE ? "0B":"0b";
            special_count = 2;
        }
    }

    reality_count = ((sign) ? 1 : 0) + special_count + bit_count;

    if(precision >= 0){
        flags = (flags & ~FORMAT_ZEROPAD);
        if(precision > bit_count)
            zeropad_count = precision - bit_count;
        reality_count = reality_count + zeropad_count;
    }

    if(field_width > reality_count){
        if(flags & FORMAT_ZEROPAD){
            zeropad_count = field_width - reality_count;
        }else{
            spacepad_count = field_width - reality_count;
        }
    }

    /* 右对齐空格填充 */
    if(!(flags & FORMAT_LEFT)){
        for(int i=0; i<spacepad_count; i++,n++){
            streamout_in_byte(stream, ' ');
        }
    }

    /* 符号位 */
    if(sign){
        streamout_in_byte(stream, sign);
        n++;
    }
    /* 特殊字符 0x 0X 0 */
    for(int i=0; i<special_count; i++,n++){
        streamout_in_byte(stream, special[i]);
    }
    /* 中间0填充 */
    for(int i=0; i<zeropad_count; i++,n++){
        streamout_in_byte(stream, '0');
    }
    /* 数字有效位 */
    for(int i=0; i<bit_count; i++,n++){
        streamout_in_byte(stream, number_buf[i]);
    }
    
    /* 左对齐空格填充 */
    if(flags & FORMAT_LEFT){
        for(int i=0; i<spacepad_count; i++,n++){
            streamout_in_byte(stream, ' ');
        }
    }
    if(number_buf != _number_buf)
        eh_free(number_buf);
    return n;
}

static int eh_stream_vprintf(struct stream_out *stream, const char *fmt, va_list args){
    int n = 0;
    int flags;
    int field_width;
    int precision;
    enum base_type base;
    enum format_qualifier qualifier;
    unsigned long long num;
    double double_num;
    const char *fmt_start;
    for(; *fmt; fmt++){
        if(*fmt != '%'){
            streamout_in_byte(stream, *fmt);
            n++;
            continue;
        }
        fmt_start = fmt;
        flags = 0x00;
        while (1){
            ++fmt;
            if (*fmt == '-'){
                flags |= FORMAT_LEFT;
            }else if (*fmt == '+'){
                flags |= FORMAT_PLUS;
            }else if (*fmt == ' '){
                flags |= FORMAT_SPACE;
            }else if (*fmt == '#'){
                flags |= FORMAT_SPECIAL;
            }else if (*fmt == '0'){
                flags |= FORMAT_ZEROPAD;
            }else{
                break;
            }
        }

        field_width = -1;
        if (isdigit(*fmt)){
            field_width = skip_atoi(&fmt);
        }else if (*fmt == '*'){
            ++fmt;
            field_width = va_arg(args, int);
            if (field_width < 0){
                field_width = -field_width;
                flags |= FORMAT_LEFT;
            }
        }

        precision = -1;
        if (*fmt == '.'){
            ++fmt;
            if (isdigit(*fmt)){
                precision = skip_atoi(&fmt);
            }
            else if (*fmt == '*'){
                ++fmt;
                precision = va_arg(args, int);
            }
            if (precision < 0){
                precision = 0;
            }
        }

        qualifier = FORMAT_QUALIFIER_NONE;
        if(*fmt == 'h'){
            fmt++;
            qualifier = FORMAT_QUALIFIER_SHORT;
            if(*fmt == 'h'){
                fmt++;
                qualifier = FORMAT_QUALIFIER_CHAR;
            }
        }else if(*fmt == 'l'){
            fmt++;
            qualifier = FORMAT_QUALIFIER_LONG;
            if(*fmt == 'l'){
                fmt++;
                qualifier = FORMAT_QUALIFIER_LONG_LONG;
            }
        }else if(*fmt == 'L'){
            fmt++;
            qualifier = FORMAT_QUALIFIER_LONG_LONG;
        }else if(*fmt == 'z'){
            fmt++;
            qualifier = FORMAT_QUALIFIER_SIZE_T;
        }

        base = BASE_TYPE_DEC;

        switch (*fmt){
            case 's':{
                n += vprintf_string(stream, va_arg(args, char *), field_width, precision, flags);
                continue;
            }
            case 'd':
                /*FALLTHROUGH*/
            case 'i':
                flags |= FORMAT_SIGNED;
            case 'u':
                goto _print_number;
            case 'X':
                flags |= FORMAT_LARGE;
			    /*FALLTHROUGH*/
            case 'x':
                base = BASE_TYPE_HEX;
                goto _print_number;
            case 'c':{
                n += vprintf_char(stream, (char)va_arg(args, int), field_width, flags);
                continue;
            }
            case '%':{
                streamout_in_byte(stream, '%');
                n++;
                continue;
            }
            case 'B':
                flags |= FORMAT_LARGE;
                /*FALLTHROUGH*/
            case 'b':{
                base = BASE_TYPE_BIN;
                goto _print_number;
            }
            case 'o':{
                base = BASE_TYPE_OCT;
                goto _print_number;
            }
            case 'p':{
                if(field_width < 0){
                    field_width = (sizeof(void *) << 1) + 2;
                    flags |= FORMAT_ZEROPAD | FORMAT_SPECIAL;
                }
                n += vprintf_number(stream, (unsigned long)va_arg(args, void *), field_width, precision, flags, BASE_TYPE_HEX);
                continue;
            }
            case 'E':
                flags |= FORMAT_LARGE;
                /*FALLTHROUGH*/
            case 'e':
                flags |= FORMAT_FLOAT_E;
                goto _print_folat;
            case 'G':
                flags |= FORMAT_LARGE;
                /*FALLTHROUGH*/
            case 'g':
                flags |= FORMAT_FLOAT_G;
                goto _print_folat;
            case 'F':
                flags |= FORMAT_LARGE;
                /*FALLTHROUGH*/
            case 'f':
                flags |= FORMAT_FLOAT_F;
                goto _print_folat;
            default:{
                streamout_in_byte(stream, '%');
                fmt = fmt_start;
                continue;
            }

        }
        continue;
    
    _print_number:
        {
            switch(qualifier){
                case FORMAT_QUALIFIER_LONG:{
                    /* long */
                    num = (flags & FORMAT_SIGNED) ? 
                        (unsigned long long)va_arg(args, signed long) : 
                        (unsigned long long)va_arg(args, unsigned long);
                    break;
                }
                case FORMAT_QUALIFIER_LONG_LONG:{
                    /* long long */
                    num = (flags & FORMAT_SIGNED) ? 
                        (unsigned long long)va_arg(args, signed long long) : 
                        (unsigned long long)va_arg(args, unsigned long long);
                    break;
                }
                case FORMAT_QUALIFIER_SHORT:{
                    /* short */
                    num = (flags & FORMAT_SIGNED) ? 
                        (unsigned long long)((signed short)va_arg(args, int)) : 
                        (unsigned long long)((unsigned short)va_arg(args, int));
                    break;
                }
                case FORMAT_QUALIFIER_CHAR:{
                    /* short */
                    num = (flags & FORMAT_SIGNED) ? 
                        (unsigned long long)((signed char)va_arg(args, int)) : 
                        (unsigned long long)((unsigned char)va_arg(args, int));
                    break;
                }
                case FORMAT_QUALIFIER_SIZE_T:{
                    /* short */
                    num = (flags & FORMAT_SIGNED) ? 
                        (unsigned long long)va_arg(args, ssize_t) : 
                        (unsigned long long)va_arg(args, size_t);
                    break;
                }
                default:{
                    /* int */
                    num = (flags & FORMAT_SIGNED) ? 
                        (unsigned long long)va_arg(args, signed int) : 
                        (unsigned long long)va_arg(args, unsigned int);
                    break;
                }
            }
            n += vprintf_number(stream, num, field_width, precision, flags, base);
            continue;
        }
    _print_folat:
        {   
            /* 目前不支持 long double */
            if(FORMAT_QUALIFIER_LONG_LONG == qualifier){
                continue;
            }
            //double_num = (qualifier == FORMAT_QUALIFIER_LONG_LONG)? va_arg(args, long double) : va_arg(args, double);
            double_num = va_arg(args, double);
            n += vprintf_float(stream, double_num, field_width, precision, flags);
            continue;
        }
    }
    return n;
}



int eh_vsnprintf(char *buf, size_t size, const char *fmt, va_list args){
    struct stream_out stream = {
        .type = STREAM_TYPE_MEMORY,
        .m = {
            .buf = (uint8_t*)buf,
            .pos = (uint8_t*)buf,
            .end = (uint8_t*)buf + size,
        },
    };
    return eh_stream_vprintf(&stream, fmt, args);
}


int eh_snprintf(char *buf, size_t size, const char *fmt, ...){
    int n;
    va_list args;
    va_start(args, fmt);
    n = eh_vsnprintf(buf, size, fmt, args);
    va_end(args);
    return n;
}

int eh_printf(const char *fmt, ...){
    int n;
    va_list args;
    va_start(args, fmt);
    n = eh_stream_vprintf(&_stdout, fmt, args);
    va_end(args);
    streamout_finish(&_stdout);
    return n;
}