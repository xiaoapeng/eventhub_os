/**
 * @file eh_types.h
 * @brief 
 * @author simon.xiaoapeng (simon.xiaoapeng@gmail.com)
 * @version 1.0
 * @date 2024-05-12
 * 
 * @copyright Copyright (c) 2024  simon.xiaoapeng@gmail.com
 * 
 * @par 修改日志:
 */

#ifndef _EH_TYPES_H_
#define _EH_TYPES_H_


#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */


#ifndef EH_SECTION
    #if defined(__CC_ARM) || defined(__CLANG_ARM)
        #define EH_SECTION(__section__)                __attribute__((section(__section__)))
    #elif defined (__IAR_SYSTEMS_ICC__)
        #define EH_SECTION(__section__)                @ __section__
    #elif defined(__GNUC__)
        #define EH_SECTION(__section__)                __attribute__((section(__section__)))
    #else
        #define EH_SECTION(__section__)
    #endif
#endif

#ifndef EH_USED
    #if defined(__CC_ARM) || defined(__CLANG_ARM)
        #define EH_USED                                 __attribute__((used))
    #elif defined (__IAR_SYSTEMS_ICC__)
        #define EH_USED                                 __root
    #elif defined(__GNUC__)
        #define EH_USED                                 __attribute__((used))
    #else
        #define EH_USED
    #endif
#endif

#ifndef EH_SECTION_BEGIN
    #if defined(__CC_ARM) || defined(__CLANG_ARM)
        #define EH_SECTION_BEGIN(__section__)           (&__section__##$$Base)
    #elif defined (__IAR_SYSTEMS_ICC__)
        #define EH_SECTION_BEGIN(__section__)           (__section_begin(#__section__))
    #elif defined(__GNUC__)
        #define EH_SECTION_BEGIN(__section__)           ({   \
            extern char __start_##__section__[];             \
            (void *)__start_##__section__;                   \
        })
    #else
        #define EH_SECTION_BEGIN(__section__)
    #endif
#endif

#ifndef EH_SECTION_END
    #if defined(__CC_ARM) || defined(__CLANG_ARM)
        #define EH_SECTION_END(__section__)             (&__section__##$$Limit)
    #elif defined (__IAR_SYSTEMS_ICC__)
        #define EH_SECTION_END(__section__)             (__section_end(#__section__))
    #elif defined(__GNUC__)
        #define EH_SECTION_END(__section__)             ({  \
            extern char __stop_##__section__[];              \
            (void *)__stop_##__section__;                    \
        })
    #else
        #define EH_SECTION_END(__section__)
    #endif
#endif


#define EH_STRINGIFY(x) #x

#undef offsetof
#define offsetof(TYPE, MEMBER)	        __builtin_offsetof(TYPE, MEMBER)
#define typeof_member(T, m)	            typeof(((T*)0)->m)
#define __same_type(a, b)               __builtin_types_compatible_p(typeof(a), typeof(b))
#define static_assert(expr, ...)        __static_assert(expr, ##__VA_ARGS__, #expr)
#define __static_assert(expr, msg, ...) _Static_assert(expr, msg)
# define likely(x)	                    __builtin_expect(!!(x), 1)
# define unlikely(x)	                __builtin_expect(!!(x), 0)

#define container_of(ptr, type, member) ({				\
	void *__mptr = (void *)(ptr);					\
	static_assert(__same_type(*(ptr), ((type *)0)->member) ||	\
		      __same_type(*(ptr), void),			\
		      "pointer type mismatch in container_of()");	\
	((type *)(__mptr - offsetof(type, member))); })


#define container_of_const(ptr, type, member)				\
	_Generic(ptr,							\
		const typeof(*(ptr)) *: ((const type *)container_of(ptr, type, member)),\
		default: ((type *)container_of(ptr, type, member))	\
	)


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif // _EH_TYPES_H_