%include "ddk/vxdn.asm"
%include "prelude.asm"

global debug_init
debug_init:
    VMMCall _CreateMutex, 0, 1 ; flag=MUTEX_MUST_COMPLETE
    test eax, eax
    jz .create_mutex_err
    mov [printf_mutex], eax
    ret
.create_mutex_err:
    Fatal_Error "VMM_CreateMutex failed"


global printf
printf:
    ; take printf mutex
    ; VMMCall _EnterMutex, [printf_mutex], 0
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
    call WSL9X_Debug_Print
    add esp, 8
    ; unlock mutex and return
    ; VMMCall _LeaveMutex, [printf_mutex]
    leave
    ret


global WSL9X_Debug_Print
WSL9X_Debug_Print:
    push ebp
    mov ebp, esp
    pushf
    cli

    push esi
    push ecx
    push edx

    mov esi, [arg(0)]
    mov ecx, [arg(1)]
    call bochs_debug_print

    pop edx
    pop ecx
    pop esi

    popf
    leave
    ret


; ESI: pointer to buffer, ECX: count of bytes
; uses DX
bochs_debug_print:
    mov dx, 0xe9
    rep outsb
    ret


section .bss
printf_mutex resd 1
printf_return resd 1
printf_buffer resb 256
