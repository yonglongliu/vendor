
#include "ringbuffer.h"
#include <errno.h>
#include <cutils/log.h>
#include <string.h>


#define min(a, b) (((a) < (b)) ? (a) : (b))

struct ringbuffer {
    uint8_t         *buffer;
    uint32_t     size;
    uint32_t     in;
    uint32_t     out;
    uint32_t is_power_2;
    uint8_t  is_copy;
};



static  int is_power_of_2(uint32_t n)
{
    return (n>0)&&(n &(n-1) == 0);
}


 void ringbuffer_free(struct ringbuffer *ring_buf)
{
    if (NULL != ring_buf) {
        if (ring_buf->buffer) {
            if(ring_buf->is_copy == 0) {
                free(ring_buf->buffer);
            }
            ring_buf->buffer = NULL;
        }
        free(ring_buf);
        ring_buf = NULL;
    }
}

 struct ringbuffer *ringbuffer_copy(struct ringbuffer *ringbuffer_orig)
{
    void *buffer = NULL;
    struct ringbuffer *ring_buf = NULL;

    if(!ringbuffer_orig) {
        return NULL;
    }

    ring_buf = (struct ringbuffer *)malloc(sizeof(struct ringbuffer));
    if (!ring_buf) {
        ALOGE("Failed to malloc ringbuffer,errno:%u,%s",
              errno, strerror(errno));
        goto err;
    }
    memset(ring_buf, 0, sizeof(struct ringbuffer));
    ring_buf->buffer = ringbuffer_orig->buffer;
    ring_buf->size = ringbuffer_orig->size;
    ring_buf->in = ringbuffer_orig->in;
    ring_buf->out = ringbuffer_orig->out;
    ring_buf->is_power_2 = ringbuffer_orig->is_power_2;
    ring_buf->is_copy = 1;
    
    return ring_buf;

err:
    ringbuffer_free(ring_buf);
    return NULL;
}



 struct ringbuffer *ringbuffer_init(uint32_t size)
{
    void *buffer = NULL;
    struct ringbuffer *ring_buf = NULL;

    buffer = (void *)malloc(size);
    if (!buffer) {
        ALOGE("Failed to malloc memory");
        return NULL;
    }
    memset(buffer, 0, size);

    ring_buf = (struct ringbuffer *)malloc(sizeof(struct ringbuffer));
    if (!ring_buf) {
        ALOGE("Failed to malloc ringbuffer,errno:%u,%s",
              errno, strerror(errno));
        goto err;
    }
    memset(ring_buf, 0, sizeof(struct ringbuffer));
    ring_buf->buffer = buffer;
    ring_buf->size = size;
    ring_buf->in = 0;
    ring_buf->out = 0;
    ring_buf->is_power_2 = is_power_of_2(size);
    
    return ring_buf;

err:
    ringbuffer_free(ring_buf);
	free(buffer);
    return NULL;
}

 uint32_t ringbuffer_data_bytes(struct ringbuffer *ring_buf)
{
    if(ring_buf == NULL) {
        return 0;
    }
    return (ring_buf->in - ring_buf->out);

}

 uint32_t ringbuffer_buf_bytes(struct ringbuffer *ring_buf)
{
    if(ring_buf == NULL) {
        return 0;
    }
    return (ring_buf->size -(ring_buf->in - ring_buf->out));

}



 int32_t ringbuffer_write(struct ringbuffer *ring_buf, void *buffer, uint32_t size)
{
    uint32_t len = 0;
    uint32_t in_index = 0;
    if(ring_buf == NULL) {
        return  -1;
    }
    if(ringbuffer_buf_bytes(ring_buf) < size) {
        return -1;
    }
    
    size = min(size, ring_buf->size - (ring_buf->in - ring_buf->out));
    /* first put the data starting from fifo->in to buffer end */
    if(ring_buf->is_power_2) {
        in_index = ring_buf->in & (ring_buf->size - 1);
    }
    else {
        in_index = ring_buf->in %ring_buf->size;
        
    }
    len  = min(size, ring_buf->size - in_index);
    memcpy(ring_buf->buffer + in_index, buffer, len);
    /* then put the rest (if any) at the beginning of the buffer */
    memcpy(ring_buf->buffer, buffer + len, size - len);
    ring_buf->in += size;
    return 0;
}

 int32_t ringbuffer_read(struct ringbuffer *ring_buf, uint8_t *buffer,
                                  uint32_t size)
{
    uint32_t len = 0;
    uint32_t out_index = 0;
    if(ring_buf == NULL) {
        return  -1;
    }
    if(ringbuffer_data_bytes(ring_buf) < size) {
        return -1;
    }
    size  = min(size, ring_buf->in - ring_buf->out);
    /* first get the data from fifo->out until the end of the buffer */
    if(ring_buf->is_power_2) {
        out_index = ring_buf->out & (ring_buf->size - 1);
    }
    else {
        out_index = ring_buf->out % ring_buf->size;
    }
    len = min(size, ring_buf->size - out_index);
    memcpy(buffer, ring_buf->buffer + out_index,
           len);
    /* then get the rest (if any) from the beginning of the buffer */
    memcpy(buffer + len, ring_buf->buffer, size - len);
    ring_buf->out += size;
    
    return 0;
}


 int32_t ringbuffer_consume(struct ringbuffer *ring_buf,
                                  uint32_t size)
{
    uint32_t len = 0;
    uint32_t out_index = 0;
    if(ring_buf == NULL) {
        return  -1;
    }
    if(ringbuffer_data_bytes(ring_buf) < size) {
        return -1;
    }
    size  = min(size, ring_buf->in - ring_buf->out);
    ring_buf->out += size;
    
    return 0;
}





 int32_t ringbuffer_peek(struct ringbuffer *ring_buf, uint8_t *buffer,
                                  uint32_t offset, uint32_t size)
{
    uint32_t len = 0;
    uint32_t out_index = 0;
    if(ring_buf == NULL) {
        return  -1;
    }
    if(ringbuffer_data_bytes(ring_buf) < (size + offset)) {
        return -1;
    }
    
    size  = min(size, ring_buf->in - ring_buf->out);
    /* first get the data from fifo->out until the end of the buffer */
    if(ring_buf->is_power_2) {
        out_index = ring_buf->out & (ring_buf->size - 1);
    }
    else {
        out_index = ring_buf->out % ring_buf->size;
    }
    if(offset < (ring_buf->size - out_index)) {
        len = min(size, ring_buf->size - (out_index + offset));
        memcpy(buffer, ring_buf->buffer + out_index + offset,
        len);
        /* then get the rest (if any) from the beginning of the buffer */
        memcpy(buffer + len, ring_buf->buffer, size - len);
    }
    else {
        memcpy(buffer, ring_buf->buffer + (offset-(ring_buf->size - out_index)), size);
    }

    return 0;
}


 int32_t ringbuffer_reset(struct ringbuffer *ring_buf)
{
    if(ring_buf == NULL) {
        return  -1;
    }
    ring_buf->out = 0;
    ring_buf->in = 0;
    return 0;
}

 int32_t ringbuffer_total_write(struct ringbuffer *ring_buf)
{
    if(ring_buf == NULL) {
        return  0;
    }
    return ring_buf->in;
}

