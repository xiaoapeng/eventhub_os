/**
 * @file eh_ringbuf.h
 * @brief 环形缓冲区实现，单读单写无锁环形缓冲区
 * @author simon.xiaoapeng (simon.xiaoapeng@gmail.com)
 * @date 2024-07-23
 * 
 * @copyright Copyright (c) 2024  simon.xiaoapeng@gmail.com
 * 
 */
#ifndef _EH_RINGBUF_H_
#define _EH_RINGBUF_H_

#include <stdint.h>

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */

typedef struct eh_ringbuf{
    uint32_t w;
    uint32_t r;
    int32_t  size;
    uint8_t *buf;
}eh_ringbuf_t;

/**
 * @brief                           创建环形缓冲区
 * @param  size                     环形缓冲区大小,要求在正数范围内，因为使用镜像法缓冲区
                                    模块中所有的int32_t都是提醒使用者注意范围
 * @param  static_buf_or_null       静态缓冲区指针，如果为NULL则动态分配内存
 * @return eh_ringbuf_t*            返回值请使用eh_ptr_to_error来判断是否创建成功
 */
extern eh_ringbuf_t* eh_ringbuf_create(int32_t size, uint8_t *static_buf_or_null);

/**
 * @brief                           销毁环形缓冲区
 * @param  ringbuf                  环形缓冲区指针
 */
extern void eh_ringbuf_destroy(eh_ringbuf_t *ringbuf);

/**
 * @brief                           获取环形缓冲区剩余空间
 * @param  ringbuf                  环形缓冲区指针
 * @return int32_t                 返回剩余空间
 */
extern int32_t eh_ringbuf_free_size(eh_ringbuf_t *ringbuf);

/**
 * @brief                           获取环形缓冲区已用大小
 * @param  ringbuf                  环形缓冲区指针
 * @return int32_t                 返回环形缓冲区已用大小
 */
extern int32_t eh_ringbuf_size(eh_ringbuf_t *ringbuf);

/**
 * @brief                           获取环形缓冲区总量
 * @param  ringbuf                  环形缓冲区指针
 * @return int32_t                 返回环形缓冲区总量
 */
static inline int32_t eh_ringbuf_total_size(eh_ringbuf_t *ringbuf){
    return ringbuf->size;
}

/**
 * @brief                           写入环形缓冲区
 * @param  ringbuf                  环形缓冲区指针
 * @param  buf                      缓冲区指针
 * @param  len                      要写的环形缓冲区长度
 * @return int32_t                  返回写入成功的数量
 */
extern int32_t eh_ringbuf_write(eh_ringbuf_t *ringbuf, const uint8_t *buf, int32_t len);

/**
 * @brief                           读取环形缓冲区
 * @param  ringbuf                  环形缓冲区指针
 * @param  buf                      要读到的缓冲区指针
 * @param  len                      要读的环形缓冲区长度
 * @return int32_t                  返回读到的数量
 */
extern int32_t eh_ringbuf_read(eh_ringbuf_t *ringbuf, uint8_t *buf, int32_t len);

/**
 * @brief                           跳过读环形缓冲区
 * @param  ringbuf                  环形缓冲区指针
 * @param  len                      要跳过的环形缓冲区长度
 * @return int32_t                  返回跳过的数量
 */
extern int32_t eh_ringbuf_read_skip(eh_ringbuf_t *ringbuf, int32_t len);

/**
 * @brief                           偷看环形缓冲区,当数据未绕回时自动触发0拷贝，当数据饶回时触发拷贝进所传入的buf中
 * @param  ringbuf                  环形缓冲区指针
 * @param  offset                   从哪开始偷看
 * @param  buf                      要读到的缓冲区指针
 * @param  len                      作为输入参数时为buf的长度，作为输出参数时为访问uint8_t 的长度,
 *                                      当传入 *len == 0 时表示不需要buf指针百分百触发0拷贝
 * @return const uint8_t*           当前缓冲区没有足够数量时返回NULL
 *                                  当数量足够且需要绕回时进行memcpy后返回buf
 *                                  当数量足够且不需要绕回时直接返回内部缓冲区指针(0拷贝)
 */
extern const uint8_t* eh_ringbuf_peek(eh_ringbuf_t *ringbuf, int32_t offset, uint8_t *buf, int32_t *len);

/**
 * @brief                           偷看环形缓冲区,并将数据拷贝到buf中
 * @param  ringbuf                  环形缓冲区指针
 * @param  offset                   从哪开始偷看
 * @param  buf                      要读到的缓冲区指针
 * @param  len                      作为输入参数时为buf的长度
 * @return int32_t                  返回拷贝到的数量
 */
extern int32_t eh_ringbuf_peek_copy(eh_ringbuf_t *ringbuf, int32_t offset, uint8_t *buf, int32_t len);

/**
 * @brief                           清空环形缓冲区(单读写安全)
 * @param  ringbuf                  环形缓冲区指针
 */
extern void eh_ringbuf_clear(eh_ringbuf_t *ringbuf);


/**
 * @brief                           清空环形缓冲区,并设置wr到0（单读写不安全）
 * @param  ringbuf                  环形缓冲区指针
 */
extern void eh_ringbuf_reset(eh_ringbuf_t *ringbuf);



#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif // _EH_RINGBUF_H_