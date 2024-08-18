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
| i7-10700                   | linux             | gcc 11.4.0                     |                                 |
| stm32f030c8t6              | Cortex-M0        | arm-none-eabi-gcc  10.3.1      | 理论可支持所有的M0核               |
| stm32f103c6t6              | Cortex-M3        | arm-none-eabi-gcc  10.3.1      | 理论可支持所有的M3核               |
| stm32f401vbt6              | Cortex-M4         | arm-none-eabi-gcc  10.3.1      | 理论可支持所有的M4核              |
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

### 3. 修改链接脚本(linux平台忽略)
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

### 4. 配置eh_user_config.h

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
TODO


















