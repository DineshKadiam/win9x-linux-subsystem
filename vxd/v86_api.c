#include "prelude.h"
#include "v86_api.h"
#include "console.h"
#include "vxdwraps.h"
#include <linux/compiler_types.h>
#include <wsl9x/task.h>

#define EXTENDED_KEY 0x0100

void wsl9x_v86_api_(HVM vm, struct VMM_Client_Regs* regs)
{
    u16 func = regs->EAX;

    switch (func) {
    case WSL9X_V86_API_START_CONSOLE:
        regs->EAX = wsl9x_v86_start_console(vm);
        break;
    case WSL9X_V86_API_SEND_KEY:
        regs->EAX = wsl9x_v86_send_key(vm, regs->EBX);
        break;
    default:
        regs->EAX = -1;
    }
}

__naked void wsl9x_v86_api(void)
{
    __asm__ volatile (
        "pushl %ebp\n"
        "pushl %ebx\n"
        "call wsl9x_v86_api_\n"
        "add $8, %esp\n"
        "ret\n"
    );
}

void wsl9x_v86_schedule_console_output(HVM vm)
{
    VMM_Call_Priority_VM_Event(vm,
        HIGH_PRI_DEVICE_BOOST,
        PEF_ALWAYS_SCHED | PEF_WAIT_FOR_STI | PEF_WAIT_NOT_HW_INT,
        wsl9x_v86__console_output,
        0);
}

void wsl9x_v86__write_output(u8 c)
{
    struct VMM_Client_Regs saved;
    VMM_Save_Client_State(&saved);
    VMM_Begin_Nest_V86_Exec();

    struct VMM_Client_Regs* regs = VMM_Get_Cur_VM_Handle()->Client_Pointer;
    regs->EAX = c;
    VMM_Exec_Int(0x29);

    VMM_End_Nest_Exec();
    VMM_Restore_Client_State(&saved);
}
