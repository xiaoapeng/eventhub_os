/**
 * @file eh_llist.c
 * @brief 单链表实现
 * @author simon.xiaoapeng (simon.xiaoapeng@gmail.com)
 * @date 2024-11-10
 * 
 * @copyright Copyright (c) 2024  simon.xiaoapeng@gmail.com
 * 
 */

#include "eh_llist.h"
#include "eh_types.h"
#include <stdbool.h>

bool eh_llist_add_batch(struct eh_llist_node *new_first,
				     struct eh_llist_node *new_last,
				     struct eh_llist_head *head)
{
	new_last->next = head->first;
	head->first = new_first;
    if(eh_unlikely(new_last->next == NULL)){
        head->last = new_last;
        return true;
    }
    return false;
}

bool eh_llist_add_batch_tail(struct eh_llist_node *new_first, struct eh_llist_node *new_last, struct eh_llist_head *head)
{
    new_last->next = NULL;
    if(eh_unlikely(eh_llist_empty(head))){
        head->first = new_first;
        head->last = new_last;
        return true;
    }
    head->last->next = new_first;
    head->last = new_last;
    return false;
}
