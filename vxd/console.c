#include "console.h"
#include "entry.h"
#include "wsl9x/entry.h"

#include <wsl9x/task.h>

#define MAX_VTERMS 8
#define RINGBUF_SIZE 1024 // must be power of two

static bool need_sched = false;

struct ringbuf {
    size_t write, read;
    u8 buf[RINGBUF_SIZE];
#ifdef DEBUG_RINGBUF
    const char* name;
#endif
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

#ifdef DEBUG_RINGBUF
    printf("ringbuf_put[%s]: write=%lu, read=%lu\n", rb->name, rb->write, rb->read);
    printf("                 size=%lu\n", size);
    printf("                 data='");
#endif

    while (n < size && !ringbuf_full(rb)) {
        char c = data[n++];

#ifdef DEBUG_RINGBUF
        printf("%c", c == '\n' ? '.' : c);
#endif

        rb->buf[rb->write++ % RINGBUF_SIZE] = c;
    }

#ifdef DEBUG_RINGBUF
    printf("'\n");
    printf("                 n=%lu\n", n);
#endif

    return n;
}

static size_t ringbuf_get(struct ringbuf* rb, u8* data, size_t size)
{
    size_t n = 0;

#ifdef DEBUG_RINGBUF
    printf("ringbuf_get[%s]: write=%lu, read=%lu\n", rb->name, rb->write, rb->read);
    printf("                 size=%lu\n", size);
    printf("                 data='");
#endif

    while (n < size && !ringbuf_empty(rb)) {
        char c = rb->buf[rb->read++];

#ifdef DEBUG_RINGBUF
        printf("%c", c == '\n' ? '.' : c);
#endif

        data[n++] = c;
    }

#ifdef DEBUG_RINGBUF
    printf("'\n");
    printf("                 n=%lu\n", n);
#endif

    return n;
}

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

#ifdef DEBUG_RINGBUF
    consoles[vtermno].in.name = "IN";
    consoles[vtermno].out.name = "OU";
#endif

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
    if (!con->owner) {
        return len;
    }

    size_t n = ringbuf_put(&con->out, buf, len);
    // printf("ringbuf_put: n=%lu len=%lu", n, len);
    need_sched = true;
    return n;
}

ssize_t WSL9X_Console_Get(u32 vtermno, u8* buf, size_t len)
{
    struct console* con = get_console(vtermno);
    if (!con) {
        return -1;
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
        if (!ringbuf_empty(&con->out) && !con->scheduled) {
#ifndef DEBUG_CONSOLE
            printf("scheduling console for vm=%08lx\n", (ulong)con->owner);
#endif
            con->scheduled = true;
            wsl9x_v86_schedule_console_output(con->owner);
        }
    }
}

static struct console* find_console(HVM vm)
{
    for (size_t i = 0; i < MAX_VTERMS; i++) {
        if (consoles[i].owner == vm) {;
            return &consoles[i];
        }
    }

    return NULL;
}

int wsl9x_v86_start_console(HVM vm, u8* buffer)
{
    for (size_t i = 0; i < MAX_VTERMS; i++) {
        if (consoles[i].owner == NULL) {
#ifdef DEBUG_RINGBUF
            consoles[i].in.name = "IN";
            consoles[i].out.name = "OU";
#endif
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
#ifndef DEBUG_CONSOLE
    printf("wsl9x_v86_send_key: vm=%08lx keycode=%04x\n", (ulong)vm, keycode);
#endif
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
    wsl9x_irq(WSL9X_IRQ_CONSOLE);
    return 0;
}

void wsl9x_v86__console_output(HVM vm)
{
    struct console* con = find_console(vm);
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
