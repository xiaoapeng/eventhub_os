/**
 * @file module_section.c
 * @brief 实现 Windows PE 格式的 eh_module_section 接口
 * @author simon.xiaoapeng (simon.xiaoapeng@gmail.com)
 * @date 2026-05-04
 *
 * @copyright Copyright (c) 2026  simon.xiaoapeng@gmail.com
 *
 */

#include <stdio.h>
#include <eh_debug.h>
#include <eh_error.h>
#include <eh_module.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

static uint8_t *s_eh_module_section = NULL;
static size_t   s_eh_module_section_len = 0;

void *eh_module_section_begin(void){
    return s_eh_module_section;
}

void *eh_module_section_end(void){
    return s_eh_module_section + s_eh_module_section_len;
}

/* COFF 字符串表缓存（从 PE 文件读取，因为内存中不加载） */
static char *s_coff_string_table = NULL;

static void load_coff_string_table(void){
    if(s_coff_string_table) return;
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(NULL, path, MAX_PATH);
    FILE *f = _wfopen(path, L"rb");
    if(!f) return;
    /* 读取 e_lfanew */
    fseek(f, 0x3C, SEEK_SET);
    LONG e_lfanew = 0;
    fread(&e_lfanew, 4, 1, f);
    /* 读取 NumberOfSections */
    fseek(f, e_lfanew + 6, SEEK_SET);
    WORD num_sections = 0;
    fread(&num_sections, 2, 1, f);
    /* 读取 SizeOfOptionalHeader */
    fseek(f, e_lfanew + 20, SEEK_SET);
    WORD opt_hdr_size = 0;
    fread(&opt_hdr_size, 2, 1, f);
    /* 字符串表在 symbol table 之后，先读 PointerToSymbolTable */
    fseek(f, e_lfanew + 12, SEEK_SET); /* FileHeader.PointerToSymbolTable at offset 8 from FileHeader start */
    DWORD sym_table_ptr = 0;
    fread(&sym_table_ptr, 4, 1, f);
    DWORD num_symbols = 0;
    fread(&num_symbols, 4, 1, f);
    if(sym_table_ptr > 0 && num_symbols > 0){
        /* 字符串表在 symbol table 之后: sym_table_ptr + num_symbols * 18 */
        long str_tab_offset = sym_table_ptr + num_symbols * 18;
        fseek(f, str_tab_offset, SEEK_SET);
        DWORD str_tab_size = 0;
        fread(&str_tab_size, 4, 1, f);
        if(str_tab_size > 4 && str_tab_size < 65536){
            s_coff_string_table = (char *)malloc(str_tab_size);
            if(s_coff_string_table){
                fseek(f, str_tab_offset, SEEK_SET);
                fread(s_coff_string_table, 1, str_tab_size, f);
            }
        }
    }
    fclose(f);
}

/* 获取 section 名称，处理 COFF 字符串表 */
static const char *get_section_name(const IMAGE_SECTION_HEADER *sect, const uint8_t *base, const IMAGE_NT_HEADERS64 *nt){
    static char buf[64];
    (void)base; (void)nt;
    if(sect->Name[0] != '/'){
        memcpy(buf, sect->Name, 8);
        buf[8] = '\0';
        return buf;
    }
    /* 从 COFF 字符串表获取名称 */
    load_coff_string_table();
    if(s_coff_string_table){
        int idx = atoi((const char *)sect->Name + 1);
        DWORD str_tab_size = *(DWORD *)s_coff_string_table;
        if(idx > 0 && (DWORD)idx < str_tab_size){
            strncpy(buf, s_coff_string_table + idx, sizeof(buf) - 1);
            buf[sizeof(buf) - 1] = '\0';
            return buf;
        }
    }
    memcpy(buf, sect->Name, 8);
    buf[8] = '\0';
    return buf;
}

int eh_module_section_init(void){
    uint8_t *base = (uint8_t *)GetModuleHandleW(NULL);
    if(base == NULL)
        return EH_RET_INVALID_STATE;

    IMAGE_DOS_HEADER *dos = (IMAGE_DOS_HEADER *)base;
    if(dos->e_magic != IMAGE_DOS_SIGNATURE)
        return EH_RET_INVALID_STATE;

    IMAGE_NT_HEADERS64 *nt = (IMAGE_NT_HEADERS64 *)(base + dos->e_lfanew);
    if(nt->Signature != IMAGE_NT_SIGNATURE)
        return EH_RET_INVALID_STATE;

    IMAGE_SECTION_HEADER *sections = IMAGE_FIRST_SECTION(nt);
    WORD num_sections = nt->FileHeader.NumberOfSections;

    /* 收集匹配 ".ehm" 前缀的 section */
    IMAGE_SECTION_HEADER *matched[64];
    WORD matched_count = 0;
    size_t total_size = 0;

    for(WORD i = 0; i < num_sections && matched_count < 64; i++){
        const char *name = get_section_name(&sections[i], base, nt);
        if(memcmp(name, ".ehm", 4) == 0){
            matched[matched_count++] = &sections[i];
            total_size += sections[i].Misc.VirtualSize;
            if(sections[i].Misc.VirtualSize % sizeof(struct eh_module) != 0)
                return EH_RET_INVALID_STATE;
        }
    }

    if(matched_count == 0)
        return EH_RET_INVALID_STATE;

    /* 按 section name 冒泡排序（用解析后的名称） */
    for(WORD i = 0; i < matched_count; i++){
        for(WORD j = 0; j < matched_count - i - 1; j++){
            char name_j[64], name_j1[64];
            strncpy(name_j, get_section_name(matched[j], base, nt), sizeof(name_j)-1);
            strncpy(name_j1, get_section_name(matched[j + 1], base, nt), sizeof(name_j1)-1);
            if(strcmp(name_j, name_j1) > 0){
                IMAGE_SECTION_HEADER *tmp = matched[j];
                matched[j] = matched[j + 1];
                matched[j + 1] = tmp;
            }
        }
    }

    s_eh_module_section = (uint8_t *)malloc(total_size);
    if(s_eh_module_section == NULL)
        return EH_RET_MALLOC_ERROR;

    s_eh_module_section_len = 0;
    for(WORD i = 0; i < matched_count; i++){
        const void *sect_data = base + matched[i]->VirtualAddress;
        size_t sect_size = matched[i]->Misc.VirtualSize;
        memcpy(s_eh_module_section + s_eh_module_section_len, sect_data, sect_size);
        s_eh_module_section_len += sect_size;
    }

    return 0;
}

void eh_module_section_exit(void){
    free(s_eh_module_section);
    s_eh_module_section = NULL;
    s_eh_module_section_len = 0;
}
