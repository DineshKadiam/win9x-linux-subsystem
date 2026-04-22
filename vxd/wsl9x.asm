%include "ddk/vxdn.asm"
%include "prelude.asm"
%include "wsl9x_v86.asm"

%define FAULT_MASK (0xff << 8)
%define FAULT_VMM  (0x01 << 8)
%define FAULT_PM   (0x02 << 8)

%define TRAP_MASK  (0xff)
; undefined opcode
%define TRAP_UD 0x06
; general protection fault
%define TRAP_GP 0x0d
; page fault
%define TRAP_PF 0x0e


extern wsl9x_init_entry
extern wsl9x_reserve_addresses
extern wsl9x_load_kernel
extern wsl9x_fault
extern wsl9x_start_kernel
extern wsl9x_v86_api
extern printf
extern debug_init


section .data

%define Create_Service_Table_WSL9X
extern WSL9X_Service_Table
extern Num_WSL9X_Services

Declare_Virtual_Device WSL9X, "WSL9X", 1, 0, WSL9X_DEVICE_ID, UNDEFINED_INIT_ORDER, wsl9x_v86_api, 0

global wsl9x_thread
wsl9x_thread dd 0

section .text


WSL9X_Control:
    cmp eax, SYS_CRITICAL_INIT
    je sys_critical_init

    cmp eax, DEVICE_INIT
    je device_init

    cmp eax, SYS_VM_INIT
    je sys_vm_init

    cmp eax, KERNEL32_INITIALIZED
    je kernel32_init

    clc
    ret


sys_critical_init:
    call debug_init
    clc
    ret

device_init:
    Debug_Log "wsl9x: device_init"

    call wsl9x_reserve_addresses
    test eax, eax
    jz .err

    call wsl9x_load_kernel
    test eax, eax
    jz .err

    clc
    ret
.err:
    stc
    ret


sys_vm_init:
    Debug_Log "wsl9x: sys_vm_init"

    call install_fault_handlers
    test eax, eax
    jz .install_fault_handlers_err

    clc
    ret
.install_fault_handlers_err:
    Fatal_Error "install_fault_handlers failed"
.call_when_thread_switched_err:
    Fatal_Error "Call_When_Thread_Switched failed"
.call_when_task_switched_err:
    Fatal_Error "Call_When_Task_Switched failed"
.create_context_err:
    Fatal_Error "VMM_CreateContext failed"


kernel32_init:
    VMMCall VMMCreateThread, ss, 0x4000000, cs, 0x40000000, ds, es, WSL9X_THREAD_TYPE, thread_init_callback, 0
    test eax, eax
    jz .create_thread_err
    mov [wsl9x_thread], eax

    mov edi, eax
    VMMCall Detach_Thread_From_Group

    call wsl9x_start_kernel

.out:
    clc
    ret
.create_thread_err:
    Fatal_Error "VMMCreateThread failed"


thread_init_callback:
    ret


%macro install_fault 3
    mov eax, %2
    mov esi, trap_%3
    VMMCall Hook_%1_Fault
    jc .err
    mov [prev_fault_handler.%3], esi
%endmacro

%macro dispatch_fault 2
    pushf
    pusha

    push ebp
    push ebx
    push %1
    call wsl9x_fault
    add esp, 12
    test eax, eax
    jz .continue

    popa
    popf
    ret

.continue:
    popa
    popf
    jmp [prev_fault_handler.%2]
%endmacro

install_fault_handlers:
    install_fault VMM, TRAP_GP, vmm_gp
    install_fault VMM, TRAP_PF, vmm_pf
    install_fault VMM, TRAP_UD, vmm_ud

    install_fault PM, TRAP_GP, pm_gp
    install_fault PM, TRAP_PF, pm_pf
    install_fault PM, TRAP_UD, pm_ud

    ret
.err:
    Fatal_Error "error installing fault handler"

install_vmm_fault_handler:
    VMMCall Hook_VMM_Fault
    jc .err
    mov [edi], esi
.err:
    ret

section .data

prev_fault_handler:
    .vmm_ud dd 0
    .vmm_gp dd 0
    .vmm_pf dd 0
    .pm_ud dd 0
    .pm_gp dd 0
    .pm_pf dd 0

section .text

trap_vmm_gp:
    dispatch_fault TRAP_GP | FAULT_VMM, vmm_gp

trap_vmm_pf:
    dispatch_fault TRAP_PF | FAULT_VMM, vmm_pf

trap_vmm_ud:
    dispatch_fault TRAP_UD | FAULT_VMM, vmm_ud

trap_pm_gp:
    dispatch_fault TRAP_GP | FAULT_PM, pm_gp

trap_pm_pf:
    dispatch_fault TRAP_PF | FAULT_PM, pm_pf

trap_pm_ud:
    dispatch_fault TRAP_UD | FAULT_PM, pm_ud
