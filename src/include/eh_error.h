/**
 * @file    eh_error.h
 * @brief   错误处理相关
 * @author simon.xiaoapeng (simon.xiaoapeng@gmail.com)
 * @version 1.0
 * @date 2024-04-21
 * 
 * @copyright Copyright (c) 2024  simon.xiaoapeng@gmail.com
 * 
 * @par 修改日志:
 */

#ifndef _EH_ERROR_H_
#define _EH_ERROR_H_


#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */

#define EH_RET_OK               0
#define EH_RET_FAULT            -1
#define EH_RET_INVALID_PARAM    -2
#define EH_RET_INVALID_STATE    -3
#define EH_RET_BUSY             -4

#define eh_param_assert(condition)          \
    do{                                     \
        if(!(condition))                    \
            return EH_RET_INVALID_PARAM;    \
    }while(0)




#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif // _EH_ERROR_H_