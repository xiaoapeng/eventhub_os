/**
 * @file eh_swab.h
 * @brief 字节交换实现
 * @author simon.xiaoapeng (simon.xiaoapeng@gmail.com)
 * @version 1.0
 * @date 2024-10-31
 * 
 * @copyright Copyright (c) 2024  simon.xiaoapeng@gmail.com
 * 
 * @par 修改日志:
 */
#ifndef _EH_SWAB_H_
#define _EH_SWAB_H_

#include <stdint.h>

#include "eh_types.h"


#ifndef __ORDER_LITTLE_ENDIAN__
#define __ORDER_LITTLE_ENDIAN__ 1234
#endif

#ifndef __ORDER_BIG_ENDIAN__
#define __ORDER_BIG_ENDIAN__ 4321
#endif

#ifndef __BYTE_ORDER__
#define __BYTE_ORDER__ __ORDER_LITTLE_ENDIAN__
#endif


#if defined(__GNUC__) || defined(__clang__)
    #define BUILTIN_BSWAP_SUPPORTED
#endif

#ifdef BUILTIN_BSWAP_SUPPORTED
#define eh_swab16(x) (uint16_t)__builtin_bswap16((uint16_t)(x))
#define eh_swab32(x) (uint32_t)__builtin_bswap32((uint32_t)(x))
#define eh_swab64(x) (uint64_t)__builtin_bswap64((uint64_t)(x))
#else
#define _eh_constant_swab16(x) ((uint16_t)(				        \
	(((uint16_t)(x) & (uint16_t)0x00ffU) << 8) |			    \
	(((uint16_t)(x) & (uint16_t)0xff00U) >> 8)))

#define _eh_constant_swab32(x) ((uint32_t)(				        \
	(((uint32_t)(x) & (uint32_t)0x000000ffUL) << 24) |		    \
	(((uint32_t)(x) & (uint32_t)0x0000ff00UL) <<  8) |		    \
	(((uint32_t)(x) & (uint32_t)0x00ff0000UL) >>  8) |		    \
	(((uint32_t)(x) & (uint32_t)0xff000000UL) >> 24)))

#define _eh_constant_swab64(x) ((uint64_t)(				        \
	(((uint64_t)(x) & (uint64_t)0x00000000000000ffULL) << 56) |	\
	(((uint64_t)(x) & (uint64_t)0x000000000000ff00ULL) << 40) |	\
	(((uint64_t)(x) & (uint64_t)0x0000000000ff0000ULL) << 24) |	\
	(((uint64_t)(x) & (uint64_t)0x00000000ff000000ULL) <<  8) |	\
	(((uint64_t)(x) & (uint64_t)0x000000ff00000000ULL) >>  8) |	\
	(((uint64_t)(x) & (uint64_t)0x0000ff0000000000ULL) >> 24) |	\
	(((uint64_t)(x) & (uint64_t)0x00ff000000000000ULL) >> 40) |	\
	(((uint64_t)(x) & (uint64_t)0xff00000000000000ULL) >> 56)))

static inline __attribute_const__ uint16_t _eh_fswab16(uint16_t val){
    return _eh_constant_swab16(val);
}

static inline __attribute_const__ uint32_t _eh_fswab32(uint32_t val){
    return _eh_constant_swab32(val);
}

static inline __attribute_const__ uint64_t _eh_fswab64(uint64_t val){
    return _eh_constant_swab64(val);
}

#define eh_swab16(x) (uint16_t)(                                    \
    __builtin_constant_p(x) ?	                                    \
	_eh_constant_swab16(x) :			                            \
	_eh_fswab16(x)                                                  \
)

#define eh_swab32(x) (uint32_t)(                                    \
    __builtin_constant_p(x) ?	                                    \
	_eh_constant_swab32(x) :			                            \
	_eh_fswab32(x)                                                  \
)   
#define eh_swab64(x) (uint64_t)(                                    \
    __builtin_constant_p(x) ?	                                    \
	_eh_constant_swab64(x) :			                            \
	_eh_fswab64(x)                                                  \
)

#endif

/**
 * 一般指针的size就是long类型的size
 */
#if (__SIZEOF_POINTER__ == 4)
    #define eh_long_swab(x) eh_swab32(x)
#elif (__SIZEOF_POINTER__ == 8)
    #define eh_long_swab(x) eh_swab64(x)
#endif

#define eh_float_swab(x) ({                                         \
    union {                                                         \
        float _f;                                                   \
        uint32_t _u;                                                \
    } _x = {                                                        \
        ._f = (float)(x)                                            \
    };                                                              \
    _x._u = eh_swab32(_x._u);                                       \
    _x._f;                                                          \
})

#define eh_double_swab(x) ({                                        \
    union {                                                         \
        double _d;                                                  \
        uint64_t _u;                                                \
    } _x = {                                                        \
        ._d = (double)(x)                                           \
    };                                                              \
    _x._u = eh_swab64(_x._u);                                       \
    _x._d;                                                          \
})

#define eh_complex_swab(x) ({                                       \
    typeof(x) _x = (x);                                             \
    uint8_t *p = (uint8_t *)&_x;                                    \
    for(size_t i=0; i<(sizeof(_x))/2; i++){                         \
        p[(sizeof(_x))-i-1] ^= p[i];                                \
        p[i] ^=p[(sizeof(_x))-i-1];                                 \
        p[(sizeof(_x))-i-1] ^= p[i];                                \
	};                                                              \
    _x;                                                             \
})

#define eh_swab(x)      _Generic((x),                               \
    char: (x),                                                      \
    unsigned char: (x),                                             \
    short: (short)eh_swab16(x),                                     \
    unsigned short: (unsigned short)eh_swab16(x),                   \
    int: (int)eh_swab32(x),                                         \
    unsigned int: (unsigned int)eh_swab32(x),                       \
    long: (long)eh_long_swab(x),                                    \
    unsigned long: (unsigned long)eh_long_swab(x),                  \
    long long: (long long)eh_swab64(x),                             \
    unsigned long long: (unsigned long long)eh_swab64(x),           \
    float: eh_float_swab(x),                                        \
    double: eh_double_swab(x)                                       \
)


#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define eh_cpu_to_be16(x)               eh_swab16(x)
#define eh_cpu_to_be32(x)               eh_swab32(x)
#define eh_cpu_to_be64(x)               eh_swab64(x)
#define eh_cpu_to_be(x)                 eh_swab(x)
#define eh_cpu_to_le16(x)               (x)
#define eh_cpu_to_le32(x)               (x)
#define eh_cpu_to_le64(x)               (x)
#define eh_cpu_to_le(x)                 (x)

#define eh_be16_to_cpu(x)               eh_swab16(x)
#define eh_be32_to_cpu(x)               eh_swab32(x)
#define eh_be64_to_cpu(x)               eh_swab64(x)
#define eh_be_to_cpu(x)                 eh_swab(x)
#define eh_le16_to_cpu(x)               (x)
#define eh_le32_to_cpu(x)               (x)
#define eh_le64_to_cpu(x)               (x)
#define eh_le_to_cpu(x)                 (x)

#else
#define eh_cpu_to_be16(x)               (x)
#define eh_cpu_to_be32(x)               (x)
#define eh_cpu_to_be64(x)               (x)
#define eh_cpu_to_be(x)                 (x)
#define eh_cpu_to_le16(x)               eh_swab16(x)
#define eh_cpu_to_le32(x)               eh_swab32(x)
#define eh_cpu_to_le64(x)               eh_swab64(x)
#define eh_cpu_to_le(x)                 eh_swab(x)

#define eh_be16_to_cpu(x)               (x)
#define eh_be32_to_cpu(x)               (x)
#define eh_be64_to_cpu(x)               (x)
#define eh_be_to_cpu(x)                 (x)
#define eh_le16_to_cpu(x)               eh_swab16(x)
#define eh_le32_to_cpu(x)               eh_swab32(x)
#define eh_le64_to_cpu(x)               eh_swab64(x)
#define eh_le_to_cpu(x)                 eh_swab(x)
#endif


#define eh_ntoh16(x)    eh_be16_to_cpu(x)
#define eh_ntoh32(x)    eh_be32_to_cpu(x)
#define eh_ntoh64(x)    eh_be64_to_cpu(x)
#define eh_ntoh(x)      eh_be_to_cpu(x)
#define eh_hton16(x)    eh_cpu_to_be16(x)
#define eh_hton32(x)    eh_cpu_to_be32(x)
#define eh_hton64(x)    eh_cpu_to_be64(x)
#define eh_hton(x)      eh_cpu_to_be(x)





#endif // _EH_SWAB_H_