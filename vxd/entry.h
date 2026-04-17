#pragma once
#include "prelude.h"

bool wsl9x_init_entry(void);

// must call on kernel thread:
void wsl9x__start_kernel(void);
void wsl9x__resume_kernel(void);
void wsl9x__user_page_fault(u32 fault_addr);
void wsl9x__user_trap(u8 trap);
void wsl9x__user_syscall(void);
