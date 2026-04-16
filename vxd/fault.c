#include "fault.h"

fault_result_t wsl9x_fault(u16 type, VMM_VMCB* vm, void* frame)
{
    // printf("wsl9x_fault: type=%04x, vm=%lx, stack=%lx\n",
    //     type, (u32)vm, (u32)frame);

    return FAULT_CONTINUE;
}
