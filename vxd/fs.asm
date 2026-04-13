%include "ddk/vxdn.asm"
%include "prelude.asm"

section .text

%define DOS_OPEN_FILE 0x3d
%define DOS_CLOSE_FILE 0x3e
%define DOS_READ_FILE 0x3f
%define DOS_SEEK_FILE 0x42

%define MAX_READ_BUFFER 0xffff

%define CARRY_BIT 0

dos_int:
    push dword 0x21
    VMMCall Exec_VxD_Int
    ret

global dos_open
dos_open:
    push ebp
    mov ebp, esp

    mov ah, DOS_OPEN_FILE
    mov al, 0 ; read only
    mov edx, [arg(0)]
    call dos_int
.ok:
    movzx eax, ax
    leave
    ret
.err:
    xor eax, eax
    leave
    ret

global dos_close
dos_close:
    push ebp
    mov ebp, esp

    mov ah, DOS_CLOSE_FILE
    mov bx, [arg(0)]
    call dos_int
    jc .err
.ok:
    mov eax, 1
    leave
    ret
.err:
    xor eax, eax
    leave
    ret

global dos_seek
dos_seek:
    push ebp
    mov ebp, esp

    xor eax, eax
    mov ah, DOS_SEEK_FILE
    movzx bx, word [arg(0)]
    mov ecx, [arg(1)]
    mov dx, cx
    shr ecx, 16
    call dos_int
    jc .err
.ok:
    xor eax, eax
    leave
    ret
.err:
    movzx eax, ax
    neg eax
    leave
    ret

global dos_read
dos_read:
    push ebp
    mov ebp, esp

    ; start nested v86 exec
    VMMCall Begin_Nest_V86_Exec

    ; load pointer to client regs struct in ebx
    VMMCall Get_Cur_VM_Handle
    mov ebx, [ebx + CB_Client_Pointer]

    ; set AH to function number
    xor eax, eax
    mov ah, DOS_READ_FILE
    mov [ebx + CRS.EAX], eax
    ; set EBX to file handle
    movzx eax, word [arg(0)]
    mov [ebx + CRS.EBX], eax
    ; ECX to byte count
    movzx eax, word [arg(2)]
    mov [ebx + CRS.ECX], eax
    ; DS:DX to buffer
    mov ax, word [arg(1)]
    mov [ebx + CRS.DS], ax
    xor eax, eax
    mov [ebx + CRS.EDX], eax

    ; do the int
    mov eax, 0x21
    VMMCall Simulate_Int
    VMMCall Resume_Exec

    ; read eax and test carry bit
    movzx eax, word [ebx + CRS.EAX]
    bt dword [ebx + CRS.EFlags], CARRY_BIT
    jnc .ok
.err:
    neg eax
.ok:
    VMMCall End_Nest_Exec
    leave
    ret
