#include "loader.h"
#include "prelude.h"
#include "entry.h"

#include <wsl9x/mem.h>
#include <wsl9x/task.h>
#include <wsl9x/sync.h>
#include <wsl9x/entry.h>

static const u32 PAGE_SHIFT = 12;
static const ulong STACK_SIZE = 0x4000; // 16 KiB

static HMUTEX mutex;
static void* entry_stack;

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
    mutex = VMM_CreateMutex(0, MUTEX_MUST_COMPLETE);
    if (!mutex) {
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

static void entry_stack_top(u32 param)
{
    struct wsl9x_entry* entry = (void*)param;
    return kernel_entry(entry);
}

void wsl9x__schedule_kernel_resume(void);

static void enter_kernel(struct wsl9x_entry* entry)
{
    VMM_EnterMutex(mutex, 0);
    VMM_Call_On_My_Stack(entry_stack_top, (u32)entry, entry_stack, STACK_SIZE);
    VMM_LeaveMutex(mutex);

    switch (entry->result) {
    case WSL9X_IDLE:
        // nothing to do. wait until there's an event!
        printf("kernel is now idle. not scheduling\n");
        break;
    case WSL9X_YIELD:
        // schedule another resume soon
        wsl9x__schedule_kernel_resume();
        break;
    case WSL9X_PANIC:
        printf("kernel panicked!!");
        __asm__ volatile ("xchgw %bx, %bx");
        break;
    }
}

void wsl9x_resume_kernel(void)
{
    struct wsl9x_entry entry = { 0 };
    entry.reason = WSL9X_RESUME;

    // printf("wsl9x: resuming kernel\n");
    enter_kernel(&entry);
}

void wsl9x_start_kernel(void)
{
    char* argv[] = {
        "vmlinux",
        "init=/bin/sh",
        "rdinit=/bin/sh",
        "console=bootcon",
    };

    int argc = sizeof(argv) / sizeof(*argv);

    char* envp[] = { 0 };

    struct wsl9x_entry entry = { 0 };
    entry.reason = WSL9X_START;
    entry.as.start.argc = argc;
    entry.as.start.argv = argv;
    entry.as.start.envp = envp;

    printf("wsl9x: starting kernel\n");
    enter_kernel(&entry);
}
