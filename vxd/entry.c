#include "loader.h"
#include "prelude.h"
#include "entry.h"

#include <wsl9x/mem.h>
#include <wsl9x/task.h>
#include <wsl9x/sync.h>

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
    (void)param;
    kernel_entry();
}

static void enter_kernel(void)
{
    VMM_EnterMutex(mutex, 0);
    VMM_Call_On_My_Stack(entry_stack_top, 0, entry_stack, STACK_SIZE);
    VMM_LeaveMutex(mutex);
}

void wsl9x_start_kernel(void)
{
    enter_kernel();
}
