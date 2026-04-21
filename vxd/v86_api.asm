%include "ddk/vxdn.asm"
%include "prelude.asm"
%include "wsl9x_v86.asm"

extern wsl9x_v86_start_console
extern wsl9x_v86_send_key
extern wsl9x_v86__console_output

global wsl9x_v86_api
wsl9x_v86_api:
    ; ebx = vm
    ; ebp = client regs

    mov ax, [ebp + CRS.EAX]

    cmp ax, WSL9X_V86_API_START_CONSOLE
    je start_console

    cmp ax, WSL9X_V86_API_SEND_KEY
    je send_key

    mov eax, -1
    jmp return

start_console:
    push ebx
    call wsl9x_v86_start_console
    add esp, 4
    jmp return

send_key:
    push dword [ebp + CRS.EBX]
    push ebx
    call wsl9x_v86_send_key
    add esp, 8

    jmp return

return:
    mov dword [ebp + CRS.EAX], eax
    ret

global wsl9x_v86_schedule_console_output
wsl9x_v86_schedule_console_output:
    push ebp
    mov ebp, esp

    push ebx
    push esi
    push edi

    mov eax, HIGH_PRI_DEVICE_BOOST
    mov ebx, [arg(0)]
    mov ecx, PEF_ALWAYS_SCHED | PEF_WAIT_FOR_STI | PEF_WAIT_NOT_HW_INT
    xor edx, edx
    mov esi, .callback
    xor edi, edi
    VMMCall Call_Priority_VM_Event

    pop edi
    pop esi
    pop ebx

    leave
    ret
.callback:
    ; ebx = vm
    ; ebp = client regs
    push ebx
    call wsl9x_v86__console_output
    add esp, 4
    ret

global wsl9x_v86__write_output
wsl9x_v86__write_output:
    ; xchg bx, bx
    push ebp
    mov ebp, esp

    push ebx

    VMMCall Get_Cur_VM_Handle

    mov ax, 0x0200
    movzx eax, ax
    movzx edx, byte [arg(0)]
    push dword 0x21
    VMMCall Exec_VxD_Int

    pop ebx

    leave
    ret
