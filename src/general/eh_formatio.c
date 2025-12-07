/**
 * @file eh_formatio.c
 * @brief  Implementation of standard io
 * @author simon.xiaoapeng (simon.xiaoapeng@gmail.com)
 * @date 2024-07-06
 * 
 * @copyright Copyright (c) 2024  simon.xiaoapeng@gmail.com
 * 
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>
#include <ctype.h>
#include <stdio.h>
#include <float.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <eh_mem.h>
#include <eh_types.h>
#include <eh_formatio.h>
#include <eh_config.h>
#include <eh_swab.h>

#define FORMAT_FLOAT_F_RANGE_MAX        (1.e+18)
#define FORMAT_FLOAT_F_RANGE_MIN        (-(1.e+18))
#define FORMAT_FLOAT_POWERS_TAB_SIZE    19
#define FORMAT_STACK_CACHE_SIZE         (16)
#define FORMAT_LOG10_TAYLOR_TERMS       (4)
#define FORMAT_DBL_EXP_OFFSET           (1023)
#define FORMAT_DBL_EXP_OFFSET           (1023)
#define FORMAT_DBL_MIN_POW10            (1.e-308)

#define FORMAT_LEFT         0x00000001          /* 输出结果左对齐 */
#define FORMAT_PLUS         0x00000002          /* 输出结果若为正数则添加正号 */
#define FORMAT_SPACE        0x00000004          /* 输出结果若为正数则添加空格 */
#define FORMAT_SPECIAL      0x00000008          /* 特殊标志位，用于添加一些特定符号，比如%#x自动添加0x */
#define FORMAT_ZEROPAD      0x00000010          /* 当宽度大于输出结果长度时，则用0填充 */
#define FORMAT_LARGE        0x00000020          /* 使用大写输出 */
#define FORMAT_SIGNED       0x00000040          /* 有符号输出 */
#define FORMAT_FLOAT_E      0x00000080          /* 浮点数输出  E格式*/
#define FORMAT_FLOAT_F      0x00000100          /* 浮点数输出  F格式*/
#define FORMAT_FLOAT_G      0x00000200          /* 浮点数输出  G格式*/



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
};



static const char small_digits[] = "0123456789abcdef";
static const char large_digits[] = "0123456789ABCDEF";
static const double powers_of_10[FORMAT_FLOAT_POWERS_TAB_SIZE] = {
  1e00, 1e01, 1e02, 1e03, 1e04, 1e05, 1e06, 1e07, 1e08,
  1e09, 1e10, 1e11, 1e12, 1e13, 1e14, 1e15, 1e16, 1e17, 1e18
};
static uint8_t _stdout_cache[EH_CONFIG_STDOUT_MEM_CACHE_SIZE];

__weak void stdout_write(void *stream, const uint8_t *buf, size_t size){
    (void)stream;
    (void)buf;
    (void)size;
}

struct stream_function _eh_stdout = {
    .base = {
        .type = STREAM_TYPE_FUNCTION,
    },
    .write = stdout_write,
    .cache = _stdout_cache,
    .pos = _stdout_cache,
    .end = _stdout_cache + EH_CONFIG_STDOUT_MEM_CACHE_SIZE,
};
struct double_components {
  uint64_t              integral;
  uint64_t              fractional;
  bool                  is_negative;
};

union double_union{
    double d;
    struct{
        uint64_t mantissa:52;
        uint64_t exponent:11;
        uint64_t sign:1;
    };
    struct{
        uint64_t v64;
    };
};

struct scaling_factor {
    double raw_factor;
    bool multiply; // if true, need to multiply by raw_factor; otherwise need to divide by it
};

static int bastardized_floor(double x)
{
    int n;
    if (x >= 0) { return (int) x; }
    n = (int) x;
    return ( ((double) n) == x ) ? n : n-1;
}

static double pow10_of_int(int floored_exp10)
{
    union double_union du = {.v64 = 0};
    int exp2;
    if (floored_exp10 == -DBL_MAX_10_EXP ) {
        return FORMAT_DBL_MIN_POW10;
    }
    exp2 = bastardized_floor(floored_exp10 * 3.321928094887362 + 0.5);
    const double z  = floored_exp10 * 2.302585092994046 - exp2 * 0.6931471805599453;
    const double z2 = z * z;
    du.exponent = (uint16_t)(((exp2 ) + FORMAT_DBL_EXP_OFFSET) & 0x7FF);
    du.d *= 1 + 2 * z / (2 - z + (z2 / (6 + (z2 / (10 + z2 / 14)))));
    return du.d;
}



static double log10_of_positive(double positive_number){
    union double_union du = {.d = positive_number};
    int exp2 = du.exponent - FORMAT_DBL_EXP_OFFSET;
    double z;
    du.exponent = FORMAT_DBL_EXP_OFFSET;
    z = du.d - 1.5;
    return (
    // Taylor expansion around 1.5:
    0.1760912590556812420           // Expansion term 0: ln(1.5)            / ln(10)
    + z     * 0.2895296546021678851 // Expansion term 1: (M - 1.5)   * 2/3  / ln(10)
#if FORMAT_LOG10_TAYLOR_TERMS > 2
    - z*z   * 0.0965098848673892950 // Expansion term 2: (M - 1.5)^2 * 2/9  / ln(10)
#if FORMAT_LOG10_TAYLOR_TERMS > 3
    + z*z*z * 0.0428932821632841311 // Expansion term 2: (M - 1.5)^3 * 8/81 / ln(10)
#endif
#endif
    // exact log_2 of the exponent x, with logarithm base change
    + exp2 * 0.30102999566398119521 // = exp2 * log_10(2) = exp2 * ln(2)/ln(10)
  );
}

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

static inline void streamout_in_byte(struct stream_base *stream, char ch){
    bool out = false;
    switch(stream->type){
        case STREAM_TYPE_FUNCTION:{
            struct stream_function *f = (struct stream_function *)stream;
            if(f->pos < f->end){
                *f->pos = (uint8_t)ch;
                f->pos++;
                if(ch == '\n') out = true;
            }
            if( f->pos == f->end || out ){
                f->write(stream, f->cache,  (size_t)(f->pos - f->cache));
                f->pos = f->cache;
            }
            break;
        }
        case STREAM_TYPE_FUNCTION_NO_CACHE:{
            struct stream_function_no_cache *f = (struct stream_function_no_cache *)stream;
            f->write(stream, (uint8_t*)&ch, 1);
            break;
        }
        case STREAM_TYPE_MEMORY:{
            struct stream_memory *m = (struct stream_memory *)stream;
            if(m->pos < m->end){
                *m->pos = (uint8_t)ch;
                m->pos++;
            }
            break;
        }
    }
}

static inline void streamout_finish(struct stream_base *stream){
    switch(stream->type){
        case STREAM_TYPE_FUNCTION:{
            struct stream_function *f = (struct stream_function *)stream;
            if(f->pos > f->cache){
                f->write(stream, f->cache,  (size_t)(f->pos - f->cache));
                f->pos = f->cache;
            }
            break;
        }
        case STREAM_TYPE_FUNCTION_NO_CACHE:{
            struct stream_function_no_cache *f = (struct stream_function_no_cache *)stream;
            f->finish(stream);
            break;
        }
        case STREAM_TYPE_MEMORY:{
            struct stream_memory *m = (struct stream_memory *)stream;
            if(m->pos < m->end){
                *m->pos = '\0';
            }else{
                *(m->end - 1) = '\0';
            }
            break;
        }
    }
}

static inline int skip_atoi(const char **s)
{
    int i = 0;
    while (isdigit((int)(**s)))
        i = i * 10 + (*((*s)++) - '0');
    return i;
}

static inline int vprintf_char(struct stream_base *stream, char ch, int field_width, int flags){
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

static inline int vprintf_string(struct stream_base *stream, char *s, int field_width, int precision, int flags){
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

static double apply_scaling(double num, struct scaling_factor normalization)
{
    return normalization.multiply ? num * normalization.raw_factor : num / normalization.raw_factor;
}

static double unapply_scaling(double normalized, struct scaling_factor normalization)
{
    return normalization.multiply ? normalized / normalization.raw_factor : normalized * normalization.raw_factor;
}

static struct scaling_factor update_normalization(struct scaling_factor sf, double extra_multiplicative_factor)
{
    struct scaling_factor result;
    if (sf.multiply) {
        result.multiply = true;
        result.raw_factor = sf.raw_factor * extra_multiplicative_factor;
    }
    else {
        union double_union du = {.d = sf.raw_factor};
        int factor_exp2 = du.exponent - FORMAT_DBL_EXP_OFFSET;
        du.d = extra_multiplicative_factor;
        int extra_factor_exp2 = du.exponent - FORMAT_DBL_EXP_OFFSET;;

        // Divide the larger-exponent raw raw_factor by the smaller
        if (abs(factor_exp2) > abs(extra_factor_exp2)) {
            result.multiply = false;
            result.raw_factor = sf.raw_factor / extra_multiplicative_factor;
        }else {
            result.multiply = true;
            result.raw_factor = extra_multiplicative_factor / sf.raw_factor;
        }
    }
    return result;
}


void float_decentralized(double num, struct double_components *components, int precision) 
{
    union double_union du = {.d = num};
    double abs_number;
    double remainder;
    components->is_negative = du.sign;
    abs_number = (components->is_negative) ? -num : num;
    components->integral = (uint64_t)abs_number;

    if(precision >= FORMAT_FLOAT_POWERS_TAB_SIZE)
        precision = FORMAT_FLOAT_POWERS_TAB_SIZE - 1;
    remainder = (abs_number - (double) components->integral) * powers_of_10[precision];
    components->fractional = (uint64_t)remainder;

    remainder -= (double) components->fractional;

    /* 银行家舍入法， 四舍六入五取偶 */
    if(remainder > 0.5){
        ++components->fractional;
        /* 检查是否溢出到整数位去了 */
        if ((double) components->fractional >= powers_of_10[precision]) {
            components->fractional = 0;
            ++components->integral;
        }
    }else if ((remainder == 0.5) && ((components->fractional == 0U) || (components->fractional & 1U))) {
        ++components->fractional;
    }

    if (precision == 0U) {
        remainder = abs_number - (double) components->integral;
        if ((!(remainder < 0.5) || (remainder > 0.5)) && (components->integral & 1)) {
            ++components->integral;
        }
    }
    return ;
}

static void float_normalized_decentralized(
   struct double_components *components,
   bool negative, int precision, double non_normalized, 
   struct scaling_factor normalization, int floored_exp10)
{
    double scaled = apply_scaling(non_normalized, normalization);
    bool close_to_representation_extremum = ( (-floored_exp10 + (int) precision) >= DBL_MAX_10_EXP - 1 );
    double remainder;
    double prec_power_of_10;
    struct scaling_factor account_for_precision;
    double scaled_remainder;
    double rounding_threshold;
    
    components->is_negative = negative;

    if(precision >= FORMAT_FLOAT_POWERS_TAB_SIZE)
        precision = FORMAT_FLOAT_POWERS_TAB_SIZE - 1;

    if (close_to_representation_extremum) {
        // We can't have a normalization factor which also accounts for the precision, i.e. moves
        // some decimal digits into the mantissa, since it's unrepresentable, or nearly unrepresentable.
        // So, we'll give up early on getting extra precision...
        float_decentralized(negative ? -scaled : scaled, components, precision);
        return ;
    }
    components->integral = (uint64_t) scaled;
    remainder = non_normalized - unapply_scaling((double) components->integral, normalization);
    prec_power_of_10 = powers_of_10[precision];
    account_for_precision = update_normalization(normalization, prec_power_of_10);
    scaled_remainder = apply_scaling(remainder, account_for_precision);
    rounding_threshold = 0.5;

    components->fractional = (uint64_t) scaled_remainder; // when precision == 0, the assigned value should be 0
    scaled_remainder -= (double) components->fractional; //when precision == 0, this will not change scaled_remainder

    components->fractional += (scaled_remainder >= rounding_threshold);
    if (scaled_remainder == rounding_threshold) {
        // banker's rounding: Round towards the even number (making the mean error 0)
        components->fractional &= ~((uint64_t) 0x1);
    }
    // handle rollover, e.g. the case of 0.99 with precision 1 becoming (0,100),
    // and must then be corrected into (1, 0).
    // Note: for precision = 0, this will "translate" the rounding effect from
    // the fractional part to the integral part where it should actually be
    // felt (as prec_power_of_10 is 1)
    if ((double) components->fractional >= prec_power_of_10) {
        components->fractional = 0;
        ++components->integral;
    }

}

static int vprintf_float_decimalism_or_normalized(struct stream_base *stream, struct double_components *components_num, 
    int field_width, int precision, int flags, int floored_exp10){
    char _number_buf[FORMAT_STACK_CACHE_SIZE];
    char *number_buf = _number_buf;
    char sign = 0;
    char dot = 0;
    char exponent_sign_e = 0;
    char exponent_sign = 0;
    int n = 0;
    int need_buf_min_size = 0;
    int integral_valid_len = 0;
    int fractional_pad_len = 0;
    int fractional_valid_len = 0;
    int fractional_precision_pad_len = 0;
    int exponent_valid_len = 0;
    int valid_len = 0;
    int space_or_zero_pad_len = 0;
    unsigned long long number = 0;
    const char *digits = small_digits;

    if(precision >= FORMAT_FLOAT_POWERS_TAB_SIZE){
        fractional_precision_pad_len = precision - (FORMAT_FLOAT_POWERS_TAB_SIZE - 1);
        precision = FORMAT_FLOAT_POWERS_TAB_SIZE - 1;
    }

    if(components_num->is_negative){
        sign = '-';
    }else if(flags & FORMAT_PLUS){
        sign = '+';
    }else if(flags & FORMAT_SPACE){
        sign = ' ';
    }
    if(precision > 0 || flags & FORMAT_SPECIAL){
        dot = '.';
    }

    if(flags & FORMAT_FLOAT_E){
        exponent_sign_e = flags & FORMAT_LARGE ? 'E' : 'e';
        exponent_sign = floored_exp10 >= 0 ? '+' : '-';
        floored_exp10 = abs(floored_exp10);
        exponent_valid_len = num_bit_count((unsigned long long)floored_exp10, BASE_TYPE_DEC);
    }


    integral_valid_len = num_bit_count(components_num->integral, BASE_TYPE_DEC);
    if(precision > 0){
        fractional_valid_len = num_bit_count(components_num->fractional, BASE_TYPE_DEC);
        if(precision > fractional_valid_len)
            fractional_pad_len = precision - fractional_valid_len;
    }

    need_buf_min_size = integral_valid_len > fractional_valid_len ? 
        integral_valid_len : fractional_valid_len;
    need_buf_min_size = need_buf_min_size > exponent_valid_len ?
        need_buf_min_size : exponent_valid_len;

    if(need_buf_min_size > FORMAT_STACK_CACHE_SIZE){
        number_buf = eh_malloc((size_t)need_buf_min_size);
    }

    if(flags & FORMAT_FLOAT_E){
        valid_len = (sign ? 1 : 0) + integral_valid_len + 
                    (dot ? 1 : 0) + fractional_pad_len + fractional_valid_len + fractional_precision_pad_len +
                    (exponent_sign_e ? 1 : 0) + (exponent_sign ? 1 : 0) + 
                    ((exponent_valid_len < 2) ? 2 : exponent_valid_len);
    }else{
        valid_len = (sign ? 1 : 0) + integral_valid_len + 
                    (dot ? 1 : 0) + fractional_pad_len + fractional_valid_len + fractional_precision_pad_len;
    }
    
    if(field_width > valid_len)
        space_or_zero_pad_len = field_width - valid_len;

    /* 右对齐填充 */
    if(!(flags & FORMAT_LEFT) && !(flags & FORMAT_ZEROPAD)){
        for(int i=0; i<space_or_zero_pad_len; i++,n++)
            streamout_in_byte(stream, ' ');
    }

    /* 符号位 */
    if(sign){
        streamout_in_byte(stream, sign);
        n++;
    }
    
    /* 0填充 */
    if(flags & FORMAT_ZEROPAD){
        for(int i=0; i<space_or_zero_pad_len; i++,n++)
            streamout_in_byte(stream, '0');
    }

    /* 整数部分 */
    number = (unsigned long long)components_num->integral;
    for(int i=(integral_valid_len-1); i>=0; i--){
        number_buf[i] = digits[num_rsh(&number, (int)BASE_TYPE_DEC)];
    }

    for(int i=0; i<integral_valid_len; i++,n++){
        streamout_in_byte(stream, number_buf[i]);
    }

    /* dot打印 */
    if(dot){
        streamout_in_byte(stream, dot);
        n++;
    }

    /* 小数部分0填充 */
    if(fractional_pad_len > 0){
        for(int i=0; i<fractional_pad_len; i++,n++)
            streamout_in_byte(stream, '0');
    }

    /* 小数部分 */
    number = (unsigned long long)components_num->fractional;
    for(int i=(fractional_valid_len-1); i>=0; i--){
        number_buf[i] = digits[num_rsh(&number, (int)BASE_TYPE_DEC)];
    }
    
    for(int i=0; i<fractional_valid_len; i++,n++){
        streamout_in_byte(stream, number_buf[i]);
    }

    /* 精度不足部分补0 */
    if(fractional_precision_pad_len > 0){
        for(int i=0; i<fractional_precision_pad_len; i++,n++)
            streamout_in_byte(stream, '0');
    }

    /* 指数部分 */
    if(flags & FORMAT_FLOAT_E){       
        streamout_in_byte(stream, exponent_sign_e);
        streamout_in_byte(stream, exponent_sign);
        n += 2;

        number = (unsigned long long)floored_exp10;
        for(int i=(exponent_valid_len-1); i>=0; i--){
            number_buf[i] = digits[num_rsh(&number, (int)BASE_TYPE_DEC)];
        }
        if(exponent_valid_len < 2){
            streamout_in_byte(stream, '0');
            n++;
        }
        for(int i=0; i<exponent_valid_len; i++,n++){
            streamout_in_byte(stream, number_buf[i]);
        }
    }

    /* 左对齐时填充 */
    if(flags & FORMAT_LEFT && !(flags & FORMAT_ZEROPAD)){
        for(int i=0; i<space_or_zero_pad_len; i++,n++)
            streamout_in_byte(stream, ' ');
    }
    if(number_buf != _number_buf)
        eh_free(number_buf);

    return n;
}

static int vprintf_float_e(struct stream_base *stream, double num, int field_width, int precision, int flags){
    union double_union du = {.d = num};
    double abs_number =  du.sign ? -num : num;
    int floored_exp10;
    bool abs_exp10_covered_by_powers_table = false;
    struct double_components components;
    struct scaling_factor normalization = {0};
    
    if(precision < 0)
        precision = 6;

    if(abs_number == 0.0){
        floored_exp10 = 0;
    }else{
        double exp10 = log10_of_positive(abs_number);
        floored_exp10 = bastardized_floor(exp10);
        double p10 = pow10_of_int(floored_exp10);
        if (abs_number < p10) {
            floored_exp10--;
            p10 /= 10;
        }
        abs_exp10_covered_by_powers_table = abs(floored_exp10) < FORMAT_FLOAT_POWERS_TAB_SIZE;
        normalization.raw_factor = abs_exp10_covered_by_powers_table ? powers_of_10[abs(floored_exp10)] : p10;
    }
    /* 只有使用powers_of_10表才有可能用到乘法 */
    normalization.multiply = (floored_exp10 < 0 && abs_exp10_covered_by_powers_table);
    float_normalized_decentralized(&components, du.sign, precision, abs_number, normalization, floored_exp10);
    return vprintf_float_decimalism_or_normalized(stream, &components, field_width, precision, flags, floored_exp10);
    return 0;
}

static inline int vprintf_float_f_or_g(struct stream_base *stream, double num, int field_width, int precision, int flags){
    struct double_components components_num;
    if(num < FORMAT_FLOAT_F_RANGE_MIN || num > FORMAT_FLOAT_F_RANGE_MAX)
        return 0;
    if(precision < 0)
        precision = 6;
    float_decentralized(num, &components_num, precision);
    return vprintf_float_decimalism_or_normalized(stream, &components_num, field_width, precision, flags, 0);
}

static int vprintf_float(struct stream_base *stream, double num, int field_width, int precision, int flags){
    int n=0;
    if(isinf(num) || isnan(num)){
        char *out_str = NULL;
        if(isinf(num)){
            if(num < 0){
                out_str = flags & FORMAT_LARGE ? "-INF":"-inf";
            }else{
                if(flags & FORMAT_PLUS){
                    out_str = flags & FORMAT_LARGE ? "+INF":"+inf";
                }else if(flags & FORMAT_SPACE){
                    out_str = flags & FORMAT_LARGE ? " INF":" inf";
                }else{
                    out_str = flags & FORMAT_LARGE ? "INF":"inf";
                }
            }
        }else if(isnan(num)){
            out_str = flags & FORMAT_LARGE ? "NAN":"nan";
        }
        n += vprintf_string(stream, out_str, field_width, -1, flags);
        return n;
    }
    if(flags & FORMAT_FLOAT_F || flags & FORMAT_FLOAT_G){
        n += vprintf_float_f_or_g(stream, num, field_width, precision, flags);
        if(n > 0) return n;
    }
    
    flags &= (~(FORMAT_FLOAT_F|FORMAT_FLOAT_G));
    flags |= FORMAT_FLOAT_E;
    return vprintf_float_e(stream, num, field_width, precision, flags);
}

static int vprintf_array(struct stream_base *stream, const uint8_t *array, int field_width, 
    int precision, int flags, enum format_qualifier qualifier){
    const char *digits = small_digits;
    const uint8_t *item;
    char item_size;
    int valid_len;
    int array_len;
    int array_reality_len;
    int remainder;
    int space_pad_len = 0;
    int n = 0;

    if(precision < 0) return 0;

    if(flags & FORMAT_LARGE)
        digits = large_digits;
    switch(qualifier){
        case FORMAT_QUALIFIER_LONG:
            item_size = sizeof(unsigned long);
            break;
        case FORMAT_QUALIFIER_LONG_LONG:
            item_size = sizeof(unsigned long long);
            break;
        case FORMAT_QUALIFIER_SHORT:
            item_size = sizeof(unsigned short);
            break;
        case FORMAT_QUALIFIER_CHAR:
            item_size = sizeof(unsigned char);
            break;
        default:
            item_size = sizeof(unsigned int);
            break;
    }
    array_len = precision/item_size;
    remainder = precision % item_size;
    array_reality_len = array_len - (remainder ? 1 : 0);
    valid_len = (array_reality_len * (item_size * 2)) + (array_reality_len - 1);
    if(field_width > valid_len){
        space_pad_len = field_width - valid_len;
    }
    
    if(!(flags & FORMAT_LEFT)){
        for(int i=0; i<space_pad_len; i++,n++){
            streamout_in_byte(stream, ' ');
        }
    }

    item = array;
    for(int i=0; i<array_len; i++, item += item_size){
        if(i){
            streamout_in_byte(stream, ' ');
            n++;
        }
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
        for(int j=item_size - 1; j>=0; j--, n+=2){
#else
        for(int j=0; j<item_size; j++, n+=2){
#endif
            streamout_in_byte(stream, digits[(item[j] >> 4) & 0x0f]);
            streamout_in_byte(stream, digits[     (item[j]) & 0x0f]);
        }
    }
    if(remainder){
        streamout_in_byte(stream, ' ');
        n++;
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
        for(int i=0;i<(item_size-remainder);i++,n+=2){
            streamout_in_byte(stream, '?');
            streamout_in_byte(stream, '?');
        }
        for(int j=remainder - 1; j>=0; j--, n+=2){
            streamout_in_byte(stream, digits[(item[j] >> 4) & 0x0f]);
            streamout_in_byte(stream, digits[     (item[j]) & 0x0f]);
        }
#else
        for(int j=0; j<remainder; j++, n+=2){
            streamout_in_byte(stream, digits[(item[j] >> 4) & 0x0f]);
            streamout_in_byte(stream, digits[     (item[j]) & 0x0f]);
        }
        for(int i=0;i<(item_size-remainder);i++,n+=2){
            streamout_in_byte(stream, '?');
            streamout_in_byte(stream, '?');
        }
#endif
    }

    if(flags & FORMAT_LEFT){
        for(int i=0; i<space_pad_len; i++,n++){
            streamout_in_byte(stream, ' ');
        }
    }
    return n;
}
static inline int vprintf_number(struct stream_base *stream, unsigned long long num, int field_width, int precision, int flags, enum base_type base){
    char _number_buf[FORMAT_STACK_CACHE_SIZE];
    char *number_buf = _number_buf;
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
    if(bit_count > FORMAT_STACK_CACHE_SIZE){
        number_buf = eh_malloc((size_t)bit_count);
        if(number_buf == NULL)  return 0;
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

static int streamout_vprintf(struct stream_base *stream, const char *fmt, va_list args){
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
        if (isdigit((int)(*fmt))){
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
            if (isdigit((int)(*fmt))){
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
                _fallthrough;
            case 'i':
                flags |= FORMAT_SIGNED;
            case 'u':
                goto _print_number;
            case 'X':
                flags |= FORMAT_LARGE;
                _fallthrough;
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
                _fallthrough;
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
                _fallthrough;
            case 'e':
                flags |= FORMAT_FLOAT_E;
                goto _print_folat;
            case 'G':
                flags |= FORMAT_LARGE;
                _fallthrough;
            case 'g':
                flags |= FORMAT_FLOAT_G;
                goto _print_folat;
            case 'F':
                flags |= FORMAT_LARGE;
                _fallthrough;
            case 'f':
                flags |= FORMAT_FLOAT_F;
                goto _print_folat;
            case 'Q':
                flags |= FORMAT_LARGE;
                _fallthrough;
            case 'q':
                n += vprintf_array(stream, va_arg(args, void *), field_width, precision, flags, qualifier);
                break;
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
    int n;
    struct stream_memory stream;
    eh_stream_memory_init(&stream, (uint8_t*)buf, size);
    n = streamout_vprintf((struct stream_base *)&stream, fmt, args);
    streamout_finish((struct stream_base *)&stream);
    return n;
}


int eh_snprintf(char *buf, size_t size, const char *fmt, ...){
    int n;
    va_list args;
    va_start(args, fmt);
    n = eh_vsnprintf(buf, size, fmt, args);
    va_end(args);
    return n;
}

int eh_sprintf(char *buf, const char *fmt, ...){
    int n;
    va_list args;
    va_start(args, fmt);
    n = eh_vsnprintf(buf, (size_t)((char *)(LONG_MAX) - buf), fmt, args);
    va_end(args);
    return n;
}

int eh_vprintf(const char *fmt, va_list args){
    int n;
    n = streamout_vprintf(EH_STDOUT, fmt, args);
    return n;
}

int eh_printf(const char *fmt, ...){
    int n;
    va_list args;
    va_start(args, fmt);
    n = streamout_vprintf(EH_STDOUT, fmt, args);
    va_end(args);
    return n;
}

int eh_stream_vprintf(struct stream_base *stream, const char *fmt, va_list args){
    int n;
    if(stream == NULL)
        return 0;
    n = streamout_vprintf(stream, fmt, args);
    return n;
}

int eh_stream_printf(struct stream_base *stream, const char *fmt, ...){
    int n;
    va_list args;
    if(stream == NULL)
        return 0;
    va_start(args, fmt);
    n = eh_stream_vprintf(stream, fmt, args);
    va_end(args);
    return n;
}

void eh_stream_putc(struct stream_base *stream, int c){
    if(stream == NULL)
        return ;
    streamout_in_byte(stream, (char)c);
}

int eh_stream_puts(struct stream_base *stream, const char *s){
    const char *p;
    size_t n = 0;
    if(stream == NULL)
        return 0;
    if(stream->type == STREAM_TYPE_FUNCTION_NO_CACHE){
        struct stream_function_no_cache *f = (struct stream_function_no_cache *)stream;
        n = strlen(s);
        f->write(stream, (uint8_t*)s, n);
        return (int)n;
    }
    p = s;
    while(*p)
        streamout_in_byte(stream, (char)*p++);
    return (int)(p - s);
}
void eh_stream_finish(struct stream_base *stream){
    if(stream == NULL)
        return ;
    streamout_finish(stream);
}