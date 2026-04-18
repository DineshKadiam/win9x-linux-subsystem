%include "ddk/vxdn.asm"
%include "prelude.asm"

%define WSL9X_DEVICE_ID 0x8381
%define PAGE_SIZE 0x1000

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


extern WSL9x_Printk
extern wsl9x_init_entry
extern wsl9x_reserve_addresses
extern wsl9x_load_kernel
extern wsl9x_notify_thread_switched
extern wsl9x_fault
extern wsl9x__start_kernel
extern wsl9x__resume_kernel
extern wsl9x__user_page_fault
extern wsl9x__user_syscall
; extern thread_task_struct_offset

section .data

Declare_Virtual_Device WSL9X, "WSL9X", 1, 0, WSL9X_DEVICE_ID, UNDEFINED_INIT_ORDER, WSL9X_API, WSL9X_API

global wsl9x_thread
wsl9x_thread dd 0

global wsl9x_context
wsl9x_context dd 0

section .text


WSL9X_Control:
    cmp eax, SYS_CRITICAL_INIT
    je sys_critical_init

    cmp eax, DEVICE_INIT
    je device_init

    cmp eax, SYS_VM_INIT
    je sys_vm_init

    ; cmp eax, CREATE_THREAD
    ; je create_thread

    clc
    ret


sys_critical_init:
    VMMCall _CreateMutex, 0, 1 ; flag=MUTEX_MUST_COMPLETE
    test eax, eax
    jz .create_mutex_err
    mov [printf_mutex], eax

    clc
    ret
.create_mutex_err:
    Fatal_Error "VMM_CreateMutex failed"

device_init:
    call wsl9x_init_entry
    test eax, eax
    jz .err

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


; create_thread:
;     ; zero initialize thread data slot:
;     xor eax, eax
;     mov ebx, [thread_task_struct_offset]
;     mov [edi + ebx], eax
;     clc
;     ret


thread_switched:
    push ebp
    mov ebp, esp

    cmp edi, [wsl9x_thread]
    jne .out

    ; switching to wsl9x thread
    VMMCall _ContextSwitch, [wsl9x_context]

.out:
    leave
    ret


thread_init_callback:
    push .msg
    call printf
    add esp, 4

    xchg bx, bx
    ud2
    ret
    .msg db "thread_init_callback", 10, 0


sys_vm_init:
    call install_fault_handlers
    test eax, eax
    jz .install_fault_handlers_err

    VMMCall _ContextCreate
    test eax, eax
    jz .create_context_err
    mov [wsl9x_context], eax

    mov esi, thread_switched
    VMMCall Call_When_Thread_Switched
    jc .call_when_thread_switched_err

    VMMCall VMMCreateThread, ss, 0x4000000, cs, 0x40000000, ds, es, WSL9X_THREAD_TYPE, thread_init_callback, 0
    test eax, eax
    jz .create_thread_err
    mov [wsl9x_thread], eax

    mov edi, eax
    mov esi, wsl9x_thread_event_callback
    mov edx, wsl9x__start_kernel
    VMMCall Schedule_Thread_Event

    clc
    ret

.install_fault_handlers_err:
    Fatal_Error "install_fault_handlers failed"
.call_when_thread_switched_err:
    Fatal_Error "Call_When_Thread_Switched failed"
.create_context_err:
    Fatal_Error "VMM_CreateContext failed"
.create_thread_err:
    Fatal_Error "VMMCreateThread failed"


wsl9x_thread_event_callback:
    push ebp
    mov ebp, esp

    call edx

    leave
    ret


global wsl9x__schedule_kernel_resume
wsl9x__schedule_kernel_resume:
    push ebp
    mov ebp, esp

    push esi
    push edi
    push ebx

    mov edi, [wsl9x_thread]
    mov esi, wsl9x_thread_event_callback
    mov edx, wsl9x__resume_kernel
    VMMCall Schedule_Thread_Event

    pop ebx
    pop edi
    pop esi

    leave
    ret


global wsl9x__schedule_user_syscall
wsl9x__schedule_user_syscall:
    push ebp
    mov ebp, esp

    push esi
    push edi
    push ebx

    mov edi, [wsl9x_thread]
    mov esi, wsl9x_thread_event_callback
    mov edx, wsl9x__user_syscall
    VMMCall Schedule_Thread_Event

    pop ebx
    pop edi
    pop esi

    leave
    ret


global wsl9x__schedule_user_page_fault
wsl9x__schedule_user_page_fault:
    push ebp
    mov ebp, esp

    push esi
    push edi
    push ebx

    mov edi, [wsl9x_thread]
    mov esi, .callback
    mov edx, [arg(0)]
    VMMCall Schedule_Thread_Event

    pop ebx
    pop edi
    pop esi

    leave
    ret
.callback:
    push ebp
    mov ebp, esp

    push edx
    call wsl9x__user_page_fault
    add esp, 4

    leave
    ret

; global wsl9x__schedule_user_page_fault
; wsl9x__schedule_user_trap:
;     push ebp
;     mov ebp, esp

;     leave
;     ret


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


WSL9X_API:
    ret


;; Debug logging

global printf
printf:
    ; take printf mutex
    VMMCall _EnterMutex, [printf_mutex], 0
    ; save our return address so that _Sprintf can see the rest of our args
    ; up the stack. prepend buffer argument
    pop dword [printf_return]
    VMMCall _Sprintf, printf_buffer
    ; restore return address
    push dword [printf_return]
    ; setup proper stack frame now
    push ebp
    mov ebp, esp
    ; sprintf returns count in eax
    push eax
    push printf_buffer
    call WSL9x_Printk
    add esp, 8
    ; unlock mutex and return
    VMMCall _LeaveMutex, [printf_mutex]
    leave
    ret


section .bss
printf_mutex resd 1
printf_return resd 1
printf_buffer resb 256
