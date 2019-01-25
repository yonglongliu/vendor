
#ifndef __RING_BUFFER_H
#define __RING_BUFFER_H

#include <stdlib.h>


struct ringbuffer;


void ringbuffer_free(struct ringbuffer *ring_buf);
struct ringbuffer *ringbuffer_copy(struct ringbuffer *ringbuffer_orig);
struct ringbuffer *ringbuffer_init(uint32_t size);
uint32_t ringbuffer_data_bytes(struct ringbuffer *ring_buf);
uint32_t ringbuffer_buf_bytes(struct ringbuffer *ring_buf);
int32_t ringbuffer_write(struct ringbuffer *ring_buf, void *buffer, uint32_t size);
int32_t ringbuffer_read(struct ringbuffer *ring_buf, uint8_t *buffer,uint32_t size);
int32_t ringbuffer_consume(struct ringbuffer *ring_buf,uint32_t size);
int32_t ringbuffer_peek(struct ringbuffer *ring_buf, uint8_t *buffer,uint32_t offset, uint32_t size);
int32_t ringbuffer_reset(struct ringbuffer *ring_buf);
int32_t ringbuffer_total_write(struct ringbuffer *ring_buf);


#endif
