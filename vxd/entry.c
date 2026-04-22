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

static HSEMAPHORE wsl9x_semaphore;

static int started = 0;
static int panicked = 0;

static u32 pending_irqs = 0;

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

static void make_start_params(struct wsl9x_entry* entry)
{
    static char* argv[] = {
        "vmlinux",
        "init=/boot.sh",
        "rdinit=/boot.sh",
        // prevent user processes from mapping in dos arena:
        "vm.mmap_min_addr=16777216", // 0x01000000
    };

    static char* envp[] = { 0 };

    int argc = sizeof(argv) / sizeof(*argv);

    entry->reason = WSL9X_START;
    entry->as.start.argc = argc;
    entry->as.start.argv = argv;
    entry->as.start.envp = envp;
}

static void idle(void)
{
    VMM_Wait_Semaphore(wsl9x_semaphore,
        BLOCK_THREAD_IDLE | BLOCK_SVC_INTS | BLOCK_ENABLE_INTS);
}

static __noreturn void fatal_error(const char* caption, const char* message)
{
    __asm__ volatile ("xchgw %bx, %bx");

    SHELL_SYSMODAL_Message(
        VMM_Get_Cur_VM_Handle(),
        MB_SYSTEMMODAL | MB_ICONEXCLAMATION | MB_OK,
        caption,
        message
    );

    while (1) {
        idle();
    }
}

static __noreturn void bug(const char* message)
{
    printf("!!! WSL9X BUG: %s\n", message);
    fatal_error("WSL9X BUG", message);
}

static __noreturn void handle_panic(const char* message)
{
    panicked = 1;
    printf("!!! kernel panicked: %s\n", message);
    fatal_error("WSL9X kernel panic", message);
}

static void assert_kernel_thread(void)
{
    if (VMM_Get_Cur_Thread_Handle() != wsl9x_thread) {
        bug("not on kernel thread");
    }
}

static u32 irq_bit(enum wsl9x_irq irq)
{
    return 1 << irq;
}

static enum wsl9x_irq pop_irq(void)
{
    for (enum wsl9x_irq irq = 0; irq < WSL9X_NR_IRQS; irq++) {
        u32 bit = irq_bit(irq);
        if (pending_irqs & bit) {
            pending_irqs &= ~bit;
            return irq;
        }
    }

    return 0;
}

static void entry_stack_top(u32 param)
{
    struct wsl9x_entry* entry = (void*)param;
    kernel_entry(entry);
}

static bool kernel_one_loop(struct wsl9x_entry* entry)
{
    assert_kernel_thread();

    // dispatch to kernel
    VMM_EnterMutex(entry_mutex, MUTEX_MUST_COMPLETE);
    VMM_Call_On_My_Stack(entry_stack_top, (u32)entry, entry_stack + STACK_SIZE, STACK_SIZE);
    VMM_LeaveMutex(entry_mutex);

    // schedule any necessary work outside of kernel
    wsl9x_console_check_sched();

    // handle kernel result
    switch (entry->result) {
    case WSL9X_IDLE:
        idle();
        break;
    case WSL9X_YIELD:
        // kernel yielding, just go around again for now.
        // no time slice release until i can prove it's actually doing anything
        break;
    case WSL9X_USER:
        return false;
    case WSL9X_PANIC:
        handle_panic(entry->panic_msg);
    }

    // setup entry to go around again
    enum wsl9x_irq irq = pop_irq();
    if (irq) {
        entry->reason = WSL9X_IRQ;
        entry->as.irq = irq;
    } else {
        entry->reason = WSL9X_RESUME;
    }

    return true;
}

static void kernel_loop(struct wsl9x_entry* entry)
{
    static int in_loop;

    if (in_loop) {
        bug("recursive kernel_loop is an error!");
    }

    in_loop = 1;
    while (kernel_one_loop(entry)) ;
    in_loop = 0;
}

static void start_kernel(void)
{
    assert_kernel_thread();

    struct wsl9x_entry entry;
    make_start_params(&entry);

    kernel_loop(&entry);
}

void wsl9x_irq(enum wsl9x_irq irq)
{
    pending_irqs |= irq_bit(irq);
    VMM_Signal_Semaphore(wsl9x_semaphore);
}

void wsl9x__user_page_fault(u32 fault_addr)
{
    struct wsl9x_entry entry = { 0 };
    entry.reason = WSL9X_PAGE_FAULT;
    entry.as.page_fault.addr = fault_addr;

    kernel_loop(&entry);
}

void wsl9x__user_syscall(void)
{
    struct wsl9x_entry entry = { 0 };
    entry.reason = WSL9X_SYSCALL;

    kernel_loop(&entry);
}

void wsl9x_start_kernel(void)
{
    if (started) {
        bug("wsl9x_start_kernel: kernel already started");
    }

    entry_mutex = VMM_CreateMutex(0, MUTEX_MUST_COMPLETE);
    if (!entry_mutex) {
        bug("wsl9x_start_kernel: VMM_CreateMutex failed");
    }

    wsl9x_semaphore = VMM_Create_Semaphore(0);
    if (!wsl9x_semaphore) {
        bug("wsl9x_start_kernel: VMM_Create_Semaphore failed");
    }

    entry_stack = allocate_stack();
    if (!entry_stack) {
        bug("wsl9x_start_kernel: allocate_stack failed");
    }

    VMM_Schedule_Thread_Event(wsl9x_thread, start_kernel, 0);
}
