#include <errno.h>
#include <spore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

static int send_command(int argc, char **argv) {
  int fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (fd < 0) {
    perror("myc: socket");
    return EXIT_FAILURE;
  }
  struct sockaddr_un sa;
  memset(&sa, 0, sizeof(sa));
  sa.sun_family = AF_UNIX;
  snprintf(sa.sun_path, sizeof(sa.sun_path), "/run/mycelium.sock");
  if (connect(fd, (struct sockaddr *)&sa, sizeof(sa)) != 0) {
    perror("myc: connect /run/mycelium.sock");
    close(fd);
    return EXIT_FAILURE;
  }

  for (int i = 0; i < argc; ++i) {
    if (i > 0) { (void)write(fd, " ", 1); }
    (void)write(fd, argv[i], strlen(argv[i]));
  }
  (void)write(fd, "\n", 1);

  char buf[512];
  for (;;) {
    ssize_t n = read(fd, buf, sizeof(buf));
    if (n < 0 && errno == EINTR) { continue; }
    if (n <= 0) { break; }
    (void)write(STDOUT_FILENO, buf, (size_t)n);
  }
  close(fd);
  return EXIT_SUCCESS;
}

int main(int argc, char **argv) {
  if (argc < 2 || streq(argv[1], "--help")) {
    puts("usage: myc COMMAND [UNIT]");
    puts("commands: start stop restart reload status show cat list-units list-timers");
    puts("          list-dependencies is-active is-failed daemon-reload logs isolate reboot poweroff");
    return argc < 2 ? EXIT_USAGE : EXIT_SUCCESS;
  }
  return send_command(argc - 1, argv + 1);
}
