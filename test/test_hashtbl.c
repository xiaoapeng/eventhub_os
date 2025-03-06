/**
 * @file test_dbg.c
 * @brief 
 * @author simon.xiaoapeng (simon.xiaoapeng@gmail.com)
 * @date 2024-07-06
 * 
 * @copyright Copyright (c) 2024  simon.xiaoapeng@gmail.com
 * 
 */

#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <eh.h>
#include <eh_types.h>
#include <eh_platform.h>
#include <eh_error.h>
#include <eh_debug.h>
#include <eh_formatio.h>
#include <eh_hashtbl.h>

void stdout_write(void *stream, const uint8_t *buf, size_t size){
    (void)stream;
    printf("%.*s", (int)size, (const char*)buf);
}




int task_app(void *arg){
    (void)arg;
    eh_hashtbl_t hashtbl;
    struct eh_hashtbl_node *node;
    struct eh_hashtbl_node *tmp_n;
    unsigned int tmp_uint_i;
    uint32_t i = 0;
    char key[20];
    char value[20];
    eh_clock_t start_time, end_time;

    EH_DBG_ERROR_EXEC(eh_ptr_to_error((hashtbl = eh_hashtbl_create(EH_HASHTBL_DEFAULT_LOADFACTOR))) < 0, return -1);

    for(i = 0; i < 10 * 10000; i++){
        eh_snprintf(key, 20, "%s%d", "test", i);
        EH_DBG_ERROR_EXEC((node = eh_hashtbl_node_new_with_string(hashtbl, key, 20)) == NULL, goto test_quit);
        eh_snprintf(eh_hashtbl_node_value(node), eh_hashtbl_node_value_len(node), "value%d", i);
        EH_DBG_ERROR_EXEC(eh_hashtbl_insert(hashtbl, node) < 0, goto test_quit);
    }

    i = 0;
    eh_hashtbl_for_each_safe(hashtbl, node, tmp_n, tmp_uint_i){
        eh_infofl("key:%.*s value:%.*s", 
            eh_hashtbl_node_key_len(node), eh_hashtbl_node_const_key(node), 
            eh_hashtbl_node_value_len(node), eh_hashtbl_node_value(node));
        i++;
    }
    eh_infofl("node count:%d", i);

    for(i = 0; i < 10 * 10000; i++){
        eh_snprintf(key, 20, "%s%d", "test", i);
        EH_DBG_ERROR_EXEC(eh_hashtbl_find_with_string(hashtbl, key, &node) != EH_RET_OK, goto test_quit);
        eh_snprintf(value, sizeof(value), "value%d", i);
        EH_DBG_ERROR_EXEC(strcmp((const char *)eh_hashtbl_node_value(node), value) != 0, goto test_quit);
        // eh_infofl("key:%s value:%s", key, (const char *)eh_hashtbl_node_value(node));
        eh_hashtbl_node_delete(hashtbl, node);
    }
    eh_infofl("test_hashtbl_node_new_with_string OK");

    start_time = eh_get_clock_monotonic_time();
    for(i = 0; i < 10 * 10000; i++){
        EH_DBG_ERROR_EXEC((node = eh_hashtbl_node_new(hashtbl, &i, 4, 4)) == NULL, goto test_quit);
        *(uint32_t*)eh_hashtbl_node_value(node) = i;
        EH_DBG_ERROR_EXEC(eh_hashtbl_insert(hashtbl, node) < 0, goto test_quit);
    }
    end_time = eh_get_clock_monotonic_time();
    eh_infofl("insert %d node cost %d usec", 10 * 10000, eh_clock_to_usec(end_time - start_time));

    
    start_time = eh_get_clock_monotonic_time();
    for(i = 10 * 10000 -1 ; i < 10 * 10000; i--){
        EH_DBG_ERROR_EXEC(eh_hashtbl_find(hashtbl, &i, 4, &node) != EH_RET_OK, goto test_quit);
        EH_DBG_ERROR_EXEC(i != *(uint32_t*)eh_hashtbl_node_value(node), goto test_quit);
        eh_hashtbl_node_delete(hashtbl, node);
    }
    end_time = eh_get_clock_monotonic_time();
    eh_infofl("remove %d node cost %d usec", 10 * 10000, eh_clock_to_usec(end_time - start_time));

    eh_infofl("test_hashtbl_node_new OK");

    /* 重建hashtbl表 */
    eh_hashtbl_destroy(hashtbl);
    EH_DBG_ERROR_EXEC(eh_ptr_to_error((hashtbl = eh_hashtbl_create(EH_HASHTBL_DEFAULT_LOADFACTOR))) < 0, return -1);

    /* 随机测试 */
    {
        uint32_t insert_i = 0;
        uint32_t remove_i = 0;
        
        for(i = 0; i < 10 * 10000; i++){
            switch (random()%5) {
                case 0:
                case 1:{
                    /* 插入 */
                    EH_DBG_ERROR_EXEC((node = eh_hashtbl_node_new(hashtbl, &insert_i, 4, 4)) == NULL, goto test_quit);
                    *(uint32_t*)eh_hashtbl_node_value(node) = insert_i;
                    EH_DBG_ERROR_EXEC(eh_hashtbl_insert(hashtbl, node) < 0, goto test_quit);
                    insert_i++;
                    break;
                }
                case 2:{
                    /* 删除 */
                    if(remove_i >= insert_i)
                        continue;
                    EH_DBG_ERROR_EXEC(eh_hashtbl_find(hashtbl, &remove_i, 4, &node) != EH_RET_OK, goto test_quit);
                    EH_DBG_ERROR_EXEC(remove_i != *(uint32_t*)eh_hashtbl_node_value(node), goto test_quit);
                    eh_hashtbl_node_delete(hashtbl, node);
                    remove_i++;
                    break;
                }
                case 3:{
                    uint32_t find_i;
                    if(remove_i >= insert_i)
                        continue;
                    find_i = ((uint32_t)random())%(insert_i - remove_i) + remove_i;
                    EH_DBG_ERROR_EXEC(eh_hashtbl_find(hashtbl, &find_i, 4, &node) != EH_RET_OK, goto test_quit);
                    EH_DBG_ERROR_EXEC(find_i != *(uint32_t*)eh_hashtbl_node_value(node), goto test_quit);
                    break;
                }
                case 4:{
                    uint32_t find_i;
                    if(remove_i >= insert_i)
                        continue;
                    find_i = ((uint32_t)random())%(insert_i - remove_i) + remove_i;
                    EH_DBG_ERROR_EXEC(eh_hashtbl_find(hashtbl, &find_i, 4, &node) != EH_RET_OK, goto test_quit);
                    EH_DBG_ERROR_EXEC((node = eh_hashtbl_node_renew(hashtbl, node, 8)) == NULL, goto test_quit);
                    *(uint64_t*)eh_hashtbl_node_value(node) = find_i;
                }
            };
        }


    }

    {
        struct eh_hashtbl_node *node_pos, *tmp_n;
        struct eh_list_head *tmp_head;


        uint32_t insert_i = 0xffeeccaa;
        EH_DBG_ERROR_EXEC((node = eh_hashtbl_node_new(hashtbl, &insert_i, 4, 4)) == NULL, goto test_quit);
        *(uint32_t*)eh_hashtbl_node_value(node) = 0x01;
        EH_DBG_ERROR_EXEC(eh_hashtbl_insert(hashtbl, node) < 0, goto test_quit);

        EH_DBG_ERROR_EXEC((node = eh_hashtbl_node_new(hashtbl, &insert_i, 4, 4)) == NULL, goto test_quit);
        *(uint32_t*)eh_hashtbl_node_value(node) = 0x02;
        EH_DBG_ERROR_EXEC(eh_hashtbl_insert(hashtbl, node) < 0, goto test_quit);

        EH_DBG_ERROR_EXEC((node = eh_hashtbl_node_new(hashtbl, &insert_i, 4, 4)) == NULL, goto test_quit);
        *(uint32_t*)eh_hashtbl_node_value(node) = 0x03;
        EH_DBG_ERROR_EXEC(eh_hashtbl_insert(hashtbl, node) < 0, goto test_quit);

        EH_DBG_ERROR_EXEC((node = eh_hashtbl_node_new(hashtbl, &insert_i, 4, 4)) == NULL, goto test_quit);
        *(uint32_t*)eh_hashtbl_node_value(node) = 0x04;
        EH_DBG_ERROR_EXEC(eh_hashtbl_insert(hashtbl, node) < 0, goto test_quit);
        
        eh_hashtbl_for_each_with_key_safe(hashtbl, &insert_i, sizeof(uint32_t), node_pos, tmp_n, tmp_head){
            eh_infofl("key:%#08x value:%.*hhq", *((const uint32_t*)eh_hashtbl_node_const_key(node_pos)), 
            eh_hashtbl_node_value_len(node_pos), eh_hashtbl_node_value(node_pos));
        }

        EH_DBG_ERROR_EXEC((node = eh_hashtbl_node_new_with_string(hashtbl, "abcdef", 4)) == NULL, goto test_quit);
        *(uint32_t*)eh_hashtbl_node_value(node) = 0x01;
        EH_DBG_ERROR_EXEC(eh_hashtbl_insert(hashtbl, node) < 0, goto test_quit);

        EH_DBG_ERROR_EXEC((node = eh_hashtbl_node_new_with_string(hashtbl, "abcdef", 4)) == NULL, goto test_quit);
        *(uint32_t*)eh_hashtbl_node_value(node) = 0x02;
        EH_DBG_ERROR_EXEC(eh_hashtbl_insert(hashtbl, node) < 0, goto test_quit);

        EH_DBG_ERROR_EXEC((node = eh_hashtbl_node_new_with_string(hashtbl, "abcdef", 4)) == NULL, goto test_quit);
        *(uint32_t*)eh_hashtbl_node_value(node) = 0x03;
        EH_DBG_ERROR_EXEC(eh_hashtbl_insert(hashtbl, node) < 0, goto test_quit);

        EH_DBG_ERROR_EXEC((node = eh_hashtbl_node_new_with_string(hashtbl, "abcdef", 4)) == NULL, goto test_quit);
        *(uint32_t*)eh_hashtbl_node_value(node) = 0x04;
        EH_DBG_ERROR_EXEC(eh_hashtbl_insert(hashtbl, node) < 0, goto test_quit);

        eh_hashtbl_for_each_with_string_safe(hashtbl, "abcdef", node_pos, tmp_n, tmp_head){
            eh_infofl("key:%.*s value:%.*hhq", eh_hashtbl_node_key_len(node_pos), (const char*)eh_hashtbl_node_const_key(node_pos), 
            eh_hashtbl_node_value_len(node_pos), eh_hashtbl_node_value(node_pos));
        }
    }

test_quit:
    eh_hashtbl_destroy(hashtbl);
    return 0;
}



int main(void){
    
    
    eh_global_init();
    
    task_app("task_app");
    
    eh_global_exit();
    return 0;
}
 
 