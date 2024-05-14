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

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif // _EH_TYPES_H_