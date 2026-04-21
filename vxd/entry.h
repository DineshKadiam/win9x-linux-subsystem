#pragma once
#include "prelude.h"

void wsl9x_start(void);

void wsl9x_irq(enum wsl9x_irq irq);

// called on kernel thread when returning from userland
void wsl9x__user_page_fault(u32 fault_addr);
void wsl9x__user_syscall(void);
