#pragma once
#include "prelude.h"
#include <wsl9x/entry.h>
#include <wsl9x/task.h>

extern const struct wsl9x_services wsl9x_services;

int wsl9x_v86_start_console(HVM vm);
int wsl9x_v86_send_key(HVM vm, u16 keycode);

void wsl9x_v86_schedule_console_output(HVM vm);
void wsl9x_v86__console_output(void);
void wsl9x_v86__write_output(u8 c);

void wsl9x_console_check_sched(void);
