#pragma once
#include "prelude.h"

typedef void(*kernel_entry_t)(void);

extern kernel_entry_t kernel_entry;

bool wsl9x_reserve_addresses(void);
bool wsl9x_load_kernel(void);
