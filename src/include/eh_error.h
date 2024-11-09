/**
 * @file    eh_error.h
 * @brief   错误处理相关
 * @author simon.xiaoapeng (simon.xiaoapeng@gmail.com)
 * @date 2024-04-21
 * 
 * @copyright Copyright (c) 2024  simon.xiaoapeng@gmail.com
 * 
 */

#ifndef _EH_ERROR_H_
#define _EH_ERROR_H_

#include "eh_types.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */

#define EH_RET_OK                           (0   )
#define EH_RET_FAULT                        (-1  )
#define EH_RET_INVALID_PARAM                (-2  )
#define EH_RET_INVALID_STATE                (-3  )
#define EH_RET_BUSY                         (-4  )
#define EH_RET_SCHEDULING_ERROR             (-5  )
#define EH_RET_EVENT_ERROR                  (-6  )
#define EH_RET_TIMEOUT                      (-7  )
#define EH_RET_MALLOC_ERROR                 (-8  )
#define EH_RET_PTR_NULL                     (-9  )
#define EH_RET_MEM_POOL_EMPTY               (-10 )
#define EH_RET_NOT_SUPPORTED                (-11 )
#define EH_RET_MIN_ERROR_NUM                (-255)

#define eh_param_assert(condition)                                      \
    do{                                                                 \
        if(!(condition))                                                \
            return EH_RET_INVALID_PARAM;                                \
    }while(0)

static inline __attribute_const__ int eh_ptr_to_error(void *ptr){
    if( eh_likely(((long)(ptr)) > 0) || eh_likely((long)(ptr) <= EH_RET_MIN_ERROR_NUM) )
        return EH_RET_OK;
    if(!ptr)
        return EH_RET_PTR_NULL;
    return (int)(long)(ptr);
}

#define eh_error_to_ptr(err_on)             ((void*)(err_on))

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif // _EH_ERROR_H_