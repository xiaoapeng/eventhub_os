/**
 * @file main.c
 * @brief  test 
 * @author simon.xiaoapeng (simon.xiaoapeng@gmail.com)
 * @version 1.0
 * @date 2024-04-13
 * 
 * @copyright Copyright (c) 2024  simon.xiaoapeng@gmail.com
 * 
 * @par 修改日志:
 */

#include <stdio.h>
#include <time.h>
#include "debug.h"

#include "eh.h"


static void init(void){
    debug_init();
}



int main(void)
{
    init();

    //dbg_debugfl("%d",EH_EVENT_SIZE);
    dbg_debugln("test app start !");
    return 0;    
}
