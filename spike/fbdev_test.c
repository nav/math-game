/* fbdev_test.c - raw /dev/fb0 + evdev spike.
 *
 * mmaps the framebuffer, fills it, draws one rectangle, and waits for
 * either a keypress on /dev/input/eventN or a ~3s timeout so it exits on
 * its own for scripted testing. Run as root (needed for /dev/fb0 and
 * /dev/input/eventN access on a stock Raspbian install).
 */

#define _DEFAULT_SOURCE
#include <fcntl.h>
#include <linux/fb.h>
#include <linux/input.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <unistd.h>

static long now_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000L + tv.tv_usec / 1000L;
}

int main(void) {
    const char *fb_path = "/dev/fb0";
    int fbfd = open(fb_path, O_RDWR);
    if (fbfd < 0) {
        perror("open fb0");
        return 1;
    }

    struct fb_var_screeninfo vinfo;
    struct fb_fix_screeninfo finfo;
    if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo) < 0) {
        perror("FBIOGET_VSCREENINFO");
        return 1;
    }
    if (ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo) < 0) {
        perror("FBIOGET_FSCREENINFO");
        return 1;
    }

    printf("fb: %dx%d, %dbpp, line_length=%d\n", vinfo.xres, vinfo.yres,
           vinfo.bits_per_pixel, finfo.line_length);

    size_t screensize = (size_t)finfo.line_length * vinfo.yres;
    unsigned char *fbp = mmap(NULL, screensize, PROT_READ | PROT_WRITE,
                               MAP_SHARED, fbfd, 0);
    if (fbp == MAP_FAILED) {
        perror("mmap fb0");
        close(fbfd);
        return 1;
    }

    /* fill screen dark blue */
    uint32_t bg = (0 << 16) | (0 << 8) | 80; /* B */
    for (unsigned y = 0; y < vinfo.yres; y++) {
        uint32_t *row = (uint32_t *)(fbp + y * finfo.line_length);
        for (unsigned x = 0; x < vinfo.xres; x++) {
            row[x] = bg;
        }
    }

    /* draw a bright green rectangle in the middle third of the screen */
    uint32_t fg = (0 << 16) | (200 << 8) | 0;
    unsigned rx0 = vinfo.xres / 3, rx1 = vinfo.xres * 2 / 3;
    unsigned ry0 = vinfo.yres / 3, ry1 = vinfo.yres * 2 / 3;
    for (unsigned y = ry0; y < ry1; y++) {
        uint32_t *row = (uint32_t *)(fbp + y * finfo.line_length);
        for (unsigned x = rx0; x < rx1; x++) {
            row[x] = fg;
        }
    }

    printf("drew background + rectangle, watching for a keypress "
           "(timeout 8s)...\n");
    fflush(stdout);

    /* find a usable evdev keyboard device */
    int evfd = -1;
    for (int i = 0; i < 8; i++) {
        char path[64];
        snprintf(path, sizeof(path), "/dev/input/event%d", i);
        int fd = open(path, O_RDONLY | O_NONBLOCK);
        if (fd < 0) continue;
        unsigned long evbits = 0;
        if (ioctl(fd, EVIOCGBIT(0, sizeof(evbits)), &evbits) >= 0 &&
            (evbits & (1 << EV_KEY))) {
            printf("using input device: %s\n", path);
            evfd = fd;
            break;
        }
        close(fd);
    }

    long deadline = now_ms() + 8000;
    int got_key = 0;
    while (now_ms() < deadline) {
        if (evfd >= 0) {
            struct input_event ev;
            ssize_t n = read(evfd, &ev, sizeof(ev));
            if (n == (ssize_t)sizeof(ev) && ev.type == EV_KEY && ev.value == 1) {
                printf("keypress detected: code=%d\n", ev.code);
                got_key = 1;
                break;
            }
        }
        usleep(20000);
    }

    if (!got_key) {
        printf("no keypress within timeout, exiting anyway\n");
    }

    if (evfd >= 0) close(evfd);
    munmap(fbp, screensize);
    close(fbfd);
    printf("fbdev_test: exited cleanly\n");
    return 0;
}
