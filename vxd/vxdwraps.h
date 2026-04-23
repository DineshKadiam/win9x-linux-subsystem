#pragma once
#include "prelude.h"
#include <wsl9x/task.h>

u32 VMM_Allocate_Temp_V86_Data_Area(u32 nbytes, u32 flags);
void VMM_Free_Temp_V86_Data_Area(void);

typedef void(*vmm_fault_proc_t)(void);

vmm_fault_proc_t VMM_Hook_VMM_Fault(u8 vector, vmm_fault_proc_t handler);

VMM_VMCB* VMM_Get_Cur_VM_Handle(void);

u32 SHELL_SYSMODAL_Message(VMM_VMCB* vm, u32 flags, const char* caption, const char* message);
#define MB_OK               0x0000
#define MB_ICONEXCLAMATION  0x0030
#define MB_SYSTEMMODAL      0x1000

enum Semaphore_Wait_Flags {
    BLOCK_SVC_INTS      = (1 << 0),
    BLOCK_ENABLE_INTS   = (1 << 2),
    BLOCK_THREAD_IDLE   = (1 << 4),
};

typedef struct Semaphore* HSEMAPHORE;
HSEMAPHORE VMM_Create_Semaphore(u32 tokens);
void VMM_Signal_Semaphore(HSEMAPHORE);
void VMM_Signal_Semaphore_No_Switch(HSEMAPHORE);
void VMM_Wait_Semaphore(HSEMAPHORE, enum Semaphore_Wait_Flags);

typedef struct VMM_Event* HEVENT;
HEVENT VMM_Schedule_Thread_Event(HTHREAD thread, void* callback, void* data);

HEVENT VMM_Call_Priority_VM_Event(HVM vm, u32 boost, u32 flags, void* callback, void* data);
#define HIGH_PRI_DEVICE_BOOST   0x1000
#define PEF_ALWAYS_SCHED        (1 << 3)
#define PEF_WAIT_FOR_STI        (1 << 0)
#define PEF_WAIT_NOT_HW_INT     (1 << 5)

void VMM_Save_Client_State(struct VMM_Client_Regs* regs);
void VMM_Restore_Client_State(const struct VMM_Client_Regs* regs);
void VMM_Begin_Nest_V86_Exec(void);
void VMM_Exec_Int(u8 interrupt);
void VMM_End_Nest_Exec(void);

struct VWIN32_VSEB {
    u32 resp;
    u16 b3;
    u16 b2;
    u16 b1;
    const char* caption;
    const char* text;
} VWIN32_VSEB;

void* VMM_HeapAllocate(ulong nbytes, ulong flags);
void VMM_HeapFree(void* ptr, ulong flags);
