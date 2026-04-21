#pragma once
#include "prelude.h"
#include <wsl9x/entry.h>
#include <wsl9x/task.h>

#define WSL9X_V86_API_CLAIM_CONSOLE 0x01
#define WSL9X_V86_API_SEND_KEY      0x02

extern const struct wsl9x_services wsl9x_services;

int wsl9x_v86_start_console(HVM vm, u8* buffer);
int wsl9x_v86_send_key(HVM vm, u16 keycode);

void wsl9x_v86_schedule_console_output(HVM vm);
void wsl9x_v86__console_output(HVM vm);
void wsl9x_v86__write_output(u8 c);

void wsl9x_console_check_sched(void);
