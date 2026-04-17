#pragma once
#include "prelude.h"
#include <wsl9x/task.h>

u32 VMM_Allocate_Temp_V86_Data_Area(u32 nbytes, u32 flags);
void VMM_Free_Temp_V86_Data_Area(void);

typedef void(*vmm_fault_proc_t)(void);

vmm_fault_proc_t VMM_Hook_VMM_Fault(u8 vector, vmm_fault_proc_t handler);

VMM_VMCB* VMM_Get_Sys_VM_Handle(void);

u32 SHELL_SYSMODAL_Message(VMM_VMCB* vm, u32 flags, const char* caption, const char* message);
#define MB_OK               0x0000
#define MB_ICONEXCLAMATION  0x0030
#define MB_SYSTEMMODAL      0x1000
