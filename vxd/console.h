#pragma once
#include "prelude.h"
#include <wsl9x/entry.h>
#include <wsl9x/task.h>

enum wsl9x_v86_func {
    WSL9X_V86_API_START_CONSOLE = 1,
    WSL9X_V86_API_SEND_KEY = 2,
};

int wsl9x_v86_start_console(HVM vm);
int wsl9x_v86_send_key(HVM vm, u16 keycode);

void wsl9x_schedule_console_output(HVM vm);
void wsl9x_console_check_sched(void);
