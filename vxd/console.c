#include "console.h"
#include "entry.h"
#include "wsl9x/entry.h"

#include <wsl9x/task.h>

#define MAX_VTERMS 8
#define RINGBUF_SIZE 1024 // must be power of two

struct ringbuf {
    size_t write, read;
    u8 buf[RINGBUF_SIZE];
};

static bool ringbuf_empty(const struct ringbuf* rb)
{
    return rb->write == rb->read;
}

static bool ringbuf_full(const struct ringbuf* rb)
{
    return (rb->write - rb->read) == RINGBUF_SIZE;
}

static size_t ringbuf_put(struct ringbuf* rb, const u8* data, size_t size)
{
    size_t n = 0;

    while (n < size && !ringbuf_full(rb)) {
        rb->buf[rb->write++ % RINGBUF_SIZE] = data[n++];
    }

    return n;
}

static size_t ringbuf_get(struct ringbuf* rb, u8* data, size_t size)
{
    size_t n = 0;

    while (n < size && !ringbuf_empty(rb)) {
        data[n++] = rb->buf[rb->read++];
    }

    return n;
}

struct console {
    HVM owner;
    struct ringbuf out, in;
    bool need_sched;
};

static struct console consoles[MAX_VTERMS];

static struct console* get_console(u32 vtermno)
{
    if (vtermno >= MAX_VTERMS) {
        return NULL;
    }

    return &consoles[vtermno];
}

static ssize_t console_put(u32 vtermno, const u8* buf, size_t len)
{
    // printf("console_put: vtermno=%lu buf=%lx len=%lu\n", vtermno, (ulong)buf, len);

    struct console* con = get_console(vtermno);
    if (!con) {
        return -1;
    }

    if (vtermno == 0) {
        WSL9x_Printk((const char*)buf, len);
    }

    // just discard all data if the console is not attached
    if (!con->owner) {
        return len;
    }

    size_t n = ringbuf_put(&con->out, buf, len);
    // printf("ringbuf_put: n=%lu len=%lu", n, len);
    con->need_sched = true;
    return n;
}

static ssize_t console_get(u32 vtermno, u8* buf, size_t len)
{
    struct console* con = get_console(vtermno);
    if (!con) {
        return -1;
    }

    return ringbuf_get(&con->in, buf, len);
}

void wsl9x_console_check_sched(void)
{
    for (size_t i = 0; i < MAX_VTERMS; i++) {
        struct console* con = &consoles[i];
        if (con->need_sched) {
            con->need_sched = false;
            printf("scheduling console for vm=%08lx\n", (ulong)con->owner);
            wsl9x_v86_schedule_console_output(con->owner);
        }
    }
}


const struct wsl9x_services wsl9x_services = {
    .console_get = console_get,
    .console_put = console_put,
};

static struct console* find_console(HVM vm)
{
    for (size_t i = 0; i < MAX_VTERMS; i++) {
        if (consoles[i].owner == vm) {
            return &consoles[i];
        }
    }

    return NULL;
}

int wsl9x_v86_start_console(HVM vm, u8* buffer)
{
    for (size_t i = 0; i < MAX_VTERMS; i++) {
        if (consoles[i].owner == NULL) {
            consoles[i].owner = vm;
            printf("starting new console on vtermno=%lu\n", i);
            return 0;
        }
    }

    return -1;
}

static const u16 keycode_mask = 0x00ff;
static const u16 extended_mask = 0xff00;

int wsl9x_v86_send_key(HVM vm, u16 keycode)
{
    printf("wsl9x_v86_send_key: vm=%08lx keycode=%04x\n", (ulong)vm, keycode);
    struct console* con = find_console(vm);
    if (!con) {
        return -1;
    }

    if (keycode & extended_mask) {
        printf("extended keycode: %02x\n", keycode & keycode_mask);
        return 0;
    }

    u8 c = keycode & keycode_mask;
    ringbuf_put(&con->in, &c, 1);
    // printf("  ringbug in: write=%lu read=%lu\n", con->in.write, con->in.read);
    wsl9x_schedule_irq(WSL9X_IRQ_CONSOLE);
    printf("  irq scheduled\n");
    return 0;
}

void wsl9x_v86__console_output(HVM vm)
{
    struct console* con = find_console(vm);
    if (!con) {
        return;
    }

    u8 buf[8];
    size_t n = ringbuf_get(&con->out, buf, 8);

    // printf("ringbuf_get: n=%lu, write=%lu, read=%lu\n", n, con->out.write, con->out.read);

    for (size_t i = 0; i < n; i++) {
        wsl9x_v86__write_output(buf[i]);
    }
}

void wsl9x_v86__write_output(u8 c);
