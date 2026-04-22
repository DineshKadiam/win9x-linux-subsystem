#include "prelude.h"
#include <wsl9x.h>

typedef void* wsl9x_service_t;

u32 WSL9X_Get_Version(void)
{
    return 1;
}

void WSL9X_Log_Info(const char* str, size_t len)
{
    static const char prefix[] = "[wsl9x info] ";
    WSL9X_Debug_Print(prefix, sizeof(prefix));
    WSL9X_Debug_Print(str, len);
    WSL9X_Debug_Print("\n", 1);
}

void WSL9X_Log_Warn(const char* str, size_t len)
{
    static const char prefix[] = "[WSL9X WARN] ";
    WSL9X_Debug_Print(prefix, sizeof(prefix));
    WSL9X_Debug_Print(str, len);
    WSL9X_Debug_Print("\n", 1);
}

wsl9x_service_t WSL9X_Service_Table[] = {
    [WSL9X__GET_VERSION] = WSL9X_Get_Version,
    [WSL9X__DEBUG_PRINT] = WSL9X_Debug_Print,
    [WSL9X__DEBUG_PRINTF] = printf,
    [WSL9X__LOG_INFO] = WSL9X_Log_Info,
    [WSL9X__LOG_WARN] = WSL9X_Log_Warn,
    [WSL9X__CONSOLE_PUT] = WSL9X_Console_Put,
    [WSL9X__CONSOLE_GET] = WSL9X_Console_Get,
};

const u32 Num_WSL9X_Services = sizeof(WSL9X_Service_Table) / sizeof(*WSL9X_Service_Table);

