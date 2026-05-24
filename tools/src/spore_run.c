#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

enum {
  BUF_CAP = 262144,
  PATH_CAP = 1024,
};

static const char *shell_commands[] = {
  "ls /bin\n",
  "cat /etc/motd\n",
  "echo hi > /tmp/f\n",
  "cat /tmp/f\n",
  "mkdir /tmp/d && cd /tmp/d && touch x && ls\n",
  "/bin/hello\n",
  "pthread-demo\n",
  "udp-echo 10.0.2.2 5555 hi\n",
  "confine net:none udp-send 10.0.2.2 5555 hi\n",
  "confine net:udp:10.0.2.2:5555 udp-send 10.0.2.2 5555 hi\n",
  "confine compute-only /demos/spinner\n",
  "confine fs:/tmp /demos/peeker /etc/motd\n",
  "confine fs:/tmp /demos/writer /tmp/d/out\n",
  "confine mem:1 /demos/memhog\n",
  "runc bad-manifest /demos/escalate\n",
  "exit\n",
};

static void usage(void) {
  fputs("usage: spore-run [--mode plain|filter|shell|stdin] --image IMAGE [--qemu QEMU] [--accel ACCEL] [--cpu CPU]\n",
        stderr);
  exit(2);
}

static bool contains(const char *haystack, const char *needle) {
  return strstr(haystack, needle) != NULL;
}

static bool find_firmware(const char *qemu, char *out, size_t cap) {
  char cmd[PATH_CAP];
  snprintf(cmd, sizeof(cmd), "%s -L help 2>/dev/null", qemu);
  FILE *pipe = popen(cmd, "r");
  if (pipe != NULL) {
    char dir[PATH_CAP];
    while (fscanf(pipe, "%1023s", dir) == 1) {
      snprintf(out, cap, "%s/edk2-aarch64-code.fd", dir);
      if (access(out, R_OK) == 0) {
        pclose(pipe);
        return true;
      }
      char *suffix = strstr(dir, "-firmware");
      if (suffix != NULL) { *suffix = '\0'; }
      snprintf(out, cap, "%s/qemu/edk2-aarch64-code.fd", dir);
      if (access(out, R_OK) == 0) {
        pclose(pipe);
        return true;
      }
    }
    pclose(pipe);
  }
  snprintf(out, cap, "/opt/homebrew/opt/qemu/share/qemu/edk2-aarch64-code.fd");
  return access(out, R_OK) == 0;
}

static void build_qemu_args(char **argv, int *argc, const char *qemu, const char *image, const char *accel,
                            const char *cpu, char *drive_arg, size_t drive_cap) {
  char firmware[PATH_CAP];
  if (!find_firmware(qemu, firmware, sizeof(firmware))) {
    fputs("spore-run: edk2-aarch64-code.fd not found\n", stderr);
    exit(1);
  }
  snprintf(drive_arg, drive_cap, "if=pflash,format=raw,readonly=on,file=%s", firmware);
  int i = 0;
  argv[i++] = (char *)qemu;
  argv[i++] = "-M";
  argv[i++] = "virt,gic-version=3";
  argv[i++] = "-accel";
  argv[i++] = (char *)accel;
  argv[i++] = "-cpu";
  argv[i++] = (char *)cpu;
  argv[i++] = "-m";
  argv[i++] = "512M";
  argv[i++] = "-global";
  argv[i++] = "virtio-mmio.force-legacy=false";
  argv[i++] = "-netdev";
  argv[i++] = "user,id=sporenet";
  argv[i++] = "-device";
  argv[i++] = "virtio-net-device,netdev=sporenet,mac=52:54:00:12:34:56";
  argv[i++] = "-boot";
  argv[i++] = "order=d,menu=off,strict=on";
  argv[i++] = "-drive";
  argv[i++] = drive_arg;
  argv[i++] = "-cdrom";
  argv[i++] = (char *)image;
  argv[i++] = "-serial";
  argv[i++] = "stdio";
  argv[i++] = "-display";
  argv[i++] = "none";
  argv[i] = NULL;
  *argc = i;
}

static int run_plain(char **qemu_argv) {
  execvp(qemu_argv[0], qemu_argv);
  perror(qemu_argv[0]);
  return 127;
}

static double now_seconds(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (double)ts.tv_sec + (double)ts.tv_nsec / 1000000000.0;
}

static void append(char *buf, size_t *len, const char *data, size_t n) {
  if (*len + n >= BUF_CAP) {
    size_t drop = (*len + n) - (BUF_CAP - 1);
    if (drop > *len) { drop = *len; }
    memmove(buf, buf + drop, *len - drop);
    *len -= drop;
  }
  memcpy(buf + *len, data, n);
  *len += n;
  buf[*len] = '\0';
}

static void print_filtered(const char *buf) {
  const char *p = buf;
  bool printed = false;
  while (*p != '\0') {
    const char *line_end = strchr(p, '\n');
    size_t len = line_end == NULL ? strlen(p) : (size_t)(line_end - p);
    char line[4096];
    size_t copy_len = len < sizeof(line) - 1 ? len : sizeof(line) - 1;
    memcpy(line, p, copy_len);
    line[copy_len] = '\0';
    if (strstr(line, "[spore]") != NULL || strstr(line, "[cell ") != NULL ||
        strstr(line, "[kernel] lower sync fault") != NULL) {
      fwrite(p, 1, len, stdout);
      fputc('\n', stdout);
      printed = true;
    }
    if (line_end == NULL) { break; }
    p = line_end + 1;
  }
  if (!printed) { fputs(buf, stdout); }
}

static int run_harness(char **qemu_argv, const char *mode) {
  int in_pipe[2];
  int out_pipe[2];
  if (pipe(in_pipe) != 0 || pipe(out_pipe) != 0) {
    perror("pipe");
    return 1;
  }
  pid_t pid = fork();
  if (pid < 0) {
    perror("fork");
    return 1;
  }
  if (pid == 0) {
    dup2(in_pipe[0], STDIN_FILENO);
    dup2(out_pipe[1], STDOUT_FILENO);
    dup2(out_pipe[1], STDERR_FILENO);
    close(in_pipe[0]);
    close(in_pipe[1]);
    close(out_pipe[0]);
    close(out_pipe[1]);
    execvp(qemu_argv[0], qemu_argv);
    perror(qemu_argv[0]);
    _exit(127);
  }
  close(in_pipe[0]);
  close(out_pipe[1]);

  char buf[BUF_CAP] = {0};
  size_t len = 0;
  size_t sent = 0;
  bool stdin_sent = false;
  double deadline = now_seconds() + (strcmp(mode, "shell") == 0 ? 75.0 : 30.0);
  for (;;) {
    int status = 0;
    if (waitpid(pid, &status, WNOHANG) == pid) {
      char chunk[4096];
      ssize_t n;
      while ((n = read(out_pipe[0], chunk, sizeof(chunk))) > 0) {
        append(buf, &len, chunk, (size_t)n);
      }
      if (strcmp(mode, "filter") == 0 || strcmp(mode, "stdin") == 0) {
        print_filtered(buf);
      } else if (strcmp(mode, "shell") != 0) {
        fputs(buf, stdout);
      }
      return WIFEXITED(status) ? WEXITSTATUS(status) : 128;
    }
    if (now_seconds() > deadline) {
      kill(pid, SIGTERM);
      waitpid(pid, NULL, 0);
      return 124;
    }
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(out_pipe[0], &rfds);
    struct timeval tv = {.tv_sec = 0, .tv_usec = 50000};
    if (select(out_pipe[0] + 1, &rfds, NULL, NULL, &tv) > 0 && FD_ISSET(out_pipe[0], &rfds)) {
      char chunk[1024];
      ssize_t n = read(out_pipe[0], chunk, sizeof(chunk));
      if (n > 0) {
        if (strcmp(mode, "shell") == 0) {
          fwrite(chunk, 1, (size_t)n, stdout);
          fflush(stdout);
        }
        append(buf, &len, chunk, (size_t)n);
        if (strcmp(mode, "stdin") == 0 && !stdin_sent &&
            contains(buf, "[spore] stdin demo: child blocking on read(0)")) {
          write(in_pipe[1], "z\n", 2);
          stdin_sent = true;
        }
        if (strcmp(mode, "shell") == 0 && sent < sizeof(shell_commands) / sizeof(shell_commands[0]) &&
            contains(buf, " $ ")) {
          write(in_pipe[1], shell_commands[sent], strlen(shell_commands[sent]));
          ++sent;
          buf[0] = '\0';
          len = 0;
        }
      }
    }
  }
}

int main(int argc, char **argv) {
  const char *mode = "filter";
  const char *image = NULL;
  const char *qemu = "qemu-system-aarch64";
  const char *accel = "hvf";
  const char *cpu = "host";
  for (int i = 1; i < argc; ++i) {
    if (strcmp(argv[i], "--mode") == 0 && i + 1 < argc) {
      mode = argv[++i];
    } else if (strcmp(argv[i], "--image") == 0 && i + 1 < argc) {
      image = argv[++i];
    } else if (strcmp(argv[i], "--qemu") == 0 && i + 1 < argc) {
      qemu = argv[++i];
    } else if (strcmp(argv[i], "--accel") == 0 && i + 1 < argc) {
      accel = argv[++i];
    } else if (strcmp(argv[i], "--cpu") == 0 && i + 1 < argc) {
      cpu = argv[++i];
    } else {
      usage();
    }
  }
  if (image == NULL) { usage(); }
  char drive_arg[PATH_CAP];
  char *qemu_argv[64];
  int qemu_argc = 0;
  build_qemu_args(qemu_argv, &qemu_argc, qemu, image, accel, cpu, drive_arg, sizeof(drive_arg));
  (void)qemu_argc;
  if (strcmp(mode, "plain") == 0) { return run_plain(qemu_argv); }
  if (strcmp(mode, "filter") == 0 || strcmp(mode, "shell") == 0 || strcmp(mode, "stdin") == 0) {
    return run_harness(qemu_argv, mode);
  }
  usage();
  return 2;
}
