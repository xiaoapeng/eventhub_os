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


#define EH_STRINGIFY(x) #x

#define eh_offsetof(TYPE, MEMBER)	        __builtin_offsetof(TYPE, MEMBER)
#define eh_same_type(a, b)               __builtin_types_compatible_p(typeof(a), typeof(b))
#define eh_static_assert(expr, msg)     _Static_assert(expr, msg)
#define eh_likely(x)	                __builtin_expect(!!(x), 1)
#define eh_unlikely(x)	                __builtin_expect(!!(x), 0)

#define __weak                         __attribute__((weak))
#define __safety                    /* 被此宏标记的函数，可在中断和其他线程中进行调用 */
#define __noreturn                      __attribute__((noreturn))
#define eh_container_of(ptr, type, member) ({				\
	void *__mptr = (void *)(ptr);					\
	eh_static_assert(eh_same_type(*(ptr), ((type *)0)->member) ||	\
		      eh_same_type(*(ptr), void),			\
		      "pointer type mismatch in eh_container_of()");	\
	((type *)(__mptr - eh_offsetof(type, member))); })


#define eh_container_of_const(ptr, type, member)				\
	_Generic(ptr,							\
		const typeof(*(ptr)) *: ((const type *)eh_container_of(ptr, type, member)),\
		default: ((type *)eh_container_of(ptr, type, member))	\
	)


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif // _EH_TYPES_H_