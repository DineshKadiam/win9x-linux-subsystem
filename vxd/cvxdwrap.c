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

HEVENT VMM_Call_Priority_VM_Event(HVM vm, u32 boost, u32 flags, void* callback, void* data)
{
    HEVENT out;
    u32 timeout = 0;
    __asm__ volatile(VMM_CALL(0x0014)
        : "=S"(out)
        : "a"(boost), "b"(vm), "c"(flags), "d"(data), "S"(callback), "D"(timeout));
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

void VMM_Save_Client_State(struct VMM_Client_Regs* regs)
{
    __asm__ volatile(VMM_CALL(0x008d)
        :: "D"(regs));
}

void VMM_Restore_Client_State(const struct VMM_Client_Regs* regs)
{
    __asm__ volatile(VMM_CALL(0x008e)
        :: "S"(regs));
}

void VMM_Begin_Nest_V86_Exec(void)
{
    __asm__ volatile(VMM_CALL(0x0082));
}

void VMM_Exec_Int(u8 interrupt)
{
    __asm__ volatile(VMM_CALL(0x0084) :: "a"(interrupt));
}

void VMM_End_Nest_Exec(void)
{
    __asm__ volatile(VMM_CALL(0x0086));
}
