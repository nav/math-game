/* platform_fbdev.c - Raspberry Pi backend: raw /dev/fb0 mmap + evdev.
 *
 * This board has no working KMS/DRM and no X server (confirmed on-device),
 * so this talks to the legacy Linux framebuffer directly instead of going
 * through SDL2 or any display server.
 */
#define _DEFAULT_SOURCE
#include <fcntl.h>
#include <linux/fb.h>
#include <linux/input.h>
#include <linux/kd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <unistd.h>

#include "platform.h"

static int g_fbfd = -1;
static unsigned char *g_fbp = NULL;
static unsigned char *g_backbuf = NULL; /* drawn into; blitted to g_fbp on present */
static size_t g_screensize = 0;
static struct fb_var_screeninfo g_vinfo;
static struct fb_fix_screeninfo g_finfo;
static int g_evfd = -1;
static int g_vtfd = -1; /* tty1: switched to KD_GRAPHICS so the kernel's
                          * own text console stops drawing over our
                          * framebuffer writes while the game runs. */

/* If the game is killed (Ctrl+C, systemd stop, etc.) without reaching
 * plat_shutdown(), tty1 would otherwise stay stuck in KD_GRAPHICS with a
 * blank/frozen console. Restore it directly from the signal handler. */
static void restore_console_and_exit(int sig) {
    (void)sig;
    if (g_vtfd >= 0) ioctl(g_vtfd, KDSETMODE, KD_TEXT);
    _exit(1);
}

static int find_keyboard_device(void) {
    for (int i = 0; i < 16; i++) {
        char path[64];
        snprintf(path, sizeof(path), "/dev/input/event%d", i);
        int fd = open(path, O_RDONLY | O_NONBLOCK);
        if (fd < 0) continue;
        unsigned long evbits = 0;
        if (ioctl(fd, EVIOCGBIT(0, sizeof(evbits)), &evbits) >= 0 &&
            (evbits & (1 << EV_KEY))) {
            return fd;
        }
        close(fd);
    }
    return -1;
}

int plat_init(void) {
    g_fbfd = open("/dev/fb0", O_RDWR);
    if (g_fbfd < 0) {
        perror("open /dev/fb0");
        return -1;
    }
    if (ioctl(g_fbfd, FBIOGET_VSCREENINFO, &g_vinfo) < 0 ||
        ioctl(g_fbfd, FBIOGET_FSCREENINFO, &g_finfo) < 0) {
        perror("fb ioctl");
        return -1;
    }
    if (g_vinfo.bits_per_pixel != 32) {
        fprintf(stderr,
                "plat_init: only 32bpp framebuffers are supported, got %dbpp\n",
                g_vinfo.bits_per_pixel);
        return -1;
    }
    g_screensize = (size_t)g_finfo.line_length * g_vinfo.yres;
    g_fbp = mmap(NULL, g_screensize, PROT_READ | PROT_WRITE, MAP_SHARED,
                 g_fbfd, 0);
    if (g_fbp == MAP_FAILED) {
        perror("mmap /dev/fb0");
        g_fbp = NULL;
        return -1;
    }

    g_backbuf = malloc(g_screensize);
    if (!g_backbuf) {
        fprintf(stderr, "plat_init: failed to allocate backbuffer\n");
        return -1;
    }

    g_evfd = find_keyboard_device();
    if (g_evfd < 0) {
        fprintf(stderr, "plat_init: no keyboard input device found\n");
        return -1;
    }

    g_vtfd = open("/dev/tty1", O_RDWR);
    if (g_vtfd < 0) {
        perror("open /dev/tty1");
        return -1;
    }
    if (ioctl(g_vtfd, KDSETMODE, KD_GRAPHICS) < 0) {
        perror("KDSETMODE KD_GRAPHICS");
        return -1;
    }
    signal(SIGINT, restore_console_and_exit);
    signal(SIGTERM, restore_console_and_exit);

    return 0;
}

void plat_shutdown(void) {
    if (g_vtfd >= 0) {
        ioctl(g_vtfd, KDSETMODE, KD_TEXT);
        close(g_vtfd);
    }
    if (g_evfd >= 0) close(g_evfd);
    free(g_backbuf);
    if (g_fbp) munmap(g_fbp, g_screensize);
    if (g_fbfd >= 0) close(g_fbfd);
}

void plat_get_screen_size(int *w, int *h) {
    *w = (int)g_vinfo.xres;
    *h = (int)g_vinfo.yres;
}

void plat_fill_rect_px(int x, int y, int w, int h, Color c) {
    int x0 = x < 0 ? 0 : x;
    int y0 = y < 0 ? 0 : y;
    int x1 = x + w;
    int y1 = y + h;
    if (x1 > (int)g_vinfo.xres) x1 = g_vinfo.xres;
    if (y1 > (int)g_vinfo.yres) y1 = g_vinfo.yres;
    if (x0 >= x1 || y0 >= y1) return;

    uint32_t pixel = ((uint32_t)c.r << g_vinfo.red.offset) |
                      ((uint32_t)c.g << g_vinfo.green.offset) |
                      ((uint32_t)c.b << g_vinfo.blue.offset);

    for (int py = y0; py < y1; py++) {
        uint32_t *row =
            (uint32_t *)(g_backbuf + (size_t)py * g_finfo.line_length);
        for (int px = x0; px < x1; px++) {
            row[px] = pixel;
        }
    }
}

void plat_blit_alpha(int x, int y, int w, int h, const unsigned char *alpha,
                      Color c) {
    for (int j = 0; j < h; j++) {
        int py = y + j;
        if (py < 0 || py >= (int)g_vinfo.yres) continue;
        uint32_t *row =
            (uint32_t *)(g_backbuf + (size_t)py * g_finfo.line_length);
        for (int i = 0; i < w; i++) {
            int px = x + i;
            if (px < 0 || px >= (int)g_vinfo.xres) continue;
            unsigned a = alpha[j * w + i];
            if (a == 0) continue;
            uint32_t existing = row[px];
            unsigned br = (existing >> g_vinfo.red.offset) & 0xFF;
            unsigned bgv = (existing >> g_vinfo.green.offset) & 0xFF;
            unsigned bb = (existing >> g_vinfo.blue.offset) & 0xFF;
            unsigned nr = (c.r * a + br * (255 - a)) / 255;
            unsigned ng = (c.g * a + bgv * (255 - a)) / 255;
            unsigned nb = (c.b * a + bb * (255 - a)) / 255;
            row[px] = ((uint32_t)nr << g_vinfo.red.offset) |
                      ((uint32_t)ng << g_vinfo.green.offset) |
                      ((uint32_t)nb << g_vinfo.blue.offset);
        }
    }
}

void plat_present(void) {
    /* One shot into the live framebuffer instead of many small writes
     * visible mid-draw, since this display has no hardware page-flip. */
    memcpy(g_fbp, g_backbuf, g_screensize);
}

Key plat_poll_key(void) {
    struct input_event ev;
    ssize_t n = read(g_evfd, &ev, sizeof(ev));
    if (n != (ssize_t)sizeof(ev)) return GK_NONE;
    if (ev.type != EV_KEY || ev.value != 1) return GK_NONE;

    switch (ev.code) {
        case KEY_0: return GK_0;
        case KEY_1: return GK_1;
        case KEY_2: return GK_2;
        case KEY_3: return GK_3;
        case KEY_4: return GK_4;
        case KEY_5: return GK_5;
        case KEY_6: return GK_6;
        case KEY_7: return GK_7;
        case KEY_8: return GK_8;
        case KEY_9: return GK_9;
        case KEY_ENTER: return GK_ENTER;
        case KEY_BACKSPACE: return GK_BACKSPACE;
        case KEY_ESC: return GK_ESCAPE;
        default: return GK_NONE;
    }
}

long plat_time_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000L + tv.tv_usec / 1000L;
}
