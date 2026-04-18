#include "services.h"
#include "wsl9x.h"

static u32 prev_vterm = -1;

static ssize_t console_put(u32 vtermno, const u8* buf, size_t len)
{
    if (vtermno != prev_vterm) {
        printf("[vterm=%lu]\n", vtermno);
        prev_vterm = vtermno;
    }

    WSL9x_Printk((const char*)buf, len);
    return len;
}

static ssize_t console_get(u32 vtermno, u8* buf, size_t len)
{
    return 0;
}

const struct wsl9x_services wsl9x_services = {
    .console_get = console_get,
    .console_put = console_put,
};
