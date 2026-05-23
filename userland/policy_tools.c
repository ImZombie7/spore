#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

static const char *base(const char *path) {
    const char *name = path;
    for (const char *p = path; *p != '\0'; ++p) {
        if (*p == '/' && p[1] != '\0') {
            name = p + 1;
        }
    }
    return name;
}

int main(int argc, char **argv) {
    const char *cmd = base(argv[0]);
    if (strcmp(cmd, "spinner") == 0) {
        volatile unsigned long x = 0;
        for (;;) {
            ++x;
        }
    }
    if (strcmp(cmd, "peeker") == 0) {
        const char *path = argc > 1 ? argv[1] : "/etc/motd";
        int fd = open(path, O_RDONLY);
        if (fd < 0) {
            printf("peeker: open(%s): Operation not permitted\n", path);
            return errno == EPERM ? 0 : 1;
        }
        close(fd);
        puts("peeker: unexpected open success");
        return 1;
    }
    if (strcmp(cmd, "writer") == 0) {
        const char *path = argc > 1 ? argv[1] : "/tmp/out";
        int fd = open(path, O_CREAT | O_WRONLY | O_TRUNC, 0666);
        if (fd < 0) {
            perror("writer");
            return 1;
        }
        const char msg[] = "hello spore\n";
        ssize_t n = write(fd, msg, sizeof(msg) - 1);
        close(fd);
        printf("writer: wrote %ld bytes\n", (long)n);
        return n == (ssize_t)(sizeof(msg) - 1) ? 0 : 1;
    }
    if (strcmp(cmd, "memhog") == 0) {
        void *p = mmap(NULL, 8 * 1024 * 1024, PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (p == MAP_FAILED) {
            puts("memhog: mmap past cap failed cleanly");
            return 0;
        }
        puts("memhog: unexpected mmap success");
        return 1;
    }
    if (strcmp(cmd, "escalate") == 0) {
        puts("escalate: should not run");
        return 1;
    }
    return 127;
}
