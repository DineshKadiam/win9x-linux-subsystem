%include "ddk/vxdn.asm"
%define WSL9X_DEVICE_ID 0x8381
%define PAGE_SIZE 0x1000

extern wsl9x_reserve_addresses
extern wsl9x_load_kernel

section .data

Declare_Virtual_Device WSL9X, "WSL9X", 1, 0, WSL9X_DEVICE_ID, UNDEFINED_INIT_ORDER, WSL9X_API, WSL9X_API

section .text

WSL9X_Control:
    cmp eax, DEVICE_INIT
    je device_init

    cmp eax, SYS_VM_INIT
    je sys_vm_init

    clc
    ret

device_init:
    call wsl9x_reserve_addresses
    call wsl9x_load_kernel
    jmp control_return

sys_vm_init:
    jmp control_return

control_return:
    test eax, eax
    jz .err
.ok:
    clc
    ret
.err:
    stc
    ret

WSL9X_API:
    ret

;; Debug logging

global printf
printf:
    ; pop return address into edi, push prepended buffer arg
    ; _Sprintf preserves edi
    pop edi
    push buffer
    VMMCall _Sprintf
    ; sprintf returns count in eax
    mov ecx, eax
    mov esi, buffer
    call bochs_debug_print
    ; pop buffer and push return address back, then return
    add esp, 4
    push edi
    ret

; ESI: pointer to buffer, ECX: count of bytes
; uses DX
bochs_debug_print:
    mov dx, 0xe9
    rep outsb
    ret

section .bss
buffer resb 256
