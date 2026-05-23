#include <dirent.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>

#ifndef SYS_getdents64
#define SYS_getdents64 61
#endif

#define SYS_SPORE_SNAPSHOT 0x4000
#define SYS_SPORE_SPAWN 0x4001
#define SYS_SPORE_REAP 0x4002
#define SYS_SPORE_RESIDENT 0x4003
#define SYS_EXIT_GROUP 94

struct linux_dirent64 {
    uint64_t d_ino;
    int64_t d_off;
    unsigned short d_reclen;
    unsigned char d_type;
    char d_name[];
};

static long raw_syscall3(long nr, long a0, long a1, long a2) {
    register long x0 __asm__("x0") = a0;
    register long x1 __asm__("x1") = a1;
    register long x2 __asm__("x2") = a2;
    register long x8 __asm__("x8") = nr;
    __asm__ volatile("svc #0" : "+r"(x0) : "r"(x1), "r"(x2), "r"(x8) : "memory");
    return x0;
}

static void regression_child(uint64_t code) {
    raw_syscall3(SYS_EXIT_GROUP, (long)code, 0, 0);
    for (;;) {
    }
}

static uintptr_t page_down(uintptr_t value) {
    return value & ~(uintptr_t)4095;
}

static int snapshot_regression(void) {
    int snap = (int)raw_syscall3(SYS_SPORE_SNAPSHOT, 0, 0, 0);
    if (snap < 0) {
        return 0;
    }
    int child = (int)raw_syscall3(SYS_SPORE_SPAWN, snap, (long)regression_child, 7);
    if (child < 0) {
        return 0;
    }
    int status = 0;
    int got = (int)raw_syscall3(SYS_SPORE_REAP, child, (long)&status, 0);
    return got == child && ((status >> 8) & 0xff) == 7;
}

int main(void) {
    size_t len = 8 * 1024 * 1024;
    unsigned char *mem = malloc(len);
    if (mem == NULL) {
        printf("[spore] cell 1: malloc 8MB ... failed\n");
        return 1;
    }
    uintptr_t base = page_down((uintptr_t)mem);
    size_t span = len + ((uintptr_t)mem - base);
    long before = raw_syscall3(SYS_SPORE_RESIDENT, (long)base, (long)span, 0);
    for (size_t i = 0; i < len; i += 4096) {
        mem[i] = (unsigned char)(i >> 12);
    }
    long after_touch = raw_syscall3(SYS_SPORE_RESIDENT, (long)base, (long)span, 0);
    printf("[spore] cell 1: malloc 8MB ... resident +%ld pages\n", after_touch - before);
    free(mem);
    long after_free = raw_syscall3(SYS_SPORE_RESIDENT, (long)base, (long)span, 0);
    printf("[spore] cell 1: free      ... resident -%ld pages (returned)\n",
           after_touch - after_free);

    int fd = openat(AT_FDCWD, "/etc/motd", O_RDONLY);
    if (fd < 0) {
        printf("[spore] cell 1: open /etc/motd failed\n");
        return 1;
    }
    struct stat st;
    if (fstat(fd, &st) != 0) {
        printf("[spore] cell 1: fstat /etc/motd failed\n");
        return 1;
    }
    char buf[64];
    ssize_t n = read(fd, buf, sizeof(buf) - 1);
    if (n < 0) {
        printf("[spore] cell 1: read /etc/motd failed\n");
        return 1;
    }
    buf[n] = 0;
    if (n > 0 && buf[n - 1] == '\n') {
        buf[n - 1] = 0;
    }
    printf("[spore] cell 1: open /etc/motd fd=%d size=%ld\n", fd, (long)st.st_size);
    printf("[spore] cell 1: read %ld bytes: \"%s\"\n", (long)n, buf);
    close(fd);

    int dfd = openat(AT_FDCWD, "/", O_RDONLY);
    char dents[256];
    long dent_bytes = syscall(SYS_getdents64, dfd, dents, sizeof(dents));
    printf("[spore] cell 1: listdir / ->");
    for (long off = 0; off < dent_bytes;) {
        struct linux_dirent64 *d = (struct linux_dirent64 *)(dents + off);
        printf(" %s", d->d_name);
        off += d->d_reclen;
    }
    printf("\n");
    close(dfd);

    printf("[spore] v1 regression (snapshot/spawn/reap): %s\n",
           snapshot_regression() ? "PASS" : "FAIL");
    printf("[spore] cell 1: exit(0)\n");
    return 0;
}
