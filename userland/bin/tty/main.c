#include <spore.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char **argv) {
  bool silent = argc == 2 && streq(argv[1], "-s");
  if (argc > 2 || (argc == 2 && !silent)) { return usage("tty", "[-s]"); }
  if (isatty(STDIN_FILENO)) {
    if (!silent) { puts("/dev/tty"); }
    return EXIT_SUCCESS;
  }
  if (!silent) { puts("not a tty"); }
  return EXIT_FAILURE;
}
