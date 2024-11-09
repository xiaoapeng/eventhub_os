/**
 * @file eh_ringbuf.c
 * @brief   环形缓冲区实现，单读单写无锁环形缓冲区
 * @author simon.xiaoapeng (simon.xiaoapeng@gmail.com)
 * @date 2024-07-23
 * 
 * @copyright Copyright (c) 2024  simon.xiaoapeng@gmail.com
 * 
 */
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <eh_types.h>
#include <eh_error.h>
#include <eh_mem.h>
#include "eh_ringbuf.h"


#define eh_ringbuf_fix(ringbuf, pos)            ((pos)%((uint32_t)(ringbuf->size << 1)))

eh_ringbuf_t* eh_ringbuf_create(int32_t size, uint8_t *static_buf_or_null){
    eh_ringbuf_t* ringbuf;
    if(size <= 0)
        return eh_error_to_ptr(EH_RET_INVALID_PARAM);
    if(static_buf_or_null == NULL){
        ringbuf = eh_malloc(sizeof(eh_ringbuf_t) + (size_t)size);
        static_buf_or_null = (uint8_t*)(ringbuf + 1);
    }else{
        ringbuf = eh_malloc(sizeof(eh_ringbuf_t));
    }
    if(ringbuf == NULL)
        return eh_error_to_ptr(EH_RET_MALLOC_ERROR);
    ringbuf->buf = static_buf_or_null;
    ringbuf->r = ringbuf->w = 0;
    ringbuf->size = size;
    return ringbuf;
}

void eh_ringbuf_destroy(eh_ringbuf_t *ringbuf){
    eh_free(ringbuf);
}

int32_t eh_ringbuf_size(eh_ringbuf_t *ringbuf){
    uint32_t w = ringbuf->w;
    uint32_t r = ringbuf->r;
    int32_t diff = (int)(w - r);
    return diff >=0 ? diff : (ringbuf->size << 1) + diff;
}

int32_t eh_ringbuf_free_size(eh_ringbuf_t *ringbuf){
    uint32_t w = ringbuf->w;
    uint32_t r = ringbuf->r;
    int32_t diff = (int)(w - r);
    return diff >=0 ? ringbuf->size - diff : (- (ringbuf->size + diff));
}


int32_t eh_ringbuf_write(eh_ringbuf_t *ringbuf, const uint8_t *buf, int32_t len){
    uint32_t w;
    int32_t free_size = eh_ringbuf_free_size(ringbuf);
    int32_t write_size_first_max,wl;
    wl = len > free_size ? free_size : len;
    if(wl <= 0) return 0;
    w = ringbuf->w % (uint32_t)ringbuf->size;
    write_size_first_max = ringbuf->size - (int32_t)w;
    if(wl <= write_size_first_max){
        memcpy(ringbuf->buf + w, buf, (size_t)wl);
    }else{
        memcpy(ringbuf->buf + w, buf, (size_t)write_size_first_max);
        memcpy(ringbuf->buf, buf + write_size_first_max, (size_t)(wl - write_size_first_max));
    }
    eh_memory_order_release_barrier();
    ringbuf->w = eh_ringbuf_fix(ringbuf, ringbuf->w + (uint32_t)wl);
    return wl;
}

int32_t eh_ringbuf_read(eh_ringbuf_t *ringbuf, uint8_t *buf, int32_t len){
    uint32_t r;
    int32_t size = eh_ringbuf_size(ringbuf);
    int32_t read_size_first_max, rl;
    rl = len > size ? size : len;
    if(rl <= 0) return 0;
    r = ringbuf->r % (uint32_t)ringbuf->size;
    read_size_first_max = ringbuf->size - (int32_t)r;
    if(rl <= read_size_first_max){
        memcpy(buf, ringbuf->buf + r, (size_t)rl);
    }else{
        memcpy(buf, ringbuf->buf + r, (size_t)read_size_first_max);
        memcpy(buf + read_size_first_max, ringbuf->buf, (size_t)(rl - read_size_first_max));
    }
    
    eh_memory_order_release_barrier();
    ringbuf->r = eh_ringbuf_fix(ringbuf, ringbuf->r + (uint32_t)rl);
    return rl;
}

int32_t eh_ringbuf_read_skip(eh_ringbuf_t *ringbuf, int32_t len){
    int32_t size = eh_ringbuf_size(ringbuf);
    int32_t rl;
    rl = len > size ? size : len;
    if(rl <= 0) return 0;
    eh_memory_order_release_barrier();
    ringbuf->r = eh_ringbuf_fix(ringbuf, ringbuf->r + (uint32_t)rl);
    return rl;
}


const uint8_t* eh_ringbuf_peek(eh_ringbuf_t *ringbuf, int32_t offset, uint8_t *buf, int32_t *len){
    uint32_t r;
    int32_t size;
    int32_t read_size_first_max;
    int32_t rl;

    /* 为了性能，这里不进行负数的判断 */

    size = eh_ringbuf_size(ringbuf) - offset;
    rl = *len;
    if(size < rl ) return NULL; /* 数量不足，禁止偷看 */
    r = (ringbuf->r + (uint32_t)offset)%(uint32_t)ringbuf->size;
    read_size_first_max = ringbuf->size - (int32_t)r;

    if(rl <= read_size_first_max){
        /* 0拷贝情况，皆大欢喜 */
        *len = size > read_size_first_max ? read_size_first_max : size;
        return ringbuf->buf + r;
    }else{
        memcpy(buf, ringbuf->buf + r, (size_t)read_size_first_max);
        memcpy(buf + read_size_first_max, ringbuf->buf, (size_t)(rl - read_size_first_max));
        return buf;
    }
}

void eh_ringbuf_clear(eh_ringbuf_t *ringbuf){
    eh_memory_order_release_barrier();
    ringbuf->r = ringbuf->w;
}

void eh_ringbuf_reset(eh_ringbuf_t *ringbuf){
    ringbuf->r = ringbuf->w = 0;
}






