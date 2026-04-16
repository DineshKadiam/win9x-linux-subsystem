#pragma once
#include "prelude.h"

u32 VMM_Allocate_Temp_V86_Data_Area(u32 nbytes, u32 flags);
void VMM_Free_Temp_V86_Data_Area(void);

typedef void(*vmm_fault_proc_t)(void);

vmm_fault_proc_t VMM_Hook_VMM_Fault(u8 vector, vmm_fault_proc_t handler);
