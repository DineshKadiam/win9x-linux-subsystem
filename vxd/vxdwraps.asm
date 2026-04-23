%include "ddk/vxdn.asm"
%include "prelude.asm"

%macro def_vmm_cproc 2
global %1
%1:
    VMMJmp %tok(%substr(%1, 4))
%endmacro

section .text

global VMM_Sprintf
VMM_Sprintf:
    VMMJmp _Sprintf

def_vmm_cproc VMM_PageReserve, 3
def_vmm_cproc VMM_PageCommit, 5
def_vmm_cproc VMM_GetFreePageCount, 1
def_vmm_cproc VMM_Allocate_Temp_V86_Data_Area, 2
def_vmm_cproc VMM_Free_Temp_V86_Data_Area, 0
def_vmm_cproc VMM_Call_On_My_Stack, 4
def_vmm_cproc VMM_CreateMutex, 2
def_vmm_cproc VMM_EnterMutex, 2
def_vmm_cproc VMM_LeaveMutex, 1
def_vmm_cproc VMM_HeapAllocate, 2
def_vmm_cproc VMM_HeapFree, 2

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

global VMM_Get_Cur_VM_Handle
VMM_Get_Cur_VM_Handle:
    push ebp
    mov ebp, esp

    push ebx
    VMMCall Get_Cur_VM_Handle
    mov eax, ebx
    pop ebx

    leave
    ret

global VMM_Signal_Semaphore
VMM_Signal_Semaphore:
    push ebp
    mov ebp, esp

    mov eax, [arg(0)]
    VMMCall Signal_Semaphore

    leave
    ret

global VMM_Signal_Semaphore_No_Switch
VMM_Signal_Semaphore_No_Switch:
    push ebp
    mov ebp, esp

    mov eax, [arg(0)]
    VMMCall Signal_Semaphore_No_Switch

    leave
    ret

global VMM_Wait_Semaphore
VMM_Wait_Semaphore:
    push ebp
    mov ebp, esp

    mov eax, [arg(0)]
    mov ecx, [arg(1)]
    VMMCall Wait_Semaphore

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

global SHELL_SYSMODAL_Message
SHELL_SYSMODAL_Message:
    push ebp
    mov ebp, esp

    push ebx
    push edi

    mov ebx, [arg(0)]
    mov eax, [arg(1)]
    mov edi, [arg(2)]
    mov ecx, [arg(3)]
    VxDCall SHELL_SYSMODAL_Message

    pop edi
    pop ebx

    leave
    ret
