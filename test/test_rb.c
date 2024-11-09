
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <eh_debug.h>
#include <eh_rbtree.h>

#define TEST_DATA_SIZE 1000

struct test_data{
    int data;
    struct eh_rbtree_node node;
};

static int cmp(struct eh_rbtree_node *a, struct eh_rbtree_node *b){
    int a_data = eh_rb_entry(a, struct test_data, node)->data;
    int b_data = eh_rb_entry(b, struct test_data, node)->data;
    return a_data < b_data ? -1 : a_data > b_data ? 1 : 0;
}


void stdout_write(void *stream, const uint8_t *buf, size_t size){
    (void)stream;
    printf("%.*s", (int)size, (const char*)buf);
}

int match(const void *key, const struct eh_rbtree_node *node){
    int a_data = eh_rb_entry(node, struct test_data, node)->data;
    //eh_debugfl("key:%d a_data:%d", (int)((unsigned long)key), a_data);
    //return a_data == (int)((unsigned long)key) ? 0 : (int)((unsigned long)key) < a_data ? -1 : 1;
    (void)key;
    if(a_data < 300) return 0;
    return -1;
}

int main(void){
    struct eh_rbtree_root test_rb_root;
    struct test_data test_data[TEST_DATA_SIZE];
    struct test_data *test_data_ptr[TEST_DATA_SIZE];
    int test_data_ptr_len = 0;
    srand((unsigned int)time(NULL));
    // 生成并打印
    eh_rb_root_init(&test_rb_root, cmp);
    eh_debugraw("test_data:");
    for (int i = 0; i < TEST_DATA_SIZE; i++){
        test_data[i].data = rand()%TEST_DATA_SIZE;
        eh_rb_node_init(&(test_data[i].node));
        eh_debugraw("%d ", test_data[i].data);
        eh_rb_add(&(test_data[i].node), &test_rb_root);
    }
    eh_debugraw("\n");

    {
        struct test_data *pos;
        eh_debugraw("s-S:");
        eh_rb_next_for_each_entry(pos, &test_rb_root, node){
            eh_debugraw("%d ", pos->data);
        }
        eh_debugraw("\n");
    }

    
    {
        struct test_data *pos;
        eh_debugraw("S-s:");
        eh_rb_prev_for_each_entry(pos, &test_rb_root, node){
            eh_debugraw("%d ", pos->data);
        }
        eh_debugraw("\n");
    }
    {
        
        eh_debugraw("DEL:");
        struct test_data *pos;
        eh_rb_for_entry_each(pos, &test_rb_root, (void*)23, match, node){
            eh_debugraw("%d ", pos->data);
            test_data_ptr[test_data_ptr_len++] = pos;
        }
        eh_debugraw("\n");
    }

    /* 删除掉300以下的内容 */
    for(int i=0;i<test_data_ptr_len;i++){
        eh_rb_del(&test_data_ptr[i]->node, &test_rb_root);
    }

    {
        struct test_data *pos;
        eh_debugraw("S-s:");
        eh_rb_prev_for_each_entry(pos, &test_rb_root, node){
            eh_debugraw("%d ", pos->data);
        }
        eh_debugraw("\n");
    }

    
    return 0;
}