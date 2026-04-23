#include "vxdwraps.h"

#define VXD_CALL_(service) "int $0x20\n" ".long " #service "\n"
#define VXD_CALL(service) VXD_CALL_(service)
#define VMM_CALL(service) VXD_CALL(0x00010000 | (service))

HEVENT VMM_Schedule_Thread_Event(HTHREAD thread, void* callback, void* data)
{
    HEVENT out;
    __asm__ volatile(VMM_CALL(0x00fb)
        : "=S"(out)
        : "D"(thread), "S"(callback), "d"(data));
    return out;
}

HSEMAPHORE VMM_Create_Semaphore(u32 tokens)
{
    HSEMAPHORE out;
    __asm__ volatile(VMM_CALL(0x0025)
        : "=a"(out)
        : "c"(tokens));
    return out;
}
