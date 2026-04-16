#pragma once
#include "prelude.h"

// Loader public interface
bool wsl9x_reserve_addresses(void);
bool wsl9x_load_kernel(void);
void wsl9x_start_kernel(void);
