#pragma once
#include "prelude.h"

struct ringbuf {
    size_t write, read, mask;
    u8* buf;
};

// size will be rounded up to power of two:
bool ringbuf_init(struct ringbuf* rb, size_t size);
bool ringbuf_valid(const struct ringbuf* rb);
void ringbuf_free(struct ringbuf* rb);

bool ringbuf_empty(const struct ringbuf* rb);
bool ringbuf_full(const struct ringbuf* rb);

size_t ringbuf_put(struct ringbuf* rb, const u8* data, size_t size);
size_t ringbuf_get(struct ringbuf* rb, u8* data, size_t size);
