
/**
 * @file module_section.c
 * @brief 实现 macos 的 eh_module_section 接口
 * @author simon.xiaoapeng (simon.xiaoapeng@gmail.com)
 * @date 2025-12-19
 * 
 * @copyright Copyright (c) 2025  simon.xiaoapeng@gmail.com
 * 
 */
#include "eh_debug.h"
#include <eh_error.h>
#include <eh_module.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <mach-o/dyld.h>
#include <mach-o/loader.h>

uint8_t *s_eh_module_section = NULL;
size_t  s_eh_module_section_len = 0;

void *eh_module_section_begin(void){
    return s_eh_module_section;
}
void *eh_module_section_end(void){
    return s_eh_module_section + s_eh_module_section_len;
}
int  eh_module_section_init(void){
    const struct mach_header_64 *mh = (const struct mach_header_64 *)_dyld_get_image_header(0);
    intptr_t slide = _dyld_get_image_vmaddr_slide(0);
    uintptr_t lc;
    struct segment_command_64* eh_ifa_seg = NULL;
    struct section_64* eh_ifa_sec = NULL;
    size_t actual_used_size = 0;
    struct section_64** eh_ifa_sec_array_sort = NULL;
    if(mh == NULL){
        return EH_RET_INVALID_STATE;
    }
    lc = (uintptr_t)(mh + 1);
    for(uint32_t i = 0; i < mh->ncmds; i++){
        struct load_command* cmd = (struct load_command*)lc;
        lc += cmd->cmdsize;
        if(cmd->cmd == LC_SEGMENT_64){
            struct segment_command_64* seg = (struct segment_command_64*)cmd;
            if(strcmp(seg->segname, _EH_SECTION_BASE_NAME) == 0){
                eh_ifa_seg = seg;
            }
        }
    }

    if(eh_ifa_seg == NULL){
        return EH_RET_INVALID_STATE;
    }

    eh_ifa_sec = (struct section_64*)(eh_ifa_seg + 1);
    eh_ifa_sec_array_sort = alloca(eh_ifa_seg->nsects * sizeof(struct section_64*));

    for(uint32_t i = 0; i < eh_ifa_seg->nsects; i++){
        eh_ifa_sec_array_sort[i] = &eh_ifa_sec[i];
        actual_used_size += eh_ifa_sec[i].size;
        if(eh_ifa_sec[i].size % sizeof(struct eh_module))
            return EH_RET_INVALID_STATE;
    }

    for(uint32_t i = 0; i < eh_ifa_seg->nsects; i++){
        for(uint32_t j = 0; j < eh_ifa_seg->nsects - i - 1; j++){
            if(strcmp(eh_ifa_sec_array_sort[j]->sectname, eh_ifa_sec_array_sort[j + 1]->sectname) > 0){
                struct section_64* tmp = eh_ifa_sec_array_sort[j];
                eh_ifa_sec_array_sort[j] = eh_ifa_sec_array_sort[j + 1];
                eh_ifa_sec_array_sort[j + 1] = tmp;
            }
        }
    }
    s_eh_module_section = (uint8_t *)malloc(actual_used_size);
    if(s_eh_module_section == NULL){
        return EH_RET_MALLOC_ERROR;
    }
    s_eh_module_section_len = 0;
    for(uint32_t i = 0; i < eh_ifa_seg->nsects; i++){
        const void* sect_addr = (const void*)((uint64_t)slide + eh_ifa_sec_array_sort[i]->addr);
        eh_debugfl("name %s, addr %p, size %ld", eh_ifa_sec_array_sort[i]->sectname, sect_addr, eh_ifa_sec_array_sort[i]->size);
        memcpy(s_eh_module_section + s_eh_module_section_len, sect_addr, eh_ifa_sec_array_sort[i]->size);
        s_eh_module_section_len += eh_ifa_sec_array_sort[i]->size;
    }

    return 0;
}
void eh_module_section_exit(void){
    free(s_eh_module_section);
    s_eh_module_section = NULL;
    s_eh_module_section_len = 0;
}