#include "console.h"
#include "entry.h"
#include "loader.h"
#include "prelude.h"
#include "vxdwraps.h"

#include <wsl9x/mem.h>
#include <wsl9x/task.h>
#include <wsl9x/sync.h>
#include <wsl9x/entry.h>
#include <wsl9x/vmm.h>

static const u32 PAGE_SHIFT = 12;
static const ulong STACK_SIZE = 0x4000; // 16 KiB

static HMUTEX entry_mutex;
static void* entry_stack;

extern HTHREAD wsl9x_thread;
extern HCONTEXT wsl9x_context;
// extern HVM wsl9x_vm;
extern HSEMAPHORE wsl9x_thread_semaphore;

static int started = 0;
static int panicked = 0;

static HEVENT event = 0;

static u32 irqs_pending = 0;
static HMUTEX irqs_mutex = 0;

void wsl9x_schedule(void);

static void* allocate_stack(void)
{
    u32 addr = VMM_PageReserve(PR_SYSTEM, STACK_SIZE >> PAGE_SHIFT, PR_FIXED);
    if (addr == PAGE_RESERVE_FAIL) {
        printf("allocate_entry_stack: VMM_PageReserve failed\n");
        return NULL;
    }

    if (!VMM_PageCommit(addr >> PAGE_SHIFT, STACK_SIZE >> PAGE_SHIFT, PD_FIXEDZERO, 0, PC_FIXED | PC_WRITEABLE)) {
        printf("allocate_entry_stack: VMM_PageCommit failed\n");
        return false;
    }

    printf("wsl9x: allocated kernel entry stack at %08lx-%08lx\n", addr, addr + STACK_SIZE - 1);

    return (void*)addr;
}

bool wsl9x_init_entry(void)
{
    entry_mutex = VMM_CreateMutex(0, MUTEX_MUST_COMPLETE);
    if (!entry_mutex) {
        printf("wsl9x_init_entry: VMM_CreateMutex failed\n");
        return false;
    }

    irqs_mutex = VMM_CreateMutex(0, MUTEX_MUST_COMPLETE);
    if (!irqs_mutex) {
        printf("wsl9x_init_entry: VMM_CreateMutex failed\n");
        return false;
    }

    entry_stack = allocate_stack();
    if (!entry_stack) {
        printf("wsl9x_init_entry: failed to allocate stack\n");
        return false;
    }

    return true;
}

// void wsl9x_start(void)
// {
//     if (wsl9x_vm) {
//         return;
//     }

//     // wsl9x_vm = VMM_Get_Cur_VM_Handle();
//     // __asm__ volatile("xchg %bx, %bx");
//     // VMM_ContextSwitch(wsl9x_context);
//     wsl9x__start_kernel();
// }

static void entry_stack_top(u32 param)
{
    struct wsl9x_entry* entry = (void*)param;
    return kernel_entry(entry);
}

// void wsl9x__schedule_kernel_resume(void);

// static void assert_kernel_vm(void)
// {
//     if (VMM_Get_Cur_VM_Handle() != wsl9x_vm) {
//         __asm__ volatile ("xchgw %bx, %bx");
//         VMM_Fatal_Error("enter_kernel on wrong thread", 0);
//     }
// }

static void assert_kernel_thread(void)
{
    if (VMM_Get_Cur_Thread_Handle() != wsl9x_thread) {
        // __asm__ volatile ("xchgw %bx, %bx");
        VMM_Fatal_Error("enter_kernel on wrong thread", 0);
    }
}

static void enter_kernel(struct wsl9x_entry* entry)
{
    if (panicked) {
        return;
    }

    u32 esp = 0;
    __asm__ volatile("mov %%esp, %0" : "=r"(esp));
    printf("enter_kernel esp=%08lx\n", esp);

    assert_kernel_thread();

    // struct Thread_Win32_Pri pri;

    // VMM_Get_Thread_Win32_Pri(VMM_Get_Cur_Thread_Handle(), &pri);
    // printf("== linux thread\n");
    // printf("       base_priority:%lu\n", pri.base_priority);
    // printf("    current_priority:%lu\n", pri.current_priority);
    // printf("    time_decay_boost:%lu\n", pri.time_decay_boost);
    // printf("  inversion_priority:%lu\n", pri.inversion_priority);

    // VMM_Get_Thread_Win32_Pri(VMM_Get_Sys_Thread_Handle(), &pri);
    // printf("== system thread\n");
    // printf("       base_priority:%lu\n", pri.base_priority);
    // printf("    current_priority:%lu\n", pri.current_priority);
    // printf("    time_decay_boost:%lu\n", pri.time_decay_boost);
    // printf("  inversion_priority:%lu\n", pri.inversion_priority);

    VMM_EnterMutex(entry_mutex, MUTEX_MUST_COMPLETE);
    VMM_Call_On_My_Stack(entry_stack_top, (u32)entry, entry_stack, STACK_SIZE);
    VMM_LeaveMutex(entry_mutex);

    wsl9x_console_check_sched();

    switch (entry->result) {
    case WSL9X_IDLE:
        printf("kernel idle\n");
        // VMM_Suspend_VM(wsl9x_vm);

        // nothing to do. wait until there's an event!
        printf("kernel is now idle. blocking VMM thread\n");
        printf("  system thread: 0x%08lx\n", (ulong)VMM_Get_Sys_Thread_Handle());
        printf(" current thread: 0x%08lx\n", (ulong)VMM_Get_Cur_Thread_Handle());
        // assert_kernel_thread();

        // if (!VMM_Set_Thread_Win32_Pri(wsl9x_thread, 1)) {
        //     printf("failed to set wsl9x thread priority\n");
        // }

        // VMM_Time_Slice_Sleep(1000);
        // VMM_Suspend_VM(wsl9x_vm);

        // wsl9x__schedule_kernel_resume();
        VMM_Wait_Semaphore(wsl9x_thread_semaphore,
            BLOCK_THREAD_IDLE | BLOCK_SVC_INTS | BLOCK_ENABLE_INTS);

        wsl9x_schedule();

        // VMM_Get_Cur_Thread_Handle()->Flags |= THFLAG_SUSPENDED | THFLAG_NOT_EXECUTEABLE;
        // VMM_BlockOnID(WSL9X_BLOCK_ID);
        break;
    case WSL9X_YIELD:
        printf("kernel yield\n");
        // schedule another resume soon
        wsl9x_schedule();
        break;
    case WSL9X_USER:
        printf("kernel user\n");
        // we are returning to userspace!
        break;
    case WSL9X_PANIC:
        panicked = 1;
        printf("!!! kernel panicked: %s\n", entry->panic_msg);
        __asm__ volatile ("xchgw %bx, %bx");
        SHELL_SYSMODAL_Message(
            VMM_Get_Cur_VM_Handle(),
            MB_SYSTEMMODAL | MB_ICONEXCLAMATION | MB_OK,
            "WSL9X kernel panic",
            entry->panic_msg
        );
        // VMM_BlockOnID(WSL9X_BLOCK_ID);
        // __asm__ volatile ("xchgw %bx, %bx");
        break;
    }
}

void wsl9x__resume_kernel(void)
{
    struct wsl9x_entry entry = { 0 };
    entry.reason = WSL9X_RESUME;

    enter_kernel(&entry);
}

// this is a thread event, when this function returns we will return to userland
void wsl9x__start_kernel(void)
{
    started = 1;

    char* argv[] = {
        "vmlinux",
        "init=/bin/sh",
        "rdinit=/bin/sh",
        // "console=dostty",
        // prevent user processes from mapping in dos arena:
        "vm.mmap_min_addr=16777216", // 0x01000000
    };

    int argc = sizeof(argv) / sizeof(*argv);

    char* envp[] = { 0 };

    struct wsl9x_entry entry;
    entry.reason = WSL9X_START;
    entry.as.start.services = &wsl9x_services;
    entry.as.start.argc = argc;
    entry.as.start.argv = argv;
    entry.as.start.envp = envp;

    enter_kernel(&entry);
}

void wsl9x__irq(enum wsl9x_irq irq)
{
    struct wsl9x_entry entry = { 0 };
    entry.reason = WSL9X_IRQ;
    entry.as.irq = irq;
    printf("  entering with irq=%d\n", irq);

    enter_kernel(&entry);
}

void wsl9x__user_page_fault(u32 fault_addr)
{
    printf("kernel page fault\n");
    struct wsl9x_entry entry = { 0 };
    entry.reason = WSL9X_PAGE_FAULT;
    entry.as.page_fault.addr = fault_addr;

    enter_kernel(&entry);
}

void wsl9x__user_syscall(void)
{
    printf("syscall\n");
    struct wsl9x_entry entry = { 0 };
    entry.reason = WSL9X_SYSCALL;

    enter_kernel(&entry);
}

static void process_irqs(void)
{
    VMM_EnterMutex(irqs_mutex, MUTEX_MUST_COMPLETE);
    u32 irqs = irqs_pending;
    irqs_pending = 0;
    VMM_LeaveMutex(irqs_mutex);

    // do pending irqs
    for (u32 i = 0; irqs; i++, irqs >>= 1) {
        if (irqs & 1) {
            wsl9x__irq(i);
        }
    }
}

static void process_kernel_events(void)
{
    process_irqs();
    wsl9x__resume_kernel();
}

static void kernel_thread_event(void)
{
    event = 0;
    process_kernel_events();
}

void wsl9x_schedule(void)
{
    if (!event) {
        event = VMM_Schedule_Thread_Event(wsl9x_thread, kernel_thread_event, 0);
    }
}

void wsl9x_schedule_irq(enum wsl9x_irq irq)
{
    VMM_EnterMutex(irqs_mutex, MUTEX_MUST_COMPLETE);
    irqs_pending |= 1 << irq;
    VMM_LeaveMutex(irqs_mutex);
    VMM_Signal_Semaphore(wsl9x_thread_semaphore);
}
