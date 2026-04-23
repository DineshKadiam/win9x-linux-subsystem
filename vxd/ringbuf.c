#include "ringbuf.h"
#include "vxdwraps.h"

static size_t next_power2(size_t n)
{
    size_t power2 = 2;
    while (power2 < n) {
        power2 <<= 1;
    }
    return n;
}

bool ringbuf_init(struct ringbuf* rb, size_t size)
{
    size = next_power2(size);

    rb->write = 0;
    rb->read = 0;
    rb->mask = size - 1;
    rb->buf = VMM_HeapAllocate(size, 0);

    return ringbuf_valid(rb);
}

bool ringbuf_valid(const struct ringbuf* rb)
{
    return !!rb->buf;
}

void ringbuf_free(struct ringbuf* rb)
{
    if (rb->buf) {
        VMM_HeapFree(rb->buf, 0);
        rb->buf = 0;
    }
}

static inline size_t ringbuf_capacity(const struct ringbuf* rb)
{
    return rb->mask + 1;
}

bool ringbuf_empty(const struct ringbuf* rb)
{
    return rb->write == rb->read;
}

bool ringbuf_full(const struct ringbuf* rb)
{
    return (rb->write - rb->read) == ringbuf_capacity(rb);
}

size_t ringbuf_put(struct ringbuf* rb, const u8* data, size_t size)
{
    size_t n = 0;

#ifdef DEBUG_RINGBUF
    printf("ringbuf_put[%s]: write=%lu, read=%lu\n", rb->name, rb->write, rb->read);
    printf("                 size=%lu\n", size);
    printf("                 data='");
#endif

    while (n < size && !ringbuf_full(rb)) {
        u8 c = data[n++];

#ifdef DEBUG_RINGBUF
        printf("%c", c == '\n' ? '.' : c);
#endif

        rb->buf[rb->write++ & rb->mask] = c;
    }

#ifdef DEBUG_RINGBUF
    printf("'\n");
    printf("                 n=%lu\n", n);
#endif

    return n;
}

size_t ringbuf_get(struct ringbuf* rb, u8* data, size_t size)
{
    size_t n = 0;

#ifdef DEBUG_RINGBUF
    printf("ringbuf_get[%s]: write=%lu, read=%lu\n", rb->name, rb->write, rb->read);
    printf("                 size=%lu\n", size);
    printf("                 data='");
#endif

    while (n < size && !ringbuf_empty(rb)) {
        char c = rb->buf[rb->read++ & rb->mask];

#ifdef DEBUG_RINGBUF
        printf("%c", c == '\n' ? '.' : c);
#endif

        data[n++] = c;
    }

#ifdef DEBUG_RINGBUF
    printf("'\n");
    printf("                 n=%lu\n", n);
#endif

    return n;
}
