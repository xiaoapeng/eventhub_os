/**
 * @file eh_module.h
 * @brief eh 模块定义
 * @author simon.xiaoapeng (simon.xiaoapeng@gmail.com)
 * @version 1.0
 * @date 2024-05-12
 * 
 * @copyright Copyright (c) 2024  simon.xiaoapeng@gmail.com
 * 
 * @par 修改日志:
 */



#ifndef _EH_MODULE_H_
#define _EH_MODULE_H_
#include "eh_types.h"

struct eh_module{
    int (*init)(void);
    void (*exit)(void);
    char *modeule_name;
    long section_id;
};

struct module_group{
    struct eh_module *module_array;
    long             module_cnt;
};


#define eh_modeule_section_begin(_section_id)  EH_SECTION_BEGIN(modeule_call_##_section_id)
#define eh_modeule_section_end(_section_id)  EH_SECTION_END(modeule_call_##_section_id)

#define __eh_define_modeule_null(_section_id) static EH_USED  const  char used_section_call_##_section_id[0]   \
    EH_SECTION( EH_STRINGIFY(modeule_call_##_section_id) ) 
#define __eh_define_modeule_export(_init__func_, _exit__func_, _section_id, _section)                        \
    static EH_USED const  struct eh_module _eh_module_ EH_SECTION( EH_STRINGIFY(_section) ) = {               \
        .init = _init__func_,                                                                                \
        .exit = _exit__func_,                                                                                \
        .modeule_name = NULL,                                                                                \
        .section_id = _section_id,                                                                           \
    }

#define _eh_define_modeule_export(_init__func_, _exit__func_, _section_id) \
    __eh_define_modeule_export(_init__func_, _exit__func_, _section_id, modeule_call_##_section_id )



#define eh_core_module_export(_init__func_, _exit__func_)   _eh_define_modeule_export(_init__func_, _exit__func_, 0)
#define eh_module_export(_init__func_, _exit__func_)        _eh_define_modeule_export(_init__func_, _exit__func_, 7)
#define eh_module_section_define() \
    __eh_define_modeule_null(0); \
    __eh_define_modeule_null(1); \
    __eh_define_modeule_null(2); \
    __eh_define_modeule_null(3); \
    __eh_define_modeule_null(4); \
    __eh_define_modeule_null(5); \
    __eh_define_modeule_null(6); \
    __eh_define_modeule_null(7)

#define EH_MODEULE_GROUP_MAX_CNT    8

#define __init EH_SECTION("eh_init")
#define __exit EH_SECTION("eh_exit")


#endif // _EH_MODULE_H_