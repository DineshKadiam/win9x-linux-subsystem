#pragma once
#include "prelude.h"
#include <wsl9x/task.h>

u32 VMM_Allocate_Temp_V86_Data_Area(u32 nbytes, u32 flags);
void VMM_Free_Temp_V86_Data_Area(void);

typedef void(*vmm_fault_proc_t)(void);

vmm_fault_proc_t VMM_Hook_VMM_Fault(u8 vector, vmm_fault_proc_t handler);

VMM_VMCB* VMM_Get_Cur_VM_Handle(void);
VMM_VMCB* VMM_Get_Sys_VM_Handle(void);

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

void VMM_Release_Time_Slice(void);
void VMM_Time_Slice_Sleep(u32 milliseconds);

void VMM_Suspend_VM(HVM vm);
void VMM_Resume_VM(HVM vm);

struct Thread_Win32_Pri {
    u32 base_priority;
    u32 current_priority;
    u32 time_decay_boost;
    u32 inversion_priority;
};

void VMM_Get_Thread_Win32_Pri(HTHREAD thread, struct Thread_Win32_Pri* out);
bool VMM_Set_Thread_Win32_Pri(HTHREAD thread, u32 priority);

typedef struct VMM_Event* HEVENT;
HEVENT VMM_Schedule_Thread_Event(HTHREAD thread, void* callback, void* data);

struct VWIN32_VSEB {
    u32 resp;
    u16 b3;
    u16 b2;
    u16 b1;
    const char* caption;
    const char* text;
} VWIN32_VSEB;
