#include "fault.h"
#include "entry.h"
#include "prelude.h"
#include "vxdwraps.h"
#include <wsl9x/sync.h>
#include <wsl9x/task.h>
#include <wsl9x/vmm.h>

extern HTHREAD wsl9x_thread;
// extern HVM wsl9x_vm;

// void wsl9x__schedule_user_page_fault(u32 fault_addr);
// void wsl9x__schedule_user_syscall(void);

static const u32 min_user_addr = 0x00400000;
static const u32 max_user_addr = 0x80000000;

static bool handle_syscall(VMM_Client_Regs* regs)
{
    if (regs->EIP < min_user_addr || regs->EIP >= max_user_addr) {
        return false;
    }

    uint8_t* code = (uint8_t*)regs->EIP;

    // is it int 0x80?
    if (code[0] == 0xcd && code[1] == 0x80) {
        // advance EIP by two and dispatch as syscall
        regs->EIP += 2;
        wsl9x__user_syscall();
        // wsl9x__schedule_user_syscall();
        return true;
    }

    return false;
}

// fault_result_t wsl9x_page_fault(HVM vm, const VMM_IPF_Data* data)
// {
//     // if (wsl9x_vm != data->faulting_vm) {
//     //     return FAULT_CONTINUE;
//     // }

//     // if (VMM_Get_Sys_VM_Handle() != data->faulting_vm) {
//     //     return FAULT_CONTINUE;
//     // }

//     // if (vm != data->faulting_vm) {
//     //     VMM_Fatal_Error("wsl9x_page_fault: vm != faulting_vm, don't know how to handle this!!!\n", 0);
//     // }

//     if (VMM_Get_Cur_Thread_Handle() != wsl9x_thread) {
//         return FAULT_CONTINUE;
//     }

//     wsl9x__schedule_user_page_fault(data->linear_addr);
//     return FAULT_HANDLED;
// }

fault_result_t wsl9x_fault(u16 type, VMM_VMCB* vm, void* context)
{
    (void)vm;

    // only interested in protected mode faults
    if (FAULT(type) != FAULT_PM) {
        return FAULT_CONTINUE;
    }

    // if (vm != wsl9x_vm) {
    //     return FAULT_CONTINUE;
    // }

    if (VMM_Get_Cur_Thread_Handle() != wsl9x_thread) {
        return FAULT_CONTINUE;
    }

    if (vm != wsl9x_thread->VMHandle) {
        return FAULT_CONTINUE;
    }

    VMM_Client_Regs* regs = context;

    // handle user page fault
    if (TRAP(type) == TRAP_PF) {
        u32 fault_addr;
        // TODO - should we use Hook_Invalid_Page_Fault instead?
        // the ddk docs say vxds should not rely on cr2 value
        __asm__ volatile ("movl %%cr2, %0" : "=r"(fault_addr));

        // wsl9x__schedule_user_page_fault(fault_addr);
        wsl9x__user_page_fault(fault_addr);
        return FAULT_HANDLED;
    }

    // if it's a GPF, see if it looks like a syscall
    if (TRAP(type) == TRAP_GP) {
        if (handle_syscall(regs)) {
            return FAULT_HANDLED;
        }
    }

    // printf("wsl9x_fault: type=%04x, regs=%08lx\n", type, (u32)regs);
    // __asm__ volatile ("xchgw %bx, %bx");

    return FAULT_CONTINUE;
}
