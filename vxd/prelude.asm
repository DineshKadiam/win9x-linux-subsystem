%define arg(i) ebp + (8 + (i * 4))

; %define WSL9X_THREAD_TYPE 'WSL9'
%define WSL9X_THREAD_TYPE 0x394c5357
%define WSL9X_BLOCK_ID WSL9X_THREAD_TYPE

%macro Fatal_Error 1
section .text
    push 0
    push %%msg
    extern VMM_Fatal_Error
    call VMM_Fatal_Error
section .data
    %%msg db %1, 0
section .text
%endmacro

%macro Debug_Log 1
section .text
    push dword (%%msg_end - %%msg)
    push dword (%%msg)
    extern bochs_safe_debug_print
    call bochs_safe_debug_print
    add esp, 8
section .data
    %%msg db %1, 10
    %%msg_end db 0
section .text
%endmacro
