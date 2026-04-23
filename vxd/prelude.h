#pragma once

#define __KERNEL__

typedef _Bool bool;
#define true 1
#define false 0

#define NULL ((void*)0)

typedef unsigned char u8, uchar, uint8_t;
typedef unsigned short u16, ushort, uint16_t;
typedef unsigned long u32, ulong, uint32_t, size_t;
typedef unsigned long long u64, uint64_t;

typedef signed long int32_t, ssize_t;
typedef signed long long int64_t;

#define __noreturn __attribute__((__noreturn__))

#include <wsl9x.h>

size_t VMM_Sprintf(char* out, const char* format, ...)
    __attribute__((format(printf, 2, 3)));

void printf(const char* format, ...)
    __attribute__((format(printf, 1, 2)));

static inline void memcpy32(void* dst, const void* src, size_t bytes)
{
    size_t dwords = (bytes + 3) / 4;
    __asm__("cld \n rep movsl" : "+S"(src), "+D"(dst), "+c"(dwords) :: "memory");
}

// #define WSL9X_THREAD_TYPE 'WSL9'
#define WSL9X_THREAD_TYPE 0x394c5357
#define WSL9X_BLOCK_ID WSL9X_THREAD_TYPE
