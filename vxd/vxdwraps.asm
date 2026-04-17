%include "ddk/vxdn.asm"
%include "prelude.asm"

; extern printf

%macro cproc_dispatcher_prologue 1
    push ebp
    mov ebp, esp

    %assign idx (%1 - 1)
    %rep %1
        push [arg(idx)]
        %assign idx idx-1
    %endrep
%endmacro

%macro cproc_dispatcher_epilogue 1
    add esp, %1*4
    leave
    ret
%endmacro

%macro def_vmm_cproc 2
global %1
%1:
    cproc_dispatcher_prologue %2
    VMMCall %tok(%substr(%1, 4))
    cproc_dispatcher_epilogue %2
%endmacro

%macro def_vxd_cproc 2
global %1
%1:
    cproc_dispatcher_prologue %2
    VxDCall %1
    cproc_dispatcher_epilogue %2
%endmacro

section .text

global VMM_Sprintf
VMM_Sprintf:
    VMMJmp _Sprintf

def_vmm_cproc VMM_PageReserve, 3
def_vmm_cproc VMM_PageCommit, 5
def_vmm_cproc VMM_PageDecommit, 3
def_vmm_cproc VMM_PageCommitPhys, 4
def_vmm_cproc VMM_PageModifyPermissions, 4
def_vmm_cproc VMM_CopyPageTable, 4
def_vmm_cproc VMM_GetFreePageCount, 1
def_vmm_cproc VMM_Allocate_Temp_V86_Data_Area, 2
def_vmm_cproc VMM_Free_Temp_V86_Data_Area, 0
def_vmm_cproc VMM_ContextCreate, 0
def_vmm_cproc VMM_ContextDestroy, 1
def_vmm_cproc VMM_ContextSwitch, 1
def_vmm_cproc VMM_GetCurrentContext, 0
def_vmm_cproc VMM_Allocate_GDT_Selector, 3
def_vmm_cproc VMM_Allocate_LDT_Selector, 5
def_vmm_cproc VMM_SetDescriptor, 5
def_vmm_cproc VMM_BuildDescriptorDWORDs, 5
def_vmm_cproc VMM_Call_On_My_Stack, 4
def_vmm_cproc VMM_CreateMutex, 2
def_vmm_cproc VMM_EnterMutex, 2
def_vmm_cproc VMM_LeaveMutex, 1
def_vmm_cproc VMM_DestroyMutex, 1
def_vmm_cproc VMM_AllocateThreadDataSlot, 0
def_vmm_cproc VMM_SignalID, 1
def_vmm_cproc VMM_BlockOnID, 1

def_vxd_cproc VTD_Get_Real_Time, 0
def_vxd_cproc VTD_Get_Date_And_Time, 0

global VMM_Get_Cur_Thread_Handle
VMM_Get_Cur_Thread_Handle:
    push ebp
    mov ebp, esp

    push edi
    VMMCall Get_Cur_Thread_Handle
    mov eax, edi
    pop edi

    leave
    ret

global VMM_Fatal_Error
VMM_Fatal_Error:
    push ebp
    mov ebp, esp

    pusha
    mov esi, [arg(0)]
    mov eax, [arg(1)]
    VMMCall Fatal_Error_Handler

    xchg bx, bx
    ud2

global VMM_VMMCreateThread
VMM_VMMCreateThread:
    cproc_dispatcher_prologue 9
    VMMCall VMMCreateThread
    cproc_dispatcher_epilogue 9

global VMM_VMMTerminateThread
VMM_VMMTerminateThread:
    push ebp
    mov ebp, esp

    pusha
    mov edi, [arg(0)]
    VMMCall VMMTerminateThread
    popa

    leave
    ret

global VMM_Call_When_Thread_Switched
VMM_Call_When_Thread_Switched:
    push ebp
    mov ebp, esp

    push esi
    mov esi, [arg(0)]
    VMMCall Call_When_Thread_Switched
    pop esi

    leave
    ret

global VMM_Set_Async_Time_Out
VMM_Set_Async_Time_Out:
    push ebp
    mov ebp, esp

    push esi
    mov eax, [arg(0)]
    mov edx, [arg(1)]
    mov esi, [arg(2)]
    VMMCall Set_Async_Time_Out
    mov eax, esi
    pop esi

    leave
    ret

global VMM_Cancel_Time_Out
VMM_Cancel_Time_Out:
    push ebp
    mov ebp, esp

    push esi
    mov esi, [arg(0)]
    VMMCall Cancel_Time_Out
    pop esi

    leave
    ret

global VMM_Begin_Critical_Section
VMM_Begin_Critical_Section:
    push ebp
    mov ebp, esp
    mov ecx, [arg(0)]
    VMMCall Begin_Critical_Section
    leave
    ret

global VMM_End_Critical_Section
VMM_End_Critical_Section:
    push ebp
    mov ebp, esp
    VMMCall End_Critical_Section
    leave
    ret

global WSL9x_Printk
WSL9x_Printk:
    push ebp
    mov ebp, esp

    push esi
    mov ecx, [arg(1)]
    mov esi, [arg(0)]
    call bochs_debug_print
    pop esi

    leave
    ret

global WSL9x_Log_Info
WSL9x_Log_Info:
    push ebp
    mov ebp, esp

    push esi

    mov ecx, msg_info.end - msg_info
    mov esi, msg_info
    call bochs_debug_print

    mov ecx, [arg(1)]
    mov esi, [arg(0)]
    call bochs_debug_print

    pop esi

    leave
    ret

global WSL9x_Log_Warn
WSL9x_Log_Warn:
    push ebp
    mov ebp, esp

    push esi

    mov ecx, msg_info.end - msg_info
    mov esi, msg_info
    call bochs_debug_print

    mov ecx, [arg(1)]
    mov esi, [arg(0)]
    call bochs_debug_print

    pop esi

    leave
    ret

; ESI: pointer to buffer, ECX: count of bytes
; uses DX
bochs_debug_print:
    mov dx, 0xe9
    rep outsb
    ret

section .rodata
msg_info db "[info] "
    .end:
msg_warn db "[WARN] "
    .end:
