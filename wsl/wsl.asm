use16
org 0x100

%include "wsl9x_v86.asm"

start:
    cli

    call init_wsl_api

    ; start console
    mov ax, WSL9X_V86_API_START_CONSOLE
    call far [wsl9x_api]

idle:
    ; this is the top of our event loop, and where the wsl9x v86 driver
    ; synchronises with us
    sti
    hlt
    cli

    ; check for key
    mov ah, 0x0b
    int 0x21
    test al, al
    jz idle ; no key

    ; read key
    mov ah, 0x07
    int 0x21
    xor ah, ah ; jump to have_key with ah = 0 for non-extended key
    test al, al
    jnz have_key

    ; read extended key
    mov ah, 0x07
    int 0x21
    mov ah, 1

have_key:
    mov bx, ax
    mov ax, WSL9X_V86_API_SEND_KEY
    call far [wsl9x_api]

    jmp idle


init_wsl_api:
    ; obtain WSL9X API entry point
    mov ax, 0x1684
    mov bx, WSL9X_DEVICE_ID
    int 0x2f
    mov [wsl9x_api.off], di
    mov [wsl9x_api.seg], es

    ; test success
    cmp dword [wsl9x_api], 0
    je no_wsl9x

    ret

no_wsl9x:
    mov dx, .msg
    jmp error
    .msg db "WSL9X is not running$"

error:
    ; print error message
    mov ah, 9
    int 0x21

    ; exit status = 1
    mov ax, 0x4c01
    int 0x21

wsl9x_api:
    .off dw 0
    .seg dw 0
