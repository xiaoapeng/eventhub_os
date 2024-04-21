

// /* @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ */
// /* #################################################### signal and slot 相关定义 ######################################################## */
// /* @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ */


// struct eh_signal{
//     void                            *signal_param;
//     eh_signal_type_t                *signal_type;
// };

// struct eh_slot{
//     struct eh_list_head             list_node;
//     void                            *slot_userdata;
//     void                            (*slot_function)(eh_signal_t *signal, void  *slot_userdata);
// };

// struct eh_signal_type{
//     struct eh_list_head             slot_function_head;                                     /* 槽函数链表 */
//     void                            (*signal_param_release_handler)(void *signal_param);    /* 信号参数释放函数 */
//     enum EH_PRIORITY          priority:5;                                             /* 信号被处理的优先级 */
// };

// /**
//  * @brief 定义可靠信号，可靠信号将进入队列，可以保证所有信号不会丢失
//  * @param signal_name                    信号名称
//  * @param param_release_handler          信号参数释放函数,为NULL时不进行参数释放调用
//  * @param priority_num                   信号的优先级,高优先级的信号将被优先处理，同优先级的信号被先注册的先处理
//  */
// #define EH_SIGNAL_RELIABLE_DEFINE(signal_name, param_release_handler, priority_num) \
//     eh_signal_type_t signal_name = { \
//         .global_signal_node = EH_LIST_HEAD_INIT(signal_name.global_signal_node), \
//         .slot_function_head = EH_LIST_HEAD_INIT(signal_name.slot_function_head), \
//         .signal_param_release_handler = param_release_handler, \
//         .priority = priority_num, \
//     }
// /**
//  * @brief 信号导出，当信号被导出后，其他模块可以引用该信号，可以对相应槽进行连接
//  */
// #define EH_SIGNAL_EXPORT(signal_name)   \
//     extern eh_signal_type_t signal_name;

// /* @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ */
// /* #################################################### signal and slot 定义结束 ######################################################## */
// /* EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE */

