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
};

struct module_group{
    struct eh_module *module_array;
    long             module_cnt;
};


#define eh_modeule_section_begin()  ({   \
            extern char __start_eh_init_fini_array[];               \
            (void *)__start_eh_init_fini_array;                     \
        })
#define eh_modeule_section_end()  ({   \
            extern char __end_eh_init_fini_array[];                 \
            (void *)__end_eh_init_fini_array;                       \
        })

#define __eh_define_modeule_export(_init__func_, _exit__func_,  _section)                                    \
    static EH_USED const  struct eh_module  EH_SECTION( _section ) _eh_module_  = {                          \
        .init = _init__func_,                                                                                \
        .exit = _exit__func_,                                                                                \
    }

#define _eh_define_modeule_export(_init__func_, _exit__func_, _section_id) \
    __eh_define_modeule_export(_init__func_, _exit__func_, ".eh_init_fini_array." _section_id )



#define eh_core_module_export(_init__func_, _exit__func_)       _eh_define_modeule_export(_init__func_, _exit__func_, "1.0.0")
#define eh_interior_module_export(_init__func_, _exit__func_)   _eh_define_modeule_export(_init__func_, _exit__func_, "1.1.0")
#define eh_module_export(_init__func_, _exit__func_)            _eh_define_modeule_export(_init__func_, _exit__func_, "7.0.0")
#define EH_MODEULE_GROUP_MAX_CNT    8

#define __init EH_SECTION(".eh_init")
#define __exit EH_SECTION(".eh_exit")


#endif // _EH_MODULE_H_