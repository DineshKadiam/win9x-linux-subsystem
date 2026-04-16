#pragma once
#include "prelude.h"
#include <wsl9x/entry.h>

extern wsl9x_entry_func_t kernel_entry;

bool wsl9x_reserve_addresses(void);
bool wsl9x_load_kernel(void);
