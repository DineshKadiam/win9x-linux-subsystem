#pragma once
#include "prelude.h"
#include <wsl9x/task.h>

typedef enum {
    FAULT_CONTINUE = 0,
    FAULT_HANDLED = 1,
} fault_result_t;

#define TRAP_MASK  (0xff)
#define TRAP_UD    (0x06)
#define TRAP_GP    (0x0d)
#define TRAP_PF    (0x0e)

#define FAULT_MASK (0xff << 8)
#define FAULT_VMM  (0x01 << 8)
#define FAULT_PM   (0x02 << 8)

fault_result_t wsl9x_vmm_fault(u16 type, VMM_VMCB* vm, void* frame);
