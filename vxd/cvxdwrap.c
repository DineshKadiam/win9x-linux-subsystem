#include "vxdwraps.h"

#define VXD_CALL_(service) "int $0x20\n" ".long " #service "\n"
#define VXD_CALL(service) VXD_CALL_(service)
#define VMM_CALL(service) VXD_CALL(0x00010000 | (service))

void VMM_Get_Thread_Win32_Pri(HTHREAD thread, struct Thread_Win32_Pri* out)
{
    __asm__ volatile(VMM_CALL(0x0166)
        :   "=a"(out->base_priority),
            "=c"(out->current_priority),
            "=d"(out->time_decay_boost),
            "=S"(out->inversion_priority)
        :   "D"(thread));
}

bool VMM_Set_Thread_Win32_Pri(HTHREAD thread, u32 priority)
{
    int carry;
    __asm__ volatile(VMM_CALL(0x0167)
        :   "=@ccc"(carry)
        :   "D"(thread),
            "a"(priority));
    return !carry;
}

HEVENT VMM_Schedule_Thread_Event(HTHREAD thread, void* callback, void* data)
{
    HEVENT out;
    __asm__ volatile(VMM_CALL(0x00fb)
        :   "=S"(out)
        :   "D"(thread),
            "S"(callback),
            "d"(data));
    return out;
}
