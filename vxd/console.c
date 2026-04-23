#include "prelude.h"
#include "console.h"
#include "entry.h"
#include "vxdwraps.h"
#include "ringbuf.h"
#include <wsl9x/entry.h>
#include <wsl9x/task.h>

#define MAX_VTERMS 8
#define BUFFER_OUT_SIZE 2048
#define BUFFER_IN_SIZE 32

static bool need_sched = false;

struct console {
    HVM owner;
    bool scheduled;
    struct ringbuf out, in;
};

static struct console consoles[MAX_VTERMS];

static struct console* get_console(u32 vtermno)
{
    if (vtermno >= MAX_VTERMS) {
        return NULL;
    }

    return &consoles[vtermno];
}

ssize_t WSL9X_Console_Put(u32 vtermno, const u8* buf, size_t len)
{
    // printf("console_put: vtermno=%lu buf=%lx len=%lu\n", vtermno, (ulong)buf, len);

    struct console* con = get_console(vtermno);
    if (!con) {
        return -1;
    }

    if (vtermno == 0) {
        WSL9X_Debug_Print((const char*)buf, len);
    }

    // just discard all data if the console is not attached
    if (!con->owner || !ringbuf_valid(&con->out)) {
        return len;
    }

    size_t n = ringbuf_put(&con->out, buf, len);
    need_sched = true;
    return n;
}

ssize_t WSL9X_Console_Get(u32 vtermno, u8* buf, size_t len)
{
    struct console* con = get_console(vtermno);
    if (!con) {
        // no such device
        return 0;
    }

    if (!ringbuf_valid(&con->in)) {
        // not attached
        return 0;
    }

    return ringbuf_get(&con->in, buf, len);
}

void wsl9x_console_check_sched(void)
{
    if (!need_sched) {
        return;
    }

    need_sched = false;

    for (size_t i = 0; i < MAX_VTERMS; i++) {
        struct console* con = &consoles[i];

        if (!con->owner || !ringbuf_valid(&con->out)) {
            continue;
        }

        if (!ringbuf_empty(&con->out) && !con->scheduled) {
            printf("scheduling console for vm=%08lx\n", (ulong)con->owner);
            con->scheduled = true;
            wsl9x_v86_schedule_console_output(con->owner);
        }
    }
}

static struct console* find_console(HVM vm, size_t* num)
{
    for (size_t i = 0; i < MAX_VTERMS; i++) {
        if (consoles[i].owner == vm) {
            if (num) {
                *num = i;
            }

            return &consoles[i];
        }
    }

    return NULL;
}

int wsl9x_v86_start_console(HVM vm)
{
    // find free console
    size_t i;
    struct console* con = find_console(NULL, &i);
    if (!con) {
        goto fail;
    }

    if (!ringbuf_init(&con->in, BUFFER_IN_SIZE)) {
        goto fail;
    }

    if (!ringbuf_init(&con->out, BUFFER_OUT_SIZE)) {
        goto fail;
    }

    printf("starting new console on vtermno=%lu\n", i);
    con->owner = vm;
    return 0;

fail:
    printf("cannot allocate console\n");
    return -1;
}

static const u16 keycode_mask = 0x00ff;
static const u16 extended_mask = 0xff00;

int wsl9x_v86_send_key(HVM vm, u16 keycode)
{
#ifndef DEBUG_CONSOLE
    printf("wsl9x_v86_send_key: vm=%08lx keycode=%04x\n", (ulong)vm, keycode);
#endif
    struct console* con = find_console(vm, 0);
    if (!con) {
        return -1;
    }

    if (keycode & extended_mask) {
        printf("extended keycode: %02x\n", keycode & keycode_mask);
        return 0;
    }

    u8 c = keycode & keycode_mask;
    ringbuf_put(&con->in, &c, 1);
    wsl9x_irq(WSL9X_IRQ_CONSOLE);
    return 0;
}

void wsl9x_v86__console_output(void)
{
    HVM vm = VMM_Get_Cur_VM_Handle();
    struct console* con = find_console(vm, 0);
    if (!con) {
        return;
    }

    u8 buf[1];
    size_t n;
    while ((n = ringbuf_get(&con->out, buf, sizeof(buf)))) {
        for (size_t i = 0; i < n; i++) {
            wsl9x_v86__write_output(buf[i]);
        }
    }

    con->scheduled = false;
}

void wsl9x_v86__write_output(u8 c);
