/**
 * eventhub_os 用户配置文件
 */

#ifndef _EH_USER_CONFIG_H_
#define _EH_USER_CONFIG_H_


/**
 *  配置事件回调函数的栈大小
 */
#define EH_CONFIG_EVENT_CALLBACK_FUNCTION_STACK_SIZE            (8*1024U)


/**
 *  platform_get_clock_monotonic_time 函数获取到的时钟使用的时钟频率
 */
#define EH_CONFIG_CLOCKS_PER_SEC                                (1000000UL)

/**
 *  EH_CONFIG_USE_LIBC_MEM_MANAGE为1时,将使用libc库的内存管理,未定义
 *   或者定义为0时使用eventhub_os的内存管理,使用libc时请务必实现：
 *    caddr_t _sbrk(int incr); 否则由于协程的特殊性(协程栈在堆上实现)，
 *    会使C库误以为发生堆栈碰撞而malloc失败。
 *  EH_CONFIG_MEM_ALLOC_ALIGN为分配空间的对齐粒度，为2的幂，这里默认为系统指针大小的2倍
 *  EH_CONFIG_MEM_HEAP_SIZE为堆内存大小，默认为20K
 */
#define EH_CONFIG_USE_LIBC_MEM_MANAGE                            0
#if (!defined(EH_CONFIG_USE_LIBC_MEM_MANAGE)) || (EH_CONFIG_USE_LIBC_MEM_MANAGE == 0)
#   define EH_CONFIG_MEM_ALLOC_ALIGN                             (sizeof(void*)*2)
#   define EH_CONFIG_MEM_HEAP_SIZE                               (1024*1024U)
#endif

/**
 *  配置标准输出缓存大小,该缓冲为单次输出的最大字节数，并不限制eh_printf的输出字节数
 */
#define EH_CONFIG_STDOUT_MEM_CACHE_SIZE                          (32U)

/*
 * 是否支持浮点格式化 
 */
#define EH_CONFIG_PRINTF_SUPPORT_FLOAT_FORMAT                     1


#endif // _EH_USER_CONFIG_H_