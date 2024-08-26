# EVENTHUB OS

## 项目介绍

本项目实现了一个基于C语言的协作式内核，涵盖了常见的RTOS基础功能。支持任务创建、信号量、互斥锁、定时器和自定义事件触发等待。所有这些机制都通过统一的事件驱动模型实现，并在此基础上实现了高效的epoll功能。由于这是一个协作式内核（协程），不支持抢占，从而避免了许多资源竞争问题，减少了潜在的BUG。

那么，在不支持抢占的情况下，如何保证实时性？与其他RTOS相比，EventHub OS的实时性如何呢？EventHub OS的实时性由程序员的代码质量决定，只要在合适的地方调用yield让出CPU，就能达到不输RTOS的实时性。由于无需依赖Tick中断，CPU可以更加专注于业务处理。在单核CPU下，协程相较于线程可以提供更好的性能。

支持动态内存分配，支持碎片合并算法(使用freeRTOS heap5一样的算法，对于小芯片也算最优解了)，可通过配置文件选择使用C库和自带的内存管理。

实现了printf家族函数（**感谢[ eyalroz/printf ](https://github.com/eyalroz/printf)项目使本项目实现浮点打印**），支持数组打印的扩展特性%q，暂时不支持功能裁剪，后续会支持。

支持系统时钟级别（微秒级别）定时器及延时，这一点是其他RTOS也能实现，但是要付出更大的代价，RTOS需要如果将TICK中断设置到微秒级别，那CPU将只为TICK中断来服务了。这将可以用eh_usleep轻松控制GPIO模拟各种时序。

支持事件和槽函数模型(类似于QT)，当资源有限时，甚至可以不创建任何任务，完全使用事件和槽函数模型，通过事件驱动。

## 平台支持

支持 linux x86_64 平台和 Cortex-M0/M3/M4/M33 架构

| 测试芯片                    | 平台/架构          | 工具链                         | 备注                             |
|----------------------------|-------------------|--------------------------------|---------------------------------|
| i7-10700                   | linux/x86_64     | gcc 11.4.0                     |                                 |
| stm32f030c8t6              | Cortex-M0        | arm-none-eabi-gcc  10.3.1      | 理论可支持所有的M0核               |
| stm32f103c6t6              | Cortex-M3        | arm-none-eabi-gcc  10.3.1      | 理论可支持所有的M3核               |
| stm32f401vbt6              | Cortex-M4         | arm-none-eabi-gcc  10.3.1      | 理论可支持所有的M4核              |
| stm32h750vbt6              | Cortex-M7         | arm-none-eabi-gcc  10.3.1      | 理论可支持所有的M7核              |
| mcxn947                    | Cortex-M33        | arm-none-eabi-gcc  10.3.1      | 理论可支持所有的M33核             |


## 如何移植到项目中使用

<p><span style="color: yellow;">目前只在gcc下进行过验证，还未适配其他编译器。</span></p>


### 1. 代码集成

* *CMAKE 方式集成*

#### 将eventhub_os添加到你的项目中后，在CMakeLists.txt中合适的位置添加

```
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/eventhub_os/")
```

#### 复制eventhub_os/test/inc/eh_user_config.h 到你的项目include（假设是./include）中,然后在添加cmake中添加

```
target_include_directories( eventhub PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
```

#### 最后给你的目标引用eventhub

```
add_executable(you_target)
    target_link_libraries(you_target xxxxx eventhub )
```

#### 定义一些cmake变量，使eventhub自动选择架构和平台

定义CMAKE变量`CMAKE_SYSTEM_NAME`和 `CMAKE_SYSTEM_PROCESSOR`

| 平台 | 定义方式 | 备注 |
| --- | --- | --- |
| linux x86_64 | `CMAKE_SYSTEM_NAME`=Linux<br>`CMAKE_SYSTEM_PROCESSOR`=x86_64  | 一般选择系统都会自动指定好 |
| Cortex-M0 | `CMAKE_SYSTEM_NAME`=Generic<br>`CMAKE_SYSTEM_PROCESSOR`=cortex-m0  | 一般要手动指定 |
| Cortex-M3 | `CMAKE_SYSTEM_NAME`=Generic<br>`CMAKE_SYSTEM_PROCESSOR`=cortex-m3  | 一般要手动指定 |
| Cortex-M4 | `CMAKE_SYSTEM_NAME`=Generic<br>`CMAKE_SYSTEM_PROCESSOR`=cortex-m4  | 一般要手动指定 |
| Cortex-M7 | `CMAKE_SYSTEM_NAME`=Generic<br>`CMAKE_SYSTEM_PROCESSOR`=cortex-m7  | 一般要手动指定 |
| Cortex-M33 | `CMAKE_SYSTEM_NAME`=Generic<br>`CMAKE_SYSTEM_PROCESSOR`=cortex-m33  | 一般要手动指定 |


* *其他方式集成*

添加以下文件在你的项目中
```
├── eh_core.c
├── eh_event.c
├── eh_event_cb.c
├── eh_mem.c
├── eh_mutex.c
├── eh_sem.c
├── eh_sleep.c
├── eh_timer.c
├── general
│   ├── eh_debug.c
│   ├── eh_formatio.c
|   ├── eh_rbtree.c
│   └── include
│       ├── eh_debug.h
|       ├── eh_list.h
|       ├── eh_rbtree.h
│       └── eh_formatio.h
└── include
    ├── eh_co.h
    ├── eh_config.h
    ├── eh_error.h
    ├── eh_event_cb.h
    ├── eh_event.h
    ├── eh.h
    ├── eh_interior.h
    ├── eh_mem.h
    ├── eh_module.h
    ├── eh_mutex.h
    ├── eh_platform.h
    ├── eh_sem.h
    ├── eh_sleep.h
    ├── eh_timer.h
    └── eh_types.h
```
#### 根据你选择的平台添加 src/<span style="color: red;">coroutine</span>/<XX平台>/ 的代码到你的项目中

假如选择m33平台，那么添加src/coroutine/cortex-m33/的代码到你的项目中，如果该目录下有多个文件，一般代表有多种任务切换实现,选择其中一种即可，若选择多种，会产生重定义,一般选择CMakeLists.txt文件中使用的默认实现即可

```
# 以cm33实现为例，CMakeLists.txt中 默认使用 coroutine_pendsv.c
.
├── CMakeLists.txt
├── coroutine_msp.c
└── coroutine_pendsv.c (default)

```

#### 根据你选择的平台添加src/<span style="color: red;">platform</span>/<XX平台>/ 的代码到你的项目中

假如选择m33平台，那么添加src/platform/cortex-m33/的代码到你的项目中
```
.
├── CMSIS
│   ├── include
│   │   └── core_cm33.h
├── inc
│   └── platform_port.h
└── platform.c
```

### 2. 实现platform_get_clock_monotonic_time函数(linux平台忽略)

例如 m33平台下

```
#define TICK_PER_SEC                                  (100U)
volatile eh_clock_t sys_clock_cnt = 0;
uint32_t tick_cycle;

void SysTick_Handler(void)
{
    sys_clock_cnt += tick_cycle;
}

/* 超精度时钟，获取从系统启动到现在的时钟次数 */
eh_clock_t  platform_get_clock_monotonic_time(void){
    eh_clock_t sys_clock_cnt_a , sys_clock_cnt_b;
    sys_clock_cnt_a = sys_clock_cnt + (SysTick->LOAD - SysTick->VAL);
    sys_clock_cnt_b = sys_clock_cnt + (SysTick->LOAD - SysTick->VAL);
    return sys_clock_cnt_a > sys_clock_cnt_b ? sys_clock_cnt_a : sys_clock_cnt_b;
}

static __init int systick_init(void)
{
    tick_cycle = SystemCoreClock/TICK_PER_SEC;
    SysTick_Config(tick_cycle);
    return 0;
}

/* 函数注册初始化列表，会在系统初始化过程中自动调用 */
eh_core_module_export(systick_init, NULL);
```

### 3. 实现标准输出接口
列如在linux平台下
```
void stdout_write(void *stream, const uint8_t *buf, size_t size){
    (void)stream;
    printf("%.*s", (int)size, (const char*)buf);
}
```
例子: [test/test_epoll.c](test/test_epoll.c)


### 4. 修改链接脚本(linux平台忽略)
在链接脚本.rodata附近添加以下内容
```
. = ALIGN(8);
PROVIDE_HIDDEN (__start_eh_init_fini_array = .);
KEEP (*(SORT(.eh_init_fini_array.*)));
PROVIDE_HIDDEN (__end_eh_init_fini_array = .);
```
例如下面的写法
```
    .text :
    {
        . = ALIGN(4);
        *(.text)                 /* .text sections (code) */
        *(.text*)                /* .text* sections (code) */
        *(.rodata)               /* .rodata sections (constants, strings, etc.) */
        *(.rodata*)              /* .rodata* sections (constants, strings, etc.) */
        . = ALIGN(8);
        PROVIDE_HIDDEN (__start_eh_init_fini_array = .);
        KEEP (*(SORT(.eh_init_fini_array.*)));
        PROVIDE_HIDDEN (__end_eh_init_fini_array = .);
        *(.glue_7)               /* glue arm to thumb code */
        *(.glue_7t)              /* glue thumb to arm code */
        *(.eh_frame)
        KEEP (*(.init))
        KEEP (*(.fini))
        . = ALIGN(4);
    } > m_text
```

### 5. 配置eh_user_config.h

| 配置项 | 解释 |
| --- | --- |
| `EH_CONFIG_EVENT_CALLBACK_FUNCTION_STACK_SIZE` | 回调槽函数堆栈大小默认为1024字节，可以根据需要调整 |
| `EH_CONFIG_CLOCKS_PER_SEC` | `platform_get_clock_monotonic_time`返回的时钟周期数比如`48000000`/`72000000`一般是单片机的MAIN频率 |
| `EH_CONFIG_USE_LIBC_MEM_MANAGE` | 是否使用C库进行内存管理，默认为0时使用自带的内存管理 |
| `EH_CONFIG_MEM_ALLOC_ALIGN` | `EH_CONFIG_USE_LIBC_MEM_MANAGE`为0时有效，内部分配内存对齐字节数，默认为指针大小的两倍 |
| `EH_CONFIG_MEM_HEAP_SIZE` | `EH_CONFIG_USE_LIBC_MEM_MANAGE`为0时有效，为默认堆的大小，OS将利用此宏定义一个数组，作为堆空间使用 |
| `EH_CONFIG_STDOUT_MEM_CACHE_SIZE` | eh_printf函数的内部缓存大小，越大对于printf的性能越有益 |
| `EH_CONFIG_DEFAULT_DEBUG_LEVEL` | 系统默认DEBUG打印等级，可选`EH_DBG_DEBUG`/`EH_DBG_INFO`/`EH_DBG_SYS`/`EH_DBG_WARNING`/`EH_DBG_ERR`|
| `EH_CONFIG_DEBUG_ENTER_SIGN` | DEBUG模块使用的默认回车符一般，在单片机上使用`"\r\n"`在linux上使用`"\n"` |
| `EH_CONFIG_DEBUG_FLAGS` | 默认DEBUG模块输出所带TAG，默认带单调时间和DEBUG等级（`EH_DBG_FLAGS_DEBUG_TAG\|EH_DBG_FLAGS_MONOTONIC_CLOCK`）,若想简单输出，设置为0即可 |
| `EH_CONFIG_INTERRUPT_STACK_SIZE`| 中断栈大小，默认为1024字节，可以根据需要调整 |
| `EH_CONFIG_TASK_DISPATCH_CNT_PER_POLL` | 配置任务调度多少次后进行一次轮询 |

## API文档

API需要包含如下头文件引入，可以按需引入，需要注意include顺序
```
#include "eh.h"                 /* 任务和全局函数 */
#include "eh_event.h"           /* 事件相关API */
#include "eh_timer.h"           /* 定时器相关API */
#include "eh_sleep.h"           /* 阻塞睡眠相关API */
#include "eh_mem.h"             /* 动态内存分配相关API */
#include "eh_sem.h"             /* 信号量相关API */
#include "eh_mutex.h"           /* 互斥锁相关API */
#include "eh_signal.h"          /* 信号相关API */
#include "eh_formatio.h"        /* 格式化输出相关API */
#include "eh_debug.h"           /* debug输出相关API */
```
### 全局初始化和销毁
#### 1.全局初始化
调用此函数后内部自动初始化eh module,调用该函数后才可使用协程相关API,调用成功返回0，调用失败返回负数, 返回值来自模块返回的错误值，应该要遵守eh_error.h中定义的错误值。
```
extern int eh_global_init(void);
```

#### 2.全局销毁
调用此函数后内部自动销毁eh module,调用该函数后才不能使用协程相关API
```
extern void eh_global_exit(void)
```

### 模块相关API
#### 1.模块自动构造与销毁
初始化时，会调用模块的初始化函数，模块销毁时，会调用模块的销毁函数。 level值越小的模块越在前面被构造，销毁时反之。
<br>在自定义模块时使用这里的宏可以降低初始化工作中模块间的耦合性
```
eh_module_level0_export(_init__func_, _exit__func_)
eh_module_level1_export(_init__func_, _exit__func_)
eh_module_level2_export(_init__func_, _exit__func_)
eh_module_level3_export(_init__func_, _exit__func_)
eh_module_level4_export(_init__func_, _exit__func_)
eh_module_level5_export(_init__func_, _exit__func_)
eh_module_level6_export(_init__func_, _exit__func_)
eh_module_level7_export(_init__func_, _exit__func_)
eh_module_level8_export(_init__func_, _exit__func_)
eh_module_level9_export(_init__func_, _exit__func_)
```
| 参数 | 解释 |
| --- | --- |
| \_init__func_ | 模块初始化函数，原型 int eh_timer_interior_init(void) |
| \_exit__func_ | 模块销毁函数，原型 void eh_timer_interior_exit(void) |


### 协程任务相关API
#### 1.创建任务
创建一个协程任务，返回一个任务句柄，可用于回收任务，获取任务状态。
```
extern eh_task_t* eh_task_create(
    const char *name, 
    uint32_t flags,  
    unsigned long stack_size, 
    void *task_arg, 
    int (*task_function)(void*) );
```
| 参数 | 解释 |
| --- | --- |
| name | 任务名称，用于打印 |
| flags | 任务属性，目前支持`EH_TASK_FLAGS_DETACH`，若设置此属性，则任务会自动销毁，不设置任何属性时填0 |
| stack_size | 任务堆栈大小 |
| task_arg | 任务参数 |
| task_function | 任务函数 |

#### 2.使用用户提供静态栈创建任务
使用用户提供的静态堆栈创建一个任务
```
extern eh_task_t* eh_task_static_stack_create(
    const char *name, uint32_t flags, 
    void *stack, 
    unsigned long stack_size, 
    void *task_arg, 
    int (*task_function)(void*) );
```
| 参数 | 解释 |
| --- | --- |
| name | 任务名称，用于打印 |
| flags | 任务属性，目前支持`EH_TASK_FLAGS_DETACH`，若设置此属性，则任务会自动销毁，不设置任何属性时填0 |
| stack | 任务静态堆栈 |
| stack_size | 任务静态堆栈大小 |
| task_arg | 任务参数 |
| task_function | 任务函数 |

#### 3.退出任务
退出任务，在任务上下文中调用，退出任务后，若设置了`EH_TASK_FLAGS_DETACH`，将会自动销毁，若没有设置，则需要手动销毁，在其他任务中调用eh_task_join。
```
extern void  eh_task_exit(int ret);
```

#### 4.任务合并/任务回收
进行任务合并（等待任务退出并销毁），该调用为一个阻塞调用，会触发异步等待机制。成功返回0，失败返回负数。
```
extern int __async eh_task_join(eh_task_t *task, int *ret, eh_sclock_t timeout);
```
| 参数 | 解释 |
| --- | --- |
| task | 任务句柄，创建任务时获得|
| ret | 接收任务退出返回值的实例化指针，可填NULL |
| timeout | 超时时间，若为0则不超时，若为`EH_TIME_FOREVER`则一直等待，若为正数则等待指定时钟数<br>若指定ms或者us，则需要使用eh_msec_to_clock和eh_usec_to_clock包裹 |

#### 5.强制任务回收
强制回收任务，不建议直接使用,唯一应用场景只有在模块销毁时使用，非该场景时视为未定义行为。
```
extern void eh_task_destroy(eh_task_t *task);
```

#### 6.获取任务状态
获取任务状态，成功返回0，失败返回负数。
```
extern int eh_task_get_state(eh_task_t *task, eh_task_state_t *state);
```
| 参数 | 解释 |
| --- | --- |
| task | 任务句柄，创建任务时获得|
| state | 接收任务状态的实例化指针 |
```
    typedef struct eh_task_sta{
        enum EH_TASK_STATE           state;
        void*                        stack;
        unsigned long                stack_size;
        unsigned long                stack_min_ever_free_size_level;
        const char*                  task_name;
    }eh_task_state_t;
```
| 状态成员 | 解释 |
| --- | --- |
| state | 任务状态:<br> `EH_TASK_STATE_READY`:就绪<br>`EH_TASK_STATE_RUNNING`:运行<br>`EH_TASK_STATE_WAIT`:等待<br>`EH_TASK_STATE_FINISH`:结束<br>|
| stack | 任务栈起始地址 |
| stack_size | 任务栈大小 |
| stack_min_ever_free_size_level | 任务栈最小空闲大小 |
| task_name | 任务名称 |

#### 7.获取当前任务句柄
获取当前任务句柄，成功返回任务句柄
```
extern eh_task_t* eh_task_self(void);
```
#### 8.让出当前CPU时间片
让出当前CPU时间片,在CPU密集型任务处调用，提高系统实时性
```
extern void __async eh_task_yield(void);
```

### 事件相关API
#### 1.创建初始化函数
初始事件，填充结构体各成员，成功返回0，若e为NULL则返回断言结果EH_RET_INVALID_PARAM。<br>可在非协程上下文(中断上下文，其他系统线程上下文)中安全调用
```
extern __safety int eh_event_init(eh_event_t *e);
```
例子: [test/test_epoll.c](test/test_epoll.c)

#### 2.事件清理
清理事件内的等待队列，等待该事件的任务将收到 EH_RET_EVENT_ERROR
```
extern void eh_event_clean(eh_event_t *e);
```
例子: [test/test_epoll.c](test/test_epoll.c)

#### 3.事件通知
通知事件，成功返回0，若e为NULL则返回断言结果EH_RET_INVALID_PARAM。<br>可在非协程上下文(中断上下文，其他系统线程上下文)中安全调用
```
extern __safety int eh_event_notify(eh_event_t *e);
```
例子: [test/test_epoll.c](test/test_epoll.c)

#### 4.事件通知，唤醒指定个数的任务
事件通知,唤醒指定个数监听事件的任务,并重新排序，此函数作为信号量的优化，唤醒指定数量的任务，并重新排序，可优化任务唤醒的效率，避免无效唤醒，更加公平。<br>可在非协程上下文(中断上下文，其他系统线程上下文)中安全调用
```
extern __safety int eh_event_notify_and_reorder(eh_event_t *e, int num);
```
例子: [test/test_epoll.c](test/test_epoll.c)

#### 5.事件等待,且同时满足某条件
事件等待，当被唤醒时，会判断条件是否满足，若满足则返回，若不满足则重新等待。成功返回0，失败返回eh_error.h中定义的错误码。<br>
在调用此函数之前发生的事件，无法被本函数捕获到，但可以通过条件函数查询用户定义变量。<br>
本函数为异步等待函数。
```
extern int __async eh_event_wait_condition_timeout(
    eh_event_t *e, 
    void* arg, 
    bool (*condition)(void* arg), 
    eh_sclock_t timeout );
```
| 参数 | 解释 |
| --- | --- |
| e | 事件句柄 |
| arg | 用户自定义参数 |
| condition | 条件函数，返回true则表示条件满足，返回false则表示条件不满足 |
| timeout | 超时时间，若为0则不进行异步等待，若为`EH_TIME_FOREVER`则一直等待，若为正数则等待指定时钟数<br>若指定ms或者us，则需要使用eh_msec_to_clock和eh_usec_to_clock包裹 |

例子: [test/test_epoll.c](test/test_epoll.c)

#### 6.事件等待
事件等待，成功返回0，失败返回eh_error.h中定义的错误码。<br>
在调用此函数之前发生的事件，无法被本函数捕获到，但可以通过条件函数查询用户定义变量。<br>
本函数为异步等待函数。
```
static inline int __async eh_event_wait_timeout(eh_event_t *e, eh_sclock_t timeout)
```
| 参数 | 解释 |
| --- | --- |
| e | 事件句柄 |
| timeout | 超时时间，若为0则不进行异步等待，若为`EH_TIME_FOREVER`则一直等待，若为正数则等待指定时钟数<br>若指定ms或者us，则需要使用eh_msec_to_clock和eh_usec_to_clock包裹 |

例子: [test/test_epoll.c](test/test_epoll.c)

#### 7.创建一个epoll句柄
创建一个epoll句柄，返回值需要使用eh_ptr_to_error转换为错误码，若错误码为0则成功，为负数则失败。<br>可在非协程上下文(中断上下文，其他系统线程上下文)中安全调用
```
extern __safety eh_epoll_t eh_epoll_new(void);
```
例子: [test/test_epoll.c](test/test_epoll.c)

#### 8.销毁一个epoll句柄
关闭一个epoll句柄。<br>可在非协程上下文(中断上下文，其他系统线程上下文)中安全调用
```
extern __safety void eh_epoll_del(eh_epoll_t epoll);
```
例子: [test/test_epoll.c](test/test_epoll.c)
#### 9.添加一个事件到epoll中
添加一个事件到epoll中，成功返回0，失败返回eh_error.h中定义的错误码。
```
extern int eh_epoll_add_event(eh_epoll_t epoll, eh_event_t *e, void *userdata);
```
| 参数 | 解释 |
| --- | --- |
| epoll | epoll句柄 |
| e | 事件句柄 |
| userdata | 用户自定义参数 |

例子: [test/test_epoll.c](test/test_epoll.c)

#### 10.从epoll中删除一个事件
从epoll中删除一个事件，成功返回0，失败返回eh_error.h中定义的错误码。
```
extern int eh_epoll_del_event(eh_epoll_t epoll,eh_event_t *e);
```
| 参数 | 解释 |
| --- | --- |
| epoll | epoll句柄 |
| e | 事件句柄 |

例子: [test/test_epoll.c](test/test_epoll.c)

#### 11.epoll等待事件
epoll等待事件，成功返回等待到事件的数量,失败返回eh_error.h中定义的错误码。
```
extern int __async eh_epoll_wait(
    eh_epoll_t epoll,
    eh_epoll_slot_t *epool_slot, 
    int slot_size, 
    eh_sclock_t timeout );
```
| 参数 | 解释 |
| --- | --- |
| epoll | epoll句柄 |
| epool_slot | epoll槽，用于存放epoll等待到的事件 |
| slot_size | epoll槽大小 |
| timeout | 超时时间，若为0则不进行异步等待，若为`EH_TIME_FOREVER`则一直等待，若为正数则等待指定时钟数<br>若指定ms或者us，则需要使用eh_msec_to_clock和eh_usec_to_clock包裹 |

例子: [test/test_epoll.c](test/test_epoll.c)

### 系统事件，定时器事件相关API
#### 1.定时器初始化（全参数版）
填充结构体内容<br>可在非协程上下文(中断上下文，其他系统线程上下文)中安全调用
```
extern __safety int eh_timer_advanced_init(eh_timer_event_t *timer, eh_sclock_t clock_interval, uint32_t attr);
```
| 参数 | 解释 |
| --- | --- |
| timer | 定时器句柄 |
| clock_interval | 定时器间隔，单位为时钟数 |
| attr | 定时器属性，默认为0，属性之间使用'\|'组合<br>`EH_TIMER_ATTR_AUTO_CIRCULATION`: 表示定时器为重复定时器<br>`EH_TIMER_ATTR_NOW_TIME_BASE`: 当EH_TIMER_ATTR_AUTO_CIRCULATION有效时,装载时以当前时间为基准 |

#### 2.定时器初始化（简化版）
填充结构体内容<br>可在非协程上下文(中断上下文，其他系统线程上下文)中安全调用
```
static __safety inline int eh_timer_init(eh_timer_event_t *timer);
```
例子: [test/test_epoll.c](test/test_epoll.c)

#### 3.启动定时器
将定时器加入系统定时器树中，成功返回0，失败返回eh_error.h中定义的错误码。
```
extern int eh_timer_start(eh_timer_event_t *timer);
```
例子: [test/test_epoll.c](test/test_epoll.c)

#### 4.停止定时器
将定时器从系统定时器树中移除，成功返回0，失败返回eh_error.h中定义的错误码。
```
extern int eh_timer_stop(eh_timer_event_t *timer);
```

#### 5.定时器重新启动
将定时器重新加入系统定时器树中，若已经start则重新设置到期时间，成功返回0，失败返回eh_error.h中定义的错误码。
```
extern int eh_timer_restart(eh_timer_event_t *timer);
```

#### 6.定时器清理
清理定时器，把本定时器从系统定时器树中移除(eh_timer_stop)，内部调用eh_event_clean函数<br>可在非协程上下文(中断上下文，其他系统线程上下文)中安全调用
```
extern __safety void eh_timer_clean(eh_timer_event_t *timer);
```
例子: [test/test_epoll.c](test/test_epoll.c)


#### 7.设置定时器超时时间
设置定时器超时时间
```
#define  eh_timer_config_interval(timer, clock_interval)
```
| 参数 | 解释 |
| --- | --- |
| timer | 定时器句柄 |
| clock_interval | 定时器间隔，单位为时钟数,若指定ms或者us，则需要使用eh_msec_to_clock和eh_usec_to_clock包裹 |

例子: [test/test_epoll.c](test/test_epoll.c)


#### 8.设置定时器属性
设置定时器属性
```
#define eh_timer_set_attr(timer, attr)
```
| 参数 | 解释 |
| --- | --- |
| timer | 定时器句柄 |
| attr | 定时器属性，默认为0，属性之间使用'\|'组合<br>`EH_TIMER_ATTR_AUTO_CIRCULATION`: 表示定时器为重复定时器<br>`EH_TIMER_ATTR_NOW_TIME_BASE`: 当EH_TIMER_ATTR_AUTO_CIRCULATION有效时,装载时以当前时间为基准 |

例子: [test/test_epoll.c](test/test_epoll.c)

#### 9.计算定时器剩余时间
计算定时器剩余时钟数,为正数时表示还有时间，为负数时表示已经到期
```
#define eh_remaining_time(now_time, timer_ptr)
```
| 参数 | 解释 |
| --- | --- |
| now_time | 当前时间,典型值:eh_get_clock_monotonic_time() |
| timer_ptr | 定时器句柄 |

#### 10.衍生睡眠函数
睡眠函数
```
extern void __async eh_usleep(eh_usec_t usec);
```
| 参数 | 解释 |
| --- | --- |
| usec | 睡眠时间，单位为微秒 |

#### 10.获取定时器运行状态
获取定时器运行状态，运行返回true,否则返回false<br>安全函数，可在其他并行或并发任务中安全调用
```
extern __safety bool eh_timer_is_running(eh_timer_event_t *timer);
```

### 信号和槽相关API
此部分api必须举例说明，请查看以下例子
#### 1.通用信号使用例子（私有信号）
```
/* test_signal_private.c */
...  
/*  包含相关头文件  */

/* 定义测试的信号 */
EH_STATIC_SIGNAL(test_signal);

/* 定义槽函数 */
static void slot_test_function(eh_event_t *e, void *slot_param){
    eh_infoln("slot_test_function %s\n", (char *)slot_param);
}

/* 定义第一个槽，一个槽同时只能被一个信号所连接 */
EH_DEFINE_SLOT(
    test_signal_slot1, 
    slot_test_function,
    "test1" /* 用户参数 */
);


/* 定义第二个槽，一个槽同时只能被一个信号所连接 */
EH_DEFINE_SLOT(
    test_signal_slot2, 
    slot_test_function,
    "test2"
);

void run(void){
    /* 注册信号 */
    eh_signal_register(&test_signal);

    /* 连接信号和槽 */
    eh_signal_slot_connect(&test_signal, &test_signal_slot1);
    eh_signal_slot_connect(&test_signal, &test_signal_slot2);

    while(1){
        /* 每秒触发一次信号 */
        eh_usleep(1000*1000);

        /* 下面两种都可以，使用一样的原理，触发信号后槽函数可以正常执行 */
        //eh_event_notify(eh_signal_to_custom_event(&test_signal));
        eh_signal_notify(&test_signal);
    }

    /* 反初始化是一个好习惯 */
    eh_signal_slot_disconnect(&test_signal_slot2);
    eh_signal_slot_disconnect(&test_signal_slot1);
    eh_signal_unregister(&test_signal);

}

```
#### 2.通用信号使用例子（公共信号）
```
/* test_signal_public.h */
/* 头文件中声明信号，让其他模块可以引用 */
EH_EXTERN_SIGNAL(test_signal);

```

```
/* test_signal_public.c */
#include "eh.h"
#include "eh_signal.h"
#include "test_signal_public.h"

EH_DEFINE_SIGNAL(test_signal);

/* 假设这里会产生一个硬件中断 */
void test_signal_public_trigger(void){
    eh_signal_notify(&test_signal);
}


static int __init test_signal_public_init(void){
    /* 注册信号 */
    eh_signal_register(&test_signal);
}

static void __exit test_signal_public_exit(void){
    /* 注销信号 */
    eh_signal_unregister(&test_signal);
}


eh_module_level0_export(test_signal_public_init, test_signal_public_exit);

```
```
/* main.c */
#include "eh.h"
#include "eh_signal.h"
#include "test_signal_public.h"


/* 定义槽函数 */
static void slot_test_function(eh_event_t *e, void *slot_param){
    eh_infoln("slot_test_function %s\n", (char *)slot_param);
}

/* 定义第一个槽，一个槽同时只能被一个信号所连接 */
EH_DEFINE_SLOT(
    test_signal_slot1, 
    slot_test_function,
    "test1" /* 用户参数 */
);


/* 定义第二个槽，一个槽同时只能被一个信号所连接 */
EH_DEFINE_SLOT(
    test_signal_slot2, 
    slot_test_function,
    "test2"
);

void run(void){

    /* 连接信号和槽 */
    eh_signal_slot_connect(&test_signal, &test_signal_slot1);
    eh_signal_slot_connect(&test_signal, &test_signal_slot2);
    while(1){
        eh_usleep(1000*1000);
    }
    /* 反初始化是一个好习惯 */
    eh_signal_slot_disconnect(&test_signal_slot2);
    eh_signal_slot_disconnect(&test_signal_slot1);

}

```

#### 3.自定义信号使用例子（私有信号）
自定义信号必须使用自定义事件进行填充，自定义事件结构体的第一个成员必须是eh_event_t结构体,这里使用定时器事件作为测试对象
```
/* test_custom_event.c */

/* 定义一个自定义信号（定时器信号），第二个参数是事件的类型，这里使用定时器事件，第三个参数是定时器事件的初始化 */
EH_DEFINE_STATIC_CUSTOM_SIGNAL(
    timer_1000ms_signal, 
    eh_timer_event_t, 
    {}
);

/* 定义槽函数 */
static void slot_test_function(eh_event_t *e, void *slot_param){
    eh_infoln("slot_test_function %s\n", (char *)slot_param);
}

/* 定义第一个槽，一个槽同时只能被一个信号所连接 */
EH_DEFINE_SLOT(
    test_signal_slot1, 
    slot_test_function,
    "test1" /* 用户参数 */
);


/* 定义第二个槽，一个槽同时只能被一个信号所连接 */
EH_DEFINE_SLOT(
    test_signal_slot2, 
    slot_test_function,
    "test2"
);

void run(void){

    /* 必须先初始化定时器事件的那一部分 */
    eh_timer_advanced_init(
        eh_signal_to_custom_event(&timer_1000ms_signal), 
        (eh_sclock_t)eh_msec_to_clock(1000), 
        EH_TIMER_ATTR_AUTO_CIRCULATION
    );

    /* 然后在注册信号 */
    eh_signal_register(&timer_1000ms_signal);

    /* 连接信号和槽 */
    eh_signal_slot_connect(&timer_1000ms_signal, &test_signal_slot1);
    eh_signal_slot_connect(&timer_1000ms_signal, &test_signal_slot2);

    /* 启动定时器 */
    eh_timer_start(eh_signal_to_custom_event(&timer_1000ms_signal));

    while(1){
        eh_usleep(1000*1000);
    }

    /* 反初始化是一个好习惯 */

    /* 停止定时器 */
    eh_timer_stop(eh_signal_to_custom_event(&timer_1000ms_signal));

    eh_signal_slot_disconnect(&timer_1000ms_signal);
    eh_signal_slot_disconnect(&timer_1000ms_signal);

    eh_signal_unregister(&timer_1000ms_signal);

    eh_signal_clean(&timer_1000ms_signal);
}

```

#### 3.自定义信号使用例子（公有信号）
```
/* test_custom_event.h */
/* 头文件中声明信号，让其他模块可以引用 */
EH_EXTERN_CUSTOM_SIGNAL(timer_1000ms_signal, eh_timer_event_t);
```

```
/* test_custom_event.c */
#include "eh.h"
#include "eh_signal.h"
#include "test_custom_event.h"

EH_EXTERN_CUSTOM_SIGNAL(timer_1000ms_signal, eh_timer_event_t, {});


static int __init test_signal_public_init(void){
    /* 必须先初始化定时器事件的那一部分 */
    eh_timer_advanced_init(
        eh_signal_to_custom_event(&timer_1000ms_signal), 
        (eh_sclock_t)eh_msec_to_clock(1000), 
        EH_TIMER_ATTR_AUTO_CIRCULATION
    );

    /* 然后在注册信号 */
    eh_signal_register(&timer_1000ms_signal);

    /* 启动定时器 */
    eh_timer_start(eh_signal_to_custom_event(&timer_1000ms_signal));

    return 0;
}

static void __exit test_signal_public_exit(void){
    /* 停止定时器 */
    eh_timer_stop(eh_signal_to_custom_event(&timer_1000ms_signal));
    eh_signal_unregister(&timer_1000ms_signal);
    eh_signal_clean(&timer_1000ms_signal);
}


eh_module_level0_export(test_signal_public_init, test_signal_public_exit);

```

```
/* main.c */
#include "eh.h"
#include "eh_signal.h"
#include "test_custom_event.h"


/* 定义槽函数 */
static void slot_test_function(eh_event_t *e, void *slot_param){
    eh_infoln("slot_test_function %s\n", (char *)slot_param);
}

/* 定义第一个槽，一个槽同时只能被一个信号所连接 */
EH_DEFINE_SLOT(
    test_signal_slot1, 
    slot_test_function,
    "test1" /* 用户参数 */
);


/* 定义第二个槽，一个槽同时只能被一个信号所连接 */
EH_DEFINE_SLOT(
    test_signal_slot2, 
    slot_test_function,
    "test2"
);

void run(void){

    /* 连接信号和槽 */
    eh_signal_slot_connect(&timer_1000ms_signal, &test_signal_slot1);
    eh_signal_slot_connect(&timer_1000ms_signal, &test_signal_slot2);
    while(1){
        eh_usleep(1000*1000);
    }
    /* 反初始化是一个好习惯 */
    eh_signal_slot_disconnect(&test_signal_slot2);
    eh_signal_slot_disconnect(&test_signal_slot1);

}
```

### 互斥锁相关API
虽然协程大部分情况下是不需要锁的
但是，在宏观资源面前，还是存在加锁的，比如,
在遍历链表时调用__async类型函数，在其他任务上就
可能操作到该链表,导致链表被两个任务同时访问到，所以
在非必要情况，是用不到本模块的。
此模块所有的函数都只能在协程上下文中使用，
也就是说，不要在中断上下文，或者其他线程（posix线程，系统线程）上下文中调用本函数

#### 1.创建互斥锁
创建互斥锁，成功返回互斥锁句柄，返回值需要使用eh_ptr_to_error转换为错误码，若错误码为0则成功，为负数则失败。
```
extern eh_mutex_t eh_mutex_create(enum eh_mutex_type type);
```
| 参数 | 解释 |
| --- | --- |
| type | 互斥锁类型<br>`EH_MUTEX_TYPE_NORMAL`:正常互斥锁<br>`EH_MUTEX_TYPE_RECURSIVE`:递归互斥锁(获得锁的任务可重复获得) |

#### 2.销毁互斥锁
销毁互斥锁
```
extern void eh_mutex_destroy(eh_mutex_t mutex);
```

#### 3.互斥锁加锁
加锁，成功返回0，失败返回eh_error.h中定义的错误码。
```
extern int __async eh_mutex_lock(eh_mutex_t mutex, eh_sclock_t timeout);
```
| 参数 | 解释 |
| --- | --- |
| mutex | 互斥锁句柄 |
| timeout | 超时时间，若为0则不进行异步等待，若为`EH_TIME_FOREVER`则一直等待，若为正数则等待指定时钟数<br>若指定ms或者us，则需要使用eh_msec_to_clock和eh_usec_to_clock包裹 |


#### 4.互斥锁解锁
解锁，成功返回0，失败返回eh_error.h中定义的错误码。
```
extern int __async eh_mutex_unlock(eh_mutex_t mutex);
```

### 信号量相关API
任务信号量的实现，虽然事件也提供了类似与休眠唤醒的功能，
但是事件只能保证多次set必有一次触发，无法保证多次触发，
而信号量的实现能增加可靠性, 信号量的实现继承了event的相关特性，
也能使用event相关特性，比如epoll

#### 1.创建信号量
创建信号量，成功返回信号量句柄，返回值需要使用eh_ptr_to_error转换为错误码，若错误码为0则成功，为负数则失败。<br>可在非协程上下文(中断上下文，其他系统线程上下文)中安全调用
```
extern __safety eh_sem_t eh_sem_create(uint32_t value);
```
| 参数 | 解释 |
| --- | --- |
| value | 初始信号量值 |

#### 2.销毁信号量
销毁信号量<br>可在非协程上下文(中断上下文，其他系统线程上下文)中安全调用
```
extern __safety void eh_sem_destroy(eh_sem_t sem);
```

#### 3.信号量释放
释放信号量，成功返回0，失败返回eh_error.h中定义的错误码。<br>可在非协程上下文(中断上下文，其他系统线程上下文)中安全调用
```
extern __safety int eh_sem_post(eh_sem_t sem);
```

#### 4.信号量获取
获取信号量，成功返回0，失败返回eh_error.h中定义的错误码。
```
extern int __async eh_sem_wait(eh_sem_t sem, eh_sclock_t timeout);
```
| 参数 | 解释 |
| --- | --- |
| sem | 信号量句柄 |
| timeout | 超时时间，若为0则不进行异步等待，若为`EH_TIME_FOREVER`则一直等待，若为正数则等待指定时钟数<br>若指定ms或者us，则需要使用eh_msec_to_clock和eh_usec_to_clock包裹 |

#### 5.获取信号量的事件句柄
获取信号量的事件句柄，获取event句柄后，可以使用event相关API进行操作，比如通过epoll进行事件监听
```
#define eh_sem_get_event(sem)   ((eh_event_t*)sem)
```

### 格式化输出API
支持浮点、字符串、十六进制、二进制、八进制、字符、指针输出
支持+-符号输出、宽度输出、精度输出、左对齐输出、右对齐输出、填充输出
```
extern int eh_vprintf(const char *fmt, va_list args);
extern int eh_printf(const char *fmt, ...);
extern int eh_snprintf(char *buf, size_t size, const char *fmt, ...);
extern int eh_sprintf(char *buf, const char *fmt, ...);
```

### DEBUG输出API
```
/* 带自动回车的版本 */
eh_debugln(fmt, ...) 
eh_infoln(fmt, ...)  
eh_sysln(fmt, ...)   
eh_warnln(fmt, ...)  
eh_errln(fmt, ...)   

/* 带自动回车，带函数定位 */
eh_debugfl(fmt, ...) 
eh_infofl(fmt, ...)  
eh_sysfl(fmt, ...)   
eh_warnfl(fmt, ...)  
eh_errfl(fmt, ...)

/* 原始数据版本 */
eh_debugraw(fmt, ...)
eh_inforaw(fmt, ...) 
eh_sysraw(fmt, ...)  
eh_warnraw(fmt, ...) 
eh_errraw(fmt, ...)

/* 16进制数组打印 */
eh_debughex(buf,len) 
eh_infohex(buf,len)  
eh_syshex(buf,len)   
eh_warnhex(buf,len)  
eh_errhex(buf,len)   

```
#### 内存管理API
#### 1.堆空间注册
将某一段内存注册为堆空间,同一段堆空间全局只需注册一次，在eh_global_init之前调用
```
extern int eh_mem_heap_register(const struct eh_mem_heap *heap);
```
例子:[src/eh_mem.c](src/eh_mem.c)

#### 2.堆空间申请
```
void* eh_malloc(size_t _size)
```

#### 3.堆空间释放
```
void eh_free(void *ptr)
```

#### 4.获取堆空间信息
```
extern void eh_mem_get_heap_info(struct eh_mem_heap_info *heap_info);
```
| 参数 | 解释 |
| --- | --- |
| heap_info | 堆空间信息结构体 |

```
struct eh_mem_heap_info{
    size_t total_size;                          /* 总大小 */
    size_t free_size;                           /* 空闲大小 */
    size_t min_ever_free_size_level;            /* 曾经最小空闲大小 */
};
```