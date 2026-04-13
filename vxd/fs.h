#pragma once
#include "prelude.h"

typedef uint16_t FileHandle;

FileHandle dos_open(const char* path);
bool dos_close(FileHandle handle);
ssize_t dos_seek(FileHandle handle, u32 offset);
ssize_t dos_read(FileHandle handle, u16 buff_seg, u16 len);
