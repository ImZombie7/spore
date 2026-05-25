#include <spore.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

int main(int argc, char **argv) {
  if (argc > 1 && streq(argv[1], "--help")) { return usage("reboot", ""); }
  if (geteuid() != 0) {
    eprintf("reboot: permission denied; try sudo reboot\n");
    return EXIT_FAILURE;
  }
  int fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (fd < 0) {
    perror("reboot: socket");
    return EXIT_FAILURE;
  }
  struct sockaddr_un sa;
  memset(&sa, 0, sizeof(sa));
  sa.sun_family = AF_UNIX;
  snprintf(sa.sun_path, sizeof(sa.sun_path), "/run/mycelium.sock");
  if (connect(fd, (struct sockaddr *)&sa, sizeof(sa)) != 0) {
    perror("reboot: connect /run/mycelium.sock");
    close(fd);
    return EXIT_FAILURE;
  }
  (void)write(fd, "reboot\n", 7);
  char buf[256];
  for (;;) {
    ssize_t n = read(fd, buf, sizeof(buf));
    if (n <= 0) { break; }
    (void)write(STDOUT_FILENO, buf, (size_t)n);
  }
  close(fd);
  for (;;) {
    pause();
  }
}
