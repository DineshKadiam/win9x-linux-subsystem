#pragma once
#include "prelude.h"

u32 VMM_Allocate_Temp_V86_Data_Area(u32 nbytes, u32 flags);
void VMM_Free_Temp_V86_Data_Area(void);
