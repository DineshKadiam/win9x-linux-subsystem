%include "ddk/vxdn.asm"
%include "prelude.asm"

section .text

global VMM_Sprintf
VMM_Sprintf:
    VMMJmp _Sprintf

global VMM_PageReserve
VMM_PageReserve:
    push ebp
    mov ebp, esp
    VMMCall _PageReserve, [arg(0)], [arg(1)], [arg(2)]
    leave
    ret

global VMM_PageCommit
VMM_PageCommit:
    push ebp
    mov ebp, esp
    VMMCall _PageCommit, [arg(0)], [arg(1)], [arg(2)], [arg(3)], [arg(4)]
    leave
    ret

global VMM_GetFreePageCount
VMM_GetFreePageCount:
    push ebp
    mov ebp, esp
    VMMCall _GetFreePageCount, [arg(0)]
    leave
    ret

global VMM_Allocate_Temp_V86_Data_Area
VMM_Allocate_Temp_V86_Data_Area:
    push ebp
    mov ebp, esp
    VMMCall _Allocate_Temp_V86_Data_Area, [arg(0)], [arg(1)]
    leave
    ret

global VMM_Free_Temp_V86_Data_Area
VMM_Free_Temp_V86_Data_Area:
    push ebp
    mov ebp, esp
    VMMCall _Free_Temp_V86_Data_Area
    leave
    ret
