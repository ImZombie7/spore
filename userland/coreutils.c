#include <dirent.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>

#ifndef SYS_getdents64
#define SYS_getdents64 61
#endif

struct linux_dirent64 {
    uint64_t d_ino;
    int64_t d_off;
    unsigned short d_reclen;
    unsigned char d_type;
    char d_name[];
};

static const char *base(const char *path) {
    const char *name = path;
    for (const char *p = path; *p != '\0'; ++p) {
        if (*p == '/' && p[1] != '\0') {
            name = p + 1;
        }
    }
    return name;
}

static int cmd_ls(int argc, char **argv) {
    const char *path = argc > 1 ? argv[1] : ".";
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        perror("ls");
        return 1;
    }
    char buf[512];
    long n = syscall(SYS_getdents64, fd, buf, sizeof(buf));
    int first = 1;
    for (long off = 0; off < n;) {
        struct linux_dirent64 *d = (struct linux_dirent64 *)(buf + off);
        printf("%s%s", first ? "" : "  ", d->d_name);
        first = 0;
        off += d->d_reclen;
    }
    printf("\n");
    close(fd);
    return 0;
}

static int cmd_cat(int argc, char **argv) {
    char buf[256];
    for (int i = 1; i < argc; ++i) {
        int fd = open(argv[i], O_RDONLY);
        if (fd < 0) {
            perror("cat");
            return 1;
        }
        for (;;) {
            ssize_t n = read(fd, buf, sizeof(buf));
            if (n <= 0) {
                break;
            }
            write(1, buf, (size_t)n);
        }
        close(fd);
    }
    return 0;
}

static int cmd_echo(int argc, char **argv) {
    for (int i = 1; i < argc; ++i) {
        if (i > 1) {
            putchar(' ');
        }
        fputs(argv[i], stdout);
    }
    putchar('\n');
    return 0;
}

static int cmd_mkdir(int argc, char **argv) {
    int rc = 0;
    for (int i = 1; i < argc; ++i) {
        if (mkdir(argv[i], 0777) != 0) {
            perror("mkdir");
            rc = 1;
        }
    }
    return rc;
}

static int cmd_rm(int argc, char **argv) {
    int rc = 0;
    for (int i = 1; i < argc; ++i) {
        if (unlink(argv[i]) != 0) {
            perror("rm");
            rc = 1;
        }
    }
    return rc;
}

static int cmd_touch(int argc, char **argv) {
    int rc = 0;
    for (int i = 1; i < argc; ++i) {
        int fd = open(argv[i], O_CREAT | O_WRONLY, 0666);
        if (fd < 0) {
            perror("touch");
            rc = 1;
        } else {
            close(fd);
        }
    }
    return rc;
}

static int cmd_pwd(void) {
    char cwd[128];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        return 1;
    }
    puts(cwd);
    return 0;
}

int main(int argc, char **argv) {
    const char *cmd = base(argv[0]);
    if (strcmp(cmd, "ls") == 0) {
        return cmd_ls(argc, argv);
    }
    if (strcmp(cmd, "cat") == 0) {
        return cmd_cat(argc, argv);
    }
    if (strcmp(cmd, "echo") == 0) {
        return cmd_echo(argc, argv);
    }
    if (strcmp(cmd, "mkdir") == 0) {
        return cmd_mkdir(argc, argv);
    }
    if (strcmp(cmd, "rm") == 0) {
        return cmd_rm(argc, argv);
    }
    if (strcmp(cmd, "touch") == 0) {
        return cmd_touch(argc, argv);
    }
    if (strcmp(cmd, "pwd") == 0) {
        return cmd_pwd();
    }
    if (strcmp(cmd, "hello") == 0) {
        puts("hello, world");
        return 0;
    }
    if (strcmp(cmd, "true") == 0) {
        return 0;
    }
    if (strcmp(cmd, "false") == 0) {
        return 1;
    }
    return 127;
}
