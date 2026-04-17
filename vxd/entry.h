#pragma once
#include "prelude.h"

bool wsl9x_init_entry(void);

// void wsl9x_notify_thread_switched(HTHREAD previous, HTHREAD current);

// must call on kernel thread:
void wsl9x__start_kernel(void);
void wsl9x__resume_kernel(void);
